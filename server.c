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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

/* Argv                 	*/
#define ARGS_BEGIN			1
#define ARGVINDRNG			3
#define BAD_ARGC        	4
#define PORT_DONE			128
#define QUIET				"-q"
#define QUIET_DONE			64

/* Error Handling			*/
#define DESTROY_ABORT(s)	if (s) free(s); break

/* Error Returns			*/
#define GOOD				0
#define BAD_ARGS			1

/* IO						*/
#define EOS					'\0'
#define FREAD				"r"
#define NEWLINE				'\n'

/* Networking				*/
#define CURL_EXEC_FAIL		"Goddammit curl"
#define GET_IP				"curl", "curl", "ifconfig.me", NULL
#define INVALID_PORT		0
#define RAND_PORT			6

/* Pipes					*/
#define FINAL           	NULL, 0
#define PIPE				2
#define	READ				0
#define WRITE				1

/* Status					*/
#define IP_STRLEN			22
#define NIP_STRLEN			25
#define START_NO_IP			"Server starting on port %s\n"
#define START_WITH_IP		"Server starting on %s:%s\n"

/* Strings					*/
#define ASPRINTF_FAIL		-1
#define SZALLOC(n)			calloc(n, sizeof(char))


bool		is_numeric	(char *								);
int			asprintf	(char **,	const char *,	...		);

char	*	machine_ip	(void								);
char	*	take_line	(FILE *,	int *					);
char	*	validate	(int,		char **,		bool *	);

int
main(int argc, char **argv) {
    int		portLen;
    char	*ip, *message, *port;
	bool	verbose;

    port = validate(argc, argv, &verbose);
	if (!port) return BAD_ARGS;
	portLen = strlen(port);
	if (verbose) {
		ip = machine_ip();
		message = SZALLOC(IP_STRLEN + strlen(ip) + portLen);
		sprintf(message, START_WITH_IP, ip, port);
		free(ip);
	} else {
		message = SZALLOC(NIP_STRLEN + portLen);
	    sprintf(message, START_NO_IP, port);
	}

    printf("%s", message);
	free(message);
	free(port);

	return (GOOD);
}

bool
is_numeric(char *test) {
	bool	verdict;
	int		check;

	check	= 0;
	verdict	= true;
	while (verdict && isdigit(test[check])) check++;

	return (verdict);
}

char *
machine_ip(void) {
	FILE	*child;
	int		size, cStdout[PIPE];
	pid_t	pid;
	char	*ip;

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
	char	add, *out;
	int		len;

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
	char		*cand, *port;
	uint16_t	tempPort;
	uint8_t		check;

	port	= NULL;
	*verb	= true;
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
			port	= strdup(cand);
			check	|= PORT_DONE;
		} else {
			DESTROY_ABORT(port);
		}
	}

	if (!(check & PORT_DONE)) {
		port = SZALLOC(RAND_PORT);
		srand(time(NULL));
		tempPort = rand();
		sprintf(port, "%d", tempPort);
	}

	return (port);
}
