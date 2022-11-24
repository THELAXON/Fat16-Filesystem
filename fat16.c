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
} RootDirectory;

int main(int argc, char *agrv[])
{

    int fd = open("fat16.img", O_RDONLY);
    if(fd < 0)
    {
        printf("Failed to read file.\n");
        exit(1);
    }
    BootSector* bsp =(BootSector*)malloc(sizeof(BootSector));
    read(fd,bsp, sizeof(BootSector));

    printf("OEM NAME: %s\n",bsp->BS_OEMName);
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


    uint16_t* fat = (uint16_t*) malloc(bsp->BPB_FATSz16*bsp->BPB_BytsPerSec);
    lseek(fd,bsp->BPB_RsvdSecCnt* bsp->BPB_BytsPerSec,SEEK_SET);
    read(fd,fat,bsp->BPB_FATSz16*bsp->BPB_BytsPerSec);

    // for(int i= 0; i < bsp->BPB_FATSz16; i++)
    // {
    //   printf("Cluster:%hu\n", fat[i]);
    // }
    // printf("\n");

    lseek(fd,(bsp->BPB_RsvdSecCnt + bsp->BPB_NumFATs * bsp->BPB_FATSz16) * bsp->BPB_BytsPerSec ,SEEK_SET);

    RootDirectory* rd[(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3];

    for(int i=0;i<(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3;i++)
    {
      rd[i] = (RootDirectory*) malloc(sizeof(RootDirectory));
      read(fd,rd[i], sizeof(RootDirectory));

      unsigned int readonly = (rd[i]->DIR_Attr & 1);
      unsigned int hidden = (rd[i]->DIR_Attr & 2) >> 1;
      unsigned int system = (rd[i]->DIR_Attr & 4) >> 2;
      unsigned int volumename = (rd[i]->DIR_Attr & 8) >> 3;
      unsigned int directory = (rd[i]->DIR_Attr & 16) >> 4;
      unsigned int archieve = (rd[i]->DIR_Attr & 32) >> 5;

      unsigned int day = (rd[i]->DIR_WrtDate & 31);
      unsigned int month = ((rd[i]->DIR_WrtDate & 480) >> 5);
      unsigned int year = (((rd[i]->DIR_WrtDate & 65024) >> 9)+1980);
      unsigned int sec = ((rd[i]->DIR_WrtTime & 31) * 2);
      unsigned int min = ((rd[i]->DIR_WrtTime & 2016) >> 5);
      unsigned int hour = ((rd[i]->DIR_WrtTime & 63488) >> 11);

      if((readonly == 1) && (hidden == 1) && (system == 1) && (volumename == 1) && (directory == 0) && (archieve == 0))
      {
        continue;
      }
      else
      {
        printf("First Cluster: %d Last Modified: %hu-%hu-%hu Time: %hu-%hu-%hu Attributes-> A-%hu D-%hu V-%hu S-%hu H-%hu R-%hu File size:%hu DIR NAME: %s\n",rd[i]->DIR_FstClusLO,year,month,day,hour,min,sec,archieve,directory,volumename,system,hidden,readonly,rd[i]->DIR_FileSize,rd[i]->DIR_Name);                                     
        printf("\n");
      }
    }

}
