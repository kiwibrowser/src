/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
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
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#ifndef BRWCONTEXT_INC
#define BRWCONTEXT_INC

#include "intel_context.h"
#include "brw_structs.h"
#include "main/imports.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Glossary:
 *
 * URB - uniform resource buffer.  A mid-sized buffer which is
 * partitioned between the fixed function units and used for passing
 * values (vertices, primitives, constants) between them.
 *
 * CURBE - constant URB entry.  An urb region (entry) used to hold
 * constant values which the fixed function units can be instructed to
 * preload into the GRF when spawning a thread.
 *
 * VUE - vertex URB entry.  An urb entry holding a vertex and usually
 * a vertex header.  The header contains control information and
 * things like primitive type, Begin/end flags and clip codes.  
 *
 * PUE - primitive URB entry.  An urb entry produced by the setup (SF)
 * unit holding rasterization and interpolation parameters.
 *
 * GRF - general register file.  One of several register files
 * addressable by programmed threads.  The inputs (r0, payload, curbe,
 * urb) of the thread are preloaded to this area before the thread is
 * spawned.  The registers are individually 8 dwords wide and suitable
 * for general usage.  Registers holding thread input values are not
 * special and may be overwritten.
 *
 * MRF - message register file.  Threads communicate (and terminate)
 * by sending messages.  Message parameters are placed in contiguous
 * MRF registers.  All program output is via these messages.  URB
 * entries are populated by sending a message to the shared URB
 * function containing the new data, together with a control word,
 * often an unmodified copy of R0.
 *
 * R0 - GRF register 0.  Typically holds control information used when
 * sending messages to other threads.
 *
 * EU or GEN4 EU: The name of the programmable subsystem of the
 * i965 hardware.  Threads are executed by the EU, the registers
 * described above are part of the EU architecture.
 *
 * Fixed function units:
 *
 * CS - Command streamer.  Notional first unit, little software
 * interaction.  Holds the URB entries used for constant data, ie the
 * CURBEs.
 *
 * VF/VS - Vertex Fetch / Vertex Shader.  The fixed function part of
 * this unit is responsible for pulling vertices out of vertex buffers
 * in vram and injecting them into the processing pipe as VUEs.  If
 * enabled, it first passes them to a VS thread which is a good place
 * for the driver to implement any active vertex shader.
 *
 * GS - Geometry Shader.  This corresponds to a new DX10 concept.  If
 * enabled, incoming strips etc are passed to GS threads in individual
 * line/triangle/point units.  The GS thread may perform arbitary
 * computation and emit whatever primtives with whatever vertices it
 * chooses.  This makes GS an excellent place to implement GL's
 * unfilled polygon modes, though of course it is capable of much
 * more.  Additionally, GS is used to translate away primitives not
 * handled by latter units, including Quads and Lineloops.
 *
 * CS - Clipper.  Mesa's clipping algorithms are imported to run on
 * this unit.  The fixed function part performs cliptesting against
 * the 6 fixed clipplanes and makes descisions on whether or not the
 * incoming primitive needs to be passed to a thread for clipping.
 * User clip planes are handled via cooperation with the VS thread.
 *
 * SF - Strips Fans or Setup: Triangles are prepared for
 * rasterization.  Interpolation coefficients are calculated.
 * Flatshading and two-side lighting usually performed here.
 *
 * WM - Windower.  Interpolation of vertex attributes performed here.
 * Fragment shader implemented here.  SIMD aspects of EU taken full
 * advantage of, as pixels are processed in blocks of 16.
 *
 * CC - Color Calculator.  No EU threads associated with this unit.
 * Handles blending and (presumably) depth and stencil testing.
 */


#define BRW_MAX_CURBE                    (32*16)

struct brw_context;
struct brw_instruction;
struct brw_vs_prog_key;
struct brw_wm_prog_key;
struct brw_wm_prog_data;

enum brw_state_id {
   BRW_STATE_URB_FENCE,
   BRW_STATE_FRAGMENT_PROGRAM,
   BRW_STATE_VERTEX_PROGRAM,
   BRW_STATE_INPUT_DIMENSIONS,
   BRW_STATE_CURBE_OFFSETS,
   BRW_STATE_REDUCED_PRIMITIVE,
   BRW_STATE_PRIMITIVE,
   BRW_STATE_CONTEXT,
   BRW_STATE_WM_INPUT_DIMENSIONS,
   BRW_STATE_PSP,
   BRW_STATE_SURFACES,
   BRW_STATE_VS_BINDING_TABLE,
   BRW_STATE_GS_BINDING_TABLE,
   BRW_STATE_PS_BINDING_TABLE,
   BRW_STATE_INDICES,
   BRW_STATE_VERTICES,
   BRW_STATE_BATCH,
   BRW_STATE_NR_WM_SURFACES,
   BRW_STATE_NR_VS_SURFACES,
   BRW_STATE_INDEX_BUFFER,
   BRW_STATE_VS_CONSTBUF,
   BRW_STATE_PROGRAM_CACHE,
   BRW_STATE_STATE_BASE_ADDRESS,
   BRW_STATE_SOL_INDICES,
};

