### Lists of source files, included by Makefiles

# This file is among different build systems. SRCDIR must be defined with
# a trailing slash because the Android build system leaves it undefined.

# this is part of MAIN_FILES
MAIN_ES_FILES = \
	$(BUILDDIR)main/api_exec_es1.c \
	$(SRCDIR)main/es1_conversion.c

MAIN_FILES = \
	$(SRCDIR)main/api_arrayelt.c \
	$(SRCDIR)main/api_exec.c \
	$(SRCDIR)main/api_loopback.c \
	$(SRCDIR)main/api_validate.c \
	$(SRCDIR)main/accum.c \
	$(SRCDIR)main/arbprogram.c \
	$(SRCDIR)main/atifragshader.c \
	$(SRCDIR)main/attrib.c \
	$(SRCDIR)main/arrayobj.c \
	$(SRCDIR)main/blend.c \
	$(SRCDIR)main/bufferobj.c \
	$(SRCDIR)main/buffers.c \
	$(SRCDIR)main/clear.c \
	$(SRCDIR)main/clip.c \
	$(SRCDIR)main/colortab.c \
	$(SRCDIR)main/condrender.c \
	$(SRCDIR)main/context.c \
	$(SRCDIR)main/convolve.c \
	$(SRCDIR)main/cpuinfo.c \
	$(SRCDIR)main/debug.c \
	$(SRCDIR)main/depth.c \
	$(SRCDIR)main/dlist.c \
	$(SRCDIR)main/drawpix.c \
	$(SRCDIR)main/drawtex.c \
	$(SRCDIR)main/enable.c \
	$(SRCDIR)main/errors.c \
	$(SRCDIR)main/eval.c \
	$(SRCDIR)main/execmem.c \
	$(SRCDIR)main/extensions.c \
	$(SRCDIR)main/fbobject.c \
	$(SRCDIR)main/feedback.c \
	$(SRCDIR)main/ffvertex_prog.c \
	$(SRCDIR)main/fog.c \
	$(SRCDIR)main/formats.c \
	$(SRCDIR)main/format_pack.c \
	$(SRCDIR)main/format_unpack.c \
	$(SRCDIR)main/framebuffer.c \
	$(SRCDIR)main/get.c \
	$(SRCDIR)main/getstring.c \
	$(SRCDIR)main/glformats.c \
	$(SRCDIR)main/hash.c \
	$(SRCDIR)main/hint.c \
	$(SRCDIR)main/histogram.c \
	$(SRCDIR)main/image.c \
	$(SRCDIR)main/imports.c \
	$(SRCDIR)main/light.c \
	$(SRCDIR)main/lines.c \
	$(SRCDIR)main/matrix.c \
	$(SRCDIR)main/mipmap.c \
	$(SRCDIR)main/mm.c \
	$(SRCDIR)main/multisample.c \
	$(SRCDIR)main/nvprogram.c \
	$(SRCDIR)main/pack.c \
	$(SRCDIR)main/pbo.c \
	$(SRCDIR)main/pixel.c \
	$(SRCDIR)main/pixelstore.c \
	$(SRCDIR)main/pixeltransfer.c \
	$(SRCDIR)main/points.c \
	$(SRCDIR)main/polygon.c \
	$(SRCDIR)main/queryobj.c \
	$(SRCDIR)main/querymatrix.c \
	$(SRCDIR)main/rastpos.c \
	$(SRCDIR)main/readpix.c \
	$(SRCDIR)main/remap.c \
	$(SRCDIR)main/renderbuffer.c \
	$(SRCDIR)main/samplerobj.c \
	$(SRCDIR)main/scissor.c \
	$(SRCDIR)main/shaderapi.c \
	$(SRCDIR)main/shaderobj.c \
	$(SRCDIR)main/shared.c \
	$(SRCDIR)main/state.c \
	$(SRCDIR)main/stencil.c \
	$(SRCDIR)main/syncobj.c \
	$(SRCDIR)main/texcompress.c \
	$(SRCDIR)main/texcompress_cpal.c \
	$(SRCDIR)main/texcompress_rgtc.c \
	$(SRCDIR)main/texcompress_s3tc.c \
	$(SRCDIR)main/texcompress_fxt1.c \
	$(SRCDIR)main/texcompress_etc.c \
	$(SRCDIR)main/texenv.c \
	$(SRCDIR)main/texformat.c \
	$(SRCDIR)main/texgen.c \
	$(SRCDIR)main/texgetimage.c \
	$(SRCDIR)main/teximage.c \
	$(SRCDIR)main/texobj.c \
	$(SRCDIR)main/texparam.c \
	$(SRCDIR)main/texstate.c \
	$(SRCDIR)main/texstorage.c \
	$(SRCDIR)main/texstore.c \
	$(SRCDIR)main/texturebarrier.c \
	$(SRCDIR)main/transformfeedback.c \
	$(SRCDIR)main/uniforms.c \
	$(SRCDIR)main/varray.c \
	$(SRCDIR)main/version.c \
	$(SRCDIR)main/viewport.c \
	$(SRCDIR)main/vtxfmt.c \
	$(BUILDDIR)main/enums.c \
	$(MAIN_ES_FILES)

