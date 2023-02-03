/**
 * Server for client/server steganography engine.
 *
 * Usage: ./stegserver [-q] [port]
 */

#include <ctype.h>
#include <fcntl.h>
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
#define SUPPRESS            "/dev/null"
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

/**
 *  Open a socket and service incoming connections
 *
 *  addr    - Public IPv4 address of this server.
 *  port    - Port top listen on; ephemeral "0" for random selected by kernel.
 *  verbose - True to display server public IPv4 address; else false.
 */
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
 *  returns - The public IPv4 address of this machine.
 *  NOTE:   - There is almost definitely a better way than this.
 *              This should be fixed later.
 */
char *
machine_ip(void) {
    FILE    *child;
    int     size, suppress, cStdout[PIPE];
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
        suppress = open(SUPPRESS, O_WRONLY);  
        dup2(suppress, STDERR_FILENO);
        dup2(cStdout[WRITE], STDOUT_FILENO);
        close(cStdout[READ]);
        close(cStdout[WRITE]);
        execlp(GET_IP);
        ip = strdup(CURL_EXEC_FAIL);
    }

    return (ip);
}

/**
 *  Read one string terminated by a newline or a NUL terminator from a stream.
 *
 *  src     - File stream to read from.
 *  size    - Reference to length of string read.
 *
 *  return  - NUL-terminated line taken from `src`.
 */
char *
take_line(FILE *src, int *size) {
    char    add, *out;
    int     len;

    out = NULL;
    len	= 0;
    while (!(feof(src) || ferror(src))) {
        add = fgetc(src);
        if ((add == NEWLINE) || (add == EOF)) add = EOS;
        out = realloc(out, sizeof(char) * (len + 1));
        out[len++] = add;
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

/**
 *  Determine whether or not entered cmdline args were legal.
 *
 *  argc    - Argument count
 *  argv    - Argument vectors
 *  verb    - Reference to whether or not to display machine IP.
 *
 *  return  - The (possibly ephemeral) port to open the server on.
 */
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
