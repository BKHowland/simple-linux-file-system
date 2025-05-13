# simple-linux-file-system
 a command line program for determining key information about the file system of a disk image, such as the number of FAT copies and sectors per FAT, and copying files between any two directories while updating FAT and related allocation information in the disk image accordingly.

Completed April 2023

Program instructions:
compile the files using the command "make"
clean the temporary files with "make clean", however, this implementation does not produce any.

To run the programs:

To run diskinfo, type "./diskinfo <disk image>" where <disk image> is the name of the desired
disk image including its extension. eg: "./diskinfo disk.ima"
This will print out the information about the disk image

To run disklist, type "./disklist <disk image>" where <disk image> is the name of the desired
disk image including its extension. eg: "./disklist disk.ima"
This will list the files and directories within the disk image

To run diskget, type "./disklist <disk image> <filename>" where <disk image> is the name of 
the desired disk image including its extension, and <filename> is the desired file you want from
the image. eg: "./disklist disk.ima icebergs.tex"

To run diskput, type "./diskput <disk image> <path> <filename>" where <disk image> is the name 
of the desired disk image including its extension, <path> is the optional path within the image 
to place the file, and <filename> is the desired file you want to add to the image. 
eg: "./disklist disk.ima /sub1/sub2 icebergs.tex"
if no path is provided, the root directory will be used. eg: "./disklist disk.ima icebergs.tex"