#define BRW_NEW_URB_FENCE               (1 << BRW_STATE_URB_FENCE)
#define BRW_NEW_FRAGMENT_PROGRAM        (1 << BRW_STATE_FRAGMENT_PROGRAM)
#define BRW_NEW_VERTEX_PROGRAM          (1 << BRW_STATE_VERTEX_PROGRAM)
#define BRW_NEW_INPUT_DIMENSIONS        (1 << BRW_STATE_INPUT_DIMENSIONS)
#define BRW_NEW_CURBE_OFFSETS           (1 << BRW_STATE_CURBE_OFFSETS)
#define BRW_NEW_REDUCED_PRIMITIVE       (1 << BRW_STATE_REDUCED_PRIMITIVE)
#define BRW_NEW_PRIMITIVE               (1 << BRW_STATE_PRIMITIVE)
#define BRW_NEW_CONTEXT                 (1 << BRW_STATE_CONTEXT)
#define BRW_NEW_WM_INPUT_DIMENSIONS     (1 << BRW_STATE_WM_INPUT_DIMENSIONS)
#define BRW_NEW_PSP                     (1 << BRW_STATE_PSP)
#define BRW_NEW_SURFACES		(1 << BRW_STATE_SURFACES)
#define BRW_NEW_VS_BINDING_TABLE	(1 << BRW_STATE_VS_BINDING_TABLE)
#define BRW_NEW_GS_BINDING_TABLE	(1 << BRW_STATE_GS_BINDING_TABLE)
#define BRW_NEW_PS_BINDING_TABLE	(1 << BRW_STATE_PS_BINDING_TABLE)
#define BRW_NEW_INDICES			(1 << BRW_STATE_INDICES)
#define BRW_NEW_VERTICES		(1 << BRW_STATE_VERTICES)
/**
 * Used for any batch entry with a relocated pointer that will be used
 * by any 3D rendering.
 */
#define BRW_NEW_BATCH                  (1 << BRW_STATE_BATCH)
/** \see brw.state.depth_region */
#define BRW_NEW_INDEX_BUFFER           (1 << BRW_STATE_INDEX_BUFFER)
#define BRW_NEW_VS_CONSTBUF            (1 << BRW_STATE_VS_CONSTBUF)
#define BRW_NEW_PROGRAM_CACHE		(1 << BRW_STATE_PROGRAM_CACHE)
#define BRW_NEW_STATE_BASE_ADDRESS	(1 << BRW_STATE_STATE_BASE_ADDRESS)
#define BRW_NEW_SOL_INDICES		(1 << BRW_STATE_SOL_INDICES)

struct brw_state_flags {
   /** State update flags signalled by mesa internals */
   GLuint mesa;
   /**
    * State update flags signalled as the result of brw_tracked_state updates
    */
   GLuint brw;
   /** State update flags signalled by brw_state_cache.c searches */
   GLuint cache;
};

#define AUB_TRACE_TYPE_MASK		0x0000ff00
#define AUB_TRACE_TYPE_NOTYPE		(0 << 8)
#define AUB_TRACE_TYPE_BATCH		(1 << 8)
#define AUB_TRACE_TYPE_VERTEX_BUFFER	(5 << 8)
#define AUB_TRACE_TYPE_2D_MAP		(6 << 8)
#define AUB_TRACE_TYPE_CUBE_MAP		(7 << 8)
#define AUB_TRACE_TYPE_VOLUME_MAP	(9 << 8)
#define AUB_TRACE_TYPE_1D_MAP		(10 << 8)
#define AUB_TRACE_TYPE_CONSTANT_BUFFER	(11 << 8)
#define AUB_TRACE_TYPE_CONSTANT_URB	(12 << 8)
#define AUB_TRACE_TYPE_INDEX_BUFFER	(13 << 8)
#define AUB_TRACE_TYPE_GENERAL		(14 << 8)
#define AUB_TRACE_TYPE_SURFACE		(15 << 8)

/**
 * state_struct_type enum values are encoded with the top 16 bits representing
 * the type to be delivered to the .aub file, and the bottom 16 bits
 * representing the subtype.  This macro performs the encoding.
 */
#define ENCODE_SS_TYPE(type, subtype) (((type) << 16) | (subtype))

enum state_struct_type {
   AUB_TRACE_VS_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 1),
   AUB_TRACE_GS_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 2),
   AUB_TRACE_CLIP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 3),
   AUB_TRACE_SF_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 4),
   AUB_TRACE_WM_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 5),
   AUB_TRACE_CC_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 6),
   AUB_TRACE_CLIP_VP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 7),
   AUB_TRACE_SF_VP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 8),
   AUB_TRACE_CC_VP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x9),
   AUB_TRACE_SAMPLER_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xa),
   AUB_TRACE_KERNEL_INSTRUCTIONS =	ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xb),
   AUB_TRACE_SCRATCH_SPACE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xc),
   AUB_TRACE_SAMPLER_DEFAULT_COLOR =    ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xd),

   AUB_TRACE_SCISSOR_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x15),
   AUB_TRACE_BLEND_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x16),
   AUB_TRACE_DEPTH_STENCIL_STATE =	ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x17),

   AUB_TRACE_VERTEX_BUFFER =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_VERTEX_BUFFER, 0),
   AUB_TRACE_BINDING_TABLE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_SURFACE, 0x100),
   AUB_TRACE_SURFACE_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_SURFACE, 0x200),
   AUB_TRACE_VS_CONSTANTS =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_CONSTANT_BUFFER, 0),
   AUB_TRACE_WM_CONSTANTS =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_CONSTANT_BUFFER, 1),
};

