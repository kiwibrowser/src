/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/*!
 * \file  esextcTextureCubeMapArraySampling.cpp
 * \brief Texture Cube Map Array Sampling (Test 1)
 */ /*-------------------------------------------------------------------*/

/* Control logging of positive results. 0 disabled, 1 enabled */
#define TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_PASS_LOG 0

/* Control logging of program source for positive results. 0 disabled, 1 enabled */
#define TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_PASS_PROGRAM_LOG 1

/* Control logging of negative results. 0 disabled, 1 enabled */
#define TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG 1

/* Control logging of program source for negative results. 0 disabled, 1 enabled */
#define TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_PROGRAM_LOG 1

/* When enabled, textures will be stored as TGA files. Test will not be executed. 0 disabled, 1 enabled */
#define TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION 0

/* Output path for TGA files */
#define TEXTURECUBEMAPARRAYSAMPLINGTEST_PATH_FOR_COMPRESSION "c:\\textures\\"

#include "esextcTextureCubeMapArraySampling.hpp"
#include "esextcTextureCubeMapArraySamplingResources.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <cmath>
#include <sstream>
#include <vector>

#if TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION
#include <fstream>
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION */

namespace glcts
{
/** Structure used to write shaders' variables to stream
 *
 **/
struct var2str
{
public:
	/** Constructor. Stores strings used to create variable name
	 *  prefixName[index]
	 *
	 *  @param prefix Prefix part. Can be null.
	 *  @param name   Name part. Must not be null.
	 *  @param index  Index part. Can be null.
	 **/
	var2str(const glw::GLchar* prefix, const glw::GLchar* name, const glw::GLchar* index)
		: m_prefix(prefix), m_name(name), m_index(index)
	{
		/* Nothing to be done here */
	}

	const glw::GLchar* m_prefix;
	const glw::GLchar* m_name;
	const glw::GLchar* m_index;
};

/* Attribute names */
const glw::GLchar* const TextureCubeMapArraySamplingTest::attribute_grad_x			   = "grad_x";
const glw::GLchar* const TextureCubeMapArraySamplingTest::attribute_grad_y			   = "grad_y";
const glw::GLchar* const TextureCubeMapArraySamplingTest::attribute_lod				   = "lod";
const glw::GLchar* const TextureCubeMapArraySamplingTest::attribute_refZ			   = "refZ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::attribute_texture_coordinate = "texture_coordinates";

/* Compute shader parts */
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_body =
	"void main()\n"
	"{\n"
	"    const int face_width         = 3;\n"
	"    const int face_height        = 3;\n"
	"    const int vertices_per_face  = face_width * face_height;\n"
	"    const int faces_per_layer    = 6;\n"
	"    const int layer_width        = faces_per_layer * face_width;\n"
	"    const int vertices_per_layer = vertices_per_face * faces_per_layer;\n"
	"\n"
	"    ivec2 image_coord            = ivec2(gl_WorkGroupID.xy);\n"
	"    ivec3 texture_size           = textureSize(sampler, 0);\n"
	"\n"
	"    int layer                    = image_coord.x / layer_width;\n"
	"    int layer_offset             = layer * layer_width;\n"
	"    int layer_index              = layer * vertices_per_layer;\n"
	"    int face                     = (image_coord.x - layer_offset) / face_width;\n"
	"    int face_offset              = face * face_width;\n"
	"    int face_index               = face * vertices_per_face;\n"
	"    int vertex                   = image_coord.x - layer_offset - face_offset;\n"
	"    int vertex_index             = layer_index + face_index + vertex + (face_height - image_coord.y - 1) * "
	"face_width;\n"
	"\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_layout_binding = "layout(std430, binding=";
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_buffer			= ") buffer ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_color			= "color";
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_image_store =
	"    imageStore(image, image_coord, ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_layout =
	"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::compute_shader_param = "cs_";

/* Fragment shader parts */
const glw::GLchar* const TextureCubeMapArraySamplingTest::fragment_shader_input  = "fs_in_color";
const glw::GLchar* const TextureCubeMapArraySamplingTest::fragment_shader_output = "fs_out_color";
const glw::GLchar* const TextureCubeMapArraySamplingTest::fragment_shader_pass_through_body_code =
	"void main()\n"
	"{\n"
	"    fs_out_color = fs_in_color;\n"
	"}\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::fragment_shader_sampling_body_code = "void main()\n"
																							   "{\n";

/* Geometry shader parts */
const glw::GLchar* const TextureCubeMapArraySamplingTest::geometry_shader_emit_vertex_code = "    EmitVertex();\n"
																							 "}\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::geometry_shader_extension = "${GEOMETRY_SHADER_REQUIRE}\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::geometry_shader_layout =
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::geometry_shader_sampling_body_code =
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position;\n";

/* Image types and name */
const glw::GLchar* const TextureCubeMapArraySamplingTest::image_float = "image2D ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::image_int   = "iimage2D ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::image_name  = "image";
const glw::GLchar* const TextureCubeMapArraySamplingTest::image_uint  = "uimage2D ";

/* Interpolation */
const glw::GLchar* const TextureCubeMapArraySamplingTest::interpolation_flat = "flat ";

/* Sampler types and name */
const glw::GLchar* const TextureCubeMapArraySamplingTest::sampler_depth = "samplerCubeArrayShadow ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::sampler_float = "samplerCubeArray ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::sampler_int   = "isamplerCubeArray ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::sampler_name  = "sampler";
const glw::GLchar* const TextureCubeMapArraySamplingTest::sampler_uint  = "usamplerCubeArray ";

/* Common shader parts for */
const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_code_preamble = "${VERSION}\n"
																				 "${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
																				 "\n";

const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_precision = "precision highp float;\n";

const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_input	 = "in ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_layout	= "layout(location = 0) ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_output	= "out ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_uniform   = "uniform ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::shader_writeonly = "writeonly ";

/* Tesselation control shader parts */
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_control_shader_layout =
	"layout(vertices = 1) out;\n"
	"\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_control_shader_sampling_body_code =
	"void main()\n"
	"{\n"
	"    gl_TessLevelInner[0] = 1.0;\n"
	"    gl_TessLevelInner[1] = 1.0;\n"
	"    gl_TessLevelOuter[0] = 1.0;\n"
	"    gl_TessLevelOuter[1] = 1.0;\n"
	"    gl_TessLevelOuter[2] = 1.0;\n"
	"    gl_TessLevelOuter[3] = 1.0;\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_control_shader_output = "tcs_out_";

/* Tesselation evaluation shader parts */
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_evaluation_shader_input = "tes_in_color";
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_evaluation_shader_layout =
	"layout(isolines, point_mode) in;\n"
	"\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_evaluation_shader_pass_through_body_code =
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position;\n"
	"    fs_in_color = tes_in_color[0];\n"
	"}\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_evaluation_shader_sampling_body_code =
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position;\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::tesselation_shader_extension =
	"${TESSELLATION_SHADER_REQUIRE}\n";

/* Texture sampling routines */
const glw::GLchar* const TextureCubeMapArraySamplingTest::texture_func		 = "texture";
const glw::GLchar* const TextureCubeMapArraySamplingTest::textureGather_func = "textureGather";
const glw::GLchar* const TextureCubeMapArraySamplingTest::textureGrad_func   = "textureGrad";
const glw::GLchar* const TextureCubeMapArraySamplingTest::textureLod_func	= "textureLod";

/* Data types */
const glw::GLchar* const TextureCubeMapArraySamplingTest::type_float = "float ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::type_ivec4 = "ivec4 ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::type_uint  = "uint ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::type_uvec4 = "uvec4 ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::type_vec3  = "vec3 ";
const glw::GLchar* const TextureCubeMapArraySamplingTest::type_vec4  = "vec4 ";

/* Vertex shader parts */
const glw::GLchar* const TextureCubeMapArraySamplingTest::vertex_shader_body_code =
	"void main()\n"
	"{\n"
	"    gl_PointSize = 1.0f;\n"
	"    gl_Position = vs_in_position;\n";
const glw::GLchar* const TextureCubeMapArraySamplingTest::vertex_shader_input	= "vs_in_";
const glw::GLchar* const TextureCubeMapArraySamplingTest::vertex_shader_output   = "vs_out_";
const glw::GLchar* const TextureCubeMapArraySamplingTest::vertex_shader_position = "position";

/* Static constants */
const glw::GLuint TextureCubeMapArraySamplingTest::m_get_type_api_status_program_resource						 = 0x02;
const glw::GLuint TextureCubeMapArraySamplingTest::m_get_type_api_status_uniform								 = 0x01;
const glw::GLuint TextureCubeMapArraySamplingTest::bufferDefinition::m_invalid_buffer_object_id					 = -1;
const glw::GLuint TextureCubeMapArraySamplingTest::programDefinition::m_invalid_program_object_id				 = 0;
const glw::GLuint TextureCubeMapArraySamplingTest::shaderDefinition::m_invalid_shader_object_id					 = 0;
const glw::GLuint TextureCubeMapArraySamplingTest::textureDefinition::m_invalid_texture_object_id				 = -1;
const glw::GLuint TextureCubeMapArraySamplingTest::textureDefinition::m_invalid_uniform_location				 = -1;
const glw::GLuint TextureCubeMapArraySamplingTest::vertexArrayObjectDefinition::m_invalid_attribute_location	 = -1;
const glw::GLuint TextureCubeMapArraySamplingTest::vertexArrayObjectDefinition::m_invalid_vertex_array_object_id = -1;

/* Functions */

/** Fill image with specified color
 *  @tparam T               Image component type
 *  @tparam N_Components    Number of image components
 *
 *  @param image_width      Width of image
 *  @param image_height     Height of image
 *  @param pixel_components Image will be filled with that color
 *  @param out_data         Image data, storage must be allocated
 **/
template <typename T, unsigned int N_Components>
void fillImage(glw::GLsizei image_width, glw::GLsizei image_height, const T* pixel_components, T* out_data)
{
	const glw::GLuint n_components_per_pixel = N_Components;
	const glw::GLuint n_components_per_line  = n_components_per_pixel * image_width;

	for (glw::GLsizei y = 0; y < image_height; ++y)
	{
		const glw::GLuint line_offset = y * n_components_per_line;

		for (glw::GLsizei x = 0; x < image_width; ++x)
		{
			for (glw::GLuint component = 0; component < n_components_per_pixel; ++component)
			{
				out_data[line_offset + x * n_components_per_pixel + component] = pixel_components[component];
			}
		}
	}
}

/* Out of alphabetical order due to use in other functions */
/** Normalize vector stored in array. Only first N_NormalizedComponents will be normalized.
 *
 *  @tparam N_NormalizedComponents Number of coordinates to normalize
 *  @tparam N_Components           Number of coordinates in vector
 *
 *  @param data                    Pointer to first coordinate of first vector in array
 *  @param index                   Index of vector to be normalized
 **/
template <unsigned int N_NormalizedComponents, unsigned int N_Components>
void vectorNormalize(glw::GLfloat* data, glw::GLuint index)
{
	glw::GLfloat* components = data + index * N_Components;

	glw::GLfloat sqr_length = 0.0f;

	for (glw::GLuint i = 0; i < N_NormalizedComponents; ++i)
	{
		const glw::GLfloat component = components[i];

		sqr_length += component * component;
	}

	const glw::GLfloat length = sqrtf(sqr_length);
	const glw::GLfloat factor = 1.0f / length;

	for (glw::GLuint i = 0; i < N_NormalizedComponents; ++i)
	{
		components[i] *= factor;
	}
}

/* Out of alphabetical order due to use in other functions */
/** Set coordinates of 4 element vector stored in array
 *
 *  @param data  Pointer to first coordinate of first vector in array
 *  @param index Index of vector to be normalized
 *  @param x     1st coordinate value
 *  @param y     2nd coordinate value
 *  @param z     3rd coordinate value
 *  @param w     4th coordinate value
 **/
void vectorSet4(glw::GLfloat* data, glw::GLuint index, glw::GLfloat x, glw::GLfloat y, glw::GLfloat z, glw::GLfloat w)
{
	const glw::GLuint n_components_per_vertex = 4;
	const glw::GLuint vector_offset			  = n_components_per_vertex * index;

	data[vector_offset + 0] = x;
	data[vector_offset + 1] = y;
	data[vector_offset + 2] = z;
	data[vector_offset + 3] = w;
}

/* Out of alphabetical order due to use in other functions */
/** Subtract vectors: a = b - a
 *
 *  @tparam N_Components Number of coordinates in vector
 *
 *  @param a             Pointer to vector a
 *  @param b             Pointer to vector b
 **/
template <unsigned int N_Components>
void vectorSubtractInPlace(glw::GLfloat* a, const glw::GLfloat* b)
{
	const glw::GLuint n_components = N_Components;

	for (glw::GLuint i = 0; i < n_components; ++i)
	{
		a[i] -= b[i];
	}
}

/** Prepare color for rgba float textures
 *
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param mipmap_level    Mipmap level
 *  @param n_elements      Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_data        Pointer to components storage
 **/
void getColorFloatComponents(glw::GLint cube_face, glw::GLint element_index, glw::GLint mipmap_level,
							 glw::GLint n_elements, glw::GLint n_mipmap_levels, glw::GLfloat* out_data)
{
	static const glw::GLfloat n_faces = 6.0f;

	out_data[0] = ((glw::GLfloat)(mipmap_level + 1)) / ((glw::GLfloat)n_mipmap_levels);
	out_data[1] = ((glw::GLfloat)(cube_face + 1)) / n_faces;
	out_data[2] = ((glw::GLfloat)(element_index + 1)) / ((glw::GLfloat)n_elements);
	out_data[3] = 1.0f / 4.0f;
}

/** Prepare color for rgba integer textures
 *
 *  @tparam T             Type of components
 *
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param mipmap_level    Mipmap level
 *  @param n_elements      Number of elements in array, ignored
 *  @param n_mipmap_levels Number of mipmap levels, ignored
 *  @param out_data        Pointer to components storage
 **/
template <typename T>
void getColorIntComponents(glw::GLint cube_face, glw::GLint element_index, glw::GLint		 mipmap_level,
						   glw::GLint /* n_elements */, glw::GLint /* n_mipmap_levels */, T* out_data)
{
	out_data[0] = static_cast<T>(mipmap_level + 1);
	out_data[1] = static_cast<T>(cube_face + 1);
	out_data[2] = static_cast<T>(element_index + 1);
	out_data[3] = 1;
}

/** Prepare color for rgba compressed textures
 *
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param mipmap_level    Mipmap level
 *  @param n_elements      Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_data        Pointer to components storage
 **/
void getCompressedColorUByteComponents(glw::GLint cube_face, glw::GLint element_index, glw::GLint mipmap_level,
									   glw::GLint n_elements, glw::GLint n_mipmap_levels, glw::GLubyte* out_data)
{
	(void)n_mipmap_levels;

	static const glw::GLuint n_faces = 6;

	const glw::GLuint  n_faces_per_level = n_elements * n_faces;
	const glw::GLubyte value =
		static_cast<glw::GLubyte>(mipmap_level * n_faces_per_level + element_index * n_faces + cube_face + 1);

	out_data[0] = value;
	out_data[1] = value;
	out_data[2] = value;
	out_data[3] = value;
}

/** Get compressed texture data and size from resources. Width, height, number of array elements and mipmap level are used to identify image.
 *  Width and height are dimmensions of base level image, same values are used to identify all mipmap levels.
 *
 *  @param width            Width of texture
 *  @param height           Height of texture
 *  @param n_array_elements Number of elemnts in array
 *  @param mipmap_level     Level
 *  @param out_image_data   Image data
 *  @param out_image_size   Image size
 **/
void getCompressedTexture(glw::GLuint width, glw::GLuint height, glw::GLuint n_array_elements, glw::GLuint mipmap_level,
						  const glw::GLubyte*& out_image_data, glw::GLuint& out_image_size)
{
	for (glw::GLuint i = 0; i < n_compressed_images; ++i)
	{
		const compressedImage& image = compressed_images[i];

		if ((image.width == width) && (image.height == height) && (image.length == n_array_elements) &&
			(image.level == mipmap_level))
		{
			out_image_data = image.image_data;
			out_image_size = image.image_size;

			return;
		}
	}

	out_image_data = 0;
	out_image_size = 0;
}

/** Prepare color for depth textures
 *
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param mipmap_level    Mipmap level
 *  @param n_elements      Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_depth       Depth value
 **/
void getDepthComponent(glw::GLint cube_face, glw::GLint element_index, glw::GLint mipmap_level, glw::GLint n_elements,
					   glw::GLint n_mipmap_levels, glw::GLfloat& out_depth)
{
	static const glw::GLuint n_faces = 6;

	out_depth = ((glw::GLfloat)(mipmap_level + 1 + cube_face + 1 + element_index + 1)) /
				((glw::GLfloat)(n_mipmap_levels + n_faces + n_elements));
}

/** Get expected color sampled by texture or textureGather from rgba float textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>. Ignored.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
void getExpectedColorFloatComponentsForTexture(glw::GLuint /* pixel_index */, glw::GLint cube_face,
											   glw::GLint element_index, glw::GLint n_layers,
											   glw::GLint n_mipmap_levels, glw::GLfloat* out_components)
{
	getColorFloatComponents(cube_face, element_index, 0 /* mipmap_level */, n_layers, n_mipmap_levels, out_components);
}

/** Get expected color sampled by textureLod or textureGrad from rgba float textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
void getExpectedColorFloatComponentsForTextureLod(glw::GLuint pixel_index, glw::GLint cube_face,
												  glw::GLint element_index, glw::GLint n_layers,
												  glw::GLint n_mipmap_levels, glw::GLfloat* out_components)
{
	glw::GLint mipmap_level = 0;

	if (1 == pixel_index % 2)
	{
		mipmap_level = n_mipmap_levels - 1;
	}

	getColorFloatComponents(cube_face, element_index, mipmap_level, n_layers, n_mipmap_levels, out_components);
}

/** Get expected color sampled by texture or textureGather from rgba integer textures
 *
 *  @tparam T              Type of image components
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>. Ignored.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
template <typename T>
void getExpectedColorIntComponentsForTexture(glw::GLuint /* pixel_index */, glw::GLint cube_face,
											 glw::GLint element_index, glw::GLint n_layers, glw::GLint n_mipmap_levels,
											 T* out_components)
{
	getColorIntComponents<T>(cube_face, element_index, 0 /* mipmap_level */, n_layers, n_mipmap_levels, out_components);
}

/** Get expected color sampled by textureLod or textureGrad from rgba integer textures
 *
 *  @tparam T              Type of image components
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
template <typename T>
void getExpectedColorIntComponentsForTextureLod(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint element_index,
												glw::GLint n_layers, glw::GLint n_mipmap_levels, T* out_components)
{
	glw::GLint mipmap_level = 0;

	if (1 == pixel_index % 2)
	{
		mipmap_level = n_mipmap_levels - 1;
	}

	getColorIntComponents<T>(cube_face, element_index, mipmap_level, n_layers, n_mipmap_levels, out_components);
}

/** Get expected color sampled by texture or textureGather from rgba compressed textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>. Ignored.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
void getExpectedCompressedComponentsForTexture(glw::GLuint /* pixel_index */, glw::GLint cube_face,
											   glw::GLint element_index, glw::GLint n_layers,
											   glw::GLint n_mipmap_levels, glw::GLubyte* out_components)
{
	getCompressedColorUByteComponents(cube_face, element_index, 0 /* mipmap_level */, n_layers, n_mipmap_levels,
									  out_components);
}

/** Get expected color sampled by textureLod or textureGrad from rgba compressed textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
void getExpectedCompressedComponentsForTextureLod(glw::GLuint pixel_index, glw::GLint cube_face,
												  glw::GLint element_index, glw::GLint n_layers,
												  glw::GLint n_mipmap_levels, glw::GLubyte* out_components)
{
	glw::GLint mipmap_level = 0;

	if (1 == pixel_index % 2)
	{
		mipmap_level = n_mipmap_levels - 1;
	}

	getCompressedColorUByteComponents(cube_face, element_index, mipmap_level, n_layers, n_mipmap_levels,
									  out_components);
}

/** Get expected color sampled by texture or textureGather from depth textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>
 *  @param cube_face       Index of cube's face. Ignored.
 *  @param element_index   Index of element in array. Ignored.
 *  @param n_layers        Number of elements in array. Ignored.
 *  @param n_mipmap_levels Number of mipmap levels. Ignored.
 *  @param out_components  Pointer to components storage
 **/
void getExpectedDepthComponentsForTexture(glw::GLuint pixel_index, glw::GLint /* cube_face */,
										  glw::GLint /* element_index */, glw::GLint /* n_layers */,
										  glw::GLint /* n_mipmap_levels */, glw::GLfloat* out_components)
{
	const glw::GLfloat results[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

	out_components[0] = results[pixel_index];
}

/** Get expected color sampled by textureLod or textureGrad from depth textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>.
 *  @param cube_face       Index of cube's face. Ignored.
 *  @param element_index   Index of element in array. Ignored.
 *  @param n_layers        Number of elements in array. Ignored.
 *  @param n_mipmap_levels Number of mipmap levels. Ignored.
 *  @param out_components  Pointer to components storage
 **/
void getExpectedDepthComponentsForTextureLod(glw::GLuint pixel_index, glw::GLint /* cube_face */,
											 glw::GLint /* element_index */, glw::GLint /* n_layers */,
											 glw::GLint /* n_mipmap_levels */, glw::GLfloat* out_components)
{
	if (0 == pixel_index % 2)
	{
		out_components[0] = 0.0f;
	}
	else
	{
		out_components[0] = 1.0f;
	}
}

/* Out of alphabetical order due to use in other functions */
/** Prepare color for stencil textures
 *
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param mipmap_level    Mipmap level
 *  @param n_elements      Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_stencil     Stencil value
 **/
void getStencilComponent(glw::GLint cube_face, glw::GLint element_index, glw::GLint mipmap_level, glw::GLint n_elements,
						 glw::GLint n_mipmap_levels, glw::GLubyte& out_stencil)
{
	static const glw::GLint n_faces = 6;

	out_stencil = (glw::GLubyte)((mipmap_level + 1 + cube_face + 1 + element_index + 1) * 255 /
								 (n_mipmap_levels + n_faces + n_elements));
}

/** Get expected color sampled by texture or textureGather from stencil textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>. Ignored.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
void getExpectedStencilComponentsForTexture(glw::GLuint /* pixel_index */, glw::GLint cube_face,
											glw::GLint element_index, glw::GLint n_layers, glw::GLint n_mipmap_levels,
											glw::GLuint* out_components)
{
	glw::GLubyte value = 0;

	getStencilComponent(cube_face, element_index, 0 /* mipmap_level */, n_layers, n_mipmap_levels, value);

	out_components[0] = value;
}

