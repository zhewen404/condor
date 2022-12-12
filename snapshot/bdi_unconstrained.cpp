#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
typedef unsigned count_t;

struct Line {
   u_int64_t * segs8;
   u_int32_t * segs4;
   u_int16_t * segs2;
   
   u_int64_t base8;
   u_int32_t base4;
   u_int16_t base2;

   bool v8;
   bool v4;
   bool v2;
   bool xored;
   bool inter_vanish; // this line should vanish when counting inter compression

   // sizes
   unsigned size;
   unsigned b8d1;
   unsigned b8d2;
   unsigned b8d4;
   unsigned b4d2;
   unsigned b4d1;
   unsigned b2d1;
   unsigned zero;
   unsigned rep;

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
      line_array[i].b8d1 = lineSize;
      line_array[i].b4d1 = lineSize;
      line_array[i].b8d2 = lineSize;
      line_array[i].b2d1 = lineSize;
      line_array[i].b4d2 = lineSize;
      line_array[i].b8d4 = lineSize;
      line_array[i].zero = lineSize;
      line_array[i].rep = lineSize;
      line_array[i].size = lineSize;
      line_array[i].v8 = false;
      line_array[i].v4 = false;
      line_array[i].v2 = false;
      line_array[i].inter_vanish = false;
      // printf("line#%d: ", i);

