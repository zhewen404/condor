#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>

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

void bdi(struct Line* line_array, unsigned lineSize, u_int8_t * p, unsigned numLine) {
   printf("calling %s\n", __func__);
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
      // printf("line#%d: ", i);

      //create segments
      unsigned base=8;
      line_array[i].segs8 = (uint64_t *) malloc(lineSize/base * sizeof(uint64_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int64_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs8[js] = p_seg;
         // printf("%"PRIu64" ", line_array[i].segs8[js]);
      }
      // printf("\n");

      base=4;
      line_array[i].segs4 = (uint32_t *) malloc(lineSize/base * sizeof(uint32_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int32_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs4[js] = p_seg;
         // printf("%" PRIu32 "\n", line_array[i].segs4[js]);
      }

      base=2;
      line_array[i].segs2 = (uint16_t *) malloc(lineSize/base * sizeof(uint16_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int16_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs2[js] = p_seg;
         // printf("%" PRIu16 "\n", line_array[i].segs2[js]);
      }

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
         continue;
      }

      
   }
}

void bdi_xor(struct Line* line_array, unsigned lineSize, 
   u_int8_t * p, unsigned numLine, 
   uint64_t * p_l1i_tag, uint64_t * p_l1d_tag,
   uint64_t * p_l2_tag) {
   printf("calling %s\n", __func__);
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
      // printf("line#%d: ", i);

      //create segments
      unsigned base=8;
      line_array[i].segs8 = (uint64_t *) malloc(lineSize/base * sizeof(uint64_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int64_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs8[js] = p_seg;
         // printf("%"PRIu64" ", line_array[i].segs8[js]);
      }
      // printf("\n");

      base=4;
      line_array[i].segs4 = (uint32_t *) malloc(lineSize/base * sizeof(uint32_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int32_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs4[js] = p_seg;
         // printf("%" PRIu32 "\n", line_array[i].segs4[js]);
      }

      base=2;
      line_array[i].segs2 = (uint16_t *) malloc(lineSize/base * sizeof(uint16_t));
      for (unsigned js = 0; js < lineSize/base; js++) {
         u_int16_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs2[js] = p_seg;
         // printf("%" PRIu16 "\n", line_array[i].segs2[js]);
      }

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
         continue;
      }

      
   }
}

int main(int argc, char *argv[]) {
   if (argc < 3) {
      printf("./bdi dir do_xor\n");
      return 1;
   }
   bool do_xor = (!strcmp(argv[2],"1"))? true : false;
   // printf("doxor=%d\n", do_xor);
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
   struct Line* line_array = (Line*) malloc(numLine * sizeof(struct Line));
   
   uint64_t * p_l2_tag;
   uint64_t * p_l1i_tag;
   uint64_t * p_l1d_tag;
   if (do_xor == true) {
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
      for (unsigned i = 0; i < size_l2_tag; i++) {
         printf("l2 tag [%d]%lx\n", i, p_l2_tag[i]);
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
      for (unsigned i = 0; i < size_l1d_tag; i++) {
         printf("l1d tag [%d]%lx\n", i, p_l1d_tag[i]);
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
      for (unsigned i = 0; i < size_l1i_tag; i++) {
         printf("l1i tag [%d]%lx\n", i, p_l1i_tag[i]);
      }
      bdi_xor(line_array, lineSize, p, numLine, 
         p_l1i_tag, p_l1d_tag, p_l2_tag);
   }
   else {
      // no need to read tag files
      bdi(line_array, lineSize, p, numLine);
   }
   
   // for every line:print
   unsigned tot_size = 0;
   for(unsigned i = 0; i < numLine; i++) {
      // printf("line#%d: ", i);

      // for (unsigned js = 0; js < lineSize/8; js++) {
         // printf("%"PRIu64" ", line_array[i].segs8[js]);
      // }
      // printf("\n");
      // printf("size=%d (zero=%d, rep=%d, b8d1=%d, b4d1=%d, b8d2=%d, b2d1=%d, d4b2=%d, b8d4=%d)\n", 
      //    line_array[i].size, line_array[i].zero, line_array[i].rep,
      //    line_array[i].b8d1, line_array[i].b4d1, line_array[i].b8d2,
      //    line_array[i].b2d1, line_array[i].b4d2, line_array[i].b8d4);
      tot_size += line_array[i].size;
   }
   double cr = (double)(numLine*lineSize)/tot_size;
   printf("total size = %d/%d (%f)\n", tot_size, numLine*lineSize, cr);

   // free up mem
   free(p);
   if (do_xor == true) {
      free(p_l2_tag);
      free(p_l1i_tag);
      free(p_l1d_tag);
   }
   for (int i=0; i < numLine; i++) {
      free(line_array[i].segs8);
      free(line_array[i].segs4);
      free(line_array[i].segs2);
   }
   free(line_array);
}