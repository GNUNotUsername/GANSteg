#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


/*  Control Flow            */
#define FAULT               if (status != GOOD)
#define SAFE_KILL(p)        if (p) free(p)
#define STEP                if (status == GOOD)
#define STEPPED             Status status = GOOD
#define STATUPD(e)          status = e


/*  File I/O                */
#define FREAD               "r"
#define KEEP_NL             true
#define SUPPRESS            false

/*  Networking              */
#define EPHEMERAL           "0"
#define INVALID_SOCKET_FD   -1
#define PORTSIZE            6
#define SERVERSIDE          true

/*  Strings                 */
#define EOS                 '\0'
#define SZALLOC(n)          calloc(n, sizeof(char))

typedef enum {
    GOOD            = 0,
    BAD_ARGS        = 1,
    SOCKFAIL        = 2,
    FCNTLFAIL       = 3,
    BAD_AP_C        = 4,
    BAD_PORT_C      = 5,
    BAD_MODE_C      = 6,
    BAD_MTYPE_C     = 7,
    FILE_NONEXIST   = 254,
    BAD_FN_USAGE    = 255
} Status;


bool        is_numeric      (char *                         );
int         init_connection (char *,    char **,    bool    );

char    *   take_line       (FILE *,    int *,      bool    );