/**
 * Decode a state_struct_type value to determine the type that should be
 * stored in the .aub file.
 */
static inline uint32_t AUB_TRACE_TYPE(enum state_struct_type ss_type)
{
   return (ss_type & 0xFFFF0000) >> 16;
}

/**
 * Decode a state_struct_type value to determine the subtype that should be
 * stored in the .aub file.
 */
static inline uint32_t AUB_TRACE_SUBTYPE(enum state_struct_type ss_type)
{
   return ss_type & 0xFFFF;
}

/** Subclass of Mesa vertex program */
struct brw_vertex_program {
   struct gl_vertex_program program;
   GLuint id;
   bool use_const_buffer;
};


/** Subclass of Mesa fragment program */
struct brw_fragment_program {
   struct gl_fragment_program program;
   GLuint id;  /**< serial no. to identify frag progs, never re-used */
};

struct brw_shader {
   struct gl_shader base;

   bool compiled_once;

   /** Shader IR transformed for native compile, at link time. */
   struct exec_list *ir;
};

struct brw_shader_program {
   struct gl_shader_program base;
};

/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs:
 */
struct brw_wm_prog_data {
   GLuint curb_read_length;
   GLuint urb_read_length;

   GLuint first_curbe_grf;
   GLuint first_curbe_grf_16;
   GLuint reg_blocks;
   GLuint reg_blocks_16;
   GLuint total_scratch;

   GLuint nr_params;       /**< number of float params/constants */
   GLuint nr_pull_params;
   bool error;
   bool dual_src_blend;
   int dispatch_width;
   uint32_t prog_offset_16;

   /**
    * Mask of which interpolation modes are required by the fragment shader.
    * Used in hardware setup on gen6+.
    */
   uint32_t barycentric_interp_modes;

   /* Pointer to tracked values (only valid once
    * _mesa_load_state_parameters has been called at runtime).
    */
   const float *param[MAX_UNIFORMS * 4]; /* should be: BRW_MAX_CURBE */
   const float *pull_param[MAX_UNIFORMS * 4];
};

/**
 * Enum representing the i965-specific vertex results that don't correspond
 * exactly to any element of gl_vert_result.  The values of this enum are
 * assigned such that they don't conflict with gl_vert_result.
 */
typedef enum
{
   BRW_VERT_RESULT_NDC = VERT_RESULT_MAX,
   BRW_VERT_RESULT_HPOS_DUPLICATE,
   BRW_VERT_RESULT_PAD,
   /*
    * It's actually not a vert_result but just a _mark_ to let sf aware that
    * he need do something special to handle gl_PointCoord builtin variable
    * correctly. see compile_sf_prog() for more info.
    */
   BRW_VERT_RESULT_PNTC,
   BRW_VERT_RESULT_MAX
} brw_vert_result;


/**
 * Data structure recording the relationship between the gl_vert_result enum
 * and "slots" within the vertex URB entry (VUE).  A "slot" is defined as a
 * single octaword within the VUE (128 bits).
 *
 * Note that each BRW register contains 256 bits (2 octawords), so when
 * accessing the VUE in URB_NOSWIZZLE mode, each register corresponds to two
 * consecutive VUE slots.  When accessing the VUE in URB_INTERLEAVED mode (as
 * in a vertex shader), each register corresponds to a single VUE slot, since
 * it contains data for two separate vertices.
 */
struct brw_vue_map {
   /**
    * Map from gl_vert_result value to VUE slot.  For gl_vert_results that are
    * not stored in a slot (because they are not written, or because
    * additional processing is applied before storing them in the VUE), the
    * value is -1.
    */
   int vert_result_to_slot[BRW_VERT_RESULT_MAX];

   /**
    * Map from VUE slot to gl_vert_result value.  For slots that do not
    * directly correspond to a gl_vert_result, the value comes from
    * brw_vert_result.
    *
    * For slots that are not in use, the value is BRW_VERT_RESULT_MAX (this
    * simplifies code that uses the value stored in slot_to_vert_result to
    * create a bit mask).
    */
   int slot_to_vert_result[BRW_VERT_RESULT_MAX];

   /**
    * Total number of VUE slots in use
    */
   int num_slots;
};

/**
 * Convert a VUE slot number into a byte offset within the VUE.
 */
static inline GLuint brw_vue_slot_to_offset(GLuint slot)
{
   return 16*slot;
}

/**
 * Convert a vert_result into a byte offset within the VUE.
 */
static inline GLuint brw_vert_result_to_offset(struct brw_vue_map *vue_map,
                                               GLuint vert_result)
{
   return brw_vue_slot_to_offset(vue_map->vert_result_to_slot[vert_result]);
}


struct brw_sf_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;

   /* Each vertex may have upto 12 attributes, 4 components each,
    * except WPOS which requires only 2.  (11*4 + 2) == 44 ==> 11
    * rows.
    *
    * Actually we use 4 for each, so call it 12 rows.
    */
   GLuint urb_entry_size;
};

