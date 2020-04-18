
/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2012 Marek Olšák <maraeo@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * This converts the VBO's vertex attribute/array information into
 * Gallium vertex state and binds it.
 *
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Marek Olšák <maraeo@gmail.com>
 */

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bufferobjects.h"
#include "st_draw.h"
#include "st_program.h"

#include "cso_cache/cso_context.h"
#include "util/u_math.h"

#include "main/bufferobj.h"
#include "main/glformats.h"


static GLuint double_types[4] = {
   PIPE_FORMAT_R64_FLOAT,
   PIPE_FORMAT_R64G64_FLOAT,
   PIPE_FORMAT_R64G64B64_FLOAT,
   PIPE_FORMAT_R64G64B64A64_FLOAT
};

static GLuint float_types[4] = {
   PIPE_FORMAT_R32_FLOAT,
   PIPE_FORMAT_R32G32_FLOAT,
   PIPE_FORMAT_R32G32B32_FLOAT,
   PIPE_FORMAT_R32G32B32A32_FLOAT
};

static GLuint half_float_types[4] = {
   PIPE_FORMAT_R16_FLOAT,
   PIPE_FORMAT_R16G16_FLOAT,
   PIPE_FORMAT_R16G16B16_FLOAT,
   PIPE_FORMAT_R16G16B16A16_FLOAT
};

static GLuint uint_types_norm[4] = {
   PIPE_FORMAT_R32_UNORM,
   PIPE_FORMAT_R32G32_UNORM,
   PIPE_FORMAT_R32G32B32_UNORM,
   PIPE_FORMAT_R32G32B32A32_UNORM
};

static GLuint uint_types_scale[4] = {
   PIPE_FORMAT_R32_USCALED,
   PIPE_FORMAT_R32G32_USCALED,
   PIPE_FORMAT_R32G32B32_USCALED,
   PIPE_FORMAT_R32G32B32A32_USCALED
};

static GLuint uint_types_int[4] = {
   PIPE_FORMAT_R32_UINT,
   PIPE_FORMAT_R32G32_UINT,
   PIPE_FORMAT_R32G32B32_UINT,
   PIPE_FORMAT_R32G32B32A32_UINT
};

static GLuint int_types_norm[4] = {
   PIPE_FORMAT_R32_SNORM,
   PIPE_FORMAT_R32G32_SNORM,
   PIPE_FORMAT_R32G32B32_SNORM,
   PIPE_FORMAT_R32G32B32A32_SNORM
};

static GLuint int_types_scale[4] = {
   PIPE_FORMAT_R32_SSCALED,
   PIPE_FORMAT_R32G32_SSCALED,
   PIPE_FORMAT_R32G32B32_SSCALED,
   PIPE_FORMAT_R32G32B32A32_SSCALED
};

static GLuint int_types_int[4] = {
   PIPE_FORMAT_R32_SINT,
   PIPE_FORMAT_R32G32_SINT,
   PIPE_FORMAT_R32G32B32_SINT,
   PIPE_FORMAT_R32G32B32A32_SINT
};

static GLuint ushort_types_norm[4] = {
   PIPE_FORMAT_R16_UNORM,
   PIPE_FORMAT_R16G16_UNORM,
   PIPE_FORMAT_R16G16B16_UNORM,
   PIPE_FORMAT_R16G16B16A16_UNORM
};

static GLuint ushort_types_scale[4] = {
   PIPE_FORMAT_R16_USCALED,
   PIPE_FORMAT_R16G16_USCALED,
   PIPE_FORMAT_R16G16B16_USCALED,
   PIPE_FORMAT_R16G16B16A16_USCALED
};

static GLuint ushort_types_int[4] = {
   PIPE_FORMAT_R16_UINT,
   PIPE_FORMAT_R16G16_UINT,
   PIPE_FORMAT_R16G16B16_UINT,
   PIPE_FORMAT_R16G16B16A16_UINT
};

static GLuint short_types_norm[4] = {
   PIPE_FORMAT_R16_SNORM,
   PIPE_FORMAT_R16G16_SNORM,
   PIPE_FORMAT_R16G16B16_SNORM,
   PIPE_FORMAT_R16G16B16A16_SNORM
};

static GLuint short_types_scale[4] = {
   PIPE_FORMAT_R16_SSCALED,
   PIPE_FORMAT_R16G16_SSCALED,
   PIPE_FORMAT_R16G16B16_SSCALED,
   PIPE_FORMAT_R16G16B16A16_SSCALED
};

static GLuint short_types_int[4] = {
   PIPE_FORMAT_R16_SINT,
   PIPE_FORMAT_R16G16_SINT,
   PIPE_FORMAT_R16G16B16_SINT,
   PIPE_FORMAT_R16G16B16A16_SINT
};

