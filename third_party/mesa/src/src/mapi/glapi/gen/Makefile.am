# This file isn't used during a normal compilation since we don't want to
# require Python in order to compile Mesa.
# Instead, when the Mesa developers update/change the API interface it's
# up to him/her to re-run this makefile and check in the newly generated files.


TOP = ../../../..

# These are the "official" xserver indent flags from utils/modular/x-indent.sh
XORG_INDENT_FLAGS = -linux -bad -bap -blf -bli0 -cbi0 -cdw -nce -cs -i4 -lc80 -psl -nbbo \
    -nbc -psl -nbfda -nut -nss -T pointer -T ScreenPtr -T ScrnInfoPtr -T pointer \
    -T DeviceIntPtr -T DevicePtr -T ClientPtr -T CallbackListPtr \
    -T CallbackProcPtr -T OsTimerPtr -T CARD32 -T CARD16 -T CARD8 \
    -T INT32 -T INT16 -T INT8 -T Atom -T Time -T WindowPtr -T DrawablePtr \
    -T PixmapPtr -T ColormapPtr -T CursorPtr -T Font -T XID -T Mask \
    -T BlockHandlerProcPtr -T WakeupHandlerProcPtr -T RegionPtr \
    -T InternalEvent -T GrabPtr -T Timestamp -T Bool -T TimeStamp \
    -T xEvent -T DeviceEvent -T RawDeviceEvent -T GrabMask -T Window \
    -T Drawable -T FontPtr -T CallbackPtr -T XIPropertyValuePtr \
    -T GrabParameters -T deviceKeyButtonPointer -T TouchOwnershipEvent \
    -T xGenericEvent -T DeviceChangedEvent -T GCPtr -T BITS32 \
    -T xRectangle -T BoxPtr -T RegionRec -T ValuatorMask -T KeyCode \
    -T KeySymsPtr -T XkbDescPtr -T InputOption -T XI2Mask -T DevUnion \
    -T DevPrivateKey -T DevScreenPrivateKey -T PropertyPtr -T RESTYPE \
    -T XkbAction -T XkbChangesPtr -T XkbControlsPtr -T PrivatePtr -T pmWait \
    -T _XFUNCPROTOBEGIN -T _XFUNCPROTOEND -T _X_EXPORT


MESA_DIR = $(TOP)/src/mesa
MESA_GLAPI_DIR = $(TOP)/src/mapi/glapi
MESA_MAPI_DIR = $(TOP)/src/mapi/mapi
MESA_GLX_DIR = $(TOP)/src/glx

MESA_GLAPI_OUTPUTS = \
	$(MESA_GLAPI_DIR)/glapi_mapi_tmp.h \
	$(MESA_GLAPI_DIR)/glprocs.h \
	$(MESA_GLAPI_DIR)/glapitemp.h \
	$(MESA_GLAPI_DIR)/glapitable.h \
	$(MESA_GLAPI_DIR)/glapi_gentable.c

MESA_GLAPI_ASM_OUTPUTS =
if HAVE_X86_ASM
MESA_GLAPI_ASM_OUTPUTS += $(MESA_GLAPI_DIR)/glapi_x86.S
endif

if HAVE_X86_64_ASM
MESA_GLAPI_ASM_OUTPUTS += $(MESA_GLAPI_DIR)/glapi_x86-64.S
endif

if HAVE_SPARC_ASM
MESA_GLAPI_ASM_OUTPUTS += $(MESA_GLAPI_DIR)/glapi_sparc.S
endif

MESA_OUTPUTS = \
	$(MESA_GLAPI_OUTPUTS) \
	$(MESA_GLAPI_ASM_OUTPUTS) \
	$(MESA_DIR)/main/enums.c \
	$(MESA_DIR)/main/dispatch.h \
	$(MESA_DIR)/main/remap_helper.h \
	$(MESA_GLX_DIR)/indirect.c \
	$(MESA_GLX_DIR)/indirect.h \
	$(MESA_GLX_DIR)/indirect_init.c \
	$(MESA_GLX_DIR)/indirect_size.h \
	$(MESA_GLX_DIR)/indirect_size.c

######################################################################

