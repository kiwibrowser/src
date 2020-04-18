Resources and derived objects
=============================

Resources represent objects that hold data: textures and buffers.

They are mostly modelled after the resources in Direct3D 10/11, but with a
different transfer/update mechanism, and more features for OpenGL support.

Resources can be used in several ways, and it is required to specify all planned uses through an appropriate set of bind flags.

TODO: write much more on resources

Transfers
---------

Transfers are the mechanism used to access resources with the CPU.

OpenGL: OpenGL supports mapping buffers and has inline transfer functions for both buffers and textures

D3D11: D3D11 lacks transfers, but has special resource types that are mappable to the CPU address space

TODO: write much more on transfers

Resource targets
----------------

Resource targets determine the type of a resource.

Note that drivers may not actually have the restrictions listed regarding
coordinate normalization and wrap modes, and in fact efficient OpenCL
support will probably require drivers that don't have any of them, which
will probably be advertised with an appropriate cap.

TODO: document all targets. Note that both 3D and cube have restrictions
that depend on the hardware generation.

TODO: can buffers have a non-R8 format?

PIPE_BUFFER
^^^^^^^^^^^

Buffer resource: can be used as a vertex, index, constant buffer (appropriate bind flags must be requested).

They can be bound to stream output if supported.
TODO: what about the restrictions lifted by the several later GL transform feedback extensions? How does one advertise that in Gallium?

They can be also be bound to a shader stage as usual.
TODO: are all drivers supposed to support this? how does this work exactly? are there size limits?

They can be also be bound to the framebuffer as usual.
TODO: are all drivers supposed to support this? how does this work exactly? are there size limits?
TODO: is there any chance of supporting GL pixel buffer object acceleration with this?

- depth0 must be 1
- last_level must be 0
- TODO: what about normalization?
- TODO: wrap modes/other sampling state?
- TODO: are arbitrary formats supported? in which cases?

OpenGL: vertex buffers in GL 1.5 or GL_ARB_vertex_buffer_object

- Binding to stream out requires GL 3.0 or GL_NV_transform_feedback
- Binding as constant buffers requires GL 3.1 or GL_ARB_uniform_buffer_object
- Binding to a sampling stage requires GL 3.1 or GL_ARB_texture_buffer_object
- TODO: can they be bound to an FBO?

D3D11: buffer resources
- Binding to a render target requires D3D_FEATURE_LEVEL_10_0

PIPE_TEXTURE_1D
^^^^^^^^^^^^^^^
1D surface accessed with normalized coordinates.

UNIMPLEMENTED: 1D texture arrays not supported

- If PIPE_CAP_NPOT_TEXTURES is not supported,
      width must be a power of two
- height0 must be 1
- depth0 must be 1
- Mipmaps can be used
- Must use normalized coordinates

OpenGL: GL_TEXTURE_1D in GL 1.0

- PIPE_CAP_NPOT_TEXTURES is equivalent to GL 2.0 or GL_ARB_texture_non_power_of_two

D3D11: 1D textures in D3D_FEATURE_LEVEL_10_0

PIPE_TEXTURE_RECT
^^^^^^^^^^^^^^^^^
2D surface with OpenGL GL_TEXTURE_RECTANGLE semantics.

- depth0 must be 1
- last_level must be 0
- Must use unnormalized coordinates
- Must use a clamp wrap mode

OpenGL: GL_TEXTURE_RECTANGLE in GL 3.1 or GL_ARB_texture_rectangle or GL_NV_texture_rectangle

OpenCL: can create OpenCL images based on this, that can then be sampled arbitrarily

D3D11: not supported (only PIPE_TEXTURE_2D with normalized coordinates is supported)

PIPE_TEXTURE_2D
^^^^^^^^^^^^^^^
2D surface accessed with normalized coordinates.

UNIMPLEMENTED: 2D texture arrays not supported

- If PIPE_CAP_NPOT_TEXTURES is not supported,
      width and height must be powers of two
- depth0 must be 1
- Mipmaps can be used
- Must use normalized coordinates
- No special restrictions on wrap modes

OpenGL: GL_TEXTURE_2D in GL 1.0

- PIPE_CAP_NPOT_TEXTURES is equivalent to GL 2.0 or GL_ARB_texture_non_power_of_two

OpenCL: can create OpenCL images based on this, that can then be sampled arbitrarily

D3D11: 2D textures

- PIPE_CAP_NPOT_TEXTURES is equivalent to D3D_FEATURE_LEVEL_9_3

PIPE_TEXTURE_3D
^^^^^^^^^^^^^^^

3-dimensional array of texels.
Mipmap dimensions are reduced in all 3 coordinates.

- If PIPE_CAP_NPOT_TEXTURES is not supported,
      width, height and depth must be powers of two
- Must use normalized coordinates

OpenGL: GL_TEXTURE_3D in GL 1.2 or GL_EXT_texture3D

- PIPE_CAP_NPOT_TEXTURES is equivalent to GL 2.0 or GL_ARB_texture_non_power_of_two

D3D11: 3D textures

- PIPE_CAP_NPOT_TEXTURES is equivalent to D3D_FEATURE_LEVEL_10_0

PIPE_TEXTURE_CUBE
^^^^^^^^^^^^^^^^^

Cube maps consist of 6 2D faces.
The 6 surfaces form an imaginary cube, and sampling happens by mapping an
input 3-vector to the point of the cube surface in that direction.

Sampling may be optionally seamless, resulting in filtering taking samples
from multiple surfaces near to the edge.
UNIMPLEMENTED: seamless cube map sampling not supported

UNIMPLEMENTED: cube map arrays not supported

- Width and height must be equal
- If PIPE_CAP_NPOT_TEXTURES is not supported,
      width and height must be powers of two
- Must use normalized coordinates

OpenGL: GL_TEXTURE_CUBE_MAP in GL 1.3 or EXT_texture_cube_map

- PIPE_CAP_NPOT_TEXTURES is equivalent to GL 2.0 or GL_ARB_texture_non_power_of_two
- Seamless cube maps require GL 3.2 or GL_ARB_seamless_cube_map or GL_AMD_seamless_cubemap_per_texture
- Cube map arrays require GL 4.0 or GL_ARB_texture_cube_map_array

D3D11: 2D array textures with the D3D11_RESOURCE_MISC_TEXTURECUBE flag

- PIPE_CAP_NPOT_TEXTURES is equivalent to D3D_FEATURE_LEVEL_10_0
- Cube map arrays require D3D_FEATURE_LEVEL_10_1
- TODO: are (non)seamless cube maps supported in D3D11? how?

Surfaces
--------

Surfaces are views of a resource that can be bound as a framebuffer to serve as the render target or depth buffer.

TODO: write much more on surfaces

OpenGL: FBOs are collections of surfaces in GL 3.0 or GL_ARB_framebuffer_object

D3D11: render target views and depth/stencil views

Sampler views
-------------

Sampler views are views of a resource that can be bound to a pipeline stage to be sampled from shaders.

TODO: write much more on sampler views

OpenGL: texture objects are actually sampler view and resource in a single unit

D3D11: shader resource views
