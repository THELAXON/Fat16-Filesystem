#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct __attribute__((__packed__)) {
uint8_t BS_jmpBoot[ 3 ]; // x86 jump instr. to boot code
uint8_t BS_OEMName[ 8 ]; // What created the filesystem
uint16_t BPB_BytsPerSec; // Bytes per Sector
uint8_t BPB_SecPerClus; // Sectors per Cluster
uint16_t BPB_RsvdSecCnt; // Reserved Sector Count
uint8_t BPB_NumFATs; // Number of copies of FAT 
uint16_t BPB_RootEntCnt; // FAT12/FAT16: size of root DIR
uint16_t BPB_TotSec16; // Sectors, may be 0, see below
uint8_t BPB_Media; // Media type, e.g. fixed
uint16_t BPB_FATSz16; // Sectors in FAT (FAT12 or FAT16)
uint16_t BPB_SecPerTrk; // Sectors per Track
uint16_t BPB_NumHeads; // Number of heads in disk
uint32_t BPB_HiddSec; // Hidden Sector count
uint32_t BPB_TotSec32; // Sectors if BPB_TotSec16 == 0
uint8_t BS_DrvNum; // 0 = floppy, 0x80 = hard disk
uint8_t BS_Reserved1; //
uint8_t BS_BootSig; // Should = 0x29
uint32_t BS_VolID; // 'Unique' ID for volume
uint8_t BS_VolLab[ 11 ]; // Non zero terminated string
uint8_t BS_FilSysType[ 8 ]; // e.g. 'FAT16' (Not 0 term.)
} BootSector;

typedef struct __attribute__((__packed__)) {
uint8_t DIR_Name[ 11 ]; // Non zero terminated string
uint8_t DIR_Attr; // File attributes
uint8_t DIR_NTRes; // Used by Windows NT, ignore
uint8_t DIR_CrtTimeTenth; // Tenths of sec. 0...199
uint16_t DIR_CrtTime; // Creation Time in 2s intervals
uint16_t DIR_CrtDate; // Date file created
uint16_t DIR_LstAccDate; // Date of last read or write
uint16_t DIR_FstClusHI; // Top 16 bits file's 1st cluster
uint16_t DIR_WrtTime; // Time of last write
uint16_t DIR_WrtDate; // Date of last write
uint16_t DIR_FstClusLO; // Lower 16 bits file's 1st cluster
uint32_t DIR_FileSize; // File size in bytes
} Directory;

int fd; //declaring file reader
uint16_t* fat; // declaring the fat array
uint16_t fat_offset; // The fat offset to get to the Fat block
uint16_t rootdir_offset;//The root directory offset to get to the root directory
uint16_t data_offset; //The data offset to get to the data region
BootSector* bsp; // Bootsector array
Directory* rd[219]; // size of root directory given to array
int sub;


void task3() // Task to print out the cluster chain in the fat table
{
  for(int i= 0; i < 2458; i++)
    {
      printf("Cluster:%hu\n", fat[i]);
    }
    printf("\n");
}

void printclusterchain(unsigned int cluster)
{
  int x = cluster;
  while(cluster < 0xfff8)
  {
    printf("The cluster given:%hu\n",cluster);
    cluster= fat[cluster];
  }
}

