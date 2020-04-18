/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __R200_IOCTL_H__
#define __R200_IOCTL_H__

#include "main/simple_list.h"
#include "radeon_dri.h"

#include "radeon_bo_gem.h"
#include "radeon_cs_gem.h"

#include "xf86drm.h"
#include "drm.h"
#include "radeon_drm.h"

extern void r200EmitMaxVtxIndex(r200ContextPtr rmesa, int count);
extern void r200EmitVertexAOS( r200ContextPtr rmesa,
			       GLuint vertex_size,
			       struct radeon_bo *bo,
			       GLuint offset );

extern void r200EmitVbufPrim( r200ContextPtr rmesa,
				GLuint primitive,
				GLuint vertex_nr );

extern void r200FlushElts(struct gl_context *ctx);

extern GLushort *r200AllocEltsOpenEnded( r200ContextPtr rmesa,
					   GLuint primitive,
					   GLuint min_nr );

extern void r200EmitAOS(r200ContextPtr rmesa, GLuint nr, GLuint offset);

extern void r200InitIoctlFuncs( struct dd_function_table *functions );

void r200SetUpAtomList( r200ContextPtr rmesa );

/* ================================================================
 * Helper macros:
 */

/* Close off the last primitive, if it exists.
 */
#define R200_NEWPRIM( rmesa )			\
do {						\
   if ( rmesa->radeon.dma.flush )			\
      rmesa->radeon.dma.flush( rmesa->radeon.glCtx );	\
} while (0)

/* Can accomodate several state changes and primitive changes without
 * actually firing the buffer.
 */
#define R200_STATECHANGE( rmesa, ATOM )			\
do {								\
   R200_NEWPRIM( rmesa );					\
   rmesa->hw.ATOM.dirty = GL_TRUE;				\
   rmesa->radeon.hw.is_dirty = GL_TRUE;				\
} while (0)

#define R200_SET_STATE( rmesa, ATOM, index, newvalue ) 	\
  do {	\
    uint32_t __index = (index); \
    uint32_t __dword = (newvalue); \
    if (__dword != (rmesa)->hw.ATOM.cmd[__index]) { \
      R200_STATECHANGE( (rmesa), ATOM ); \
      (rmesa)->hw.ATOM.cmd[__index] = __dword; \
    } \
  } while(0)

#define R200_DB_STATE( ATOM )			        \
   memcpy( rmesa->hw.ATOM.lastcmd, rmesa->hw.ATOM.cmd,	\
	   rmesa->hw.ATOM.cmd_size * 4)

static INLINE int R200_DB_STATECHANGE( 
   r200ContextPtr rmesa,
   struct radeon_state_atom *atom )
{
   if (memcmp(atom->cmd, atom->lastcmd, atom->cmd_size*4)) {
      GLuint *tmp;
      R200_NEWPRIM( rmesa );
      atom->dirty = GL_TRUE;
      rmesa->radeon.hw.is_dirty = GL_TRUE;
      tmp = atom->cmd; 
      atom->cmd = atom->lastcmd;
      atom->lastcmd = tmp;
      return 1;
   }
   else
      return 0;
}


/* Command lengths.  Note that any time you ensure ELTS_BUFSZ or VBUF_BUFSZ
 * are available, you will also be adding an rmesa->state.max_state_size because
 * r200EmitState is called from within r200EmitVbufPrim and r200FlushElts.
 */
#define AOS_BUFSZ(nr)	((3 + ((nr / 2) * 3) + ((nr & 1) * 2) + nr*2))
#define VERT_AOS_BUFSZ	(5)
#define ELTS_BUFSZ(nr)	(12 + nr * 2)
#define VBUF_BUFSZ	(3)
#define SCISSOR_BUFSZ	(8)
#define INDEX_BUFSZ	(8+2)

static inline uint32_t cmdpacket3(int cmd_type)
{
  drm_radeon_cmd_header_t cmd;

  cmd.i = 0;
  cmd.header.cmd_type = cmd_type;

  return (uint32_t)cmd.i;

}

#define OUT_BATCH_PACKET3(packet, num_extra) do {	      \
    OUT_BATCH(CP_PACKET2);				      \
    OUT_BATCH(CP_PACKET3((packet), (num_extra)));	      \
  } while(0)

#define OUT_BATCH_PACKET3_CLIP(packet, num_extra) do {	      \
    OUT_BATCH(CP_PACKET2);				      \
    OUT_BATCH(CP_PACKET3((packet), (num_extra)));	      \
  } while(0)


#endif /* __R200_IOCTL_H__ */
