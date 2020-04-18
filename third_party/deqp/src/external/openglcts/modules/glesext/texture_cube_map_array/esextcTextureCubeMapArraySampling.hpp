#ifndef _ESEXTCTEXTURECUBEMAPARRAYSAMPLING_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYSAMPLING_HPP
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
 * \file  esextcTextureCubeMapArraySampling.hpp
 * \brief Texture Cube Map Array Sampling (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

#include <string>

namespace glcts
{

/** Implementation of (Test 1) for texture_cube_map_array extension. Test description follows:
 *
 *  Make sure sampling cube-map array textures works correctly.
 *
 *  Category: Functionality tests,
 *            Optional dependency on EXT_geometry_shader;
 *            Optional dependency on EXT_tessellation_shader;
 *            Optional dependency on OES_texture_stencil8;
 *
 *  Priority: Must-have.
 *
 *  Verify that both mutable and immutable cube-map array textures initialized
 *  with:
 *
 *  * a color-renderable internalformat (GL_RGBA8);
 *  * a depth-renderable internalformat (GL_DEPTH_COMPONENT32F);
 *  * GL_RGBA32I internalformat;
 *  * GL_RGBA32UI internalformat;
 *  * GL_STENCIL_INDEX8 internalformat;
 *  * GL_COMPRESSED_RGBA8_ETC2_EAC internalformat;
 *
 *  can be correctly sampled from:
 *
 *  * a compute shader.
 *  * a fragment shader;
 *  * a geometry shader;                (if supported)
 *  * a tessellation control shader;    (if supported)
 *  * a tessellation evaluation shader; (if supported)
 *  * a vertex shader.
 *
 *  For depth textures, the test should verify results reported by
 *  samplerCubeArray and samplerCubeArrayShadow samplers are valid.
 *
 *  Use the following texture sampling GLSL functions:
 *
 *  * texture();
 *  * textureLod()  (lod argument set to 0 and n_total_texture_mipmaps-1;
 *                   in the former case we expect base mip-map data to be
 *                   used, in the latter it should be the last mip-map that
 *                   is used as texel data source)
 *  * textureGrad() (dPdx, dPdy should be calculated as distance to "right"
 *                   and "top" "neighbours". Set of neighbours should
 *                   ensure sampling of base and last mipmap level);
 *  * textureGather();
 *
 *  The following samplers should be used: (whichever applies for texture
 *  considered)
 *
 *  * isamplerCubeArray;
 *  *  samplerCubeArray;
 *  *  samplerCubeArrayShadow;
 *  * usamplerCubeArray;
 *
 *  The following cube-map array texture resolutions should be used for the
 *  purpose of the test:
 *
 *  * not compressed formats
 *         [width x height x depth]
 *      1)   64   x   64   x  18;
 *      2)  117   x  117   x   6;
 *      3)  256   x  256   x   6;
 *      4)  173   x  173   x  12;
 *  * compressed formats
 *         [width x height x depth]
 *      1)    8   x    8   x  12
 *      2)   13   x   13   x  12;
 *
 *  In case of color data, for m-th mip-map level of n-th face at i-th layer,
 *  the mip-map should be filled with the following color:
 *
 *          ((m+1)/n_mipmaps, (n+1)/6, (i+1)/n_layers, 1.0 / 4.0)
 *
 *  For signed/unsigned integer data, remove the denominator from all channels.
 *
 *  For compressed formats, each component should be equal:
 *
 *          (m * n_layers * 6) + (i * 6) + n
 *
 *  In case of depth data, for m-th mip-map level of n-th face at i-th layer,
 *  the mip-map should be filled with the following depth:
 *
 *           (m+1 + n+1 + i+1) / (n_mipmaps + 6 + n_layers)
 *
 *  In case of stencil data, for m-th mip-map level of n-th face at i-th layer,
 *  the mip-map should be filled with the following stencil index value:
 *
 *        (m+1 + n+1 + i+1) / (n_mipmaps + 6 + n_layers) * 255
 *
 *  The test should use the maximum allowed amount of mip-maps, equal to:
 *
 *              (floor(log2(max(width, height))) + 1;
 *
 *  In each iteration, make sure that type of the uniform (as reported by
 *  glGetActiveUniform() and glGetProgramResourceiv() ) used for the test
 *  iteration is valid.
 **/
class TextureCubeMapArraySamplingTest : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArraySamplingTest(Context& context, const ExtParameters& extParams, const char* name,
									const char* description);

	virtual ~TextureCubeMapArraySamplingTest(void)
	{
	}

	virtual IterateResult iterate(void);

private:
	/* Enums */
	/** Identifies vertex attribute
	 *
	 **/
	enum attributeId
	{
		Position,
		TextureCoordinates,
		TextureCoordinatesForGather,
		Lod,
		GradX,
		GradY,
		RefZ,
	};

	/** Identifies type of sampler
	 *
	 **/
	enum samplerType
	{
		Float = 0,
		Int,
		UInt,
		Depth,
		Stencil
	};

	/** Identifies type of sampling function
	 *
	 **/
	enum samplingFunction
	{
		Texture = 0,
		TextureLod,
		TextureGrad,
		TextureGather,
	};

	/** Identifies shader stage
	 *
	 **/
	enum shaderType
	{
		Compute = 0,
		Fragment,
		Geometry,
		Tesselation_Control,
		Tesselation_Evaluation,
		Vertex
	};

	/* Structures */
	/** Defines vertex attribute
	 *
	 **/
	struct attributeDefinition
	{
		const glw::GLchar* name;
		const glw::GLchar* type;
		attributeId		   attribute_id;
		int				   binding;
	};

	/** Provide set of "getComponents" routines
	 *
	 **/
	struct componentProvider
	{
		void (*getColorFloatComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
										glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLfloat* out_components);

		void (*getColorUByteComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
										glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLubyte* out_components);

		void (*getColorUintComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
									   glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLuint* out_components);

		void (*getColorIntComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
									  glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLint* out_components);

		void (*getDepthComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
								   glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLfloat* out_components);

		void (*getStencilComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
									 glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLuint* out_components);

		void (*getCompressedComponents)(glw::GLuint pixel_index, glw::GLint cube_face, glw::GLint layer_index,
										glw::GLint n_layers, glw::GLint n_mipmap_levels, glw::GLubyte* out_components);
	};

	/** Defines a GL "format"
	 *
	 **/
	struct formatInfo
	{
		formatInfo(glw::GLenum internal_format, glw::GLenum format, glw::GLenum type, bool is_compressed);

		glw::GLenum m_internal_format;
		glw::GLenum m_format;
		glw::GLenum m_type;
		bool		m_is_compressed;
	};

	/** Defines type of sampler, source and destination GL formats
	 *
	 **/
	struct formatDefinition
	{
		formatDefinition(glw::GLenum internal_format, glw::GLenum format, glw::GLenum type, bool is_compressed,
						 samplerType sampler_type, const glw::GLchar* name);

		formatDefinition(glw::GLenum src_internal_format, glw::GLenum src_format, glw::GLenum src_type,
						 bool src_is_compressed, glw::GLenum dst_internal_format, glw::GLenum dst_format,
						 glw::GLenum dst_type, samplerType sampler_type, const glw::GLchar* name);

		formatInfo		   m_source;
		formatInfo		   m_destination;
		samplerType		   m_sampler_type;
		const glw::GLchar* m_name;
	};

	/** Defines cube map texture resolution
	 *
	 **/
	struct resolutionDefinition
	{
		resolutionDefinition(glw::GLuint width, glw::GLuint height, glw::GLuint depth);

		glw::GLuint m_width;
		glw::GLuint m_height;
		glw::GLuint m_depth;
	};

	/** Defines sampling function
	 *
	 **/
	struct samplingFunctionDefinition
	{
		samplingFunctionDefinition(samplingFunction function, const glw::GLchar* name);

		samplingFunction   m_function;
		const glw::GLchar* m_name;
	};

	/** Defines sampling shader stage and type of primitive used by draw call
	 *
	 **/
	struct shaderConfiguration
	{
		shaderConfiguration(shaderType type, glw::GLenum primitive_type, const glw::GLchar* name);

		shaderType		   m_type;
		glw::GLenum		   m_primitive_type;
		const glw::GLchar* m_name;
	};

	/* Typedefs */
	typedef std::vector<formatDefinition>			formatsVectorType;
	typedef std::vector<bool>						mutablitiesVectorType;
	typedef std::vector<resolutionDefinition>		resolutionsVectorType;
	typedef std::vector<samplingFunctionDefinition> samplingFunctionsVectorType;
	typedef std::vector<shaderConfiguration>		shadersVectorType;

	/* Classes */

	/** Defines vertex buffer
	 *
	 **/
	class bufferDefinition
	{
	public:
		bufferDefinition();
		~bufferDefinition();

		void init(const glw::Functions& gl, glw::GLsizeiptr buffer_size, glw::GLvoid* buffer_data);

		void bind(glw::GLenum target) const;
		void bind(glw::GLenum target, glw::GLuint index) const;

		static const glw::GLuint m_invalid_buffer_object_id;

	private:
		const glw::Functions* m_gl;
		glw::GLuint			  m_buffer_object_id;
	};

	/** Defines a collection of vertex buffers for specific texture resolution
	 *
	 **/
	class bufferCollection
	{
	public:
		void init(const glw::Functions& gl, const formatDefinition& format, const resolutionDefinition& resolution);

		bufferDefinition postion;
		bufferDefinition texture_coordinate;
		bufferDefinition texture_coordinate_for_gather;
		bufferDefinition lod;
		bufferDefinition grad_x;
		bufferDefinition grad_y;
		bufferDefinition refZ;
	};

	/** Wrapper for texture object id
	 *
	 **/
	class textureDefinition
	{
	public:
		textureDefinition();
		~textureDefinition();

		void init(const glw::Functions& gl);

		void bind(glw::GLenum binding_point) const;

		glw::GLuint getTextureId() const;

		void setupImage(glw::GLuint image_unit, glw::GLenum internal_format);
		void setupSampler(glw::GLuint texture_unit, const glw::GLchar* sampler_name, glw::GLuint program_id,
						  bool is_shadow);

		static const glw::GLuint m_invalid_uniform_location;
		static const glw::GLuint m_invalid_texture_object_id;

	private:
		const glw::Functions* m_gl;
		glw::GLuint			  m_texture_object_id;
	};

	/** Wrapper for shader object id
	 *
	 **/
	class shaderDefinition
	{
	public:
		shaderDefinition();
		~shaderDefinition();

		void init(const glw::Functions& gl, glw::GLenum shader_stage, const std::string& source,
				  class TextureCubeMapArraySamplingTest* test);

		bool compile();

		void attach(glw::GLuint program_object_id) const;

		glw::GLuint				 getShaderId() const;
		static const glw::GLuint m_invalid_shader_object_id;

		const std::string& getSource() const;

	private:
		const glw::Functions* m_gl;
		glw::GLenum			  m_shader_stage;
		std::string			  m_source;
		glw::GLuint			  m_shader_object_id;
	};

	/** Defines a collection of shaders for specific sampling routine
	 *
	 **/
	class shaderCollectionForSamplingRoutine
	{
	public:
		shaderCollectionForSamplingRoutine() : m_sampling_function(Texture)
		{
		}

		~shaderCollectionForSamplingRoutine()
		{
		}

		void init(const glw::Functions& gl, const formatDefinition& format, const samplingFunction& sampling_function,
				  TextureCubeMapArraySamplingTest& test);

		shaderDefinition pass_through_vertex_shader;
		shaderDefinition pass_through_tesselation_control_shader;

		shaderDefinition sampling_compute_shader;
		shaderDefinition sampling_fragment_shader;
		shaderDefinition sampling_geometry_shader;
		shaderDefinition sampling_tesselation_control_shader;
		shaderDefinition sampling_tesselation_evaluation_shader;
		shaderDefinition sampling_vertex_shader;

		samplingFunction m_sampling_function;
	};

	/** Defines a complete set of shaders for one format and sampling routine
	 *
	 **/
	struct shaderGroup
	{
		void init();

		const shaderDefinition* pass_through_fragment_shader;
		const shaderDefinition* pass_through_tesselation_control_shader;
		const shaderDefinition* pass_through_tesselation_evaluation_shader;
		const shaderDefinition* pass_through_vertex_shader;
		const shaderDefinition* sampling_compute_shader;
		const shaderDefinition* sampling_fragment_shader;
		const shaderDefinition* sampling_geometry_shader;
		const shaderDefinition* sampling_tesselation_control_shader;
		const shaderDefinition* sampling_tesselation_evaluation_shader;
		const shaderDefinition* sampling_vertex_shader;
	};

	/** Defines a collection of shaders for one format
	 *
	 **/
	class shaderCollectionForTextureFormat
	{
	public:
		shaderCollectionForTextureFormat()
		{
		}

		~shaderCollectionForTextureFormat()
		{
		}

		void init(const glw::Functions& gl, const formatDefinition& format,
				  const samplingFunctionsVectorType& sampling_routines, TextureCubeMapArraySamplingTest& test);

		void getShaderGroup(samplingFunction function, shaderGroup& shader_group) const;

	private:
		shaderDefinition pass_through_fragment_shader;
		shaderDefinition pass_through_tesselation_control_shader;
		shaderDefinition pass_through_tesselation_evaluation_shader;

		typedef std::vector<shaderCollectionForSamplingRoutine> shaderCollectionForSamplingFunctionVectorType;
		shaderCollectionForSamplingFunctionVectorType			per_sampling_routine;
	};

	/** Wrapper for program object id
	 *
	 **/
	class programDefinition
	{
	public:
		programDefinition();
		~programDefinition();

		void init(const glw::Functions& gl, const shaderGroup& shader_group, shaderType shader_type, bool isContextES);

		bool link();

		glw::GLuint				 getProgramId() const;
		static const glw::GLuint m_invalid_program_object_id;

		const shaderDefinition* getShader(shaderType shader_type) const;

	private:
		const shaderDefinition* compute_shader;
		const shaderDefinition* geometry_shader;
		const shaderDefinition* fragment_shader;
		const shaderDefinition* tesselation_control_shader;
		const shaderDefinition* tesselation_evaluation_shader;
		const shaderDefinition* vertex_shader;

		glw::GLuint m_program_object_id;

	private:
		const glw::Functions* m_gl;
	};

	/** Collection of programs for one sampling routine
	 *
	 **/
	class programCollectionForFunction
	{
	public:
		void init(const glw::Functions& gl, const shaderGroup& shader_group, TextureCubeMapArraySamplingTest& test,
				  bool isContextES);

		const programDefinition* getProgram(shaderType shader_type) const;

	private:
		programDefinition program_with_sampling_compute_shader;
		programDefinition program_with_sampling_fragment_shader;
		programDefinition program_with_sampling_geometry_shader;
		programDefinition program_with_sampling_tesselation_control_shader;
		programDefinition program_with_sampling_tesselation_evaluation_shader;
		programDefinition program_with_sampling_vertex_shader;
	};

	/** Program collection for one format
	 *
	 **/
	class programCollectionForFormat
	{
	public:
		void init(const glw::Functions& gl, const shaderCollectionForTextureFormat& shader_collection,
				  TextureCubeMapArraySamplingTest& test, bool isContextES);

		const programCollectionForFunction* getPrograms(samplingFunction function) const;

	private:
		programCollectionForFunction m_programs_for_texture;
		programCollectionForFunction m_programs_for_textureLod;
		programCollectionForFunction m_programs_for_textureGrad;
		programCollectionForFunction m_programs_for_textureGather;
	};

	/** Wrapper for vertex array object id
	 *
	 **/
	class vertexArrayObjectDefinition
	{
	public:
		vertexArrayObjectDefinition();
		~vertexArrayObjectDefinition();

		void init(const glw::Functions& gl, const formatDefinition& format, const samplingFunction& sampling_function,
				  const bufferCollection& buffers, glw::GLuint program_id);

	private:
		void setupAttribute(const attributeDefinition& attribute, const bufferCollection& buffers,
							glw::GLuint program_id);

		static const glw::GLuint m_invalid_attribute_location;
		static const glw::GLuint m_invalid_vertex_array_object_id;

		const glw::Functions* m_gl;
		glw::GLuint			  m_vertex_array_object_id;
	};

	/* Methods */
	glw::GLuint checkUniformAndResourceApi(glw::GLuint program_id, const glw::GLchar* sampler_name,
										   samplerType sampler_type);

	void compile(shaderDefinition& info);

	void dispatch(glw::GLuint program_id, glw::GLuint width, glw::GLuint height);

	void draw(glw::GLuint program_id, glw::GLenum m_primitive_type, glw::GLuint n_vertices, glw::GLenum format);

	static void getAttributes(samplerType sampler_type, const attributeDefinition*& out_attribute_definitions,
							  glw::GLuint& out_n_attributes);

	static void getAttributes(samplingFunction sampling_function, const attributeDefinition*& out_attribute_definitions,
							  glw::GLuint& out_n_attributes);

	void getColorType(samplerType sampler_type, const glw::GLchar*& out_color_type,
					  const glw::GLchar*& out_interpolation_type, const glw::GLchar*& out_sampler_type,
					  glw::GLuint& out_n_components, bool& out_is_shadow);

	void getColorType(samplerType sampler_type, const glw::GLchar*& out_color_type,
					  const glw::GLchar*& out_interpolation_type, const glw::GLchar*& out_sampler_type,
					  const glw::GLchar*& out_image_type, const glw::GLchar*& out_image_format,
					  glw::GLuint& out_n_components, bool& out_is_shadow);

	void getPassThroughFragmentShaderCode(samplerType sampler_type, std::string& out_fragment_shader_code);

	void getPassThroughTesselationControlShaderCode(const samplerType&		sampler_type,
													const samplingFunction& sampling_function,
													std::string&			out_tesselation_control_shader_code);

	void getPassThroughTesselationEvaluationShaderCode(samplerType  sampler_type,
													   std::string& out_tesselation_evaluation_shader_code);

	void getPassThroughVertexShaderCode(const samplerType& sampler_type, const samplingFunction& sampling_function,
										std::string& out_vertex_shader_code);

	void getSamplingComputeShaderCode(const samplerType& sampler_type, const samplingFunction& sampling_function,
									  std::string& out_vertex_shader_code);

	void getSamplingFragmentShaderCode(const samplerType& sampler_type, const samplingFunction& sampling_function,
									   std::string& out_fragment_shader_code);

	void getSamplingFunctionCall(samplingFunction sampling_function, const glw::GLchar* color_type,
								 glw::GLuint n_components, const glw::GLchar* attribute_name_prefix,
								 const glw::GLchar* attribute_index, const glw::GLchar* color_variable_name,
								 const glw::GLchar* color_variable_index, const glw::GLchar* sampler_name,
								 std::string& out_code);

	void getSamplingGeometryShaderCode(const samplerType& sampler_type, const samplingFunction& sampling_function,
									   std::string& out_geometry_shader_code);

	void getSamplingTesselationControlShaderCode(const samplerType&		 sampler_type,
												 const samplingFunction& sampling_function,
												 std::string&			 out_tesselation_control_shader_code);

	void getSamplingTesselationEvaluationShaderCode(const samplerType&		sampler_type,
													const samplingFunction& sampling_function,
													std::string&			out_tesselation_evaluation_shader_code);

	void getSamplingVertexShaderCode(const samplerType& sampler_type, const samplingFunction& sampling_function,
									 std::string& out_vertex_shader_code);

	void getShadowSamplingFunctionCall(samplingFunction sampling_function, const glw::GLchar* color_type,
									   glw::GLuint n_components, const glw::GLchar* attribute_name_prefix,
									   const glw::GLchar* attribute_index, const glw::GLchar* color_variable_name,
									   const glw::GLchar* color_variable_index, const glw::GLchar* sampler_name,
									   std::string& out_code);

	static bool isSamplerSupportedByFunction(const samplerType sampler_type, const samplingFunction sampling_function);

	void link(programDefinition& info);

	void logCompilationLog(const shaderDefinition& info);

	void logLinkingLog(const programDefinition& info);

	void logProgram(const programDefinition& info);

	void prepareCompresedTexture(const textureDefinition& texture, const formatDefinition& format,
								 const resolutionDefinition& resolution, bool mutability);

	void prepareTexture(const textureDefinition& texture, const formatDefinition& format,
						const resolutionDefinition& resolution, bool mutability);

	void setupSharedStorageBuffer(const attributeDefinition& attribute, const bufferCollection& buffers,
								  glw::GLuint program_id);

	void setupSharedStorageBuffers(const formatDefinition& format, const samplingFunction& sampling_function,
								   const bufferCollection& buffers, glw::GLuint program_id);

	void testFormats(formatsVectorType& formats, resolutionsVectorType& resolutions);

	void testTexture(const formatDefinition& format, bool mutability, const resolutionDefinition& resolution,
					 textureDefinition& texture, programCollectionForFormat& program_collection);

	bool verifyResult(const formatDefinition& format, const resolutionDefinition& resolution,
					  const samplingFunction sampling_function, unsigned char* data);

	bool verifyResultHelper(const formatDefinition& format, const resolutionDefinition& resolution,
							const componentProvider& component_provider, unsigned char* data);

	/* Fields */
	formatsVectorType			m_formats;
	formatsVectorType			m_compressed_formats;
	samplingFunctionsVectorType m_functions;
	mutablitiesVectorType		m_mutabilities;
	resolutionsVectorType		m_resolutions;
	resolutionsVectorType		m_compressed_resolutions;
	shadersVectorType			m_shaders;

	glw::GLuint m_framebuffer_object_id;

	/* Static variables */
	static const glw::GLuint m_get_type_api_status_program_resource;
	static const glw::GLuint m_get_type_api_status_uniform;

	/* Fields */
	glw::GLuint compiled_shaders;
	glw::GLuint invalid_shaders;
	glw::GLuint linked_programs;
	glw::GLuint invalid_programs;
	glw::GLuint tested_cases;
	glw::GLuint failed_cases;
	glw::GLuint invalid_type_cases;

	/* Static variables */
	static const glw::GLchar* const attribute_grad_x;
	static const glw::GLchar* const attribute_grad_y;
	static const glw::GLchar* const attribute_lod;
	static const glw::GLchar* const attribute_refZ;
	static const glw::GLchar* const attribute_texture_coordinate;
	static const glw::GLchar* const compute_shader_body;
	static const glw::GLchar* const compute_shader_layout_binding;
	static const glw::GLchar* const compute_shader_buffer;
	static const glw::GLchar* const compute_shader_color;
	static const glw::GLchar* const compute_shader_image_store;
	static const glw::GLchar* const compute_shader_layout;
	static const glw::GLchar* const compute_shader_param;
	static const glw::GLchar* const fragment_shader_input;
	static const glw::GLchar* const fragment_shader_output;
	static const glw::GLchar* const fragment_shader_pass_through_body_code;
	static const glw::GLchar* const fragment_shader_sampling_body_code;
	static const glw::GLchar* const geometry_shader_emit_vertex_code;
	static const glw::GLchar* const geometry_shader_extension;
	static const glw::GLchar* const geometry_shader_layout;
	static const glw::GLchar* const geometry_shader_sampling_body_code;
	static const glw::GLchar* const image_float;
	static const glw::GLchar* const image_int;
	static const glw::GLchar* const image_name;
	static const glw::GLchar* const image_uint;
	static const glw::GLchar* const interpolation_flat;
	static const glw::GLchar* const sampler_depth;
	static const glw::GLchar* const sampler_float;
	static const glw::GLchar* const sampler_int;
	static const glw::GLchar* const sampler_name;
	static const glw::GLchar* const sampler_uint;
	static const glw::GLchar* const shader_code_preamble;
	static const glw::GLchar* const shader_precision;
	static const glw::GLchar* const shader_input;
	static const glw::GLchar* const shader_layout;
	static const glw::GLchar* const shader_output;
	static const glw::GLchar* const shader_uniform;
	static const glw::GLchar* const shader_writeonly;
	static const glw::GLchar* const tesselation_control_shader_layout;
	static const glw::GLchar* const tesselation_control_shader_sampling_body_code;
	static const glw::GLchar* const tesselation_control_shader_output;
	static const glw::GLchar* const tesselation_evaluation_shader_input;
	static const glw::GLchar* const tesselation_evaluation_shader_layout;
	static const glw::GLchar* const tesselation_evaluation_shader_pass_through_body_code;
	static const glw::GLchar* const tesselation_evaluation_shader_sampling_body_code;
	static const glw::GLchar* const tesselation_shader_extension;
	static const glw::GLchar* const texture_func;
	static const glw::GLchar* const textureGather_func;
	static const glw::GLchar* const textureGrad_func;
	static const glw::GLchar* const textureLod_func;
	static const glw::GLchar* const type_float;
	static const glw::GLchar* const type_ivec4;
	static const glw::GLchar* const type_uint;
	static const glw::GLchar* const type_uvec4;
	static const glw::GLchar* const type_vec3;
	static const glw::GLchar* const type_vec4;
	static const glw::GLchar* const vertex_shader_body_code;
	static const glw::GLchar* const vertex_shader_input;
	static const glw::GLchar* const vertex_shader_output;
	static const glw::GLchar* const vertex_shader_position;
};

} /* glcts */

#endif // _ESEXTCTEXTURECUBEMAPARRAYSAMPLING_HPP
