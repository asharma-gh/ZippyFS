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
        os.kill(int(file.readline()), 0)
    except OSError:
        is_running = False

    # if another process is running, then we don't need to
    if (is_running):
        sys.exit()

    # clean up file
    f.truncate()

else:
    f = open(path, "w+");


# write our pid to file
f.write(str(getpid()))

# close file
f.close()

# do something every so often