MAIN_CXX_FILES = \
	$(SRCDIR)main/ff_fragment_shader.cpp \
	$(SRCDIR)main/shader_query.cpp \
	$(SRCDIR)main/uniform_query.cpp

MATH_FILES = \
	$(SRCDIR)math/m_debug_clip.c \
	$(SRCDIR)math/m_debug_norm.c \
	$(SRCDIR)math/m_debug_xform.c \
	$(SRCDIR)math/m_eval.c \
	$(SRCDIR)math/m_matrix.c \
	$(SRCDIR)math/m_translate.c \
	$(SRCDIR)math/m_vector.c

MATH_XFORM_FILES = \
	$(SRCDIR)math/m_xform.c

SWRAST_FILES = \
	$(SRCDIR)swrast/s_aaline.c \
	$(SRCDIR)swrast/s_aatriangle.c \
	$(SRCDIR)swrast/s_alpha.c \
	$(SRCDIR)swrast/s_atifragshader.c \
	$(SRCDIR)swrast/s_bitmap.c \
	$(SRCDIR)swrast/s_blend.c \
	$(SRCDIR)swrast/s_blit.c \
	$(SRCDIR)swrast/s_clear.c \
	$(SRCDIR)swrast/s_copypix.c \
	$(SRCDIR)swrast/s_context.c \
	$(SRCDIR)swrast/s_depth.c \
	$(SRCDIR)swrast/s_drawpix.c \
	$(SRCDIR)swrast/s_feedback.c \
	$(SRCDIR)swrast/s_fog.c \
	$(SRCDIR)swrast/s_fragprog.c \
	$(SRCDIR)swrast/s_lines.c \
	$(SRCDIR)swrast/s_logic.c \
	$(SRCDIR)swrast/s_masking.c \
	$(SRCDIR)swrast/s_points.c \
	$(SRCDIR)swrast/s_renderbuffer.c \
	$(SRCDIR)swrast/s_span.c \
	$(SRCDIR)swrast/s_stencil.c \
	$(SRCDIR)swrast/s_texcombine.c \
	$(SRCDIR)swrast/s_texfetch.c \
	$(SRCDIR)swrast/s_texfilter.c \
	$(SRCDIR)swrast/s_texrender.c \
	$(SRCDIR)swrast/s_texture.c \
	$(SRCDIR)swrast/s_triangle.c \
	$(SRCDIR)swrast/s_zoom.c

SWRAST_SETUP_FILES = \
	$(SRCDIR)swrast_setup/ss_context.c \
	$(SRCDIR)swrast_setup/ss_triangle.c

TNL_FILES = \
	$(SRCDIR)tnl/t_context.c \
	$(SRCDIR)tnl/t_pipeline.c \
	$(SRCDIR)tnl/t_draw.c \
	$(SRCDIR)tnl/t_rasterpos.c \
	$(SRCDIR)tnl/t_vb_program.c \
	$(SRCDIR)tnl/t_vb_render.c \
	$(SRCDIR)tnl/t_vb_texgen.c \
	$(SRCDIR)tnl/t_vb_texmat.c \
	$(SRCDIR)tnl/t_vb_vertex.c \
	$(SRCDIR)tnl/t_vb_fog.c \
	$(SRCDIR)tnl/t_vb_light.c \
	$(SRCDIR)tnl/t_vb_normals.c \
	$(SRCDIR)tnl/t_vb_points.c \
	$(SRCDIR)tnl/t_vp_build.c \
	$(SRCDIR)tnl/t_vertex.c \
	$(SRCDIR)tnl/t_vertex_sse.c \
	$(SRCDIR)tnl/t_vertex_generic.c

VBO_FILES = \
	$(SRCDIR)vbo/vbo_context.c \
	$(SRCDIR)vbo/vbo_exec.c \
	$(SRCDIR)vbo/vbo_exec_api.c \
	$(SRCDIR)vbo/vbo_exec_array.c \
	$(SRCDIR)vbo/vbo_exec_draw.c \
	$(SRCDIR)vbo/vbo_exec_eval.c \
	$(SRCDIR)vbo/vbo_noop.c \
	$(SRCDIR)vbo/vbo_primitive_restart.c \
	$(SRCDIR)vbo/vbo_rebase.c \
	$(SRCDIR)vbo/vbo_split.c \
	$(SRCDIR)vbo/vbo_split_copy.c \
	$(SRCDIR)vbo/vbo_split_inplace.c \
	$(SRCDIR)vbo/vbo_save.c \
	$(SRCDIR)vbo/vbo_save_api.c \
	$(SRCDIR)vbo/vbo_save_draw.c \
	$(SRCDIR)vbo/vbo_save_loopback.c

