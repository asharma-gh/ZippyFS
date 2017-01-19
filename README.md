# zippyfs

Distributed file system using libzip and fuse  
Features: 
- file synchronization via ssh  
- interprocess / intermachine synchronized file i/o  
- shadowed file i/o independent of processes and machines  
- Log of previous files (similar to to that of log file system), compressed as zip-archives      
- full file i/o per Unix API   
- garbage collection on both server / client  
- runs in-memory, concurrently to the machine's actual file system  
- (soon) encrypted files on server  

other branches:  
(branch nozippyfs) -> exciting and more efficient version in development in C++  
(branch master) -> the original, basic file system that allows users to mount amd edit a single zip file

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



