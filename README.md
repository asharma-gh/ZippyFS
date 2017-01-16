# zippyfs
distributed file system using libzip and fuse  

How to install:
How to use:
./file-system [-fs] [relative path to mount point] [absolute path to directory of zip archives]  

e.g. ./file-system -f mount /home/test/dir  
-f to run in foreground  
-s for single threaded mode (multithreaded mode is probably still buggy)  