static GLuint ubyte_types_norm[4] = {
   PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R8G8_UNORM,
   PIPE_FORMAT_R8G8B8_UNORM,
   PIPE_FORMAT_R8G8B8A8_UNORM
};

static GLuint ubyte_types_scale[4] = {
   PIPE_FORMAT_R8_USCALED,
   PIPE_FORMAT_R8G8_USCALED,
   PIPE_FORMAT_R8G8B8_USCALED,
   PIPE_FORMAT_R8G8B8A8_USCALED
};

static GLuint ubyte_types_int[4] = {
   PIPE_FORMAT_R8_UINT,
   PIPE_FORMAT_R8G8_UINT,
   PIPE_FORMAT_R8G8B8_UINT,
   PIPE_FORMAT_R8G8B8A8_UINT
};

static GLuint byte_types_norm[4] = {
   PIPE_FORMAT_R8_SNORM,
   PIPE_FORMAT_R8G8_SNORM,
   PIPE_FORMAT_R8G8B8_SNORM,
   PIPE_FORMAT_R8G8B8A8_SNORM
};

static GLuint byte_types_scale[4] = {
   PIPE_FORMAT_R8_SSCALED,
   PIPE_FORMAT_R8G8_SSCALED,
   PIPE_FORMAT_R8G8B8_SSCALED,
   PIPE_FORMAT_R8G8B8A8_SSCALED
};

static GLuint byte_types_int[4] = {
   PIPE_FORMAT_R8_SINT,
   PIPE_FORMAT_R8G8_SINT,
   PIPE_FORMAT_R8G8B8_SINT,
   PIPE_FORMAT_R8G8B8A8_SINT
};

static GLuint fixed_types[4] = {
   PIPE_FORMAT_R32_FIXED,
   PIPE_FORMAT_R32G32_FIXED,
   PIPE_FORMAT_R32G32B32_FIXED,
   PIPE_FORMAT_R32G32B32A32_FIXED
};


/**
 * Return a PIPE_FORMAT_x for the given GL datatype and size.
 */
enum pipe_format
st_pipe_vertex_format(GLenum type, GLuint size, GLenum format,
                      GLboolean normalized, GLboolean integer)
{
   assert((type >= GL_BYTE && type <= GL_DOUBLE) ||
          type == GL_FIXED || type == GL_HALF_FLOAT ||
          type == GL_INT_2_10_10_10_REV ||
          type == GL_UNSIGNED_INT_2_10_10_10_REV);
   assert(size >= 1);
   assert(size <= 4);
   assert(format == GL_RGBA || format == GL_BGRA);

   if (type == GL_INT_2_10_10_10_REV ||
       type == GL_UNSIGNED_INT_2_10_10_10_REV) {
      assert(size == 4);
      assert(!integer);

      if (format == GL_BGRA) {
         if (type == GL_INT_2_10_10_10_REV) {
            if (normalized)
               return PIPE_FORMAT_B10G10R10A2_SNORM;
            else
               return PIPE_FORMAT_B10G10R10A2_SSCALED;
         } else {
            if (normalized)
               return PIPE_FORMAT_B10G10R10A2_UNORM;
            else
               return PIPE_FORMAT_B10G10R10A2_USCALED;
         }
      } else {
         if (type == GL_INT_2_10_10_10_REV) {
            if (normalized)
               return PIPE_FORMAT_R10G10B10A2_SNORM;
            else
               return PIPE_FORMAT_R10G10B10A2_SSCALED;
         } else {
            if (normalized)
               return PIPE_FORMAT_R10G10B10A2_UNORM;
            else
               return PIPE_FORMAT_R10G10B10A2_USCALED;
         }
      }
   }

   if (format == GL_BGRA) {
      /* this is an odd-ball case */
      assert(type == GL_UNSIGNED_BYTE);
      assert(normalized);
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   }

   if (integer) {
      switch (type) {
      case GL_INT: return int_types_int[size-1];
      case GL_SHORT: return short_types_int[size-1];
      case GL_BYTE: return byte_types_int[size-1];
      case GL_UNSIGNED_INT: return uint_types_int[size-1];
      case GL_UNSIGNED_SHORT: return ushort_types_int[size-1];
      case GL_UNSIGNED_BYTE: return ubyte_types_int[size-1];
      default: assert(0); return 0;
      }
   }
   else if (normalized) {
      switch (type) {
      case GL_DOUBLE: return double_types[size-1];
      case GL_FLOAT: return float_types[size-1];
      case GL_HALF_FLOAT: return half_float_types[size-1];
      case GL_INT: return int_types_norm[size-1];
      case GL_SHORT: return short_types_norm[size-1];
      case GL_BYTE: return byte_types_norm[size-1];
      case GL_UNSIGNED_INT: return uint_types_norm[size-1];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size-1];
      case GL_UNSIGNED_BYTE: return ubyte_types_norm[size-1];
      case GL_FIXED: return fixed_types[size-1];
      default: assert(0); return 0;
      }
   }
   else {
      switch (type) {
      case GL_DOUBLE: return double_types[size-1];
      case GL_FLOAT: return float_types[size-1];
      case GL_HALF_FLOAT: return half_float_types[size-1];
      case GL_INT: return int_types_scale[size-1];
      case GL_SHORT: return short_types_scale[size-1];
      case GL_BYTE: return byte_types_scale[size-1];
      case GL_UNSIGNED_INT: return uint_types_scale[size-1];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size-1];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size-1];
      case GL_FIXED: return fixed_types[size-1];
      default: assert(0); return 0;
      }
   }
   return PIPE_FORMAT_NONE; /* silence compiler warning */
}