/** Get expected color sampled by textureLod or textureGrad from stencil textures
 *
 *  @param pixel_index     Index of pixel, identifies pixel at face <0:8> <left-top:rigth-bottom>.
 *  @param cube_face       Index of cube's face
 *  @param element_index   Index of element in array
 *  @param n_layers        Number of elements in array
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param out_components  Pointer to components storage
 **/
void getExpectedStencilComponentsForTextureLod(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint element_index,
											   glw::GLint n_layers, glw::GLint n_mipmap_levels,
											   glw::GLuint* out_components)
{
	glw::GLubyte value		  = 0;
	glw::GLint   mipmap_level = 0;

	if (1 == pixel_index % 2)
	{
		mipmap_level = n_mipmap_levels - 1;
	}

	getStencilComponent(cube_face, element_index, mipmap_level, n_layers, n_mipmap_levels, value);

	out_components[0] = value;
}

/** Returns number of mipmaps for given image dimmensions
 *
 *  @param texture_width  Width of image
 *  @param texture_height Height of image
 *
 *  @returns Number of mipmaps
 **/
glw::GLubyte getMipmapLevelCount(glw::GLsizei texture_width, glw::GLsizei texture_height)
{
	glw::GLsizei size  = de::max(texture_width, texture_height);
	glw::GLuint  count = 1;

	while (1 < size)
	{
		size /= 2;
		count += 1;
	}

	return (glw::GLubyte)count;
}

/** Calculate texture coordinates for "right neighbour" of given texture coordinates
 *
 *  @param texture_coordinates Texture coordinates of original point
 *  @param face                Cube map's face index
 *  @param offset              Offset of "neighbour" in "right" direction
 *  @param width               Image width
 *  @param out_neighbour       Texture coordinates of "neighbour" point
 **/
void getRightNeighbour(const glw::GLfloat* texture_coordinates, glw::GLuint face, glw::GLuint offset, glw::GLuint width,
					   glw::GLfloat* out_neighbour)
{
	const glw::GLfloat step = (float)offset / (float)width;

	glw::GLfloat& x = out_neighbour[0];
	glw::GLfloat& y = out_neighbour[1];
	glw::GLfloat& z = out_neighbour[2];

	const glw::GLfloat coord_x = texture_coordinates[0];
	const glw::GLfloat coord_y = texture_coordinates[1];
	const glw::GLfloat coord_z = texture_coordinates[2];

	switch (face)
	{
	case 0: // +X
		x = coord_x;
		y = coord_y - step;
		z = coord_z;
		break;
	case 1: // -X
		x = coord_x;
		y = coord_y + step;
		z = coord_z;
		break;
	case 2: // +Y
		x = coord_x + step;
		y = coord_y;
		z = coord_z;
		break;
	case 3: // -Y
		x = coord_x - step;
		y = coord_y;
		z = coord_z;
		break;
	case 4: // +Z
		x = coord_x + step;
		y = coord_y;
		z = coord_z;
		break;
	case 5: // -Z
		x = coord_x - step;
		y = coord_y;
		z = coord_z;
		break;
	}
}

/** Calculate texture coordinates for "top neighbour" of given texture coordinates
 *
 *  @param texture_coordinates Texture coordinates of original point
 *  @param face                Cube map's face index
 *  @param offset              Offset of "neighbour" in "top" direction
 *  @param width               Image width
 *  @param out_neighbour       Texture coordinates of "neighbour" point
 **/
void getTopNeighbour(const glw::GLfloat* texture_coordinates, glw::GLuint face, glw::GLuint offset, glw::GLuint width,
					 glw::GLfloat* out_neighbour)
{
	glw::GLfloat step = (float)offset / (float)width;

	glw::GLfloat& x = out_neighbour[0];
	glw::GLfloat& y = out_neighbour[1];
	glw::GLfloat& z = out_neighbour[2];

	const glw::GLfloat coord_x = texture_coordinates[0];
	const glw::GLfloat coord_y = texture_coordinates[1];
	const glw::GLfloat coord_z = texture_coordinates[2];

	switch (face)
	{
	case 0: // +X
		x = coord_x;
		y = coord_y;
		z = coord_z + step;
		break;
	case 1: // -X
		x = coord_x;
		y = coord_y;
		z = coord_z + step;
		break;
	case 2: // +Y
		x = coord_x;
		y = coord_y;
		z = coord_z + step;
		break;
	case 3: // -Y
		x = coord_x;
		y = coord_y;
		z = coord_z + step;
		break;
	case 4: // +Z
		x = coord_x;
		y = coord_y - step;
		z = coord_z;
		break;
	case 5: // -Z
		x = coord_x;
		y = coord_y + step;
		z = coord_z;
		break;
	}
}

/** Write var2str instance to output stream
 *
 *  @param stream Stream instance
 *  @param var    var2str instance
 *
 *  @returns Stream instance
 **/
std::ostream& operator<<(std::ostream& stream, const var2str& var)
{
	if (0 != var.m_prefix)
	{
		stream << var.m_prefix;
	}

	stream << var.m_name;

	if (0 != var.m_index)
	{
		stream << "[" << var.m_index << "]";
	}

	return stream;
}

/* Out of alphabetical order due to use in other functions */
/** Fill texture's face at given index and level with given color
 *
 *  @tparam T                 Type of image component
 *  @tparam N_Components      Number of components
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 *  @param components         Color used to fill texture
 **/
