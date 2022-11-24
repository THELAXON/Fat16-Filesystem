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
    printf("Number of copies of FAT: %hu\n",bsp->BPB_NumFATs);
    printf("FAT12/FAT16: size of root DIR: %hu\n",bsp->BPB_RootEntCnt);
    printf("Sectors, may be 0, see below: %hu\n",bsp->BPB_TotSec16);
    printf("Sectors in FAT(FAT12 OR FAT16):%hu\n",bsp ->BPB_FATSz16);
    printf("Sectors if BPB_TOtSec16 == 0: %hu\n",bsp->BPB_TotSec32);
    printf("Non zero terminated String: %s\n",bsp->BS_VolLab);
    printf("\n");


    uint16_t* fat = (uint16_t*) malloc(bsp->BPB_FATSz16);
    lseek(fd,bsp->BPB_RsvdSecCnt* bsp->BPB_BytsPerSec,SEEK_CUR);
    read(fd,fat,bsp->BPB_FATSz16);

    for(int i= 0; i < bsp->BPB_FATSz16; i++)
    {
      printf("Cluster:%hu\n", fat[i]);
    }
    printf("\n");

    lseek(fd,(bsp->BPB_RsvdSecCnt + bsp->BPB_NumFATs * bsp->BPB_FATSz16) * bsp->BPB_BytsPerSec ,SEEK_SET);

    RootDirectory* rd[(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3];

    for(int i=0;i<(bsp->BPB_RootEntCnt/bsp->BPB_FATSz16)+3;i++)
    {
      rd[i] = (RootDirectory*) malloc(sizeof(RootDirectory));
      read(fd,rd[i], sizeof(RootDirectory));
      printf("DIR NAME: %s\n",rd[i]->DIR_Name);
      printf("DIR Attr: %hu\n",rd[i]->DIR_Attr);
      printf("Last read or write: %hu\n",rd[i]->DIR_LstAccDate);
      printf("Top 16 bits file's 1st cluster: %d\n",rd[i]->DIR_FstClusHI);
      printf("Time of last write: %hu\n",rd[i]->DIR_WrtTime);
      printf("Date of last write: %hu\n",rd[i]->DIR_WrtDate);                                             
      printf("File Size in bytes: %hu\n",rd[i]->DIR_FileSize);
      printf("\n");
    }

    // uint16_t* fat = (uint16_t*) malloc(bsp->BPB_FATSz16);
    // lseek(fd,bsp->BPB_RsvdSecCnt* bsp->BPB_BytsPerSec,SEEK_CUR);
    // read(fd,fat,bsp->BPB_FATSz16);

    // for(int i= 0; i < bsp->BPB_FATSz16; i++)
    // {
    //   printf("Cluster:%hu\n", fat[i]);
    // }

    // uint16_t* fat = (uint16_t*) malloc(bsp->BPB_FATSz16*bsp->BPB_BytsPerSec);
    // lseek(fd,bsp->BPB_BytsPerSec * bsp->BPB_RsvdSecCnt,SEEK_CUR);
    // read(fd,fat,bsp->BPB_BytsPerSec*bsp->BPB_FATSz16);

    // for(int i= 0; i < ; i++)
    // {
    //   printf("Cluster:%hu\n", fat[i]);
    // }

    //RootDirectory* rd = (RootDirectory*) malloc(sizeof(RootDirectory));
    //lseek(fd,(bsp->BPB_RsvdSecCnt + bsp->BPB_NumFATs * bsp->BPB_FATSz16) * bsp->BPB_BytsPerSec ,SEEK_SET);
    //read(fd,rd, sizeof(RootDirectory));
    //printf("DIR NAME: %s\n",rd->DIR_Name);
    //printf("DIR Attr: %hu\n",rd->DIR_Attr);
    //printf("Used by windows: %hu\n",rd->DIR_NTRes);
    //printf("Creation Time in tenths of seconds: %hu\n",rd->DIR_CrtTimeTenth);
    //printf("Creation Time in seconds: %hu\n",rd->DIR_CrtTime);
    //printf("Creation Date: %hu\n",rd->DIR_CrtDate);
    //printf("Last read or write: %hu\n",rd->DIR_LstAccDate);
    //printf("Top 16 bits file's 1st cluster: %d\n",rd->DIR_FstClusHI);
    //printf("Time of last write: %hu\n",rd->DIR_WrtTime);
    //printf("Date of last write: %hu\n",rd->DIR_WrtDate);
    //printf("Lower 16 bits file's 1st cluster: %d\n",rd->DIR_FstClusLO);                                              
    //printf("File Size in bytes: %hu\n",rd->DIR_FileSize);

    
    
}