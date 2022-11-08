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

void dedup(struct Line* line_array, unsigned lineSize, u_int8_t * p, unsigned numLine) {
   // for every line
   for(unsigned i = 0; i < numLine; i++) {
      line_array[i].dedup_hash = 0;
      line_array[i].dedup_size = lineSize;
      line_array[i].unique = false;

      //create segments
      unsigned base=2;
      unsigned num = lineSize/base;
      line_array[i].segs2 = (uint16_t *) malloc(num * sizeof(uint16_t));
      for (unsigned js = 0; js < num; js++) {
         u_int16_t p_seg = 0; 
         for (unsigned j = 0; j < base; j++) {
            p_seg = p_seg << base;
            p_seg += p[i*lineSize + js *base + j];
         }
         line_array[i].segs2[js] = p_seg;
         // printf("%" PRIu16 "\n", line_array[i].segs2[js]);
      }
      
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

int main(int argc, char *argv[]) {
    if (argc < 1) return 0;
   char* filename = argv[1];
   // printf("%s\n", argv[1]);
   
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
   struct Line* line_array = malloc(numLine * sizeof(struct Line));
   
   dedup(line_array, lineSize, p, numLine);
   
   // for every line:print
   unsigned tot_size = 0;
   for(unsigned i = 0; i < numLine; i++) {
      tot_size += line_array[i].dedup_size;
      if (line_array[i].dedup_size != 0) {
        for (unsigned js = 0; js < lineSize/2; js++) {
            // printf("%d ", line_array[i].segs2[js]);
        }
        // printf("\n");
      }
   }
   double cr = (double)(numLine*lineSize)/tot_size;
   printf("total size = %d/%d (%f)\n", tot_size, numLine*lineSize, cr);

   // free up mem
   free(p);
   for (int i=0; i < numLine; i++) {
      free(line_array[i].segs2);
   }
   free(line_array);
}