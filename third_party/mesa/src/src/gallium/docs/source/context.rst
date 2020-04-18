.. _context:

Context
=======

A Gallium rendering context encapsulates the state which effects 3D
rendering such as blend state, depth/stencil state, texture samplers,
etc.

Note that resource/texture allocation is not per-context but per-screen.


Methods
-------

CSO State
^^^^^^^^^

All Constant State Object (CSO) state is created, bound, and destroyed,
with triplets of methods that all follow a specific naming scheme.
For example, ``create_blend_state``, ``bind_blend_state``, and
``destroy_blend_state``.

CSO objects handled by the context object:

* :ref:`Blend`: ``*_blend_state``
* :ref:`Sampler`: Texture sampler states are bound separately for fragment,
  vertex and geometry samplers.  Note that sampler states are set en masse.
  If M is the max number of sampler units supported by the driver and N
  samplers are bound with ``bind_fragment_sampler_states`` then sampler
  units N..M-1 are considered disabled/NULL.
* :ref:`Rasterizer`: ``*_rasterizer_state``
* :ref:`Depth, Stencil, & Alpha`: ``*_depth_stencil_alpha_state``
* :ref:`Shader`: These are create, bind and destroy methods for vertex,
  fragment and geometry shaders.
* :ref:`Vertex Elements`: ``*_vertex_elements_state``


Resource Binding State
^^^^^^^^^^^^^^^^^^^^^^

This state describes how resources in various flavours (textures,
buffers, surfaces) are bound to the driver.


* ``set_constant_buffer`` sets a constant buffer to be used for a given shader
  type. index is used to indicate which buffer to set (some apis may allow
  multiple ones to be set, and binding a specific one later, though drivers
  are mostly restricted to the first one right now).

* ``set_framebuffer_state``

* ``set_vertex_buffers``

* ``set_index_buffer``


Non-CSO State
^^^^^^^^^^^^^

These pieces of state are too small, variable, and/or trivial to have CSO
objects. They all follow simple, one-method binding calls, e.g.
``set_blend_color``.

* ``set_stencil_ref`` sets the stencil front and back reference values
  which are used as comparison values in stencil test.
* ``set_blend_color``
* ``set_sample_mask``
* ``set_clip_state``
* ``set_polygon_stipple``
* ``set_scissor_state`` sets the bounds for the scissor test, which culls
  pixels before blending to render targets. If the :ref:`Rasterizer` does
  not have the scissor test enabled, then the scissor bounds never need to
  be set since they will not be used.  Note that scissor xmin and ymin are
  inclusive, but  xmax and ymax are exclusive.  The inclusive ranges in x
  and y would be [xmin..xmax-1] and [ymin..ymax-1].
* ``set_viewport_state``


Sampler Views
^^^^^^^^^^^^^

These are the means to bind textures to shader stages. To create one, specify
its format, swizzle and LOD range in sampler view template.

If texture format is different than template format, it is said the texture
is being cast to another format. Casting can be done only between compatible
formats, that is formats that have matching component order and sizes.

Swizzle fields specify they way in which fetched texel components are placed
in the result register. For example, ``swizzle_r`` specifies what is going to be
placed in first component of result register.

The ``first_level`` and ``last_level`` fields of sampler view template specify
the LOD range the texture is going to be constrained to. Note that these
values are in addition to the respective min_lod, max_lod values in the
pipe_sampler_state (that is if min_lod is 2.0, and first_level 3, the first mip
level used for sampling from the resource is effectively the fifth).

The ``first_layer`` and ``last_layer`` fields specify the layer range the
texture is going to be constrained to. Similar to the LOD range, this is added
to the array index which is used for sampling.

* ``set_fragment_sampler_views`` binds an array of sampler views to
  fragment shader stage. Every binding point acquires a reference
  to a respective sampler view and releases a reference to the previous
  sampler view.  If M is the maximum number of sampler units and N units
  is passed to set_fragment_sampler_views, the driver should unbind the
  sampler views for units N..M-1.

* ``set_vertex_sampler_views`` binds an array of sampler views to vertex
  shader stage. Every binding point acquires a reference to a respective
  sampler view and releases a reference to the previous sampler view.

* ``create_sampler_view`` creates a new sampler view. ``texture`` is associated
  with the sampler view which results in sampler view holding a reference
  to the texture. Format specified in template must be compatible
  with texture format.

