#include <stdlib.h>

/* Networking       */
#define EPHEMERAL   "0"
#define SERVERSIDE  true

/* Strings          */
#define SZALLOC(n)  calloc(n, sizeof(char)) 

int     init_connection(char *, char **,    bool    );