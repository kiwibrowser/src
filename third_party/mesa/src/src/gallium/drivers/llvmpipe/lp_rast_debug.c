#include "util/u_math.h"
#include "lp_rast_priv.h"
#include "lp_state_fs.h"

struct tile {
   int coverage;
   int overdraw;
   const struct lp_rast_state *state;
   char data[TILE_SIZE][TILE_SIZE];
};

static char get_label( int i )
{
   static const char *cmd_labels = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
   unsigned max_label = (2*26+10);

   if (i < max_label)
      return cmd_labels[i];
   else
      return '?';
}



static const char *cmd_names[LP_RAST_OP_MAX] = 
{
   "clear_color",
   "clear_zstencil",
   "triangle_1",
   "triangle_2",
   "triangle_3",
   "triangle_4",
   "triangle_5",
   "triangle_6",
   "triangle_7",
   "triangle_8",
   "triangle_3_4",
   "triangle_3_16",
   "triangle_4_16",
   "shade_tile",
   "shade_tile_opaque",
   "begin_query",
   "end_query",
   "set_state",
};

static const char *cmd_name(unsigned cmd)
{
   assert(Elements(cmd_names) > cmd);
   return cmd_names[cmd];
}

static const struct lp_fragment_shader_variant *
get_variant( const struct lp_rast_state *state,
             const struct cmd_block *block,
             int k )
{
   if (!state)
      return NULL;

   if (block->cmd[k] == LP_RAST_OP_SHADE_TILE ||
       block->cmd[k] == LP_RAST_OP_SHADE_TILE_OPAQUE ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_1 ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_2 ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_3 ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_4 ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_5 ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_6 ||
       block->cmd[k] == LP_RAST_OP_TRIANGLE_7)
      return state->variant;

   return NULL;
}


static boolean
is_blend( const struct lp_rast_state *state,
          const struct cmd_block *block,
          int k )
{
   const struct lp_fragment_shader_variant *variant = get_variant(state, block, k);

   if (variant)
      return  variant->key.blend.rt[0].blend_enable;

   return FALSE;
}



static void
debug_bin( const struct cmd_bin *bin )
{
   const struct lp_rast_state *state = NULL;
   const struct cmd_block *head = bin->head;
   int i, j = 0;

   debug_printf("bin %d,%d:\n", bin->x, bin->y);
                
   while (head) {
      for (i = 0; i < head->count; i++, j++) {
         if (head->cmd[i] == LP_RAST_OP_SET_STATE)
            state = head->arg[i].state;

         debug_printf("%d: %s %s\n", j,
                      cmd_name(head->cmd[i]),
                      is_blend(state, head, i) ? "blended" : "");
      }
      head = head->next;
   }
}


static void plot(struct tile *tile,
                 int x, int y,
                 char val,
                 boolean blend)
{
   if (tile->data[x][y] == ' ')
      tile->coverage++;
   else
      tile->overdraw++;

   tile->data[x][y] = val;
}






static int
debug_shade_tile(int x, int y,
                 const union lp_rast_cmd_arg arg,
                 struct tile *tile,
                 char val)
{
   const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   boolean blend;
   unsigned i,j;

   if (!tile->state)
      return 0;

   blend = tile->state->variant->key.blend.rt[0].blend_enable;

   if (inputs->disable)
      return 0;

   for (i = 0; i < TILE_SIZE; i++)
      for (j = 0; j < TILE_SIZE; j++)
         plot(tile, i, j, val, blend);

   return TILE_SIZE * TILE_SIZE;
}

static int
debug_clear_tile(int x, int y,
                 const union lp_rast_cmd_arg arg,
                 struct tile *tile,
                 char val)
{
   unsigned i,j;

   for (i = 0; i < TILE_SIZE; i++)
      for (j = 0; j < TILE_SIZE; j++)
         plot(tile, i, j, val, FALSE);

   return TILE_SIZE * TILE_SIZE;

}


static int
debug_triangle(int tilex, int tiley,
               const union lp_rast_cmd_arg arg,
               struct tile *tile,
               char val)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   unsigned plane_mask = arg.triangle.plane_mask;
   const struct lp_rast_plane *tri_plane = GET_PLANES(tri);
   struct lp_rast_plane plane[8];
   int x, y;
   int count = 0;
   unsigned i, nr_planes = 0;
   boolean blend = tile->state->variant->key.blend.rt[0].blend_enable;

   if (tri->inputs.disable) {
      /* This triangle was partially binned and has been disabled */
      return 0;
   }

   while (plane_mask) {
      plane[nr_planes] = tri_plane[u_bit_scan(&plane_mask)];
      plane[nr_planes].c = (plane[nr_planes].c +
                            plane[nr_planes].dcdy * tiley -
                            plane[nr_planes].dcdx * tilex);
      nr_planes++;
   }

   for(y = 0; y < TILE_SIZE; y++)
   {
      for(x = 0; x < TILE_SIZE; x++)
      {
         for (i = 0; i < nr_planes; i++)
            if (plane[i].c <= 0)
               goto out;
         
         plot(tile, x, y, val, blend);
         count++;

      out:
         for (i = 0; i < nr_planes; i++)
            plane[i].c -= plane[i].dcdx;
      }

      for (i = 0; i < nr_planes; i++) {
         plane[i].c += plane[i].dcdx * TILE_SIZE;
         plane[i].c += plane[i].dcdy;
      }
   }
   return count;
}





