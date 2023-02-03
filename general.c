#include <limits.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "general.h"

/* Networking               */
#define INVALID_SOCKET_FD   -1
#define PORTSIZE            6

int
init_connection(char *address, char **port, bool serverSide) {
	struct addrinfo		*addrList, *check, criteria;
	struct sockaddr_in	addr;
    char                *tport;
	int					socketfd, err;
	socklen_t			len;

	/* Assume this is run by client and then rectify later if not */
	socketfd = INVALID_SOCKET_FD;
	bzero(&criteria, sizeof(struct addrinfo));
	addrList = NULL;
	criteria.ai_family = AF_UNSPEC;
	criteria.ai_socktype = SOCK_STREAM;
	err = 0;

	if (serverSide) {
		/* Alter a few options for server usage */
		criteria.ai_flags = AI_PASSIVE;
		address = NULL;
	}

	err = getaddrinfo(address, *port, &criteria, &addrList);
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

	if (err)	socketfd = INVALID_SOCKET_FD;
	else {
        /* This could probably be better */
        free(*port);
        tport = SZALLOC(PORTSIZE);
        sprintf(tport, "%d", ntohs(addr.sin_port));
        *port = tport;
    }

	return (socketfd);
}