template <typename T, unsigned int N_Components>
void prepareDataForTexture(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
						   glw::GLint mipmap_level, glw::GLenum texture_format, glw::GLenum texture_type,
						   glw::GLsizei texture_width, glw::GLsizei texture_height, const T* components)
{
	static const glw::GLuint n_components_per_pixel = N_Components;

	const glw::GLuint n_pixels			  = texture_width * texture_height;
	const glw::GLuint n_total_componenets = n_components_per_pixel * n_pixels;
	const glw::GLuint z_offset			  = element_index * 6 + cube_face;

	std::vector<T> texture_data;
	texture_data.resize(n_total_componenets);

	fillImage<T, N_Components>(texture_width, texture_height, components, &texture_data[0]);

	gl.texSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, 0 /* x */, 0 /* y */, z_offset, texture_width,
					 texture_height, 1 /* depth */, texture_format, texture_type, &texture_data[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to update texture data");
}

/** Prepare texture's face at given index and level, for rgba float textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareDataForColorFloatTexture(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
									 glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
									 glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
									 glw::GLsizei texture_height)
{
	glw::GLfloat components[4];

	getColorFloatComponents(cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels, components);

	prepareDataForTexture<glw::GLfloat, 4>(gl, cube_face, element_index, mipmap_level, texture_format, texture_type,
										   texture_width, texture_height, components);
}

/** Prepare texture's face at given index and level, for rgba integer textures.
 *
 *  @tparam T                 Type of image component
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
template <typename T>
void prepareDataForColorIntTexture(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
								   glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
								   glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
								   glw::GLsizei texture_height)
{
	T components[4];

	getColorIntComponents<T>(cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels, components);

	prepareDataForTexture<T, 4>(gl, cube_face, element_index, mipmap_level, texture_format, texture_type, texture_width,
								texture_height, components);
}

/** Prepare texture's face at given index and level, for depth textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareDataForDepthFloatTexture(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
									 glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
									 glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
									 glw::GLsizei texture_height)
{
	glw::GLfloat component = 0;

	getDepthComponent(cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels, component);

	prepareDataForTexture<glw::GLfloat, 1>(gl, cube_face, element_index, mipmap_level, texture_format, texture_type,
										   texture_width, texture_height, &component);
}

/** Prepare texture's face at given index and level, for stencil textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareDataForStencilUIntTexture(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
									  glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
									  glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
									  glw::GLsizei texture_height)
{
	glw::GLubyte component = 0;

	getStencilComponent(cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels, component);

	prepareDataForTexture<glw::GLubyte, 1>(gl, cube_face, element_index, mipmap_level, texture_format, texture_type,
										   texture_width, texture_height, &component);
}

/** Prepare texture's face at given index and level, for depth textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareDepthTextureFace(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
							 glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
							 glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
							 glw::GLsizei texture_height)
{
	switch (texture_type)
	{
	case GL_FLOAT:
		prepareDataForDepthFloatTexture(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels,
										texture_format, texture_type, texture_width, texture_height);
		break;

	default:
		TCU_FAIL("Not implemented case !");
		break;
	}
}

/** Prepare grad_x vector for given texture_coordinates
 *
 *  @param grad_x              Storage for grad_x
 *  @param face                Cube map's face index
 *  @param texture_coordinates Texture coordinate
 *  @param width               Image width
 **/
void prepareGradXForFace(glw::GLfloat* grad_x, glw::GLuint face, glw::GLfloat* texture_coordinates, glw::GLuint width)
{
	static const glw::GLuint n_points_per_face				  = 9;
	static const glw::GLuint n_texture_coordinates_components = 4;
	static const glw::GLuint n_grad_components				  = 4;

	for (glw::GLuint i = 0; i < n_points_per_face; i += 2)
	{
		const glw::GLuint texture_coordinates_offset = i * n_texture_coordinates_components;
		const glw::GLuint grad_offset				 = i * n_grad_components;

		getRightNeighbour(texture_coordinates + texture_coordinates_offset, face, 1, width, grad_x + grad_offset);
	}

	for (glw::GLuint i = 1; i < n_points_per_face; i += 2)
	{
		const glw::GLuint texture_coordinates_offset = i * n_texture_coordinates_components;
		const glw::GLuint grad_offset				 = i * n_grad_components;

		getRightNeighbour(texture_coordinates + texture_coordinates_offset, face, 4 * width, width,
						  grad_x + grad_offset);
	}

	for (glw::GLuint i = 0; i < n_points_per_face; ++i)
	{
		const glw::GLuint texture_coordinates_offset = i * n_texture_coordinates_components;
		const glw::GLuint grad_offset				 = i * n_grad_components;

		vectorSubtractInPlace<3>(grad_x + grad_offset, texture_coordinates + texture_coordinates_offset);
	}
}

/** Prepare grad_y vector for given texture_coordinates
 *
 *  @param grad_y              Storage for grad_x
 *  @param face                Cube map's face index
 *  @param texture_coordinates Texture coordinate
 *  @param width               Image width
 **/
void prepareGradYForFace(glw::GLfloat* grad_y, glw::GLuint face, glw::GLfloat* texture_coordinates, glw::GLuint width)
{
	static const glw::GLuint n_points_per_face				  = 9;
	static const glw::GLuint n_texture_coordinates_components = 4;
	static const glw::GLuint n_grad_components				  = 4;

	for (glw::GLuint i = 0; i < n_points_per_face; i += 2)
	{
		const glw::GLuint texture_coordinates_offset = i * n_texture_coordinates_components;
		const glw::GLuint grad_offset				 = i * n_grad_components;

		getTopNeighbour(texture_coordinates + texture_coordinates_offset, face, 1, width, grad_y + grad_offset);
	}

	for (glw::GLuint i = 1; i < n_points_per_face; i += 2)
	{
		const glw::GLuint texture_coordinates_offset = i * n_texture_coordinates_components;
		const glw::GLuint grad_offset				 = i * n_grad_components;

		getTopNeighbour(texture_coordinates + texture_coordinates_offset, face, 4 * width, width, grad_y + grad_offset);
	}

	for (glw::GLuint i = 0; i < n_points_per_face; ++i)
	{
		const glw::GLuint texture_coordinates_offset = i * n_texture_coordinates_components;
		const glw::GLuint grad_offset				 = i * n_grad_components;

		vectorSubtractInPlace<3>(grad_y + grad_offset, texture_coordinates + texture_coordinates_offset);
	}
}

/** Prepare "lods" for face.
 *  Pattern is: B T B
 *              T B T
 *              B T B
 *  B - base, T - top
 *
 *  @param lods            Storage for lods
 *  @param n_mipmap_levels Number of mipmap levels
 **/
void prepareLodForFace(glw::GLfloat* lods, glw::GLuint n_mipmap_levels)
{
	const glw::GLfloat base_level = 0.0f;
	const glw::GLfloat top_level  = (glw::GLfloat)(n_mipmap_levels - 1);

	lods[0] = base_level;
	lods[1] = top_level;
	lods[2] = base_level;
	lods[3] = top_level;
	lods[4] = base_level;
	lods[5] = top_level;
	lods[6] = base_level;
	lods[7] = top_level;
	lods[8] = base_level;
}

/** Prepare position for vertices. Each vertex is placed on a unique pixel of output image.
 *
 *  @param positions     Storage for positions
 *  @param cube_face     Texture coordinate
 *  @param element_index Index of element in array
 *  @param n_layers      Image width
 **/
void preparePositionForFace(glw::GLfloat* positions, glw::GLuint cube_face, glw::GLuint element_index,
							glw::GLuint n_layers)
{
	static const glw::GLuint x_offset_per_face = 3;
	static const glw::GLuint n_faces		   = 6;

	const glw::GLuint x_offset_for_face = (element_index * n_faces + cube_face) * x_offset_per_face;

	const glw::GLfloat x_step	 = 2.0f / ((glw::GLfloat)(n_layers * 3));
	const glw::GLfloat x_mid_step = x_step / 2.0f;
	const glw::GLfloat y_step	 = 2.0f / 3.0f;
	const glw::GLfloat y_mid_step = y_step / 2.0f;

	const glw::GLfloat x_left   = -1.0f + x_mid_step + ((glw::GLfloat)x_offset_for_face) * x_step;
	const glw::GLfloat x_middle = x_left + x_step;
	const glw::GLfloat x_right  = x_middle + x_step;

	const glw::GLfloat y_top	= 1.0f - y_mid_step;
	const glw::GLfloat y_middle = y_top - y_step;
	const glw::GLfloat y_bottom = y_middle - y_step;

	vectorSet4(positions, 0, x_left, y_top, 0.0f, 1.0f);
	vectorSet4(positions, 1, x_middle, y_top, 0.0f, 1.0f);
	vectorSet4(positions, 2, x_right, y_top, 0.0f, 1.0f);
	vectorSet4(positions, 3, x_left, y_middle, 0.0f, 1.0f);
	vectorSet4(positions, 4, x_middle, y_middle, 0.0f, 1.0f);
	vectorSet4(positions, 5, x_right, y_middle, 0.0f, 1.0f);
	vectorSet4(positions, 6, x_left, y_bottom, 0.0f, 1.0f);
	vectorSet4(positions, 7, x_middle, y_bottom, 0.0f, 1.0f);
	vectorSet4(positions, 8, x_right, y_bottom, 0.0f, 1.0f);
}

/** Prepare "refZ" for face.
 *  Pattern is: - = +
 *              - = +
 *              - = +
 *  '-' - lower than depth
 *   =  - eqaul to depth
 *   +  - higher thatn depth
 *
 *  @param refZs     Storage for refZs
 *  @param n_mipmaps Number of mipmap levels
 *  @param face      Cube map's face index
 *  @param layer     Index of element in array
 *  @param n_layers  Number of elements in array
 **/
void prepareRefZForFace(glw::GLfloat* refZs, glw::GLuint n_mipmaps, glw::GLuint face, glw::GLuint layer,
						glw::GLuint n_layers)
{
	glw::GLfloat expected_base_depth_value = 0;
	glw::GLfloat expected_top_depth_value  = 0;

	/* Get depth for top and base levles */
	getDepthComponent(face, layer, 0, n_layers, n_mipmaps, expected_base_depth_value);
	getDepthComponent(face, layer, n_mipmaps - 1, n_layers, n_mipmaps, expected_top_depth_value);

	/* Use step of 10% */
	const glw::GLfloat base_depth_step = expected_base_depth_value * 0.1f;
	const glw::GLfloat top_depth_step  = expected_top_depth_value * 0.1f;

	/* Top row */
	refZs[0] = expected_base_depth_value - base_depth_step;
	refZs[1] = expected_top_depth_value;
	refZs[2] = expected_base_depth_value + base_depth_step;

	/* Center row */
	refZs[3] = expected_top_depth_value - top_depth_step;
	refZs[4] = expected_base_depth_value;
	refZs[5] = expected_top_depth_value + top_depth_step;

	/* Bottom row */
	refZs[6] = expected_base_depth_value - base_depth_step;
	refZs[7] = expected_top_depth_value;
	refZs[8] = expected_base_depth_value + base_depth_step;
}

/** Prepare texture's face at given index and level, for rgba integer textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareRGBAIntegerTextureFace(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
								   glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
								   glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
								   glw::GLsizei texture_height)
{
	switch (texture_type)
	{
	case GL_UNSIGNED_INT:
		prepareDataForColorIntTexture<glw::GLuint>(gl, cube_face, element_index, mipmap_level, n_elements,
												   n_mipmap_levels, texture_format, texture_type, texture_width,
												   texture_height);
		break;

	case GL_INT:
		prepareDataForColorIntTexture<glw::GLint>(gl, cube_face, element_index, mipmap_level, n_elements,
												  n_mipmap_levels, texture_format, texture_type, texture_width,
												  texture_height);
		break;

	default:
		TCU_FAIL("Not implemented case !");
		break;
	}
}

/** Prepare texture's face at given index and level, for rgba textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareRGBATextureFace(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
							glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
							glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
							glw::GLsizei texture_height)
{
	switch (texture_type)
	{
	case GL_UNSIGNED_BYTE:
		prepareDataForColorIntTexture<glw::GLubyte>(gl, cube_face, element_index, mipmap_level, n_elements,
													n_mipmap_levels, texture_format, texture_type, texture_width,
													texture_height);
		break;

	case GL_FLOAT:
		prepareDataForColorFloatTexture(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels,
										texture_format, texture_type, texture_width, texture_height);
		break;

	default:
		TCU_FAIL("Not implemented case !");
		break;
	}
}

/** Prepare texture's face at given index and level, for stencil textures.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareStencilTextureFace(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
							   glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
							   glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
							   glw::GLsizei texture_height)
{
	switch (texture_type)
	{
	case GL_UNSIGNED_BYTE:
		prepareDataForStencilUIntTexture(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels,
										 texture_format, texture_type, texture_width, texture_height);
		break;

	default:
		TCU_FAIL("Not implemented case !");
		break;
	}
}

/** Prepare texture coordinates for vertices.
 *  Each vertex has unique value. 4 corners, centers of 4 edges and central points are selected.
 *
 *  @param positions     Storage for positions
 *  @param cube_face     Texture coordinate
 *  @param element_index Index of element in array
 *  @param n_layers      Image width
 **/
void prepareTextureCoordinatesForFace(glw::GLfloat* data, glw::GLuint width, glw::GLuint height, glw::GLfloat layer,
									  glw::GLuint face)
{
	const glw::GLfloat x_range = (glw::GLfloat)width;
	const glw::GLfloat y_range = (glw::GLfloat)height;

	const glw::GLfloat x_step = 2.0f / x_range;
	const glw::GLfloat y_step = 2.0f / y_range;

	const glw::GLfloat x_mid_step = x_step / 2.0f;
	const glw::GLfloat y_mid_step = y_step / 2.0f;

	const glw::GLfloat left		= -1.0f + x_mid_step;
	const glw::GLfloat right	= 1.0f - x_mid_step;
	const glw::GLfloat top		= 1.0f - y_mid_step;
	const glw::GLfloat bottom   = -1.0f + y_mid_step;
	const glw::GLfloat middle   = 0.0f;
	const glw::GLfloat negative = -1.0f;
	const glw::GLfloat positive = 1.0f;

	switch (face)
	{
	case 0:
		vectorSet4(data, 0, positive, left, top, layer);
		vectorSet4(data, 1, positive, middle, top, layer);
		vectorSet4(data, 2, positive, right, top, layer);
		vectorSet4(data, 3, positive, left, middle, layer);
		vectorSet4(data, 4, positive, middle, middle, layer);
		vectorSet4(data, 5, positive, right, middle, layer);
		vectorSet4(data, 6, positive, left, bottom, layer);
		vectorSet4(data, 7, positive, middle, bottom, layer);
		vectorSet4(data, 8, positive, right, bottom, layer);
		break;
	case 1:
		vectorSet4(data, 0, negative, left, top, layer);
		vectorSet4(data, 1, negative, middle, top, layer);
		vectorSet4(data, 2, negative, right, top, layer);
		vectorSet4(data, 3, negative, left, middle, layer);
		vectorSet4(data, 4, negative, middle, middle, layer);
		vectorSet4(data, 5, negative, right, middle, layer);
		vectorSet4(data, 6, negative, left, bottom, layer);
		vectorSet4(data, 7, negative, middle, bottom, layer);
		vectorSet4(data, 8, negative, right, bottom, layer);
		break;
	case 2:
		vectorSet4(data, 0, left, positive, top, layer);
		vectorSet4(data, 1, middle, positive, top, layer);
		vectorSet4(data, 2, right, positive, top, layer);
		vectorSet4(data, 3, left, positive, middle, layer);
		vectorSet4(data, 4, middle, positive, middle, layer);
		vectorSet4(data, 5, right, positive, middle, layer);
		vectorSet4(data, 6, left, positive, bottom, layer);
		vectorSet4(data, 7, middle, positive, bottom, layer);
		vectorSet4(data, 8, right, positive, bottom, layer);
		break;
	case 3:
		vectorSet4(data, 0, left, negative, top, layer);
		vectorSet4(data, 1, middle, negative, top, layer);
		vectorSet4(data, 2, right, negative, top, layer);
		vectorSet4(data, 3, left, negative, middle, layer);
		vectorSet4(data, 4, middle, negative, middle, layer);
		vectorSet4(data, 5, right, negative, middle, layer);
		vectorSet4(data, 6, left, negative, bottom, layer);
		vectorSet4(data, 7, middle, negative, bottom, layer);
		vectorSet4(data, 8, right, negative, bottom, layer);
		break;
	case 4:
		vectorSet4(data, 0, left, top, positive, layer);
		vectorSet4(data, 1, middle, top, positive, layer);
		vectorSet4(data, 2, right, top, positive, layer);
		vectorSet4(data, 3, left, middle, positive, layer);
		vectorSet4(data, 4, middle, middle, positive, layer);
		vectorSet4(data, 5, right, middle, positive, layer);
		vectorSet4(data, 6, left, bottom, positive, layer);
		vectorSet4(data, 7, middle, bottom, positive, layer);
		vectorSet4(data, 8, right, bottom, positive, layer);
		break;
	case 5:
		vectorSet4(data, 0, left, top, negative, layer);
		vectorSet4(data, 1, middle, top, negative, layer);
		vectorSet4(data, 2, right, top, negative, layer);
		vectorSet4(data, 3, left, middle, negative, layer);
		vectorSet4(data, 4, middle, middle, negative, layer);
		vectorSet4(data, 5, right, middle, negative, layer);
		vectorSet4(data, 6, left, bottom, negative, layer);
		vectorSet4(data, 7, middle, bottom, negative, layer);
		vectorSet4(data, 8, right, bottom, negative, layer);
		break;
	};

	vectorNormalize<3, 4>(data, 0);
	vectorNormalize<3, 4>(data, 1);
	vectorNormalize<3, 4>(data, 2);
	vectorNormalize<3, 4>(data, 3);
	vectorNormalize<3, 4>(data, 4);
	vectorNormalize<3, 4>(data, 5);
	vectorNormalize<3, 4>(data, 6);
	vectorNormalize<3, 4>(data, 7);
	vectorNormalize<3, 4>(data, 8);
}

/** Prepare texture coordinates for vertices. For sampling with textureGather routine.
 *  Each vertex has unique value. 4 corners, centers of 4 edges and central points are selected.
 *
 *  @param positions     Storage for positions
 *  @param cube_face     Texture coordinate
 *  @param element_index Index of element in array
 *  @param n_layers      Image width
 **/
void prepareTextureCoordinatesForGatherForFace(glw::GLfloat* data, glw::GLuint width, glw::GLuint height,
											   glw::GLfloat layer, glw::GLuint face)
{
	const glw::GLfloat x_range = (glw::GLfloat)width;
	const glw::GLfloat y_range = (glw::GLfloat)height;

	const glw::GLfloat x_step = 2.0f / x_range;
	const glw::GLfloat y_step = 2.0f / y_range;

	const glw::GLfloat x_mid_step = x_step / 2.0f;
	const glw::GLfloat y_mid_step = y_step / 2.0f;

	const glw::GLfloat left		= -1.0f + x_mid_step + x_step;
	const glw::GLfloat right	= 1.0f - x_mid_step - x_step;
	const glw::GLfloat top		= 1.0f - y_mid_step - y_step;
	const glw::GLfloat bottom   = -1.0f + y_mid_step + y_step;
	const glw::GLfloat middle   = 0.0f;
	const glw::GLfloat negative = -1.0f;
	const glw::GLfloat positive = 1.0f;

	switch (face)
	{
	case 0:
		vectorSet4(data, 0, positive, left, top, layer);
		vectorSet4(data, 1, positive, middle, top, layer);
		vectorSet4(data, 2, positive, right, top, layer);
		vectorSet4(data, 3, positive, left, middle, layer);
		vectorSet4(data, 4, positive, middle, middle, layer);
		vectorSet4(data, 5, positive, right, middle, layer);
		vectorSet4(data, 6, positive, left, bottom, layer);
		vectorSet4(data, 7, positive, middle, bottom, layer);
		vectorSet4(data, 8, positive, right, bottom, layer);
		break;
	case 1:
		vectorSet4(data, 0, negative, left, top, layer);
		vectorSet4(data, 1, negative, middle, top, layer);
		vectorSet4(data, 2, negative, right, top, layer);
		vectorSet4(data, 3, negative, left, middle, layer);
		vectorSet4(data, 4, negative, middle, middle, layer);
		vectorSet4(data, 5, negative, right, middle, layer);
		vectorSet4(data, 6, negative, left, bottom, layer);
		vectorSet4(data, 7, negative, middle, bottom, layer);
		vectorSet4(data, 8, negative, right, bottom, layer);
		break;
	case 2:
		vectorSet4(data, 0, left, positive, top, layer);
		vectorSet4(data, 1, middle, positive, top, layer);
		vectorSet4(data, 2, right, positive, top, layer);
		vectorSet4(data, 3, left, positive, middle, layer);
		vectorSet4(data, 4, middle, positive, middle, layer);
		vectorSet4(data, 5, right, positive, middle, layer);
		vectorSet4(data, 6, left, positive, bottom, layer);
		vectorSet4(data, 7, middle, positive, bottom, layer);
		vectorSet4(data, 8, right, positive, bottom, layer);
		break;
	case 3:
		vectorSet4(data, 0, left, negative, top, layer);
		vectorSet4(data, 1, middle, negative, top, layer);
		vectorSet4(data, 2, right, negative, top, layer);
		vectorSet4(data, 3, left, negative, middle, layer);
		vectorSet4(data, 4, middle, negative, middle, layer);
		vectorSet4(data, 5, right, negative, middle, layer);
		vectorSet4(data, 6, left, negative, bottom, layer);
		vectorSet4(data, 7, middle, negative, bottom, layer);
		vectorSet4(data, 8, right, negative, bottom, layer);
		break;
	case 4:
		vectorSet4(data, 0, left, top, positive, layer);
		vectorSet4(data, 1, middle, top, positive, layer);
		vectorSet4(data, 2, right, top, positive, layer);
		vectorSet4(data, 3, left, middle, positive, layer);
		vectorSet4(data, 4, middle, middle, positive, layer);
		vectorSet4(data, 5, right, middle, positive, layer);
		vectorSet4(data, 6, left, bottom, positive, layer);
		vectorSet4(data, 7, middle, bottom, positive, layer);
		vectorSet4(data, 8, right, bottom, positive, layer);
		break;
	case 5:
		vectorSet4(data, 0, left, top, negative, layer);
		vectorSet4(data, 1, middle, top, negative, layer);
		vectorSet4(data, 2, right, top, negative, layer);
		vectorSet4(data, 3, left, middle, negative, layer);
		vectorSet4(data, 4, middle, middle, negative, layer);
		vectorSet4(data, 5, right, middle, negative, layer);
		vectorSet4(data, 6, left, bottom, negative, layer);
		vectorSet4(data, 7, middle, bottom, negative, layer);
		vectorSet4(data, 8, right, bottom, negative, layer);
		break;
	};

	vectorNormalize<3, 4>(data, 0);
	vectorNormalize<3, 4>(data, 1);
	vectorNormalize<3, 4>(data, 2);
	vectorNormalize<3, 4>(data, 3);
	vectorNormalize<3, 4>(data, 4);
	vectorNormalize<3, 4>(data, 5);
	vectorNormalize<3, 4>(data, 6);
	vectorNormalize<3, 4>(data, 7);
	vectorNormalize<3, 4>(data, 8);
}

/** Prepare texture's face at given index and level.
 *
 *  @param gl                 GL functions
 *  @param cube_face          Index of cube map's face
 *  @param element_index      Index of element in array
 *  @param mipmap_level       Mipmap level
 *  @param n_elements         Number of elements in array
 *  @param n_mipmap_levels    Number of mipmap levels
 *  @param texture_format     Texture format
 *  @param texture_width      Texture width
 *  @param texture_height     Texture height
 **/
void prepareTextureFace(const glw::Functions& gl, glw::GLint cube_face, glw::GLint element_index,
						glw::GLint mipmap_level, glw::GLint n_elements, glw::GLint n_mipmap_levels,
						glw::GLenum texture_format, glw::GLenum texture_type, glw::GLsizei texture_width,
						glw::GLsizei texture_height)
{
	switch (texture_format)
	{
	case GL_RGBA:
		prepareRGBATextureFace(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels, texture_format,
							   texture_type, texture_width, texture_height);
		break;

	case GL_RGBA_INTEGER:
		prepareRGBAIntegerTextureFace(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels,
									  texture_format, texture_type, texture_width, texture_height);
		break;

	case GL_DEPTH_COMPONENT:
		prepareDepthTextureFace(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels, texture_format,
								texture_type, texture_width, texture_height);
		break;

	case GL_STENCIL_INDEX:
		prepareStencilTextureFace(gl, cube_face, element_index, mipmap_level, n_elements, n_mipmap_levels,
								  texture_format, texture_type, texture_width, texture_height);
		break;

	default:
		TCU_FAIL("Not implemented case !");
		break;
	}
}

/** Verifies that all pixels rendered for specific face match expectations
 *
 *  @tparam T            Type of image component
 *  @tparam N_Components Number of image components
 *  @tparam Width        Width of single face
 *  @tparam Height       Height of single face
 *
 *  @param data            Rendered data
 *  @param cube_face       Index of face in array
 *  @param expected_values Expected values
 *  @param image_width     Widht of whole image
 **/
template <typename T, unsigned int N_Components, unsigned int Width, unsigned int Height>
bool verifyFace(const T* data, const glw::GLuint cube_face, const T* expected_values, const glw::GLuint image_width)
{
	static const glw::GLuint size_of_pixel = N_Components;

	const glw::GLuint data_face_offset  = N_Components * Width * cube_face;
	const glw::GLuint exp_face_offset   = N_Components * Width * Height * cube_face;
	const glw::GLuint data_size_of_line = image_width * size_of_pixel;
	const glw::GLuint exp_size_of_line  = Width * size_of_pixel;

	for (glw::GLuint y = 0; y < Height; ++y)
	{
		const glw::GLuint data_line_offset = y * data_size_of_line;
		const glw::GLuint exp_line_offset  = y * exp_size_of_line;

		for (glw::GLuint x = 0; x < Width; ++x)
		{
			const glw::GLuint data_pixel_offset = data_line_offset + data_face_offset + x * size_of_pixel;
			const glw::GLuint exp_pixel_offset  = exp_line_offset + exp_face_offset + x * size_of_pixel;

			for (glw::GLuint component = 0; component < N_Components; ++component)
			{
				if (data[data_pixel_offset + component] != expected_values[exp_pixel_offset + component])
				{
					return false;
				}
			}
		}
	}

	return true;
}

/** Verifies that all rendered pixels match expectation
 *
 *  @tparam T            Type of image component
 *  @tparam N_Components Number of image components
 *  @tparam Width        Width of single face
 *  @tparam Height       Height of single face
 *
 *  @param data            Rendered data
 *  @param expected_values Expected values
 *  @param n_layers        Number of elements in array
 **/
template <typename T, unsigned int N_Components, unsigned int Width, unsigned int Height>
bool verifyImage(const T* data, const T* expected_values, const glw::GLuint n_layers)
{
	static const glw::GLuint n_faces = 6;

	const glw::GLuint n_total_faces = n_layers * n_faces;

	for (glw::GLuint face = 0; face < n_total_faces; ++face)
	{
		if (false == verifyFace<T, N_Components, Width, Height>(data, face, expected_values, n_total_faces * Width))
		{
			return false;
		}
	}

	return true;
}

/** Verifies that all rendered pixels match expectation
 *
 *  @tparam T            Type of image component
 *  @tparam N_Components Number of image components
 *  @tparam Width        Width of single face
 *  @tparam Height       Height of single face
 *
 *  @param n_mipmap_levels Number of mipmap levels
 *  @param n_layers        Number of elements in array
 *  @param getComponents   Routine which is used to obtain components
 *  @param data            Rendered data
 **/
template <typename T, unsigned int N_Components, unsigned int Width, unsigned int Height>
bool verifyResultImage(glw::GLuint n_mipmap_levels, glw::GLuint n_layers,
					   void (*getComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
											 glw::GLint n_layers, glw::GLint n_mipmap_levels, T* out_components),
					   const glw::GLubyte* data)
{
	const glw::GLuint n_components			= N_Components;
	const glw::GLuint face_width			= Width;
	const glw::GLuint face_height			= Height;
	const glw::GLuint n_pixels_per_face		= face_width * face_height;
	const glw::GLuint n_components_per_face = n_pixels_per_face * n_components;
	const glw::GLuint n_faces				= 6;
	const glw::GLuint n_total_faces			= n_layers * n_faces;
	const glw::GLuint n_total_components	= n_total_faces * n_components_per_face;
	const T*		  result_image			= (const T*)data;

	std::vector<T> expected_values;
	expected_values.resize(n_total_components);

	for (glw::GLuint layer = 0; layer < n_layers; ++layer)
	{
		const glw::GLuint layer_offset = layer * n_faces * n_components_per_face;

		for (glw::GLuint face = 0; face < n_faces; ++face)
		{
			const glw::GLuint face_offset = face * n_components_per_face + layer_offset;

			for (glw::GLuint pixel = 0; pixel < n_pixels_per_face; ++pixel)
			{
				const glw::GLuint pixel_offset = pixel * n_components + face_offset;

				T components[n_components];

				getComponents(pixel, face, layer, n_layers, n_mipmap_levels, components);

				for (glw::GLuint component = 0; component < n_components; ++component)
				{
					const glw::GLuint component_offset = pixel_offset + component;

					expected_values[component_offset] = components[component];
				}
			}
		}
	}

	return verifyImage<T, N_Components, Width, Height>(result_image, &expected_values[0], n_layers);
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
TextureCubeMapArraySamplingTest::TextureCubeMapArraySamplingTest(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_framebuffer_object_id(0)
	, compiled_shaders(0)
	, invalid_shaders(0)
	, linked_programs(0)
	, invalid_programs(0)
	, tested_cases(0)
	, failed_cases(0)
	, invalid_type_cases(0)
{
	/* Prepare formats set */
	m_formats.push_back(formatDefinition(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false, Float, "GL_RGBA8"));
	m_formats.push_back(formatDefinition(GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, false, Int, "GL_RGBA32I"));
	m_formats.push_back(formatDefinition(GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, false, UInt, "GL_RGBA32UI"));

	m_formats.push_back(formatDefinition(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, false, GL_RGBA8, GL_RGBA,
										 GL_UNSIGNED_BYTE, Depth, "GL_DEPTH_COMPONENT32F"));
	m_formats.push_back(formatDefinition(GL_STENCIL_INDEX8, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, false, GL_R32UI,
										 GL_RGBA_INTEGER, GL_UNSIGNED_INT, Stencil, "GL_STENCIL_INDEX8"));

	/* Prepare sampling functions set */
	m_functions.push_back(samplingFunctionDefinition(Texture, "Texture"));
	m_functions.push_back(samplingFunctionDefinition(TextureLod, "TextureLod"));
	m_functions.push_back(samplingFunctionDefinition(TextureGrad, "TextureGrad"));
	m_functions.push_back(samplingFunctionDefinition(TextureGather, "TextureGather"));

	/* Prepare mutabilities set */
	m_mutabilities.push_back(true);
	m_mutabilities.push_back(false);

	/* Prepare resolutions set */
	m_resolutions.push_back(resolutionDefinition(64, 64, 18));
	m_resolutions.push_back(resolutionDefinition(117, 117, 6));
	m_resolutions.push_back(resolutionDefinition(256, 256, 6));
	m_resolutions.push_back(resolutionDefinition(173, 173, 12));

	/* Prepare resolutions set for compressed formats */
	m_compressed_resolutions.push_back(resolutionDefinition(8, 8, 12));
	m_compressed_resolutions.push_back(resolutionDefinition(13, 13, 12));
}

/** Check if getActiveUniform and glGetProgramResourceiv returns correct type for cube array samplers.
 *
 *  @param program_id   Program id
 *  @param sampler_name_p Name of sampler
 *  @param sampler_type Expected type of sampler
 *
 *  @return Status. 1st LSB - glGetActiveUniform, second LSB glGetProgramResourceiv, 0 valid, 1 invalid.
 **/
glw::GLuint TextureCubeMapArraySamplingTest::checkUniformAndResourceApi(glw::GLuint		   program_id,
																		const glw::GLchar* sampler_name_p,
																		samplerType		   sampler_type)
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum  expected_type				= 0;
	glw::GLuint  index_getActiveUniform		= GL_INVALID_INDEX;
	glw::GLuint  index_getProgramResourceiv = GL_INVALID_INDEX;
	glw::GLenum  props						= GL_TYPE;
	glw::GLuint  result						= 0;
	glw::GLchar* name						= 0;
	glw::GLint   size						= 0;
	glw::GLenum  type_getActiveUniform		= 0;
	glw::GLint   type_getProgramResourceiv  = 0;

	// Get type by getActiveUniform
	gl.getUniformIndices(program_id, 1, &sampler_name_p, &index_getActiveUniform);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformIndices");

	if (GL_INVALID_INDEX == index_getActiveUniform)
	{
		throw tcu::InternalError("glGetUniformIndices: GL_INVALID_INDEX", "", __FILE__, __LINE__);
	}

	gl.getActiveUniform(program_id, index_getActiveUniform, 0, 0, &size, &type_getActiveUniform, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveUniform");

	// Get type by gl.getProgramResourceiv
	index_getProgramResourceiv = gl.getProgramResourceIndex(program_id, GL_UNIFORM, sampler_name_p);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceIndex");

	if (GL_INVALID_INDEX == index_getProgramResourceiv)
	{
		throw tcu::InternalError("glGetProgramResourceIndex: GL_INVALID_INDEX", "", __FILE__, __LINE__);
	}

	gl.getProgramResourceiv(program_id, GL_UNIFORM, index_getProgramResourceiv, 1, &props, 1, 0,
							&type_getProgramResourceiv);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv");

	// Verification
	switch (sampler_type)
	{
	case Float:
		expected_type = GL_SAMPLER_CUBE_MAP_ARRAY;
		break;
	case Int:
		expected_type = GL_INT_SAMPLER_CUBE_MAP_ARRAY;
		break;
	case UInt:
		expected_type = GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY;
		break;
	case Depth:
		expected_type = GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW;
		break;
	case Stencil:
		expected_type = GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY;
		break;
	}

	if (expected_type != type_getActiveUniform)
	{
		result |= m_get_type_api_status_uniform;
	}
	if (expected_type != (glw::GLuint)type_getProgramResourceiv)
	{
		result |= m_get_type_api_status_program_resource;
	}

	return result;
}

/** Compile shader
 *
 *  @param info Shader info
 **/
void TextureCubeMapArraySamplingTest::compile(shaderDefinition& info)
{
	compiled_shaders += 1;

	if (false == info.compile())
	{
		invalid_shaders += 1;

		logCompilationLog(info);
	}
}

/** Execute compute shader
 *
 *  @param program_id Program id
 *  @param width      Width of result image
 *  @param height     Height of result image
 **/
void TextureCubeMapArraySamplingTest::dispatch(glw::GLuint program_id, glw::GLuint width, glw::GLuint height)
{
	(void)program_id;

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.dispatchCompute(width, height, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glDispatchCompute call.");
}

/** Execute render call
 *
 *  @param program_id     Program id
 *  @param primitive_type Type of primitive
 *  @param n_vertices     Number of vertices
 **/
void TextureCubeMapArraySamplingTest::draw(glw::GLuint program_id, glw::GLenum primitive_type, glw::GLuint n_vertices,
										   glw::GLenum format)
{
	(void)program_id;

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const glw::GLenum framebuffer_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status)
	{
		throw tcu::InternalError("Framebuffer is incomplete", "", __FILE__, __LINE__);
	}

	switch (format)
	{
	case GL_RGBA32I:
	{
		const glw::GLint clearValue[4] = { 255, 255, 255, 255 };
		gl.clearBufferiv(GL_COLOR, 0, clearValue);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glClearBufferiv call.");
	}
	break;
	case GL_RGBA32UI:
	case GL_R32UI:
	{
		const glw::GLuint clearValue[4] = { 255, 255, 255, 255 };
		gl.clearBufferuiv(GL_COLOR, 0, clearValue);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glClearBufferuiv call.");
	}
	break;
	case GL_DEPTH_COMPONENT32F:
		gl.clearDepthf(1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glClearDepthf call.");
		break;
	case GL_STENCIL_INDEX8:
		gl.clearStencil(1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glClearStencil call.");
		break;

	default:
		gl.clearColor(1.0f, 1.0f, 1.0f, 1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glClearColor call.");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glClear call.");
	}

	gl.drawArrays(primitive_type, 0, n_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glDrawArrays call.");
}

/** Get attributes specific for type of sampler
 *
 *  @param sampler_type              Type of sampler
 *  @param out_attribute_definitions Array of attributes
 *  @param out_n_attributes          Number of attributes
 **/
void TextureCubeMapArraySamplingTest::getAttributes(samplerType					sampler_type,
													const attributeDefinition*& out_attribute_definitions,
													glw::GLuint&				out_n_attributes)
{
	static attributeDefinition depth_attributes[] = { { attribute_refZ, type_float, RefZ, 1 } };

	static const glw::GLuint n_depth_attributes = sizeof(depth_attributes) / sizeof(depth_attributes[0]);

	switch (sampler_type)
	{
	case Depth:
		out_attribute_definitions = depth_attributes;
		out_n_attributes		  = n_depth_attributes;
		break;
	default:
		out_attribute_definitions = 0;
		out_n_attributes		  = 0;
		break;
	};
}

/** Get attributes specific for sampling function
 *
 *  @param sampling_function         Sampling function
 *  @param out_attribute_definitions Array of attributes
 *  @param out_n_attributes          Number of attributes
 **/
void TextureCubeMapArraySamplingTest::getAttributes(samplingFunction			sampling_function,
													const attributeDefinition*& out_attribute_definitions,
													glw::GLuint&				out_n_attributes)
{
	static attributeDefinition texture_attributes[] = { { attribute_texture_coordinate, type_vec4, TextureCoordinates,
														  0 } };

	static attributeDefinition textureLod_attributes[] = {
		{ attribute_texture_coordinate, type_vec4, TextureCoordinates, 0 }, { attribute_lod, type_float, Lod, 1 }
	};

	static attributeDefinition textureGrad_attributes[] = { { attribute_texture_coordinate, type_vec4,
															  TextureCoordinates, 0 },
															{ attribute_grad_x, type_vec3, GradX, 1 },
															{ attribute_grad_y, type_vec3, GradY, 2 } };

	static attributeDefinition textureGather_attributes[] = { { attribute_texture_coordinate, type_vec4,
																TextureCoordinatesForGather, 0 } };

	static const glw::GLuint n_texture_attributes	= sizeof(texture_attributes) / sizeof(texture_attributes[0]);
	static const glw::GLuint n_textureLod_attributes = sizeof(textureLod_attributes) / sizeof(textureLod_attributes[0]);
	static const glw::GLuint n_textureGrad_attributes =
		sizeof(textureGrad_attributes) / sizeof(textureGrad_attributes[0]);
	static const glw::GLuint n_textureGather_attributes =
		sizeof(textureGather_attributes) / sizeof(textureGather_attributes[0]);

	switch (sampling_function)
	{
	case Texture:
		out_attribute_definitions = texture_attributes;
		out_n_attributes		  = n_texture_attributes;
		break;
	case TextureLod:
		out_attribute_definitions = textureLod_attributes;
		out_n_attributes		  = n_textureLod_attributes;
		break;
	case TextureGrad:
		out_attribute_definitions = textureGrad_attributes;
		out_n_attributes		  = n_textureGrad_attributes;
		break;
	case TextureGather:
		out_attribute_definitions = textureGather_attributes;
		out_n_attributes		  = n_textureGather_attributes;
		break;
	};
}

/** Get information about color type for type of sampler
 *
 *  @param sampler_type           Type of sampler
 *  @param out_color_type         Type used for color storage
 *  @param out_interpolation_type Type of interpolation
 *  @param out_sampler_type       Type of sampler
 *  @param out_n_components       Number of components in color
 *  @param out_is_shadow          If shadow sampler
 **/
void TextureCubeMapArraySamplingTest::getColorType(samplerType sampler_type, const glw::GLchar*& out_color_type,
												   const glw::GLchar*& out_interpolation_type,
												   const glw::GLchar*& out_sampler_type, glw::GLuint& out_n_components,
												   bool& out_is_shadow)
{
	switch (sampler_type)
	{
	case Float:
		out_color_type		   = type_vec4;
		out_interpolation_type = "";
		out_sampler_type	   = sampler_float;
		out_n_components	   = 4;
		out_is_shadow		   = false;
		break;
	case Int:
		out_color_type		   = type_ivec4;
		out_interpolation_type = interpolation_flat;
		out_sampler_type	   = sampler_int;
		out_n_components	   = 4;
		out_is_shadow		   = false;
		break;
	case UInt:
		out_color_type		   = type_uvec4;
		out_interpolation_type = interpolation_flat;
		out_sampler_type	   = sampler_uint;
		out_n_components	   = 4;
		out_is_shadow		   = false;
		break;
	case Depth:
		out_color_type		   = type_float;
		out_interpolation_type = "";
		out_sampler_type	   = sampler_depth;
		out_n_components	   = 1;
		out_is_shadow		   = true;
		break;
	case Stencil:
		out_color_type		   = type_uint;
		out_interpolation_type = interpolation_flat;
		out_sampler_type	   = sampler_uint;
		out_n_components	   = 1;
		out_is_shadow		   = false;
		break;
	};
}

/** Get information about color type for type of sampler
 *
 *  @param sampler_type           Type of sampler
 *  @param out_color_type         Type used for color storage
 *  @param out_interpolation_type Type of interpolation
 *  @param out_sampler_type       Type of sampler
 *  @param out_image_type         Type of image
 *  @param out_n_components       Number of components in color
 *  @param out_is_shadow          If shadow sampler
 **/
void TextureCubeMapArraySamplingTest::getColorType(samplerType sampler_type, const glw::GLchar*& out_color_type,
												   const glw::GLchar*& out_interpolation_type,
												   const glw::GLchar*& out_sampler_type,
												   const glw::GLchar*& out_image_type,
												   const glw::GLchar*& out_image_layout, glw::GLuint& out_n_components,
												   bool& out_is_shadow)
{
	getColorType(sampler_type, out_color_type, out_interpolation_type, out_sampler_type, out_n_components,
				 out_is_shadow);

	switch (sampler_type)
	{
	case Float:
		out_image_type   = image_float;
		out_image_layout = "rgba8";
		break;
	case Depth:
		out_image_type   = image_float;
		out_image_layout = "rgba8";
		break;
	case Int:
		out_image_type   = image_int;
		out_image_layout = "rgba32i";
		break;
	case UInt:
		out_image_type   = image_uint;
		out_image_layout = "rgba32ui";
		break;
	case Stencil:
		out_image_type   = image_uint;
		out_image_layout = "r32ui";
		break;
	};
}

/** Prepare code for passthrough fragment shader
 *
 *  @param sampler_type             Type of sampler
 *  @param out_fragment_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getPassThroughFragmentShaderCode(samplerType  sampler_type,
																	   std::string& out_fragment_shader_code)
{
	std::stringstream  stream;
	const glw::GLchar* color_type;
	const glw::GLchar* interpolation_type;
	const glw::GLchar* ignored_sampler_type;
	glw::GLuint		   ignored_n_components;
	bool			   ignored_is_shadow;

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, ignored_sampler_type, ignored_n_components,
				 ignored_is_shadow);

	/* Preamble */
	stream << shader_code_preamble << shader_precision << "/* Pass through fragment shader */" << std::endl;

	/* in vec4 fs_in_color */
	stream << interpolation_type << shader_input << color_type << fragment_shader_input << ";" << std::endl;

	stream << std::endl;

	/* layout(location = 0) out vec4 fs_out_color */
	stream << shader_layout << shader_output << color_type << fragment_shader_output << ";" << std::endl;

	stream << std::endl;

	/* Body */
	stream << fragment_shader_pass_through_body_code << std::endl;

	/* Store result */
	out_fragment_shader_code = stream.str();
}

/** Prepare code for passthrough tesselation control shader
 *
 *  @param sampler_type                        Type of sampler
 *  @param out_tesselation_control_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getPassThroughTesselationControlShaderCode(
	const samplerType& sampler_type, const samplingFunction& sampling_function,
	std::string& out_tesselation_control_shader_code)
{
	std::stringstream		   stream;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;

	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Preamble, extension : require  */
	stream << shader_code_preamble << tesselation_shader_extension << shader_precision << std::endl
		   << "/* Passthrough tesselation control shader */" << std::endl;

	/* layout(vertices = 1) out */
	stream << tesselation_control_shader_layout;

	/* in type attribute*/
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << vertex_shader_output
			   << routine_attribute_definitions[i].name << "[];" << std::endl;
		stream << shader_output << routine_attribute_definitions[i].type << tesselation_control_shader_output
			   << routine_attribute_definitions[i].name << "[];" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << vertex_shader_output
			   << type_attribute_definitions[i].name << "[];" << std::endl;
		stream << shader_output << type_attribute_definitions[i].type << tesselation_control_shader_output
			   << type_attribute_definitions[i].name << "[];" << std::endl;
	}

	/* Body */
	stream << tesselation_control_shader_sampling_body_code;

	/* tcs_out[gl_InvocationID] = vs_out[gl_InvocationID] */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << "    " << tesselation_control_shader_output << routine_attribute_definitions[i].name
			   << "[gl_InvocationID] = " << vertex_shader_output << routine_attribute_definitions[i].name
			   << "[gl_InvocationID];" << std::endl;
	}

	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << "    " << tesselation_control_shader_output << type_attribute_definitions[i].name
			   << "[gl_InvocationID] = " << vertex_shader_output << type_attribute_definitions[i].name
			   << "[gl_InvocationID];" << std::endl;
	}

	stream << "}" << std::endl << std::endl;

	/* Store result */
	out_tesselation_control_shader_code = stream.str();
}

/** Prepare code for passthrough tesselation evaluation shader
 *
 *  @param sampler_type                           Type of sampler
 *  @param out_tesselation_evaluation_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getPassThroughTesselationEvaluationShaderCode(
	samplerType sampler_type, std::string& out_tesselation_evaluation_shader_code)
{
	const glw::GLchar* color_type			= 0;
	bool			   ignored_is_shadow	= false;
	glw::GLuint		   ignored_n_components = 0;
	const glw::GLchar* ignored_sampler_type = 0;
	const glw::GLchar* interpolation_type   = 0;
	std::stringstream  stream;

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, ignored_sampler_type, ignored_n_components,
				 ignored_is_shadow);

	/* Preamble, extension : require */
	stream << shader_code_preamble << tesselation_shader_extension << shader_precision << std::endl
		   << "/* Pass through tesselation evaluation shader */" << std::endl;

	/* layout(point_mode) in; */
	stream << tesselation_evaluation_shader_layout;

	/* in vec4 tes_in_color[] */
	stream << interpolation_type << shader_input << color_type << tesselation_evaluation_shader_input << "[];"
		   << std::endl;

	stream << std::endl;

	/* out vec4 fs_in_color[] */
	stream << interpolation_type << shader_output << color_type << fragment_shader_input << ";" << std::endl;

	stream << std::endl;

	/* Body */
	stream << tesselation_evaluation_shader_pass_through_body_code << std::endl;

	/* Store result */
	out_tesselation_evaluation_shader_code = stream.str();
}

