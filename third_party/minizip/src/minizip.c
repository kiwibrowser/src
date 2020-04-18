/* minizip.c
   Version 1.2.0, September 16th, 2017
   sample part of the MiniZip project

   Copyright (C) 2012-2017 Nathan Moinvaziri
     https://github.com/nmoinvaz/minizip
   Copyright (C) 2009-2010 Mathias Svensson
     Modifications for Zip64 support
     http://result42.com
   Copyright (C) 2007-2008 Even Rouault
     Modifications of Unzip for Zip64
   Copyright (C) 1998-2010 Gilles Vollant
     http://www.winimage.com/zLibDll/minizip.html

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <utime.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#endif

#include "zip.h"

#ifdef _WIN32
#  define USEWIN32IOAPI
#  include "iowin32.h"
#endif

#include "minishared.h"

void minizip_banner()
{
    printf("MiniZip 1.2.0, demo of zLib + MiniZip64 package\n");
    printf("more info on MiniZip at https://github.com/nmoinvaz/minizip\n\n");
}

void minizip_help()
{
    printf("Usage : minizip [-o] [-a] [-0 to -9] [-p password] [-j] file.zip [files_to_add]\n\n" \
           "  -o  Overwrite existing file.zip\n" \
           "  -a  Append to existing file.zip\n" \
           "  -0  Store only\n" \
           "  -1  Compress faster\n" \
           "  -9  Compress better\n\n" \
           "  -j  exclude path. store only the file name.\n\n");
}

int minizip_addfile(zipFile zf, const char *path, const char *filenameinzip, int level, const char *password)
{
    zip_fileinfo zi = { 0 };
    FILE *fin = NULL;
    int size_read = 0;
    int zip64 = 0;
    int err = ZIP_OK;
    char buf[UINT16_MAX];


    /* Get information about the file on disk so we can store it in zip */
    get_file_date(path, &zi.dos_date);

    zip64 = is_large_file(path);

    /* Add to zip file */
    err = zipOpenNewFileInZip3_64(zf, filenameinzip, &zi,
        NULL, 0, NULL, 0, NULL /* comment*/,
        (level != 0) ? Z_DEFLATED : 0, level, 0,
        -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
        password, 0, zip64);

    if (err != ZIP_OK)
    {
        printf("error in opening %s in zipfile (%d)\n", filenameinzip, err);
    }
    else
    {
        fin = fopen64(path, "rb");
        if (fin == NULL)
        {
            err = ZIP_ERRNO;
            printf("error in opening %s for reading\n", path);
        }
    }

    if (err == ZIP_OK)
    {
        /* Read contents of file and write it to zip */
        do
        {
            size_read = (int)fread(buf, 1, sizeof(buf), fin);
            if ((size_read < (int)sizeof(buf)) && (feof(fin) == 0))
            {
                printf("error in reading %s\n", filenameinzip);
                err = ZIP_ERRNO;
            }

            if (size_read > 0)
            {
                err = zipWriteInFileInZip(zf, buf, size_read);
                if (err < 0)
                    printf("error in writing %s in the zipfile (%d)\n", filenameinzip, err);
            }
        } while ((err == ZIP_OK) && (size_read > 0));
    }

    if (fin)
        fclose(fin);

    if (err < 0)
    {
        err = ZIP_ERRNO;
    }
    else
    {
        err = zipCloseFileInZip(zf);
        if (err != ZIP_OK)
            printf("error in closing %s in the zipfile (%d)\n", filenameinzip, err);
    }

    return err;
}

