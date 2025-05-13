#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "dirinfo_def.h"

#include <sys/stat.h>
#include <ctype.h>


int getFatEntry(char filename[], int n, int bpsector) {
	// returns the n-th fat entry
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


void list_files(entry_t direntry, char imagename[], int n, int bpsector){
	//lists the file information given a directory.
	FILE *diskimage = fopen(imagename, "rb");

	char cluster[bpsector];
	char attributes;
	int num_files = 0;
	int flcluster = 0;
	int directorybit = 0;
	int isfree = 0;
	int physcluster =0;

	unsigned int filesize_int = 0;

	unsigned int day = 0;
	unsigned int month = 0;
	unsigned int year = 0;

	unsigned int hour = 0;
	unsigned int minute = 0;

	char isdir = 'N';


	
	//obtain and print info about file/directory
	flcluster = direntry.cluster & 0xFF;
	flcluster += (direntry.cluster & 0x00FF)<< 8;
	directorybit = direntry.attributes & 0x10;
	isfree = direntry.filename[0];

	if((direntry.attributes == 0x0F) || (isfree == 0xE5) || (direntry.attributes == 0x08)){
		//in these cases we ignore. part of long file name? or is marked as free (0xE5) or is vol label (0x08)
		return;
		
	}
	else if(isfree == 0x00){
		//in this case, the current directory entry is free as well as all remaining entries in this directory
		return;
	}
	else if(directorybit == 0x10){
		//Then the current entry describes a subdirectory
		if((flcluster == 0) || (flcluster == 1)){
			//in this case, the file/subdir is within reserved area, so ignore. 
			return;
		}
		isdir = 'D';
	}
	else{
		//current entry is a file to be printed
		isdir = 'F';
	}

		unsigned int mask;
		mask = 0b0000000000011111;
		day = direntry.create_date & mask;
		mask = 0b0000000111100000;
		month = direntry.create_date & mask;
		month = month >> 5;
		mask = 0b1111111000000000;
		year = direntry.create_date & mask;
		year = year >> 9;
		year = year + 1980;



		
		mask = 0b0000011111100000;
		minute = direntry.create_time & mask;
		minute = minute >> 5;
		mask = 0b1111100000000000;
		hour = direntry.create_time & mask;
		hour = hour >> 11;
		

		char filename[9] = {'\0'};
		strncpy(filename, direntry.filename, 8);
		// Trim trailing space
		char* end = filename + strlen(filename) - 1;
		while(end > filename && isspace((unsigned char)*end)) end--;
		// Write new null terminator character
		end[1] = '\0';

		char extension[4] = {'\0'};
		strncpy(extension, direntry.extension, 3);
		// Trim trailing space
		end = extension + strlen(extension) - 1;
		while(end > extension && isspace((unsigned char)*end)) end--;
		// Write new null terminator character
		end[1] = '\0';

		char finalname[13] = {'\0'};
		sprintf(finalname, "%s.%s", filename, extension);
		if(finalname[0] == '.'){
			return;
		}
		
		for(int i=0; i<n; i++){
			printf("	");
		}
		
		printf("%c %10d %20s %d/%.2d/%.2d %.2d:%.2d\n", isdir, direntry.size, finalname, year, month, day, hour, minute);


		//now we handle its subdirectories...
		if(directorybit == 0x10){
			for(int i=0; i<n; i++){
				printf("	");
			}
			printf("==================\n");
			//use first logical cluster number to find physical cluser number (x+31)
			physcluster = direntry.cluster + 31;

			//check x-th (flc-th) FAT entry to see if 0xFFF (end of file)
			
			//read in cluster from phys location
			fseek(diskimage, physcluster*bpsector, SEEK_SET);
			//every 32 bytes is a direntry pointing to another file or directory. call recursively.
			entry_t recurentry;
			//512 bytes, 32 bytes per entries = 16 possible entries to search.
			for(int i = 0; i<16; i++){
				fread(&recurentry, sizeof(recurentry), 1, diskimage);
				int fatEntry = getFatEntry(imagename, direntry.cluster, bpsector);
				if((fatEntry == 0xFFF)){
					//do nothing
				}
				if(recurentry.filename[0] != '.'){
					n += 1;
					list_files(recurentry, imagename, n, bpsector);
				}
			}
		}
		
		
	

	fclose(diskimage);
	return;
}


int main(int argc, char *argv[])
{	
	if( argc == 2 ) {
    	printf("Searching %s\n", argv[1]);

   	}
   	else if( argc > 2 ) {
    	printf("Too many arguments supplied.\n");
		exit(0);
   	}
   	else if (argc == 1){
    	printf("Too few arguments provided. Please provide disk image to search.\n");
		exit(0);
	}
	//open disk image in binary read mode
	FILE *diskimage = fopen(argv[1], "rb");
	boot_t  boot_sector;
	//check if open successful
	if(diskimage == NULL){
		printf("Failed to open disk image file...\n");
		exit(0);
	}

	//image successfully opened. 
	fread(&boot_sector, sizeof(boot_sector), 1, diskimage);

	//get bpsector
	int bpsector = boot_sector.bytes_per_sector;

	//move to the root directory
	fseek(diskimage, 19*bpsector, SEEK_SET);
	printf("Root Directory\n");
	printf("==================\n");
	entry_t direntry;
	//14 sectors, each with 16 entries = 224 possible entries to search.
	for(int i = 0; i<(14*16); i++){
		//for the whole of the root directory, obtain directory entry
		fread(&direntry, sizeof(direntry), 1, diskimage);
		list_files(direntry, argv[1], 0, bpsector);
	}
	



	fclose(diskimage);
	exit(0);
}

