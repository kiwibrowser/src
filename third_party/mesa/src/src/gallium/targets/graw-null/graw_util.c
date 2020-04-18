
#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "tgsi/tgsi_text.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "state_tracker/graw.h"


/* Helper functions.  These are the same for all graw implementations.
 */
PUBLIC void *
graw_parse_geometry_shader(struct pipe_context *pipe,
                           const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_gs_state(pipe, &state);
}

PUBLIC void *
graw_parse_vertex_shader(struct pipe_context *pipe,
                         const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_vs_state(pipe, &state);
}

PUBLIC void *
graw_parse_fragment_shader(struct pipe_context *pipe,
                           const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_fs_state(pipe, &state);
}

static char out_filename[256] = "";

PUBLIC boolean
graw_parse_args(int *argi,
                int argc,
                char *argv[])
{
   if (strcmp(argv[*argi], "-o") == 0) {
      if (*argi + 1 >= argc) {
         return FALSE;
      }

      strncpy(out_filename, argv[*argi + 1], sizeof(out_filename) - 1);
      out_filename[sizeof(out_filename) - 1] = '\0';
      *argi += 2;
      return TRUE;
   }

   return FALSE;
}

PUBLIC boolean
graw_save_surface_to_file(struct pipe_context *pipe,
                          struct pipe_surface *surface,
                          const char *filename)
{
   if (!filename || !*filename) {
      filename = out_filename;
      if (!filename || !*filename) {
         return FALSE;
      }
   }

   /* XXX: Make that working in release builds.
    */
   debug_dump_surface_bmp(pipe, filename, surface);
   return TRUE;
}
