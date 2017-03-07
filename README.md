# zippyfs
distributed file system in fuse, early in development    
Original version: branch nozippyfs

## Rational
Seeking thru .root files line by line ends up being a huge bottleneck when there are a lot of files.

The original plan was instead of iterating thru the directory to find a given root with a given file,  
roots have pointers to each other in a b-tree like structure where inode numbers are the keys.  
This would make finding a particular file a matter of hopping thru references to .root files.  
This would still require seeking thru the .roots, which even if it's cached as a string, would still be too slow.  


In this branch I'm going to give each file version its own file that'll make locating a particular group of file versions
as expensive as glob'ing either all files with that path or parent  
Idea:
```
[hash of parent][delim][hash of path][delim][128 rand nums].meta
```
for each flushed file version, so locating a file is a matter of checking if [hash or parent][hash of path][random bits].meta exists  

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
 

