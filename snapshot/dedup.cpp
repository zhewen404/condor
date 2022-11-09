#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <math.h>

struct Line {
   uint16_t dedup_hash;
   unsigned dedup_size;
   u_int16_t * segs2;
   bool unique;
   bool cachedAbove;
   bool xored;
} line;  

u_int8_t * read_file(unsigned *size, FILE * file) //Don't forget to free retval after use
{
   *size = 0;
   unsigned int val;
   int startpos = ftell(file);
   while (fscanf(file, "%d ", &val) == 1)
   {
      *size += 1;
   }
   // printf("%d\n", *size);
   u_int8_t * retval = (unsigned char *) malloc(*size);
   fseek(file, startpos, SEEK_SET); //if the file was not on the beginning when we started
   int pos = 0;
   while (fscanf(file, "%d ", &val) == 1)
   {
      retval[pos++] = (unsigned char) val;
   }
   return retval;
}

uint64_t * read_tag(unsigned *size, FILE * file) //Don't forget to free retval after use
{
   *size = 0;
   uint64_t val;
   int startpos = ftell(file);
   while (fscanf(file, "%lx ", &val) == 1)
   {
      *size += 1;
   }
   // printf("%d\n", *size);
   uint64_t * retval = (uint64_t *) malloc(sizeof(uint64_t)*(*size));
   fseek(file, startpos, SEEK_SET); //if the file was not on the beginning when we started
   int pos = 0;
   while (fscanf(file, "%lx ", &val) == 1)
   {
      retval[pos++] = (uint64_t) val;
   }
   return retval;
}

