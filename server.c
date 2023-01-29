/**
 * Server for client/server steganography engine.
 *
 * Usage: ./stegserver [port]
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

/* Argv                 */
#define BAD_ARGC        3
#define PORT            1
#define USE_PORT		2

/* Error Returns		*/
#define BAD_PORT		1

/* IO					*/
#define EOS				'\0'
#define FREAD			"r"
#define NEWLINE			'\n'

/* Networking			*/
#define CURL_EXEC_FAIL	"Goddammit curl"
#define GET_IP			"curl", "curl", "ifconfig.me", NULL
#define INVALID_PORT	0

/* Pipes				*/
#define FINAL           NULL, 0
#define PIPE			2
#define	READ			0
#define WRITE			1

bool			is_numeric	(char *				);
uint16_t		validate	(int,		char **	);

char		*	machine_ip	(void				);
char		*	take_line	(FILE *,	int *	);

int
main(int argc, char **argv) {
    uint16_t	port;
    char		*ip;

    port	= validate(argc, argv);
    ip		= machine_ip();

    printf("Server starting on %s:%d\n", ip, port);
	free(ip);
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

uint16_t
validate(int argc, char **argv) {
	char		*cand;
	uint16_t	port;

	port = INVALID_PORT;
	if (argc >= BAD_ARGC) return (port);
	if (argc == USE_PORT) {
		cand = argv[PORT];
		if (is_numeric(cand)) port = atoi(cand);
	} else {
		srand(time(NULL));
		port = rand();
	}

	return (port);
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