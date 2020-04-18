Navigating in the Dev Environment
=================================

The NaCl Dev Environment behaves like a mini-UNIX system.
You start-up in bash command prompt.
Standard UNIX commands like cd, ls, mkdir, rm, rmdir, etc.
can be used to navigate and modify the Dev Environment's
virtual filesystem.

There are 3 locations mounted by the environment:

  * /tmp mounts the HTML5 Filesystem temporary storage area.
  * /mnt/html5 mounts the HTML5 Filesystem persistent storage area.
  * Your home directory ~/, located in /home/user,
    mounts a portion of the HTML5 Filsystem persistent storage area
    that corresponds to the /mnt/html5/home directory.
