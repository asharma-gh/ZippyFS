# zipfs
toy file system using libzip and fuse  
Allows users to mount a zip archive and read/write stuff  
(branch distfs) -> networked and distributed file system using zip files
(branch nozippyfs) -> faster and more efficient version of distfs in development


How to use:  
./file-system -f -s empty-folder-name zip-file  
where empty-folder-name is an empty folder, and zip-file is the zip file to mount.  
-f is optional. It turns on print statements to view progress  
-s is necessary. It runs it in single threaded mode. Unexpected behavior occures in multi-threaded mode!


