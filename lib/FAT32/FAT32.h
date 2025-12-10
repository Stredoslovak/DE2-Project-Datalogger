//********************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD *****
//********************************************************
//Controller        : ATmega32 (Clock: 8 Mhz-internal)
//Compiler          : AVR-GCC (winAVR with AVRStudio-4)
//Project Version   : DL_1.0
//Author            : CC Dharmani, Chennai (India)
//                    [www.dharmanitech.com](https://www.dharmanitech.com)
//Date              : 10 May 2011
//*********************************************************


#ifndef _FAT32_H_
#define _FAT32_H_


/**
 * @brief Structure to access Master Boot Record (MBR) for getting info about partitions.
 */
struct MBRinfo_Structure{
unsigned char   nothing[446];       //ignore, placed here to fill the gap in the structure
unsigned char   partitionData[64];  //partition records (16x4)
unsigned int    signature;      //0xaa55
};


/**
 * @brief Structure to access info of the first partition of the disk.
 */
struct partitionInfo_Structure{                 
unsigned char   status;             //0x80 - active partition
unsigned char   headStart;          //starting head
unsigned int    cylSectStart;       //starting cylinder and sector
unsigned char   type;               //partition type 
unsigned char   headEnd;            //ending head of the partition
unsigned int    cylSectEnd;         //ending cylinder and sector
unsigned long   firstSector;        //total sectors between MBR & the first sector of the partition
unsigned long   sectorsTotal;       //size of this partition in sectors
};


/**
 * @brief Structure to access boot sector data (BIOS Parameter Block).
 */
struct BS_Structure{
unsigned char jumpBoot[3]; //default: 0x009000EB
unsigned char OEMName[8];
unsigned int bytesPerSector; //deafault: 512
unsigned char sectorPerCluster;
unsigned int reservedSectorCount;
unsigned char numberofFATs;
unsigned int rootEntryCount;
unsigned int totalSectors_F16; //must be 0 for FAT32
unsigned char mediaType;
unsigned int FATsize_F16; //must be 0 for FAT32
unsigned int sectorsPerTrack;
unsigned int numberofHeads;
unsigned long hiddenSectors;
unsigned long totalSectors_F32;
unsigned long FATsize_F32; //count of sectors occupied by one FAT
unsigned int extFlags;
unsigned int FSversion; //0x0000 (defines version 0.0)
unsigned long rootCluster; //first cluster of root directory (=2)
unsigned int FSinfo; //sector number of FSinfo structure (=1)
unsigned int BackupBootSector;
unsigned char reserved[12];
unsigned char reserved1;
unsigned char bootSignature;
unsigned long volumeID;
unsigned char volumeLabel[11]; //"NO NAME "
unsigned char fileSystemType[8]; //"FAT32"
unsigned char bootData[420];
unsigned int bootEndSignature; //0xaa55
};



/**
 * @brief Structure to access FSinfo sector data.
 * Used for tracking free cluster counts and next free cluster.
 */
struct FSInfo_Structure
{
unsigned long leadSignature; //0x41615252
unsigned char reserved1[480];
unsigned long structureSignature; //0x61417272
unsigned long freeClusterCount; //initial: 0xffffffff
unsigned long nextFreeCluster; //initial: 0xffffffff
unsigned char reserved2[12];
unsigned long trailSignature; //0xaa550000
};


/**
 * @brief Structure to access Directory Entry in the FAT file system.
 */
struct dir_Structure{
unsigned char name[11];
unsigned char attrib; //file attributes
unsigned char NTreserved; //always 0
unsigned char timeTenth; //tenths of seconds, set to 0 here
unsigned int createTime; //time file was created
unsigned int createDate; //date file was created
unsigned int lastAccessDate;
unsigned int firstClusterHI; //higher word of the first cluster number
unsigned int writeTime; //time of last write
unsigned int writeDate; //date of last write
unsigned int firstClusterLO; //lower word of the first cluster number
unsigned long fileSize; //size of file in bytes
};


/**
 * @defgroup FileAttributes File Attribute Definitions
 * @{
 */
#define ATTR_READ_ONLY     0x01
#define ATTR_HIDDEN        0x02
#define ATTR_SYSTEM        0x04
#define ATTR_VOLUME_ID     0x08
#define ATTR_DIRECTORY     0x10
#define ATTR_ARCHIVE       0x20
#define ATTR_LONG_NAME     0x0f
/** @} */


