#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "dirinfo_def.h"


int getFatEntry(char filename[], int n, int bpsector) {
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
   
	fseek(imageptr, bpsector, SEEK_SET); // FAT1 at sector 1
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



int main(int argc, char *argv[])
{	
	if( argc == 3 ) {
    	printf("The arguments supplied are DiskImage: %s and FileName: %s\n", argv[1], argv[2]);
		if(strchr(argv[2], '.') == NULL){
			printf("Incorrectly formatted filename.\n");
			exit(0);
		}

   	}
   	else if( argc > 3 ) {
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
		printf("program usage: \"./diskget disk.IMA <filename>\" \n");
		exit(0);
	}

	//image successfully opened. 
	fread(&boot_sector, sizeof(boot_sector), 1, diskimage);
	int bpsector = boot_sector.bytes_per_sector;

	//move to start of root directory
	fseek(diskimage, 19*bpsector, SEEK_SET);

	//must search all of root directory for the desired file

	//split the requested filename/extension for comparison purposes.
	char desired_filename[9]={'\0'};
	char desired_extension[4]={'\0'};
	char found_filename[9]={'\0'};
	char found_extension[4]={'\0'};
   	/* get the first token */
	char buffer[20]={'\0'};
	strcpy(buffer, argv[2]);
	
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
		printf("Incorrectly formatted filename!\n");
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
		//first clear both of remaining characters
		for(int a =0; a<9; a++){
			found_filename[a] = '\0';
		}
		for(int b =0; b<4; b++){
			found_extension[b] = '\0';
		}

		for(int j =0; j<8; j++){
			if(direntry.filename[j] != ' '){
				found_filename[j] = direntry.filename[j];
			}
		}
		// printf("found filename: %s\n", found_filename);
		for(int k =0; k<3; k++){
			if(direntry.extension[k] != ' '){
				found_extension[k] = direntry.extension[k];
			}
		}
		// printf("found extension: %s\n", found_extension);


		if((found_filename[0] != 0xE5)&&(found_filename[0]!= 0x00)&&(direntry.attributes != 0x0F)&&((direntry.attributes & 0x10 )!= 0x10)&&((direntry.attributes & 0x02) != 0x02)){
			// printf("here1\n");
			if((strncmp(found_filename, desired_filename, strlen(found_filename))==0) && (strncmp(found_extension, desired_extension, strlen(found_extension))==0)){
				printf("the desired file has been found\n");
				break;
			}
		}
		
		if(i==224){
			printf("File not found.\n");
			exit(0);
		}
	}
	//direntry is now the start of the file we want to copy
	//check to make sure the file we need is not already in current directory:
	FILE *testwrite = fopen(argv[2], "rb");
	if(testwrite != NULL){
		printf("There is already a file of that name in the current directory.\n");
		exit(0);
	}
	//we know it does not exist now.
	FILE *writefile = fopen(argv[2], "wb");
	if(writefile == NULL){
		printf("Error creating write file.\n");
		exit(0);
	}

	//get Filesize and total num sectors required to store it:
	int size = direntry.size;
	printf("size of file to copy: %d\n", size);
	int numsectors = size / bpsector + (size % bpsector != 0);

	int cursectors = 0;
	char sectorData[bpsector];
	char* finalsector=malloc(sizeof(char)*bpsector);
	for (int j = 0; j<bpsector; j++){
		finalsector[j] = '\0';
	}

	int flcluster = 0;
	int curcluster = 0;
	flcluster = direntry.cluster;

	//read one sector at a time until end of file.
	while((cursectors < numsectors-1) && curcluster != 0xFFF){
		if(cursectors == 0){
			//if its the first sector of data to pull, use flc not the fat table return val
			fseek(diskimage, (flcluster+31)*bpsector, SEEK_SET);
			curcluster = getFatEntry(argv[1], flcluster, bpsector);
		}
		else{
			//for every other sector use the fat val for cluster
			fseek( diskimage, (curcluster+31)*bpsector, SEEK_SET);
			curcluster = getFatEntry(argv[1], curcluster, bpsector);
		}
		fread(sectorData, 1, bpsector, diskimage);
		//write sector data to new file
		fwrite(sectorData, sizeof(sectorData), 1, writefile);

		//go to FLC-th entry in FAT table to get new sector		
		cursectors += 1;

	}
	//now we get the remaining bits from the final sector (if the file had more than one sector)
	if(numsectors > 1){
		fseek(diskimage, (curcluster+31)*bpsector, SEEK_SET);
		fread(finalsector, 1, (size-(bpsector*(numsectors-1))), diskimage);
		fwrite(finalsector, (size-(bpsector*(numsectors-1))), 1, writefile);
	}
	else{
		//else, the size of the file was less than one sector so we need a different read size formula
		fseek(diskimage, (flcluster+31)*bpsector, SEEK_SET);
		fread(finalsector, 1, size, diskimage);

		curcluster = getFatEntry(argv[1], curcluster, bpsector);
		fwrite(finalsector, (size), 1, writefile);
	}

	printf("File successfully copied!\n");


	fclose(diskimage);
	free(finalsector);
	exit(0);
}
