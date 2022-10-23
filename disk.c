#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "vsfs.h"
#include "library.h"

// For shell simulation
typedef struct Symbol {
	char *cmd;
	int	val;
} Symbol;

static Symbol lookupTable [CMDCNT] = {
 {"help", HELP}, {"dpd", DUMPDISK}, {"dpbm", DUMPBITMAP}, {"dpi", DUMPINODE}, {"dpbl", DUMPBLOCK}, {"dpbld", DUMPBLOCKDIR}, {"ls", LS}, {"cd", CD}, {"make", MAK}, {"mkdir", MKD}, {"rmf", RMF}, {"rmd", RMD}, {"q", QUIT}
};

char *currDir;	// Current Directory (to display as prompt)
void *disk;
/************************************************************************
 * Mount disk and its file system: we don't attach the new file system  *
 * to a mount point (as Unix does). We only prepare for vsfs to be used *
 * - verify the signature in superblock                                 *
 * - create the root directory (named "/")                              *
 * - initialize currDir to the root                                     *
 ***********************************************************************/
void vsfs_mount () {
    // Verify the signature in superblock
	assert(disk != NULL);
	SuperBlock_t *p = (SuperBlock_t*)disk;

	assert(p->signature == MAGICNB);
	int32_t numInode = getFreeInodeNb();
	assert(numInode == 0);

	int32_t num = getFreeDataBlockNb();
	assert(num == 0);

    // Create the root directory (named "/")
	Inode_t *node = getInode (numInode);
    node->number = numInode;
	node->ptr[0] = num;
	node->type = FT_DIR;

	for(int i=1; i<parameters.directCnt; ++i){
		node->ptr[i] = -1;
    }

    // Update root directory
	DirEntry_t *root = (DirEntry_t*)getDataBlock(num);
	root->next = NULL;
	root->iNodeNb = numInode;

    strcpy (root->fileName, "/");
    // Initialize currDir to the root
	strcpy(currDir, "/");
}//vsfs_mount

/***********************************************
 * Create a virtual disk with blockCnt blocks. *
 * Params points to a Parameters structure     *
 * defined in vsfs.h containing value either   *
 * default or populated from parameter file.   *
 **********************************************/
void vsfs_initDisk (int32_t blockCnt) {
	int32_t diskSize = parameters.blockSize * blockCnt;
	disk = malloc (diskSize);
	assert (disk != NULL);

	// zero the disk
	memset (disk, 0, diskSize);

	// Create superblock with ad hoc values
	SuperBlock_t *sb = (SuperBlock_t *) disk;
	sb->signature = MAGICNB;
 	sb->blockCnt = blockCnt;
 	sb->blockSize = parameters.blockSize;
	sb->iNodeBMSize = parameters.iNodeBMSize;
	sb->dataBMSize = parameters.dataBMSize;
 	sb->iNodeTabSize = parameters.iNodeTabSize;

 	sb->iNodeSize = sizeof(Inode_t);

	// Reserve space for the currDir. Will be set up in vsfs_mount
	currDir = (char *)(malloc (PATH_MAXLEN*sizeof(char)));
	assert (currDir != NULL);

}//vsfs_initDisk

/******************************************
 * Create a file in the current directory *
 * Parameters: name of file               *
 *             file type                  *
 * return -1 if no space available        *
 *        -2 duplicate file name          *
 *        -3 incorrect file name (length) *
 *****************************************/