/**
 * @defgroup FATConstants FAT32 Constants and Flags
 * @{
 */
#define DIR_ENTRY_SIZE     0x32
#define EMPTY              0x00
#define DELETED            0xe5
#define GET     0
#define SET     1
#define READ    0
#define VERIFY  1
#define ADD     0
#define REMOVE  1
#define LOW     0
#define HIGH    1   
#define TOTAL_FREE   1
#define NEXT_FREE    2
#define GET_LIST     0
#define GET_FILE     1
#define DELETE       2
#define EOF     0x0fffffff
/** @} */


#define MAX_STRING_SIZE     100  //defining the maximum size of the dataString



//************* external variables *************
volatile unsigned long firstDataSector, rootCluster, totalClusters;
volatile unsigned int  bytesPerSector, sectorPerCluster, reservedSectorCount;
unsigned long unusedSectors, appendFileSector, appendFileLocation, fileSize, appendStartCluster;


//global flag to keep track of free cluster count updating in FSinfo sector
unsigned char freeClusterCountUpdated;


//data string where data is collected before sending to the card
volatile unsigned char dataString[MAX_STRING_SIZE];




//************* functions *************

/**
 * @brief  Read boot sector data from SD card to determine FAT32 parameters.
 * @return 0 on success, non-zero on error.
 */
unsigned char getBootSectorData (void);

/**
 * @brief  Calculate first sector address of a given cluster.
 * @param  clusterNumber Cluster number.
 * @return First sector address.
 */
unsigned long getFirstSector(unsigned long clusterNumber);

/**
 * @brief  Get or set free cluster information from FSinfo sector.
 * @param  totOrNext    TOTAL_FREE for count, NEXT_FREE for pointer.
 * @param  get_set      GET to read, SET to update.
 * @param  FSEntry      Value to set (if SET).
 * @return Requested value (GET) or 0xffffffff (error/SET).
 */
unsigned long getSetFreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry);

/**
 * @brief  Find file or list directory contents.
 * @param  flag      GET_LIST, GET_FILE, or DELETE.
 * @param  fileName  Pointer to filename (for GET_FILE/DELETE).
 * @return Pointer to directory entry or NULL.
 */
struct dir_Structure* findFiles (unsigned char flag, unsigned char *fileName);

/**
 * @brief  Get or set FAT cluster entry (next cluster in chain).
 * @param  clusterNumber Current cluster.
 * @param  get_set       GET for next cluster, SET to link new cluster.
 * @param  clusterEntry  New next cluster (if SET).
 * @return Next cluster (GET) or 0 (SET).
 */
unsigned long getSetNextCluster (unsigned long clusterNumber,unsigned char get_set,unsigned long clusterEntry);

/**
 * @brief  Read file content or verify existence.
 * @param  flag      READ (output to UART) or VERIFY (check existence).
 * @param  fileName  Pointer to filename.
 * @return 0 (success/READ), 1 (found/VERIFY), 2 (invalid name).
 */
unsigned char readFile (unsigned char flag, unsigned char *fileName);

/**
 * @brief  Convert standard filename to FAT 8.3 format.
 * @param  fileName  Filename buffer (in/out).
 * @return 0 on success, 1 on error (name too long).
 */
unsigned char convertFileName (unsigned char *fileName);

/**
 * @brief  Write data to file (create new or append).
 * @param  fileName  Pointer to filename.
 * @return 0 on success, 1 on error.
 */
unsigned char writeFile (unsigned char *fileName);

/**
 * @brief  Append data to existing file (helper/legacy).
 */
void appendFile (void);

/**
 * @brief  Search for next free cluster in FAT.
 * @param  startCluster Starting search point.
 * @return Free cluster number or 0 if full/error.
 */
unsigned long searchNextFreeCluster (unsigned long startCluster);

/**
 * @brief  Display memory size via UART.
 * @param  flag    HIGH (KB) or LOW (Bytes).
 * @param  memory  Size value.
 */
void displayMemory (unsigned char flag, unsigned long memory);

/**
 * @brief  Delete file from directory.
 * @param  fileName Pointer to filename.
 */
void deleteFile (unsigned char *fileName);

/**
 * @brief  Update free space count in FSinfo.
 * @param  flag  ADD or REMOVE free space.
 * @param  size  Size in bytes.
 */
void freeMemoryUpdate (unsigned char flag, unsigned long size);


#endif
