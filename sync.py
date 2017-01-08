#!/usr/bin/python
import os

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
        sys.exit()

    # close file
    f.close()
    

# remake / make file to erase old contents
f = open(path, "w+");


# write our pid to file
f.write(str(os.getpid()))

# close file
f.close()

# call rsync every so often