void init_line_data(struct Line* line_array, unsigned lineSize, u_int8_t * p, unsigned numLine) {
   // printf("calling %s\n", __func__);
   // for every line
   for(unsigned i = 0; i < numLine; i++) {
      line_array[i].dedup_hash = 0;
      line_array[i].dedup_size = lineSize;
      line_array[i].unique = false;

      //create segments
      unsigned base=2;
      line_array[i].segs2 = (uint16_t *) malloc(lineSize/base * sizeof(uint16_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int16_t p_seg = 0; 
         for (unsigned j = 0; j < base-1; j++) {
            p_seg += p[i*lineSize + js *base + j];
            p_seg = p_seg << 8;
         }
         p_seg += p[i*lineSize + js * base + base - 1];
         line_array[i].segs2[js] = p_seg;
         // printf("%" PRIu16 "\n", line_array[i].segs2[js]);
      }
      
      // set dedup hash
      unsigned num = lineSize/base;
      uint16_t * segs2_temp = (uint16_t *) malloc(num/2 * sizeof(uint16_t));
      for (unsigned k=0; k<(int)num/2; k++) {
        // printf("k=%d,k+...=%d\n", k, k+(int)(num/2));
        segs2_temp[k] = line_array[i].segs2[k] ^ line_array[i].segs2[k+(int)(num/2)];
      }

      for (unsigned fold = 1; fold < 5; fold++) {
        // printf("fold=%d\n", fold);
        for (unsigned k=0; k<(int)num/2/pow(2,fold); k++) {
            // printf("k=%d,k+...=%d\n", k, k+(int)(num/2/pow(2,fold)));
            segs2_temp[k] = segs2_temp[k] ^ segs2_temp[k+(int)(num/2/pow(2,fold))];
        }
      }
      line_array[i].dedup_hash = segs2_temp[0];
      free(segs2_temp);
    //   printf("hash=%d\n", line_array[i].dedup_hash);
   }

}
void dedup(struct Line* line_array, unsigned lineSize, u_int8_t * p, unsigned numLine) {
   // for every line
   unsigned base=2;
   unsigned num = lineSize/base;
   for(unsigned i = 0; i < numLine; i++) {
      // try compression
      // check every previous line
      for (unsigned j = 0; j < i; j++) {
        bool equal = true;
        //check equal
        for (unsigned js = 0; js < num; js++) {
            if (line_array[i].segs2[js] != line_array[j].segs2[js]) {
                // i j not equal
                equal = false;
                break;
            }
        }
        if (equal == true) {
            line_array[i].dedup_size = 0;
            line_array[j].unique = true;
            break;
        }
        else {
            line_array[i].dedup_size = lineSize;
        }
      }
      
   }
}

unsigned countSetBits(uint16_t n)
{
   unsigned count = 0;
   while (n != 0) {
      count += n & 1;
      n = n >> 1;
   }
   return count;
}

void xor_preprocess(struct Line* line_array, unsigned lineSize, 
   u_int8_t * p, unsigned numLine, 
   uint64_t * p_l1i_tag, unsigned numLine_l1i,
   uint64_t * p_l1d_tag, unsigned numLine_l1d,
   uint64_t * p_l2_tag, unsigned do_xor) {
   // printf("calling %s\n", __func__);
   // first pass: for every line
   // 1. find if exists in L1
   unsigned ct = 0;
   for(unsigned i = 0; i < numLine; i++) {
      line_array[i].cachedAbove = false;
      line_array[i].xored = false;
      // read l2 tag
      uint64_t l2_addr = p_l2_tag[i];
      // search in l1i
      for (unsigned l1i = 0; l1i < numLine_l1i; l1i++) {
         if(l2_addr == p_l1i_tag[l1i]) {
            line_array[i].cachedAbove = true;
            ct++;
            // printf("l2 line is cached in l1i!\n");
            break;
         }
      }
      // search in l1d
      if (!line_array[i].cachedAbove) {
         for (unsigned l1d = 0; l1d < numLine_l1d; l1d++) {
            if(l2_addr == p_l1d_tag[l1d]) {
               line_array[i].cachedAbove = true;
               ct++;
               // printf("l2 line is cached in l1d!\n");
               break;
            }
         }
      }
   }
   printf("ct=%d\n", ct);
   //cached above lines are too many!
   assert(ct <= numLine/2);

   // second pass: for every line cached above
   // 1. xor with another block and modify data
   for(unsigned i = 0; i < numLine; i++) {
      // perform xor
      if (line_array[i].cachedAbove) {
         unsigned ind;
         if (do_xor == 1) {
            // rand xor
            // printf("using rand xor\n");
            do {
               ind = rand() % numLine;
            } while(ind != i && 
               line_array[ind].cachedAbove == false && 
               line_array[ind].xored == false);
            line_array[ind].xored = true;
            line_array[i].xored = true;
            // printf("[%d] ^ [%d]\n", i, ind);

            for (unsigned js = 0; js < lineSize/2; js++) {
               uint16_t temp2 = line_array[ind].segs2[js] ^ line_array[i].segs2[js];
               line_array[ind].segs2[js] = temp2;
               line_array[i].segs2[js] = temp2;
            }
            
         }
         else if (do_xor == 2) {
            // printf("using ideal xor\n");
            // ideal xor
            unsigned min_ham = lineSize*8 + 1;
            unsigned best_cand_ind = numLine + 1;
            for (unsigned ind = 0; ind < numLine; ind++) {
               if (line_array[ind].cachedAbove == true ||
                  line_array[ind].xored == true) continue;
               unsigned ham = 0;
               for (unsigned js = 0; js < lineSize/2; js++) {
                  uint16_t temp2 = line_array[ind].segs2[js] ^ line_array[i].segs2[js];
                  ham += countSetBits(temp2);
                  // printf("temp2=%" PRIu16 " ham=%d\n", temp2, ham);
               }
               // printf("i=%d, ham[%d]=%d\n", i, ind, ham);
               // exit(1);
               if (ham < min_ham) {
                  min_ham = ham;
                  best_cand_ind = ind;
               }
            }
            // printf("i=%d,min ham[%d]=%d\n", i, best_cand_ind, min_ham);
            
            for (unsigned js = 0; js < lineSize/2; js++) {
               uint16_t temp2 = line_array[best_cand_ind].segs2[js] ^ line_array[i].segs2[js];
               line_array[best_cand_ind].segs2[js] = temp2;
               line_array[i].segs2[js] = temp2;
            }
            line_array[best_cand_ind].xored = true;
            line_array[i].xored = true;
         }
         else {
            assert(false);
         }
      }
   }
}

int main(int argc, char *argv[]) {
   if (argc < 3) {
      printf("./dedup dir do_xor\n");
      return 1;
   }

   unsigned do_xor;
   if (!strcmp(argv[2], "rand")) {
      do_xor = 1;
   }
   else if (!strcmp(argv[2], "ideal")) {
      do_xor = 2;
   }
   else if (!strcmp(argv[2], "none")) {
      do_xor = 0;
   }
   else {
      printf("unknow doxor option: %s\n", argv[2]);
      return 1;
   }

   char filename[256];
   char filename_l1i[256];
   char filename_l1d[256];
   char filename_l2[256];
   char suf[] = "/board.cache_hierarchy.l2cache.tags.cache";
   char suf_tag_l1i[] = "/board.cache_hierarchy.l1icaches.tags.addr";
   char suf_tag_l1d[] = "/board.cache_hierarchy.l1dcaches.tags.addr";
   char suf_tag_l2[] = "/board.cache_hierarchy.l2cache.tags.addr";
   snprintf(filename, sizeof(filename), "%s%s", argv[1], suf);
   snprintf(filename_l1i, sizeof(filename_l1i), "%s%s", argv[1], suf_tag_l1i);
   snprintf(filename_l1d, sizeof(filename_l1d), "%s%s", argv[1], suf_tag_l1d);
   snprintf(filename_l2, sizeof(filename_l2), "%s%s", argv[1], suf_tag_l2);
   // printf("%s\n", filename);
   
   // process input file
   FILE *fp;
   fp = fopen(filename, "r");
   if (fp == NULL) {
      perror("fopen Failed: ");
      return 1;
   }

   unsigned size = 0;
   u_int8_t * p = read_file(&size, fp);
   unsigned numByte = size/sizeof(p[0]);
   unsigned lineSize = 64;
   unsigned numLine = numByte/lineSize;
   printf("numLine=%d, lineSize=%d\n", numLine, lineSize);
   fclose(fp);
   if (numLine == 0) {
      printf("no valid line in L2!\n");
      return 1;
   }
   struct Line* line_array = (Line*) malloc(numLine * sizeof(struct Line));
   
   uint64_t * p_l2_tag;
   uint64_t * p_l1i_tag;
   uint64_t * p_l1d_tag;
   if (do_xor == 1 || do_xor == 2) {
      // read l2 tag
      FILE *fp_l2_tag;
      fp_l2_tag = fopen(filename_l2, "r");
      if (fp_l2_tag == NULL) {
         perror("fopen l2 tag Failed: ");
         return 1;
      }
      unsigned size_l2_tag = 0;
      p_l2_tag = read_tag(&size_l2_tag, fp_l2_tag);
      fclose(fp_l2_tag);
      if (size_l2_tag == 0) {
         printf("no valid addr in L2!\n");
         return 1;
      }

      // read l1d tag
      FILE *fp_l1d_tag;
      fp_l1d_tag = fopen(filename_l1d, "r");
      if (fp_l1d_tag == NULL) {
         perror("fopen l1d tag Failed: ");
         return 1;
      }
      unsigned size_l1d_tag = 0;
      p_l1d_tag = read_tag(&size_l1d_tag, fp_l1d_tag);
      fclose(fp_l1d_tag);
      if (size_l1d_tag == 0) {
         printf("no valid addr in L1d!\n");
         return 1;
      }
      // read l1i tag
      FILE *fp_l1i_tag;
      fp_l1i_tag = fopen(filename_l1i, "r");
      if (fp_l1i_tag == NULL) {
         perror("fopen l1i tag Failed: ");
         return 1;
      }
      unsigned size_l1i_tag = 0;
      p_l1i_tag = read_tag(&size_l1i_tag, fp_l1i_tag);
      fclose(fp_l1i_tag);
      if (size_l1i_tag == 0) {
         printf("no valid addr in L1i!\n");
         return 1;
      }

      init_line_data(line_array, lineSize, p, numLine);
      // xor preprocess
      xor_preprocess(line_array, lineSize, p, numLine, 
         p_l1i_tag, size_l1i_tag, p_l1d_tag, size_l1d_tag, p_l2_tag,
         do_xor);
      // dedup compression
      dedup(line_array, lineSize, p, numLine);
   }
   else {
      // no need to read tag files
      init_line_data(line_array, lineSize, p, numLine);
      dedup(line_array, lineSize, p, numLine);
   }
   
   // for every line:print
   unsigned tot_size = 0;
   for(unsigned i = 0; i < numLine; i++) {
      tot_size += line_array[i].dedup_size;
   }
   double cr = (double)(numLine*lineSize)/tot_size;
   printf("total size = %d/%d (%f)\n", tot_size, numLine*lineSize, cr);

   // free up mem
   free(p);
   if (do_xor == 1 || do_xor == 2) {
      free(p_l2_tag);
      free(p_l1i_tag);
      free(p_l1d_tag);
   }
   for (int i=0; i < numLine; i++) {
      free(line_array[i].segs2);
   }
   free(line_array);
}