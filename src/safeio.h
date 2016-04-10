#ifndef __SAFEIO_H__
#define __SAFEIO_H__
#include<stdlib.h>

#define DIE(msg)\
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#endif