int32_t vsfs_create (char *name, int8_t ft) {
	assert(disk != NULL);
	SuperBlock_t *block = (SuperBlock_t*)disk;

	// Return -3 if incorrect file name (length)
	if(strlen(name) > FILENAME_LENGTH){
        return -3;
    }

	int32_t upperNode = getInodeNbFromPath(currDir, FT_DIR);
	assert(upperNode >= 0);

	// Return -2 if duplicate file name
	int32_t num = getInodeNbFromParent(upperNode, name, ft);
	if (num != -1) {
		return -2;
	}

	Inode_t *d = getInode (upperNode);
	assert(d != NULL);

    num = getFreeInodeNb ();
	assert(num > 0);

	int32_t bNum = getFreeDataBlockNb();
	assert(bNum > 0);

	Inode_t *node = getInode(num);
	node->number = num;
	node->type = ft;
	node->ptr[0] = bNum;
	for(int32_t i=1; i<parameters.directCnt; ++i) {
		node->ptr[i] = -1;
    }

    assert (disk != NULL);
	SuperBlock_t *sb = (SuperBlock_t *)disk;

    Inode_t *pNode = getInode (upperNode);
	assert(pNode->type == FT_DIR);

    // Find Available space
    for (int i=0; i<DIRECTCNT; i++) {
		int32_t numB = pNode->ptr[i];
		if (numB == -1) {
			numB = getFreeDataBlockNb ();
			assert(numB != -1);
			pNode->ptr[i] = numB;

			DirEntry_t *entry = (DirEntry_t *) getDataBlock (numB);
			assert(entry != NULL);

			entry->next = NULL;
			strcpy(entry->fileName, name);
			entry->iNodeNb = num;
            //check ft is a file or file block
            if (ft == FT_FIL) {
		        char *mem = (char*)getDataBlock(bNum);
		        char msg[80];
		        sprintf(msg, "%s is empty", name);
		        strcpy(mem, msg);
	        }
	        else if (ft == FT_DIR) {
		        char *temp = (char*)getDataBlock(bNum) + sizeof (DirEntry_t);
		        DirEntry_t *dot = (DirEntry_t*)(char*)getDataBlock(bNum);
		        DirEntry_t *doubleDot = (DirEntry_t *)temp;
		        strcpy(dot->fileName, ".");
		        dot->iNodeNb = num;
		        dot->next = doubleDot;
		        strcpy(doubleDot->fileName, "..");
		        doubleDot->iNodeNb = upperNode;
		        doubleDot->next = NULL;
            }
            return 0;
        }
        DirEntry_t *size = (DirEntry_t *)getDataBlock (numB);
		assert (size != NULL);

        DirEntry_t *dir = size;
		while(dir->next != NULL) {
			if (dir->iNodeNb == -1) {
				strcpy (dir->fileName, name);
				dir->iNodeNb = num;
                //check ft is a file or file block
                if (ft == FT_FIL) {
		            char *mem = (char*)getDataBlock(bNum);
		            char msg[80];
		            sprintf (msg, "%s is empty", name);
		            strcpy(mem, msg);
	            }
	            else if (ft == FT_DIR) {
	                char *temp = (char*)getDataBlock(bNum) + sizeof (DirEntry_t);
	                DirEntry_t *dot = (DirEntry_t*)(char*)getDataBlock(bNum);
                    DirEntry_t *doubleDot = (DirEntry_t *)temp;
		            strcpy(dot->fileName, ".");
		            dot->iNodeNb = num;
		            dot->next = doubleDot;
		            strcpy(doubleDot->fileName, "..");
		            doubleDot->iNodeNb = upperNode;
	                doubleDot->next = NULL;
	            }
			    return 0;
            }
		    dir = dir->next;
        }

        // Calculate space needed
        int64_t hold = (int64_t) dir + sizeof (DirEntry_t) - (int64_t)size;
        if ((hold + sizeof (DirEntry_t)) <= (uint64_t)(sb->blockSize)) {
		    char *newAddress = (char*) dir + sizeof (DirEntry_t);
		    DirEntry_t *new = (DirEntry_t *) newAddress;
		    dir->next = new;
		    new->next = NULL;
		    strcpy (new->fileName, name);
		    new->iNodeNb = num;
            // Check if ft is a file or file block
            if (ft == FT_FIL) {
		        char *mem = (char*)getDataBlock(bNum);
		        char msg[80];
		        sprintf (msg, "%s is empty", name);
		        strcpy(mem, msg);
	        }
	        else if (ft == FT_DIR) {
		        char *temp = (char*)getDataBlock(bNum) + sizeof (DirEntry_t);
		        DirEntry_t *dot = (DirEntry_t*)(char*)getDataBlock(bNum);
		        DirEntry_t *doubleDot = (DirEntry_t *)temp;
		        strcpy(dot->fileName, ".");
		        dot->iNodeNb = num;
		        dot->next = doubleDot;
		        strcpy(doubleDot->fileName, "..");
		        doubleDot->iNodeNb = upperNode;
		        doubleDot->next = NULL;
	        }
		    return 0;
	    }
    }

    // If no room is available return -1
    printf ("No room for file %s\n", name);
	void *mem = (int8_t *) (disk + block->blockSize);
	resetBitInBB (mem, num);
	mem = (int8_t *) (disk + block->blockSize + block->blockSize * block->iNodeBMSize);
	resetBitInBB (mem, bNum);
	return -1;
}//create

