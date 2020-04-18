/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#ifndef __RADEON_IOCTL_H__
#define __RADEON_IOCTL_H__

#include "main/simple_list.h"
#include "radeon_bo_gem.h"
#include "radeon_cs_gem.h"

extern void radeonEmitVertexAOS( r100ContextPtr rmesa,
				 GLuint vertex_size,
				 struct radeon_bo *bo,
				 GLuint offset );

extern void radeonEmitVbufPrim( r100ContextPtr rmesa,
				GLuint vertex_format,
				GLuint primitive,
				GLuint vertex_nr );

extern void radeonFlushElts( struct gl_context *ctx );
			    

extern GLushort *radeonAllocEltsOpenEnded( r100ContextPtr rmesa,
					   GLuint vertex_format,
					   GLuint primitive,
					   GLuint min_nr );


extern void radeonEmitAOS( r100ContextPtr rmesa,
			   GLuint n,
			   GLuint offset );

extern void radeonEmitBlit( r100ContextPtr rmesa,
			    GLuint color_fmt,
			    GLuint src_pitch,
			    GLuint src_offset,
			    GLuint dst_pitch,
			    GLuint dst_offset,
			    GLint srcx, GLint srcy,
			    GLint dstx, GLint dsty,
			    GLuint w, GLuint h );

extern void radeonEmitWait( r100ContextPtr rmesa, GLuint flags );

extern void radeonFlushCmdBuf( r100ContextPtr rmesa, const char * );

extern void radeonFlush( struct gl_context *ctx );
extern void radeonFinish( struct gl_context *ctx );
extern void radeonInitIoctlFuncs( struct gl_context *ctx );
extern void radeonGetAllParams( r100ContextPtr rmesa );
extern void radeonSetUpAtomList( r100ContextPtr rmesa );

/* ================================================================
 * Helper macros:
 */

/* Close off the last primitive, if it exists.
 */
#define RADEON_NEWPRIM( rmesa )			\
do {						\
   if ( rmesa->radeon.dma.flush )			\
      rmesa->radeon.dma.flush( rmesa->radeon.glCtx );	\
} while (0)

/* Can accomodate several state changes and primitive changes without
 * actually firing the buffer.
 */

#define RADEON_STATECHANGE( rmesa, ATOM )			\
do {								\
   RADEON_NEWPRIM( rmesa );					\
   rmesa->hw.ATOM.dirty = GL_TRUE;				\
   rmesa->radeon.hw.is_dirty = GL_TRUE;				\
} while (0)

#define RADEON_DB_STATE( ATOM )				\
   memcpy( rmesa->hw.ATOM.lastcmd, rmesa->hw.ATOM.cmd,	\
	   rmesa->hw.ATOM.cmd_size * 4)

static INLINE int RADEON_DB_STATECHANGE(r100ContextPtr rmesa,
					struct radeon_state_atom *atom )
{
   if (memcmp(atom->cmd, atom->lastcmd, atom->cmd_size*4)) {
      GLuint *tmp;
      RADEON_NEWPRIM( rmesa );
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
#if RADEON_OLD_PACKETS
#define AOS_BUFSZ(nr)	((3 + ((nr / 2) * 3) + ((nr & 1) * 2))+nr*2)
#define VERT_AOS_BUFSZ	(0)
#define ELTS_BUFSZ(nr)	(24 + nr * 2)
#define VBUF_BUFSZ	(8)
#else
#define AOS_BUFSZ(nr)	((3 + ((nr / 2) * 3) + ((nr & 1) * 2) + nr*2))
#define VERT_AOS_BUFSZ	(5)
#define ELTS_BUFSZ(nr)	(16 + nr * 2)
#define VBUF_BUFSZ	(4)
#endif
#define SCISSOR_BUFSZ	(8)
#define INDEX_BUFSZ	(7)


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


#endif /* __RADEON_IOCTL_H__ */
