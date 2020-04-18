
#ifndef __NV50_DEBUG_H__
#define __NV50_DEBUG_H__

#include <stdio.h>

#include "util/u_debug.h"

#define NV50_DEBUG_MISC       0x0001
#define NV50_DEBUG_SHADER     0x0100
#define NV50_DEBUG_PROG_IR    0x0200
#define NV50_DEBUG_PROG_RA    0x0400
#define NV50_DEBUG_PROG_CFLOW 0x0800
#define NV50_DEBUG_PROG_ALL   0x1f00

#define NV50_DEBUG 0

#define NOUVEAU_ERR(fmt, args...)                                 \
   fprintf(stderr, "%s:%d - "fmt, __FUNCTION__, __LINE__, ##args)

#define NV50_DBGMSG(ch, args...)           \
   if ((NV50_DEBUG) & (NV50_DEBUG_##ch))        \
      debug_printf(args)

#endif /* __NV50_DEBUG_H__ */