* ``sampler_view_destroy`` destroys a sampler view and releases its reference
  to associated texture.

Shader Resources
^^^^^^^^^^^^^^^^

Shader resources are textures or buffers that may be read or written
from a shader without an associated sampler.  This means that they
have no support for floating point coordinates, address wrap modes or
filtering.

Shader resources are specified for all the shader stages at once using
the ``set_shader_resources`` method.  When binding texture resources,
the ``level``, ``first_layer`` and ``last_layer`` pipe_surface fields
specify the mipmap level and the range of layers the texture will be
constrained to.  In the case of buffers, ``first_element`` and
``last_element`` specify the range within the buffer that will be used
by the shader resource.  Writes to a shader resource are only allowed
when the ``writable`` flag is set.

Surfaces
^^^^^^^^

These are the means to use resources as color render targets or depthstencil
attachments. To create one, specify the mip level, the range of layers, and
the bind flags (either PIPE_BIND_DEPTH_STENCIL or PIPE_BIND_RENDER_TARGET).
Note that layer values are in addition to what is indicated by the geometry
shader output variable XXX_FIXME (that is if first_layer is 3 and geometry
shader indicates index 2, the 5th layer of the resource will be used). These
first_layer and last_layer parameters will only be used for 1d array, 2d array,
cube, and 3d textures otherwise they are 0.

* ``create_surface`` creates a new surface.

* ``surface_destroy`` destroys a surface and releases its reference to the
  associated resource.

Stream output targets
^^^^^^^^^^^^^^^^^^^^^

Stream output, also known as transform feedback, allows writing the primitives
produced by the vertex pipeline to buffers. This is done after the geometry
shader or vertex shader if no geometry shader is present.

The stream output targets are views into buffer resources which can be bound
as stream outputs and specify a memory range where it's valid to write
primitives. The pipe driver must implement memory protection such that any
primitives written outside of the specified memory range are discarded.

Two stream output targets can use the same resource at the same time, but
with a disjoint memory range.

Additionally, the stream output target internally maintains the offset
into the buffer which is incremented everytime something is written to it.
The internal offset is equal to how much data has already been written.
It can be stored in device memory and the CPU actually doesn't have to query
it.

The stream output target can be used in a draw command to provide
the vertex count. The vertex count is derived from the internal offset
discussed above.

* ``create_stream_output_target`` create a new target.

* ``stream_output_target_destroy`` destroys a target. Users of this should
  use pipe_so_target_reference instead.

* ``set_stream_output_targets`` binds stream output targets. The parameter
  append_bitmask is a bitmask, where the i-th bit specifies whether new
  primitives should be appended to the i-th buffer (writing starts at
  the internal offset), or whether writing should start at the beginning
  (the internal offset is effectively set to 0).

NOTE: The currently-bound vertex or geometry shader must be compiled with
the properly-filled-in structure pipe_stream_output_info describing which
outputs should be written to buffers and how. The structure is part of
pipe_shader_state.

Clearing
^^^^^^^^

Clear is one of the most difficult concepts to nail down to a single
interface (due to both different requirements from APIs and also driver/hw
specific differences).

``clear`` initializes some or all of the surfaces currently bound to
the framebuffer to particular RGBA, depth, or stencil values.
Currently, this does not take into account color or stencil write masks (as
used by GL), and always clears the whole surfaces (no scissoring as used by
GL clear or explicit rectangles like d3d9 uses). It can, however, also clear
only depth or stencil in a combined depth/stencil surface, if the driver
supports PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE.
If a surface includes several layers then all layers will be cleared.

``clear_render_target`` clears a single color rendertarget with the specified
color value. While it is only possible to clear one surface at a time (which can
include several layers), this surface need not be bound to the framebuffer.

``clear_depth_stencil`` clears a single depth, stencil or depth/stencil surface
with the specified depth and stencil values (for combined depth/stencil buffers,
is is also possible to only clear one or the other part). While it is only
possible to clear one surface at a time (which can include several layers),
this surface need not be bound to the framebuffer.


Drawing
^^^^^^^

``draw_vbo`` draws a specified primitive.  The primitive mode and other
properties are described by ``pipe_draw_info``.

The ``mode``, ``start``, and ``count`` fields of ``pipe_draw_info`` specify the
the mode of the primitive and the vertices to be fetched, in the range between
``start`` to ``start``+``count``-1, inclusive.

Every instance with instanceID in the range between ``start_instance`` and
``start_instance``+``instance_count``-1, inclusive, will be drawn.