/****************************
 * Change current directory *
 ***************************/
void vsfs_CD (char *dirName){
    char *mem = malloc (FILENAME_LENGTH + sizeof (*currDir));
	assert(mem != 0);
    strcpy(mem, currDir);

    if (strcmp(dirName, "..") == 0) {
		// Go up one level if possible
		moveDirUp();
		return;
	}
    if (strcmp(dirName, "/") == 0) {
		// Go back to root
		strcpy(currDir, "/");
		return;
	}
    // CD xxx if xxx is an existing sub directory
	if (strcmp(currDir, "/") == 0 ) {
        strcat(mem, dirName);
    }
    else {
		strcat(mem, "/");
        strcat(mem, dirName);
	}

	if (getInodeNbFromPath(mem, FT_DIR) != -1
    && isDirectory(getInodeNbFromPath(mem, FT_DIR))) {
		strcpy(currDir, mem);
		return;
	}

    // If the parameter is not an existing sub directory,
    // display a message “no such file or directory
	printf("%s no such file or directory\n", dirName);
	free(mem);
}

/***********************************************
 * Display the files in the current directory. *
 **********************************************/
void vsfs_LS (){
	int32_t num =  getInodeNbFromPath(currDir, FT_DIR);

	// Make sure directory is not empty
	if(getFileCnt(num) != 0) {

	    Inode_t *node = getInode(num);
	    assert(node->type == FT_DIR);

        // Count of files
        int32_t count = 0;
        // Loop through data
	    for (int32_t i=0; i<parameters.directCnt; i++) {
		    int32_t data = node->ptr[i];
		    if (data == -1){
                break;
            }
		    DirEntry_t *curr = (DirEntry_t*)getDataBlock(data);
		    while (curr != NULL) {
			    if (curr->iNodeNb != -1) {
				    // Don’t print the files dot nor dot-dot
				    if ((curr->iNodeNb != 0) &&
					    (strcmp (curr->fileName, ".") != 0) &&
					    (strcmp (curr->fileName, "..") != 0)){
                        printf ("%s", curr->fileName);
                        if(isDirectory(curr->iNodeNb)){
						    // Print a star after any name which is a directory.
						    printf ("*");
                        }
                        printf (" ");
				    }
                    // Count increments
				    ++count;
			    }
			    curr = curr->next;
			    if ((count % 8) == 0 ) {
				    printf ("\n");
                }
            }
	    }
        printf ("\n");
    }
}


/****************************************
 * remove file in the current directory *
 * based on file name.                  *
 ***************************************/
int32_t vsfs_RMF (char *name){
	SuperBlock_t *block = (SuperBlock_t *) disk;

	int32_t num = getInodeNbFromPath(currDir, FT_DIR);
	assert(num >= 0);

    // Make sure there is a file
	int32_t nodeNum = getInodeNbFromParent (num, name, FT_FIL);
	if (nodeNum == -1){
		return -1;
    }

	Inode_t *node = getInode(nodeNum);
    void *mem;
    for(int32_t i=0; i<parameters.directCnt; ++i){
        int32_t dataSeg = node->ptr[i];
        // Clear the data block
        if(dataSeg != -1) {
            mem = (int8_t *) (disk + block->blockSize + block->blockSize*block->iNodeBMSize);
            resetBitInBB (mem, dataSeg);
            void *p = (void*)getDataBlock (dataSeg);
            memset (p, 0, block->blockSize);
        }
    }
    // Update upper level directory
    int32_t result = -1;
    assert(disk != NULL);

    Inode_t *upper = getInode(num);
    assert (upper->type == FT_DIR);

    for(int32_t i=0; i<DIRECTCNT; ++i) {
        if (upper->ptr[i] == -1) {
            result = -1;
        }
        DirEntry_t *dir = (DirEntry_t *) getDataBlock (upper->ptr[i]);
        assert (dir != NULL);

        while (dir->next != NULL) {
            if (dir->iNodeNb == nodeNum) {
                dir->iNodeNb = -1;
                result = 0;
            }
            dir = dir->next;
        }
    }

    if (result != -1) {
        mem = (int8_t *) (disk + block->blockSize);
        resetBitInBB (mem, nodeNum);
    }

	return result;
}//vsfs_RMF

