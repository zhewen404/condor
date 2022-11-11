#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <math.h>
typedef unsigned count_t;

struct Line {
   uint16_t dedup_hash;
   unsigned dedup_size;
   u_int16_t * segs2;
   bool unique;
   bool cachedAbove;
   bool xored;
   bool inter_vanish; // this line should vanish when counting inter compression
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

void init_line_data(struct Line* line_array, unsigned lineSize, u_int8_t * p, unsigned numLine) {
   // printf("calling %s\n", __func__);
   // for every line
   for(unsigned i = 0; i < numLine; i++) {
      line_array[i].dedup_hash = 0;
      line_array[i].dedup_size = lineSize;
      line_array[i].unique = false;
      line_array[i].inter_vanish = false;

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

void xor_preprocess_unconstrained(struct Line* line_array, unsigned lineSize, 
   u_int8_t * p, unsigned numLine, unsigned do_xor) {
   count_t xor_ct = 0;
   // printf("calling %s\n", __func__);
   // for every line cached above
   // 1. xor with another block and modify data
   for(unsigned i = 0; i < numLine; i++) {
      if (xor_ct == numLine-(numLine%2)) break; // all possible xor done
      if (line_array[i].xored == true) continue;
      // perform xor
      unsigned ind;
      if (do_xor == 1) {
         // ****** rand xor ****** //
         do {
            ind = rand() % numLine;
            // printf("rand=%d\n", ind);
         } while(ind == i || 
            line_array[ind].xored == true);
         line_array[ind].xored = true;
         line_array[i].xored = true;
         xor_ct += 2;
         line_array[ind].inter_vanish = true;
         // printf("[%d] ^ [%d]\n", i, ind);

         for (unsigned js = 0; js < lineSize/2; js++) {
            uint16_t temp2 = line_array[ind].segs2[js] ^ line_array[i].segs2[js];
            line_array[ind].segs2[js] = temp2;
            line_array[i].segs2[js] = temp2;
         }
         
      }
      else if (do_xor == 2) {
         // ideal xor
         unsigned min_ham = lineSize*8 + 1;
         unsigned best_cand_ind = numLine + 1;
         for (unsigned ind = 0; ind < numLine; ind++) {
            if (line_array[ind].xored == true || i == ind) continue;
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
         line_array[best_cand_ind].inter_vanish = true;
         line_array[i].xored = true;
         xor_ct += 2;
      }
      else {
         assert(false);
      }
   }
}

int main(int argc, char *argv[]) {
   if (argc < 3) {
      printf("./dedupuc dir do_xor\n");
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
   char suf[] = "/board.cache_hierarchy.l2cache.tags.cache";
   snprintf(filename, sizeof(filename), "%s%s", argv[1], suf);
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
   
   if (do_xor == 1 || do_xor == 2) {
      init_line_data(line_array, lineSize, p, numLine);
      // xor preprocess
      xor_preprocess_unconstrained(line_array, lineSize, p, numLine, 
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
   unsigned inter_gain = 0;
   for(unsigned i = 0; i < numLine; i++) {
      tot_size += line_array[i].dedup_size;
      if (line_array[i].inter_vanish) inter_gain += line_array[i].dedup_size;
   }
   double cr = (double)(numLine*lineSize)/tot_size;
   printf("total size = %d/%d (%f)\n", tot_size, numLine*lineSize, cr);
   double cr_both = (double)(numLine*lineSize)/(tot_size-inter_gain);
   printf("intra+inter = %d/%d  {%f}\n", tot_size-inter_gain, numLine*lineSize, cr_both);

   // free up mem
   free(p);
   for (int i=0; i < numLine; i++) {
      free(line_array[i].segs2);
   }
   free(line_array);
}