void task4() // Prints the table for task 4 and only present the the directories which have the first three bits set to 1 while the directory and archive is set to 0
{
    printf("---------------------------------------------------------------------------------------------------------\n");
    printf("|  First Cluster  |  Last Modified  |    Time    |        Attributes         |  File Size  |   Dir Name |\n");
    printf("---------------------------------------------------------------------------------------------------------\n");
    for(int i=0;i<(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3;i++)
    {
      unsigned int readonly = (rd[i]->DIR_Attr & 1);
      unsigned int hidden = (rd[i]->DIR_Attr & 2) >> 1;
      unsigned int system = (rd[i]->DIR_Attr & 4) >> 2;
      unsigned int volumename = (rd[i]->DIR_Attr & 8) >> 3;
      unsigned int directory = (rd[i]->DIR_Attr & 16) >> 4;
      unsigned int archive = (rd[i]->DIR_Attr & 32) >> 5;

      unsigned int day = (rd[i]->DIR_WrtDate & 31);
      unsigned int month = ((rd[i]->DIR_WrtDate & 480) >> 5);
      unsigned int year = (rd[i]->DIR_WrtDate >> 9)+1980;
      unsigned int sec = ((rd[i]->DIR_WrtTime & 31)*2);
      unsigned int min = ((rd[i]->DIR_WrtTime & 2016) >> 5);
      unsigned int hour = (rd[i]->DIR_WrtTime >> 11);

      if(!(readonly == 1 && hidden == 1 && system == 1 && volumename == 1 && directory == 0 && archive == 0)) // checks if it is the given 
      {
        printf("|%.4d             |   %hu-%hu-%hu    |  %.2hu-%.2hu-%.2hu  |  A-%hu D-%hu V-%hu S-%hu H-%hu R-%hu  | %.4hu        | %.11s|\n",rd[i]->DIR_FstClusLO,year,month,day,hour,min,sec,archive,directory,volumename,system,hidden,readonly,rd[i]->DIR_FileSize,rd[i]->DIR_Name);
        printf("---------------------------------------------------------------------------------------------------------");                                     
        printf("\n");
      }
    }
  
}

void task5()
{
  /*Bitmasking and bitshifting the  directory attributes to check if it is a text file and then printing the text file for task 5*/
  for(int i=0;i<(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3;i++)
    {
      unsigned int readonly = (rd[i]->DIR_Attr & 1);             
      unsigned int hidden = (rd[i]->DIR_Attr & 2) >> 1;
      unsigned int system = (rd[i]->DIR_Attr & 4) >> 2;
      unsigned int volumename = (rd[i]->DIR_Attr & 8) >> 3;
      unsigned int directory = (rd[i]->DIR_Attr & 16) >> 4;
      unsigned int archive = (rd[i]->DIR_Attr & 32) >> 5;

      if((readonly == 0) && (hidden == 0) && (system == 0) && (volumename == 0) && (directory == 0) && (archive == 1))
      {
        unsigned int cluster = rd[i]->DIR_FstClusLO;
        char* buffer = (char*) malloc(bsp->BPB_BytsPerSec* bsp->BPB_SecPerClus);
        while(cluster < 0xfff8)
        {
          lseek(fd,data_offset+((cluster-2)*bsp->BPB_BytsPerSec* bsp->BPB_SecPerClus),SEEK_SET);
          read(fd,buffer,bsp->BPB_BytsPerSec* bsp->BPB_SecPerClus);
          printf("%s",buffer);
          cluster= fat[cluster];
        }
      }
    }
}

void openFile(unsigned int volume,unsigned int ShortDirEntry) // Opens file using given cluster number as short directory entry and the buffer size using volume to print the contents of the file
{
    char* buffer = (char*) malloc(volume);
    while(ShortDirEntry < 0xfff8)
    {
        lseek(fd,data_offset+((ShortDirEntry-2)*bsp->BPB_BytsPerSec* bsp->BPB_SecPerClus),SEEK_SET);
        read(fd,buffer,volume);
        printf("%s",buffer);
        ShortDirEntry = fat[ShortDirEntry];
    }
}


int main(int argc, char *agrv[])
{
    fd = open("fat16.img", O_RDONLY); // opens the file

    if(fd < 0)  // if the file isn't there it picks up this error message
    {
        printf("Failed to read file.\n");
        exit(1);
    }
    bsp =(BootSector*)malloc(sizeof(BootSector)); // creating boot sector pointers
    read(fd,bsp, sizeof(BootSector)); // read the boot sector as it is the first block in fat to read

    printf("OEM NAME: %s\n",bsp->BS_OEMName);       // Prints contents given in the boot sector for later use
    printf("Bytes per sector: %hu\n",bsp->BPB_BytsPerSec);
    printf("Sectors per cluster: %hu\n",bsp->BPB_SecPerClus);
    printf("Reserved Sector Count: %hu\n",bsp->BPB_RsvdSecCnt);
    printf("Number of FATs: %hu\n",bsp->BPB_NumFATs);
    printf("Size of root DIR: %hu\n",bsp->BPB_RootEntCnt);
    printf("Total Sector count: %hu\n",bsp->BPB_TotSec16);
    printf("Sectors in FAT: %hu\n",bsp ->BPB_FATSz16);
    printf("Sectors if Total Sector count is 0 : %hu\n",bsp->BPB_TotSec32);
    printf("Non zero terminated String: %s\n",bsp->BS_VolLab);
    printf("\n");

    fat_offset = bsp->BPB_RsvdSecCnt* bsp->BPB_BytsPerSec;  // offset iniated for the fat
    rootdir_offset = (bsp->BPB_RsvdSecCnt + bsp->BPB_NumFATs * bsp->BPB_FATSz16) * bsp->BPB_BytsPerSec; // offset iniated for root directory
    data_offset = rootdir_offset+ (bsp->BPB_RootEntCnt*32); // offset iniated for the data region offset

    fat = (uint16_t*) malloc(bsp->BPB_FATSz16*bsp->BPB_BytsPerSec); // reading only 1 fat as the second is a backup incase of corrupt fat table
    lseek(fd,fat_offset,SEEK_SET);   // lseek from the start to check for the fat table
    read(fd,fat,bsp->BPB_FATSz16*bsp->BPB_BytsPerSec);  // reading fat region to fat table

    lseek(fd,rootdir_offset,SEEK_SET);  // lseek to root directory offset to used to print he root directories out

  
    for(int i=0;i<(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3;i++)  // fill the root directories array to print out contents of the root directory
    {
      rd[i] = (Directory*) malloc(sizeof(Directory));
      read(fd,rd[i], sizeof(Directory));
    } 
    task3(); // prints cluster chain
    task4(); // prints the files and folders in root directory
    task5(); // prints the contents of files in root directory
    //openFile(302,1342); // opens a file based on the cluster number given and the number of bytes wanted to be read in terminal
    printf("\n");
    //printclusterchain(350);
}