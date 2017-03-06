# "no"zippyfs
distributed file system in fuse, early in development    
Original version: branch nozippyfs


Seeking thru .root files line by line ends up being a huge bottleneck when there are a lot of files

In this branch I'm going to give each file version its own file that'll make locating a particular group of file versions
as expensive as iterating thru the directory of all file versions.

 
 
## Features (so far):  
read / write stuff over a network  
more to come . . .

## How to install:  
requires libzip, fuse, linux
## How to use:   
todo

