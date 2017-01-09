#!/usr/bin/python
import os
import signal
import threading

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

        # if another process is running, then we don't need to
        if (is_running):
            f.close()
            sys.exit()

        # close file
        f.close()

    # remake / make file to erase old contents
    f = open(path, "w+")


    # write our pid to file
    f.write(str(os.getpid()))

    # close file
    f.close()

def sync():
    print("TODO..")
    # resync every 5 minutes. This works like a delayed recursive call
    threading.Timer(300, sync).start()
    #upload: rsync -r -a -v -e ssh ~/FileSystem/zipfs/o/dir/ arvinsharma@login.ccs.neu.edu:/home/arvinsharma/test
    #os.system(...)

#sync on SIGUSR1
def signal_handler(signum, frame):
    print('Received Signal ', signum)
    sync()


## Begin Execution ##
init()
## sync on start ##
sync()