/** Prepare code for passthrough vertex shader
 *
 *  @param sampler_type           Type of sampler
 *  @param sampling_function      Type of sampling function
 *  @param out_vertex_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getPassThroughVertexShaderCode(const samplerType&		 sampler_type,
																	 const samplingFunction& sampling_function,
																	 std::string&			 out_vertex_shader_code)
{
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	std::stringstream		   stream;
	const attributeDefinition* type_attribute_definitions = 0;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Preamble */
	stream << shader_code_preamble << "/* Pass through vertex shader */" << std::endl << shader_precision;

	/* in vec4 vs_in_position */
	stream << shader_input << type_vec4 << vertex_shader_input << vertex_shader_position << ";" << std::endl;

	/* in type vs_in_attribute */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << vertex_shader_input
			   << routine_attribute_definitions[i].name << ";" << std::endl;
	}

	/* in float vs_in_refZ */
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << vertex_shader_input
			   << type_attribute_definitions[i].name << ";" << std::endl;
	}

	stream << std::endl;

	/* out type vs_out_attribute */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_output << routine_attribute_definitions[i].type << vertex_shader_output
			   << routine_attribute_definitions[i].name << ";" << std::endl;
	}

	/* out float vs_out_refZ */
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_output << type_attribute_definitions[i].type << vertex_shader_output
			   << type_attribute_definitions[i].name << ";" << std::endl;
	}

	stream << std::endl;

	/* Body */
	stream << vertex_shader_body_code;

	/* vs_out = vs_in */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << "    " << vertex_shader_output << routine_attribute_definitions[i].name << " = "
			   << vertex_shader_input << routine_attribute_definitions[i].name << ";" << std::endl;
	}

	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << "    " << vertex_shader_output << type_attribute_definitions[i].name << " = " << vertex_shader_input
			   << type_attribute_definitions[i].name << ";" << std::endl;
	}

	stream << "}" << std::endl << std::endl;

	/* Store result */
	out_vertex_shader_code = stream.str();
}

/** Prepare code for sampling compute shader
 *
 *  @param sampler_type            Type of sampler
 *  @param sampling_function       Type of sampling function
 *  @param out_compute_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getSamplingComputeShaderCode(const samplerType&	  sampler_type,
																   const samplingFunction& sampling_function,
																   std::string&			   out_compute_shader_code)
{
	const glw::GLchar*		   color_type					 = 0;
	const glw::GLchar*		   image_type_str				 = 0;
	const glw::GLchar*		   image_layout_str				 = 0;
	const glw::GLchar*		   interpolation_type			 = 0;
	bool					   is_shadow_sampler			 = false;
	glw::GLuint				   n_components					 = 0;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;
	const glw::GLchar*		   sampler_type_str				 = 0;
	std::string				   sampling_code;
	std::stringstream		   stream;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, sampler_type_str, image_type_str, image_layout_str,
				 n_components, is_shadow_sampler);

	/* Get sampling code */
	if (false == is_shadow_sampler)
	{
		getSamplingFunctionCall(sampling_function, color_type, n_components, compute_shader_param, 0,
								compute_shader_color, 0, sampler_name, sampling_code);
	}
	else
	{
		getShadowSamplingFunctionCall(sampling_function, color_type, n_components, compute_shader_param, 0,
									  compute_shader_color, 0, sampler_name, sampling_code);
	}

	/* Preamble */
	stream << shader_code_preamble << shader_precision << "/* Sampling compute shader */" << std::endl;

	/* uniform samplerType sampler */
	stream << shader_uniform << "highp " << sampler_type_str << sampler_name << ";" << std::endl;

	/* uniform writeonly image2D image*/
	stream << "layout(" << image_layout_str << ") " << shader_uniform << shader_writeonly << "highp " << image_type_str
		   << image_name << ";" << std::endl;

	/* layout(shared) buffer attribute { type attribute_data[]; }; */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << compute_shader_layout_binding << routine_attribute_definitions[i].binding << compute_shader_buffer
			   << routine_attribute_definitions[i].name << std::endl;

		stream << "{\n";
		stream << "    " << routine_attribute_definitions[i].type << " " << routine_attribute_definitions[i].name
			   << "_data[];\n";
		stream << "};\n";
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << compute_shader_layout_binding << type_attribute_definitions[i].binding << compute_shader_buffer
			   << type_attribute_definitions[i].name << std::endl;

		stream << "{\n";
		stream << "    " << type_attribute_definitions[i].type << " " << type_attribute_definitions[i].name
			   << "_data[];\n";
		stream << "};\n";
	}

	/* layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in; */
	stream << compute_shader_layout << std::endl;

	/* main + body */
	stream << compute_shader_body;

	/* type cs_attribute = attribute_data[vertex_index] */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << "    " << routine_attribute_definitions[i].type << compute_shader_param
			   << routine_attribute_definitions[i].name << " = " << routine_attribute_definitions[i].name
			   << "_data[vertex_index];" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << "    " << type_attribute_definitions[i].type << compute_shader_param
			   << type_attribute_definitions[i].name << " = " << type_attribute_definitions[i].name
			   << "_data[vertex_index];" << std::endl;
	}

	/* type color */
	stream << std::endl << "    " << color_type << compute_shader_color << ";" << std::endl;

	/* color = texture*/
	stream << std::endl << sampling_code << std::endl;
	//stream << std::endl << compute_shader_color << " = vec4(cs_grad_x, 255.0);" << std::endl;

	/* imageStore */
	stream << compute_shader_image_store;
	switch (n_components)
	{
	case 1:
		/* imageStore(image, image_coord, color.r);*/
		if (sampler_type == Depth)
		{
			stream << "vec4(" << compute_shader_color << ")";
		}
		else if (sampler_type == Stencil)
		{
			stream << "uvec4(" << compute_shader_color << ")";
		}
		else
		{
			// unexpected case
			DE_ASSERT(false);
		}
		break;
	case 4:
		/* imageStore(image, image_coord, color);*/
		stream << compute_shader_color;
		break;
	};

	stream << ");\n";

	stream << "}\n" << std::endl;

	out_compute_shader_code = stream.str();
}

/** Prepare code for sampling fragment shader
 *
 *  @param sampler_type             Type of sampler
 *  @param sampling_function        Type of sampling function
 *  @param out_fragment_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getSamplingFragmentShaderCode(const samplerType&		sampler_type,
																	const samplingFunction& sampling_function,
																	std::string&			out_fragment_shader_code)
{
	const glw::GLchar*		   color_type					 = 0;
	const glw::GLchar*		   interpolation_type			 = 0;
	bool					   is_shadow_sampler			 = false;
	glw::GLuint				   n_components					 = 0;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;
	const glw::GLchar*		   sampler_type_str				 = 0;
	std::string				   sampling_code;
	std::stringstream		   stream;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, sampler_type_str, n_components, is_shadow_sampler);

	/* Get sampling code */
	if (false == is_shadow_sampler)
	{
		getSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_output, 0,
								fragment_shader_output, 0, sampler_name, sampling_code);
	}
	else
	{
		getShadowSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_output, 0,
									  fragment_shader_output, 0, sampler_name, sampling_code);
	}

	/* Preamble */
	stream << shader_code_preamble << shader_precision << "/* Sampling fragment shader */" << std::endl;

	/* uniform samplerType sampler */
	stream << shader_uniform << "highp " << sampler_type_str << sampler_name << ";" << std::endl;

	stream << std::endl;

	/* in type attribute */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << vertex_shader_output
			   << routine_attribute_definitions[i].name << ";" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << vertex_shader_output
			   << type_attribute_definitions[i].name << ";" << std::endl;
	}

	stream << std::endl;

	/* layout(location = 0) out vec4 fs_out_color */
	stream << shader_layout << shader_output << color_type << fragment_shader_output << ";" << std::endl;

	stream << std::endl;

	/* Body */
	stream << fragment_shader_sampling_body_code;

	/* Sampling code */
	stream << sampling_code;

	stream << "}" << std::endl << std::endl;

	/* Store result */
	out_fragment_shader_code = stream.str();
}

/** Prepare sampling code
 *
 *  @param sampling_function     Type of sampling function
 *  @param color_type            Type of color
 *  @param n_components          Number of components
 *  @param attribute_name_prefix Prefix for attributes
 *  @param attribute_index       Index for attributes
 *  @param color_variable_name   Name of color variable
 *  @param color_variable_index  Index for color variable
 *  @param sampler_name_p        Name of sampler
 *  @param out_code              Result code
 **/
void TextureCubeMapArraySamplingTest::getSamplingFunctionCall(samplingFunction   sampling_function,
															  const glw::GLchar* color_type, glw::GLuint n_components,
															  const glw::GLchar* attribute_name_prefix,
															  const glw::GLchar* attribute_index,
															  const glw::GLchar* color_variable_name,
															  const glw::GLchar* color_variable_index,
															  const glw::GLchar* sampler_name_p, std::string& out_code)
{
	std::stringstream stream;

	switch (sampling_function)
	{
	case Texture:
		/* fs_in_color = texture(sampler, vs_out_texture_coordinates); */
		stream << "    " << var2str(0, color_variable_name, color_variable_index);

		stream << " = " << texture_func << "(" << sampler_name_p;

		stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index);

		if (1 == n_components)
		{
			stream << ").x;" << std::endl;
		}
		else
		{
			stream << ");" << std::endl;
		}
		break;

	case TextureLod:
		/* fs_in_color = textureLod(sampler, vs_out_texture_coordinates, lod); */
		stream << "    " << var2str(0, color_variable_name, color_variable_index);
		stream << " = " << textureLod_func << "(" << sampler_name_p;

		stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index) << ", "
			   << var2str(attribute_name_prefix, attribute_lod, attribute_index);

		if (1 == n_components)
		{
			stream << ").x;" << std::endl;
		}
		else
		{
			stream << ");" << std::endl;
		}
		break;

	case TextureGrad:
		/* fs_in_color = textureGrad(sampler, vs_out_texture_coordinates, vs_out_grad_x, vs_out_grad_y); */
		stream << "    " << var2str(0, color_variable_name, color_variable_index);

		stream << " = " << textureGrad_func << "(" << sampler_name_p;

		stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index) << ", "
			   << var2str(attribute_name_prefix, attribute_grad_x, attribute_index) << ", "
			   << var2str(attribute_name_prefix, attribute_grad_y, attribute_index);

		if (1 == n_components)
		{
			stream << ").x;" << std::endl;
		}
		else
		{
			stream << ");" << std::endl;
		}
		break;

	case TextureGather:
		if (4 == n_components)
		{
			/**
			 *  color_type component_0 = textureGather(sampler, vs_out_texture_coordinates, 0);
			 *  color_type component_1 = textureGather(sampler, vs_out_texture_coordinates, 1);
			 *  color_type component_2 = textureGather(sampler, vs_out_texture_coordinates, 2);
			 *  color_type component_3 = textureGather(sampler, vs_out_texture_coordinates, 3);
			 *  fs_in_color = color_type(component_0.r, component_1.g, component_2.b, component_3.a);
			 **/
			for (glw::GLuint i = 0; i < 4; ++i)
			{
				stream << "    " << color_type << "component_" << i << " = " << textureGather_func << "("
					   << sampler_name_p;

				stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index);

				stream << ", " << i << ");" << std::endl;
			}

			stream << "    " << var2str(0, color_variable_name, color_variable_index);

			stream << " = " << color_type << "(component_0.r, "
				   << "component_1.g, "
				   << "component_2.b, "
				   << "component_3.a);" << std::endl;
		}
		else
		{
			stream << "    " << var2str(0, color_variable_name, color_variable_index);

			stream << " = " << textureGather_func << "(" << sampler_name_p;

			stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index);

			stream << ").x;" << std::endl;
		}
		break;
	}

	out_code = stream.str();
}

