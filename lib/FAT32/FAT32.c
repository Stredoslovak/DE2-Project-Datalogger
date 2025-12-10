//************************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD ****
//************************************************************
//Controller    : ATmega32 (Clock: 8 Mhz-internal)
//Compiler      : AVR-GCC (winAVR with AVRStudio-4)
//Project Version : DL_1.0
//Author      : CC Dharmani, Chennai (India)
//              [www.dharmanitech.com](https://www.dharmanitech.com)
//Date        : 10 May 2011
//************************************************************


#include <avr/io.h>
#include <avr/pgmspace.h>
#include "FAT32.h"
#include "uart.h"
#include "sd_routines.h"
#include "rtc.h"
#include "uart_compat.h"


/**
 * @brief  Read boot sector data from SD card to determine FAT32 parameters.
 * 
 * Reads sector 0 to check for boot sector signature. If not found, reads MBR
 * to locate partition information. Extracts critical FAT32 parameters including
 * bytes per sector, sectors per cluster, reserved sectors, root cluster, and
 * total number of clusters available on the card.
 * 
 * @return 0 on success, 1 if boot sector not found or invalid MBR signature.
 */
unsigned char getBootSectorData (void)
{
struct BS_Structure *bpb;
struct MBRinfo_Structure *mbr;
struct partitionInfo_Structure *partition;
unsigned long dataSectors;

unusedSectors = 0;

SD_readSingleBlock(0);
bpb = (struct BS_Structure *)buffer;

if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)
{
  mbr = (struct MBRinfo_Structure *) buffer;
  
  if(mbr->signature != 0xaa55) return 1;
    
  partition = (struct partitionInfo_Structure *)(mbr->partitionData);
  unusedSectors = partition->firstSector;
  
  SD_readSingleBlock(partition->firstSector);
  bpb = (struct BS_Structure *)buffer;
  if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) return 1; 
}

bytesPerSector = bpb->bytesPerSector;
sectorPerCluster = bpb->sectorPerCluster;
reservedSectorCount = bpb->reservedSectorCount;
rootCluster = bpb->rootCluster;
firstDataSector = bpb->hiddenSectors + reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);

dataSectors = bpb->totalSectors_F32
              - bpb->reservedSectorCount
              - ( bpb->numberofFATs * bpb->FATsize_F32);
totalClusters = dataSectors / sectorPerCluster;

if((getSetFreeCluster (TOTAL_FREE, GET, 0)) > totalClusters)
     freeClusterCountUpdated = 0;
else
   freeClusterCountUpdated = 1;
return 0;
}


/**
 * @brief  Calculate first sector address of a given cluster.
 * 
 * Uses the formula: ((clusterNumber - 2) * sectorPerCluster) + firstDataSector
 * to determine the absolute sector number for the first sector of any cluster
 * on the SD card.
 * 
 * @param  clusterNumber Cluster number for which first sector is to be found.
 * @return First sector address (absolute sector number) of the cluster.
 */
unsigned long getFirstSector(unsigned long clusterNumber)
{
  return (((clusterNumber - 2) * sectorPerCluster) + firstDataSector);
}


/**
 * @brief  Get or set cluster entry value in FAT to navigate or modify cluster chain.
 * 
 * Locates the FAT entry for a given cluster, retrieves it if GET operation,
 * or updates it if SET operation. Each FAT entry is 4 bytes (32-bit) and contains
 * the next cluster number in the chain (or EOF marker).
 * 
 * @param  clusterNumber Current cluster number to access in FAT.
 * @param  get_set       GET to retrieve next cluster, SET to modify FAT entry.
 * @param  clusterEntry  Next cluster value when get_set=SET; 0 when get_set=GET.
 * @return Next cluster number if get_set=GET; 0 if get_set=SET.
 */
unsigned long getSetNextCluster (unsigned long clusterNumber,
                                 unsigned char get_set,
                                 unsigned long clusterEntry)
{
unsigned int FATEntryOffset;
unsigned long *FATEntryValue;
unsigned long FATEntrySector;
unsigned char retry = 0;

FATEntrySector = unusedSectors + reservedSectorCount + ((clusterNumber * 4) / bytesPerSector);

FATEntryOffset = (unsigned int) ((clusterNumber * 4) % bytesPerSector);

while(retry <10)
{ if(!SD_readSingleBlock(FATEntrySector)) break; retry++;}

FATEntryValue = (unsigned long *) &buffer[FATEntryOffset];

if(get_set == GET)
  return ((*FATEntryValue) & 0x0fffffff);

*FATEntryValue = clusterEntry;

SD_writeSingleBlock(FATEntrySector);

return (0);
}


