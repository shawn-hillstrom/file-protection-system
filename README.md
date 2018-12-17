# SetUID System for Linux

## About

This project implements a rudimentary file protection system using Linux's SetUID methods. **get.c** ensures that it runs in EUID mode only when opening restricted administrative files. Additionally, **get.c** does not close secure files until termination because it requires multiple stages for read/write throughout execution. 

## Versioning

**VERSION:** 1.0

**RELEASE:** N/A

**LAST UPDATED:** October 7th, 2018

## Resources

**get.c:** Source file using SetUID methods to implement a file protection system.

**Makefile:** Makefile for building **get.c**.

## How To Use

To create a protected file:
1. Place the file in a directory of the administrator's choosing, accessible by other users.
2. Change permissions so that other users cannot access the file.
3. Create another file with the format **FILENAME.access** which is also private and specify all users with access permission. Each user's name and permission level (r, w, or b) should be specified on separate lines with whitespace separating each name and permission level.

To build the system:
1. Place **get.c** and **Makefile** in a directory accessible by other users and run
'''
make
'''
2. To remove the executable run
'''
make clean
'''

To use the system:
1. Run
'''
./get SOURCEPATH DESTINATIONPATH
'''
where SOURCEPATH is the path for the restricted file and DESTINATIONPATH is the path for the copy of the file with specified access priviledges for the user running **get**.

**NOTE:** Users trying to access files should specify a destination path that he or she has write access in, otherwise get will exit silently.

## Future Development

No future development is planned at this time.
