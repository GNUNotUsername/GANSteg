#include <limits.h>
#include <netdb.h>
#include <stdbool.h>
//#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "general.h"


/*  IO                      */
#define EMPTY               1
#define EOS                 '\0'
#define NEWLINE             '\n'

/* Networking               */
#define PORTSIZE            6


/**
 * returns  - True iff `test` contains only ascii digits and a NUL-terminator.
 * requires - `test` is NUL-terminated.
 */
bool
is_numeric(char *test) {
    bool    verdict;
    int     check;

    check   = 0;
    verdict = true;

    while ((test[check]) && (verdict = isdigit(test[check]))) check++;

    return (verdict);
}

/**
 *  Open a socket; can be used by both client and server side.
 *
 *  address - Address of server if used by client, ignored if used by server.
 *  port    - Reference to port to listen on / connect to.
 *              If ephemeral port "0" was used, reference is updated to new
 *              string containing port selected by the kernel
 *  bool    - True iff called by server process; else false
 *
 *  return  - Resulting network socket or error code
 */
int
init_connection(char *address, char **port, bool serverSide) {
    struct addrinfo     *addrList, *check, crits;
    struct sockaddr_in  addr;
    char                *tport;
    int                 socketfd, err;
    socklen_t           len;

    /* Assume this is run by client and then rectify later if not */
    socketfd = INVALID_SOCKET_FD;
    bzero(&crits, sizeof(struct addrinfo));
    addrList            = NULL;
    crits.ai_family     = AF_UNSPEC;
    crits.ai_socktype   = SOCK_STREAM;
    err = 0;

    if (serverSide) {
        /* Alter a few options for server usage */
        crits.ai_flags  = AI_PASSIVE;
        address         = NULL;
    }

    err = getaddrinfo(address, *port, &crits, &addrList);
    if (!err) {
        for (check = addrList; check != NULL; check = check->ai_next) {
            socketfd = socket(check->ai_family, check->ai_socktype,
                    check->ai_protocol);

            if (socketfd == INVALID_SOCKET_FD) continue;
            if (serverSide) {
                err |= bind(socketfd,
                        (struct sockaddr*)check->ai_addr,
                        sizeof(struct sockaddr));
                err |= listen(socketfd, INT_MAX);
                if (!err) {
                    len = sizeof(struct sockaddr_in);
                    bzero(&addr, len);
                    err = getsockname(
                            socketfd, (struct sockaddr *)&addr, &len);
                }
            } else err = connect(socketfd,
                    (struct sockaddr*)check->ai_addr,
                    sizeof(struct sockaddr));

	        if (!err) break;
            close(socketfd);
        }
        freeaddrinfo(addrList);
    }

    if (err) socketfd = INVALID_SOCKET_FD;
    else {
        /* This could probably be better */
        free(*port);
        tport = SZALLOC(PORTSIZE);
        sprintf(tport, "%d", ntohs(addr.sin_port));
        *port = tport;
    }

    return (socketfd);
}

/**
 *  Read one NUL / newline terminated string from a stream.
 *
 *  src         - File stream to read from.
 *  size        - Reference to length of string read.
 *  newlines    - Whether or not to suppress newlines
 *
 *  return      - NUL-terminated line taken from `src`.
 */
char *
take_line(FILE *src, int *size, bool newlines) {
    char    add, *out;
    int     len;

    out = NULL;
    len = 0;
    while (!(feof(src) || ferror(src))) {
        add = fgetc(src);
        if ((!newlines && (add == NEWLINE)) || (add == EOF)) add = EOS;
        out = realloc(out, sizeof(char) * (len + 1));
        out[len++] = add;
        if (!add || (!newlines && (add == NEWLINE))) break;
    }

    /* Just in case the first thing read is EOF or an empty string  */
    if (len == EMPTY) {
        free(out);
        out = NULL;
        len = 0;
    }
    *size = len;

    return (out);
}