/**
 * @brief  Get or set free cluster information from FSinfo sector.
 * 
 * Accesses the FSinfo sector (sector 1) to read or update either the total
 * free cluster count or the next available free cluster pointer. Validates
 * FSinfo sector signatures before reading/writing data.
 * 
 * @param  totOrNext TOTAL_FREE to access free cluster count, NEXT_FREE for next free cluster pointer.
 * @param  get_set   GET to read value, SET to update FSinfo sector.
 * @param  FSEntry   New value when get_set=SET; 0 when get_set=GET.
 * @return Requested value if get_set=GET; 0xffffffff on error or if get_set=SET.
 */
unsigned long getSetFreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry)
{
struct FSInfo_Structure *FS = (struct FSInfo_Structure *) &buffer;
unsigned char error;
SD_readSingleBlock(unusedSectors + 1);

if((FS->leadSignature != 0x41615252) || (FS->structureSignature != 0x61417272) || (FS->trailSignature !=0xaa550000))
  return 0xffffffff;

 if(get_set == GET)
 {
   if(totOrNext == TOTAL_FREE)
      return(FS->freeClusterCount);
   else
      return(FS->nextFreeCluster);
 }
 else
 {
   if(totOrNext == TOTAL_FREE)
      FS->freeClusterCount = FSEntry;
   else
    FS->nextFreeCluster = FSEntry;
 
   error = SD_writeSingleBlock(unusedSectors + 1);
 }
 return 0xffffffff;
}


/**
 * @brief  Search for files/directories, retrieve file address, or delete specified file.
 * 
 * Traverses the root directory cluster chain and searches directory entries.
 * Can list all files (GET_LIST), locate a specific file (GET_FILE), or delete
 * a file (DELETE). For GET_FILE, returns pointer to directory entry and stores
 * sector/location info for later append operations.
 * 
 * @param  flag     GET_LIST to list directory contents, GET_FILE to find file, DELETE to remove file.
 * @param  fileName Pointer to file name (NULL/unused if flag=GET_LIST).
 * @return Pointer to directory entry structure if found, NULL if not found or on error.
 */
struct dir_Structure* findFiles (unsigned char flag, unsigned char *fileName)
{
unsigned long cluster, sector, firstSector, firstCluster, nextCluster;
struct dir_Structure *dir;
unsigned int i;
unsigned char j;
unsigned char loopCount = 0;

cluster = rootCluster;

while(1)
{
   loopCount++;
   if(loopCount > 100) {
       return 0;
   }
   
   firstSector = getFirstSector (cluster);
   
   for(sector = 0; sector < sectorPerCluster; sector++)
   {
     SD_readSingleBlock (firstSector + sector);

     for(i=0; i<bytesPerSector; i+=32)
     {
        dir = (struct dir_Structure *) &buffer[i];

        if(dir->name[0] == EMPTY)
        {
          if(flag == DELETE)
              return 0;
          return 0;   
        }
    if((dir->name[0] != DELETED) && (dir->attrib != ATTR_LONG_NAME))
        {
          if((flag == GET_FILE) || (flag == DELETE))
          {
            for(j=0; j<11; j++)
            if(dir->name[j] != fileName[j]) break;
            if(j == 11)
            {
              if(flag == GET_FILE)
                    {
                appendFileSector = firstSector + sector;
              appendFileLocation = i;
              appendStartCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
              fileSize = dir->fileSize;
                return (dir);
              } 
              else
              {
                TX_NEWLINE;
              TX_NEWLINE;
              TX_NEWLINE;
              firstCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
                      
              dir->name[0] = DELETED;    
              SD_writeSingleBlock (firstSector+sector);
                    
              freeMemoryUpdate (ADD, dir->fileSize);

              cluster = getSetFreeCluster (NEXT_FREE, GET, 0); 
              if(firstCluster < cluster)
                  getSetFreeCluster (NEXT_FREE, SET, firstCluster);

                while(1)  
                {
                    nextCluster = getSetNextCluster (firstCluster, GET, 0);
                getSetNextCluster (firstCluster, SET, 0);
                if(nextCluster > 0x0ffffff6) 
                  {uart_puts_P("File deleted!");return 0;}
                firstCluster = nextCluster;
                } 
              }
            }
        }
        else
        {
          for(j=0; j<11; j++)
            {
            if(j == 8) transmitByte(' ');
          }
            uart_puts_P("   ");
            if((dir->attrib != 0x10) && (dir->attrib != 0x08))
          {
              uart_puts_P("FILE" );
                uart_puts_P("   ");
              displayMemory(LOW, dir->fileSize);
          }
          else
            {
              if (dir->attrib == 0x10)
              {
                uart_puts(("DIR" ));
              }
              else
              {
                uart_puts(("ROOT" ));
              }
            }
        }
       }
     }
   }

   cluster = (getSetNextCluster (cluster, GET, 0));

   if(cluster > 0x0ffffff6) {
          return 0;
   }
   if(cluster == 0) {
       return 0;
   }
}
return 0;
}