STATETRACKER_FILES = \
	$(SRCDIR)state_tracker/st_atom.c \
	$(SRCDIR)state_tracker/st_atom_array.c \
	$(SRCDIR)state_tracker/st_atom_blend.c \
	$(SRCDIR)state_tracker/st_atom_clip.c \
	$(SRCDIR)state_tracker/st_atom_constbuf.c \
	$(SRCDIR)state_tracker/st_atom_depth.c \
	$(SRCDIR)state_tracker/st_atom_framebuffer.c \
	$(SRCDIR)state_tracker/st_atom_msaa.c \
	$(SRCDIR)state_tracker/st_atom_pixeltransfer.c \
	$(SRCDIR)state_tracker/st_atom_sampler.c \
	$(SRCDIR)state_tracker/st_atom_scissor.c \
	$(SRCDIR)state_tracker/st_atom_shader.c \
	$(SRCDIR)state_tracker/st_atom_rasterizer.c \
	$(SRCDIR)state_tracker/st_atom_stipple.c \
	$(SRCDIR)state_tracker/st_atom_texture.c \
	$(SRCDIR)state_tracker/st_atom_viewport.c \
	$(SRCDIR)state_tracker/st_cb_bitmap.c \
	$(SRCDIR)state_tracker/st_cb_blit.c \
	$(SRCDIR)state_tracker/st_cb_bufferobjects.c \
	$(SRCDIR)state_tracker/st_cb_clear.c \
	$(SRCDIR)state_tracker/st_cb_condrender.c \
	$(SRCDIR)state_tracker/st_cb_flush.c \
	$(SRCDIR)state_tracker/st_cb_drawpixels.c \
	$(SRCDIR)state_tracker/st_cb_drawtex.c \
	$(SRCDIR)state_tracker/st_cb_eglimage.c \
	$(SRCDIR)state_tracker/st_cb_fbo.c \
	$(SRCDIR)state_tracker/st_cb_feedback.c \
	$(SRCDIR)state_tracker/st_cb_program.c \
	$(SRCDIR)state_tracker/st_cb_queryobj.c \
	$(SRCDIR)state_tracker/st_cb_rasterpos.c \
	$(SRCDIR)state_tracker/st_cb_readpixels.c \
	$(SRCDIR)state_tracker/st_cb_syncobj.c \
	$(SRCDIR)state_tracker/st_cb_strings.c \
	$(SRCDIR)state_tracker/st_cb_texture.c \
	$(SRCDIR)state_tracker/st_cb_texturebarrier.c \
	$(SRCDIR)state_tracker/st_cb_viewport.c \
	$(SRCDIR)state_tracker/st_cb_xformfb.c \
	$(SRCDIR)state_tracker/st_context.c \
	$(SRCDIR)state_tracker/st_debug.c \
	$(SRCDIR)state_tracker/st_draw.c \
	$(SRCDIR)state_tracker/st_draw_feedback.c \
	$(SRCDIR)state_tracker/st_extensions.c \
	$(SRCDIR)state_tracker/st_format.c \
	$(SRCDIR)state_tracker/st_gen_mipmap.c \
	$(SRCDIR)state_tracker/st_manager.c \
	$(SRCDIR)state_tracker/st_mesa_to_tgsi.c \
	$(SRCDIR)state_tracker/st_program.c \
	$(SRCDIR)state_tracker/st_texture.c

PROGRAM_FILES = \
	$(SRCDIR)program/arbprogparse.c \
	$(SRCDIR)program/hash_table.c \
	$(SRCDIR)program/nvfragparse.c \
	$(SRCDIR)program/nvvertparse.c \
	$(SRCDIR)program/program.c \
	$(SRCDIR)program/program_parse_extra.c \
	$(SRCDIR)program/prog_cache.c \
	$(SRCDIR)program/prog_execute.c \
	$(SRCDIR)program/prog_instruction.c \
	$(SRCDIR)program/prog_noise.c \
	$(SRCDIR)program/prog_optimize.c \
	$(SRCDIR)program/prog_opt_constant_fold.c \
	$(SRCDIR)program/prog_parameter.c \
	$(SRCDIR)program/prog_parameter_layout.c \
	$(SRCDIR)program/prog_print.c \
	$(SRCDIR)program/prog_statevars.c \
	$(SRCDIR)program/programopt.c \
	$(SRCDIR)program/register_allocate.c \
	$(SRCDIR)program/symbol_table.c \
	$(BUILDDIR)program/lex.yy.c \
	$(BUILDDIR)program/program_parse.tab.c