/****************************************
 * remove directory in the current      *
 * directory based on file name.        *
 * Directory must be empty.             *
 ***************************************/
int32_t vsfs_RMD (char *name){
	SuperBlock_t *block = (SuperBlock_t*)disk;

	int32_t num = getInodeNbFromPath(currDir, FT_DIR);
	assert(num >= 0);

	int32_t nodeNum = getInodeNbFromParent(num, name, FT_DIR);
	if (nodeNum == -1){
		return -1;
    }

	// Make sure directory is empty
	if (getFileCnt(nodeNum) != 0) {
		return -1;
	}

	Inode_t *iNode = getInode(nodeNum);

    void *mem;
	for(int32_t i=0; i<parameters.directCnt; ++i){
		int32_t numB = iNode->ptr[i];
		if(numB != -1) {
			mem = (int8_t *) (disk + block->blockSize + block->blockSize*block->iNodeBMSize);
            // Clear the data block
			resetBitInBB (mem, numB);
			void *data = (void*) getDataBlock(numB);
			memset(data, 0, block->blockSize);
		}
	}

	// Update parent directory
	int32_t result = -1;
    assert(disk != NULL);

	Inode_t *upper = getInode (num);
	assert(upper->type == FT_DIR);

	for(int32_t i=0; i<DIRECTCNT; ++i) {
		if(upper->ptr[i] == -1) {
			result = -1;
    }

		DirEntry_t *curr = (DirEntry_t*) getDataBlock(upper->ptr[i]);
		assert(curr != NULL);

		while (curr->next != NULL) {
			if (curr->iNodeNb == nodeNum) {
				curr->iNodeNb = -1;
				result = 0;
			}
			curr = curr->next;
		}
	}

	if (result != -1) {
		mem = (int8_t *) (disk + block->blockSize);
		resetBitInBB(mem, nodeNum);
	}

	return result;
}//vsfs_RMD

/*******************************************
 * get the corresponding symbol from the   *
 * command (such that we can use a switch) *
 * For larger implementation, use a hash   *
 * table or binary search...               *
 ******************************************/
int getCmd (char *cmd) {
	for (int i=0; i<CMDCNT; i++) {
		Symbol *s = &lookupTable [i];
		if (strcmp (s->cmd, cmd) == 0)
			return s->val;
	}
	return BADCMDE;
}//getCmd

void help (){
	printf ("help\t\tdisplay this file\n");
	printf ("ls\t\tlist files in current directory\n");
	printf ("cd\t\tchange directory\n");
	printf ("make xxx\tcreate file xxx in current directory\n");
	printf ("mkdir xxx\tcreate directory xxx in current directory\n");
	printf ("rmf xxx\t\tremove file xxx from current directory\n");
	printf ("rmd xxx\t\tremove directory xxx from current directory\n");
	printf ("q\t\tQuit simulation\n");
	printf ("\n");
	printf ("dpd\t\tdump disk\n");
	printf ("dpbm\t\tdump bit map blocks and inodes\n");
	printf ("dpi x\t\tdump block #x in inodes table\n");
	printf ("dpbl x\t\tdump block #x in data blocks\n");
	printf ("dpbld x\t\tdump block #x (seen as directory) in data blocks\n");
} // help