/**
 * @brief  Read file from SD card or verify file existence.
 * 
 * If flag=READ, reads file contents from SD card and outputs to UART.
 * If flag=VERIFY, checks whether a specified file already exists on the card
 * without reading contents.
 * 
 * @param  flag     READ to output file contents to UART, VERIFY to check file existence.
 * @param  fileName Pointer to file name to read or verify.
 * @return 0 if normal operation or flag=READ; 1 if file exists (VERIFY) or not found (READ); 2 if invalid filename format.
 */
unsigned char readFile (unsigned char flag, unsigned char *fileName)
{
struct dir_Structure *dir;
unsigned long cluster, byteCounter = 0, fileSize, firstSector;
unsigned int k;
unsigned char j, error;

error = convertFileName (fileName);
if(error) {
    return 2;
}

dir = findFiles (GET_FILE, fileName);

if(dir == 0) 
{
  if(flag == READ) return (1);
  else return (0);
}

if(flag == VERIFY) return (1);

cluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;

fileSize = dir->fileSize;

TX_NEWLINE;
TX_NEWLINE;

while(1)
{
  firstSector = getFirstSector (cluster);

  for(j=0; j<sectorPerCluster; j++)
  {
    SD_readSingleBlock(firstSector + j);
    
  for(k=0; k<512; k++)
    {
      if ((byteCounter++) >= fileSize ) return 0;
    }
  }
  cluster = getSetNextCluster (cluster, GET, 0);
  if(cluster == 0) {
    return 0;}
}
return 0;
}


/**
 * @brief  Convert filename from standard format to FAT 8.3 format.
 * 
 * Transforms filename into 11-byte FAT format: 8 bytes for name, 3 bytes for extension.
 * Pads with spaces and converts lowercase to uppercase. Validates that filename
 * does not exceed 8 characters before extension.
 * 
 * @param  fileName Pointer to filename (input: standard format, output: FAT 8.3 format).
 * @return 0 on success, 1 if filename exceeds 8 characters before extension.
 */
unsigned char convertFileName (unsigned char *fileName)
{
unsigned char fileNameFAT[11];
unsigned char j, k;

for(j=0; j<12; j++)
  if(fileName[j] == '.') break;

if(j>8) {
  return 1;
}

for(k=0; k<j; k++)
  fileNameFAT[k] = fileName[k];

for(k=j; k<=7; k++)
  fileNameFAT[k] = ' ';

j++;
for(k=8; k<11; k++)
{
  if(fileName[j] != 0)
    fileNameFAT[k] = fileName[j++];
  else
    while(k<11)
      fileNameFAT[k++] = ' ';
}

for(j=0; j<11; j++)
  if((fileNameFAT[j] >= 0x61) && (fileNameFAT[j] <= 0x7a))
    fileNameFAT[j] -= 0x20;

for(j=0; j<11; j++)
  fileName[j] = fileNameFAT[j];

return 0;
}


/**
 * @brief  Create new file in FAT32 format or append data to existing file.
 * 
 * If file exists, appends data to it. If file does not exist, creates new file
 * in root directory with proper directory entry. Allocates new clusters as needed,
 * updates FAT chain, and writes timestamp information from RTC.
 * 
 * @param  fileName Pointer to filename (will be converted to FAT format).
 * @return 0 on success, 1 on failure (invalid filename, no free clusters, or SD write error).
 */
