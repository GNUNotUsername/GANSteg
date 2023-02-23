/**
 *  Client for client/server steganography engine.
 *
 *  Usage: ./stegclient address:port -m message | -f file | -d image
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "general.h"


/* Argv                 */
#define ADDR_PORT       1
#define AP_DELIM        ":"
#define DECRYPT_MODE    'd'
#define FILE_MODE       'f'
#define FLAG_SIG        '-'
#define GOOD_ARGC       4
#define MESSAGE_MODE    'm'
#define MODE            2
#define MODE_FLAG_LEN   2
#define MF_IND          0
#define MT_IND          1
#define SRC_IND         3

typedef struct {
    char    *addr;
    char    *port;
    char    *contents;
    int     len;
    bool    dir;
} Options;

Status  rip_contents(char *,    char **,    int *       );
Status  validate    (int,       char **,    Options *   );

int
main(int argc, char **argv) {
    STEPPED;
    Options options;
    int     sockfd;

    STATUPD(validate(argc, argv, &options));
    STEP {
        sockfd = init_connection(options.addr, &(options.port), CLIENTSIDE);
        if (sockfd == INVALID_SOCKET_FD) STATUPD(SOCKFAIL);
    }

    // TEMP
    STEP {
        FILE *a = fdopen(sockfd, READWRITE);
        int c;
        char *b = take_line(a, &c, SUPPRESS);
        printf("%s\n", b);
        free(b);
    }

    return (status);
}

Status
rip_contents(char *src, char **dest, int *len) {
    STEPPED;
    int     cumlSize,   lineSize;
    char    *cuml,      *line;
    FILE    *stream;

    stream = fopen(src, FREAD);
    if (!stream) STATUPD(FILE_NONEXIST);

    STEP {
        cuml = NULL;
        while (!(feof(stream) || ferror(stream))) {
            line = take_line(stream, &lineSize, KEEP_NL);
            cuml = realloc(cuml, (cumlSize + lineSize + 1) * sizeof(char));
            memcpy(&(cuml[cumlSize]), line, lineSize);
            cumlSize += lineSize;
            cuml[cumlSize] = EOS;
        }
    }

    *len    = (cumlSize + 1);
    *dest   = cuml;

    return (status);
}

Status
validate(int argc, char **argv, Options *options) {
    STEPPED;
    char modeType, *addr, *addrPort, *contents, *mode, *port, *src, *temp;

    if (!options) { STATUPD(BAD_FN_USAGE); }
    bzero(options, sizeof(Options));
    addrPort = addr = port = NULL;

    STEP { if (argc != GOOD_ARGC) STATUPD(BAD_ARGS); }

    STEP {
        addrPort    = strdup(argv[ADDR_PORT]);
        temp        = strtok(argv[ADDR_PORT], AP_DELIM);
        if (!strcmp(temp, addrPort)) STATUPD(BAD_AP_C);
    }

    STEP {
        addr    = strdup(temp);
        temp    = strtok(NULL, AP_DELIM);
        if (is_numeric(temp)) port = strdup(temp);
        else STATUPD(BAD_PORT_C);
        temp    = strtok(NULL, AP_DELIM);
        if (temp) STATUPD(BAD_AP_C);
    }

    STEP {
        mode = argv[MODE];
        if ((strlen(mode) != MODE_FLAG_LEN) || (mode[MF_IND] != FLAG_SIG))
            STATUPD(BAD_MODE_C);
    }

    STEP {
        modeType = mode[MT_IND];
        if ((modeType != MESSAGE_MODE) && (modeType != FILE_MODE)
                && (modeType != DECRYPT_MODE))
            STATUPD(BAD_MTYPE_C);
    }

    STEP {
        src = argv[SRC_IND];
        if (modeType == MESSAGE_MODE) {
            contents        = strdup(src);
            options->len    = strlen(contents);
        } else STATUPD(rip_contents(src, &contents, &(options->len)));
    }

    STEP {
        options->contents   = contents;
        options->addr       = addr;
        options->port       = port;
        options->dir        = modeType == DECRYPT_MODE;
    }

    FAULT {
        SAFE_KILL(addr);
        SAFE_KILL(port);
    }
    SAFE_KILL(addrPort);

    return (status);
}
