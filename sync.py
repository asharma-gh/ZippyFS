#!/usr/bin/python
import os
import signal
import time
import sys

def init():
    # open sync file
    path = os.path.expanduser("~/.cache/zipfs/sync.pid")

    if os.path.exists(path) and os.stat(path).st_size > 0:
        f = open(path, "r+")

        # read the previous pid
        # check if it is running
        is_running = True
        try:
            os.kill(int(f.readline()), 0)
        except OSError:
            is_running = False

        #close file
        f.close()

        # if another process is running, then we don't need to
        if (is_running):
            sys.exit()

    # remake / make file to erase old contents
    f = open(path, "w+")


    # write our pid to file
    f.write(str(os.getpid()))

    # close file
    f.close()

def sync():
    #check rm log
    #upload new contents to server
    os.system("rsync --ignore-existing -r -a -v -e ssh ~/FileSystem/zipfs/o/dir/ arvinsharma@login.ccs.neu.edu:/home/arvinsharma/test/")
    #download content from server
    os.system("rsync --ignore-existing -r -a -v -e ssh arvinsharma@login.ccs.neu.edu:/home/arvinsharma/test/ ~/FileSystem/zipfs/o/dir/")

    #delete rm log'd files on server
    #PATH to rmlog in zip file directory will be passed in as a command line argument.
    path_to_log = sys.argv[1]
    lines = open(path_to_log,  "r").readlines()
    for line in lines:
        path_to_idx = "/home/arvinsharma/test/" + line.strip()
        path_to_zip = "/home/arvinsharma/test/" + line.strip()[:len(line.strip()) - 4] + ".zip"
        os.system("ssh arvinsharma@login.ccs.neu.edu 'rm " + path_to_idx + " && rm " + path_to_zip + "'")


    # resync every 5 minutes
    time.sleep(10)
    sync()
#sync on SIGUSR1
def signal_handler(signum, frame):
        print('Received Signal ', signum)
        sync()


## Begin Execution ##
init()
## sync on sigusr1 ##
signal.signal(signal.SIGUSR1, signal_handler)
## sync on start ##
sync()