struct brw_clip_prog_data {
   GLuint curb_read_length;	/* user planes? */
   GLuint clip_mode;
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_gs_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;

   /**
    * Gen6 transform feedback: Amount by which the streaming vertex buffer
    * indices should be incremented each time the GS is invoked.
    */
   unsigned svbi_postincrement_value;
};

struct brw_vs_prog_data {
   struct brw_vue_map vue_map;

   GLuint curb_read_length;
   GLuint urb_read_length;
   GLuint total_grf;
   GLbitfield64 outputs_written;
   GLuint nr_params;       /**< number of float params/constants */
   GLuint nr_pull_params; /**< number of dwords referenced by pull_param[] */
   GLuint total_scratch;

   GLbitfield64 inputs_read;

   /* Used for calculating urb partitions:
    */
   GLuint urb_entry_size;

   const float *param[MAX_UNIFORMS * 4]; /* should be: BRW_MAX_CURBE */
   const float *pull_param[MAX_UNIFORMS * 4];

   bool uses_new_param_layout;
   bool uses_vertexid;
   bool userclip;

   int num_surfaces;
};


/* Size == 0 if output either not written, or always [0,0,0,1]
 */
struct brw_vs_ouput_sizes {
   GLubyte output_size[VERT_RESULT_MAX];
};


/** Number of texture sampler units */
#define BRW_MAX_TEX_UNIT 16

/** Max number of render targets in a shader */
#define BRW_MAX_DRAW_BUFFERS 8

/**
 * Max number of binding table entries used for stream output.
 *
 * From the OpenGL 3.0 spec, table 6.44 (Transform Feedback State), the
 * minimum value of MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS is 64.
 *
 * On Gen6, the size of transform feedback data is limited not by the number
 * of components but by the number of binding table entries we set aside.  We
 * use one binding table entry for a float, one entry for a vector, and one
 * entry per matrix column.  Since the only way we can communicate our
 * transform feedback capabilities to the client is via
 * MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, we need to plan for the
 * worst case, in which all the varyings are floats, so we use up one binding
 * table entry per component.  Therefore we need to set aside at least 64
 * binding table entries for use by transform feedback.
 *
 * Note: since we don't currently pack varyings, it is currently impossible
 * for the client to actually use up all of these binding table entries--if
 * all of their varyings were floats, they would run out of varying slots and
 * fail to link.  But that's a bug, so it seems prudent to go ahead and
 * allocate the number of binding table entries we will need once the bug is
 * fixed.
 */
#define BRW_MAX_SOL_BINDINGS 64

/** Maximum number of actual buffers used for stream output */
#define BRW_MAX_SOL_BUFFERS 4

#define BRW_MAX_WM_UBOS              12
#define BRW_MAX_VS_UBOS              12

/**
 * Helpers to create Surface Binding Table indexes for draw buffers,
 * textures, and constant buffers.
 *
 * Shader threads access surfaces via numeric handles, rather than directly
 * using pointers.  The binding table maps these numeric handles to the
 * address of the actual buffer.
 *
 * For example, a shader might ask to sample from "surface 7."  In this case,
 * bind[7] would contain a pointer to a texture.
 *
 * Currently, our WM binding tables are (arbitrarily) programmed as follows:
 *
 *    +-------------------------------+
 *    |   0 | Draw buffer 0           |
 *    |   . |     .                   |
 *    |   : |     :                   |
 *    |   7 | Draw buffer 7           |
 *    |-----|-------------------------|
 *    |   8 | WM Pull Constant Buffer |
 *    |-----|-------------------------|
 *    |   9 | Texture 0               |
 *    |   . |     .                   |
 *    |   : |     :                   |
 *    |  24 | Texture 15              |
 *    |-----|-------------------------|
 *    |  25 | UBO 0                   |
 *    |   . |     .                   |
 *    |   : |     :                   |
 *    |  36 | UBO 11                  |
 *    +-------------------------------+
 *
 * Our VS binding tables are programmed as follows:
 *
 *    +-----+-------------------------+
 *    |   0 | VS Pull Constant Buffer |
 *    +-----+-------------------------+
 *    |   1 | Texture 0               |
 *    |   . |     .                   |
 *    |   : |     :                   |
 *    |  16 | Texture 15              |
 *    +-----+-------------------------+
 *    |  17 | UBO 0                   |
 *    |   . |     .                   |
 *    |   : |     :                   |
 *    |  28 | UBO 11                  |
 *    +-------------------------------+
 *
 * Our (gen6) GS binding tables are programmed as follows:
 *
 *    +-----+-------------------------+
 *    |   0 | SOL Binding 0           |
 *    |   . |     .                   |
 *    |   : |     :                   |
 *    |  63 | SOL Binding 63          |
 *    +-----+-------------------------+
 *
 * Note that nothing actually uses the SURF_INDEX_DRAW macro, so it has to be
 * the identity function or things will break.  We do want to keep draw buffers
 * first so we can use headerless render target writes for RT 0.
 */
#define SURF_INDEX_DRAW(d)           (d)
#define SURF_INDEX_FRAG_CONST_BUFFER (BRW_MAX_DRAW_BUFFERS + 1)
#define SURF_INDEX_TEXTURE(t)        (BRW_MAX_DRAW_BUFFERS + 2 + (t))
#define SURF_INDEX_WM_UBO(u)         (SURF_INDEX_TEXTURE(BRW_MAX_TEX_UNIT) + u)