/** Prepare code for sampling geometry shader
 *
 *  @param sampler_type             Type of sampler
 *  @param sampling_function        Type of sampling function
 *  @param out_geometry_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getSamplingGeometryShaderCode(const samplerType&		sampler_type,
																	const samplingFunction& sampling_function,
																	std::string&			out_geometry_shader_code)
{
	const glw::GLchar*		   color_type					 = 0;
	const glw::GLchar*		   interpolation_type			 = 0;
	bool					   is_shadow_sampler			 = false;
	glw::GLuint				   n_components					 = 0;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;
	const glw::GLchar*		   sampler_type_str				 = 0;
	std::string				   sampling_code;
	std::stringstream		   stream;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, sampler_type_str, n_components, is_shadow_sampler);

	/* Get sampling code */
	if (false == is_shadow_sampler)
	{
		getSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_output, "0",
								fragment_shader_input, 0, sampler_name, sampling_code);
	}
	else
	{
		getShadowSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_output, "0",
									  fragment_shader_input, 0, sampler_name, sampling_code);
	}

	/* Preamble, extension : require  */
	stream << shader_code_preamble << geometry_shader_extension << shader_precision << std::endl
		   << "/* Sampling geometry shader */" << std::endl;

	/* In out layout */
	stream << geometry_shader_layout;

	/* uniform samplerType sampler */
	stream << shader_uniform << "highp " << sampler_type_str << sampler_name << ";" << std::endl;

	stream << std::endl;

	/* in type attribute[]*/
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << vertex_shader_output
			   << routine_attribute_definitions[i].name << "[];" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << vertex_shader_output
			   << type_attribute_definitions[i].name << "[];" << std::endl;
	}

	stream << std::endl;

	/* out vec4 fs_in_color */
	stream << interpolation_type << shader_output << color_type << fragment_shader_input << ";" << std::endl;

	stream << std::endl;

	/* Body */
	stream << geometry_shader_sampling_body_code;

	/* Sampling code */
	stream << sampling_code;

	stream << geometry_shader_emit_vertex_code << std::endl;

	/* Store result */
	out_geometry_shader_code = stream.str();
}

/** Prepare code for sampling tesselation control shader
 *
 *  @param sampler_type                        Type of sampler
 *  @param sampling_function                   Type of sampling function
 *  @param out_tesselation_control_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getSamplingTesselationControlShaderCode(
	const samplerType& sampler_type, const samplingFunction& sampling_function,
	std::string& out_tesselation_control_shader_code)
{
	const glw::GLchar*		   color_type					 = 0;
	const glw::GLchar*		   interpolation_type			 = 0;
	bool					   is_shadow_sampler			 = false;
	glw::GLuint				   n_components					 = 0;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;
	const glw::GLchar*		   sampler_type_str				 = 0;
	std::string				   sampling_code;
	std::stringstream		   stream;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, sampler_type_str, n_components, is_shadow_sampler);

	/* Get sampling code */
	if (false == is_shadow_sampler)
	{
		getSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_output, "gl_InvocationID",
								tesselation_evaluation_shader_input, "gl_InvocationID", sampler_name, sampling_code);
	}
	else
	{
		getShadowSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_output,
									  "gl_InvocationID", tesselation_evaluation_shader_input, "gl_InvocationID",
									  sampler_name, sampling_code);
	}

	/* Preamble, extension : require  */
	stream << shader_code_preamble << tesselation_shader_extension << shader_precision << std::endl
		   << "/* Sampling tesselation control shader */" << std::endl;

	/* layout(vertices = 1) out */
	stream << tesselation_control_shader_layout;

	/* uniform samplerType sampler */
	stream << shader_uniform << "highp " << sampler_type_str << sampler_name << ";" << std::endl;

	stream << std::endl;

	/* in type attribute[]*/
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << vertex_shader_output
			   << routine_attribute_definitions[i].name << "[];" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << vertex_shader_output
			   << type_attribute_definitions[i].name << "[];" << std::endl;
	}

	stream << std::endl;

	/* out vec4 tes_in_color */
	stream << interpolation_type << shader_output << color_type << tesselation_evaluation_shader_input << "[];"
		   << std::endl;

	stream << std::endl;

	/* Body */
	stream << tesselation_control_shader_sampling_body_code;

	/* Sampling code */
	stream << sampling_code;

	stream << "}" << std::endl << std::endl;

	/* Store result */
	out_tesselation_control_shader_code = stream.str();
}

/** Prepare code for sampling tesselation evaluation shader
 *
 *  @param sampler_type                           Type of sampler
 *  @param sampling_function                      Type of sampling function
 *  @param out_tesselation_evaluation_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getSamplingTesselationEvaluationShaderCode(
	const samplerType& sampler_type, const samplingFunction& sampling_function,
	std::string& out_tesselation_evaluation_shader_code)
{
	const glw::GLchar*		   color_type					 = 0;
	const glw::GLchar*		   interpolation_type			 = 0;
	bool					   is_shadow_sampler			 = false;
	glw::GLuint				   n_components					 = 0;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;
	const glw::GLchar*		   sampler_type_str				 = 0;
	std::string				   sampling_code;
	std::stringstream		   stream;
	const glw::GLchar*		   prev_stage_output = (glu::isContextTypeES(m_context.getRenderContext().getType())) ?
											   tesselation_control_shader_output :
											   vertex_shader_output;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, sampler_type_str, n_components, is_shadow_sampler);

	/* Get sampling code */
	if (false == is_shadow_sampler)
	{
		getSamplingFunctionCall(sampling_function, color_type, n_components, prev_stage_output, "0",
								fragment_shader_input, 0, sampler_name, sampling_code);
	}
	else
	{
		getShadowSamplingFunctionCall(sampling_function, color_type, n_components, prev_stage_output, "0",
									  fragment_shader_input, 0, sampler_name, sampling_code);
	}

	/* Preamble, extension : require */
	stream << shader_code_preamble << tesselation_shader_extension << shader_precision << std::endl
		   << "/* Sampling tesselation evaluation shader */" << std::endl;

	/* layout(point_mode) in; */
	stream << tesselation_evaluation_shader_layout;

	/* uniform samplerType sampler */
	stream << shader_uniform << "highp " << sampler_type_str << sampler_name << ";" << std::endl;

	stream << std::endl;

	/* in type attribute[]*/
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << prev_stage_output
			   << routine_attribute_definitions[i].name << "[];" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << prev_stage_output
			   << type_attribute_definitions[i].name << "[];" << std::endl;
	}

	stream << std::endl;

	/* out vec4 tes_in_color */
	stream << interpolation_type << shader_output << color_type << fragment_shader_input << ";" << std::endl;

	stream << std::endl;

	/* Body */
	stream << tesselation_evaluation_shader_sampling_body_code;

	/* Sampling code */
	stream << sampling_code;

	stream << "}" << std::endl << std::endl;

	/* Store result */
	out_tesselation_evaluation_shader_code = stream.str();
}

/** Prepare code for sampling vertex shader
 *
 *  @param sampler_type           Type of sampler
 *  @param sampling_function      Type of sampling function
 *  @param out_vertex_shader_code Storage for code
 **/
void TextureCubeMapArraySamplingTest::getSamplingVertexShaderCode(const samplerType&	  sampler_type,
																  const samplingFunction& sampling_function,
																  std::string&			  out_vertex_shader_code)
{
	const glw::GLchar*		   color_type					 = 0;
	const glw::GLchar*		   interpolation_type			 = 0;
	bool					   is_shadow_sampler			 = false;
	glw::GLuint				   n_components					 = 0;
	glw::GLuint				   n_routine_attributes			 = 0;
	glw::GLuint				   n_type_attributes			 = 0;
	const attributeDefinition* routine_attribute_definitions = 0;
	const attributeDefinition* type_attribute_definitions	= 0;
	const glw::GLchar*		   sampler_type_str				 = 0;
	std::string				   sampling_code;
	std::stringstream		   stream;

	/* Get attributes for sampling function */
	getAttributes(sampling_function, routine_attribute_definitions, n_routine_attributes);
	getAttributes(sampler_type, type_attribute_definitions, n_type_attributes);

	/* Get type for color variables */
	getColorType(sampler_type, color_type, interpolation_type, sampler_type_str, n_components, is_shadow_sampler);

	/* Get sampling code */
	if (false == is_shadow_sampler)
	{
		getSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_input, 0,
								fragment_shader_input, 0, sampler_name, sampling_code);
	}
	else
	{
		getShadowSamplingFunctionCall(sampling_function, color_type, n_components, vertex_shader_input, 0,
									  fragment_shader_input, 0, sampler_name, sampling_code);
	}

	/* Preamble */
	stream << shader_code_preamble << shader_precision << "/* Sampling vertex shader */" << std::endl;

	/* uniform samplerType sampler */
	stream << shader_uniform << "highp " << sampler_type_str << sampler_name << ";" << std::endl;

	stream << std::endl;

	/* in vec4 vs_in_position */
	stream << shader_input << type_vec4 << vertex_shader_input << vertex_shader_position << ";" << std::endl;

	/* in type attribute */
	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		stream << shader_input << routine_attribute_definitions[i].type << vertex_shader_input
			   << routine_attribute_definitions[i].name << ";" << std::endl;
	}
	for (glw::GLuint i = 0; i < n_type_attributes; ++i)
	{
		stream << shader_input << type_attribute_definitions[i].type << vertex_shader_input
			   << type_attribute_definitions[i].name << ";" << std::endl;
	}

	stream << std::endl;

	/* out vec4 fs_in_color; */
	stream << interpolation_type << shader_output << color_type << fragment_shader_input << ";" << std::endl;

	stream << std::endl;

	/* Body */
	stream << vertex_shader_body_code;

	/* Sampling code */
	stream << sampling_code;

	stream << "}" << std::endl << std::endl;

	/* Store result */
	out_vertex_shader_code = stream.str();
}

/** Prepare shadow sampling code
 *
 *  @param sampling_function     Type of sampling function
 *  @param color_type            Type of color
 *  @param n_components          Number of components
 *  @param attribute_name_prefix Prefix for attributes
 *  @param attribute_index       Index for attributes
 *  @param color_variable_name   Name of color variable
 *  @param color_variable_index  Index for color variable
 *  @param sampler_name_p        Name of sampler
 *  @param out_code              Result code
 **/
void TextureCubeMapArraySamplingTest::getShadowSamplingFunctionCall(
	samplingFunction sampling_function, const glw::GLchar* color_type, glw::GLuint n_components,
	const glw::GLchar* attribute_name_prefix, const glw::GLchar* attribute_index,
	const glw::GLchar* color_variable_name, const glw::GLchar* color_variable_index, const glw::GLchar* sampler_name_p,
	std::string& out_code)
{
	std::stringstream stream;

	switch (sampling_function)
	{
	case Texture:
		/* fs_in_color = texture(sampler, vs_out_texture_coordinates); */
		stream << "    " << var2str(0, color_variable_name, color_variable_index);

		stream << " = " << texture_func << "(" << sampler_name_p;

		stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index) << ", "
			   << var2str(attribute_name_prefix, attribute_refZ, attribute_index);

		stream << ");" << std::endl;
		break;
	case TextureLod:
		/* fs_in_color = textureLod(sampler, vs_out_texture_coordinates, lod); */
		stream << "    " << var2str(0, color_variable_name, color_variable_index);

		stream << " = " << textureLod_func << "(" << sampler_name_p;

		stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index) << ", "
			   << var2str(attribute_name_prefix, attribute_lod, attribute_index) << ", "
			   << var2str(attribute_name_prefix, attribute_refZ, attribute_index);

		stream << ");" << std::endl;
		break;
	case TextureGrad:
		/* fs_in_color = textureGrad(sampler, vs_out_texture_coordinates, vs_out_grad_x, vs_out_grad_y); */
		throw tcu::NotSupportedError("textureGrad operation is not available for samplerCubeArrayShadow", "", __FILE__,
									 __LINE__);
		break;
	case TextureGather:
		if (4 == n_components)
		{
			/**
			 *  color_type component_0 = textureGather(sampler, vs_out_texture_coordinates, 0);
			 *  color_type component_1 = textureGather(sampler, vs_out_texture_coordinates, 1);
			 *  color_type component_2 = textureGather(sampler, vs_out_texture_coordinates, 2);
			 *  color_type component_3 = textureGather(sampler, vs_out_texture_coordinates, 3);
			 *  fs_in_color = color_type(component_0.r, component_1.g, component_2.b, component_3.a);
			 **/
			for (glw::GLuint i = 0; i < 4; ++i)
			{
				stream << "    " << color_type << "component_" << i;

				stream << " = " << textureGather_func << "(" << sampler_name_p;

				stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index) << ", "
					   << var2str(attribute_name_prefix, attribute_refZ, attribute_index);

				stream << ");" << std::endl;
			}

			stream << "    " << var2str(0, color_variable_name, color_variable_index);

			stream << " = " << color_type << "(component_0.r, "
				   << "component_1.g, "
				   << "component_2.b, "
				   << "component_3.a);" << std::endl;
		}
		else
		{
			stream << "    " << var2str(0, color_variable_name, color_variable_index);

			stream << " = " << textureGather_func << "(" << sampler_name_p;

			stream << ", " << var2str(attribute_name_prefix, attribute_texture_coordinate, attribute_index) << ", "
				   << var2str(attribute_name_prefix, attribute_refZ, attribute_index);

			stream << ").x;" << std::endl;
		}
		break;
	}

	out_code = stream.str();
}

/** Check if combination of sampler type and sampling function is supported
 *
 *  @param sampler_type      Type of sampler
 *  @param sampling_function Type of sampling function
 *
 *  @return true  When supported
 *          false When not supported
 **/
bool TextureCubeMapArraySamplingTest::isSamplerSupportedByFunction(const samplerType	  sampler_type,
																   const samplingFunction sampling_function)
{
	if ((Depth == sampler_type) && ((TextureLod == sampling_function) || (TextureGrad == sampling_function)))
	{
		return false;
	}

	return true;
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult TextureCubeMapArraySamplingTest::iterate()
{
#if TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION

	for (resolutionsVectorType::iterator resolution		= m_compressed_resolutions.begin(),
										 end_resolution = m_compressed_resolutions.end();
		 end_resolution != resolution; ++resolution)
	{
		prepareDumpForTextureCompression(*resolution);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;

#else  /* TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION */

	if (false == m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	// These shader stages are always supported
	m_shaders.push_back(shaderConfiguration(Compute, GL_POINTS, "Compute"));
	m_shaders.push_back(shaderConfiguration(Fragment, GL_POINTS, "Fragment"));
	m_shaders.push_back(shaderConfiguration(Vertex, GL_POINTS, "Vertex"));

	// Check if geometry shader is supported
	if (true == m_is_geometry_shader_extension_supported)
	{
		m_shaders.push_back(shaderConfiguration(Geometry, GL_POINTS, "Geometry"));
	}

	// Check if tesselation shaders are supported
	if (true == m_is_tessellation_shader_supported)
	{
		m_shaders.push_back(shaderConfiguration(Tesselation_Control, m_glExtTokens.PATCHES, "Tesselation_Control"));
		m_shaders.push_back(
			shaderConfiguration(Tesselation_Evaluation, m_glExtTokens.PATCHES, "Tesselation_Evaluation"));
	}

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_framebuffer_object_id);

	if (true == m_is_tessellation_shader_supported)
	{
		gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	}

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);

	testFormats(m_formats, m_resolutions);
	testFormats(m_compressed_formats, m_compressed_resolutions);

	if (true == m_is_tessellation_shader_supported)
	{
		gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);
	}

	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);

	gl.deleteFramebuffers(1, &m_framebuffer_object_id);
	m_framebuffer_object_id = 0;

	m_testCtx.getLog() << tcu::TestLog::Section("Summary", "");
	if ((0 != failed_cases) || (0 != invalid_type_cases))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! Number of found errors: " << failed_cases
						   << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid shaders: " << invalid_shaders
						   << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid programs: " << invalid_programs
						   << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetActiveUniform or glGetProgramResourceiv reported invalid type: "
						   << invalid_type_cases << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Number of executed test cases: " << tested_cases
					   << tcu::TestLog::EndMessage;

	m_testCtx.getLog() << tcu::TestLog::Message << "Total shaders: " << compiled_shaders << tcu::TestLog::EndMessage;

	m_testCtx.getLog() << tcu::TestLog::Message << "Total programs: " << linked_programs << tcu::TestLog::EndMessage;

	m_testCtx.getLog() << tcu::TestLog::EndSection;

	return STOP;
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION */
}

/** Link program
 *
 *  @param info Program information
 **/
void TextureCubeMapArraySamplingTest::link(programDefinition& info)
{
	linked_programs += 1;

	/* Not supported format */
	if (programDefinition::m_invalid_program_object_id == info.getProgramId())
	{
		return;
	}

	if (false == info.link())
	{
		invalid_programs += 1;

		logLinkingLog(info);
		logProgram(info);
	}
}

/** Logs compilation log
 *
 *  @param info Shader information
 **/
void TextureCubeMapArraySamplingTest::logCompilationLog(const shaderDefinition& info)
{
	std::string info_log = getCompilationInfoLog(info.getShaderId());
	m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failure:\n\n"
					   << info_log << tcu::TestLog::EndMessage;
	m_testCtx.getLog() << tcu::TestLog::Message << "Shader source:\n\n" << info.getSource() << tcu::TestLog::EndMessage;
}

/** Logs linkig log
 *
 *  @param info Program information
 **/
void TextureCubeMapArraySamplingTest::logLinkingLog(const programDefinition& info)
{
	glw::GLuint program_object_id = info.getProgramId();

	if (programDefinition::m_invalid_program_object_id == program_object_id)
	{
		return;
	}

	std::string info_log = getLinkingInfoLog(program_object_id);
	m_testCtx.getLog() << tcu::TestLog::Message << "Program linking failure:\n\n"
					   << info_log << tcu::TestLog::EndMessage;
}

/** Logs shaders used by program
 *
 *  @param info Program information
 **/
void TextureCubeMapArraySamplingTest::logProgram(const programDefinition& info)
{
	glw::GLuint program_object_id = info.getProgramId();

	if (programDefinition::m_invalid_program_object_id == program_object_id)
	{
		return;
	}

	tcu::MessageBuilder message = m_testCtx.getLog() << tcu::TestLog::Message;

	message << "Program id: " << program_object_id;

	const shaderDefinition* compute  = info.getShader(Compute);
	const shaderDefinition* fragment = info.getShader(Fragment);
	const shaderDefinition* geometry = info.getShader(Geometry);
	const shaderDefinition* tcs		 = info.getShader(Tesselation_Control);
	const shaderDefinition* tes		 = info.getShader(Tesselation_Evaluation);
	const shaderDefinition* vertex   = info.getShader(Vertex);

	if (0 != compute)
	{
		message << "\nCompute shader:\n" << compute->getSource();
	}

	if (0 != vertex)
	{
		message << "\nVertex shader:\n" << vertex->getSource();
	}

	if (0 != geometry)
	{
		message << "\nGeometry shader:\n" << geometry->getSource();
	}

	if (0 != tcs)
	{
		message << "\nTCS shader:\n" << tcs->getSource();
	}

	if (0 != tes)
	{
		message << "\nTES shader:\n" << tes->getSource();
	}

	if (0 != fragment)
	{
		message << "\nFragment shader:\n" << fragment->getSource();
	}

	message << tcu::TestLog::EndMessage;
}

/** Prepare compressed textures
 *
 *  @param texture    Texture information
 *  @param format     Texture format
 *  @param resolution Texture resolution
 *  @param mutability Texture mutability
 **/