unsigned char writeFile (unsigned char *fileName)
{
unsigned char j,k, data, error, fileCreatedFlag = 0, start = 0, appendFile = 0, sector=0;
unsigned int i, firstClusterHigh=0, firstClusterLow=0;
struct dir_Structure *dir;
unsigned long cluster, nextCluster, prevCluster, firstSector, clusterCount, extraMemory;

j = readFile (VERIFY, fileName);

if(j == 1) 
{
  appendFile = 1;
  cluster = appendStartCluster;
  clusterCount=0;
  while(1)
  {
    nextCluster = getSetNextCluster (cluster, GET, 0);
    if(nextCluster == EOF) break;
  cluster = nextCluster;
  clusterCount++;
  }

  sector = (fileSize - (clusterCount * sectorPerCluster * bytesPerSector)) / bytesPerSector;
  start = 1;
}
else if(j == 2) 
{
   return 1;
}
else
{
  cluster = getSetFreeCluster (NEXT_FREE, GET, 0);
  
  if(cluster > totalClusters)
     cluster = rootCluster;

  cluster = searchNextFreeCluster(cluster);
  if(cluster == 0)
  {
    return 1;
  }
  
  getSetNextCluster(cluster, SET, EOF);
  firstClusterHigh = (unsigned int) ((cluster & 0xffff0000) >> 16 );
  firstClusterLow = (unsigned int) ( cluster & 0x0000ffff);
  fileSize = 0;
}

k=0;

while(1)
{
   if(start)
   {
      start = 0;
    startBlock = getFirstSector (cluster) + sector;
    SD_readSingleBlock (startBlock);
    i = fileSize % bytesPerSector;
    j = sector;
   }
   else
   {
      startBlock = getFirstSector (cluster);
    i=0;
    j=0;
   }
   
   do
   {
   data = dataString[k++];
     buffer[i++] = data;
   fileSize++;
     
     if(i >= 512)
   {
     i=0;
     error = SD_writeSingleBlock (startBlock);
       j++;
     if(j == sectorPerCluster) {j = 0; break;}
     startBlock++; 
     }
   } while((data != '\n') && (k < MAX_STRING_SIZE));

   if((data == '\n') || (k >= MAX_STRING_SIZE))
   {
      for(;i<512;i++)
        buffer[i]= 0x00;
      error = SD_writeSingleBlock (startBlock);
      break;
   } 
 
   prevCluster = cluster;
   cluster = searchNextFreeCluster(prevCluster);

   if(cluster == 0)
   {
    return 1;
   }

   getSetNextCluster(prevCluster, SET, cluster);
   getSetNextCluster(cluster, SET, EOF);
}        

getSetFreeCluster (NEXT_FREE, SET, cluster);

error = getDateTime_FAT();
if(error) { dateFAT = 0; timeFAT = 0;}

if(appendFile)
{
  SD_readSingleBlock (appendFileSector);    
  dir = (struct dir_Structure *) &buffer[appendFileLocation]; 

  dir->lastAccessDate = 0;
  dir->writeTime = timeFAT;
  dir->writeDate = dateFAT;
  extraMemory = fileSize - dir->fileSize;
  dir->fileSize = fileSize;
  SD_writeSingleBlock (appendFileSector);
  freeMemoryUpdate (REMOVE, extraMemory);

  return 0;
}

prevCluster = rootCluster;

while(1)
{
   firstSector = getFirstSector (prevCluster);

   for(sector = 0; sector < sectorPerCluster; sector++)
   {
     SD_readSingleBlock (firstSector + sector);

     for(i=0; i<bytesPerSector; i+=32)
     {
      dir = (struct dir_Structure *) &buffer[i];

    if(fileCreatedFlag)
     {
           return 0;
         }

        if((dir->name[0] == EMPTY) || (dir->name[0] == DELETED))
    {
      for(j=0; j<11; j++)
        dir->name[j] = fileName[j];
      dir->attrib = ATTR_ARCHIVE;
      dir->NTreserved = 0;
      dir->timeTenth = 0;
      dir->createTime = timeFAT;
      dir->createDate = dateFAT;
      dir->lastAccessDate = 0;
      dir->writeTime = timeFAT;
      dir->writeDate = dateFAT;
      dir->firstClusterHI = firstClusterHigh;
      dir->firstClusterLO = firstClusterLow;
      dir->fileSize = fileSize;

      SD_writeSingleBlock (firstSector + sector);
      fileCreatedFlag = 1;

      freeMemoryUpdate (REMOVE, fileSize);
       
        }
     }
   }

   cluster = getSetNextCluster (prevCluster, GET, 0);

   if(cluster > 0x0ffffff6)
   {
      if(cluster == EOF)
    {  
    cluster = searchNextFreeCluster(prevCluster);
    getSetNextCluster(prevCluster, SET, cluster);
    getSetNextCluster(cluster, SET, EOF);
      } 
      else
      { 
      return 1;
      }
   }
   if(cluster == 0) {
     return 1;}
   
   prevCluster = cluster;
 }
 
 return 0;
}