/** Maximum size of the binding table. */
#define BRW_MAX_WM_SURFACES          SURF_INDEX_WM_UBO(BRW_MAX_WM_UBOS)

#define SURF_INDEX_VERT_CONST_BUFFER (0)
#define SURF_INDEX_VS_TEXTURE(t)     (SURF_INDEX_VERT_CONST_BUFFER + 1 + (t))
#define SURF_INDEX_VS_UBO(u)         (SURF_INDEX_VS_TEXTURE(BRW_MAX_TEX_UNIT) + u)
#define BRW_MAX_VS_SURFACES          SURF_INDEX_VS_UBO(BRW_MAX_VS_UBOS)

#define SURF_INDEX_SOL_BINDING(t)    ((t))
#define BRW_MAX_GS_SURFACES          SURF_INDEX_SOL_BINDING(BRW_MAX_SOL_BINDINGS)

enum brw_cache_id {
   BRW_BLEND_STATE,
   BRW_DEPTH_STENCIL_STATE,
   BRW_COLOR_CALC_STATE,
   BRW_CC_VP,
   BRW_CC_UNIT,
   BRW_WM_PROG,
   BRW_BLORP_BLIT_PROG,
   BRW_SAMPLER,
   BRW_WM_UNIT,
   BRW_SF_PROG,
   BRW_SF_VP,
   BRW_SF_UNIT, /* scissor state on gen6 */
   BRW_VS_UNIT,
   BRW_VS_PROG,
   BRW_GS_UNIT,
   BRW_GS_PROG,
   BRW_CLIP_VP,
   BRW_CLIP_UNIT,
   BRW_CLIP_PROG,

   BRW_MAX_CACHE
};

struct brw_cache_item {
   /**
    * Effectively part of the key, cache_id identifies what kind of state
    * buffer is involved, and also which brw->state.dirty.cache flag should
    * be set when this cache item is chosen.
    */
   enum brw_cache_id cache_id;
   /** 32-bit hash of the key data */
   GLuint hash;
   GLuint key_size;		/* for variable-sized keys */
   GLuint aux_size;
   const void *key;

   uint32_t offset;
   uint32_t size;

   struct brw_cache_item *next;
};   



struct brw_cache {
   struct brw_context *brw;

   struct brw_cache_item **items;
   drm_intel_bo *bo;
   GLuint size, n_items;

   uint32_t next_offset;
   bool bo_used_by_gpu;
};


/* Considered adding a member to this struct to document which flags
 * an update might raise so that ordering of the state atoms can be
 * checked or derived at runtime.  Dropped the idea in favor of having
 * a debug mode where the state is monitored for flags which are
 * raised that have already been tested against.
 */
struct brw_tracked_state {
   struct brw_state_flags dirty;
   void (*emit)( struct brw_context *brw );
};

/* Flags for brw->state.cache.
 */
#define CACHE_NEW_BLEND_STATE            (1<<BRW_BLEND_STATE)
#define CACHE_NEW_DEPTH_STENCIL_STATE    (1<<BRW_DEPTH_STENCIL_STATE)
#define CACHE_NEW_COLOR_CALC_STATE       (1<<BRW_COLOR_CALC_STATE)
#define CACHE_NEW_CC_VP                  (1<<BRW_CC_VP)
#define CACHE_NEW_CC_UNIT                (1<<BRW_CC_UNIT)
#define CACHE_NEW_WM_PROG                (1<<BRW_WM_PROG)
#define CACHE_NEW_SAMPLER                (1<<BRW_SAMPLER)
#define CACHE_NEW_WM_UNIT                (1<<BRW_WM_UNIT)
#define CACHE_NEW_SF_PROG                (1<<BRW_SF_PROG)
#define CACHE_NEW_SF_VP                  (1<<BRW_SF_VP)
#define CACHE_NEW_SF_UNIT                (1<<BRW_SF_UNIT)
#define CACHE_NEW_VS_UNIT                (1<<BRW_VS_UNIT)
#define CACHE_NEW_VS_PROG                (1<<BRW_VS_PROG)
#define CACHE_NEW_GS_UNIT                (1<<BRW_GS_UNIT)
#define CACHE_NEW_GS_PROG                (1<<BRW_GS_PROG)
#define CACHE_NEW_CLIP_VP                (1<<BRW_CLIP_VP)
#define CACHE_NEW_CLIP_UNIT              (1<<BRW_CLIP_UNIT)
#define CACHE_NEW_CLIP_PROG              (1<<BRW_CLIP_PROG)

struct brw_cached_batch_item {
   struct header *header;
   GLuint sz;
   struct brw_cached_batch_item *next;
};
   


/* Protect against a future where VERT_ATTRIB_MAX > 32.  Wouldn't life
 * be easier if C allowed arrays of packed elements?
 */
#define ATTRIB_BIT_DWORDS  ((VERT_ATTRIB_MAX+31)/32)

struct brw_vertex_buffer {
   /** Buffer object containing the uploaded vertex data */
   drm_intel_bo *bo;
   uint32_t offset;
   /** Byte stride between elements in the uploaded array */
   GLuint stride;
   GLuint step_rate;
};
struct brw_vertex_element {
   const struct gl_client_array *glarray;