void TextureCubeMapArraySamplingTest::prepareCompresedTexture(const textureDefinition&	texture,
															  const formatDefinition&	 format,
															  const resolutionDefinition& resolution, bool mutability)
{
	static const glw::GLint n_faces = 6;

	const glw::GLint array_length	= resolution.m_depth / n_faces;
	const glw::GLint n_mipmap_levels = getMipmapLevelCount(resolution.m_width, resolution.m_height);
	glw::GLsizei	 texture_width   = 0;
	glw::GLsizei	 texture_height  = 0;

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	texture.bind(GL_TEXTURE_CUBE_MAP_ARRAY);

	if (false == mutability)
	{
		gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, n_mipmap_levels, format.m_source.m_internal_format,
						resolution.m_width, resolution.m_height, resolution.m_depth);

		GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage3D");

		texture_width  = resolution.m_width;
		texture_height = resolution.m_height;

		for (glw::GLint mipmap_level = 0; mipmap_level < n_mipmap_levels; ++mipmap_level)
		{
			const glw::GLubyte* image_data = 0;
			glw::GLuint			image_size = 0;

			getCompressedTexture(resolution.m_width, resolution.m_height, array_length, mipmap_level, image_data,
								 image_size);

			if (0 == image_data)
			{
				throw tcu::InternalError("Invalid compressed texture", "", __FILE__, __LINE__);
			}

			gl.compressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, 0, /* x_offset */
									   0,										   /* y offset */
									   0,										   /* z offset */
									   texture_width, texture_height, resolution.m_depth,
									   format.m_source.m_internal_format, image_size, image_data);

			GLU_EXPECT_NO_ERROR(gl.getError(), "compressedTexSubImage3D");

			texture_width  = de::max(1, texture_width / 2);
			texture_height = de::max(1, texture_height / 2);
		}
	}
	else
	{
		texture_width  = resolution.m_width;
		texture_height = resolution.m_height;

		for (glw::GLint mipmap_level = 0; mipmap_level < n_mipmap_levels; ++mipmap_level)
		{
			const glw::GLubyte* image_data = 0;
			glw::GLuint			image_size = 0;

			getCompressedTexture(resolution.m_width, resolution.m_height, array_length, mipmap_level, image_data,
								 image_size);

			if (0 == image_data)
			{
				throw tcu::InternalError("Invalid compressed texture", "", __FILE__, __LINE__);
			}

			gl.compressedTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, format.m_source.m_internal_format,
									texture_width, texture_height, resolution.m_depth, 0 /* border */, image_size,
									image_data);

			GLU_EXPECT_NO_ERROR(gl.getError(), "compressedTexImage3D");

			texture_width  = de::max(1, texture_width / 2);
			texture_height = de::max(1, texture_height / 2);
		}
	}
}

/** Prepare not comporessed textures
 *
 *  @param texture    Texture information
 *  @param format     Texture format
 *  @param resolution Texture resolution
 *  @param mutability Texture mutability
 **/
void TextureCubeMapArraySamplingTest::prepareTexture(const textureDefinition&	texture,
													 const formatDefinition&	 texture_format,
													 const resolutionDefinition& resolution, bool mutability)
{
	static const glw::GLint n_faces = 6;

	const glw::GLint n_elements		 = resolution.m_depth / n_faces;
	const glw::GLint n_mipmap_levels = getMipmapLevelCount(resolution.m_width, resolution.m_height);
	glw::GLsizei	 texture_width   = 0;
	glw::GLsizei	 texture_height  = 0;

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	texture.bind(GL_TEXTURE_CUBE_MAP_ARRAY);

	if (false == mutability)
	{
		gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, n_mipmap_levels, texture_format.m_source.m_internal_format,
						resolution.m_width, resolution.m_height, resolution.m_depth);
	}
	else
	{
		texture_width  = resolution.m_width;
		texture_height = resolution.m_height;

		for (glw::GLint mipmap_level = 0; mipmap_level < n_mipmap_levels; ++mipmap_level)
		{
			gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, texture_format.m_source.m_internal_format,
						  texture_width, texture_height, resolution.m_depth, 0 /* border */,
						  texture_format.m_source.m_format, texture_format.m_source.m_type, 0 /* data */);

			texture_width  = de::max(1, texture_width / 2);
			texture_height = de::max(1, texture_height / 2);
		}
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to allocate storage for texture");

	texture_width  = resolution.m_width;
	texture_height = resolution.m_height;

	for (glw::GLint mipmap_level = 0; mipmap_level < n_mipmap_levels; ++mipmap_level)
	{
		for (glw::GLint element_index = 0; element_index < n_elements; ++element_index)
		{
			for (glw::GLint face = 0; face < n_faces; ++face)
			{
				prepareTextureFace(gl, face, element_index, mipmap_level, n_elements, n_mipmap_levels,
								   texture_format.m_source.m_format, texture_format.m_source.m_type, texture_width,
								   texture_height);
			}
		}

		texture_width  = de::max(1, texture_width / 2);
		texture_height = de::max(1, texture_height / 2);
	}

	// not texture filterable formats
	if ((texture_format.m_source.m_internal_format == GL_RGBA32UI) ||
		(texture_format.m_source.m_internal_format == GL_RGBA32I) ||
		(texture_format.m_source.m_internal_format == GL_STENCIL_INDEX8) ||
		(texture_format.m_source.m_internal_format == GL_DEPTH_COMPONENT32F))
	{
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	}
}

/** Setup shader storabe buffer for use with compute shader
 *
 *  @param attribute  Attribute information
 *  @param buffers    Collection of buffers
 *  @param program_id Program id
 **/
void TextureCubeMapArraySamplingTest::setupSharedStorageBuffer(const attributeDefinition& attribute,
															   const bufferCollection& buffers, glw::GLuint program_id)
{
	(void)program_id;

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::string				attribute_name = attribute.name;
	const bufferDefinition* buffer		   = 0;

	switch (attribute.attribute_id)
	{
	case Position:
		buffer = &buffers.postion;
		break;
	case TextureCoordinates:
		buffer = &buffers.texture_coordinate;
		break;
	case TextureCoordinatesForGather:
		buffer = &buffers.texture_coordinate_for_gather;
		break;
	case Lod:
		buffer = &buffers.lod;
		break;
	case GradX:
		buffer = &buffers.grad_x;
		break;
	case GradY:
		buffer = &buffers.grad_y;
		break;
	case RefZ:
		buffer = &buffers.refZ;
		break;
	}

	buffer->bind(GL_SHADER_STORAGE_BUFFER, attribute.binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup shared storage block");
}

/** Setup shader storabe buffers for use with compute shader
 *
 *  @param format            Texture format
 *  @param sampling_function Sampling routine
 *  @param buffers           Collection of buffers
 *  @param program_id        Program id
 **/
void TextureCubeMapArraySamplingTest::setupSharedStorageBuffers(const formatDefinition& format,
																const samplingFunction& sampling_function,
																const bufferCollection& buffers, glw::GLuint program_id)
{
	const attributeDefinition* format_attributes	= 0;
	glw::GLuint				   n_format_attributes  = 0;
	glw::GLuint				   n_routine_attributes = 0;
	const attributeDefinition* routine_attributes   = 0;

	getAttributes(format.m_sampler_type, format_attributes, n_format_attributes);
	getAttributes(sampling_function, routine_attributes, n_routine_attributes);

	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		setupSharedStorageBuffer(routine_attributes[i], buffers, program_id);
	}

	for (glw::GLuint i = 0; i < n_format_attributes; ++i)
	{
		setupSharedStorageBuffer(format_attributes[i], buffers, program_id);
	}
}

/** Execute tests for set of formats and resolutions
 *
 *  @param formats     Set of texture formats
 *  @param resolutions Set of texture resolutions
 **/
void TextureCubeMapArraySamplingTest::testFormats(formatsVectorType& formats, resolutionsVectorType& resolutions)
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (formatsVectorType::iterator format = formats.begin(), end_format = formats.end(); end_format != format;
		 ++format)
	{
		shaderCollectionForTextureFormat shader_collection;
		programCollectionForFormat		 program_collection;

		shader_collection.init(gl, *format, m_functions, *this);
		bool isContextES = (glu::isContextTypeES(m_context.getRenderContext().getType()));
		program_collection.init(gl, shader_collection, *this, isContextES);

		for (mutablitiesVectorType::iterator mutability = m_mutabilities.begin(), end_muatbility = m_mutabilities.end();
			 end_muatbility != mutability; ++mutability)
		{
			for (resolutionsVectorType::iterator resolution = resolutions.begin(), end_resolution = resolutions.end();
				 end_resolution != resolution; ++resolution)
			{
				textureDefinition texture;
				texture.init(gl);

				try
				{
					if (false == format->m_source.m_is_compressed)
					{
						prepareTexture(texture, *format, *resolution, *mutability);
					}
					else
					{
						prepareCompresedTexture(texture, *format, *resolution, *mutability);
					}
				}
#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG
				catch (std::exception& exc)
				{
					m_testCtx.getLog() << tcu::TestLog::Section("Exception during texture creation", exc.what());

					m_testCtx.getLog() << tcu::TestLog::Message << "Format: " << format->m_name
									   << ", Mutability: " << *mutability << ", W: " << resolution->m_width
									   << ", H: " << resolution->m_height << tcu::TestLog::EndMessage;

					m_testCtx.getLog() << tcu::TestLog::EndSection;
#else  /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG */
				catch (...)
				{
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG */
					continue;
				}

				testTexture(*format, *mutability, *resolution, texture, program_collection);
			}
		}
	}
}

/** Execute tests for given texture
 *
 *  @param format             Texture format
 *  @param mutability         Texture mutabilibty
 *  @param resolution         Texture resolution
 *  @param texture            Textue information
 *  @param shader_collection  Collection of shaders
 *  @param program_collection Collection of programs
 **/
void TextureCubeMapArraySamplingTest::testTexture(const formatDefinition& format, bool mutability,
												  const resolutionDefinition& resolution, textureDefinition& texture,
												  programCollectionForFormat& program_collection)
{
	std::vector<unsigned char> result_image;

	const glw::GLuint image_width  = 3 * resolution.m_depth;
	const glw::GLuint image_height = 3;
	const glw::GLuint estimated_image_size =
		static_cast<glw::GLuint>(image_width * image_height * 4 /* components */ * sizeof(glw::GLuint));

	result_image.resize(estimated_image_size);

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bufferCollection buffers;
	buffers.init(gl, format, resolution);

	for (samplingFunctionsVectorType::iterator function = m_functions.begin(), end_function = m_functions.end();
		 end_function != function; ++function)
	{
		for (shadersVectorType::iterator shader = m_shaders.begin(), end_shader = m_shaders.end(); end_shader != shader;
			 ++shader)
		{
			const programCollectionForFunction* programs		  = 0;
			const programDefinition*			program			  = 0;
			glw::GLuint							program_object_id = programDefinition::m_invalid_program_object_id;
			textureDefinition					color_attachment;

			programs		  = program_collection.getPrograms(function->m_function);
			program			  = programs->getProgram(shader->m_type);
			program_object_id = program->getProgramId();

			if (programDefinition::m_invalid_program_object_id == program_object_id)
			{
				continue;
			}

			tested_cases += 1;

			color_attachment.init(gl);

			if (Compute != shader->m_type)
			{
				setupFramebufferWithTextureAsAttachment(m_framebuffer_object_id, color_attachment.getTextureId(),
														format.m_destination.m_internal_format, image_width,
														image_height);
			}
			else
			{
				color_attachment.bind(GL_TEXTURE_2D);
				gl.texStorage2D(GL_TEXTURE_2D, 1, format.m_destination.m_internal_format, image_width, image_height);
				gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				color_attachment.setupImage(0, format.m_destination.m_internal_format);
			}

			try
			{
				gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
				gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_object_id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

				gl.useProgram(program_object_id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "Failed glUseProgram call.");

				texture.setupSampler(0, sampler_name, program_object_id, format.m_sampler_type == Depth);

				if (Compute != shader->m_type)
				{
					vertexArrayObjectDefinition vao;
					vao.init(gl, format, function->m_function, buffers, program_object_id);
					draw(program_object_id, shader->m_primitive_type, image_width * image_height,
						 format.m_destination.m_internal_format);
				}
				else
				{
					setupSharedStorageBuffers(format, function->m_function, buffers, program_object_id);
					dispatch(program_object_id, image_width, image_height);
				}

				gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer_object_id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

				color_attachment.bind(GL_TEXTURE_2D);
				gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
										color_attachment.getTextureId(), 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

				gl.viewport(0, 0, 3 * resolution.m_depth, 3);
				GLU_EXPECT_NO_ERROR(gl.getError(), "viewport");

				gl.memoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT);
				gl.readPixels(0 /* x */, 0 /* y */, image_width, image_height, format.m_destination.m_format,
							  format.m_destination.m_type, &result_image[0]);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

				/* GL_DEPTH_COMPONENT is nominally R32F, however R32F is not renderable, so we convert to
				 * RGBA8 instead.  Convert the red channel back to R32F for comparison.
				 */
				if (format.m_source.m_format == GL_DEPTH_COMPONENT)
				{
					unsigned char* p = (unsigned char*)&result_image[0];
					float*		   f = (float*)&result_image[0];

					for (unsigned int i = 0; i < image_width * image_height; i++)
					{
						*f = (float)p[0] / 255.0f;
						p += 4;
						f += 1;
					}
				}

				/* GL_STENCIL_INDEX is nominally one-channel format, however ReadPixels supports only RGBA formats.
				 * Convert the RGBA image to R for comparison.
				 */
				if (format.m_source.m_format == GL_STENCIL_INDEX && format.m_destination.m_format == GL_RGBA_INTEGER)
				{
					unsigned int* pRGBA = (unsigned int*)&result_image[0];
					unsigned int* pR	= (unsigned int*)&result_image[0];
					for (unsigned int i = 0; i < image_width * image_height; i++)
					{
						*pR = pRGBA[0];
						pR += 1;
						pRGBA += 4;
					}
				}

				glw::GLuint get_type_api_status =
					checkUniformAndResourceApi(program_object_id, sampler_name, format.m_sampler_type);
				bool verification_result = verifyResult(format, resolution, function->m_function, &result_image[0]);

				if ((true == verification_result) && (0 == get_type_api_status))
				{
#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_PASS_LOG
					m_testCtx.getLog() << tcu::TestLog::Message << "Valid result. "
									   << " Format: " << format.m_name << ", Mutability: " << mutability
									   << ", Sampling shader: " << shader->m_name
									   << ", Sampling function: " << function->m_name << ", W: " << resolution.m_width
									   << ", H: " << resolution.m_height << tcu::TestLog::EndMessage;
#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_PASS_PROGRAM_LOG
					logProgram(*program);
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_PASS_PROGRAM_LOG */
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_PASS_LOG */
				}
				else
				{
#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG
					m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

					if (true != verification_result)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid image" << tcu::TestLog::EndMessage;
					}

					if (0 != get_type_api_status)
					{
						if (0 != (m_get_type_api_status_uniform & get_type_api_status))
						{
							m_testCtx.getLog() << tcu::TestLog::Message
											   << "glGetActiveUniform returns wrong type for sampler"
											   << tcu::TestLog::EndMessage;
						}

						if (0 != (m_get_type_api_status_program_resource & get_type_api_status))
						{
							m_testCtx.getLog() << tcu::TestLog::Message
											   << "glGetProgramResourceiv returns wrong type for sampler"
											   << tcu::TestLog::EndMessage;
						}
					}

					m_testCtx.getLog() << tcu::TestLog::Message << "Format: " << format.m_name
									   << ", Mutability: " << mutability << ", Sampling shader: " << shader->m_name
									   << ", Sampling function: " << function->m_name << ", W: " << resolution.m_width
									   << ", H: " << resolution.m_height << tcu::TestLog::EndMessage;

#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_PROGRAM_LOG
					logProgram(*program);
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_PROGRAM_LOG */

					m_testCtx.getLog() << tcu::TestLog::EndSection;
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG */

					if (false == verification_result)
					{
						failed_cases += 1;
					}

					if (0 != get_type_api_status)
					{
						invalid_type_cases += 1;
					}
				}
			}
#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG
			catch (std::exception& exc)
			{
				m_testCtx.getLog() << tcu::TestLog::Section("Exception during test execution", exc.what());

				m_testCtx.getLog() << tcu::TestLog::Message << "Format: " << format.m_name
								   << ", Mutability: " << mutability << ", W: " << resolution.m_width
								   << ", H: " << resolution.m_height << tcu::TestLog::EndMessage;

#if TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_PROGRAM_LOG
				logProgram(*program);
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_PROGRAM_LOG */

				m_testCtx.getLog() << tcu::TestLog::EndSection;
#else  /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG */
			catch (...)
			{
#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_IS_FAIL_LOG */

				failed_cases += 1;
			}
			//run()
		}
	}
}

/** Verify that rendered image match expectations
 *
 *  @param format            Texture format
 *  @param resolution        Texture resolution
 *  @param shader_type       Shader type
 *  @param sampling_function Type of sampling function
 *  @param data              Image data
 **/
bool TextureCubeMapArraySamplingTest::verifyResult(const formatDefinition&	 format,
												   const resolutionDefinition& resolution,
												   const samplingFunction sampling_function, unsigned char* data)
{
	componentProvider component_provider = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

	switch (sampling_function)
	{
	case Texture:
	case TextureGather:
		component_provider.getColorFloatComponents = getExpectedColorFloatComponentsForTexture;
		component_provider.getColorUByteComponents = getExpectedColorIntComponentsForTexture<glw::GLubyte>;
		component_provider.getColorUintComponents  = getExpectedColorIntComponentsForTexture<glw::GLuint>;
		component_provider.getColorIntComponents   = getExpectedColorIntComponentsForTexture<glw::GLint>;
		component_provider.getDepthComponents	  = getExpectedDepthComponentsForTexture;
		component_provider.getStencilComponents	= getExpectedStencilComponentsForTexture;
		component_provider.getCompressedComponents = getExpectedCompressedComponentsForTexture;
		break;
	case TextureLod:
	case TextureGrad:
		component_provider.getColorFloatComponents = getExpectedColorFloatComponentsForTextureLod;
		component_provider.getColorUByteComponents = getExpectedColorIntComponentsForTextureLod<glw::GLubyte>;
		component_provider.getColorUintComponents  = getExpectedColorIntComponentsForTextureLod<glw::GLuint>;
		component_provider.getColorIntComponents   = getExpectedColorIntComponentsForTextureLod<glw::GLint>;
		component_provider.getDepthComponents	  = getExpectedDepthComponentsForTextureLod;
		component_provider.getStencilComponents	= getExpectedStencilComponentsForTextureLod;
		component_provider.getCompressedComponents = getExpectedCompressedComponentsForTextureLod;
		break;
	}

	return verifyResultHelper(format, resolution, component_provider, data);
}

/** Verify that rendered image match expectations
 *
 *  @param format            Texture format
 *  @param resolution        Texture resolution
 *  @param shader_type       Shader type
 *  @param sampling_function Type of sampling function
 *  @param data              Image data
 **/
bool TextureCubeMapArraySamplingTest::verifyResultHelper(const formatDefinition&	 format,
														 const resolutionDefinition& resolution,
														 const componentProvider&	component_provider,
														 unsigned char*				 data)
{
	const glw::GLuint n_mipmap_levels = getMipmapLevelCount(resolution.m_width, resolution.m_height);
	const glw::GLuint n_layers		  = resolution.m_depth / 6;

	bool result = false;

	if (GL_RGBA == format.m_source.m_format)
	{
		if (GL_UNSIGNED_BYTE == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLubyte, 4, 3, 3>(n_mipmap_levels, n_layers,
															  component_provider.getColorUByteComponents, data);
		}
		else if (GL_FLOAT == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLfloat, 4, 3, 3>(n_mipmap_levels, n_layers,
															  component_provider.getColorFloatComponents, data);
		}
		else if (GL_COMPRESSED_RGBA8_ETC2_EAC == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLubyte, 4, 3, 3>(n_mipmap_levels, n_layers,
															  component_provider.getCompressedComponents, data);
		}
	}
	else if (GL_RGBA_INTEGER == format.m_source.m_format)
	{
		if (GL_UNSIGNED_INT == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLuint, 4, 3, 3>(n_mipmap_levels, n_layers,
															 component_provider.getColorUintComponents, data);
		}
		else if (GL_INT == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLint, 4, 3, 3>(n_mipmap_levels, n_layers,
															component_provider.getColorIntComponents, data);
		}
	}
	if (GL_DEPTH_COMPONENT == format.m_source.m_format)
	{
		if (GL_FLOAT == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLfloat, 1, 3, 3>(n_mipmap_levels, n_layers,
															  component_provider.getDepthComponents, data);
		}
	}
	if (GL_STENCIL_INDEX == format.m_source.m_format)
	{
		if (GL_UNSIGNED_BYTE == format.m_source.m_type)
		{
			result = verifyResultImage<glw::GLuint, 1, 3, 3>(n_mipmap_levels, n_layers,
															 component_provider.getStencilComponents, data);
		}
	}

	return result;
}

/****************************************************************************/

/** Initialize buffer collection
 *
 *  @param gl         GL functions
 *  @param format     Texture format
 *  @param resolution Texture resolution
 **/