XORG_GLX_DIR = $(XORG_BASE)/glx
XORG_GLAPI_DIR = $(XORG_BASE)/glx

XORG_GLAPI_OUTPUTS = \
	$(XORG_GLAPI_DIR)/glprocs.h \
	$(XORG_GLAPI_DIR)/glapitable.h \
	$(XORG_GLAPI_DIR)/dispatch.h \
	$(XORG_GLAPI_DIR)/glapi_gentable.c

XORG_OUTPUTS = \
	$(XORG_GLAPI_OUTPUTS) \
	$(XORG_GLX_DIR)/indirect_dispatch.c \
	$(XORG_GLX_DIR)/indirect_dispatch_swap.c \
	$(XORG_GLX_DIR)/indirect_dispatch.h \
	$(XORG_GLX_DIR)/indirect_reqsize.c \
	$(XORG_GLX_DIR)/indirect_reqsize.h \
	$(XORG_GLX_DIR)/indirect_size.h \
	$(XORG_GLX_DIR)/indirect_size_get.c \
	$(XORG_GLX_DIR)/indirect_size_get.h \
	$(XORG_GLX_DIR)/indirect_table.c

######################################################################

API_XML = \
	gl_API.xml \
	ARB_base_instance.xml \
	ARB_color_buffer_float.xml \
	ARB_copy_buffer.xml \
	ARB_debug_output.xml \
	ARB_depth_buffer_float.xml \
	ARB_depth_clamp.xml \
	ARB_draw_buffers_blend.xml \
	ARB_draw_elements_base_vertex.xml \
	ARB_draw_instanced.xml \
	ARB_ES2_compatibility.xml \
	ARB_framebuffer_object.xml \
	ARB_geometry_shader4.xml \
	ARB_instanced_arrays.xml \
	ARB_map_buffer_range.xml \
	ARB_robustness.xml \
	ARB_sampler_objects.xml \
	ARB_seamless_cube_map.xml \
	ARB_sync.xml \
	ARB_texture_buffer_object.xml \
	ARB_texture_compression_rgtc.xml \
	ARB_texture_float.xml \
	ARB_texture_rg.xml \
	ARB_texture_storage.xml \
	ARB_vertex_array_object.xml \
	AMD_draw_buffers_blend.xml \
	ARB_vertex_type_2_10_10_10_rev.xml \
	APPLE_object_purgeable.xml \
	APPLE_vertex_array_object.xml \
	ARB_uniform_buffer_object.xml \
	EXT_draw_buffers2.xml \
	EXT_framebuffer_object.xml \
	EXT_gpu_shader4.xml \
	EXT_packed_depth_stencil.xml \
	EXT_provoking_vertex.xml \
	EXT_separate_shader_objects.xml \
	EXT_texture_array.xml \
	EXT_texture_integer.xml \
	EXT_transform_feedback.xml \
	NV_conditional_render.xml \
	NV_primitive_restart.xml \
	NV_texture_barrier.xml \
	OES_EGL_image.xml \
	GL3x.xml


COMMON = $(API_XML) gl_XML.py glX_XML.py license.py typeexpr.py

COMMON_ES = \
	$(COMMON) \
	gl_and_es_API.xml \
	es_EXT.xml \
	ARB_ES2_compatibility.xml \
	ARB_get_program_binary.xml \
	OES_fixed_point.xml \
	OES_single_precision.xml

COMMON_GLX = $(COMMON) glX_API.xml glX_XML.py glX_proto_common.py

PYTHON_GEN = $(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS)

######################################################################

all-local: $(MESA_OUTPUTS)

xorg: check-xorg-source $(XORG_OUTPUTS)

check-xorg-source:
	@if ! test -d $(XORG_GLX_DIR); then \
		echo "ERROR: Must specify path to xserver/ checkout. Set XORG_BASE env var."; \
		if test x$(XORG_BASE) != x; then \
			echo "'$(XORG_GLX_DIR)' does not exist."; \
		fi; \
		exit 1; \
	fi

clean-local:
	-rm -f *~ *.pyo
	-rm -f $(MESA_OUTPUTS)

######################################################################

