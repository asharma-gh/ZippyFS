# zippyfs
distributed file system in fuse, early in development    
Original version: branch nozippyfs

## Rational
Seeking thru .root files line by line ends up being a huge bottleneck when there are a lot of files

In this branch I'm going to give each file version its own file that'll make locating a particular group of file versions
as expensive as iterating thru the directory of all file versions.

Idea:
```
[hash of path][delim][128 rand nums].file  
```
for each flushed file version, so locating a file is a matter of checking if [hash of path][random*].file exists  



 
 