void TextureCubeMapArraySamplingTest::bufferCollection::init(const glw::Functions& gl, const formatDefinition& format,
															 const resolutionDefinition& resolution)
{
	(void)format;

	static const glw::GLuint n_faces						  = 6;
	static const glw::GLuint n_lods_components				  = 1;
	static const glw::GLuint n_grad_components				  = 4;
	static const glw::GLuint n_points_per_face				  = 9;
	static const glw::GLuint n_position_components			  = 4;
	static const glw::GLuint n_refZ_components				  = 1;
	static const glw::GLuint n_texture_coordinates_components = 4;

	const glw::GLuint n_layers = resolution.m_depth / n_faces;

	const glw::GLuint n_points_per_layer = n_points_per_face * n_faces;
	const glw::GLuint n_total_points	 = n_points_per_layer * n_layers;

	const glw::GLuint n_position_step_per_face			  = n_position_components * n_points_per_face;
	const glw::GLuint n_texture_coordinates_step_per_face = n_texture_coordinates_components * n_points_per_face;
	const glw::GLuint n_lods_step_per_face				  = n_lods_components * n_points_per_face;
	const glw::GLuint n_grad_step_per_face				  = n_grad_components * n_points_per_face;
	const glw::GLuint n_refZ_step_per_face				  = n_refZ_components * n_points_per_face;

	const glw::GLuint n_position_step_per_layer			   = n_faces * n_position_step_per_face;
	const glw::GLuint n_texture_coordinates_step_per_layer = n_faces * n_texture_coordinates_step_per_face;
	const glw::GLuint n_lods_step_per_layer				   = n_faces * n_lods_step_per_face;
	const glw::GLuint n_grad_step_per_layer				   = n_faces * n_grad_step_per_face;
	const glw::GLuint n_refZ_step_per_layer				   = n_faces * n_refZ_step_per_face;

	const glw::GLuint texture_width	= resolution.m_width;
	const glw::GLuint texture_height   = resolution.m_height;
	const glw::GLuint n_mip_map_levels = getMipmapLevelCount(texture_width, texture_height);

	std::vector<glw::GLfloat> position_buffer_data;
	std::vector<glw::GLfloat> texture_coordinate_buffer_data;
	std::vector<glw::GLfloat> texture_coordinate_for_gather_buffer_data;
	std::vector<glw::GLfloat> lod_buffer_data;
	std::vector<glw::GLfloat> grad_x_buffer_data;
	std::vector<glw::GLfloat> grad_y_buffer_data;
	std::vector<glw::GLfloat> refZ_buffer_data;

	position_buffer_data.resize(n_total_points * n_position_components);
	texture_coordinate_buffer_data.resize(n_total_points * n_texture_coordinates_components);
	texture_coordinate_for_gather_buffer_data.resize(n_total_points * n_texture_coordinates_components);
	lod_buffer_data.resize(n_total_points * n_lods_components);
	grad_x_buffer_data.resize(n_total_points * n_grad_components);
	grad_y_buffer_data.resize(n_total_points * n_grad_components);
	refZ_buffer_data.resize(n_total_points * n_refZ_components);

	/* Prepare data */
	for (glw::GLuint layer = 0; layer < n_layers; ++layer)
	{
		const glw::GLfloat layer_coordinate = (float)layer;

		for (glw::GLuint face = 0; face < n_faces; ++face)
		{
			/* Offsets */
			const glw::GLuint position_offset = layer * n_position_step_per_layer + face * n_position_step_per_face;
			const glw::GLuint texture_coordinates_offset =
				layer * n_texture_coordinates_step_per_layer + face * n_texture_coordinates_step_per_face;
			const glw::GLuint lods_offset = layer * n_lods_step_per_layer + face * n_lods_step_per_face;
			const glw::GLuint grad_offset = layer * n_grad_step_per_layer + face * n_grad_step_per_face;
			const glw::GLuint refZ_offset = layer * n_refZ_step_per_layer + face * n_refZ_step_per_face;

			/* Prepare data */
			preparePositionForFace(&position_buffer_data[0] + position_offset, face, layer, n_layers * n_faces);
			prepareTextureCoordinatesForFace(&texture_coordinate_buffer_data[0] + texture_coordinates_offset,
											 texture_width, texture_height, layer_coordinate, face);
			prepareTextureCoordinatesForGatherForFace(&texture_coordinate_for_gather_buffer_data[0] +
														  texture_coordinates_offset,
													  texture_width, texture_height, layer_coordinate, face);
			prepareLodForFace(&lod_buffer_data[0] + lods_offset, n_mip_map_levels);
			prepareGradXForFace(&grad_x_buffer_data[0] + grad_offset, face,
								&texture_coordinate_buffer_data[0] + texture_coordinates_offset, texture_width);
			prepareGradYForFace(&grad_y_buffer_data[0] + grad_offset, face,
								&texture_coordinate_buffer_data[0] + texture_coordinates_offset, texture_width);
			prepareRefZForFace(&refZ_buffer_data[0] + refZ_offset, n_mip_map_levels, face, layer, n_layers);
		}
	}

	/* Initialize buffers */
	postion.init(gl, (glw::GLsizeiptr)(position_buffer_data.size() * sizeof(glw::GLfloat)), &position_buffer_data[0]);
	texture_coordinate.init(gl, (glw::GLsizeiptr)(texture_coordinate_buffer_data.size() * sizeof(glw::GLfloat)),
							&texture_coordinate_buffer_data[0]);
	texture_coordinate_for_gather.init(
		gl, (glw::GLsizeiptr)(texture_coordinate_for_gather_buffer_data.size() * sizeof(glw::GLfloat)),
		&texture_coordinate_for_gather_buffer_data[0]);
	lod.init(gl, (glw::GLsizeiptr)(lod_buffer_data.size() * sizeof(glw::GLfloat)), &lod_buffer_data[0]);
	grad_x.init(gl, (glw::GLsizeiptr)(grad_x_buffer_data.size() * sizeof(glw::GLfloat)), &grad_x_buffer_data[0]);
	grad_y.init(gl, (glw::GLsizeiptr)(grad_y_buffer_data.size() * sizeof(glw::GLfloat)), &grad_y_buffer_data[0]);
	refZ.init(gl, (glw::GLsizeiptr)(refZ_buffer_data.size() * sizeof(glw::GLfloat)), &refZ_buffer_data[0]);
}

/** Constructor.
 *
 **/
TextureCubeMapArraySamplingTest::bufferDefinition::bufferDefinition()
	: m_gl(0), m_buffer_object_id(m_invalid_buffer_object_id)
{
}

/** Destructor
 *
 **/
TextureCubeMapArraySamplingTest::bufferDefinition::~bufferDefinition()
{
	if (m_invalid_buffer_object_id != m_buffer_object_id)
	{
		if (0 != m_gl)
		{
			m_gl->deleteBuffers(1, &m_buffer_object_id);

			m_gl = 0;
		}

		m_buffer_object_id = m_invalid_buffer_object_id;
	}
}

/** Bind buffer
 *
 *  @param target Target for bind
 **/