   int buffer;

   /** The corresponding Mesa vertex attribute */
   gl_vert_attrib attrib;
   /** Size of a complete element */
   GLuint element_size;
   /** Offset of the first element within the buffer object */
   unsigned int offset;
};



struct brw_vertex_info {
   GLuint sizes[ATTRIB_BIT_DWORDS * 2]; /* sizes:2[VERT_ATTRIB_MAX] */
};

struct brw_query_object {
   struct gl_query_object Base;

   /** Last query BO associated with this query. */
   drm_intel_bo *bo;
   /** First index in bo with query data for this object. */
   int first_index;
   /** Last index in bo with query data for this object. */
   int last_index;
};


/**
 * brw_context is derived from intel_context.
 */
struct brw_context 
{
   struct intel_context intel;  /**< base class, must be first field */
   GLuint primitive; /**< Hardware primitive, such as _3DPRIM_TRILIST. */

   bool emit_state_always;
   bool has_surface_tile_offset;
   bool has_compr4;
   bool has_negative_rhw_bug;
   bool has_aa_line_parameters;
   bool has_pln;
   bool precompile;

   /**
    * Some versions of Gen hardware don't do centroid interpolation correctly
    * on unlit pixels, causing incorrect values for derivatives near triangle
    * edges.  Enabling this flag causes the fragment shader to use
    * non-centroid interpolation for unlit pixels, at the expense of two extra
    * fragment shader instructions.
    */
   bool needs_unlit_centroid_workaround;

   struct {
      struct brw_state_flags dirty;
   } state;

   struct brw_cache cache;
   struct brw_cached_batch_item *cached_batch_items;

   struct {
      struct brw_vertex_element inputs[VERT_ATTRIB_MAX];
      struct brw_vertex_buffer buffers[VERT_ATTRIB_MAX];
      struct {
	      uint32_t handle;
	      uint32_t offset;
	      uint32_t stride;
	      uint32_t step_rate;
      } current_buffers[VERT_ATTRIB_MAX];

      struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
      GLuint nr_enabled;
      GLuint nr_buffers, nr_current_buffers;

      /* Summary of size and varying of active arrays, so we can check
       * for changes to this state:
       */
      struct brw_vertex_info info;
      unsigned int min_index, max_index;

      /* Offset from start of vertex buffer so we can avoid redefining
       * the same VB packed over and over again.
       */
      unsigned int start_vertex_bias;
   } vb;

   struct {
      /**
       * Index buffer for this draw_prims call.
       *
       * Updates are signaled by BRW_NEW_INDICES.
       */
      const struct _mesa_index_buffer *ib;

      /* Updates are signaled by BRW_NEW_INDEX_BUFFER. */
      drm_intel_bo *bo;
      GLuint type;

      /* Offset to index buffer index to use in CMD_3D_PRIM so that we can
       * avoid re-uploading the IB packet over and over if we're actually
       * referencing the same index buffer.
       */
      unsigned int start_vertex_offset;
   } ib;

   /* Active vertex program: 
    */
   const struct gl_vertex_program *vertex_program;
   const struct gl_fragment_program *fragment_program;

   /* hw-dependent 3DSTATE_VF_STATISTICS opcode */
   uint32_t CMD_VF_STATISTICS;
   /* hw-dependent 3DSTATE_PIPELINE_SELECT opcode */
   uint32_t CMD_PIPELINE_SELECT;

   /**
    * Platform specific constants containing the maximum number of threads
    * for each pipeline stage.
    */
   int max_vs_threads;
   int max_gs_threads;
   int max_wm_threads;

   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      GLuint vsize;		/* vertex size plus header in urb registers */
      GLuint csize;		/* constant buffer size in urb registers */
      GLuint sfsize;		/* setup data size in urb registers */

      bool constrained;

      GLuint max_vs_entries;	/* Maximum number of VS entries */
      GLuint max_gs_entries;	/* Maximum number of GS entries */

      GLuint nr_vs_entries;
      GLuint nr_gs_entries;
      GLuint nr_clip_entries;
      GLuint nr_sf_entries;
      GLuint nr_cs_entries;

      /* gen6:
       * The length of each URB entry owned by the VS (or GS), as
       * a number of 1024-bit (128-byte) rows.  Should be >= 1.
       *
       * gen7: Same meaning, but in 512-bit (64-byte) rows.
       */
      GLuint vs_size;
      GLuint gs_size;

      GLuint vs_start;
      GLuint gs_start;
      GLuint clip_start;
      GLuint sf_start;
      GLuint cs_start;
      GLuint size; /* Hardware URB size, in KB. */