/**
 * Examine the active arrays to determine if we have interleaved
 * vertex arrays all living in one VBO, or all living in user space.
 */
static GLboolean
is_interleaved_arrays(const struct st_vertex_program *vp,
                      const struct st_vp_variant *vpv,
                      const struct gl_client_array **arrays)
{
   GLuint attr;
   const struct gl_buffer_object *firstBufObj = NULL;
   GLint firstStride = -1;
   const GLubyte *firstPtr = NULL;
   GLboolean userSpaceBuffer = GL_FALSE;

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      const struct gl_buffer_object *bufObj = array->BufferObj;
      const GLsizei stride = array->StrideB; /* in bytes */

      if (attr == 0) {
         /* save info about the first array */
         firstStride = stride;
         firstPtr = array->Ptr;
         firstBufObj = bufObj;
         userSpaceBuffer = !bufObj || !bufObj->Name;
      }
      else {
         /* check if other arrays interleave with the first, in same buffer */
         if (stride != firstStride)
            return GL_FALSE; /* strides don't match */

         if (bufObj != firstBufObj)
            return GL_FALSE; /* arrays in different VBOs */

         if (abs(array->Ptr - firstPtr) > firstStride)
            return GL_FALSE; /* arrays start too far apart */

         if ((!_mesa_is_bufferobj(bufObj)) != userSpaceBuffer)
            return GL_FALSE; /* mix of VBO and user-space arrays */
      }
   }

   return GL_TRUE;
}

/**
 * Set up for drawing interleaved arrays that all live in one VBO
 * or all live in user space.
 * \param vbuffer  returns vertex buffer info
 * \param velements  returns vertex element info
 */
static boolean
setup_interleaved_attribs(const struct st_vertex_program *vp,
                          const struct st_vp_variant *vpv,
                          const struct gl_client_array **arrays,
                          struct pipe_vertex_buffer *vbuffer,
                          struct pipe_vertex_element velements[])
{
   GLuint attr;
   const GLubyte *low_addr = NULL;
   GLboolean usingVBO;      /* all arrays in a VBO? */
   struct gl_buffer_object *bufobj;
   GLsizei stride;

   /* Find the lowest address of the arrays we're drawing,
    * Init bufobj and stride.
    */
   if (vpv->num_inputs) {
      const GLuint mesaAttr0 = vp->index_to_input[0];
      const struct gl_client_array *array = arrays[mesaAttr0];

      /* Since we're doing interleaved arrays, we know there'll be at most
       * one buffer object and the stride will be the same for all arrays.
       * Grab them now.
       */
      bufobj = array->BufferObj;
      stride = array->StrideB;

      low_addr = arrays[vp->index_to_input[0]]->Ptr;

      for (attr = 1; attr < vpv->num_inputs; attr++) {
         const GLubyte *start = arrays[vp->index_to_input[attr]]->Ptr;
         low_addr = MIN2(low_addr, start);
      }
   }
   else {
      /* not sure we'll ever have zero inputs, but play it safe */
      bufobj = NULL;
      stride = 0;
      low_addr = 0;
   }

   /* are the arrays in user space? */
   usingVBO = _mesa_is_bufferobj(bufobj);

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      unsigned src_offset = (unsigned) (array->Ptr - low_addr);
      GLuint element_size = array->_ElementSize;

      assert(element_size == array->Size * _mesa_sizeof_type(array->Type));

