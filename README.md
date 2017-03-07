# zippyfs
distributed file system in fuse, early in development    
Original version: branch nozippyfs
## Design
This file system is built using FUSE. This allows us to create a program that utilizes the unix i/o interface.  
This program consists of 3 main components, a block cache, directory on disk, and the server.  
The block cache contains files in-memory for modification. When files are modified, the changes are flushed to disk in the directory on disk.  
If a particular section of a file was modified, that block of the file is flushed.  
Contents in the disk directory are synchronized with the server. Contents on the server are synchronized with all peers connected to it.  
"Contents" consists of meta-data associated with each file and corresponding blocks. When changes are flushed, a new version of the meta data and data are created, so all changed versions exist.  

currently finding a file involves glob'ing the directory of stuff, linking together the files in a b-tree may make this much faster  
 

## Installation/Set-up
This is still in very early stages, and some hacking at the source may be required to get it running. sync.py for instance needs to be overhauled for your server configuration.  

Other than that you will need libfuse and sodium, along with a linux machine and g++  

To start the program:  
```
./file-system <flags> <relative path to mount point> <absolute path to sync directory>

Flags:
-f - run in foreground, useful for debugging / seeing progress
-s - run in single-threaded mode
```

for example:

 ```
 ./file-system -f mount /home/me/zippyfs/sync
 ```

then simply cd / move files in the mount point.
 

