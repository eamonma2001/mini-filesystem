# mini-filesystem

This is a basic implementation of a filesystem and can be ported to raspberry pi 4. The end product contains a shell, a file system, and a virtual disk. The shell allows users to perform operations on the filesystem while the filesystem take operations specified by the user and perform them on the virtual disk. Within the filesystems, superblock, bit map, and iNode blacks contain metadata while data blocks contain the file and directory data.

Here is the list of available commands:

help           display the file
ls             list files in current directory
cd             change directory
make xxx       create file
mkdir xxx      create directory
rmf xxx        remove file
rmd xxx        remove directory
q              quit silulation

dpd            dump disk
dpbm           dump bit map blocks and iNodes
dpi x          dump block #x in iNodes table
dpbl x         dump block #x in data blocks
dpbld x        dump block #x (directory data) in data blocks