      /* gen6: True if the most recently sent _3DSTATE_URB message allocated
       * URB space for the GS.
       */
      bool gen6_gs_previously_active;
   } urb;

   
   /* BRW_NEW_CURBE_OFFSETS: 
    */
   struct {
      GLuint wm_start;  /**< pos of first wm const in CURBE buffer */
      GLuint wm_size;   /**< number of float[4] consts, multiple of 16 */
      GLuint clip_start;
      GLuint clip_size;
      GLuint vs_start;
      GLuint vs_size;
      GLuint total_size;

      drm_intel_bo *curbe_bo;
      /** Offset within curbe_bo of space for current curbe entry */
      GLuint curbe_offset;
      /** Offset within curbe_bo of space for next curbe entry */
      GLuint curbe_next_offset;

      /**
       * Copy of the last set of CURBEs uploaded.  Frequently we'll end up
       * in brw_curbe.c with the same set of constant data to be uploaded,
       * so we'd rather not upload new constants in that case (it can cause
       * a pipeline bubble since only up to 4 can be pipelined at a time).
       */
      GLfloat *last_buf;
      /**
       * Allocation for where to calculate the next set of CURBEs.
       * It's a hot enough path that malloc/free of that data matters.
       */
      GLfloat *next_buf;
      GLuint last_bufsz;
   } curbe;

   /** SAMPLER_STATE count and offset */
   struct {
      GLuint count;
      uint32_t offset;
   } sampler;

   struct {
      struct brw_vs_prog_data *prog_data;
      int8_t *constant_map; /* variable array following prog_data */

      drm_intel_bo *scratch_bo;
      drm_intel_bo *const_bo;
      /** Offset in the program cache to the VS program */
      uint32_t prog_offset;
      uint32_t state_offset;

      uint32_t push_const_offset; /* Offset in the batchbuffer */
      int push_const_size; /* in 256-bit register increments */

      /** @{ register allocator */

      struct ra_regs *regs;

      /**
       * Array of the ra classes for the unaligned contiguous register
       * block sizes used.
       */
      int *classes;

      /**
       * Mapping for register-allocated objects in *regs to the first
       * GRF for that object.
      */
      uint8_t *ra_reg_to_grf;
      /** @} */

      uint32_t bind_bo_offset;
      uint32_t surf_offset[BRW_MAX_VS_SURFACES];
   } vs;

   struct {
      struct brw_gs_prog_data *prog_data;

      bool prog_active;
      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;
      uint32_t state_offset;

      uint32_t bind_bo_offset;
      uint32_t surf_offset[BRW_MAX_GS_SURFACES];
   } gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;

      /* Offset in the batch to the CLIP state on pre-gen6. */
      uint32_t state_offset;

      /* As of gen6, this is the offset in the batch to the CLIP VP,
       * instead of vp_bo.
       */
      uint32_t vp_offset;
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;
      uint32_t state_offset;
      uint32_t vp_offset;
   } sf;

   struct {
      struct brw_wm_prog_data *prog_data;
      struct brw_wm_compile *compile_data;

      /** Input sizes, calculated from active vertex program.
       * One bit per fragment program input attribute.
       */
      GLbitfield input_size_masks[4];

      /** offsets in the batch to sampler default colors (texture border color)
       */
      uint32_t sdc_offset[BRW_MAX_TEX_UNIT];

      GLuint render_surf;

      drm_intel_bo *scratch_bo;

      /**
       * Buffer object used in place of multisampled null render targets on
       * Gen6.  See brw_update_null_renderbuffer_surface().
       */
      drm_intel_bo *multisampled_null_render_target_bo;

      /** Offset in the program cache to the WM program */
      uint32_t prog_offset;

      uint32_t state_offset; /* offset in batchbuffer to pre-gen6 WM state */

      drm_intel_bo *const_bo; /* pull constant buffer. */
      /**
       * This is offset in the batch to the push constants on gen6.
       *
       * Pre-gen6, push constants live in the CURBE.
       */
      uint32_t push_const_offset;

      /** Binding table of pointers to surf_bo entries */
      uint32_t bind_bo_offset;
      uint32_t surf_offset[BRW_MAX_WM_SURFACES];

      /** @{ register allocator */

      struct ra_regs *regs;

      /** Array of the ra classes for the unaligned contiguous
       * register block sizes used.
       */
      int *classes;

      /**
       * Mapping for register-allocated objects in *regs to the first
       * GRF for that object.
      */
      uint8_t *ra_reg_to_grf;

      /**
       * ra class for the aligned pairs we use for PLN, which doesn't
       * appear in *classes.
       */
      int aligned_pairs_class;

      /** @} */
   } wm;


   struct {
      uint32_t state_offset;
      uint32_t blend_state_offset;
      uint32_t depth_stencil_state_offset;
      uint32_t vp_offset;
   } cc;

   struct {
      struct brw_query_object *obj;
      drm_intel_bo *bo;
      int index;
      bool active;
   } query;
   /* Used to give every program string a unique id
    */
   GLuint program_id;

   int num_atoms;
   const struct brw_tracked_state **atoms;

   /* If (INTEL_DEBUG & DEBUG_BATCH) */
   struct {
      uint32_t offset;
      uint32_t size;
      enum state_struct_type type;
   } *state_batch_list;
   int state_batch_count;

   struct brw_sol_state {
      uint32_t svbi_0_starting_index;
      uint32_t svbi_0_max_index;
      uint32_t offset_0_batch_start;
      uint32_t primitives_generated;
      uint32_t primitives_written;
      bool counting_primitives_generated;
      bool counting_primitives_written;
   } sol;

   uint32_t render_target_format[MESA_FORMAT_COUNT];
   bool format_supported_as_render_target[MESA_FORMAT_COUNT];

   /* PrimitiveRestart */
   struct {
      bool in_progress;
      bool enable_cut_index;
   } prim_restart;

   uint32_t num_instances;
};



