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
as expensive as iterating thru the directory of all file versions.

Idea:
```
[hash of path][delim][128 rand nums].meta
```
for each flushed file version, so locating a file is a matter of checking if [hash of path][random*].file exists  



 
 

