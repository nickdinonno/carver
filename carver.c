/*********************************************************************************************************
 * Carver
 * Purpose: Identify files embedded in selected file. Practice multi-threading and learn more on forensics
 * Nicholas DiNonno
*****************************************/

//  gcc carver.c -o carver -g -Wall -Wextra -pthread

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *artifactHunter(void *id) {
   printf("Finding an artifact for ya!");
}

long calcFileSize(FILE* fp) {

   // Calculate size of file in bytes
   fseek(fp, 0, SEEK_END); // Move pointer to end of file
   long fileSizeinBytes = ftell(fp);
   rewind(fp); // Move back to beginning
   return fileSizeinBytes;

}

int main (int argc, char ** argv) {

   printf("Welcome to Carver. Please specify the file and the number of threads to begin carving!\n");
   
   if (argc != 3) {
      printf("Improper usage. Should be...\n./carver [filename] [numberOfThreads]\n");
      return 1;
   }

   // Input validation
   if (argv[1] == NULL) {
      printf("Error in processing selected file.\n");
      return 1;
   }

   // if (argv[2]) {

   // }

   // Read the file as bytes
   FILE* fp = fopen(argv[1], "rb");

   // See if it is empty
   if (fp == NULL) {
      printf("Error opening file!\n");
      return 1;
   }

   long fileSize = calcFileSize(fp);
   printf("%ld\n", fileSize);


   // P threads
   int nthreads = strtol(argv[2], NULL, 10);
   pthread_t *threadArray = malloc(nthreads * sizeof(pthread_t));

   for (int i = 0; i < nthreads; i++) {
      pthread_create(&threadArray[i], NULL, artifactHunter, i);
   }

   // Join threads together
   for (int i = 0; i < nthreads; i++) {
      pthread_join(threadArray[i], NULL);
   }

   free(threadArray);
   fclose(fp);

   return fileSize;

}