$(XORG_GLAPI_DIR)/%.c: $(MESA_GLAPI_DIR)/%.c
	$(INDENT) $(XORG_INDENT_FLAGS) < $< > $@

$(XORG_GLAPI_DIR)/dispatch.h: $(MESA_DIR)/main/dispatch.h
	$(INDENT) $(XORG_INDENT_FLAGS) < $< > $@

$(XORG_GLAPI_DIR)/%.h: $(MESA_GLAPI_DIR)/%.h
	$(INDENT) $(XORG_INDENT_FLAGS) < $< > $@

######################################################################

$(MESA_GLAPI_DIR)/glapi_mapi_tmp.h: $(MESA_MAPI_DIR)/mapi_abi.py $(COMMON_ES)
	$(PYTHON_GEN) $< \
		--printer glapi --mode lib $(srcdir)/gl_and_es_API.xml > $@

$(MESA_GLAPI_DIR)/glprocs.h: gl_procs.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

$(MESA_GLAPI_DIR)/glapitemp.h: gl_apitemp.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

$(MESA_GLAPI_DIR)/glapitable.h: gl_table.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

$(MESA_GLAPI_DIR)/glapi_gentable.c: gl_gentable.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

######################################################################

$(MESA_GLAPI_DIR)/glapi_x86.S: gl_x86_asm.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

$(MESA_GLAPI_DIR)/glapi_x86-64.S: gl_x86-64_asm.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

$(MESA_GLAPI_DIR)/glapi_sparc.S: gl_SPARC_asm.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

######################################################################

$(MESA_DIR)/main/enums.c: gl_enums.py $(COMMON_ES)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_and_es_API.xml > $@

$(MESA_DIR)/main/dispatch.h: gl_table.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m remap_table > $@

$(MESA_DIR)/main/remap_helper.h: remap_helper.py $(COMMON)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml > $@

######################################################################

$(MESA_GLX_DIR)/indirect.c: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m proto \
	  | $(INDENT) $(INDENT_FLAGS) > $@

$(MESA_GLX_DIR)/indirect.h: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m init_h > $@

$(MESA_GLX_DIR)/indirect_init.c: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m init_c > $@

$(MESA_GLX_DIR)/indirect_size.h $(XORG_GLX_DIR)/indirect_size.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m size_h --only-set \
	    -h _INDIRECT_SIZE_H_ \
	  | $(INDENT) $(INDENT_FLAGS) > $@

$(MESA_GLX_DIR)/indirect_size.c: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m size_c --only-set \
	  | $(INDENT) $(INDENT_FLAGS) > $@

######################################################################

$(XORG_GLX_DIR)/indirect_dispatch.c: glX_proto_recv.py $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -f $(srcdir)/gl_API.xml -m dispatch_c \
	| $(INDENT) $(XORG_INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_dispatch_swap.c: glX_proto_recv.py $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -f $(srcdir)/gl_API.xml -m dispatch_c -s \
	| $(INDENT) $(XORG_INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_dispatch.h: glX_proto_recv.py gl_and_glX_API.xml $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -m dispatch_h -f $(srcdir)/gl_and_glX_API.xml -s \
	| $(INDENT) $(XORG_INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_size_get.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -f $(srcdir)/gl_API.xml -m size_h \
           --only-get -h '_INDIRECT_SIZE_GET_H_' \
	  | $(INDENT) $(XORG_INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_size_get.c: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON_GEN) $< -f $(srcdir)/gl_API.xml -m size_c \
	  | $(INDENT) $(INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_reqsize.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -f $(srcdir)/gl_API.xml -m reqsize_h \
           --only-get -h '_INDIRECT_SIZE_GET_H_' \
	  | $(INDENT) $(XORG_INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_reqsize.c: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -f $(srcdir)/gl_API.xml -m reqsize_c \
          | $(INDENT) $(XORG_INDENT_FLAGS) > $@

$(XORG_GLX_DIR)/indirect_table.c: glX_server_table.py gl_and_glX_API.xml $(COMMON_GLX)
	$(PYTHON_GEN) $(PYTHON_FLAGS) $< -f $(srcdir)/gl_and_glX_API.xml \
	| $(INDENT) $(XORG_INDENT_FLAGS) > $@