/**
 * @brief  Search for next free cluster in FAT starting from specified cluster.
 * 
 * Scans through the FAT (File Allocation Table) to locate the first available
 * free cluster starting from the given cluster number. Searches in 128-cluster
 * increments (one FAT sector at a time) for efficiency.
 * 
 * @param  startCluster Starting cluster number for search.
 * @return First free cluster found, 0 if no free clusters available or error.
 */
unsigned long searchNextFreeCluster (unsigned long startCluster)
{
  unsigned long cluster, *value, sector;
  unsigned char i;
    
  startCluster -=  (startCluster % 128);
    for(cluster =startCluster; cluster <totalClusters; cluster+=128) 
    {
      sector = unusedSectors + reservedSectorCount + ((cluster * 4) / bytesPerSector);
      SD_readSingleBlock(sector);
      for(i=0; i<128; i++)
      {
         value = (unsigned long *) &buffer[i*4];
         if(((*value) & 0x0fffffff) == 0)
            return(cluster+i);
      }  
    } 

 return 0;
}


/**
 * @brief  Display memory size as formatted numeric string sent to UART.
 * 
 * Converts unsigned long memory value into ASCII decimal string with comma
 * separators. Appends "Bytes" or "KBytes" suffix based on flag parameter.
 * Output format: "XXX,XXX,XXX Bytes" (19 characters).
 * 
 * @param  flag   HIGH to display in KiloBytes (append 'K'), LOW to display in Bytes.
 * @param  memory Memory value to display (in Bytes or clusters).
 * @return none
 */
void displayMemory (unsigned char flag, unsigned long memory)
{
  unsigned char memoryString[] = "              Bytes";
  unsigned char i;
  for(i=12; i>0; i--)
  {
    if(i==5 || i==9) 
  {
     memoryString[i-1] = ',';  
     i--;
  }
    memoryString[i-1] = (memory % 10) | 0x30;
    memory /= 10;
  if(memory == 0) break;
  }
  if(flag == HIGH)  memoryString[13] = 'K';
  transmitString(memoryString);
}


/**
 * @brief  Delete specified file from root directory.
 * 
 * Converts filename to FAT format, searches for file in root directory,
 * and calls findFiles with DELETE flag to remove the file.
 * 
 * @param  fileName Pointer to filename to delete.
 * @return none
 */
void deleteFile (unsigned char *fileName)
{
  unsigned char error;

  error = convertFileName (fileName);
  if(error) return;

  findFiles (DELETE, fileName);
}


/**
 * @brief  Update free cluster count in FSinfo sector.
 * 
 * Maintains accurate free space information by adding or removing cluster
 * counts when files are created or deleted. Converts file size (Bytes) to
 * cluster count before updating FSinfo sector.
 * 
 * @param  flag ADD to increase free clusters, REMOVE to decrease.
 * @param  size File size in Bytes (converted to clusters internally).
 * @return none
 */
void freeMemoryUpdate (unsigned char flag, unsigned long size)
{
  unsigned long freeClusters;
  if((size % 512) == 0) size = size / 512;
  else size = (size / 512) +1;
  if((size % 8) == 0) size = size / 8;
  else size = (size / 8) +1;

  if(freeClusterCountUpdated)
  {
  freeClusters = getSetFreeCluster (TOTAL_FREE, GET, 0);
  if(flag == ADD)
       freeClusters = freeClusters + size;
  else
     freeClusters = freeClusters - size;
  getSetFreeCluster (TOTAL_FREE, SET, freeClusters);
  }
}


//******** END ****** [www.dharmanitech.com](https://www.dharmanitech.com) *****
