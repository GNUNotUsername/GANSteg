/**
 *  Server for client/server steganography engine.
 *
 *  Usage: ./stegserver [-q] [port]
 */

//#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
//#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "general.h"

/*  Argv                    */
#define ARGS_BEGIN          1
#define ARGVINDRNG          3
#define BAD_ARGC            4
#define PORT_DONE           128
#define QUIET               "-q"
#define QUIET_DONE          64

/*  Control Flow            */
#define EVER                ;;

/*  Error Handling          */
#define DESTROY_ABORT(s)    if (s) free(s); s = NULL; goto ABORT

/*  Error Returns           */
#define GOOD                0
#define BAD_ARGS            1
#define SOCKFAIL            2
#define FCNTLFAIL           3

/*  File Descriptors        */
#define ACCEPT_FAIL         -1
#define FCNTL_FAIL          -1

/*  IO                      */

/*  Networking              */
#define CURL_EXEC_FAIL      "Goddammit curl"
#define GET_IP              "curl", "curl", "ifconfig.me", NULL
#define INVALID_PORT        0
#define RAND_PORT           6

/*  Pipes                   */
#define FINAL               NULL, 0
#define PIPE                2
#define	READ                0
#define SUPPRESS_OUTP       "/dev/null"
#define WRITE               1

/*  Status                  */
#define STARTLEN            42
#define START_WITH_IP       "Server starting on %s:%s\n"
#define START_NO_IP         "Server starting on port %s\n"
#define SERVER_SOCKET_FAIL  "%s\n", "Could not create socket."


int         accept_connections  (char *,    char *,     bool    );
int         reap                (pid_t **,  int                 );
void        service             (int                            );

char    *   machine_ip          (void                           );
char    *   validate            (int,       char **,    bool *  );

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
service(int client) {
    printf("We got a connection on socket %d yayyyyyyy\n", client);
    close(client);
}

/**
 *  Open a socket and service incoming connections
 *
 *  addr    - Public IPv4 address of this server.
 *  port    - Port top listen on; ephemeral "0" for random selected by kernel.
 *  verbose - True to display server public IPv4 address; else false.
 *
 *  return  - error code on failure; else non-terminating.
 */
int
accept_connections(char *addr, char *port, bool verbose) {
    int                 client,     conns, serverfd;
    char                *message,   *tport;
    socklen_t           clSiz;
    pid_t               handler,    *openConns;
    struct sockaddr_in  clAddr;

    tport       = strdup(port);
    serverfd    = init_connection(NULL, &tport, SERVERSIDE);
    if (serverfd != INVALID_SOCKET_FD) {
        message = SZALLOC(STARTLEN);
        if (verbose) sprintf(message, START_WITH_IP, addr, tport);
        else sprintf(message, START_NO_IP, tport);
        printf("%s", message);
        printf("%d\n", serverfd);
        free(message);
    } else printf(SERVER_SOCKET_FAIL);

    free(port);
    free(tport);
    if (serverfd == INVALID_SOCKET_FD)                      return (SOCKFAIL);
    if (fcntl(serverfd, F_SETFL, O_NONBLOCK) == FCNTL_FAIL) return (FCNTLFAIL);
    openConns   = NULL;
    conns       = 0;

    for (EVER) {
        clSiz   = sizeof(struct sockaddr_in);
        client  = accept(serverfd, (struct sockaddr *)&clAddr, &clSiz);
        if (client == ACCEPT_FAIL) conns = reap(&openConns, conns);
        else {
            handler = fork();
            if (handler) {
                openConns = realloc(openConns, conns + 1);
                openConns[conns++] = handler;
            } else {
                service(client);
                break;
            }
        }
    }

    return (GOOD);
}

/**
 *  Reap old service processes.
 *
 *  active  - Reference to buffer of IDs for all active service processes.
 *  len     - The number of currently active service processes.
 *
 *  return  - The number of processes still alive.
 */
int
reap(pid_t **active, int len) {
    int     check,  kept;
    pid_t   cand,   *alive;

    /* There's gotta be a better way of doing this. I'm tired ok    */
    alive = NULL;
    for (check = kept = 0; check < len; check++) {
        cand = (*active)[check];
        if (!waitpid(cand, NULL, WNOHANG)) {
            alive = realloc(alive, sizeof(pid_t) * (kept + 1));
            alive[kept++] = cand;
        }
    }

    *active = alive;

    return (kept);
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
        ip = take_line(child, &size, SUPPRESS);
        fclose(child);
    } else {
        /* Child	*/
        suppress = open(SUPPRESS_OUTP, O_WRONLY);  
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