static void
do_debug_bin( struct tile *tile,
              const struct cmd_bin *bin,
              boolean print_cmds)
{
   unsigned k, j = 0;
   const struct cmd_block *block;

   int tx = bin->x * TILE_SIZE;
   int ty = bin->y * TILE_SIZE;

   memset(tile->data, ' ', sizeof tile->data);
   tile->coverage = 0;
   tile->overdraw = 0;
   tile->state = NULL;

   for (block = bin->head; block; block = block->next) {
      for (k = 0; k < block->count; k++, j++) {
         boolean blend = is_blend(tile->state, block, k);
         char val = get_label(j);
         int count = 0;
            
         if (print_cmds)
            debug_printf("%c: %15s", val, cmd_name(block->cmd[k]));

         if (block->cmd[k] == LP_RAST_OP_SET_STATE)
            tile->state = block->arg[k].state;
         
         if (block->cmd[k] == LP_RAST_OP_CLEAR_COLOR ||
             block->cmd[k] == LP_RAST_OP_CLEAR_ZSTENCIL)
            count = debug_clear_tile(tx, ty, block->arg[k], tile, val);

         if (block->cmd[k] == LP_RAST_OP_SHADE_TILE ||
             block->cmd[k] == LP_RAST_OP_SHADE_TILE_OPAQUE)
            count = debug_shade_tile(tx, ty, block->arg[k], tile, val);

         if (block->cmd[k] == LP_RAST_OP_TRIANGLE_1 ||
             block->cmd[k] == LP_RAST_OP_TRIANGLE_2 ||
             block->cmd[k] == LP_RAST_OP_TRIANGLE_3 ||
             block->cmd[k] == LP_RAST_OP_TRIANGLE_4 ||
             block->cmd[k] == LP_RAST_OP_TRIANGLE_5 ||
             block->cmd[k] == LP_RAST_OP_TRIANGLE_6 ||
             block->cmd[k] == LP_RAST_OP_TRIANGLE_7)
            count = debug_triangle(tx, ty, block->arg[k], tile, val);

         if (print_cmds) {
            debug_printf(" % 5d", count);

            if (blend)
               debug_printf(" blended");
            
            debug_printf("\n");
         }
      }
   }
}

void
lp_debug_bin( const struct cmd_bin *bin)
{
   struct tile tile;
   int x,y;

   if (bin->head) {
      do_debug_bin(&tile, bin, TRUE);

      debug_printf("------------------------------------------------------------------\n");
      for (y = 0; y < TILE_SIZE; y++) {
         for (x = 0; x < TILE_SIZE; x++) {
            debug_printf("%c", tile.data[y][x]);
         }
         debug_printf("|\n");
      }
      debug_printf("------------------------------------------------------------------\n");

      debug_printf("each pixel drawn avg %f times\n",
                   ((float)tile.overdraw + tile.coverage)/(float)tile.coverage);
   }
}






/** Return number of bytes used for a single bin */
static unsigned
lp_scene_bin_size( const struct lp_scene *scene, unsigned x, unsigned y )
{
   struct cmd_bin *bin = lp_scene_get_bin((struct lp_scene *) scene, x, y);
   const struct cmd_block *cmd;
   unsigned size = 0;
   for (cmd = bin->head; cmd; cmd = cmd->next) {
      size += (cmd->count *
               (sizeof(uint8_t) + sizeof(union lp_rast_cmd_arg)));
   }
   return size;
}



void
lp_debug_draw_bins_by_coverage( struct lp_scene *scene )
{
   unsigned x, y;
   unsigned total = 0;
   unsigned possible = 0;
   static unsigned long long _total;
   static unsigned long long _possible;

   for (x = 0; x < scene->tiles_x; x++)
      debug_printf("-");
   debug_printf("\n");

   for (y = 0; y < scene->tiles_y; y++) {
      for (x = 0; x < scene->tiles_x; x++) {
         struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
         const char *bits = "0123456789";
         struct tile tile;

         if (bin->head) {
            //lp_debug_bin(bin);

            do_debug_bin(&tile, bin, FALSE);

            total += tile.coverage;
            possible += 64*64;

            if (tile.coverage == 64*64)
               debug_printf("*");
            else if (tile.coverage) {
               int bit = tile.coverage/(64.0*64.0)*10;
               debug_printf("%c", bits[MIN2(bit,10)]);
            }
            else
               debug_printf("?");
         }
         else {
            debug_printf(" ");
         }
      }
      debug_printf("|\n");
   }

   for (x = 0; x < scene->tiles_x; x++)
      debug_printf("-");
   debug_printf("\n");

   debug_printf("this tile total: %u possible %u: percentage: %f\n",
                total,
                possible,
                total * 100.0 / (float)possible);

   _total += total;
   _possible += possible;

   debug_printf("overall   total: %llu possible %llu: percentage: %f\n",
                _total,
                _possible,
                _total * 100.0 / (double)_possible);
}


void
lp_debug_draw_bins_by_cmd_length( struct lp_scene *scene )
{
   unsigned x, y;

   for (y = 0; y < scene->tiles_y; y++) {
      for (x = 0; x < scene->tiles_x; x++) {
         const char *bits = " ...,-~:;=o+xaw*#XAWWWWWWWWWWWWWWWW";
         unsigned sz = lp_scene_bin_size(scene, x, y);
         unsigned sz2 = util_logbase2(sz);
         debug_printf("%c", bits[MIN2(sz2,32)]);
      }
      debug_printf("\n");
   }
}


void
lp_debug_bins( struct lp_scene *scene )
{
   unsigned x, y;

   for (y = 0; y < scene->tiles_y; y++) {
      for (x = 0; x < scene->tiles_x; x++) {
         struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
         if (bin->head) {
            debug_bin(bin);
         }
      }
   }
}