#define BRW_PACKCOLOR8888(r,g,b,a)  ((r<<24) | (g<<16) | (b<<8) | a)

struct brw_instruction_info {
    char    *name;
    int	    nsrc;
    int	    ndst;
    bool is_arith;
};
extern const struct brw_instruction_info brw_opcodes[128];

/*======================================================================
 * brw_vtbl.c
 */
void brwInitVtbl( struct brw_context *brw );

/*======================================================================
 * brw_context.c
 */
bool brwCreateContext(int api,
		      const struct gl_config *mesaVis,
		      __DRIcontext *driContextPriv,
                      unsigned major_version,
                      unsigned minor_version,
                      uint32_t flags,
                      unsigned *error,
		      void *sharedContextPrivate);

/*======================================================================
 * brw_queryobj.c
 */
void brw_init_queryobj_functions(struct dd_function_table *functions);
void brw_prepare_query_begin(struct brw_context *brw);
void brw_emit_query_begin(struct brw_context *brw);
void brw_emit_query_end(struct brw_context *brw);

/*======================================================================
 * brw_state_dump.c
 */
void brw_debug_batch(struct intel_context *intel);
void brw_annotate_aub(struct intel_context *intel);

/*======================================================================
 * brw_tex.c
 */
void brw_validate_textures( struct brw_context *brw );


/*======================================================================
 * brw_program.c
 */
void brwInitFragProgFuncs( struct dd_function_table *functions );

int brw_get_scratch_size(int size);
void brw_get_scratch_bo(struct intel_context *intel,
			drm_intel_bo **scratch_bo, int size);


/* brw_urb.c
 */
void brw_upload_urb_fence(struct brw_context *brw);

/* brw_curbe.c
 */
void brw_upload_cs_urb_state(struct brw_context *brw);

/* brw_disasm.c */
int brw_disasm (FILE *file, struct brw_instruction *inst, int gen);

/* brw_vs.c */
gl_clip_plane *brw_select_clip_planes(struct gl_context *ctx);

/* brw_wm_surface_state.c */
void brw_init_surface_formats(struct brw_context *brw);
void
brw_update_sol_surface(struct brw_context *brw,
                       struct gl_buffer_object *buffer_obj,
                       uint32_t *out_offset, unsigned num_vector_components,
                       unsigned stride_dwords, unsigned offset_dwords);
void brw_upload_ubo_surfaces(struct brw_context *brw,
			     struct gl_shader *shader,
			     uint32_t *surf_offsets);

/* gen6_sol.c */
void
brw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
			     struct gl_transform_feedback_object *obj);
void
brw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj);

/* gen7_sol_state.c */
void
gen7_end_transform_feedback(struct gl_context *ctx,
			    struct gl_transform_feedback_object *obj);

/* brw_blorp_blit.cpp */
GLbitfield
brw_blorp_framebuffer(struct intel_context *intel,
                      GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter);

/* gen6_multisample_state.c */
void
gen6_emit_3dstate_multisample(struct brw_context *brw,
                              unsigned num_samples);
void
gen6_emit_3dstate_sample_mask(struct brw_context *brw,
                              unsigned num_samples, float coverage,
                              bool coverage_invert);

/* gen7_urb.c */
void
gen7_allocate_push_constants(struct brw_context *brw);

void
gen7_emit_urb_state(struct brw_context *brw, GLuint nr_vs_entries,
                    GLuint vs_size, GLuint vs_start);



/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct brw_context *
brw_context( struct gl_context *ctx )
{
   return (struct brw_context *)ctx;
}

static INLINE struct brw_vertex_program *
brw_vertex_program(struct gl_vertex_program *p)
{
   return (struct brw_vertex_program *) p;
}

static INLINE const struct brw_vertex_program *
brw_vertex_program_const(const struct gl_vertex_program *p)
{
   return (const struct brw_vertex_program *) p;
}

static INLINE struct brw_fragment_program *
brw_fragment_program(struct gl_fragment_program *p)
{
   return (struct brw_fragment_program *) p;
}

static INLINE const struct brw_fragment_program *
brw_fragment_program_const(const struct gl_fragment_program *p)
{
   return (const struct brw_fragment_program *) p;
}

/**
 * Pre-gen6, the register file of the EUs was shared between threads,
 * and each thread used some subset allocated on a 16-register block
 * granularity.  The unit states wanted these block counts.
 */
static inline int
brw_register_blocks(int reg_count)
{
   return ALIGN(reg_count, 16) / 16 - 1;
}

static inline uint32_t
brw_program_reloc(struct brw_context *brw, uint32_t state_offset,
		  uint32_t prog_offset)
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen >= 5) {
      /* Using state base address. */
      return prog_offset;
   }

   drm_intel_bo_emit_reloc(intel->batch.bo,
			   state_offset,
			   brw->cache.bo,
			   prog_offset,
			   I915_GEM_DOMAIN_INSTRUCTION, 0);

   return brw->cache.bo->offset + prog_offset;
}

bool brw_do_cubemap_normalize(struct exec_list *instructions);
bool brw_lower_texture_gradients(struct exec_list *instructions);

#ifdef __cplusplus
}
#endif

#endif