If there is an index buffer bound, and ``indexed`` field is true, all vertex
indices will be looked up in the index buffer.

In indexed draw, ``min_index`` and ``max_index`` respectively provide a lower
and upper bound of the indices contained in the index buffer inside the range
between ``start`` to ``start``+``count``-1.  This allows the driver to
determine which subset of vertices will be referenced during te draw call
without having to scan the index buffer.  Providing a over-estimation of the
the true bounds, for example, a ``min_index`` and ``max_index`` of 0 and
0xffffffff respectively, must give exactly the same rendering, albeit with less
performance due to unreferenced vertex buffers being unnecessarily DMA'ed or
processed.  Providing a underestimation of the true bounds will result in
undefined behavior, but should not result in program or system failure.

In case of non-indexed draw, ``min_index`` should be set to
``start`` and ``max_index`` should be set to ``start``+``count``-1.

``index_bias`` is a value added to every vertex index after lookup and before
fetching vertex attributes.

When drawing indexed primitives, the primitive restart index can be
used to draw disjoint primitive strips.  For example, several separate
line strips can be drawn by designating a special index value as the
restart index.  The ``primitive_restart`` flag enables/disables this
feature.  The ``restart_index`` field specifies the restart index value.

When primitive restart is in use, array indexes are compared to the
restart index before adding the index_bias offset.

If a given vertex element has ``instance_divisor`` set to 0, it is said
it contains per-vertex data and effective vertex attribute address needs
to be recalculated for every index.

  attribAddr = ``stride`` * index + ``src_offset``

If a given vertex element has ``instance_divisor`` set to non-zero,
it is said it contains per-instance data and effective vertex attribute
address needs to recalculated for every ``instance_divisor``-th instance.

  attribAddr = ``stride`` * instanceID / ``instance_divisor`` + ``src_offset``

In the above formulas, ``src_offset`` is taken from the given vertex element
and ``stride`` is taken from a vertex buffer associated with the given
vertex element.

The calculated attribAddr is used as an offset into the vertex buffer to
fetch the attribute data.

The value of ``instanceID`` can be read in a vertex shader through a system
value register declared with INSTANCEID semantic name.


Queries
^^^^^^^

Queries gather some statistic from the 3D pipeline over one or more
draws.  Queries may be nested, though only d3d1x currently exercises this.

Queries can be created with ``create_query`` and deleted with
``destroy_query``. To start a query, use ``begin_query``, and when finished,
use ``end_query`` to end the query.

``get_query_result`` is used to retrieve the results of a query.  If
the ``wait`` parameter is TRUE, then the ``get_query_result`` call
will block until the results of the query are ready (and TRUE will be
returned).  Otherwise, if the ``wait`` parameter is FALSE, the call
will not block and the return value will be TRUE if the query has
completed or FALSE otherwise.

The interface currently includes the following types of queries:

``PIPE_QUERY_OCCLUSION_COUNTER`` counts the number of fragments which
are written to the framebuffer without being culled by
:ref:`Depth, Stencil, & Alpha` testing or shader KILL instructions.
The result is an unsigned 64-bit integer.
This query can be used with ``render_condition``.

In cases where a boolean result of an occlusion query is enough,
``PIPE_QUERY_OCCLUSION_PREDICATE`` should be used. It is just like
``PIPE_QUERY_OCCLUSION_COUNTER`` except that the result is a boolean
value of FALSE for cases where COUNTER would result in 0 and TRUE
for all other cases.
This query can be used with ``render_condition``.

``PIPE_QUERY_TIME_ELAPSED`` returns the amount of time, in nanoseconds,
the context takes to perform operations.
The result is an unsigned 64-bit integer.

``PIPE_QUERY_TIMESTAMP`` returns a device/driver internal timestamp,
scaled to nanoseconds, recorded after all commands issued prior to
``end_query`` have been processed.
This query does not require a call to ``begin_query``.
The result is an unsigned 64-bit integer.

``PIPE_QUERY_TIMESTAMP_DISJOINT`` can be used to check whether the
internal timer resolution is good enough to distinguish between the
events at ``begin_query`` and ``end_query``.
The result is a 64-bit integer specifying the timer resolution in Hz,
followed by a boolean value indicating whether the timer has incremented.

``PIPE_QUERY_PRIMITIVES_GENERATED`` returns a 64-bit integer indicating
the number of primitives processed by the pipeline.

