#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "dirinfo_def.h"

int getFreeSpace(char imagename[], int bpsector) {
	// returns the number of free sectors in the disk image
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



int num_files_fromfat(FILE *fp, int bpsector) {
	//obtains the number of files on the disk.

   int i, num_files = 0;
   uint8_t X0, X1, X2;
   uint16_t X, Y;
   fat_entry_t fat_entry;
   
   fseek(fp, bpsector, SEEK_SET); /* FAT1 at sector 1, but skip first two fat reserved */
   fread(&fat_entry, sizeof(fat_entry), 1, fp);
   fread(&fat_entry, sizeof(fat_entry), 1, fp);
   for (i=0; i< (((9)*bpsector)/3)-2; i++) {
      fread(&fat_entry, sizeof(fat_entry), 1, fp);
      X0 = fat_entry.b0;
      X1 = fat_entry.b1;
      X2 = fat_entry.b2;
      
      X = ((X1 & 0x0F) << 8) + X0 ;
      Y = ((X1 & 0xF0) >> 4) + (X2 << 4);
      
      if (X >= 0xFF8) {
         num_files += 1;  /* for testing, let's count how many sectors */
      }
      if (Y >= 0xFF8) {
         num_files += 1;
      }
   }
   return num_files;
}


int main(int argc, char *argv[])
{	
	if( argc == 2 ) {
		//correct number of arguments
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
   	
	//get os name
	char osystem[9]={'\0'};
	fseek(diskimage, 3, SEEK_SET);
	fread(osystem, 1, 8, diskimage);
	printf("OS Name: %s\n", osystem);

	//get volume label from the root directory
	int bpsector = boot_sector.bytes_per_sector;
	char vol_label[11]={'\0'};
	char attribute[2];
	attribute[1] = '\0';
	char entry[32];
	//set curattr to first attribute in RD
	int curentry = 19*bpsector;
	
	int entry_num = 1;
	while((entry_num < 16*14)){
		//move to the Root directory
		fseek(diskimage, curentry, SEEK_SET);
		//read one entry from root directory in its entirety
		fread(entry, 1, 32, diskimage);
		//obtain attribute byte from current entry
		attribute[0] = entry[11];
		//check if label is in fact the volume label
		if((int)attribute[0] == 0x08){
			strncpy(vol_label, entry, 8);
			vol_label[9] = '\0';
			break;
		}
		//if not, move to next entry
		curentry += 32;
		entry_num += 1;
	}
	printf("Label of the disk: %s\n", vol_label);



	//get total disk size from sector count
	int total_sectors = boot_sector.total_sectors;
	printf("Total size of the disk: %d bytes\n", total_sectors*bpsector);
	fseek(diskimage, 0, SEEK_SET);
	printf("Free size of the disk: %d bytes\n", getFreeSpace(argv[1], bpsector)*bpsector);
	
	printf("The number of files in the disk: %d\n", num_files_fromfat(diskimage, bpsector));

	printf("Number of FAT copies: %d\n", boot_sector.fats);
	printf("Sectors per FAT: %d\n", boot_sector.sectors_per_fat);


	fclose(diskimage);
	exit(0);
}