      velements[attr].src_offset = src_offset;
      velements[attr].instance_divisor = array->InstanceDivisor;
      velements[attr].vertex_buffer_index = 0;
      velements[attr].src_format = st_pipe_vertex_format(array->Type,
                                                         array->Size,
                                                         array->Format,
                                                         array->Normalized,
                                                         array->Integer);
      assert(velements[attr].src_format);
   }

   /*
    * Return the vbuffer info and setup user-space attrib info, if needed.
    */
   if (vpv->num_inputs == 0) {
      /* just defensive coding here */
      vbuffer->buffer = NULL;
      vbuffer->user_buffer = NULL;
      vbuffer->buffer_offset = 0;
      vbuffer->stride = 0;
   }
   else if (usingVBO) {
      /* all interleaved arrays in a VBO */
      struct st_buffer_object *stobj = st_buffer_object(bufobj);

      if (!stobj || !stobj->buffer) {
         return FALSE; /* out-of-memory error probably */
      }

      vbuffer->buffer = stobj->buffer;
      vbuffer->user_buffer = NULL;
      vbuffer->buffer_offset = pointer_to_offset(low_addr);
      vbuffer->stride = stride;
   }
   else {
      /* all interleaved arrays in user memory */
      vbuffer->buffer = NULL;
      vbuffer->user_buffer = low_addr;
      vbuffer->buffer_offset = 0;
      vbuffer->stride = stride;
   }
   return TRUE;
}

/**
 * Set up a separate pipe_vertex_buffer and pipe_vertex_element for each
 * vertex attribute.
 * \param vbuffer  returns vertex buffer info
 * \param velements  returns vertex element info
 */
static boolean
setup_non_interleaved_attribs(struct st_context *st,
                              const struct st_vertex_program *vp,
                              const struct st_vp_variant *vpv,
                              const struct gl_client_array **arrays,
                              struct pipe_vertex_buffer vbuffer[],
                              struct pipe_vertex_element velements[])
{
   struct gl_context *ctx = st->ctx;
   GLuint attr;

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      struct gl_buffer_object *bufobj = array->BufferObj;
      GLsizei stride = array->StrideB;

      assert(array->_ElementSize == array->Size * _mesa_sizeof_type(array->Type));

      if (_mesa_is_bufferobj(bufobj)) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);

         if (!stobj || !stobj->buffer) {
            return FALSE; /* out-of-memory error probably */
         }

         vbuffer[attr].buffer = stobj->buffer;
         vbuffer[attr].user_buffer = NULL;
         vbuffer[attr].buffer_offset = pointer_to_offset(array->Ptr);
      }
      else {
         /* wrap user data */
         void *ptr;

         if (array->Ptr) {
            ptr = (void *) array->Ptr;
         }
         else {
            /* no array, use ctx->Current.Attrib[] value */
            ptr = (void *) ctx->Current.Attrib[mesaAttr];
            stride = 0;
         }

         assert(ptr);

         vbuffer[attr].buffer = NULL;
         vbuffer[attr].user_buffer = ptr;
         vbuffer[attr].buffer_offset = 0;
      }

      /* common-case setup */
      vbuffer[attr].stride = stride; /* in bytes */

      velements[attr].src_offset = 0;
      velements[attr].instance_divisor = array->InstanceDivisor;
      velements[attr].vertex_buffer_index = attr;
      velements[attr].src_format = st_pipe_vertex_format(array->Type,
                                                         array->Size,
                                                         array->Format,
                                                         array->Normalized,
                                                         array->Integer);
      assert(velements[attr].src_format);
   }
   return TRUE;
}

static void update_array(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;
   const struct gl_client_array **arrays = ctx->Array._DrawArrays;
   const struct st_vertex_program *vp;
   const struct st_vp_variant *vpv;
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
   unsigned num_vbuffers, num_velements;

   st->vertex_array_out_of_memory = FALSE;

   /* No drawing has been done yet, so do nothing. */
   if (!arrays)
      return;

   /* vertex program validation must be done before this */
   vp = st->vp;
   vpv = st->vp_variant;

   memset(velements, 0, sizeof(struct pipe_vertex_element) * vpv->num_inputs);

   /*
    * Setup the vbuffer[] and velements[] arrays.
    */
   if (is_interleaved_arrays(vp, vpv, arrays)) {
      if (!setup_interleaved_attribs(vp, vpv, arrays, vbuffer, velements)) {
         st->vertex_array_out_of_memory = TRUE;
         return;
      }

      num_vbuffers = 1;
      num_velements = vpv->num_inputs;
      if (num_velements == 0)
         num_vbuffers = 0;
   }
   else {
      if (!setup_non_interleaved_attribs(st, vp, vpv, arrays, vbuffer,
                                         velements)) {
         st->vertex_array_out_of_memory = TRUE;
         return;
      }

      num_vbuffers = vpv->num_inputs;
      num_velements = vpv->num_inputs;
   }

   cso_set_vertex_buffers(st->cso_context, num_vbuffers, vbuffer);
   cso_set_vertex_elements(st->cso_context, num_velements, velements);
}


const struct st_tracked_state st_update_array = {
   "st_update_array",					/* name */
   {							/* dirty */
      (_NEW_PROGRAM | _NEW_BUFFER_OBJECT),		/* mesa */
      ST_NEW_VERTEX_ARRAYS | ST_NEW_VERTEX_PROGRAM,     /* st */
   },
   update_array						/* update */
};