/******************************************************
 * Command line has 2 elements:                       *
 * a command followed by a parameter to the command.  *
 * get them and call appropriate routine.             *
 * Need to duplicate the command to use getToken which*
 * jeopardizes the string being parsed.               *
 *****************************************************/
int parseAndExecute (char *cmdLine) {
	int quit = 0;
	int32_t retVal;

	char *dupCmdLine = malloc ((1 + CMDE_LENGTH) * sizeof (char));
	assert (dupCmdLine != NULL);

	strcpy (dupCmdLine, cmdLine);

	char *cmde = getToken (&dupCmdLine, ' ');
	char *param = getToken (&dupCmdLine, ' ');

	if (cmde == NULL) {
		printf ("command %s not found\n", cmdLine);
		return 0;
	}

	switch (getCmd (cmde)) {
		case QUIT:
				printf ("Simulation terminated.\n");
				quit = 1;
				break;
		case HELP:
				if (param != NULL) {
					printf ("%s: bad operand\n", param);
				} else {
					help ();
				}
				break;
		case LS:
				if (param != NULL) {
					printf ("%s: bad operand\n", param);
				} else {
					vsfs_LS ();
				}
				break;
		case CD:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					vsfs_CD (param);
				}
				break;
		case MAK:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					retVal = vsfs_create (param, FT_FIL);
					if (retVal != 0)
						printf ("Error %d in creation of %s\n", retVal, param);
				}
				break;
		case MKD:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					retVal = vsfs_create (param, FT_DIR);
					if (retVal != 0)
						printf ("Error %d in creation of %s\n", retVal, param);
				}
				break;
		case RMF:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					retVal = vsfs_RMF (param);
					if (retVal != 0)
						printf ("Error %d in removing %s\n", retVal, param);
				}
				break;
		case RMD:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					retVal = vsfs_RMD (param);
					if (retVal != 0)
						printf ("Error %d in removing %s\n", retVal, param);
				}
				break;
		case DUMPDISK:
				if (param != NULL) {
					printf ("%s: bad operand\n", cmdLine);
				} else {
					dumpDisk();
				}
				break;
		case DUMPBITMAP:
				if (param != NULL) {
					printf ("%s: bad operand\n", cmdLine);
				} else {
					dumpInodesBM ();
					dumpDataBM ();
				}
				break;
		case DUMPINODE:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					dumpInode (atoi(param));
				}
				break;
		case DUMPBLOCK:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					dumpDataBlock (atoi(param));
				}
				break;
		case DUMPBLOCKDIR:
				if (param == NULL) {
					printf ("%s: missing operand\n", cmdLine);
				} else {
					dumpDataDirBlock (atoi(param));
				}
				break;
		case BADCMDE:
				printf ("command %s not found\n", cmdLine);
	}
	//free (dupCmdLine);
	return quit;

}//parseAndExecute

int main () {
	int32_t retVal;
	char *cmdLine = NULL;			// Command line for our shell
	size_t len = 0;					// Number of char read for our shell

	// Set parameters
	parameters.blockSize = BLOCKSIZE;
	parameters.iNodeBMSize = INODEBMSIZE;
	parameters.dataBMSize = DATABMSIZE;
	parameters.iNodeTabSize = INODETABSIZE;
	parameters.directCnt = DIRECTCNT;
	paramDebug = FALSE;

	// Set disk ang go !
	currDir = (char *)(malloc (PATH_MAXLEN*sizeof(char)));
	assert (currDir != NULL);

	vsfs_initDisk (100);
	vsfs_mount ();
	printf ("%s: ", currDir);

	cmdLine = malloc (CMDE_LENGTH * sizeof (char));
	assert (currDir != NULL);

	retVal = getline (&cmdLine, &len, stdin);
	while ( retVal != -1) {
		// remove cmdLine last char (\n)
		cmdLine [strlen(cmdLine)-1] = 0;
		if (parseAndExecute (cmdLine) == 1) break;
		printf ("%s: ", currDir);
  		retVal = getline (&cmdLine, &len, stdin);
	}
	free (cmdLine);
	free (disk);
	return 0;
}//main