      //create segments
      unsigned base=8;
      line_array[i].segs8 = (uint64_t *) malloc(lineSize/base * sizeof(uint64_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int64_t p_seg = 0; 
         for (unsigned j = 0; j < base-1; j++) {
            p_seg += p[i*lineSize + js * base + j];
            p_seg = p_seg << 8;
         }
         p_seg += p[i*lineSize + js * base + base - 1];
         line_array[i].segs8[js] = p_seg;
         // printf("%" PRIu64 " ", line_array[i].segs8[js]);
      }
      // printf("\n");

      base=4;
      line_array[i].segs4 = (uint32_t *) malloc(lineSize/base * sizeof(uint32_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int32_t p_seg = 0; 
         for (unsigned j = 0; j < base-1; j++) {
            p_seg += p[i*lineSize + js *base + j];
            p_seg = p_seg << 8;
         }
         p_seg += p[i*lineSize + js * base + base - 1];
         line_array[i].segs4[js] = p_seg;
         // printf("%" PRIu32 "\n", line_array[i].segs4[js]);
      }

      base=2;
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
   }
}

void bdi(struct Line* line_array, unsigned lineSize, u_int8_t * p, unsigned numLine, unsigned do_xor) {
   //counts
   count_t zero_ct=0;
   count_t rep_ct=0;
   count_t b8d1_ct=0;
   count_t b8d2_ct=0;
   count_t b8d4_ct=0;
   count_t b4d2_ct=0;
   count_t b4d1_ct=0;
   count_t b2d1_ct=0;
   // printf("calling %s\n", __func__);
   // for every line
   for(unsigned i = 0; i < numLine; i++) {
      // if (do_xor != 0 && line_array[i].cachedAbove) {
      //    line_array[i].size = 0;
      //    continue;
      // }

      // try compression
      // zero
      line_array[i].zero = 1;
      for (unsigned j=0; j < lineSize/8; j++) {
         if(line_array[i].segs8[j] != UINT64_C(0)) {
            line_array[i].zero = lineSize;
            // printf("i=%d, j=%d: nonzero detected, ls=%d\n", i, j,line_array[i].zero);
            break;
         }
      }
      if (line_array[i].zero != lineSize) {
         line_array[i].size = line_array[i].zero;
         zero_ct += 1;
         continue;
      }

      // rep
      line_array[i].rep = 8;
      uint64_t ref = 0;
      for (unsigned j=0; j < lineSize/8; j++) {
         if(j == 0) {
            ref = line_array[i].segs8[j];
         }
         else {
            if (line_array[i].segs8[j] != ref) {
               line_array[i].rep = lineSize;
               break;
            }
         }
      }
      if (line_array[i].rep != lineSize) {
         line_array[i].size = line_array[i].rep;
         rep_ct += 1;
         continue;
      }

      // b8d1
      line_array[i].b8d1 = 0;
      line_array[i].b8d1 += 8;
      for (unsigned j=0; j < lineSize/8; j++) {
         if(line_array[i].segs8[j] <= UINT8_MAX) {
            line_array[i].b8d1 += 1;
         }
         else {
            if (line_array[i].v8 == false) {
               line_array[i].v8 = true;
               line_array[i].base8 = line_array[i].segs8[j];
            }
            assert(line_array[i].v8 == true);
            if (((line_array[i].segs8[j] >= line_array[i].base8)?
               (line_array[i].segs8[j] - line_array[i].base8) : 
               (line_array[i].base8 - line_array[i].segs8[j])) <= UINT8_MAX) {
               line_array[i].b8d1 += 1;
            }
            else {
               line_array[i].b8d1 = lineSize;
               break;
            }
         }
      }
      line_array[i].v8 = false;
      if (line_array[i].b8d1 != lineSize) {
         line_array[i].size = line_array[i].b8d1;
         b8d1_ct += 1;
         continue;
      }
      
      // b4d1
      line_array[i].b4d1 = 0;
      if (line_array[i].b8d1 != lineSize) {line_array[i].b4d1 = lineSize;}
      else {
         line_array[i].b4d1 += 4;
         for (unsigned j=0; j < lineSize/4; j++) {
            if(line_array[i].segs4[j] <= UINT8_MAX) {
               line_array[i].b4d1 += 1;
            }
            else {
               if (line_array[i].v4 == false) {
                  line_array[i].v4 = true;
                  line_array[i].base4 = line_array[i].segs4[j];
               }
               assert(line_array[i].v4 == true);
               if (((line_array[i].segs4[j] >= line_array[i].base4)?
               (line_array[i].segs4[j] - line_array[i].base4) : 
               (line_array[i].base4 - line_array[i].segs4[j])) <= UINT8_MAX) {
                  line_array[i].b4d1 += 1;
               }
               else {
                  line_array[i].b4d1 = lineSize;
                  break;
               }
            }
         }
      }
      line_array[i].v4 = false;
      if (line_array[i].b4d1 != lineSize) {
         line_array[i].size = line_array[i].b4d1;
         b4d1_ct += 1;
         continue;
      }
      
      // b8d2
      line_array[i].b8d2 = 0;
      if (line_array[i].b8d1 != lineSize || 
         line_array[i].b4d1 != lineSize) {
            line_array[i].b8d2 = lineSize;
      }
      else {
         line_array[i].b8d2 += 8;
         for (unsigned j=0; j < lineSize/8; j++) {
            if(line_array[i].segs8[j] <= UINT16_MAX) {
               line_array[i].b8d2 += 2;
            }
            else {
               if (line_array[i].v8 == false) {
                  line_array[i].v8 = true;
                  line_array[i].base8 = line_array[i].segs8[j];
               }
               assert(line_array[i].v8 == true);
               if (((line_array[i].segs8[j] >= line_array[i].base8)?
               (line_array[i].segs8[j] - line_array[i].base8) : 
               (line_array[i].base8 - line_array[i].segs8[j])) <= UINT16_MAX) {
                  line_array[i].b8d2 += 2;
               }
               else {
                  line_array[i].b8d2 = lineSize;
                  break;
               }
            }
         }
      }
      line_array[i].v8 = false;
      if (line_array[i].b8d2 != lineSize) {
         line_array[i].size = line_array[i].b8d2;
         b8d2_ct += 1;
         continue;
      }

      // b2d1
      line_array[i].b2d1 = 0;
      if (line_array[i].b8d1 != lineSize || 
         line_array[i].b4d1 != lineSize || 
         line_array[i].b8d2 != lineSize) {
            line_array[i].b2d1 = lineSize;
      }
      else {
         line_array[i].b2d1 += 2;
         for (unsigned j=0; j < lineSize/2; j++) {
            if(line_array[i].segs2[j] <= UINT8_MAX) {
               line_array[i].b2d1 += 1;
            }
            else {
               if (line_array[i].v2 == false) {
                  line_array[i].v2 = true;
                  line_array[i].base2 = line_array[i].segs2[j];
               }
               assert(line_array[i].v2 == true);
               if (((line_array[i].segs2[j] >= line_array[i].base2)?
               (line_array[i].segs2[j] - line_array[i].base2) : 
               (line_array[i].base2 - line_array[i].segs2[j])) <= UINT8_MAX) {
                  line_array[i].b2d1 += 1;
               }
               else {
                  line_array[i].b2d1 = lineSize;
                  break;
               }
            }
         }
      }
      line_array[i].v2 = false;
      if (line_array[i].b2d1 != lineSize) {
         line_array[i].size = line_array[i].b2d1;
         b2d1_ct += 1;
         continue;
      }

      // b4d2
      line_array[i].b4d2 = 0;
      if (line_array[i].b8d1 != lineSize || 
         line_array[i].b4d1 != lineSize || 
         line_array[i].b8d2 != lineSize || 
         line_array[i].b2d1 != lineSize) {
            line_array[i].b4d2 = lineSize;
      }
      else {
         line_array[i].b4d2 += 4;
         for (unsigned j=0; j < lineSize/4; j++) {
            if(line_array[i].segs4[j] <= UINT16_MAX) {
               line_array[i].b4d2 += 2;
            }
            else {
               if (line_array[i].v4 == false) {
                  line_array[i].v4 = true;
                  line_array[i].base4 = line_array[i].segs4[j];
               }
               assert(line_array[i].v4 == true);
               if (((line_array[i].segs4[j] >= line_array[i].base4)?
               (line_array[i].segs4[j] - line_array[i].base4) : 
               (line_array[i].base4 - line_array[i].segs4[j])) <= UINT16_MAX) {
                  line_array[i].b4d2 += 2;
               }
               else {
                  line_array[i].b4d2 = lineSize;
                  break;
               }
            }
         }
      }
      line_array[i].v4 = false;
      if (line_array[i].b4d2 != lineSize) {
         line_array[i].size = line_array[i].b4d2;
         b4d2_ct += 1;
         continue;
      }

      // b8d4
      line_array[i].b8d4 = 0;
      if (line_array[i].b8d1 != lineSize || 
         line_array[i].b4d1 != lineSize || 
         line_array[i].b8d2 != lineSize || 
         line_array[i].b2d1 != lineSize || 
         line_array[i].b4d2 != lineSize) {
            line_array[i].b8d4 = lineSize;
      }
      else {
         line_array[i].b8d4 += 8;
         for (unsigned j=0; j < lineSize/8; j++) {
            if(line_array[i].segs8[j] <= UINT32_MAX) {
               line_array[i].b8d4 += 4;
            }
            else {
               if (line_array[i].v8 == false) {
                  line_array[i].v8 = true;
                  line_array[i].base8 = line_array[i].segs8[j];
               }
               assert(line_array[i].v8 == true);
               if (((line_array[i].segs8[j] >= line_array[i].base8)?
               (line_array[i].segs8[j] - line_array[i].base8) : 
               (line_array[i].base8 - line_array[i].segs8[j])) <= UINT32_MAX) {
                  line_array[i].b8d4 += 4;
               }
               else {
                  line_array[i].b8d4 = lineSize;
                  break;
               }
            }
         }
      }
      line_array[i].v8 = false;
      if (line_array[i].b8d4 != lineSize) {
         line_array[i].size = line_array[i].b8d4;
         b8d4_ct += 1;
         continue;
      }

      
   }
   printf("==counts==: \nzero=%d, rep=%d\nb8d1=%d, b8d2=%d, b8d4=%d\nb4d2=%d,b4d1=%d,b2d1=%d\n",
      zero_ct, rep_ct, 
      b8d1_ct,b8d2_ct,b8d4_ct,b4d2_ct,b4d1_ct,b2d1_ct);
}

unsigned countSetBits(uint64_t n)
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
   // for every line
   // xor with another block and modify data
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