#ifndef NOMAIN
int main(int argc, char *argv[])
{
    zipFile zf = NULL;
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc = {0};
#endif
    char *zipfilename = NULL;
    const char *password = NULL;
    int zipfilenamearg = 0;
    int errclose = 0;
    int err = 0;
    int i = 0;
    int opt_overwrite = APPEND_STATUS_CREATE;
    int opt_compress_level = Z_DEFAULT_COMPRESSION;
    int opt_exclude_path = 0;

    minizip_banner();
    if (argc == 1)
    {
        minizip_help();
        return 0;
    }

    /* Parse command line options */
    for (i = 1; i < argc; i++)
    {
        if ((*argv[i]) == '-')
        {
            const char *p = argv[i]+1;

            while ((*p) != '\0')
            {
                char c = *(p++);;
                if ((c == 'o') || (c == 'O'))
                    opt_overwrite = APPEND_STATUS_CREATEAFTER;
                if ((c == 'a') || (c == 'A'))
                    opt_overwrite = APPEND_STATUS_ADDINZIP;
                if ((c >= '0') && (c <= '9'))
                    opt_compress_level = (c - '0');
                if ((c == 'j') || (c == 'J'))
                    opt_exclude_path = 1;

                if (((c == 'p') || (c == 'P')) && (i+1 < argc))
                {
                    password=argv[i+1];
                    i++;
                }
            }
        }
        else
        {
            if (zipfilenamearg == 0)
                zipfilenamearg = i;
        }
    }

    if (zipfilenamearg == 0)
    {
        minizip_help();
        return 0;
    }
    zipfilename = argv[zipfilenamearg];

    if (opt_overwrite == 2)
    {
        /* If the file don't exist, we not append file */
        if (check_file_exists(zipfilename) == 0)
            opt_overwrite = 1;
    }
    else if (opt_overwrite == 0)
    {
        /* If ask the user what to do because append and overwrite args not set */
        if (check_file_exists(zipfilename) != 0)
        {
            char rep = 0;
            do
            {
                char answer[128];
                printf("The file %s exists. Overwrite ? [y]es, [n]o, [a]ppend : ", zipfilename);
                if (scanf("%1s", answer) != 1)
                    exit(EXIT_FAILURE);
                rep = answer[0];

                if ((rep >= 'a') && (rep <= 'z'))
                    rep -= 0x20;
            }
            while ((rep != 'Y') && (rep != 'N') && (rep != 'A'));

            if (rep == 'A')
            {
                opt_overwrite = 2;
            }
            else if (rep == 'N')
            {
                minizip_help();
                return 0;
            }
        }
    }

#ifdef USEWIN32IOAPI
    fill_win32_filefunc64A(&ffunc);
    zf = zipOpen2_64(zipfilename, opt_overwrite, NULL, &ffunc);
#else
    zf = zipOpen64(zipfilename, opt_overwrite);
#endif

    if (zf == NULL)
    {
        printf("error opening %s\n", zipfilename);
        err = ZIP_ERRNO;
    }
    else
        printf("creating %s\n", zipfilename);

    /* Go through command line args looking for files to add to zip */
    for (i = zipfilenamearg + 1; (i < argc) && (err == ZIP_OK); i++)
    {
        const char *filename = argv[i];
        const char *filenameinzip;

        /* Skip command line options */
        if ((((*(argv[i])) == '-') || ((*(argv[i])) == '/')) && (strlen(argv[i]) == 2) &&
            ((argv[i][1] == 'o') || (argv[i][1] == 'O') || (argv[i][1] == 'a') || (argv[i][1] == 'A') ||
             (argv[i][1] == 'p') || (argv[i][1] == 'P') || ((argv[i][1] >= '0') && (argv[i][1] <= '9'))))
            continue;

        /* Construct the filename that our file will be stored in the zip as.
        The path name saved, should not include a leading slash.
        If it did, windows/xp and dynazip couldn't read the zip file. */

        filenameinzip = filename;
        while (filenameinzip[0] == '\\' || filenameinzip[0] == '/')
            filenameinzip++;

        /* Should the file be stored with any path info at all? */
        if (opt_exclude_path)
        {
            const char *tmpptr = NULL;
            const char *lastslash = 0;

            for (tmpptr = filenameinzip; *tmpptr; tmpptr++)
            {
                if (*tmpptr == '\\' || *tmpptr == '/')
                    lastslash = tmpptr;
            }

            if (lastslash != NULL)
                filenameinzip = lastslash + 1; /* base filename follows last slash. */
        }

        err = minizip_addfile(zf, filename, filenameinzip, opt_compress_level, password);
    }

    errclose = zipClose(zf, NULL);
    if (errclose != ZIP_OK)
        printf("error in closing %s (%d)\n", zipfilename, errclose);

    return err;
}
#endif
