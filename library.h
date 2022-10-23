#ifndef LIBRARY_H
#define LIBRARY_H

#include <stdbool.h>

/////////////////////////////////////////////////////
// The following are helper - not visible to user  //
/////////////////////////////////////////////////////

// To parse a string
char *getToken (char **, const char); 

// Return TRUE if iNodeNb is a directory
bool isDirectory (int32_t);

// Reset specific bit in bitmap
void resetBitInBB (int8_t *, int32_t);

// return first null bit in BM and set it to 1
int32_t findAndSetBB (int8_t *);

// return pointer to the top of a data block		
void *getDataBlock (int32_t);

// Return the number of files stored in directory from iNodeNb 
int32_t getFileCnt (int32_t);

// return first null bit in DATA BM and set it to 1
int32_t getFreeDataBlockNb ();		

// return first null bit in iNodes BM and set it to 1
int32_t getFreeInodeNb();

// return pointer to iNode of specific number
Inode_t *getInode (int32_t);

// Return the iNode number of a file knowning the iNodeNb of its parent, its name and type.
int32_t getInodeNbFromParent (int32_t, char *, int8_t);

// Return the iNode number of a file knowning its full path and type.
int32_t getInodeNbFromPath (char *, int8_t);

// Update currDir when moving up one level
void moveDirUp ();
#endif