         for (unsigned js = 0; js < lineSize/8; js++) {
            uint64_t temp8 = line_array[ind].segs8[js] ^ line_array[i].segs8[js];
            line_array[ind].segs8[js] = temp8;
            line_array[i].segs8[js] = temp8;
            // printf("%" PRIu32 "\n", temp8);
         }
         for (unsigned js = 0; js < lineSize/4; js++) {
            uint32_t temp4 = line_array[ind].segs4[js] ^ line_array[i].segs4[js];
            line_array[ind].segs4[js] = temp4;
            line_array[i].segs4[js] = temp4;
         }
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
            for (unsigned js = 0; js < lineSize/8; js++) {
               uint64_t temp8 = line_array[ind].segs8[js] ^ line_array[i].segs8[js];
               ham += countSetBits(temp8);
               // printf("temp8=%" PRIu64 " ham=%d\n", temp8, ham);
            }
            // printf("i=%d, ham[%d]=%d\n", i, ind, ham);
            // exit(1);
            if (ham < min_ham) {
               min_ham = ham;
               best_cand_ind = ind;
            }               
         }
         // printf("i=%d,min ham[%d]=%d\n", i, best_cand_ind, min_ham);
         
         for (unsigned js = 0; js < lineSize/8; js++) {
            uint64_t temp8 = line_array[best_cand_ind].segs8[js] ^ line_array[i].segs8[js];
            line_array[best_cand_ind].segs8[js] = temp8;
            line_array[i].segs8[js] = temp8;
            // printf("%" PRIu32 "\n", temp8);
         }
         for (unsigned js = 0; js < lineSize/4; js++) {
            uint32_t temp4 = line_array[best_cand_ind].segs4[js] ^ line_array[i].segs4[js];
            line_array[best_cand_ind].segs4[js] = temp4;
            line_array[i].segs4[js] = temp4;
         }
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
      else if (do_xor == 3) {
         // worst xor
         int max_ham = -1;
         unsigned best_cand_ind = numLine + 1;
         for (unsigned ind = 0; ind < numLine; ind++) {
            if (line_array[ind].xored == true || i == ind) continue;
            int ham = 0;
            for (unsigned js = 0; js < lineSize/8; js++) {
               uint64_t temp8 = line_array[ind].segs8[js] ^ line_array[i].segs8[js];
               ham += countSetBits(temp8);
               // printf("temp8=%" PRIu64 " ham=%d\n", temp8, ham);
            }
            // printf("i=%d, ham[%d]=%d\n", i, ind, ham);
            // exit(1);
            if (ham > max_ham) {
               max_ham = ham;
               best_cand_ind = ind;
            }               
         }
         // printf("i=%d,min ham[%d]=%d\n", i, best_cand_ind, min_ham);
         
         for (unsigned js = 0; js < lineSize/8; js++) {
            uint64_t temp8 = line_array[best_cand_ind].segs8[js] ^ line_array[i].segs8[js];
            line_array[best_cand_ind].segs8[js] = temp8;
            line_array[i].segs8[js] = temp8;
            // printf("%" PRIu32 "\n", temp8);
         }
         for (unsigned js = 0; js < lineSize/4; js++) {
            uint32_t temp4 = line_array[best_cand_ind].segs4[js] ^ line_array[i].segs4[js];
            line_array[best_cand_ind].segs4[js] = temp4;
            line_array[i].segs4[js] = temp4;
         }
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
      printf("./bdiuc dir do_xor\n");
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
   else if (!strcmp(argv[2], "worst")) {
      do_xor = 3;
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
   
   if (do_xor == 1 || do_xor == 2 || do_xor == 3) {
      init_line_data(line_array, lineSize, p, numLine);
      // xor preprocess
      xor_preprocess_unconstrained(line_array, lineSize, p, numLine, 
         do_xor);
      // bdi compression
      bdi(line_array, lineSize, p, numLine, do_xor);
   }
   else {
      // no need to read tag files
      init_line_data(line_array, lineSize, p, numLine);
      bdi(line_array, lineSize, p, numLine, do_xor);
   }
   
   // for every line:print
   unsigned tot_size = 0;
   unsigned inter_gain = 0;
   for(unsigned i = 0; i < numLine; i++) {
      tot_size += line_array[i].size;
      if (line_array[i].inter_vanish) inter_gain += line_array[i].size;
   }
   double cr = (double)(numLine*lineSize)/tot_size;
   printf("total size = %d/%d (%f)\n", tot_size, numLine*lineSize, cr);
   double cr_both = (double)(numLine*lineSize)/(tot_size-inter_gain);
   printf("intra+inter = %d/%d  {%f}\n", tot_size-inter_gain, numLine*lineSize, cr_both);

   // free up mem
   free(p);
   for (int i=0; i < numLine; i++) {
      free(line_array[i].segs8);
      free(line_array[i].segs4);
      free(line_array[i].segs2);
   }
   free(line_array);
}