void TextureCubeMapArraySamplingTest::bufferDefinition::bind(glw::GLenum target) const
{
	if (m_invalid_buffer_object_id == m_buffer_object_id)
	{
		throw tcu::InternalError("Invalid buffer object id used", "", __FILE__, __LINE__);
	}

	m_gl->bindBuffer(target, m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to bind buffer.");
}

/** Bind buffer
 *
 *  @param target Target for bind
 *  @param index  Index for target
 **/
void TextureCubeMapArraySamplingTest::bufferDefinition::bind(glw::GLenum target, glw::GLuint index) const
{
	if (m_invalid_buffer_object_id == m_buffer_object_id)
	{
		throw tcu::InternalError("Invalid buffer object id used", "", __FILE__, __LINE__);
	}

	m_gl->bindBufferBase(target, index, m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to bind buffer.");
}

/** Initialize buffer definition
 *
 *  @param gl          GL functions
 *  @param buffer_size Size of buffer
 *  @param buffer_data Buffer data
 **/
void TextureCubeMapArraySamplingTest::bufferDefinition::init(const glw::Functions& gl, glw::GLsizeiptr buffer_size,
															 glw::GLvoid* buffer_data)
{
	m_gl = &gl;

	m_gl->genBuffers(1, &m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to generate buffer.");

	m_gl->bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to bind buffer.");

	m_gl->bufferData(GL_ARRAY_BUFFER, buffer_size, buffer_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to update buffer's data.");

	m_gl->bindBuffer(GL_ARRAY_BUFFER, 0);
	m_gl->getError();
}

/** Constructor
 *
 *  @param internal_format Internal format
 *  @param format          Format
 *  @param type            Type
 *  @param is_compressed   If format is compressed
 *  @param sampler_type    Type of sampler
 *  @param name            Name of format
 **/
TextureCubeMapArraySamplingTest::formatDefinition::formatDefinition(glw::GLenum internal_format, glw::GLenum format,
																	glw::GLenum type, bool is_compressed,
																	samplerType sampler_type, const glw::GLchar* name)
	: m_source(internal_format, format, type, is_compressed)
	, m_destination(internal_format, format, type, false /* is_compressed*/)
	, m_sampler_type(sampler_type)
	, m_name(name)
{
}

/** Constructor
 *
 *  @param src_internal_format Internal format of source image
 *  @param src_format          Format of source image
 *  @param src_type            Type of source image
 *  @param src_is_compressed   If format of source image is compressed
 *  @param dst_internal_format Internal format of destination image
 *  @param dst_format          Format of destination image
 *  @param dst_type            Type of destination image
 *  @param sampler_type        Type of sampler
 *  @param name                Name of format
 **/
TextureCubeMapArraySamplingTest::formatDefinition::formatDefinition(glw::GLenum src_internal_format,
																	glw::GLenum src_format, glw::GLenum src_type,
																	bool		src_is_compressed,
																	glw::GLenum dst_internal_format,
																	glw::GLenum dst_format, glw::GLenum dst_type,
																	samplerType sampler_type, const glw::GLchar* name)
	: m_source(src_internal_format, src_format, src_type, src_is_compressed)
	, m_destination(dst_internal_format, dst_format, dst_type, false /* is_compressed*/)
	, m_sampler_type(sampler_type)
	, m_name(name)
{
}

/** Constructor
 *
 *  @param internal_format Internal format
 *  @param format          Format
 *  @param type            Type
 *  @param is_compressed   If format is compressed
 **/
TextureCubeMapArraySamplingTest::formatInfo::formatInfo(glw::GLenum internal_format, glw::GLenum format,
														glw::GLenum type, bool is_compressed)
	: m_internal_format(internal_format), m_format(format), m_type(type), m_is_compressed(is_compressed)
{
}

/** Get collection of programs for sampling function
 *
 *  @param function Type of sampling function
 *
 *  @return Collection of programs for given sampling function
 **/
const TextureCubeMapArraySamplingTest::programCollectionForFunction* TextureCubeMapArraySamplingTest::
	programCollectionForFormat::getPrograms(samplingFunction function) const
{
	switch (function)
	{
	case Texture:
		return &m_programs_for_texture;
		break;
	case TextureLod:
		return &m_programs_for_textureLod;
		break;
	case TextureGrad:
		return &m_programs_for_textureGrad;
		break;
	case TextureGather:
		return &m_programs_for_textureGather;
		break;
	};

	return 0;
}

/** Initialize program collection for format
 *
 *  @param gl                GL functions
 *  @param shader_collection Collection of shaders
 *  @param test              Instance of test class
 **/
void TextureCubeMapArraySamplingTest::programCollectionForFormat::init(
	const glw::Functions& gl, const shaderCollectionForTextureFormat& shader_collection,
	TextureCubeMapArraySamplingTest& test, bool isContextES)
{
	shaderGroup shader_group;

	shader_collection.getShaderGroup(Texture, shader_group);
	m_programs_for_texture.init(gl, shader_group, test, isContextES);

	shader_collection.getShaderGroup(TextureLod, shader_group);
	m_programs_for_textureLod.init(gl, shader_group, test, isContextES);

	shader_collection.getShaderGroup(TextureGrad, shader_group);
	m_programs_for_textureGrad.init(gl, shader_group, test, isContextES);

	shader_collection.getShaderGroup(TextureGather, shader_group);
	m_programs_for_textureGather.init(gl, shader_group, test, isContextES);
}

/** Get program with specified sampling shader
 *
 *  @param shader_type Type of shader
 *
 *  @returns Program information
 **/
const TextureCubeMapArraySamplingTest::programDefinition* TextureCubeMapArraySamplingTest::
	programCollectionForFunction::getProgram(shaderType shader_type) const
{
	switch (shader_type)
	{
	case Compute:
		return &program_with_sampling_compute_shader;
		break;
	case Fragment:
		return &program_with_sampling_fragment_shader;
		break;
	case Geometry:
		return &program_with_sampling_geometry_shader;
		break;
	case Tesselation_Control:
		return &program_with_sampling_tesselation_control_shader;
		break;
	case Tesselation_Evaluation:
		return &program_with_sampling_tesselation_evaluation_shader;
		break;
	case Vertex:
		return &program_with_sampling_vertex_shader;
		break;
	}

	return 0;
}

/** Initialize program collection for sampling function
 *
 *  @param gl           GL functions
 *  @param shader_group Group of shader compatible with sampling function
 *  @param test         Instance of test class
 **/
void TextureCubeMapArraySamplingTest::programCollectionForFunction::init(const glw::Functions&			  gl,
																		 const shaderGroup&				  shader_group,
																		 TextureCubeMapArraySamplingTest& test,
																		 bool							  isContextES)
{
	program_with_sampling_compute_shader.init(gl, shader_group, Compute, isContextES);
	program_with_sampling_fragment_shader.init(gl, shader_group, Fragment, isContextES);
	program_with_sampling_vertex_shader.init(gl, shader_group, Vertex, isContextES);

	test.link(program_with_sampling_compute_shader);
	test.link(program_with_sampling_fragment_shader);
	test.link(program_with_sampling_vertex_shader);

	if (test.m_is_geometry_shader_extension_supported)
	{
		program_with_sampling_geometry_shader.init(gl, shader_group, Geometry, isContextES);
		test.link(program_with_sampling_geometry_shader);
	}

	if (test.m_is_tessellation_shader_supported)
	{
		program_with_sampling_tesselation_control_shader.init(gl, shader_group, Tesselation_Control, isContextES);
		program_with_sampling_tesselation_evaluation_shader.init(gl, shader_group, Tesselation_Evaluation, isContextES);
		test.link(program_with_sampling_tesselation_control_shader);
		test.link(program_with_sampling_tesselation_evaluation_shader);
	}
}

/** Constructor
 *
 **/
TextureCubeMapArraySamplingTest::programDefinition::programDefinition()
	: compute_shader(0)
	, geometry_shader(0)
	, fragment_shader(0)
	, tesselation_control_shader(0)
	, tesselation_evaluation_shader(0)
	, vertex_shader(0)
	, m_program_object_id(m_invalid_program_object_id)
	, m_gl(DE_NULL)
{
}

/** Destructor
 *
 **/
TextureCubeMapArraySamplingTest::programDefinition::~programDefinition()
{
	if (m_invalid_program_object_id != m_program_object_id)
	{
		if (0 != m_gl)
		{
			m_gl->deleteProgram(m_program_object_id);
			m_program_object_id = m_invalid_program_object_id;
			m_gl				= 0;
		}
	}
}

/** Get program id
 *
 *  @returns Program id
 **/
glw::GLuint TextureCubeMapArraySamplingTest::programDefinition::getProgramId() const
{
	return m_program_object_id;
}

/** Get shader
 *
 *  @param shader_type Requested shader type
 *
 *  @returns Pointer to shader information. Can be null.
 **/
const TextureCubeMapArraySamplingTest::shaderDefinition* TextureCubeMapArraySamplingTest::programDefinition::getShader(
	shaderType shader_type) const
{
	switch (shader_type)
	{
	case Compute:
		return compute_shader;
		break;
	case Fragment:
		return fragment_shader;
		break;
	case Geometry:
		return geometry_shader;
		break;
	case Tesselation_Control:
		return tesselation_control_shader;
		break;
	case Tesselation_Evaluation:
		return tesselation_evaluation_shader;
		break;
	case Vertex:
		return vertex_shader;
		break;
	}

	return 0;
}

/** Initialize program information
 *
 *  @param gl           GL functions
 *  @param shader_group Group of shaders compatible with samplinbg function and texture format
 *  @param shader_type  Stage that will execute sampling
 **/
void TextureCubeMapArraySamplingTest::programDefinition::init(const glw::Functions& gl, const shaderGroup& shader_group,
															  shaderType shader_type, bool isContextES)
{
	m_gl = &gl;

	bool is_program_defined = false;

	switch (shader_type)
	{
	case Compute:
		compute_shader	 = shader_group.sampling_compute_shader;
		is_program_defined = (0 != compute_shader);
		break;
	case Fragment:
		fragment_shader	= shader_group.sampling_fragment_shader;
		vertex_shader	  = shader_group.pass_through_vertex_shader;
		is_program_defined = ((0 != fragment_shader) && (0 != vertex_shader));
		break;
	case Geometry:
		fragment_shader	= shader_group.pass_through_fragment_shader;
		geometry_shader	= shader_group.sampling_geometry_shader;
		vertex_shader	  = shader_group.pass_through_vertex_shader;
		is_program_defined = ((0 != fragment_shader) && (0 != geometry_shader) && (0 != vertex_shader));
		break;
	case Tesselation_Control:
		fragment_shader				  = shader_group.pass_through_fragment_shader;
		tesselation_control_shader	= shader_group.sampling_tesselation_control_shader;
		tesselation_evaluation_shader = shader_group.pass_through_tesselation_evaluation_shader;
		vertex_shader				  = shader_group.pass_through_vertex_shader;
		is_program_defined			  = ((0 != fragment_shader) && (0 != tesselation_control_shader) &&
							  (0 != tesselation_evaluation_shader) && (0 != vertex_shader));
		break;
	case Tesselation_Evaluation:
		fragment_shader = shader_group.pass_through_fragment_shader;
		if (isContextES)
		{
			tesselation_control_shader = shader_group.pass_through_tesselation_control_shader;
		}
		tesselation_evaluation_shader = shader_group.sampling_tesselation_evaluation_shader;
		vertex_shader				  = shader_group.pass_through_vertex_shader;
		is_program_defined			  = ((0 != fragment_shader) && (0 != tesselation_control_shader) &&
							  (0 != tesselation_evaluation_shader) && (0 != vertex_shader));
		break;
	case Vertex:
		fragment_shader	= shader_group.pass_through_fragment_shader;
		vertex_shader	  = shader_group.sampling_vertex_shader;
		is_program_defined = ((0 != fragment_shader) && (0 != vertex_shader));
		break;
	}

	if (true == is_program_defined)
	{
		m_program_object_id = m_gl->createProgram();

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program");

		if (m_invalid_program_object_id == m_program_object_id)
		{
			throw tcu::InternalError("glCreateProgram return invalid id", "", __FILE__, __LINE__);
		}
	}
}

/** Link program
 *
 *  @return  true When linking was successful
 *          false When linking failed
 **/
bool TextureCubeMapArraySamplingTest::programDefinition::link()
{
	if (m_invalid_program_object_id == m_program_object_id)
	{
		return false;
	}

	if (0 != compute_shader)
	{
		compute_shader->attach(m_program_object_id);
	}

	if (0 != geometry_shader)
	{
		geometry_shader->attach(m_program_object_id);
	}

	if (0 != fragment_shader)
	{
		fragment_shader->attach(m_program_object_id);
	}

	if (0 != tesselation_control_shader)
	{
		tesselation_control_shader->attach(m_program_object_id);
	}

	if (0 != tesselation_evaluation_shader)
	{
		tesselation_evaluation_shader->attach(m_program_object_id);
	}

	if (0 != vertex_shader)
	{
		vertex_shader->attach(m_program_object_id);
	}

	/* Link Program */
	m_gl->linkProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glLinkProgram() call failed.");

	/* Check linking status */
	glw::GLint linkStatus = GL_FALSE;
	m_gl->getProgramiv(m_program_object_id, GL_LINK_STATUS, &linkStatus);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glGetProgramiv() call failed.");

	if (linkStatus == GL_FALSE)
	{
		return false;
	}

	return true;
}

/** Constructor
 *
 *  @param width  Width
 *  @param height Height
 *  @param depth  Depth
 **/
TextureCubeMapArraySamplingTest::resolutionDefinition::resolutionDefinition(glw::GLuint width, glw::GLuint height,
																			glw::GLuint depth)
	: m_width(width), m_height(height), m_depth(depth)
{
}

/** Constructor
 *
 *  @param function Type of sampling function
 *  @param name     Name of sampling function
 **/
TextureCubeMapArraySamplingTest::samplingFunctionDefinition::samplingFunctionDefinition(samplingFunction   function,
																						const glw::GLchar* name)
	: m_function(function), m_name(name)
{
}

/** Initialize shader collection for sampling function
 *
 *  @param gl                GL functions
 *  @param format            Texture format
 *  @param sampling_function Sampling function
 *  @param test              Instance of test class
 **/
void TextureCubeMapArraySamplingTest::shaderCollectionForSamplingRoutine::init(
	const glw::Functions& gl, const formatDefinition& format, const samplingFunction& sampling_function,
	TextureCubeMapArraySamplingTest& test)
{
	m_sampling_function = sampling_function;

	std::string pass_through_vertex_shader_source;
	std::string pass_through_tesselation_control_shader_source;
	std::string sampling_compute_shader_source;
	std::string sampling_fragment_shader_source;
	std::string sampling_geometry_shader_source;
	std::string sampling_tesselation_control_shader_source;
	std::string sampling_tesselation_evaluation_shader_source;
	std::string sampling_vertex_shader_source;

	test.getPassThroughVertexShaderCode(format.m_sampler_type, m_sampling_function, pass_through_vertex_shader_source);

	test.getPassThroughTesselationControlShaderCode(format.m_sampler_type, m_sampling_function,
													pass_through_tesselation_control_shader_source);

	test.getSamplingComputeShaderCode(format.m_sampler_type, m_sampling_function, sampling_compute_shader_source);

	test.getSamplingFragmentShaderCode(format.m_sampler_type, m_sampling_function, sampling_fragment_shader_source);

	test.getSamplingGeometryShaderCode(format.m_sampler_type, m_sampling_function, sampling_geometry_shader_source);

	test.getSamplingTesselationControlShaderCode(format.m_sampler_type, m_sampling_function,
												 sampling_tesselation_control_shader_source);

	test.getSamplingTesselationEvaluationShaderCode(format.m_sampler_type, m_sampling_function,
													sampling_tesselation_evaluation_shader_source);

	test.getSamplingVertexShaderCode(format.m_sampler_type, m_sampling_function, sampling_vertex_shader_source);

	pass_through_vertex_shader.init(gl, GL_VERTEX_SHADER, pass_through_vertex_shader_source, &test);
	sampling_compute_shader.init(gl, GL_COMPUTE_SHADER, sampling_compute_shader_source, &test);
	sampling_fragment_shader.init(gl, GL_FRAGMENT_SHADER, sampling_fragment_shader_source, &test);
	sampling_vertex_shader.init(gl, GL_VERTEX_SHADER, sampling_vertex_shader_source, &test);

	test.compile(pass_through_vertex_shader);
	test.compile(sampling_compute_shader);
	test.compile(sampling_fragment_shader);
	test.compile(sampling_vertex_shader);

	if (test.m_is_tessellation_shader_supported)
	{
		pass_through_tesselation_control_shader.init(gl, test.m_glExtTokens.TESS_CONTROL_SHADER,
													 pass_through_tesselation_control_shader_source, &test);
		sampling_tesselation_control_shader.init(gl, test.m_glExtTokens.TESS_CONTROL_SHADER,
												 sampling_tesselation_control_shader_source, &test);
		sampling_tesselation_evaluation_shader.init(gl, test.m_glExtTokens.TESS_EVALUATION_SHADER,
													sampling_tesselation_evaluation_shader_source, &test);

		test.compile(pass_through_tesselation_control_shader);
		test.compile(sampling_tesselation_control_shader);
		test.compile(sampling_tesselation_evaluation_shader);
	}

	if (test.m_is_geometry_shader_extension_supported)
	{
		sampling_geometry_shader.init(gl, test.m_glExtTokens.GEOMETRY_SHADER, sampling_geometry_shader_source, &test);

		test.compile(sampling_geometry_shader);
	}
}

/** Get group of shader compatible with sampling function and texture format
 *
 *  @param function     Sampling function
 *  @param shader_group Group of shaders
 **/
void TextureCubeMapArraySamplingTest::shaderCollectionForTextureFormat::getShaderGroup(samplingFunction function,
																					   shaderGroup& shader_group) const
{
	shader_group.init();

	for (shaderCollectionForSamplingFunctionVectorType::const_iterator it  = per_sampling_routine.begin(),
																	   end = per_sampling_routine.end();
		 end != it; ++it)
	{
		if (it->m_sampling_function == function)
		{
			shader_group.pass_through_fragment_shader				= &pass_through_fragment_shader;
			shader_group.pass_through_tesselation_control_shader	= &it->pass_through_tesselation_control_shader;
			shader_group.pass_through_tesselation_evaluation_shader = &pass_through_tesselation_evaluation_shader;
			shader_group.pass_through_vertex_shader					= &it->pass_through_vertex_shader;

			shader_group.sampling_compute_shader				= &it->sampling_compute_shader;
			shader_group.sampling_fragment_shader				= &it->sampling_fragment_shader;
			shader_group.sampling_geometry_shader				= &it->sampling_geometry_shader;
			shader_group.sampling_tesselation_control_shader	= &it->sampling_tesselation_control_shader;
			shader_group.sampling_tesselation_evaluation_shader = &it->sampling_tesselation_evaluation_shader;
			shader_group.sampling_vertex_shader					= &it->sampling_vertex_shader;

			return;
		}
	}
}

/** Initialize shader collection for texture format
 *
 *  @param gl                GL functions
 *  @param format            Texture format
 *  @param sampling_routines Set of sampling functions
 *  @param test              Instance of test class
 **/
void TextureCubeMapArraySamplingTest::shaderCollectionForTextureFormat::init(
	const glw::Functions& gl, const formatDefinition& format, const samplingFunctionsVectorType& sampling_routines,
	TextureCubeMapArraySamplingTest& test)
{
	std::string pass_through_fragment_shader_source;
	std::string pass_through_tesselation_evaluation_shader_source;
	glw::GLuint n_routines_supporting_format = 0;

	test.getPassThroughFragmentShaderCode(format.m_sampler_type, pass_through_fragment_shader_source);
	test.getPassThroughTesselationEvaluationShaderCode(format.m_sampler_type,
													   pass_through_tesselation_evaluation_shader_source);

	pass_through_fragment_shader.init(gl, GL_FRAGMENT_SHADER, pass_through_fragment_shader_source, &test);

	if (test.m_is_tessellation_shader_supported)
	{
		pass_through_tesselation_evaluation_shader.init(gl, test.m_glExtTokens.TESS_EVALUATION_SHADER,
														pass_through_tesselation_evaluation_shader_source, &test);
	}

	test.compile(pass_through_fragment_shader);

	if (test.m_is_tessellation_shader_supported)
	{
		test.compile(pass_through_tesselation_evaluation_shader);
	}

	for (samplingFunctionsVectorType::const_iterator it = sampling_routines.begin(), end = sampling_routines.end();
		 end != it; ++it)
	{
		if (TextureCubeMapArraySamplingTest::isSamplerSupportedByFunction(format.m_sampler_type, it->m_function))
		{
			n_routines_supporting_format += 1;
		}
	}

	per_sampling_routine.resize(n_routines_supporting_format);
	shaderCollectionForSamplingFunctionVectorType::iterator jt = per_sampling_routine.begin();

	for (samplingFunctionsVectorType::const_iterator it = sampling_routines.begin(), end = sampling_routines.end();
		 end != it; ++it)
	{
		if (TextureCubeMapArraySamplingTest::isSamplerSupportedByFunction(format.m_sampler_type, it->m_function))
		{
			jt->init(gl, format, it->m_function, test);
			++jt;
		}
	}
}

/** Constructor
 *
 *  @param type           Type of shader
 *  @param is_supported   If configuration is supported
 *  @param primitive_type Type of primitive
 *  @param name           Name of sampling shader stage
 **/
TextureCubeMapArraySamplingTest::shaderConfiguration::shaderConfiguration(shaderType type, glw::GLenum primitive_type,
																		  const glw::GLchar* name)
	: m_type(type), m_primitive_type(primitive_type), m_name(name)
{
}

/** Constructor
 *
 **/
TextureCubeMapArraySamplingTest::shaderDefinition::shaderDefinition()
	: m_gl(0), m_shader_stage(0), m_shader_object_id(m_invalid_shader_object_id)
{
}

/** Destructor
 *
 **/
TextureCubeMapArraySamplingTest::shaderDefinition::~shaderDefinition()
{
	if (m_invalid_shader_object_id != m_shader_object_id)
	{
		if (0 != m_gl)
		{
			m_gl->deleteShader(m_shader_object_id);

			m_gl = 0;
		}

		m_shader_object_id = m_invalid_shader_object_id;
	}

	m_source.clear();
}

/** Attach shade to program
 *
 *  @parma program_object_id Progam id
 **/
void TextureCubeMapArraySamplingTest::shaderDefinition::attach(glw::GLuint program_object_id) const
{
	if (0 == m_gl)
	{
		throw tcu::InternalError("shaderDefinition not initialized", "", __FILE__, __LINE__);
	}

	m_gl->attachShader(program_object_id, m_shader_object_id);

	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glAttachShader() call failed.");
}

/** Compile shader
 *
 *  @returns true  When successful
 *           false When compilation failed
 **/
bool TextureCubeMapArraySamplingTest::shaderDefinition::compile()
{
	glw::GLint		   compile_status = GL_FALSE;
	const glw::GLchar* source		  = m_source.c_str();

	/* Set shaders source */
	m_gl->shaderSource(m_shader_object_id, 1, &source, NULL);

	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glShaderSource() call failed.");

	/* Try to compile the shader */
	m_gl->compileShader(m_shader_object_id);

	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glCompileShader() call failed.");

	/* Check if all shaders compiled successfully */
	m_gl->getShaderiv(m_shader_object_id, GL_COMPILE_STATUS, &compile_status);

	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glGetShaderiv() call failed.");

	if (compile_status == GL_FALSE)
	{
		return false;
	}

	return true;
}

/** Get shader id
 *
 *  @returns Shader id
 **/
glw::GLuint TextureCubeMapArraySamplingTest::shaderDefinition::getShaderId() const
{
	return m_shader_object_id;
}

/** Get source
 *
 *  @returns Code of shader
 **/
const std::string& TextureCubeMapArraySamplingTest::shaderDefinition::getSource() const
{
	return m_source;
}

/** Initialize shader informations
 *
 *  @param gl           GL functions
 *  @param shader_stage Stage of shader
 *  @param source       Source of shader
 **/
void TextureCubeMapArraySamplingTest::shaderDefinition::init(const glw::Functions& gl, glw::GLenum shader_stage,
															 const std::string&				  source,
															 TextureCubeMapArraySamplingTest* test)
{
	m_gl						   = &gl;
	m_shader_stage				   = shader_stage;
	const glw::GLchar* source_cstr = source.c_str();
	m_source					   = test->specializeShader(1, &source_cstr);

	if (m_invalid_shader_object_id == m_shader_object_id)
	{
		if (0 != m_gl)
		{
			m_shader_object_id = m_gl->createShader(m_shader_stage);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create shader");

			if (m_invalid_shader_object_id == m_shader_object_id)
			{
				throw tcu::InternalError("glCreateShader returned invalid id", "", __FILE__, __LINE__);
			}
		}
	}
}

/** Initialize shader group
 *
 **/
void TextureCubeMapArraySamplingTest::shaderGroup::init()
{
	pass_through_fragment_shader			   = 0;
	pass_through_tesselation_control_shader	= 0;
	pass_through_tesselation_evaluation_shader = 0;
	pass_through_vertex_shader				   = 0;
	sampling_compute_shader					   = 0;
	sampling_fragment_shader				   = 0;
	sampling_geometry_shader				   = 0;
	sampling_tesselation_control_shader		   = 0;
	sampling_tesselation_evaluation_shader	 = 0;
	sampling_vertex_shader					   = 0;
}

/** Constructor
 *
 **/
TextureCubeMapArraySamplingTest::textureDefinition::textureDefinition()
	: m_gl(0), m_texture_object_id(m_invalid_texture_object_id)
{
}

/** Destructor
 *
 **/
TextureCubeMapArraySamplingTest::textureDefinition::~textureDefinition()
{
	if (m_invalid_texture_object_id != m_texture_object_id)
	{
		if (0 != m_gl)
		{
			m_gl->deleteTextures(1, &m_texture_object_id);

			m_gl = 0;
		}

		m_texture_object_id = m_invalid_texture_object_id;
	}
}

/** Bind texture
 *
 *  @param binding_point Where texture will be bound
 **/
void TextureCubeMapArraySamplingTest::textureDefinition::bind(glw::GLenum binding_point) const
{
	if (0 == m_gl)
	{
		throw tcu::InternalError("TextureDefinition not initialized", "", __FILE__, __LINE__);
	}

	m_gl->bindTexture(binding_point, m_texture_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to bind texture");
}

/** Get texture id
 *
 *  @returns Texture id
 **/
glw::GLuint TextureCubeMapArraySamplingTest::textureDefinition::getTextureId() const
{
	return m_texture_object_id;
}

/** Initialize texture information
 *
 *  @param gl         GL functions
 *  @param bind_image Address of glBindImageTexture procedure
 **/
void TextureCubeMapArraySamplingTest::textureDefinition::init(const glw::Functions& gl)
{
	m_gl = &gl;

	gl.genTextures(1, &m_texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to generate texture");
}

/** Set texture as image
 *
 *  @param image_unit      Index of image unit
 *  @param internal_format Format for image unit
 **/
void TextureCubeMapArraySamplingTest::textureDefinition::setupImage(glw::GLuint image_unit, glw::GLenum internal_format)
{
	if ((0 == m_gl) || (m_invalid_texture_object_id == m_texture_object_id))
	{
		throw tcu::InternalError("Not initialized textureDefinition", "", __FILE__, __LINE__);
	}

	m_gl->bindImageTexture(image_unit, m_texture_object_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, internal_format);

	GLU_EXPECT_NO_ERROR(m_gl->getError(), "glBindImageTexture");
}

/** Setup texture unit with this
 *
 *  @param texture_unit Index of texture unit
 *  @param sampler_name_p Name of sampler uniform
 *  @param program_id   Program id
 *  @param is_shadow    If depth comparison should be enabled
 **/
void TextureCubeMapArraySamplingTest::textureDefinition::setupSampler(glw::GLuint		 texture_unit,
																	  const glw::GLchar* sampler_name_p,
																	  glw::GLuint program_id, bool is_shadow)
{
	if ((0 == m_gl) || (m_invalid_texture_object_id == m_texture_object_id))
	{
		throw tcu::InternalError("Not initialized textureDefinition", "", __FILE__, __LINE__);
	}

	glw::GLint sampler_location = m_gl->getUniformLocation(program_id, sampler_name_p);
	if ((m_invalid_uniform_location == (glw::GLuint)sampler_location) || (GL_NO_ERROR != m_gl->getError()))
	{
		//throw tcu::InternalError("Failed to get sampler location", sampler_name_p, __FILE__, __LINE__);
		return;
	}

	m_gl->activeTexture(GL_TEXTURE0 + texture_unit);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to activate texture unit");

	m_gl->bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to bind texture to GL_TEXTURE_CUBE_MAP_ARRAY_EXT");

	if (true == is_shadow)
	{
		m_gl->texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		m_gl->texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	}

	m_gl->uniform1i(sampler_location, texture_unit);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to set sampler uniform");
}

/** Constructor
 *
 **/
TextureCubeMapArraySamplingTest::vertexArrayObjectDefinition::vertexArrayObjectDefinition()
	: m_gl(0), m_vertex_array_object_id(m_invalid_vertex_array_object_id)
{
}

/** Destructor
 *
 **/
TextureCubeMapArraySamplingTest::vertexArrayObjectDefinition::~vertexArrayObjectDefinition()
{
	if (m_invalid_vertex_array_object_id != m_vertex_array_object_id)
	{
		if (0 != m_gl)
		{
			m_gl->deleteVertexArrays(1, &m_vertex_array_object_id);

			m_gl = 0;
		}

		m_vertex_array_object_id = m_invalid_vertex_array_object_id;
	}
}

/** Initialize vertex array object
 *
 *  @param gl                GL functions
 *  @param format            Texture format
 *  @param sampling_function Type of sampling function
 *  @param buffers           Buffer collection
 *  @param program_id        Program id
 **/
void TextureCubeMapArraySamplingTest::vertexArrayObjectDefinition::init(const glw::Functions&   gl,
																		const formatDefinition& format,
																		const samplingFunction& sampling_function,
																		const bufferCollection& buffers,
																		glw::GLuint				program_id)
{
	m_gl = &gl;

	m_gl->genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to generate VAO.");

	m_gl->bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to bind VAO.");

	attributeDefinition		   positionAttribute = { vertex_shader_position, type_vec4, Position, 0 };
	const attributeDefinition* format_attributes;
	glw::GLuint				   n_format_attributes;
	const attributeDefinition* routine_attributes   = 0;
	glw::GLuint				   n_routine_attributes = 0;

	getAttributes(format.m_sampler_type, format_attributes, n_format_attributes);
	getAttributes(sampling_function, routine_attributes, n_routine_attributes);

	setupAttribute(positionAttribute, buffers, program_id);

	for (glw::GLuint i = 0; i < n_routine_attributes; ++i)
	{
		setupAttribute(routine_attributes[i], buffers, program_id);
	}

	for (glw::GLuint i = 0; i < n_format_attributes; ++i)
	{
		setupAttribute(format_attributes[i], buffers, program_id);
	}
}

/** Setup vertex array object
 *
 *  @param attribute  Attribute information
 *  @param buffers    Buffer collection
 *  @param program_id Program id
 **/
void TextureCubeMapArraySamplingTest::vertexArrayObjectDefinition::setupAttribute(const attributeDefinition& attribute,
																				  const bufferCollection&	buffers,
																				  glw::GLuint				 program_id)
{
	std::string				attribute_name = vertex_shader_input;
	const bufferDefinition* buffer		   = 0;
	glw::GLuint				n_components   = 0;
	glw::GLenum				type		   = GL_FLOAT;

	attribute_name.append(attribute.name);

	switch (attribute.attribute_id)
	{
	case Position:
		n_components = 4;
		buffer		 = &buffers.postion;
		break;
	case TextureCoordinates:
		n_components = 4;
		buffer		 = &buffers.texture_coordinate;
		break;
	case TextureCoordinatesForGather:
		n_components = 4;
		buffer		 = &buffers.texture_coordinate_for_gather;
		break;
	case Lod:
		n_components = 1;
		buffer		 = &buffers.lod;
		break;
	case GradX:
		n_components = 4;
		buffer		 = &buffers.grad_x;
		break;
	case GradY:
		n_components = 4;
		buffer		 = &buffers.grad_y;
		break;
	case RefZ:
		n_components = 1;
		buffer		 = &buffers.refZ;
		break;
	}

	/* Get attribute location */
	glw::GLint attribute_location = m_gl->getAttribLocation(program_id, attribute_name.c_str());

	if ((m_invalid_attribute_location == (glw::GLuint)attribute_location) || (GL_NO_ERROR != m_gl->getError()))
	{
		//throw tcu::InternalError("Failed to get location of attribute:", attribute_name.c_str(), __FILE__, __LINE__);
		return;
	}

	buffer->bind(GL_ARRAY_BUFFER);

	m_gl->enableVertexAttribArray(attribute_location);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to enable attribute");

	m_gl->vertexAttribPointer(attribute_location, n_components, type, GL_FALSE, 0 /* stride */, 0 /* offset */);

	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Failed to setup vertex attribute arrays");
}

#if TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION
/** Get file name for given face
 * Such pattern is used: texture_cube_map_array_sampling_test_w_WIDTH_h_HEIGHT_l_LEVEL_i_INDEX_f_FACE
 *
 *  @param width  Width of texture
 *  @param height Height of texture
 *  @param level  Mipmap level
 *  @param index  Index of element in array
 *  @param face   Cube map's face index
 *  @param name   File name
 **/
void getTextureFileName(glw::GLuint width, glw::GLuint height, glw::GLuint level, glw::GLuint index, glw::GLuint face,
						std::string& name)
{
	std::stringstream file_name;

	file_name << TEXTURECUBEMAPARRAYSAMPLINGTEST_PATH_FOR_COMPRESSION;

	file_name << "texture_cube_map_array_sampling_test_"
			  << "w_" << width << "_h_" << height << "_l_" << level << "_i_" << index << "_f_" << face;

	name = file_name.str();
}

/** Store whole cube map as TGA files. Each face is stored as separate image.
 *
 *  @param resolution Resolution of base level
 **/
void prepareDumpForTextureCompression(const TextureCubeMapArraySamplingTest::resolutionDefinition& resolution)
{
	glw::GLsizei texture_width  = resolution.m_width;
	glw::GLsizei texture_height = resolution.m_height;

	const glw::GLuint n_components	 = 4;
	const glw::GLuint n_faces		   = 6;
	const glw::GLint  n_array_elements = resolution.m_depth / n_faces;
	const glw::GLint  n_mipmap_levels  = getMipmapLevelCount(resolution.m_width, resolution.m_height);

	const unsigned char  tga_id_length				  = 0; // no id
	const unsigned char  tga_color_map_type			  = 0; // no color map
	const unsigned char  tga_image_type				  = 2; // rgb no compression
	const unsigned short tga_color_map_offset		  = 0; // no color map
	const unsigned short tga_color_map_length		  = 0; // no color map
	const unsigned char  tga_color_map_bits_per_pixel = 0; // no color map
	const unsigned short tga_image_x				  = 0;
	const unsigned short tga_image_y				  = 0;
	const unsigned char  tga_image_bits_per_pixel	 = 32;
	const unsigned char  tga_image_descriptor		  = 0x8; // 8 per alpha

	for (glw::GLint mipmap_level = 0; mipmap_level < n_mipmap_levels; ++mipmap_level)
	{
		const unsigned short tga_image_width  = texture_width;
		const unsigned short tga_image_height = texture_height;

		for (glw::GLint array_index = 0; array_index < n_array_elements; ++array_index)
		{
			for (glw::GLint face = 0; face < n_faces; ++face)
			{
				std::fstream file;
				std::string  file_name;
				getTextureFileName(resolution.m_width, resolution.m_height, mipmap_level, array_index, face, file_name);
				file_name.append(".tga");

				file.open(file_name.c_str(), std::fstream::out | std::fstream::binary);

				file.write((const char*)&tga_id_length, sizeof(tga_id_length));
				file.write((const char*)&tga_color_map_type, sizeof(tga_color_map_type));
				file.write((const char*)&tga_image_type, sizeof(tga_image_type));
				file.write((const char*)&tga_color_map_offset, sizeof(tga_color_map_offset));
				file.write((const char*)&tga_color_map_length, sizeof(tga_color_map_length));
				file.write((const char*)&tga_color_map_bits_per_pixel, sizeof(tga_color_map_bits_per_pixel));
				file.write((const char*)&tga_image_x, sizeof(tga_image_x));
				file.write((const char*)&tga_image_y, sizeof(tga_image_y));
				file.write((const char*)&tga_image_width, sizeof(tga_image_width));
				file.write((const char*)&tga_image_height, sizeof(tga_image_height));
				file.write((const char*)&tga_image_bits_per_pixel, sizeof(tga_image_bits_per_pixel));
				file.write((const char*)&tga_image_descriptor, sizeof(tga_image_descriptor));

				glw::GLubyte components[n_components];

				getCompressedColorUByteComponents(face, array_index, mipmap_level, n_array_elements, n_mipmap_levels,
												  components);

				for (glw::GLuint y = 0; y < texture_height; ++y)
				{
					for (glw::GLuint x = 0; x < texture_width; ++x)
					{
						for (glw::GLuint i = 0; i < n_components; ++i)
						{
							file.write((const char*)&components[i], sizeof(glw::GLubyte));
						}
					}
				}
			}
		}

		texture_width  = de::max(1, texture_width / 2);
		texture_height = de::max(1, texture_height / 2);
	}
}

#endif /* TEXTURECUBEMAPARRAYSAMPLINGTEST_DUMP_TEXTURES_FOR_COMPRESSION */

} /* glcts */
