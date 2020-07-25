/* FFIND.C: This program uses the 32-bit _find functions to print
 * a list of all files (and their attributes) with a .C extension
 * in the current directory.
 */

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <stdlib.h>
#include "queue.h"
#include "find.h"

int myGlob( char *globString, struct qnode **fileNameQ, int *fileCount )
{
    struct FINDDATA_TYPE c_file;
    long hFile;
    char full_path_buffer[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    if (strchr(globString,'+') != NULL)
    {
        *fileNameQ = addq(*fileNameQ, globString, fileCount);
        return 0;
    }
    _splitpath( globString, drive, dir, fname, ext );
#ifdef _WIN32
    if( (hFile = _findfirst( globString, &c_file )) == -1L )
#else
    if( (hFile = findfirst( globString, &c_file, 0 )) == -1L )
#endif
       printf( "No files matching %s in current directory!\n", globString );
   else
   {
            if (!(c_file.FFATTRIB & FA_DIREC))
                sprintf(full_path_buffer,"%s%s%s",drive,dir,c_file.FFNAME);
                *fileNameQ = addq(*fileNameQ, full_path_buffer, fileCount);
/*              printf( " %-12s\n", c_file.FFNAME); */

            /* Find the rest of the .c files */
#ifdef _WIN32
            while( _findnext( hFile, &c_file ) == 0 )
#else
            while( findnext( &c_file ) == 0 )
#endif
            {
                if (!(c_file.FFATTRIB & FA_DIREC))
                    sprintf(full_path_buffer,"%s%s%s",drive,dir,c_file.FFNAME);
                    *fileNameQ = addq(*fileNameQ, full_path_buffer, fileCount);
/*                  printf( " %-12s\n", c_file.FFNAME);*/
            }
#ifdef _WIN32
       _findclose( hFile );
#endif
/*     qprint(fileNameQ); */
   }
   return 0;
}