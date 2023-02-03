/**
 * Server for client/server steganography engine.
 *
 * Usage: ./stegserver [-q] [port]
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "general.h"

/* Argv                     */
#define ARGS_BEGIN          1
#define ARGVINDRNG          3
#define BAD_ARGC            4
#define PORT_DONE           128
#define QUIET               "-q"
#define QUIET_DONE          64

/* Error Handling           */
#define DESTROY_ABORT(s)    if (s) free(s); s = NULL; goto ABORT

/* Error Returns            */
#define GOOD                0
#define BAD_ARGS            1

/* IO                       */
#define EOS                 '\0'
#define FREAD               "r"
#define NEWLINE             '\n'

/* Networking               */
#define CURL_EXEC_FAIL      "Goddammit curl"
#define GET_IP              "curl", "curl", "ifconfig.me", NULL
#define INVALID_PORT        0
#define RAND_PORT           6

/* Pipes                    */
#define FINAL               NULL, 0
#define PIPE                2
#define	READ                0
#define WRITE               1

/* Status                   */
#define STARTLEN            42
#define START_WITH_IP       "Server starting on %s:%s\n"
#define START_NO_IP         "Server starting on port %s\n"
#define SERVER_SOCKET_FAIL  "%s\n", "Could not create socket."


void        accept_connections  (char *,         char *,    bool    );
bool        is_numeric	        (char *                             );

char    *   machine_ip          (void                               );
char    *   take_line	        (FILE *,    int *                   );
char    *   validate            (int,       char **,        bool *  );

int
main(int argc, char **argv) {
    char    *ip, *port;
    bool    verbose;

    port = validate(argc, argv, &verbose);
    if (!port) return BAD_ARGS;
    ip = machine_ip();
	
    accept_connections(ip, port, verbose);

    return (GOOD);
}

void
accept_connections(char *addr, char *port, bool verbose) {
    int     serverfd;
    char    *message, *tport;

    tport       = strdup(port);
    serverfd    = init_connection(NULL, &tport, SERVERSIDE);
    if (serverfd) {
        message = SZALLOC(STARTLEN);
        if (verbose) sprintf(message, START_WITH_IP, addr, tport);
        else sprintf(message, START_NO_IP, tport);
        printf("%s", message);
        printf("%d\n", serverfd);
        free(message);
    } else printf(SERVER_SOCKET_FAIL);
    free(port);
    free(tport);
}

bool
is_numeric(char *test) {
	bool    verdict;
    int     check;

    check   = 0;
    verdict = true;
    while ((test[check]) && (verdict = isdigit(test[check]))) check++;

    return (verdict);
}

char *
machine_ip(void) {
    FILE    *child;
    int     size, cStdout[PIPE];
    pid_t   pid;
    char    *ip;

    pipe(cStdout);
    if ((pid = fork())) {
        /* Parent	*/
        close(cStdout[WRITE]);
        waitpid(pid, FINAL);
        child = fdopen(cStdout[READ], FREAD);
        ip = take_line(child, &size);
        fclose(child);
    } else {
        /* Child	*/
        dup2(cStdout[WRITE], STDOUT_FILENO);
        close(cStdout[READ]);
        close(cStdout[WRITE]);
        execlp(GET_IP);
        ip = strdup(CURL_EXEC_FAIL);
    }

    return (ip);
}

char *
take_line(FILE *src, int *size) {
    char    add, *out;
    int     len;

    out = NULL;
    len	= 0;
    while (!(feof(src) || ferror(src))) {
        add = fgetc(src);
        if (add == EOF) break;
        if (add == NEWLINE) add = EOS;
        out = realloc(out, sizeof(char) * (len + 1));
        out[len++] = add;
    }

    *size = len;

    return (out);
}

char *
validate(int argc, char **argv, bool *verb) {
    char        *cand, *port;
    uint8_t     check;

    port    = NULL;
    *verb   = true;
    if (argc >= BAD_ARGC) return (port);
    for (check = ARGS_BEGIN; (check & ARGVINDRNG) < argc; check++) {
        cand = argv[(check & ARGVINDRNG)];
        if (!strcmp(cand, QUIET)) {
            if (check & QUIET_DONE) {
                DESTROY_ABORT(port);
            } else {
                *verb = false;
                check |= QUIET_DONE;
            }
        } else if (is_numeric(cand)) {
            if (check & PORT_DONE) {
                DESTROY_ABORT(port);
            }
            port    = strdup(cand);
			check   |= PORT_DONE;
        } else {
            DESTROY_ABORT(port);
        }
    }

    if (!(check & PORT_DONE)) port = strdup(EPHEMERAL);

ABORT:

    return (port);
}