SHADER_CXX_FILES = \
	$(SRCDIR)program/ir_to_mesa.cpp \
	$(SRCDIR)program/sampler.cpp \
	$(SRCDIR)program/string_to_uint_map.cpp

ASM_C_FILES =	\
	$(SRCDIR)x86/common_x86.c \
	$(SRCDIR)x86/x86_xform.c \
	$(SRCDIR)x86/3dnow.c \
	$(SRCDIR)x86/sse.c \
	$(SRCDIR)x86/rtasm/x86sse.c \
	$(SRCDIR)sparc/sparc.c \
	$(SRCDIR)x86-64/x86-64.c

X86_FILES =			\
	$(SRCDIR)x86/common_x86_asm.S	\
	$(SRCDIR)x86/x86_xform2.S	\
	$(SRCDIR)x86/x86_xform3.S	\
	$(SRCDIR)x86/x86_xform4.S	\
	$(SRCDIR)x86/x86_cliptest.S	\
	$(SRCDIR)x86/mmx_blend.S		\
	$(SRCDIR)x86/3dnow_xform1.S	\
	$(SRCDIR)x86/3dnow_xform2.S	\
	$(SRCDIR)x86/3dnow_xform3.S	\
	$(SRCDIR)x86/3dnow_xform4.S	\
	$(SRCDIR)x86/3dnow_normal.S	\
	$(SRCDIR)x86/sse_xform1.S	\
	$(SRCDIR)x86/sse_xform2.S	\
	$(SRCDIR)x86/sse_xform3.S	\
	$(SRCDIR)x86/sse_xform4.S	\
	$(SRCDIR)x86/sse_normal.S	\
	$(SRCDIR)x86/read_rgba_span_x86.S

X86_64_FILES =		\
	$(SRCDIR)x86-64/xform4.S

SPARC_FILES =			\
	$(SRCDIR)sparc/sparc_clip.S	\
	$(SRCDIR)sparc/norm.S		\
	$(SRCDIR)sparc/xform.S

COMMON_DRIVER_FILES =			\
	$(SRCDIR)drivers/common/driverfuncs.c	\
	$(SRCDIR)drivers/common/meta.c


# Sources for building non-Gallium drivers
MESA_FILES = \
	$(MAIN_FILES)		\
	$(MATH_FILES)		\
	$(MATH_XFORM_FILES)	\
	$(VBO_FILES)		\
	$(TNL_FILES)		\
	$(PROGRAM_FILES)	\
	$(SWRAST_FILES)	\
	$(SWRAST_SETUP_FILES)	\
	$(COMMON_DRIVER_FILES)\
	$(ASM_C_FILES)

MESA_CXX_FILES = \
	$(MAIN_CXX_FILES) \
	$(SHADER_CXX_FILES)

# Sources for building Gallium drivers
MESA_GALLIUM_FILES = \
	$(MAIN_FILES)		\
	$(MATH_FILES)		\
	$(VBO_FILES)		\
	$(STATETRACKER_FILES)	\
	$(PROGRAM_FILES)	\
	$(SRCDIR)x86/common_x86.c

MESA_GALLIUM_CXX_FILES = \
	$(MESA_CXX_FILES) \
	$(SRCDIR)state_tracker/st_glsl_to_tgsi.cpp

# All the core C sources, for dependency checking
ALL_FILES = \
	$(MESA_FILES)		\
	$(MESA_GALLIUM_CXX_FILES) \
	$(MESA_ASM_FILES)	\
	$(STATETRACKER_FILES)


### Object files

MESA_OBJECTS = \
	$(MESA_FILES:.c=.o) \
	$(MESA_CXX_FILES:.cpp=.o) \
	$(MESA_ASM_FILES:.S=.o)

MESA_GALLIUM_OBJECTS = \
	$(MESA_GALLIUM_FILES:.c=.o) \
	$(MESA_GALLIUM_CXX_FILES:.cpp=.o) \
	$(MESA_ASM_FILES:.S=.o)


COMMON_DRIVER_OBJECTS = $(COMMON_DRIVER_FILES:.c=.o)


### Include directories

INCLUDE_DIRS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/glsl \
	-I$(top_builddir)/src/glsl \
	-I$(top_srcdir)/src/glsl/glcpp \
	-I$(top_srcdir)/src/mesa \
	-I$(top_builddir)/src/mesa \
	-I$(top_srcdir)/src/mesa/main \
	-I$(top_builddir)/src/mesa/main \
	-I$(top_srcdir)/src/mapi \
	-I$(top_builddir)/src/mapi \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary
