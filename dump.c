#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "vsfs.h"
#include "library.h"

extern char *currDir;
extern void *disk;

void breakAddress (void *address, int32_t *blockNb, int32_t *offset) {
	SuperBlock_t *sb = disk;
	int blockSize = sb->blockSize;  
	*blockNb = (address - disk) / blockSize;
	*offset = (address - disk) % blockSize;
}

/******************
 * Dump meta data *
 *****************/
void dumpDisk () {
	assert (disk != NULL);
	SuperBlock_t *sb = disk;
	printf ("==== Dump Disk ====\n");
	printf (" Superblock:\n");
	printf ("\tsignature:%08x", sb->signature);
	printf ("\t\tblock count:%d", sb->blockCnt);
	printf ("\t\t\tblock size (bytes): %d\n", sb->blockSize);
	printf ("\tiNode BM size (blocks): %d", sb->iNodeBMSize);
	printf ("\tdata BM size (blocks): %d", sb->dataBMSize);
	printf ("\tiNode table size (blocks): %d\n", sb->iNodeSize);
	printf ("===================\n");
}

/*******************************
 * Dump data block normal file *
 ******************************/
void dumpDataBlock (int32_t blockNb) {
	int i;
	char ascii[17];
	int32_t block, offset;
	assert (disk != NULL);
	SuperBlock_t *sb = disk;

	int8_t *add = getDataBlock (blockNb);
	unsigned char *ptr = (unsigned char *) add; 
	breakAddress (add, &block, &offset);
	printf ("==== Dump Data Block ====\n");
	printf ("\tDataBlock #[%d] - Address: |%p| = block %d offset %d First byte: %02x\n", blockNb, add, block, offset, ptr[0]);
	
	// process every byte 
	for (i=0; i<sb->blockSize; i++) {
		// write 16 bytes per line
		if ( i%16 == 0) {
			// print ascii + new line except for i=0
			if (i!=0) printf (" %s\n", ascii);
			// and then the offset
			printf (" %04x: ", i);
		}
		// print hex code for that address
		printf (" %02x ", ptr[i]);
		// and store a printable character
		if ((*add < 0x20) || (*add > 0x7e)) {
			ascii [i%16] = '.';
		} else {
			ascii [i%16] = ptr[i];
		}
		// make sure there is always a null byte at the end
		ascii [1+i%16] = '\0';
		add++;
	}
	// Pad last line if necessary
	while ((i%16) != 0) {
		printf ("  ");
		i++;
	}
	// And finally the ascii correspondance
	printf (" %s\n", ascii);
	printf ("=========================\n");
}

/***********************************
 * Dump data block of a directory. *
 **********************************/
void dumpDataDirBlock (int32_t blockNb) {
	int32_t block, offset;
	assert (disk != NULL);

	DirEntry_t *add = (DirEntry_t *) getDataBlock (blockNb);
	breakAddress (add, &block, &offset);
	printf ("==== Dump Directory Data Block ====\n");
	printf ("\tDataDirBlock #[%d] - Address: |%p| = block %d offset %d First word: %08x\n", blockNb, add, block, offset, *(int8_t *) add);

	int iCnt = 0;
	while (add != NULL) {
		printf ("\tEntry #%d iNode: %d name: %.15s next: %p", iCnt, add->iNodeNb, add->fileName, add->next);
		if (add->next != NULL) {
			breakAddress (add->next, &block, &offset);
			printf (" = block %d offset %d", block, offset);
		}
		printf ("\n");
		add = add->next;
		iCnt++;
	}
	printf ("===================================\n");
}

/********************************
 * Dump iNode given its number. *
 *******************************/
void dumpInode (int32_t iNodeNb) {
	int32_t block, offset;		// for debug
	assert (disk != NULL);
	// SuperBlock_t *sb = disk;
	Inode_t *iNode = getInode (iNodeNb);

	breakAddress(iNode, &block, &offset);
	printf("iNode #[%d] - Address: |%p| = block %d offset %d\n", iNodeNb, iNode, block, offset);

	printf("\tNumber: %d\tType: %d\t", iNode->number, iNode->type);
	if (iNode->type == FT_DIR) {
		printf (" (directory)\n");
	} else {
		printf (" (not a directory)\n");
	}
	for (int i = 0; i<DIRECTCNT; i++) {
		printf ("\tptr[%d]: %d", i, iNode->ptr[i]);
	}
	printf ("\n");
}

/****************************************
 * Dump a block representing a bit map. *
 ***************************************/
void dumpBM (int8_t *bm) {
	assert (disk != NULL);
	SuperBlock_t *sb = disk;
	int32_t blockSize = sb->blockSize;

	int8_t byte;	// 8 bits
	assert (bm != NULL);
	for (int i=0; i<blockSize; i++) {
		if ((i > 0) && (i % 8 == 0))
			printf ("\n");
		byte = bm[i];
		printf ("%2d:", i);
		for (int iBit=0; iBit<8; iBit++) {
			if ( (byte & (1 << iBit)) != 0) {
				printf ("1");
			} else {
				printf ("0");
			}
		}
		printf (" ");
	}
	printf ("\n");
}

void dumpInodesBM () {
	assert (disk != NULL);
	SuperBlock_t *sb = disk;
	// iNode Bit Map is right after superblock
	int8_t *bm = (int8_t *) (disk + sb->blockSize);
	printf ("==== Dump iNodes Bit Map ==== Address: |%p|\n", bm);
	dumpBM (bm);
}

void dumpDataBM () {
	assert (disk != NULL);
	SuperBlock_t *sb = disk;
	// Data Bit Map is right after superblock iNode bit map
	int8_t *bm = (int8_t *) (disk + sb->blockSize + sb->blockSize*sb->iNodeBMSize);
	printf("==== Dump data Bit Map ==== Address: |%p|\n", bm);
	dumpBM (bm);
}
