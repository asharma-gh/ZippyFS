# zippyfs

distributed file system using libzip and fuse  
(branch nozippyfs) -> exciting and more efficient version in development  
(branch zipfs) -> basic file system allowing users to mount a single zip file

How to install  
-----------------
requires libfuse, libzip, and glib

How to use  
-----------------
./file-system [-f -s] [relative path to mount point] [absolute path to directory of zip archives]  

e.g. ./file-system -f mount /home/test/dir  
-f to run in foreground  
-s for single threaded mode (multithreaded mode is probably still buggy)  

(tentatively) after doing a write, the sync program can be started by:  
python sync.py [relative path to rmlog file]  
rmlog file is in /relative/path/to/dir-of-zips/rmlog/  
e.g. python sync.py dir/rmlog/DEADBEEF  



