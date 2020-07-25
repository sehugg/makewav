#ifdef _WIN32
    #define FINDDATA_TYPE _finddata_t
    #define FFATTRIB attrib
    #define FFNAME name
    #define FA_DIREC _A_SUBDIR
#else
    #include <dir.h>

    #define FINDDATA_TYPE ffblk
    #define FFATTRIB      ff_attrib
    #define _findfirst    findfirst
    #define _makepath     fnmerge
    #define _splitpath    fnsplit
    #define FFNAME        ff_name
    #define _MAX_DRIVE    MAXDRIVE
    #define _MAX_PATH     MAXPATH
    #define _MAX_DIR      MAXDIR
    #define _MAX_FNAME    MAXFILE
    #define _MAX_EXT      MAXEXT
#endif

int myGlob( char *globString, struct qnode **fileNameQ, int *fileCount );