``PIPE_QUERY_PRIMITIVES_EMITTED`` returns a 64-bit integer indicating
the number of primitives written to stream output buffers.

``PIPE_QUERY_SO_STATISTICS`` returns 2 64-bit integers corresponding to
the results of
``PIPE_QUERY_PRIMITIVES_EMITTED`` and
``PIPE_QUERY_PRIMITIVES_GENERATED``, in this order.

``PIPE_QUERY_SO_OVERFLOW_PREDICATE`` returns a boolean value indicating
whether the stream output targets have overflowed as a result of the
commands issued between ``begin_query`` and ``end_query``.
This query can be used with ``render_condition``.

``PIPE_QUERY_GPU_FINISHED`` returns a boolean value indicating whether
all commands issued before ``end_query`` have completed. However, this
does not imply serialization.
This query does not require a call to ``begin_query``.

``PIPE_QUERY_PIPELINE_STATISTICS`` returns an array of the following
64-bit integers:
Number of vertices read from vertex buffers.
Number of primitives read from vertex buffers.
Number of vertex shader threads launched.
Number of geometry shader threads launched.
Number of primitives generated by geometry shaders.
Number of primitives forwarded to the rasterizer.
Number of primitives rasterized.
Number of fragment shader threads launched.
Number of tessellation control shader threads launched.
Number of tessellation evaluation shader threads launched.
If a shader type is not supported by the device/driver,
the corresponding values should be set to 0.

Gallium does not guarantee the availability of any query types; one must
always check the capabilities of the :ref:`Screen` first.


Conditional Rendering
^^^^^^^^^^^^^^^^^^^^^

A drawing command can be skipped depending on the outcome of a query
(typically an occlusion query).  The ``render_condition`` function specifies
the query which should be checked prior to rendering anything.

If ``render_condition`` is called with ``query`` = NULL, conditional
rendering is disabled and drawing takes place normally.

If ``render_condition`` is called with a non-null ``query`` subsequent
drawing commands will be predicated on the outcome of the query.  If
the query result is zero subsequent drawing commands will be skipped.

If ``mode`` is PIPE_RENDER_COND_WAIT the driver will wait for the
query to complete before deciding whether to render.

If ``mode`` is PIPE_RENDER_COND_NO_WAIT and the query has not yet
completed, the drawing command will be executed normally.  If the query
has completed, drawing will be predicated on the outcome of the query.

If ``mode`` is PIPE_RENDER_COND_BY_REGION_WAIT or
PIPE_RENDER_COND_BY_REGION_NO_WAIT rendering will be predicated as above
for the non-REGION modes but in the case that an occulusion query returns
a non-zero result, regions which were occluded may be ommitted by subsequent
drawing commands.  This can result in better performance with some GPUs.
Normally, if the occlusion query returned a non-zero result subsequent
drawing happens normally so fragments may be generated, shaded and
processed even where they're known to be obscured.


Flushing
^^^^^^^^

``flush``


Resource Busy Queries
^^^^^^^^^^^^^^^^^^^^^

``is_resource_referenced``



Blitting
^^^^^^^^

These methods emulate classic blitter controls.

These methods operate directly on ``pipe_resource`` objects, and stand
apart from any 3D state in the context.  Blitting functionality may be
moved to a separate abstraction at some point in the future.

``resource_copy_region`` blits a region of a resource to a region of another
resource, provided that both resources have the same format, or compatible
formats, i.e., formats for which copying the bytes from the source resource
unmodified to the destination resource will achieve the same effect of a
textured quad blitter.. The source and destination may be the same resource,
but overlapping blits are not permitted.

``resource_resolve`` resolves a multisampled resource into a non-multisampled
one. Their formats must match. This function must be present if a driver
supports multisampling.
The region that is to be resolved is described by ``pipe_resolve_info``, which
provides a source and a destination rectangle.
The source rectangle may be vertically flipped, but otherwise the dimensions
of the rectangles must match, unless PIPE_CAP_SCALED_RESOLVE is supported,
in which case scaling and horizontal flipping are allowed as well.
The result of resolving depth/stencil values may be any function of the values at
the sample points, but returning the value of the centermost sample is preferred.

The interfaces to these calls are likely to change to make it easier
for a driver to batch multiple blits with the same source and
destination.

Transfers
^^^^^^^^^

These methods are used to get data to/from a resource.

``get_transfer`` creates a transfer object.

