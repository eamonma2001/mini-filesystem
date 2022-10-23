#ifndef VSFS_H
#define VSFS_H

#define MAGICNB				0x56534653
#define FT_DIR 					1			// File is a directory
#define FT_FIL					2			// File is a data file
#define PATH_MAXLEN			255		// Path maximum length
#define TRUE						1
#define FALSE						0

// Default Values
#define BLOCKSIZE				96		// bytes
#define INODEBMSIZE			1			// Inode bit map size in block
#define DATABMSIZE			1			// Data bit map size in block
#define INODETABSIZE		10		// Inode table size in block
#define DIRECTCNT				3			// number of pointers in an iNode
#define FILENAME_LENGTH	16		// Max chars in a file name

// The following for shell simulation
#define CMDE_LENGTH		64			// Max length of command
#define CMDCNT				13			// Number of commands
#define BADCMDE 			-1
#define HELP					0
#define LS 						1
#define CD 						2
#define MAK						3
#define MKD						4
#define RMF						5
#define RMD						6
#define DUMPDISK			7
#define DUMPBITMAP		8
#define DUMPINODE			9
#define DUMPBLOCK			10
#define DUMPBLOCKDIR	11
#define QUIT					99

// Colors used in printf (B for bold)
// Example: printf (COLOR_BBLACK); printf ("Hello"); printf (COLOR_RESET);
#define COLOR_BLACK		"\033[0;30m"
#define COLOR_BBLACK	"\033[1;30m"
#define COLOR_RED			"\033[0;31m"
#define COLOR_BRED		"\033[1;31m"
#define COLOR_GREEN		"\033[0;32m"
#define COLOR_BGREEN	"\033[1;32m"
#define COLOR_YELLOW	"\033[0;33m"
#define COLOR_BYELLOW	"\033[1;33m"
#define COLOR_BLUE		"\033[0;34m"
#define COLOR_BBLUE		"\033[1;34m"
#define COLOR_PURPLE	"\033[0;35m"
#define COLOR_BPURPLE	"\033[1;35m"
#define COLOR_CYAN		"\033[0;36m"
#define COLOR_BCYAN		"\033[1;36m"
#define COLOR_WHITE		"\033[0;37m"
#define COLOR_BWHITE	"\033[1;37m"
#define COLOR_RESET		"\033[0m"

// Any of these parameters will receive a value from the parameter file.
typedef struct Parameters {
	int blockSize;				// Size (in bytes) of one block
	int iNodeBMSize;			// Size (in blocks) of iNode bit map
	int dataBMSize;				// Size (in blocks) of data block bit map
	int iNodeTabSize;			// Size (in blocks) of iNode table
	int directCnt;				// Number of direct pointers in an iNode
} Parameters;

	Parameters parameters;
	int paramDebug;

// Superblock contains general information about the file system
typedef struct SuperBlock {
	int32_t signature;		// magic number
	int32_t blockCnt;			// blocks on the disk
	int32_t blockSize;		// in bytes
	int32_t iNodeBMSize;	// in blocks	
	int32_t dataBMSize;		// in blocks	
	int32_t iNodeTabSize;	// in blocks	
	int32_t iNodeSize;		// in bytes
} SuperBlock_t;

// iNode contains a limited data.
typedef struct Inode {
	int8_t type;						// file type
	int32_t number;					// iNode number
	int32_t ptr[DIRECTCNT];	// pointers to data block
}Inode_t;

// Datablock for a directory is a chain of DirEntry.
typedef struct DirEntry {
  struct DirEntry *next;						// Pointer to next DirEntry. NULL at the end.
  int32_t iNodeNb;									// -1 <=> entry available (corresponding file has been deleted)
  char fileName[FILENAME_LENGTH];		// File name
} DirEntry_t;

// Reading parameters values
void getParams (char *);				// get parameters from file
void initParams ();							// assign default values

// Dumping data.
void dumpBM (int8_t *);					// dump bit map given its address 
void dumpDataBM ();							// dump data bit map
void dumpInodesBM ();						// dump iNodes bit map
void dumpInode (int32_t);				// dump iNode (iNode number)
void dumpDataDirBlock (int32_t);// dump data block viewed as dir (block number)
void dumpDataBlock (int32_t);		// dump data block (pure data)
void dumpDisk ();								// dump super block

//////////////////////////////////////////////
// The following are API (visible to users) //
//////////////////////////////////////////////
void vsfs_initDisk (int32_t);					// disk initialization
void vsfs_mount ();										// mount disk
void vsfs_LS ();											// list files in current directory
void vsfs_CD (char *);								// change directory
int32_t vsfs_create (char *, int8_t);	// create a file of a given type in the current directory
int32_t vsfs_RMD (char *);						// remove directory in current directory
int32_t vsfs_RMF (char *);						// remove file in current directory

#endif