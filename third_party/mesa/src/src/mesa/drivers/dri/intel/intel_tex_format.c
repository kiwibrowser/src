#include "intel_context.h"
#include "intel_tex.h"
#include "main/enums.h"
#include "main/formats.h"

/**
 * Returns the renderbuffer DataType for a MESA_FORMAT.
 */
GLenum
intel_mesa_format_to_rb_datatype(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_XRGB8888:
   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_R8:
   case MESA_FORMAT_GR88:
   case MESA_FORMAT_A8:
   case MESA_FORMAT_I8:
   case MESA_FORMAT_L8:
   case MESA_FORMAT_AL88:
   case MESA_FORMAT_RGB565:
   case MESA_FORMAT_ARGB1555:
   case MESA_FORMAT_ARGB4444:
   case MESA_FORMAT_S8:
      return GL_UNSIGNED_BYTE;
   case MESA_FORMAT_R16:
   case MESA_FORMAT_RG1616:
   case MESA_FORMAT_Z16:
      return GL_UNSIGNED_SHORT;
   case MESA_FORMAT_X8_Z24:
      return GL_UNSIGNED_INT;
   case MESA_FORMAT_S8_Z24:
      return GL_UNSIGNED_INT_24_8_EXT;
   case MESA_FORMAT_RGBA_FLOAT32:
   case MESA_FORMAT_RG_FLOAT32:
   case MESA_FORMAT_R_FLOAT32:
   case MESA_FORMAT_INTENSITY_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
   case MESA_FORMAT_ALPHA_FLOAT32:
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
      return GL_FLOAT;

      /* The core depthstencil wrappers demand this. */
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

   default:
      /* Unsupported format.  We may hit this when people ask for FBO-incomplete
       * formats.
       */
      return GL_NONE;
   }
}

int intel_compressed_num_bytes(GLuint mesaFormat)
{
   GLuint bw, bh;
   GLuint block_size;

   block_size = _mesa_get_format_bytes(mesaFormat);
   _mesa_get_format_block_size(mesaFormat, &bw, &bh);

   return block_size / bw;
}
