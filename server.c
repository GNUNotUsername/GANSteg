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


/* Argv					*/
#define BAD_ARGC		3
#define PORT			1
#define USE_PORT		2

/* Error Returns		*/
#define BAD_PORT		1

/* Networking			*/
#define INVALID_PORT	0


bool			is_numeric	(char *			);
uint16_t		validate	(int,	char **	);


int
main(int argc, char **argv) {
	uint16_t port;

	port = validate(argc, argv);

	printf("Server starting on %d\n", port);
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
	} else port = rand();

	return (port);
}
