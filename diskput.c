#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "dirinfo_def.h"


int getFatEntry(char filename[], int n) {
	//returns the fat_index-th fat entry.
	FILE *imageptr = fopen(filename, "rb");
	int even = 0;
   	int fatIndex = 0;
	if((n&1) == 1){
		//entry is odd
		fatIndex = (3*(n-1))/2;
		even = 0;
	}
	else{
		//entry is even
		fatIndex = (3*n)/2;
		even = 1;
	}
	int i, num_files = 0;
	uint8_t X0, X1, X2;
	uint16_t X, Y;
	fat_entry_t fat_entry;
   
	fseek(imageptr, 512, SEEK_SET); // FAT1 at sector 1
	//go to fatIndex-th entry
	fseek(imageptr, fatIndex, SEEK_CUR);
   
	fread(&fat_entry, sizeof(fat_entry), 1, imageptr);
	X0 = fat_entry.b0;
	X1 = fat_entry.b1;
	X2 = fat_entry.b2;

	X = ((X1 & 0x0F) << 8) + X0 ;
	Y = ((X1 & 0xF0) >> 4) + (X2 << 4);

	if(even == 1){
		return X;
	}
	else{
		return Y;
	}
}


int getFreeSpace(char imagename[], int bpsector) {
	//obtains the amount of free space in the disk image.
	int free_sectors = 0;

	FILE *diskimage = fopen(imagename, "rb");

	//move to data area
	fseek(diskimage, 33*bpsector, SEEK_SET);

	char* sectorData=malloc(sizeof(char)*bpsector);
	for (int j = 0; j<bpsector; j++){
		sectorData[j] = '\0';
	}
	int isfree = 1;
	for (int i = 0; i < 2847; i++){
		isfree = 1;
		//read all data section sectors
		fread(sectorData, 1, bpsector, diskimage);
		for(int j = 0; j < 511; j++){
			//for each sector in data area, make sure free
			if(sectorData[j] != 0){
				isfree = 0;
			}
		}
		if(isfree == 1){
			free_sectors += 1;
		}
	}

   free(sectorData);
   return free_sectors;
}



int main(int argc, char *argv[])
{	
	if( argc == 4 ) {
    	printf("The arguments supplied are DiskImage: %s, Path: %s, and FileName: %s\n", argv[1], argv[2], argv[3]);

   	}
	if( argc == 3 ) {
    	printf("The arguments supplied are DiskImage: %s, and FileName: %s\n", argv[1], argv[2]);

   	}
   	else if( argc > 4 ) {
    	printf("Too many arguments supplied.\n");
		exit(0);
   	}
   	else if (argc < 3){
    	printf("Too few arguments provided. Please provide disk image to search, and a file to copy.\n");
		exit(0);
	}
	//open disk image in binary read mode
	FILE *diskimage = fopen(argv[1], "rb");
	boot_t  boot_sector;
	//check if open successful
	if(diskimage == NULL){
		printf("Failed to open disk image file...\n");
		printf("program usage: \"./diskget disk.IMA <filename>\" or \"./diskget disk.IMA <path> <filename>\"\n");
		exit(0);
	}

	//image successfully opened. 
	fread(&boot_sector, sizeof(boot_sector), 1, diskimage);
	int bpsector = boot_sector.bytes_per_sector;

	//check to make sure that the file we want to copy exists
	FILE *readFile;
	if(argc == 4){
		readFile = fopen(argv[3], "rb");
	}
	else{
		readFile = fopen(argv[2], "rb");
	}
	
	if(readFile == NULL){
		printf("File not found.\n");
		exit(0);
	}
	//we know it does exist now.

	//move to start of root directory
	fseek(diskimage, 19*512, SEEK_SET);

	//must search all of root directory for the desired file

	//split the requested filename/extension for comparison purposes.
	char desired_filename[9]={'\0'};
	char desired_extension[4]={'\0'};
	char found_filename[9]={'\0'};
	char found_extension[4]={'\0'};
   	/* get the first token */
	char buffer[20]={'\0'};
	if(argc == 4){
		strcpy(buffer, argv[3]);
	}
	else{
		strcpy(buffer, argv[2]);
	}
	
	
    int i = 0;
    char *p = strtok (buffer, ".");
    char *array[3];
    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, ".");
    }
	//check to make sure both exist:
	if(array[0] == NULL || array[1] == NULL){
		printf("incorrectly formatted filename!\n");
		exit(0);
	}
	strcpy(desired_filename, array[0]);
	//convert to upper case
	for(int i=0;i<strlen(desired_filename);i++){
        desired_filename[i] = toupper(desired_filename[i]);
    }
	strcpy(desired_extension, array[1]);
	for(int i=0;i<strlen(desired_extension);i++){
        desired_extension[i] = toupper(desired_extension[i]);
    }


	entry_t direntry;	
	//14 sectors, each with 16 entries = 224 possible entries to search.
	for(int i = 0; i<=224; i++){
		//obtain directory entry
		fread(&direntry, sizeof(direntry), 1, diskimage);
		//remove all whitespace from direntry filenames.
		
		for(int j =0; j<8; j++){
			if(direntry.filename[j] != ' '){
				found_filename[j] = direntry.filename[j];
			}
		}
		for(int k =0; k<3; k++){
			if(direntry.extension[k] != ' '){
				found_extension[k] = direntry.extension[k];
			}
		}
		if((found_filename[0] != 0xE5)&&(found_filename[0]!= 0x00)&&(direntry.attributes != 0x0F)&&((direntry.attributes & 0x10 )!= 1)&&((direntry.attributes & 0x02) !=1)){
			if((strncmp(found_filename, desired_filename, strlen(found_filename))==0) && (strncmp(found_extension, desired_extension, strlen(found_extension))==0)){
				printf("There is already a file of the same name in the disk.\n");
				exit(0);
			}
		}
	}
	//if we get here, then the file was not in the disk which is what we want. 
	

	//get Filesize and total num sectors required to store it:
	fseek(readFile, 0 , SEEK_END);
  	long int fileSize = ftell(readFile);
	printf("size of file to copy: %ld bytes\n", fileSize);

	//check that we have room to store the file:
	long int freeSpace = (getFreeSpace(argv[1], bpsector))*bpsector;
	if(fileSize >= freeSpace){
		printf("Not enough free space in the disk image.");
	}

	printf("File successfully copied\n");

	fclose(diskimage);
	exit(0);
}
