#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define BYTE_ORDER  BIG_ENDIAN

// we have unistd.h available (it's not auto-detected since SSIZE_MAX isn't set)
// bug in newlib?
#define SSIZE_MAX INT_MAX

#endif /* __ARCH_CC_H__ */