``transfer_destroy`` destroys the transfer object. May cause
data to be written to the resource at this point.

``transfer_map`` creates a memory mapping for the transfer object.
The returned map points to the start of the mapped range according to
the box region, not the beginning of the resource.

``transfer_unmap`` remove the memory mapping for the transfer object.
Any pointers into the map should be considered invalid and discarded.

``transfer_inline_write`` performs a simplified transfer for simple writes.
Basically get_transfer, transfer_map, data write, transfer_unmap, and
transfer_destroy all in one.


The box parameter to some of these functions defines a 1D, 2D or 3D
region of pixels.  This is self-explanatory for 1D, 2D and 3D texture
targets.

For PIPE_TEXTURE_1D_ARRAY, the box::y and box::height fields refer to the
array dimension of the texture.

For PIPE_TEXTURE_2D_ARRAY, the box::z and box::depth fields refer to the
array dimension of the texture.

For PIPE_TEXTURE_CUBE, the box:z and box::depth fields refer to the
faces of the cube map (z + depth <= 6).



.. _transfer_flush_region:

transfer_flush_region
%%%%%%%%%%%%%%%%%%%%%

If a transfer was created with ``FLUSH_EXPLICIT``, it will not automatically
be flushed on write or unmap. Flushes must be requested with
``transfer_flush_region``. Flush ranges are relative to the mapped range, not
the beginning of the resource.



.. _texture_barrier

texture_barrier
%%%%%%%%%%%%%%%

This function flushes all pending writes to the currently-set surfaces and
invalidates all read caches of the currently-set samplers.



.. _pipe_transfer:

PIPE_TRANSFER
^^^^^^^^^^^^^

These flags control the behavior of a transfer object.

``PIPE_TRANSFER_READ``
  Resource contents read back (or accessed directly) at transfer create time.

``PIPE_TRANSFER_WRITE``
  Resource contents will be written back at transfer_destroy time (or modified
  as a result of being accessed directly).

``PIPE_TRANSFER_MAP_DIRECTLY``
  a transfer should directly map the resource. May return NULL if not supported.

``PIPE_TRANSFER_DISCARD_RANGE``
  The memory within the mapped region is discarded.  Cannot be used with
  ``PIPE_TRANSFER_READ``.

``PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE``
  Discards all memory backing the resource.  It should not be used with
  ``PIPE_TRANSFER_READ``.

``PIPE_TRANSFER_DONTBLOCK``
  Fail if the resource cannot be mapped immediately.

``PIPE_TRANSFER_UNSYNCHRONIZED``
  Do not synchronize pending operations on the resource when mapping. The
  interaction of any writes to the map and any operations pending on the
  resource are undefined. Cannot be used with ``PIPE_TRANSFER_READ``.

``PIPE_TRANSFER_FLUSH_EXPLICIT``
  Written ranges will be notified later with :ref:`transfer_flush_region`.
  Cannot be used with ``PIPE_TRANSFER_READ``.


Compute kernel execution
^^^^^^^^^^^^^^^^^^^^^^^^

A compute program can be defined, bound or destroyed using
``create_compute_state``, ``bind_compute_state`` or
``destroy_compute_state`` respectively.

Any of the subroutines contained within the compute program can be
executed on the device using the ``launch_grid`` method.  This method
will execute as many instances of the program as elements in the
specified N-dimensional grid, hopefully in parallel.

The compute program has access to four special resources:

* ``GLOBAL`` represents a memory space shared among all the threads
  running on the device.  An arbitrary buffer created with the
  ``PIPE_BIND_GLOBAL`` flag can be mapped into it using the
  ``set_global_binding`` method.

* ``LOCAL`` represents a memory space shared among all the threads
  running in the same working group.  The initial contents of this
  resource are undefined.

* ``PRIVATE`` represents a memory space local to a single thread.
  The initial contents of this resource are undefined.

* ``INPUT`` represents a read-only memory space that can be
  initialized at ``launch_grid`` time.

These resources use a byte-based addressing scheme, and they can be
accessed from the compute program by means of the LOAD/STORE TGSI
opcodes.  Additional resources to be accessed using the same opcodes
may be specified by the user with the ``set_compute_resources``
method.

In addition, normal texture sampling is allowed from the compute
program: ``bind_compute_sampler_states`` may be used to set up texture
samplers for the compute stage and ``set_compute_sampler_views`` may
be used to bind a number of sampler views to it.
