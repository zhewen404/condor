#include <stdio.h>
#include <stdlib.h>
u_int8_t * read_file(FILE * file) //Don't forget to free retval after use
{
   int size = 0;
   unsigned int val;
   int startpos = ftell(file);
   while (fscanf(file, "%d ", &val) == 1)
   {
      ++size;
   }
   printf("%d\n", size);
   u_int8_t * retval = (unsigned char *) malloc(size);
   fseek(file, startpos, SEEK_SET); //if the file was not on the beginning when we started
   int pos = 0;
   while (fscanf(file, "%d ", &val) == 1)
   {
      retval[pos++] = (unsigned char) val;
   }
   return retval;
}
int main() {

   FILE *fp;
   fp = fopen("l2.cache", "r");
   u_int8_t * p = read_file(fp);
   unsigned numLine = sizeof(p)/sizeof(p[0]);
   unsigned lineSize = sizeof(p)/numLine;
   printf("numLine=%d, lineSize=%d\n", numLine, lineSize);
   fclose(fp);
   unsigned base = 8; //4,2
   unsigned delta = 4; // 2,1
   for(unsigned i = 0; i < numLine; i++) {
    //p
   }

}