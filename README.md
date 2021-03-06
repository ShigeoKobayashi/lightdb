# lightdb
[B-Tree based simple and fast C/C++ database API library](http://www.tinyforest.jp/oss/lightdb.html)


The LightDB is a simple and fast C/C++ database API library based on the B-Tree(not B+Tree) indexing.
The B-Tree is a self-balancing search tree having multiple nodes that contain multiple records.
All records in B-Tree are stored in sorted order. 

For more informations about B-Tree,refer to any other appropriate documentations.  
For more informations and usage of LightDB,refer to lightdb.html on your browser.

This software can be redistributed under GNU Lesser General Public License.

LightDB file can be more than 2GB in size on both 32-bit and 64-bit operating systems.  
This software has been tested on Windows-10(32-bit & 64-bit) and Linux(32-bit CentOS-5 & 64-bit CentOS-7).

To build this software on Linux => see makefile 
  
To build this software on Windows => create Visual studio solution,and add source files.

To use this library => include lightdb.h in your source codes(see test.c) and link appropriate library files.

Windows binary files(32-bit:Test.exe,lightdb.dll,lightdb.lib) can be [downloaded from here](http://www.tinyforest.jp/oss/lightdb.zip).
