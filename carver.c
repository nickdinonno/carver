/*********************************************************************************************************
 * Carver
 * Purpose: Identify files embedded in selected file. Practice multi-threading and learn more on forensics
 * Nicholas DiNonno
*****************************************/

//  gcc carver.c -o carver -g -Wall -Wextra -pthread

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// Magic Numbers
static const unsigned char PNGMAGIC[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
static const unsigned char JPGMAGIC[] = {0xFF, 0xD8, 0xFF};

// File extension struct
struct fileExtension {
   const char *name;
   const unsigned char *magicNumbers;
   int length;
};

// Initialized array storing file extensions
struct fileExtension fileExtArray[] = {
   { "png", PNGMAGIC, sizeof(PNGMAGIC)},
   { "jpg", JPGMAGIC, sizeof(JPGMAGIC)}
};

// Struct for passing multiple arguments to threaded function artifactHunter
struct hunterThreadData {
   int threadId;
   unsigned long fileSize;
   long chunkSize;
   int start;
   int stop;
   unsigned char *buffer;
};

// Find the length of the longest file signature stored (used for creating optimized chunk overlaps)
int findLongestFileExtension(struct fileExtension *array, int count) {
   int maxLength = 0;
   for (int i = 0; i < count; i++) {
      if (array[i].length > maxLength) {
         maxLength = array[i].length;
      }
   }
   return maxLength;
}

void *artifactHunter(void *arg) {

   // Error handling
   if (arg == NULL) {
      perror("Error processing thread function artifactHunter thread arguments");
      return NULL;
   }

   struct hunterThreadData* arguments = (struct hunterThreadData*) arg;

   int numExtensions = (int)(sizeof(fileExtArray) / sizeof(fileExtArray[0]));
   int maxMagicLen = findLongestFileExtension(fileExtArray, numExtensions);

   // Takes the smaller of the two -cannot exceed filesize
   int extendedEnd = (maxMagicLen - 1) + arguments->stop;
   if ((unsigned long)extendedEnd > arguments->fileSize) {
      extendedEnd = arguments->fileSize;
   }
   int numSignatures = sizeof(fileExtArray) / sizeof(fileExtArray[0]);

   // Iterate through the chunk byte by byte
   for (int i = arguments->start; i < extendedEnd; i++) { 

      // Iterate through the file extension array list
      for (int j = 0; j < numSignatures; j++) { 
         
         int signatureLength = fileExtArray[j].length;

         // Note enough space left
         if ((unsigned long)(i + signatureLength) > arguments->fileSize) {
            continue;
         }

         // Compare the bytes
         bool match = true;
         for (int k = 0; k < signatureLength; k++) {
            if (arguments->buffer[i + k] != fileExtArray[j].magicNumbers[k]) {
               //printf("Bytes do not match!\n");
               match = false;
               break;
            }
            //printf("File Byte Found: %hhu\n", arguments->buffer[i + k]);
         }

         // Print match
         if (match) {

            // Only print if found in the original chunk, not overlap region
            if (i < arguments->stop) {
               printf("\n********** Match **********\n");
               printf("File Type Found: %s\n", fileExtArray[j].name);
               printf("Found by Thread ID: %d\n", arguments->threadId);
               printf("Found at offset: %d\n", i);
            }
         }
      }
   }
   return NULL;
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
   
   // Error handling
   if (argc != 3) {
      perror("Improper usage. Should be...\n./carver [filename] [numberOfThreads]\n");
      return 1;
   }
   if (!argv[1]) {
      perror("Error in processing selected file.\n");
      return 1;
   }
   long numThreads = strtol(argv[2], NULL, 10);    // Converts user input for nuimber of threads to int 
   if (numThreads < 1 || numThreads > 25) {
      printf("Please select a number of threads between 1 and 25.\n");
      return 1;
   }

   // Read the file as bytes and check if it is empty
   FILE* fp = fopen(argv[1], "rb");

   if (!fp) {
      perror("Error opening file!\n");
      return 1;
   }

   // Store the file in a buffer to be used by multiple threads later, confirm buffer and filesize match
   unsigned long fileSize = calcFileSize(fp);
   unsigned char *buffer = malloc(fileSize);
   unsigned int bytesRead = fread(buffer, 1, fileSize, fp);

   printf("Filesize: %lu\n", fileSize);
   //printf("bytesRead: %d\n", bytesRead);

   if (bytesRead != fileSize) {
      perror("Error storing file in bufer. Please try again");
   }

   // Create thread array
   pthread_t *threadArray = malloc(numThreads * sizeof(pthread_t));

   // Create an args list ot be passed to threaded artifacthunter function
   struct hunterThreadData* args = malloc(numThreads * sizeof(struct hunterThreadData));

   // Create and join threads
   for (int i = 0; i < numThreads; i++) {
      args[i].threadId = i;
      args[i].fileSize = fileSize;
      args[i].chunkSize = fileSize / numThreads;
      args[i].start = i * args[i].chunkSize;
      args[i].stop = (i + 1) * args[i].chunkSize;
      args[i].buffer = buffer;

      pthread_create(&threadArray[i], NULL, artifactHunter, &args[i]);
   }

   // Join threads together
   for (int i = 0; i < numThreads; i++) {
      pthread_join(threadArray[i], NULL);
   }

   // Clean up!
   free(threadArray);
   free(buffer);
   free(args);
   fclose(fp);

   return 0;
}