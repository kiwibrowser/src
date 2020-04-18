#ifndef _GL4CENHANCEDLAYOUTSTESTS_HPP
#define _GL4CENHANCEDLAYOUTSTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/**
 * \file  gl4cEnhancedLayoutsTests.hpp
 * \brief Declares test classes for "Enhanced Layouts" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace tcu
{
class MessageBuilder;
} /* namespace tcu */

namespace gl4cts
{

namespace EnhancedLayouts
{
namespace Utils
{
/** Represents data type
 *
 **/
struct Type
{
public:
	enum TYPES
	{
		Float,
		Double,
		Int,
		Uint,
	};

	/* Public methods */
	/* Functionality */
	std::vector<glw::GLubyte> GenerateData() const;
	std::vector<glw::GLubyte> GenerateDataPacked() const;
	glw::GLuint GetActualAlignment(glw::GLuint align, bool is_array) const;
	glw::GLuint GetBaseAlignment(bool is_array) const;
	std::string GetGLSLConstructor(const glw::GLvoid* data) const;
	const glw::GLchar* GetGLSLTypeName() const;
	glw::GLuint GetLocations(bool is_vs_input = false) const;
	glw::GLuint GetSize(const bool is_std140 = false) const;
	glw::GLenum GetTypeGLenum() const;
	glw::GLuint GetNumComponents() const;

	/* Public static routines */
	/* Functionality */
	static glw::GLuint CalculateStd140Stride(glw::GLuint alignment, glw::GLuint n_columns,
											 glw::GLuint n_array_elements);

	static bool DoesTypeSupportMatrix(TYPES type);

	static glw::GLuint GetActualOffset(glw::GLuint start_offset, glw::GLuint actual_alignment);

	static Type GetType(TYPES basic_type, glw::GLuint n_columns, glw::GLuint n_rows);

	static glw::GLuint GetTypeSize(TYPES type);

	/* GL gets */
	static glw::GLenum GetTypeGLenum(TYPES type);

	/* Public fields */
	TYPES		m_basic_type;
	glw::GLuint m_n_columns;
	glw::GLuint m_n_rows;

	/* Public constants */
	static const Type _double;
	static const Type dmat2;
	static const Type dmat2x3;
	static const Type dmat2x4;
	static const Type dmat3x2;
	static const Type dmat3;
	static const Type dmat3x4;
	static const Type dmat4x2;
	static const Type dmat4x3;
	static const Type dmat4;
	static const Type dvec2;
	static const Type dvec3;
	static const Type dvec4;
	static const Type _float;
	static const Type _int;
	static const Type ivec2;
	static const Type ivec3;
	static const Type ivec4;
	static const Type mat2;
	static const Type mat2x3;
	static const Type mat2x4;
	static const Type mat3x2;
	static const Type mat3;
	static const Type mat3x4;
	static const Type mat4x2;
	static const Type mat4x3;
	static const Type mat4;
	static const Type vec2;
	static const Type vec3;
	static const Type vec4;
	static const Type uint;
	static const Type uvec2;
	static const Type uvec3;
	static const Type uvec4;
};

/** Represents buffer instance
 * Provides basic buffer functionality
 **/
class Buffer
{
public:
	/* Public enums */
	enum BUFFERS
	{
		Array,
		Element,
		Shader_Storage,
		Texture,
		Transform_feedback,
		Uniform,
	};

	enum USAGE
	{
		DynamicCopy,
		DynamicDraw,
		DynamicRead,
		StaticCopy,
		StaticDraw,
		StaticRead,
		StreamCopy,
		StreamDraw,
		StreamRead,
	};

	enum ACCESS
	{
		ReadOnly,
		WriteOnly,
		ReadWrite,
	};

	/* Public methods */
	/* Ctr & Dtr */
	Buffer(deqp::Context& context);
	~Buffer();

	/* Init & Release */
	void Init(BUFFERS buffer, USAGE usage, glw::GLsizeiptr size, glw::GLvoid* data);
	void Release();

	/* Functionality */
	void Bind() const;
	void BindBase(glw::GLuint index) const;

	void BindRange(glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size) const;

	void Data(USAGE usage, glw::GLsizeiptr size, glw::GLvoid* data);

	glw::GLvoid* Map(ACCESS access);

	void SubData(glw::GLintptr offset, glw::GLsizeiptr size, glw::GLvoid* data);

	void UnMap();

	/* Public static routines */
	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, BUFFERS buffer);

	static void BindBase(const glw::Functions& gl, glw::GLuint id, BUFFERS buffer, glw::GLuint index);

	static void BindRange(const glw::Functions& gl, glw::GLuint id, BUFFERS buffer, glw::GLuint index,
						  glw::GLintptr offset, glw::GLsizeiptr size);

	static void Data(const glw::Functions& gl, BUFFERS buffer, USAGE usage, glw::GLsizeiptr size, glw::GLvoid* data);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void* Map(const glw::Functions& gl, BUFFERS buffer, ACCESS access);

	static void SubData(const glw::Functions& gl, BUFFERS buffer, glw::GLintptr offset, glw::GLsizeiptr size,
						glw::GLvoid* data);

	static void UnMap(const glw::Functions& gl, BUFFERS buffer);

	/* GL gets */
	static glw::GLenum GetAccessGLenum(ACCESS access);
	static glw::GLenum GetBufferGLenum(BUFFERS buffer);
	static glw::GLenum GetUsageGLenum(USAGE usage);

	/* Gets */
	static const glw::GLchar* GetBufferName(BUFFERS buffer);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

	/* Buffer type maybe changed for different cases*/
	BUFFERS m_buffer;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/** Represents framebuffer
 * Provides basic functionality
 **/
class Framebuffer
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Framebuffer(deqp::Context& context);
	~Framebuffer();

	/* Init & Release */
	void Init();
	void Release();

	/* Functionality */
	void AttachTexture(glw::GLenum attachment, glw::GLuint texture_id, glw::GLuint width, glw::GLuint height);

	void Bind();
	void Clear(glw::GLenum mask);

	void ClearColor(glw::GLfloat red, glw::GLfloat green, glw::GLfloat blue, glw::GLfloat alpha);

	/* Public static routines */
	static void AttachTexture(const glw::Functions& gl, glw::GLenum attachment, glw::GLuint texture_id,
							  glw::GLuint width, glw::GLuint height);

	static void Bind(const glw::Functions& gl, glw::GLuint id);

	static void Clear(const glw::Functions& gl, glw::GLenum mask);

	static void ClearColor(const glw::Functions& gl, glw::GLfloat red, glw::GLfloat green, glw::GLfloat blue,
						   glw::GLfloat alpha);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/** Represents shader instance.
 * Provides basic functionality for shaders.
 **/
class Shader
{
public:
	/* Public enums */
	enum STAGES
	{
		COMPUTE = 0,
		VERTEX,
		TESS_CTRL,
		TESS_EVAL,
		GEOMETRY,
		FRAGMENT,

		/* */
		STAGE_MAX
	};

	/* Public types */

	class InvalidSourceException : public std::exception
	{
	public:
		InvalidSourceException(const glw::GLchar* error_message, const std::string& source, STAGES stage);

		virtual ~InvalidSourceException() throw()
		{
		}

		virtual const char* what() const throw();

		void log(deqp::Context& context) const;

		std::string m_message;
		std::string m_source;
		STAGES		m_stage;
	};

	/* Public methods */
	/* Ctr & Dtr */
	Shader(deqp::Context& context);
	~Shader();

	/* Init & Realese */
	void Init(STAGES stage, const std::string& source);
	void Release();

	/* Public static routines */
	/* Functionality */
	static void Compile(const glw::Functions& gl, glw::GLuint id);

	static void Create(const glw::Functions& gl, STAGES stage, glw::GLuint& out_id);

	static void Source(const glw::Functions& gl, glw::GLuint id, const std::string& source);

	/* GL gets */
	static glw::GLenum GetShaderStageGLenum(STAGES stage);

	/* Get stage name */
	static const glw::GLchar* GetStageName(STAGES stage);

	/* Logs sources */
	static void LogSource(deqp::Context& context, const std::string& source, STAGES stage);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private types */
	class CompilationException : public std::exception
	{
	public:
		CompilationException(const glw::GLchar* message);

		virtual ~CompilationException() throw()
		{
		}

		virtual const char* what() const throw();

		std::string m_message;
	};

	/* Private fields */
	deqp::Context& m_context;
};

/* Forward declaration */
struct Interface;

/** Represents GLSL variable
 *
 **/
struct Variable
{
public:
	/* Typedefs */
	typedef std::vector<Variable>  Vector;
	typedef std::vector<Variable*> PtrVector;

	/* Enums */
	enum STORAGE
	{
		VARYING_INPUT,
		VARYING_OUTPUT,
		UNIFORM,
		SSB,
		MEMBER,

		/* */
		STORAGE_MAX
	};

	enum VARYING_DIRECTION
	{
		INPUT,
		OUTPUT,
	};

	enum FLAVOUR
	{
		BASIC,
		ARRAY,
		INDEXED_BY_INVOCATION_ID,
	};

	/**/
	enum TYPE
	{
		BUILTIN,
		INTERFACE,
	};

	/* Types */
	struct Descriptor
	{
		/* */
		typedef std::vector<Descriptor> Vector;

		/* */
		Descriptor(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,
				   glw::GLint expected_location, const Type& type, glw::GLboolean normalized,
				   glw::GLuint n_array_elements, glw::GLint expected_stride_of_element, glw::GLuint offset);

		Descriptor(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_componenet,
				   glw::GLint expected_location, Interface* interface, glw::GLuint n_array_elements,
				   glw::GLint expected_stride_of_element, glw::GLuint offset);

		/* */
		std::string GetDefinition(FLAVOUR flavour, STORAGE storage) const;

		/* */
		glw::GLint	 m_expected_component;
		glw::GLint	 m_expected_location;
		glw::GLint	 m_expected_stride_of_element;
		glw::GLuint	m_n_array_elements;
		std::string	m_name;
		glw::GLboolean m_normalized;
		glw::GLuint	m_offset;
		std::string	m_qualifiers;

		TYPE m_type;
		union {
			Type	   m_builtin;
			Interface* m_interface;
		};
	};

	/* Constructors */
	template <typename T>
	Variable(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,
			 glw::GLint expected_location, const Type& type, glw::GLboolean normalized, glw::GLuint n_array_elements,
			 glw::GLint expected_stride_of_element, glw::GLuint offset, const T* data, size_t data_size,
			 STORAGE storage)
		: m_data((glw::GLvoid*)data)
		, m_data_size(data_size)
		, m_descriptor(name, qualifiers, expected_component, expected_location, type, normalized, n_array_elements,
					   expected_stride_of_element, offset)
		, m_storage(storage)
	{
	}

	template <typename T>
	Variable(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,
			 glw::GLint expected_location, Interface* interface, glw::GLuint n_array_elements,
			 glw::GLint expected_stride_of_element, glw::GLuint offset, const T* data, size_t data_size,
			 STORAGE storage)
		: m_data((glw::GLvoid*)data)
		, m_data_size(data_size)
		, m_descriptor(name, qualifiers, expected_component, expected_location, interface, n_array_elements,
					   expected_stride_of_element, offset)
		, m_storage(storage)
	{
	}

	Variable(const Variable& var);

	/* Functionality */
	std::string GetDefinition(FLAVOUR flavour) const;
	glw::GLuint GetSize() const;
	glw::GLuint GetStride() const;
	bool		IsBlock() const;
	bool		IsStruct() const;
	/* Static routines */
	static FLAVOUR GetFlavour(Shader::STAGES stage, VARYING_DIRECTION direction);
	static std::string GetReference(const std::string& parent_name, const Descriptor& variable, FLAVOUR flavour,
									glw::GLuint array_index);

	/* Fields */
	glw::GLvoid* m_data;
	size_t		 m_data_size;
	Descriptor   m_descriptor;
	STORAGE		 m_storage;

	/* Constants */
	static const glw::GLint m_automatic_location;
};

/* Define the methods NAME, that will add new variable to VECTOR with STORAGE set		*/
#define DEFINE_VARIABLE_CLASS(NAME, STORAGE, VECTOR)                                                                  \
	template <typename T>                                                                                             \
	Variable* NAME(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,             \
				   glw::GLint expected_location, const Type& type, glw::GLboolean normalized,                         \
				   glw::GLuint n_array_elements, glw::GLint expected_stride_of_element, glw::GLuint offset,           \
				   const T* data, size_t data_size)                                                                   \
	{                                                                                                                 \
		Variable* var = new Variable(name, qualifiers, expected_component, expected_location, type, normalized,       \
									 n_array_elements, expected_stride_of_element, offset, data, data_size, STORAGE); \
		if (0 == var)                                                                                                 \
		{                                                                                                             \
			TCU_FAIL("Memory allocation");                                                                            \
		}                                                                                                             \
		VECTOR.push_back(var);                                                                                        \
		return VECTOR.back();                                                                                         \
	}                                                                                                                 \
                                                                                                                      \
	template <typename T>                                                                                             \
	Variable* NAME(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,             \
				   glw::GLint expected_location, Interface* interface, glw::GLuint n_array_elements,                  \
				   glw::GLint expected_stride_of_element, glw::GLuint offset, const T* data, size_t data_size)        \
	{                                                                                                                 \
		Variable* var = new Variable(name, qualifiers, expected_component, expected_location, interface,              \
									 n_array_elements, expected_stride_of_element, offset, data, data_size, STORAGE); \
		if (0 == var)                                                                                                 \
		{                                                                                                             \
			TCU_FAIL("Memory allocation");                                                                            \
		}                                                                                                             \
		VECTOR.push_back(var);                                                                                        \
		return VECTOR.back();                                                                                         \
	}

/** Represents structures and block
 *
 **/
struct Interface
{
public:
	/* Typedefs */
	typedef std::vector<Interface>  Vector;
	typedef std::vector<Interface*> PtrVector;

	/**/
	enum TYPE
	{
		STRUCT,
		BLOCK
	};

	/* Constructor */
	Interface(const glw::GLchar* name, TYPE type);

	/*  */
	Variable::Descriptor* Member(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,
								 glw::GLint expected_location, const Type& type, glw::GLboolean normalized,
								 glw::GLuint n_array_elements, glw::GLint expected_stride_of_element,
								 glw::GLuint offset);
	Variable::Descriptor* Member(const glw::GLchar* name, const glw::GLchar* qualifiers, glw::GLint expected_component,
								 glw::GLint expected_location, Interface* interface, glw::GLuint n_array_elements,
								 glw::GLint expected_stride_of_element, glw::GLuint offset);

	/**/
	Variable::Descriptor* AddMember(const Variable::Descriptor& member);

	std::string GetDefinition() const;

	/**/
	Variable::Descriptor::Vector m_members;
	std::string					 m_name;
	TYPE						 m_type;
};

struct ShaderInterface
{
	ShaderInterface(Shader::STAGES stage);

	DEFINE_VARIABLE_CLASS(Input, Variable::VARYING_INPUT, m_inputs);
	DEFINE_VARIABLE_CLASS(Output, Variable::VARYING_OUTPUT, m_outputs);
	DEFINE_VARIABLE_CLASS(Uniform, Variable::UNIFORM, m_uniforms);
	DEFINE_VARIABLE_CLASS(SSB, Variable::SSB, m_ssb_blocks);

	/**/
	std::string GetDefinitionsGlobals() const;
	std::string GetDefinitionsInputs() const;
	std::string GetDefinitionsOutputs() const;
	std::string GetDefinitionsSSBs() const;
	std::string GetDefinitionsUniforms() const;

	std::string			m_globals;
	Variable::PtrVector m_inputs;
	Variable::PtrVector m_outputs;
	Variable::PtrVector m_uniforms;
	Variable::PtrVector m_ssb_blocks;

	Shader::STAGES m_stage;
};

struct VaryingConnection
{
	/* */
	typedef std::vector<VaryingConnection> Vector;

	/* */
	VaryingConnection(Variable* in, Variable* out);

	/* */
	Variable* m_in;
	Variable* m_out;
};

struct VaryingPassthrough
{
	/* */
	void Add(Shader::STAGES stage, Variable* in, Variable* out);

	VaryingConnection::Vector& Get(Shader::STAGES stage);

	/**/
	VaryingConnection::Vector m_fragment;
	VaryingConnection::Vector m_geometry;
	VaryingConnection::Vector m_tess_ctrl;
	VaryingConnection::Vector m_tess_eval;
	VaryingConnection::Vector m_vertex;
};

struct ProgramInterface
{

	/* */
	ProgramInterface();
	~ProgramInterface();

	/* */
	Interface* AddInterface(const glw::GLchar* name, Interface::TYPE type);
	Interface* Block(const glw::GLchar* name);
	Interface* GetBlock(const glw::GLchar* name);
	ShaderInterface& GetShaderInterface(Shader::STAGES stage);
	const ShaderInterface& GetShaderInterface(Shader::STAGES stage) const;
	Interface* GetStructure(const glw::GLchar* name);
	Interface* Structure(const glw::GLchar* name);

	/**/
	std::string GetDefinitionsStructures() const;
	std::string GetInterfaceForStage(Shader::STAGES stage) const;

	/* */
	Interface* CloneBlockForStage(const Interface& block, Shader::STAGES stage, Variable::STORAGE storage,
								  const glw::GLchar* prefix);
	void CloneVertexInterface(VaryingPassthrough& variable_pass);

	/* */
	static const glw::GLchar* GetStagePrefix(Shader::STAGES stage, Variable::STORAGE storage);

	/* */
	Interface::PtrVector m_structures;
	Interface::PtrVector m_blocks;

	ShaderInterface m_compute;
	ShaderInterface m_vertex;
	ShaderInterface m_tess_ctrl;
	ShaderInterface m_tess_eval;
	ShaderInterface m_geometry;
	ShaderInterface m_fragment;

	//Variable::Vector     m_fragment_outputs;
	//Variable::PtrVector  m_captured_varyings;

private:
	/* */
	void cloneVariableForStage(const Variable& variable, Shader::STAGES stage, const glw::GLchar* prefix,
							   VaryingPassthrough& varying_passthrough);
	Variable* cloneVariableForStage(const Variable& variable, Shader::STAGES stage, Variable::STORAGE storage,
									const glw::GLchar* prefix);
	void replaceBinding(Variable& variable, Shader::STAGES stage);
};

/** Represents program pipeline
 *
 **/
class Pipeline
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Pipeline(deqp::Context& context);
	~Pipeline();

	/* Init & Release */
	void Init();
	void Release();

	/* Functionality */
	void Bind();
	void UseProgramStages(glw::GLuint program_id, glw::GLenum stages);

	/* Public static routines */
	/* Functionality */
	void Bind(const glw::Functions& gl, glw::GLuint id);
	void UseProgramStages(const glw::Functions& gl, glw::GLuint id, glw::GLuint program_id, glw::GLenum stages);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/** Represents program instance.
 * Provides basic functionality
 **/
class Program
{
public:
	/* Public types */
	class BuildException : public std::exception
	{
	public:
		BuildException(const glw::GLchar* error_message, const std::string compute_shader,
					   const std::string fragment_shader, const std::string geometry_shader,
					   const std::string tess_ctrl_shader, const std::string tess_eval_shader,
					   const std::string vertex_shader);
		virtual ~BuildException() throw()
		{
		}

		virtual const char* what() const throw();

		void log(deqp::Context& context) const;

		std::string m_error_message;
		std::string m_compute_shader;
		std::string m_fragment_shader;
		std::string m_geometry_shader;
		std::string m_tess_ctrl_shader;
		std::string m_tess_eval_shader;
		std::string m_vertex_shader;
	};

	typedef std::vector<std::string> NameVector;

	/* Public methods */
	/* Ctr & Dtr */
	Program(deqp::Context& context);
	~Program();

	/* Init & Release */
	void Init(const std::string& compute_shader, const std::string& fragment_shader, const std::string& geometry_shader,
			  const std::string& tessellation_control_shader, const std::string& tessellation_evaluation_shader,
			  const std::string& vertex_shader, const NameVector& captured_varyings, bool capture_interleaved,
			  bool is_separable);

	void Init(const std::string& compute_shader, const std::string& fragment_shader, const std::string& geometry_shader,
			  const std::string& tessellation_control_shader, const std::string& tessellation_evaluation_shader,
			  const std::string& vertex_shader, bool is_separable);

	void Release();

	/* Functionality */
	void GetActiveUniformsiv(glw::GLsizei count, const glw::GLuint* indices, glw::GLenum pname,
							 glw::GLint* params) const;

	glw::GLint GetAttribLocation(const std::string& name) const;

	void GetResource(glw::GLenum interface, glw::GLuint index, glw::GLenum property, glw::GLsizei buf_size,
					 glw::GLint* params) const;

	glw::GLuint GetResourceIndex(const std::string& name, glw::GLenum interface) const;

	void GetUniformIndices(glw::GLsizei count, const glw::GLchar** names, glw::GLuint* indices) const;

	glw::GLint GetUniformLocation(const std::string& name) const;
	void Use() const;

	/* Public static routines */
	/* Functionality */
	static void Attach(const glw::Functions& gl, glw::GLuint program_id, glw::GLuint shader_id);

	static void Capture(const glw::Functions& gl, glw::GLuint id, const NameVector& captured_varyings,
						bool capture_interleaved);

	static void Create(const glw::Functions& gl, glw::GLuint& out_id);

	static void GetActiveUniformsiv(const glw::Functions& gl, glw::GLuint program_id, glw::GLsizei count,
									const glw::GLuint* indices, glw::GLenum pname, glw::GLint* params);

	static void GetUniformIndices(const glw::Functions& gl, glw::GLuint program_id, glw::GLsizei count,
								  const glw::GLchar** names, glw::GLuint* indices);

	static void Link(const glw::Functions& gl, glw::GLuint id);

	static void Uniform(const glw::Functions& gl, const Type& type, glw::GLsizei count, glw::GLint location,
						const glw::GLvoid* data);

	static void Use(const glw::Functions& gl, glw::GLuint id);

	/* Get locations */
	static glw::GLint GetAttribLocation(const glw::Functions& gl, glw::GLuint id, const std::string& name);

	static void GetResource(const glw::Functions& gl, glw::GLuint id, glw::GLenum interface, glw::GLuint index,
							glw::GLenum property, glw::GLsizei buf_size, glw::GLint* params);

	static glw::GLuint GetResourceIndex(const glw::Functions& gl, glw::GLuint id, const std::string& name,
										glw::GLenum interface);

	static glw::GLint GetUniformLocation(const glw::Functions& gl, glw::GLuint id, const std::string& name);

	/* Public fields */
	glw::GLuint m_id;

	Shader m_compute;
	Shader m_fragment;
	Shader m_geometry;
	Shader m_tess_ctrl;
	Shader m_tess_eval;
	Shader m_vertex;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private types */
	class LinkageException : public std::exception
	{
	public:
		LinkageException(const glw::GLchar* error_message);

		virtual ~LinkageException() throw()
		{
		}

		virtual const char* what() const throw();

		std::string m_error_message;
	};

	/* Private fields */
	deqp::Context& m_context;
};

class Texture
{
public:
	/* Public enums */
	enum TYPES
	{
		TEX_BUFFER,
		TEX_2D,
		TEX_2D_RECT,
		TEX_2D_ARRAY,
		TEX_3D,
		TEX_CUBE,
		TEX_1D,
		TEX_1D_ARRAY,
	};

	/* Public methods */
	/* Ctr & Dtr */
	Texture(deqp::Context& context);
	~Texture();

	/* Init & Release */
	void Init(TYPES tex_type, glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum internal_format,
			  glw::GLenum format, glw::GLenum type, glw::GLvoid* data);

	void Init(glw::GLenum internal_format, glw::GLuint buffer_id);

	void Release();

	/* Functionality */
	void Bind() const;
	void Get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data) const;

	/* Public static routines */
	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, TYPES tex_type);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void Get(const glw::Functions& gl, TYPES tex_type, glw::GLenum format, glw::GLenum type,
					glw::GLvoid* out_data);

	static void Storage(const glw::Functions& gl, TYPES tex_type, glw::GLuint width, glw::GLuint height,
						glw::GLuint depth, glw::GLenum internal_format);

	static void TexBuffer(const glw::Functions& gl, glw::GLenum internal_format, glw::GLuint& buffer_id);

	static void Update(const glw::Functions& gl, TYPES tex_type, glw::GLuint width, glw::GLuint height,
					   glw::GLuint depth, glw::GLenum format, glw::GLenum type, glw::GLvoid* data);

	/* GL gets */
	static glw::GLenum GetTargetGLenum(TYPES tex_type);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	deqp::Context& m_context;
	TYPES		   m_type;
};

/** Represents Vertex array object
 * Provides basic functionality
 **/
class VertexArray
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	VertexArray(deqp::Context& Context);
	~VertexArray();

	/* Init & Release */
	void Init();
	//void Init(const ProgramInterface& program_interface,
	//                glw::GLuint       vertex_buffer,
	//                glw::GLuint       index_buffer);
	void Release();

	void Attribute(glw::GLuint index, const Type& type, glw::GLuint n_array_elements, glw::GLboolean normalized,
				   glw::GLsizei stride, const glw::GLvoid* pointer);

	void Bind();

	/* Public static methods */
	static void AttribPointer(const glw::Functions& gl, glw::GLuint index, const Type& type,
							  glw::GLuint n_array_elements, glw::GLboolean normalized, glw::GLsizei stride,
							  const glw::GLvoid* pointer);

	static void Bind(const glw::Functions& gl, glw::GLuint id);

	static void Disable(const glw::Functions& gl, glw::GLuint index, const Type& type, glw::GLuint n_array_elements);

	static void Divisor(const glw::Functions& gl, glw::GLuint index, glw::GLuint divisor);

	static void Enable(const glw::Functions& gl, glw::GLuint index, const Type& type, glw::GLuint n_array_elements);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	deqp::Context& m_context;
};

/* UniformN*v prototypes */
typedef GLW_APICALL void(GLW_APIENTRY* uniformNdv)(glw::GLint, glw::GLsizei, const glw::GLdouble*);
typedef GLW_APICALL void(GLW_APIENTRY* uniformNfv)(glw::GLint, glw::GLsizei, const glw::GLfloat*);
typedef GLW_APICALL void(GLW_APIENTRY* uniformNiv)(glw::GLint, glw::GLsizei, const glw::GLint*);
typedef GLW_APICALL void(GLW_APIENTRY* uniformNuiv)(glw::GLint, glw::GLsizei, const glw::GLuint*);
typedef GLW_APICALL void(GLW_APIENTRY* uniformMatrixNdv)(glw::GLint, glw::GLsizei, glw::GLboolean,
														 const glw::GLdouble*);
typedef GLW_APICALL void(GLW_APIENTRY* uniformMatrixNfv)(glw::GLint, glw::GLsizei, glw::GLboolean, const glw::GLfloat*);

/* Public static methods */
/* UniformN*v routine getters */
uniformNdv getUniformNdv(const glw::Functions& gl, glw::GLuint n_rows);
uniformNfv getUniformNfv(const glw::Functions& gl, glw::GLuint n_rows);
uniformNiv getUniformNiv(const glw::Functions& gl, glw::GLuint n_rows);
uniformNuiv getUniformNuiv(const glw::Functions& gl, glw::GLuint n_rows);
uniformMatrixNdv getUniformMatrixNdv(const glw::Functions& gl, glw::GLuint n_columns, glw::GLuint n_rows);
uniformMatrixNfv getUniformMatrixNfv(const glw::Functions& gl, glw::GLuint n_columns, glw::GLuint n_rows);

/* Stuff */
bool checkProgramInterface(const ProgramInterface& program_interface, Program& program, std::stringstream& stream);

bool isExtensionSupported(deqp::Context& context, const glw::GLchar* extension_name);

bool isGLVersionAtLeast(const glw::Functions& gl, glw::GLint required_major, glw::GLint required_minor);

void replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text, std::string& string);

void replaceAllTokens(const glw::GLchar* token, const glw::GLchar* text, std::string& string);

glw::GLuint roundUpToPowerOf2(glw::GLuint value);

void insertElementOfList(const glw::GLchar* element, const glw::GLchar* separator, size_t& search_position,
						 std::string& string);

void endList(const glw::GLchar* separator, size_t& search_position, std::string& string);
} /* Utils namespace */

/** Base class for tests **/
class TestBase : public deqp::TestCase
{
public:
	/* Public methods */
	TestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~TestBase()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Methods to be implemented by child class */
	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool testCase(glw::GLuint test_case_index) = 0;
	virtual void testInit();

	/* Routines avaiable for children */
	glw::GLuint calculateStride(const Utils::Interface& interface) const;
	void generateData(const Utils::Interface& interface, glw::GLuint offset, std::vector<glw::GLubyte>& out_data) const;

	glw::GLint getLastInputLocation(Utils::Shader::STAGES stage, const Utils::Type& type, glw::GLuint array_lenth, bool ignore_prev_stage);

	glw::GLint getLastOutputLocation(Utils::Shader::STAGES stage, const Utils::Type& type, glw::GLuint array_lenth, bool ignore_next_stage);

	Utils::Type getType(glw::GLuint index) const;
	std::string getTypeName(glw::GLuint index) const;
	glw::GLuint getTypesNumber() const;

	bool isFlatRequired(Utils::Shader::STAGES stage, const Utils::Type& type, Utils::Variable::STORAGE storage) const;

private:
	/* Private methods */
	bool test();
};

/** Base class for test doing Buffer alghorithm **/
class BufferTestBase : public TestBase
{
public:
	/* Public methods */
	BufferTestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~BufferTestBase()
	{
	}

protected:
	/* */
	struct bufferDescriptor
	{
		/* Typedefs */
		typedef std::vector<bufferDescriptor> Vector;

		/* Fileds */
		std::vector<glw::GLubyte> m_expected_data;
		std::vector<glw::GLubyte> m_initial_data;
		glw::GLuint				  m_index;
		Utils::Buffer::BUFFERS	m_target;

		/* Constants */
		static const glw::GLuint m_non_indexed;
	};

	class bufferCollection
	{
	public:
		struct pair
		{
			Utils::Buffer*	m_buffer;
			bufferDescriptor* m_descriptor;
		};

		~bufferCollection();

		typedef std::vector<pair> Vector;

		Vector m_vector;
	};

	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);

	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getCapturedVaryings(glw::GLuint test_case_index, Utils::Program::NameVector& captured_varyings, glw::GLint* xfb_components);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual bool inspectProgram(glw::GLuint test_case_index, Utils::Program& program, std::stringstream& out_stream);

	virtual bool testCase(glw::GLuint test_case_index);
	virtual bool verifyBuffers(bufferCollection& buffers);

private:
	void		cleanBuffers();
	std::string getShaderTemplate(Utils::Shader::STAGES stage);
	void prepareBuffer(Utils::Buffer& buffer, bufferDescriptor& descriptor);
	void prepareBuffers(bufferDescriptor::Vector& descriptors, bufferCollection& out_buffers);

	/* */
};

class NegativeTestBase : public TestBase
{
public:
	/* Public methods */
	NegativeTestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~NegativeTestBase()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage) = 0;
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual bool testCase(glw::GLuint test_case_index);
};

/** Base class for test doing Texture alghorithm **/
class TextureTestBase : public TestBase
{
public:
	/* Public methods */
	TextureTestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~TextureTestBase()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool checkResults(glw::GLuint test_case_index, Utils::Texture& color_0);

	virtual void executeDispatchCall(glw::GLuint test_case_index);
	virtual void executeDrawCall(glw::GLuint test_case_index);

	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getPassSnippet(glw::GLuint test_case_index, Utils::VaryingPassthrough& varying_passthrough,
									   Utils::Shader::STAGES stage);

	virtual std::string getVerificationSnippet(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
											   Utils::Shader::STAGES stage);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isDrawRelevant(glw::GLuint test_case_index);

	virtual void prepareAttributes(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								   Utils::Buffer& buffer, Utils::VertexArray& vao);

	virtual void prepareAttribLocation(Utils::Program& program, Utils::ProgramInterface& program_interface);

	virtual void prepareFragmentDataLoc(Utils::Program& program, Utils::ProgramInterface& program_interface);

	virtual void prepareFramebuffer(Utils::Framebuffer& framebuffer, Utils::Texture& color_0_texture);

	virtual void prepareImage(glw::GLint location, Utils::Texture& image_texture) const;

	virtual void prepareSSBs(glw::GLuint test_case_index, Utils::ShaderInterface& si, Utils::Program& program,
							 Utils::Buffer& buffer);

	virtual void prepareSSBs(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
							 Utils::Program& program, Utils::Buffer& cs_buffer);

	virtual void prepareSSBs(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
							 Utils::Program& program, Utils::Buffer& fs_buffer, Utils::Buffer& gs_buffer,
							 Utils::Buffer& tcs_buffer, Utils::Buffer& tes_buffer, Utils::Buffer& vs_buffer);

	virtual void prepareUniforms(glw::GLuint test_case_index, Utils::ShaderInterface& si, Utils::Program& program,
								 Utils::Buffer& buffer);

	virtual void prepareUniforms(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								 Utils::Program& program, Utils::Buffer& cs_buffer);

	virtual void prepareUniforms(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								 Utils::Program& program, Utils::Buffer& fs_buffer, Utils::Buffer& gs_buffer,
								 Utils::Buffer& tcs_buffer, Utils::Buffer& tes_buffer, Utils::Buffer& vs_buffer);

	virtual void prepareUniforms(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								 Utils::Program& fs_program, Utils::Program& gs_program, Utils::Program& tcs_program,
								 Utils::Program& tes_program, Utils::Program& vs_program, Utils::Buffer& fs_buffer,
								 Utils::Buffer& gs_buffer, Utils::Buffer& tcs_buffer, Utils::Buffer& tes_buffer,
								 Utils::Buffer& vs_buffer);
	//virtual void        prepareDrawPrograms   (glw::GLuint                 test_case_index,
	//                                           Utils::Program&             fragment,
	//                                           Utils::Program&             geometry,
	//                                           Utils::Program&             tess_ctrl,
	//                                           Utils::Program&             tess_eval,
	//                                           Utils::Program&             vertex);
	virtual bool testCase(glw::GLuint test_case_index);
	virtual bool testMonolithic(glw::GLuint test_case_index);
	virtual bool testSeparable(glw::GLuint test_case_index);
	virtual bool useComponentQualifier(glw::GLuint test_case_index);
	virtual bool useMonolithicProgram(glw::GLuint test_case_index);

	/* Protected constants */
	static const glw::GLuint m_width;
	static const glw::GLuint m_height;

private:
	std::string getShaderSource(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								Utils::VaryingPassthrough& varying_passthrough, Utils::Shader::STAGES stage);

	const glw::GLchar* getShaderTemplate(Utils::Shader::STAGES stage);

	std::string getVariablePassthrough(const std::string&				  in_parent_name,
									   const Utils::Variable::Descriptor& in_variable,
									   Utils::Variable::FLAVOUR in_flavour, const std::string& out_parent_name,
									   const Utils::Variable::Descriptor& out_variable,
									   Utils::Variable::FLAVOUR			  out_flavour);

	std::string getVariableVerification(const std::string& parent_name, const glw::GLvoid* data,
										const Utils::Variable::Descriptor& variable, Utils::Variable::FLAVOUR flavour);

	void prepareSSB(Utils::Program& program, Utils::Variable& variable, Utils::Buffer& buffer);

	void prepareUniform(Utils::Program& program, Utils::Variable& variable, Utils::Buffer& buffer);
};

/** Implementation of test APIConstantValues. Description follows:
 *
 *  Test verifies values of the following constants are at least as specified:
 * - MAX_TRANSFORM_FEEDBACK_BUFFERS                - 4,
 * - MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS - 64.
 **/
class APIConstantValuesTest : public deqp::TestCase
{
public:
	/* Public methods */
	APIConstantValuesTest(deqp::Context& context);
	virtual ~APIConstantValuesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test APIErrors. Description follows:
 *
 * Test verifies that errors are generated as specified:
 * - GetProgramInterfaceiv should generate INVALID_OPERATION when
 * <programInterface> is TRANSFORM_FEEDBACK_BUFFER and <pname> is
 * MAX_NAME_LENGTH;
 * - GetProgramResourceIndex should generate INVALID_ENUM when
 * <programInterface> is TRANSFORM_FEEDBACK_BUFFER;
 * - GetProgramResourceName should generate INVALID_ENUM when
 * <programInterface> is TRANSFORM_FEEDBACK_BUFFER;
 **/
class APIErrorsTest : public deqp::TestCase
{
public:
	/* Public methods */
	APIErrorsTest(deqp::Context& context);
	virtual ~APIErrorsTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	void checkError(glw::GLenum expected_error, const glw::GLchar* message, bool& test_result);
};

/** Implementation of test GLSLContantValues. Description follows:
 *
 * Test verifies values of the following symbols:
 *
 *     GL_ARB_enhanced_layouts,
 *     gl_MaxTransformFeedbackBuffers,
 *     gl_MaxTransformFeedbackInterleavedComponents.
 *
 * This test implements Texture algorithm. Test following code snippet:
 *
 *     if (1 != GL_ARB_enhanced_layouts)
 *     {
 *         result = 0;
 *     }
 *     else if (MAX_TRANSFORM_FEEDBACK_BUFFERS
 *         != gl_MaxTransformFeedbackBuffers)
 *     {
 *         result = 0;
 *     }
 *     else if (MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS
 *         != gl_MaxTransformFeedbackInterleavedComponents)
 *     {
 *         result = 0;
 *     }
 **/
class GLSLContantValuesTest : public TextureTestBase
{
public:
	GLSLContantValuesTest(deqp::Context& context);
	~GLSLContantValuesTest()
	{
	}

protected:
	virtual std::string getVerificationSnippet(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
											   Utils::Shader::STAGES stage);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
};

/** Implementation of test GLSLContantImmutablity. Description follows:
 *
 * Test verifies that values of the following symbols cannot be changed in
 * shader:
 *
 *     GL_ARB_enhanced_layouts,
 *     gl_MaxTransformFeedbackBuffers,
 *     gl_MaxTransformFeedbackInterleavedComponents.
 *
 * Compile following code snippet:
 *
 *     CONSTANT = 3;
 *
 * It is expected that compilation will fail. Test each shader stage
 * separately. Test each constant separately.
 **/
class GLSLContantImmutablityTest : public NegativeTestBase
{
public:
	/* Public methods */
	GLSLContantImmutablityTest(deqp::Context& context);

	virtual ~GLSLContantImmutablityTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private enums */
	enum CONSTANTS
	{
		GL_ARB_ENHANCED_LAYOUTS,
		GL_MAX_XFB,
		GL_MAX_XFB_INT_COMP,

		/* */
		CONSTANTS_MAX,
	};

	/* Private types */
	struct testCase
	{
		CONSTANTS			  m_constant;
		Utils::Shader::STAGES m_stage;
	};

	/* Private methods */
	const glw::GLchar* getConstantName(CONSTANTS constant);

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test GLSLConstantIntegralExpression. Description follows:
 *
 * Check that following symbols can be used as integral constant expressions:
 *
 *     GL_ARB_enhanced_layouts,
 *     gl_MaxTransformFeedbackBuffers,
 *     gl_MaxTransformFeedbackInterleavedComponents.
 *
 * Test implement Texture algorithm. Test following code snippet:
 *
 *     uniform uint goku [GL_ARB_enhanced_layouts / GOKU_DIV];
 *     uniform uint gohan[gl_MaxTransformFeedbackBuffers / GOHAN_DIV];
 *     uniform uint goten[gl_MaxTransformFeedbackInterleavedComponents /
 *                         GOTEN_DIV];
 *
 *     for (i = 0; i < goku.length; ++i)
 *         goku_sum += goku[i];
 *
 *     for (i = 0; i < gohan.length; ++i)
 *         gohan_sum += gohan[i];
 *
 *     for (i = 0; i < goten.length; ++i)
 *         goten_sum += goten[i];
 *
 *     if ( (expected_goku_sum  == goku_sum)  &&
 *          (expected_gohan_sum == gohan_sum) &&
 *          (expected_goten_sum == goten_sum) )
 *         result = 1;
 *     else
 *         result = 0;
 *
 * Select DIV values so as array lengths are below 16.
 **/
class GLSLConstantIntegralExpressionTest : public TextureTestBase
{
public:
	/* Public methods */
	GLSLConstantIntegralExpressionTest(deqp::Context& context);

	virtual ~GLSLConstantIntegralExpressionTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getVerificationSnippet(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
											   Utils::Shader::STAGES stage);

	virtual void prepareUniforms(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								 Utils::Program& program, Utils::Buffer& cs_buffer);

	virtual void prepareUniforms(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								 Utils::Program& program, Utils::Buffer& fs_buffer, Utils::Buffer& gs_buffer,
								 Utils::Buffer& tcs_buffer, Utils::Buffer& tes_buffer, Utils::Buffer& vs_buffer);

private:
	glw::GLint m_gohan_length;
	glw::GLint m_goten_length;
};

/** Implementation of test UniformBlockMemberOffsetAndAlign. Description follows:
 *
 * Test verifies that:
 *     - offset and align qualifiers are respected for uniform block members,
 *     - align qualifier is ignored if value is too small,
 *     - align qualifier accepts values bigger than size of base type,
 *     - manual and automatic offsets and alignments can be mixed.
 *
 * This test implement Texture algorithm. Test following code snippet:
 *
 *     const int basic_size = sizeof(basic_type_of(type));
 *     const int type_size  = sizeof(type);
 *     const int type_align = roundUpToPowerOf2(type_size);
 *
 *     layout (std140, offset = 0) uniform Block {
 *         layout(align = 8 * basic_size)  type at_first_offset;
 *         layout(offset = type_size, align = basic_size / 2)
 *                                         type at_second_offset;
 *         layout(align = 2 * type_align)  type at_third_offset;
 *         layout(offset = 3 * type_align + type_size)
 *                                         type at_fourth_offset;
 *                                         type at_fifth_offset;
 *                                         type at_sixth_offset[2];
 *         layout(align = 8 * basic_size)  type at_eight_offset;
 *     };
 *
 *     if ( (at_first_offset  == at_eight_offset   ) &&
 *          (at_second_offset == at_sixth_offset[1]) &&
 *          (at_third_offset  == at_sixth_offset[0]) &&
 *          (at_fourth_offset == at_fifth_offset   ) )
 *         result = 1;
 *     else
 *         result = 0;
 *
 * Additionally inspect program to verify that all block members:
 * - are reported,
 * - have correct offsets.
 *
 * Test should be executed for all types.
 **/
class UniformBlockMemberOffsetAndAlignTest : public TextureTestBase
{
public:
	/* Public methods */
	UniformBlockMemberOffsetAndAlignTest(deqp::Context& context);

	virtual ~UniformBlockMemberOffsetAndAlignTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual std::string getVerificationSnippet(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
											   Utils::Shader::STAGES stage);

private:
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test UniformBlockLayoutQualifierConflict. Description follows:
 *
 * Test verifies that offset and align can only be used on members of blocks
 * declared as std140.
 *
 * Test following code snippet with all shader stages:
 *
 *     layout(QUALIFIER) uniform Block {
 *         layout(offset = 16) vec4 boy;
 *         layout(align  = 48) vec4 man;
 *     };
 *
 * Test following block qualifiers and all types:
 *
 *     default - meaning not declared by shader
 *     std140,
 *     shared,
 *     packed.
 *
 * Qualifier std430 is not allowed for uniform blocks.
 *
 * Test expect that only case using std140 will compile and link successfully,
 * while the rest will fail to compile or link.
 **/
class UniformBlockLayoutQualifierConflictTest : public NegativeTestBase
{
public:
	/* Public methods */
	UniformBlockLayoutQualifierConflictTest(deqp::Context& context);

	virtual ~UniformBlockLayoutQualifierConflictTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private enums */
	enum QUALIFIERS
	{
		DEFAULT,
		STD140,
		SHARED,
		PACKED,

		/* */
		QUALIFIERS_MAX,
	};

	/* Private types */
	struct testCase
	{
		QUALIFIERS			  m_qualifier;
		Utils::Shader::STAGES m_stage;
	};

	/* Private methods */
	const glw::GLchar* getQualifierName(QUALIFIERS qualifier);

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test UniformBlockMemberInvalidOffsetAlignment. Description follows:
 *
 * Test verifies that offset alignment rules are enforced.
 *
 * Declare uniform block, which contains following member declaration:
 *
 *     layout(offset = X) type block_member
 *
 * Shader compilation is expected to fail for any X that is not a multiple of
 * the base alignment of type.
 * Test all offsets covering locations first and one before last:
 *
 *     <0, sizeof(type)>
 *     <MAX_UNIFORM_BLOCK_SIZE - 2 * sizeof(type),
 *         MAX_UNIFORM_BLOCK_SIZE - sizeof(type)>
 *
 * Test all shader stages. Test all types. Each case should be tested
 * separately.
 **/
class UniformBlockMemberInvalidOffsetAlignmentTest : public NegativeTestBase
{
public:
	/* Public methods */
	UniformBlockMemberInvalidOffsetAlignmentTest(deqp::Context& context);

	UniformBlockMemberInvalidOffsetAlignmentTest(deqp::Context& context, const glw::GLchar* name,
												 const glw::GLchar* description);

	virtual ~UniformBlockMemberInvalidOffsetAlignmentTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual glw::GLint  getMaxBlockSize();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual bool isStageSupported(Utils::Shader::STAGES stage);
	virtual void testInit();

protected:
	/* Protected types */
	struct testCase
	{
		glw::GLuint			  m_offset;
		bool				  m_should_fail;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Protected fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test UniformBlockMemberOverlappingOffsets. Description follows:
 *
 * Test verifies that block members cannot overlap.
 *
 * Use following code snippet:
 *
 *     layout (std140) uniform Block {
 *         layout (offset = boy_offset) boy_type boy;
 *         layout (offset = man_offset) man_type man;
 *     };
 *
 * It is expected that overlapping members will cause compilation failure.
 *
 * There are three cases to test:
 *
 *     - when member is declared with the same offset as already declared
 *     member,
 *     - when member is declared with offset that lay in the middle of already
 *     declared member,
 *     - when member is declared with offset just before already declared
 *     member and there is not enough space.
 *
 * Test all shader stages. Test all types. Test cases separately.
 *
 * Note that not all type combinations let to test all three cases, e.g.
 * vec4 and float.
 **/
class UniformBlockMemberOverlappingOffsetsTest : public NegativeTestBase
{
public:
	/* Public methods */
	UniformBlockMemberOverlappingOffsetsTest(deqp::Context& context);

	UniformBlockMemberOverlappingOffsetsTest(deqp::Context& context, const glw::GLchar* name,
											 const glw::GLchar* description);

	virtual ~UniformBlockMemberOverlappingOffsetsTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isStageSupported(Utils::Shader::STAGES stage);
	virtual void testInit();

protected:
	/* Protected types */
	struct testCase
	{
		glw::GLuint			  m_boy_offset;
		Utils::Type			  m_boy_type;
		glw::GLuint			  m_man_offset;
		Utils::Type			  m_man_type;
		Utils::Shader::STAGES m_stage;
	};

	/* Protected methods */
	glw::GLuint gcd(glw::GLuint a, glw::GLuint b);
	glw::GLuint lcm(glw::GLuint a, glw::GLuint b);

	/* Protected fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test UniformBlockMemberAlignNonPowerOf2. Description follows:
 *
 * Test verifies that align qualifier must use power of 2.
 *
 * Test following code snippet:
 *
 *     layout (std140, offset = 8) uniform Block {
 *         vec4 boy;
 *         layout (align = man_alignment) type man;
 *     };
 *
 * It is expected that compilation will fail whenever man_alignment is not
 * a power of 2.
 *
 * Test all alignment in range <0, sizeof(dmat4)>. Test all shader stages.
 * Test all types.
 **/
class UniformBlockMemberAlignNonPowerOf2Test : public NegativeTestBase
{
public:
	/* Public methods */
	UniformBlockMemberAlignNonPowerOf2Test(deqp::Context& context);

	UniformBlockMemberAlignNonPowerOf2Test(deqp::Context& context, const glw::GLchar* name,
										   const glw::GLchar* description);

	virtual ~UniformBlockMemberAlignNonPowerOf2Test()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual bool isStageSupported(Utils::Shader::STAGES stage);
	virtual void testInit();

protected:
	/* Protected types */
	struct testCase
	{
		glw::GLuint			  m_alignment;
		Utils::Type			  m_type;
		bool				  m_should_fail;
		Utils::Shader::STAGES m_stage;
	};

	/* Protected methods */
	bool isPowerOf2(glw::GLuint val);

	/* Protected fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test UniformBlockAlignment. Description follows:
 *
 * UniformBlockAlignment
 *
 *   Test verifies that align qualifier is applied to block members as specified.
 *
 *   This test implements Texture algorithm. Test following code snippet:
 *
 *       struct Data {
 *           vec4  vector;
 *           float scalar;
 *       };
 *
 *       layout (std140, offset = 8, align = 64) uniform Block {
 *                               vec4 first;
 *                               Data second;
 *                               Data third[2];
 *                               vec4 fourth[3];
 *           layout (align = 16) vec4 fifth[2];
 *                               Data sixth;
 *       };
 *
 *   Verify that all uniforms have correct values. Additionally inspect program
 *   to check that all offsets are as expected.
 **/
class UniformBlockAlignmentTest : public TextureTestBase
{
public:
	/* Public methods */
	UniformBlockAlignmentTest(deqp::Context& context);

	virtual ~UniformBlockAlignmentTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

private:
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test SSBMemberOffsetAndAlign. Description follows:
 *
 * Test verifies that:
 *     - offset and align qualifiers are respected for shader storage block
 *     members,
 *     - align qualifier is ignored if value is too small,
 *     - align qualifier accepts values bigger than size of base type,
 *     - manual and automatic offsets and alignments can be mixed.
 *
 * Modify UniformBlockMemberOffsetAndAlign to test shader storage block instead
 * of uniform block.
 **/
class SSBMemberOffsetAndAlignTest : public TextureTestBase
{
public:
	/* Public methods */
	SSBMemberOffsetAndAlignTest(deqp::Context& context);
	virtual ~SSBMemberOffsetAndAlignTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual std::string getVerificationSnippet(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
											   Utils::Shader::STAGES stage);

	virtual bool isDrawRelevant(glw::GLuint test_case_index);

private:
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test SSBLayoutQualifierConflict. Description follows:
 *
 * Test verifies that offset and align can only be used on members of blocks
 * declared as std140 or std430.
 *
 * Modify UniformBlockMemberOffsetAndAlign to test shader storage block instead
 * of uniform block.
 *
 * Qualifier std430 is allowed for shader storage blocks, it should be included
 * in test. It is expected that std430 case will build successfully.
 **/
class SSBLayoutQualifierConflictTest : public NegativeTestBase
{
public:
	/* Public methods */
	SSBLayoutQualifierConflictTest(deqp::Context& context);

	virtual ~SSBLayoutQualifierConflictTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private enums */
	enum QUALIFIERS
	{
		DEFAULT,
		STD140,
		STD430,
		SHARED,
		PACKED,

		/* */
		QUALIFIERS_MAX,
	};

	/* Private types */
	struct testCase
	{
		QUALIFIERS			  m_qualifier;
		Utils::Shader::STAGES m_stage;
	};

	/* Private methods */
	const glw::GLchar* getQualifierName(QUALIFIERS qualifier);
	bool isStageSupported(Utils::Shader::STAGES stage);

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test SSBMemberInvalidOffsetAlignment. Description follows:
 *
 * Test verifies that offset alignment rules are enforced.
 *
 * Modify UniformBlockMemberInvalidOffsetAlignment to test shader
 * storage block against MAX_SHADER_STORAGE_BLOCK_SIZE instead of
 * uniform block
 **/
class SSBMemberInvalidOffsetAlignmentTest : public UniformBlockMemberInvalidOffsetAlignmentTest
{
public:
	/* Public methods */
	SSBMemberInvalidOffsetAlignmentTest(deqp::Context& context);

	virtual ~SSBMemberInvalidOffsetAlignmentTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual glw::GLint  getMaxBlockSize();
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual bool isStageSupported(Utils::Shader::STAGES stage);
};

/** Implementation of test SSBMemberOverlappingOffsets. Description follows:
 *
 * Test verifies that block members cannot overlap.
 *
 * Modify UniformBlockMemberOverlappingOffsets to test shader storage block
 * instead of uniform block.
 **/
class SSBMemberOverlappingOffsetsTest : public UniformBlockMemberOverlappingOffsetsTest
{
public:
	/* Public methods */
	SSBMemberOverlappingOffsetsTest(deqp::Context& context);
	virtual ~SSBMemberOverlappingOffsetsTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual bool isStageSupported(Utils::Shader::STAGES stage);
};

/** Implementation of test SSBMemberAlignNonPowerOf2. Description follows:
 *
 * Test verifies that align qualifier must use power of 2.
 *
 * Modify UniformBlockMemberAlignNonPowerOf2 to test shader storage block
 * instead of uniform block.
 **/
class SSBMemberAlignNonPowerOf2Test : public UniformBlockMemberAlignNonPowerOf2Test
{
public:
	/* Public methods */
	SSBMemberAlignNonPowerOf2Test(deqp::Context& context);

	virtual ~SSBMemberAlignNonPowerOf2Test()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual bool isStageSupported(Utils::Shader::STAGES stage);
};

/** Implementation of test SSBAlignment. Description follows:
 *
 * Test verifies that align qualifier is applied to block members as specified.
 *
 * Modify UniformBlockAlignment to test shader storage block instead
 * of uniform block.
 **/
class SSBAlignmentTest : public TextureTestBase
{
public:
	/* Public methods */
	SSBAlignmentTest(deqp::Context& context);

	virtual ~SSBAlignmentTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual bool isDrawRelevant(glw::GLuint test_case_index);

private:
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test VaryingLocations. Description follows:
 *
 * Test verifies that "varying" locations are assigned as declared in shader.
 *
 * This test implements Texture algorithm. Use separate shader objects instead
 * of monolithic program. Test following code snippet:
 *
 *     layout(location = 0)           in  type input_at_first_location;
 *     layout(location = last_input)  in  type input_at_last_location;
 *     layout(location = 1)           out type output_at_first_location;
 *     layout(location = last_output) out type output_at_last_location;
 *
 *     output_at_first_location = input_at_first_location;
 *     output_at_last_location  = input_at_last_location;
 *
 *     if ( (EXPECTED_VALUE == input_at_first_location) &&
 *          (EXPECTED_VALUE == input_at_last_location)  )
 *     {
 *         result = 1;
 *     }
 *
 * Additionally inspect program to check that all locations are as expected.
 *
 * Test all types. Test all shader stages.
 **/
class VaryingLocationsTest : public TextureTestBase
{
public:
	VaryingLocationsTest(deqp::Context& context);

	VaryingLocationsTest(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	~VaryingLocationsTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool useMonolithicProgram(glw::GLuint test_case_index);

	/* To be implemented by children */
	virtual void prepareShaderStage(Utils::Shader::STAGES stage, const Utils::Type& type,
									Utils::ProgramInterface&   program_interface,
									Utils::VaryingPassthrough& varying_passthrough);

	/* Protected methods */
	std::string prepareGlobals(glw::GLint last_in_loc, glw::GLint last_out_loc);

	/* Protected fields */
	std::vector<glw::GLubyte> m_first_data;
	std::vector<glw::GLubyte> m_last_data;
};

/** Implementation of test VertexAttribLocations. Description follows:
 *
 * Test verifies that drawing operations provide vertex attributes at expected
 * locations.
 *
 * This test implements Texture algorithm. Use separate shader objects instead
 * of monolithic program. Tessellation stages are not necessary and can be
 * omitted. Test following code snippet:
 *
 *     layout (location = 2) in uint vertex_index;
 *     layout (location = 5) in uint instance_index;
 *
 *     if ( (gl_VertexID   == vertex_index)   &&
 *          (gl_InstanceID == instance_index) )
 *     {
 *         result = 1;
 *     }
 *
 * Test following Draw* operations:
 *     - DrawArrays,
 *     - DrawArraysInstanced,
 *     - DrawElements,
 *     - DrawElementsBaseVertex,
 *     - DrawElementsInstanced,
 *     - DrawElementsInstancedBaseInstance,
 *     - DrawElementsInstancedBaseVertex,
 *     - DrawElementsInstancedBaseVertexBaseInstance.
 *
 * Number of drawn instances should be equal 4. base_vertex parameter should be
 * set to 4. base_instance should be set to 2.
 *
 * Values provided for "vertex_index" should match index of vertex. Values
 * provided for "instance_index" should match index of instance
 * (use VertexAttribDivisor).
 **/
class VertexAttribLocationsTest : public TextureTestBase
{
public:
	VertexAttribLocationsTest(deqp::Context& context);

	~VertexAttribLocationsTest()
	{
	}

protected:
	/* Protected methods */
	virtual void executeDrawCall(glw::GLuint test_case_index);

	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual std::string getVerificationSnippet(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
											   Utils::Shader::STAGES stage);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);

	virtual void prepareAttributes(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
								   Utils::Buffer& buffer, Utils::VertexArray& vao);

	virtual bool useMonolithicProgram(glw::GLuint test_case_index);

private:
	/* Private enums */
	enum TESTCASES
	{
		DRAWARRAYS,
		DRAWARRAYSINSTANCED,
		DRAWELEMENTS,
		DRAWELEMENTSBASEVERTEX,
		DRAWELEMENTSINSTANCED,
		DRAWELEMENTSINSTANCEDBASEINSTANCE,
		DRAWELEMENTSINSTANCEDBASEVERTEX,
		DRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE,

		/* */
		TESTCASES_MAX
	};

	/* Private constants */
	static const glw::GLuint m_base_vertex;
	static const glw::GLuint m_base_instance;
	static const glw::GLuint m_loc_vertex;
	static const glw::GLuint m_loc_instance;
	static const glw::GLuint m_n_instances;
};

/** Implementation of test VaryingArrayLocations. Description follows:
 *
 * VaryingArrayLocations
 *
 *   Test verifies that locations of arrays of "varying" are assigned as declared
 *   in shader.
 *
 *   This test implements Texture algorithm. Use separate shader objects instead
 *   of monolithic program. Test following code snippet:
 *
 *       layout(location = 0)           in  type in_at_first_loc[FIRST_LENGTH];
 *       layout(location = last_input)  in  type in_at_last_loc[LAST_LENGTH];
 *       layout(location = 1)           out type out_at_first_loc[FIRST_LENGTH];
 *       layout(location = last_output) out type out_at_last_loc[LAST_LENGTH];
 *
 *       for (uint i = 0u; i < in_at_first_loc.length(); ++i)
 *       {
 *           out_at_first_loc[i] = in_at_first_loc[i];
 *
 *           if (EXPECTED_VALUE[i] != in_at_first_loc[i])
 *           {
 *               result = 0;
 *           }
 *       }
 *
 *       for (uint i = 0u; i < in_at_last_loc.length(); ++i)
 *       {
 *           out_at_last_loc[i] = in_at_last_loc[i];
 *
 *           if (EXPECTED_VALUE[i] != in_at_last_loc[i])
 *           {
 *               result = 0;
 *           }
 *       }
 *
 *   FIRST_LENGTH and LAST_LENGTH values should be selected so as not to exceed
 *   limits.
 *
 *   Additionally inspect program to check that all locations are as expected.
 *
 *   Test all types. Test all shader stages.
 **/
class VaryingArrayLocationsTest : public VaryingLocationsTest
{
public:
	VaryingArrayLocationsTest(deqp::Context& context);

	~VaryingArrayLocationsTest()
	{
	}

protected:
	/* Protected methods */
	virtual void prepareShaderStage(Utils::Shader::STAGES stage, const Utils::Type& type,
									Utils::ProgramInterface&   program_interface,
									Utils::VaryingPassthrough& varying_passthrough);
};

/** Implementation of test VaryingStructureLocations. Description follows:
 *
 * Test verifies that structures locations are as expected.
 *
 * This test implements Texture algorithm. Use separate shader objects instead
 * of monolithic program. Test following code snippet:
 *
 *     struct Data
 *     {
 *         type single;
 *         type array[ARRAY_LENGTH];
 *     };
 *
 *     layout (location = INPUT_LOCATION)  in  Data input[VARYING_LENGTH];
 *     layout (location = OUTPUT_LOCATION) out Data output[VARYING_LENGTH];
 *
 *     if ( (EXPECTED_VALUE == input[0].single)         &&
 *          (EXPECTED_VALUE == input[0].array[0])       &&
 *          (EXPECTED_VALUE == input[0].array[last])    &&
 *          (EXPECTED_VALUE == input[last].single)      &&
 *          (EXPECTED_VALUE == input[last].array[0])    &&
 *          (EXPECTED_VALUE == input[last].array[last]) )
 *     {
 *         result = 1;
 *     }
 *
 *     output[0].single         = input[0].single;
 *     output[0].array[0]       = input[0].array[0];
 *     output[0].array[last]    = input[0].array[last];
 *     output[last].single      = input[last].single;
 *     output[last].array[0]    = input[last].array[0];
 *     output[last].array[last] = input[last].array[last];
 *
 * Select array lengths and locations so as no limits are exceeded.
 **/

class VaryingStructureLocationsTest : public TextureTestBase
{
public:
	VaryingStructureLocationsTest(deqp::Context& context);

	~VaryingStructureLocationsTest()
	{
	}

protected:
	/* Protected methods */
	virtual std::string getPassSnippet(glw::GLuint test_case_index, Utils::VaryingPassthrough& varying_passthrough,
									   Utils::Shader::STAGES stage);

	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool useMonolithicProgram(glw::GLuint test_case_index);

	/* Protected fields */
	std::vector<glw::GLubyte> m_single_data;
	std::vector<glw::GLubyte> m_array_data;
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test VaryingStructureMemberLocation. Description follows:
 *
 * Test verifies that it is not allowed to declare structure member at specific
 * location.
 *
 * Test following code snippet:
 *
 *     struct Data
 *     {
 *         vec4 gohan;
 *         layout (location = LOCATION) vec4 goten;
 *     }
 *
 *     in Data goku;
 *
 * Select LOCATION so as not to exceed limits. Test all shader stages. Test
 * both in and out varyings.
 *
 * It is expected that compilation will fail.
 **/
class VaryingStructureMemberLocationTest : public NegativeTestBase
{
public:
	VaryingStructureMemberLocationTest(deqp::Context& context);

	~VaryingStructureMemberLocationTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingBlockLocations. Description follows:
 *
 * Test verifies that "block varyings" locations are as expected.
 *
 * This test implements Texture algorithm. Use separate shader objects instead
 * of monolithic program. Test following code snippet:
 *
 *     layout (location = GOKU_LOCATION) in Goku
 *     {
 *                                            vec4 gohan;
 *         layout (location = GOTEN_LOCATION) vec4 goten;
 *                                            vec4 chichi;
 *     };
 *
 *     layout (location = VEGETA_LOCATION) out Vegeta
 *     {
 *                                          vec4 trunks;
 *         layout (location = BRA_LOCATION) vec4 bra;
 *                                          vec4 bulma;
 *     };
 *
 *     if ( (EXPECTED_VALUE == gohan) &&
 *          (EXPECTED_VALUE == goten) &&
 *          (EXPECTED_VALUE == chichi) )
 *     {
 *         result = 1;
 *     }
 *
 *     trunks = gohan;
 *     bra    = goten;
 *     bulma  = chichi;
 *
 * Select all locations so as not to cause any conflicts or exceed limits.
 **/
class VaryingBlockLocationsTest : public TextureTestBase
{
public:
	VaryingBlockLocationsTest(deqp::Context& context);
	~VaryingBlockLocationsTest()
	{
	}

protected:
	/* Protected methods */
	virtual std::string getPassSnippet(glw::GLuint test_case_index, Utils::VaryingPassthrough& varying_passthrough,
									   Utils::Shader::STAGES stage);

	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool useMonolithicProgram(glw::GLuint test_case_index);

	/* Protected fields */
	std::vector<glw::GLubyte> m_third_data;
	std::vector<glw::GLubyte> m_fourth_data;
	std::vector<glw::GLubyte> m_fifth_data;
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test VaryingBlockMemberLocations. Description follows:
 *
 * Test verifies that it is a compilation error to declare some of block
 * members with location qualifier, but not all, when there is no "block level"
 * location qualifier.
 *
 * Test following code snippets:
 *
 *     in Goku
 *     {
 *         vec4 gohan;
 *         layout (location = GOTEN_LOCATION) vec4 goten;
 *         vec4 chichi;
 *     };
 *
 * ,
 *
 *     in Goku
 *     {
 *         layout (location = GOHAN_LOCATION)  vec4 gohan;
 *         layout (location = GOTEN_LOCATION)  vec4 goten;
 *         layout (location = CHICHI_LOCATION) vec4 chichi;
 *     };
 *
 * Select all locations so as not to exceed any limits.
 *
 * It is expected that compilation of first snippet will fail. Compilation of
 * second snippet should be successful.
 *
 * Test all shader stages. Test both in and out blocks.
 **/
class VaryingBlockMemberLocationsTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingBlockMemberLocationsTest(deqp::Context& context);

	virtual ~VaryingBlockMemberLocationsTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		bool				  m_is_input;
		bool				  m_qualify_all;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingBlockAutomaticMemberLocations. Description follows:
 *
 * Test verifies that compiler will assign subsequent locations to block
 * members.
 *
 * Test following code snippet:
 *
 *     layout (location = 2) in DBZ
 *     {
 *         vec4 goku;                         // 2
 *         vec4 gohan[GOHAN_LENGTH];          // 3
 *         vec4 goten;                        // 3 + GOHAN_LENGTH
 *         layout (location = 1) vec4 chichi; // 1
 *         vec4 pan;                          // 2; ERROR location 2 used twice
 *     };
 *
 * Test all shader stages. Test both in and out blocks.
 *
 * It is expected that build process will fail.
 **/
class VaryingBlockAutomaticMemberLocationsTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingBlockAutomaticMemberLocationsTest(deqp::Context& context);

	virtual ~VaryingBlockAutomaticMemberLocationsTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingLocationLimit. Description follows:
 *
 * Test verifies that "location" qualifier cannot exceed limits.
 *
 * Test following code snippet:
 *
 *     layout (location = LAST + 1) in type goku;
 *
 * LAST should be set to index of last available location.
 *
 * Test all types. Test all shader stages. Test both in and out varyings.
 *
 * It is expected that shader compilation will fail.
 **/
class VaryingLocationLimitTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingLocationLimitTest(deqp::Context& context);

	virtual ~VaryingLocationLimitTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		bool				  m_is_input;
		Utils::Type			  m_type;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingComponents. Description follows:
 *
 * VaryingComponents
 *
 *   Test verifies that "varying" can be assigned to specific components.
 *
 *   Modify VaryingLocations to test all possible combinations of components
 *   layout:
 *       - gvec4
 *       - scalar, gvec3
 *       - gvec3, scalar
 *       - gvec2, gvec2
 *       - gvec2, scalar, scalar
 *       - scalar, gvec2, scalar
 *       - scalar, scalar, gvec2
 *       - scalar, scalar, scalar, scalar.
 *
 *   Additionally inspect program to check that all locations and components are
 *   as expected.
 **/
class VaryingComponentsTest : public VaryingLocationsTest
{
public:
	VaryingComponentsTest(deqp::Context& context);

	VaryingComponentsTest(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	~VaryingComponentsTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual void		testInit();
	virtual bool useComponentQualifier(glw::GLuint test_case_index);

	/* To be implemented by children */
	virtual glw::GLuint getArrayLength();

private:
	/* Private enums */
	enum COMPONENTS_LAYOUT
	{
		GVEC4,
		SCALAR_GVEC3,
		GVEC3_SCALAR,
		GVEC2_GVEC2,
		GVEC2_SCALAR_SCALAR,
		SCALAR_GVEC2_SCALAR,
		SCALAR_SCALAR_GVEC2,
		SCALAR_SCALAR_SCALAR_SCALAR,
	};

	/* Private struct */
	struct descriptor
	{
		void assign(glw::GLint component, const glw::GLchar* component_str, glw::GLint location,
					const glw::GLchar* location_str, glw::GLuint n_rows, const glw::GLchar* name);

		glw::GLint		   m_component;
		const glw::GLchar* m_component_str;
		glw::GLint		   m_location;
		const glw::GLchar* m_location_str;
		glw::GLuint		   m_n_rows;
		const glw::GLchar* m_name;
	};

	struct testCase
	{
		testCase(COMPONENTS_LAYOUT layout, Utils::Type::TYPES type);

		COMPONENTS_LAYOUT  m_layout;
		Utils::Type::TYPES m_type;
	};

	/* Private routines */
	std::string prepareGlobals(glw::GLuint last_in_location, glw::GLuint last_out_location);

	std::string prepareName(const glw::GLchar* name, glw::GLint location, glw::GLint component,
							Utils::Shader::STAGES stage, Utils::Variable::STORAGE storage);

	std::string prepareQualifiers(const glw::GLchar* location, const glw::GLchar* component,
								  const glw::GLchar* interpolation);

	using VaryingLocationsTest::prepareShaderStage;

	void prepareShaderStage(Utils::Shader::STAGES stage, const Utils::Type& vector_type,
							Utils::ProgramInterface& program_interface, const testCase& test_case,
							Utils::VaryingPassthrough& varying_passthrough);

	Utils::Variable* prepareVarying(const Utils::Type& basic_type, const descriptor& desc,
									const glw::GLchar* interpolation, Utils::ShaderInterface& si,
									Utils::Shader::STAGES stage, Utils::Variable::STORAGE storage);

	/* Private fields */
	std::vector<testCase>	 m_test_cases;
	std::vector<glw::GLubyte> m_data;
};

/** Implementation of test VaryingArrayComponents. Description follows:
 *
 * Test verifies that arrays of "varyings" can be assigned to specific
 * components.
 *
 * Modify VaryingComponents similarly to VaryingArrayLocations.
 **/
class VaryingArrayComponentsTest : public VaryingComponentsTest
{
public:
	VaryingArrayComponentsTest(deqp::Context& context);

	~VaryingArrayComponentsTest()
	{
	}

protected:
	/* Protected methods */
	virtual glw::GLuint getArrayLength();
};

/** Implementation of test VaryingExceedingComponents. Description follows:
 *
 * Test verifies that it is not allowed to exceed components.
 *
 * Test following code snippets:
 *
 *     layout (location = 1, component = COMPONENT) in type gohan;
 *
 * and
 *
 *     layout (location = 1, component = COMPONENT) in type gohan[LENGTH];
 *
 * Select COMPONENT so as to exceed space available at location, eg. 2 for
 * vec4. Select array length so as to not exceed limits of available locations.
 *
 * Test all types. Test all shader stages. Test both in and out varyings.
 *
 * It is expected that build process will fail.
 **/
class VaryingExceedingComponentsTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingExceedingComponentsTest(deqp::Context& context);

	virtual ~VaryingExceedingComponentsTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component;
		bool				  m_is_input;
		bool				  m_is_array;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingComponentWithoutLocation. Description follows:
 *
 * Test verifies that "component" qualifier cannot be used without "location"
 * qualifier.
 *
 * Test following code snippet:
 *
 *     layout (component = COMPONENT) in type goku;
 *
 * Test all types. Test all valid COMPONENT values. Test all shader stages.
 * Test both in and out varyings.
 *
 * It is expected that shader compilation will fail.
 **/
class VaryingComponentWithoutLocationTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingComponentWithoutLocationTest(deqp::Context& context);

	virtual ~VaryingComponentWithoutLocationTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component;
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingComponentOfInvalidType. Description follows:
 *
 * Test verifies that it is not allowed to declare matrix, struct, block and
 * array of those at specific component.
 *
 * Test following code snippets:
 *
 *     layout (location = 0, component = COMPONENT) in matrix_type varying;
 *
 * ,
 *
 *     layout (location = 0, component = COMPONENT)
 *         in matrix_type varying[LENGTH];
 *
 * ,
 *
 *     layout (location = 0, component = COMPONENT) in Block
 *     {
 *         type member;
 *     };
 *
 * ,
 *
 *     layout (location = 0, component = COMPONENT) in Block
 *     {
 *         type member;
 *     } block[LENGTH];
 *
 * ,
 *
 *     struct Data
 *     {
 *         type member;
 *     };
 *
 *     layout (location = 0, component = COMPONENT) in Data varying;
 *
 * and
 *
 *     struct Data
 *     {
 *         type member;
 *     };
 *
 *     layout (location = 0, component = COMPONENT) in Data varying[LENGTH];
 *
 * Test all types. Test all shader stages. Test both in and out varyings.
 *
 * It is expected that build process will fail.
 **/
class VaryingComponentOfInvalidTypeTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingComponentOfInvalidTypeTest(deqp::Context& context);

	virtual ~VaryingComponentOfInvalidTypeTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private enums */
	enum CASES
	{
		MATRIX = 0,
		BLOCK,
		STRUCT,

		/* */
		MAX_CASES
	};

	/* Private types */
	struct testCase
	{
		CASES				  m_case;
		glw::GLuint			  m_component;
		bool				  m_is_array;
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test InputComponentAliasing. Description follows:
 *
 * Test verifies that component aliasing cause compilation or linking error.
 *
 * Test following code snippet:
 *
 *     layout (location = 1, component = GOHAN_COMPONENT) in type gohan;
 *     layout (location = 1, component = GOTEN_COMPONENT) in type goten;
 *
 *     if (EXPECTED_VALUE == gohan)
 *     {
 *         result = 1;
 *     }
 *
 * Test all components combinations that cause aliasing. Test all types. Test
 * all shader stages. It is expected that build process will fail.
 *
 * Vertex shader allows component aliasing on input as long as only one of the
 * attributes is used in each execution path. Test vertex shader stage with two
 * variants:
 *     - first as presented above,
 *     - second, where "result = 1;" is replaced with "result = goten;".
 * In first case build process should succeed, in the second case build process
 * should fail.
 **/
class InputComponentAliasingTest : public NegativeTestBase
{
public:
	/* Public methods */
	InputComponentAliasingTest(deqp::Context& context);

	virtual ~InputComponentAliasingTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component_gohan;
		glw::GLuint			  m_component_goten;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test OutputComponentAliasing. Description follows:
 *
 * Test verifies that component aliasing cause compilation or linking error.
 *
 * Test following code snippet:
 *
 *     layout (location = 1, component = GOHAN_COMPONENT) out type gohan;
 *     layout (location = 1, component = GOTEN_COMPONENT) out type goten;
 *
 *     gohan = GOHAN_VALUE;
 *     goten = GOTEN_VALUE;
 *
 * Test all components combinations that cause aliasing. Test all types. Test
 * all shader stages. It is expected that build process will fail.
 **/
class OutputComponentAliasingTest : public NegativeTestBase
{
public:
	/* Public methods */
	OutputComponentAliasingTest(deqp::Context& context);

	virtual ~OutputComponentAliasingTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component_gohan;
		glw::GLuint			  m_component_goten;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingLocationAliasingWithMixedTypes. Description follows:
 *
 * Test verifies that it is not allowed to mix integer and float base types at
 * aliased location.
 *
 * Test following code snippet:
 *
 *     layout (location = 1, component = GOHAN_COMPONENT) in gohan_type gohan;
 *     layout (location = 1, component = GOTEN_COMPONENT) in goten_type goten;
 *
 * Test all components combinations that do not cause component aliasing. Test
 * all types combinations that cause float/integer conflict. Test all shader
 * stages. Test both in and out varyings.
 *
 * It is expected that build process will fail.
 **/
class VaryingLocationAliasingWithMixedTypesTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingLocationAliasingWithMixedTypesTest(deqp::Context& context);

	virtual ~VaryingLocationAliasingWithMixedTypesTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component_gohan;
		glw::GLuint			  m_component_goten;
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type_gohan;
		Utils::Type			  m_type_goten;
	};

	/* Private routines */
	bool isFloatType(const Utils::Type& type);

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingLocationAliasingWithMixedInterpolation. Description follows:
 *
 * Test verifies that it is not allowed to mix interpolation methods at aliased
 * location.
 *
 * Test following code snippet:
 *
 *     layout (location = 1, component = GOHAN_COMPONENT)
 *         GOHAN_INTERPOLATION in type gohan;
 *     layout (location = 1, component = GOTEN_COMPONENT)
 *         GOTEN_INTERPOLATION in type goten;
 *
 * Test all interpolation combinations that cause conflict. Select components
 * so as not to cause component aliasing. Test all types. Test all shader
 * stages. Test both in and out varyings.
 *
 * Note, that vertex shader's input and fragment shader's output cannot be
 * qualified with interpolation method.
 *
 * It is expected that build process will fail.
 **/
class VaryingLocationAliasingWithMixedInterpolationTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingLocationAliasingWithMixedInterpolationTest(deqp::Context& context);

	virtual ~VaryingLocationAliasingWithMixedInterpolationTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum INTERPOLATIONS
	{
		SMOOTH = 0,
		FLAT,
		NO_PERSPECTIVE,

		/* */
		INTERPOLATION_MAX
	};

	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component_gohan;
		glw::GLuint			  m_component_goten;
		INTERPOLATIONS		  m_interpolation_gohan;
		INTERPOLATIONS		  m_interpolation_goten;
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type_gohan;
		Utils::Type			  m_type_goten;
	};

	/* Private routines */
	const glw::GLchar* getInterpolationQualifier(INTERPOLATIONS interpolation);
	bool isFloatType(const Utils::Type& type);

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VaryingLocationAliasingWithMixedAuxiliaryStorage. Description follows:
 *
 * Test verifies that it is not allowed to mix auxiliary storage at aliased
 * location.
 *
 * Test following code snippet:
 *
 * layout (location = 1, component = GOHAN_COMPONENT)
 *     GOHAN_AUXILIARY in type gohan;
 * layout (location = 1, component = GOTEN_COMPONENT)
 *     GOTEN_AUXILIARY in type goten;
 *
 * Test all auxiliary storage combinations that cause conflict. Select
 * components so as not to cause component aliasing. Test all types. Test all
 * shader stages. Test both in and out varyings.
 *
 * It is expected that build process will fail.
 **/
class VaryingLocationAliasingWithMixedAuxiliaryStorageTest : public NegativeTestBase
{
public:
	/* Public methods */
	VaryingLocationAliasingWithMixedAuxiliaryStorageTest(deqp::Context& context);

	virtual ~VaryingLocationAliasingWithMixedAuxiliaryStorageTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum AUXILIARIES
	{
		NONE = 0,
		PATCH,
		CENTROID,
		SAMPLE,

		/* */
		AUXILIARY_MAX
	};

	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_component_gohan;
		glw::GLuint			  m_component_goten;
		AUXILIARIES			  m_aux_gohan;
		AUXILIARIES			  m_aux_goten;
		const glw::GLchar*	m_int_gohan;
		const glw::GLchar*	m_int_goten;
		bool				  m_is_input;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type_gohan;
		Utils::Type			  m_type_goten;
	};

	/* Private routines */
	const glw::GLchar* getAuxiliaryQualifier(AUXILIARIES aux);
	bool isFloatType(const Utils::Type& type);

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test VertexAttribLocationAPI. Description follows:
 *
 * Test verifies that vertex attribute location API works as expected.
 *
 * This test implements Texture algorithm. Tessellation shaders are not
 * necessary and can be omitted. Test following code snippet in vertex shader:
 *
 *     layout (location = GOKU_LOCATION) in vec4 goku;
 *                                       in vec4 gohan;
 *                                       in vec4 goten;
 *                                       in vec4 chichi;
 *
 *     if ( (EXPECTED_VALUE == goku)   &&
 *          (EXPECTED_VALUE == gohan)  &&
 *          (EXPECTED_VALUE == gotan)  &&
 *          (EXPECTED_VALUE == chichi) )
 *     {
 *         result = 1;
 *     }
 *
 * After compilation, before program is linked, specify locations for goku,
 * and goten with glBindAttribLocation. Specify different location than the one
 * used in shader.
 *
 * Select all locations so as not to exceed any limits.
 *
 * Additionally inspect program to verify that:
 *     - goku location is as specified in shader text,
 *     - goten location is as specified with API.
 **/
class VertexAttribLocationAPITest : public TextureTestBase
{
public:
	VertexAttribLocationAPITest(deqp::Context& context);

	~VertexAttribLocationAPITest()
	{
	}

protected:
	/* Protected methods */
	virtual void prepareAttribLocation(Utils::Program& program, Utils::ProgramInterface& program_interface);

	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);

private:
	/* Private fields */
	std::vector<glw::GLubyte> m_goku_data;
	std::vector<glw::GLubyte> m_gohan_data;
	std::vector<glw::GLubyte> m_goten_data;
	std::vector<glw::GLubyte> m_chichi_data;

	/* Private constants */
	static const glw::GLuint m_goten_location;
};
/** Implementation of test FragmentDataLocationAPI. Description follows:
 *
 * Test verifies that fragment data location API works as expected.
 *
 * This test implements Texture algorithm. Tessellation shaders are not
 * necessary and can be omitted. "result" is not necessary and can be omitted.
 * Test following code snippet in fragment shader:
 *
 *     layout (location = GOKU_LOCATION)  out vec4 goku;
 *                                        out vec4 gohan;
 *                                        out vec4 goten;
 *                                        out vec4 chichi;
 *
 *     goku   = EXPECTED_VALUE;
 *     gohan  = EXPECTED_VALUE;
 *     goten  = EXPECTED_VALUE;
 *     chichi = EXPECTED_VALUE;
 *
 * After compilation, before program is linked, specify locations for goku,
 * and goten with glBindFragDataLocation. Specify different location than the
 * one used in shader.
 *
 * Select all locations so as not to exceed any limits.
 *
 * Additionally inspect program to verify that:
 *     - goku location is as specified in shader text,
 *     - goten location is as specified with API.
 **/
class FragmentDataLocationAPITest : public TextureTestBase
{
public:
	FragmentDataLocationAPITest(deqp::Context& context);

	~FragmentDataLocationAPITest()
	{
	}

protected:
	/* Protected methods */
	virtual bool checkResults(glw::GLuint test_case_index, Utils::Texture& color_0);

	virtual std::string getPassSnippet(glw::GLuint test_case_index, Utils::VaryingPassthrough& varying_passthrough,
									   Utils::Shader::STAGES stage);

	virtual void getProgramInterface(glw::GLuint test_case_index, Utils::ProgramInterface& program_interface,
									 Utils::VaryingPassthrough& varying_passthrough);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);

	virtual void prepareFragmentDataLoc(Utils::Program& program, Utils::ProgramInterface& program_interface);

	virtual void prepareFramebuffer(Utils::Framebuffer& framebuffer, Utils::Texture& color_0_texture);

private:
	/* Private fields */
	Utils::Texture m_goku;
	Utils::Texture m_gohan;
	Utils::Texture m_goten;
	Utils::Texture m_chichi;

	glw::GLint m_goku_location;
	glw::GLint m_gohan_location;
	glw::GLint m_chichi_location;

	/* Private constants */
	static const glw::GLuint m_goten_location;
};

/** Implementation of test XFBInput. Description follows:
 *
 * Test verifies that using xfb_buffer, xfb_stride or xfb_offset qualifiers on
 * input variables will cause failure of build process.
 *
 * Test all shader stages.
 **/
class XFBInputTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBInputTest(deqp::Context& context);
	virtual ~XFBInputTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum QUALIFIERS
	{
		BUFFER = 0,
		STRIDE,
		OFFSET,

		/* */
		QUALIFIERS_MAX
	};

	/* Private types */
	struct testCase
	{
		QUALIFIERS			  m_qualifier;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBAllStages. Description follows:
 *
 * Test verifies that only outputs from last stage processing primitives can be
 * captured with XFB.
 *
 * This test implements Buffer algorithm. Rasterization can be discarded.
 *
 * At each stage declare a single active output variable qualified with
 * xfb_buffer and xfb_offset = 0. Use separate buffers for each stage.
 *
 * Test pass if outputs from geometry shader are captured, while outputs from:
 * vertex and tessellation stages are ignored.
 **/
class XFBAllStagesTest : public BufferTestBase
{
public:
	XFBAllStagesTest(deqp::Context& context);

	~XFBAllStagesTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

private:
	/* Constants */
	static const glw::GLuint m_gs_index;
};

/** Implementation of test XFBStrideOfEmptyList. Description follows:
 *
 * Test verifies correct behavior when xfb_stride qualifier is specified
 * but no variables are qualified with xfb_offset.
 *
 * Test implements Buffer algorithm. Rasterization can be discarded.
 *
 * Test following code snippet:
 *
 *     layout (xfb_buffer = 1, xfb_stride = 64) out;
 *
 * Moreover program should output something to xfb at index 0
 *
 * Test following cases:
 *     1 Provide buffers to XFB at index 0 and 1
 *     2 Provide buffer to XFB at index 1, index 0 should be missing
 *     3 Provide buffer to XFB at index 0, index 1 should be missing
 *
 * It is expected that:
 *     - BeginTransformFeedback operation will report GL_INVALID_OPERATION in case 2
 *     - XFB at index 1 will not be modified in cases 1 and 3.
 **/

class XFBStrideOfEmptyListTest : public BufferTestBase
{
public:
	XFBStrideOfEmptyListTest(deqp::Context& context);

	~XFBStrideOfEmptyListTest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;
	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);

	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

private:
	enum CASES
	{
		VALID = 0,
		FIRST_MISSING,
		SECOND_MISSING,
	};

	/* Private constants */
	static const glw::GLuint m_stride;
};

/** Implementation of test XFBStrideOfEmptyListAndAPI. Description follows:
 *
 * Test verifies that xfb_stride qualifier is not overridden by API.
 *
 * Test implements Buffer algorithm. Rasterization can be discarded.
 *
 * Test following code snippet:
 *
 *     layout (xfb_buffer = 0, xfb_stride = 64) out;
 *
 * Moreover program should output something to xfb at index 1

 * Use TransformFeedbackVaryings to declare a single vec4 output variable to be
 * captured.
 *
 * Test following cases:
 *     1 Provide buffers to XFB at index 0 and 1
 *     2 Provide buffer to XFB at index 1, index 0 should be missing
 *     3 Provide buffer to XFB at index 0, index 1 should be missing
 *
 * It is expected that:
 *     - BeginTransformFeedback operation will report GL_INVALID_OPERATION in cases
 *     2 and 3,
 *     - XFB at index 0 will not be modified in case 1.
 **/
class XFBStrideOfEmptyListAndAPITest : public BufferTestBase
{
public:
	XFBStrideOfEmptyListAndAPITest(deqp::Context& context);

	~XFBStrideOfEmptyListAndAPITest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;
	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getCapturedVaryings(glw::GLuint test_case_index, Utils::Program::NameVector& captured_varyings, glw::GLint* xfb_components);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

private:
	enum CASES
	{
		VALID = 0,
		FIRST_MISSING,
		SECOND_MISSING,
	};

	/* Private constants */
	static const glw::GLuint m_stride;
};

/** Implementation of test XFBTooSmallStride. Description follows:
 *
 * Test verifies that build process fails when xfb_stride qualifier sets not
 * enough space for all variables.
 *
 * Test following code snippets:
 *
 *     layout (xfb_buffer = 0, xfb_stride = 40) out;
 *
 *     layout (xfb_offset = 32) out vec4 goku;
 *
 *     goku = EXPECTED_VALUE.
 *
 * ,
 *
 *     layout (xfb_buffer = 0, xfb_stride = 32) out;
 *
 *     layout (xfb_offset = 16, xfb_stride = 32) out vec4 goku;
 *
 *     goku = EXPECTED_VALUE.
 *
 * ,
 *
 *     layout (xfb_buffer = 0, xfb_stride = 32) out;
 *
 *     layout (xfb_offset = 0) out Goku {
 *         vec4 gohan;
 *         vec4 goten;
 *         vec4 chichi;
 *     };
 *
 *     gohan  = EXPECTED_VALUE;
 *     goten  = EXPECTED_VALUE;
 *     chichi = EXPECTED_VALUE;
 *
 * ,
 *
 *     layout (xfb_buffer = 0, xfb_stride = 32) out;
 *
 *     layout (xfb_offset = 16) out vec4 goku[4];
 *
 *     goku[0] = EXPECTED_VALUE.
 *     goku[1] = EXPECTED_VALUE.
 *     goku[2] = EXPECTED_VALUE.
 *     goku[3] = EXPECTED_VALUE.
 *
 * It is expected that build process will fail.
 *
 * Test all shader stages.
 **/
class XFBTooSmallStrideTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBTooSmallStrideTest(deqp::Context& context);

	virtual ~XFBTooSmallStrideTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum CASES
	{
		OFFSET = 0,
		STRIDE,
		BLOCK,
		ARRAY,

		/* */
		CASE_MAX
	};

	/* Private types */
	struct testCase
	{
		CASES				  m_case;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBVariableStride. Description follows:
 *
 * Test verifies that xfb_stride qualifier change stride of output variable.
 *
 * Test following code snippets:
 *
 *     layout (xfb_offset = 0, xfb_stride = 2 * sizeof(type)) out type goku;
 *
 * and
 *
 *     layout (xfb_offset = 0, xfb_stride = 2 * sizeof(type)) out type goku;
 *     layout (xfb_offset = sizeof(type))                     out type vegeta;
 *
 * It is expected that:
 *     - first snippet will build successfully,
 *     - second snippet will fail to build.
 *
 * Test all types. Test all shader stages.
 **/
class XFBVariableStrideTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBVariableStrideTest(deqp::Context& context);
	virtual ~XFBVariableStrideTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum CASES
	{
		VALID = 0,
		INVALID,

		/* */
		CASE_MAX
	};

	/* Private types */
	struct testCase
	{
		CASES				  m_case;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBBlockStride. Description follows:
 *
 * Test verifies that xfb_stride qualifier change stride of output block.
 *
 * Test following code snippet:
 *
 *     layout (xfb_offset = 0, xfb_stride = 128) out Goku {
 *         vec4 gohan;
 *         vec4 goten;
 *         vec4 chichi;
 *     };
 *
 * Inspect program to check if Goku stride is 128 units.
 *
 * Test all shader stages.
 **/
class XFBBlockStrideTest : public TestBase
{
public:
	/* Public methods */
	XFBBlockStrideTest(deqp::Context& context);

	virtual ~XFBBlockStrideTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool testCase(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private methods */
	std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	bool inspectProgram(Utils::Program& program);

	/* Private fields */
	std::vector<Utils::Shader::STAGES> m_test_cases;
};

/** Implementation of test XFBBlockMemberStride. Description follows:
 *
 * Test verifies that xfb_stride qualifier change stride of output block
 * member.
 *
 * This test implements Buffer algorithm. Rasterization can be discarded. Test
 * following code snippet:
 *
 *     layout (xfb_offset = 0) out Goku {
 *                                  vec4 gohan;
 *         layout (xfb_stride = 32) vec4 goten;
 *                                  vec4 chichi;
 *     };
 *
 *     gohan  = EXPECTED_VALUE;
 *     goten  = EXPECTED_VALUE;
 *     chichi = EXPECTED_VALUE;
 *
 * Test pass if:
 *     - goten stride is reported as 32,
 *     - chichi offset is reported as 48,
 *     - values captured for all members match expected values,
 *     - part of memory reserved for goten, that is not used, is not modified.
 **/
class XFBBlockMemberStrideTest : public BufferTestBase
{
public:
	XFBBlockMemberStrideTest(deqp::Context& context);
	~XFBBlockMemberStrideTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual bool inspectProgram(glw::GLuint test_case_index, Utils::Program& program, std::stringstream& out_stream);
};

/** Implementation of test XFBDuplicatedStride. Description follows:
 *
 * Test verifies that conflicting xfb_stride qualifiers cause build process
 * failure.
 *
 * Test following code snippets:
 *
 *     layout (xfb_buffer = 0, xfb_stride = 64) out;
 *     layout (xfb_buffer = 0, xfb_stride = 64) out;
 *
 * and
 *
 *     layout (xfb_buffer = 0, xfb_stride = 64)  out;
 *     layout (xfb_buffer = 0, xfb_stride = 128) out;
 *
 * It is expected that:
 *     - first snippet will build successfully,
 *     - second snippet will fail to build.
 **/
class XFBDuplicatedStrideTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBDuplicatedStrideTest(deqp::Context& context);

	virtual ~XFBDuplicatedStrideTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual bool isFailureExpected(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum CASES
	{
		VALID = 0,
		INVALID,

		/* */
		CASE_MAX
	};

	/* Private types */
	struct testCase
	{
		CASES				  m_case;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBGetProgramResourceAPI. Description follows:
 *
 * Test verifies that GetProgramResource* API work as expected for transform
 * feedback.
 *
 * Test results of following queries:
 *     - OFFSET,
 *     - TRANSFORM_FEEDBACK_BUFFER_INDEX,
 *     - TRANSFORM_FEEDBACK_BUFFER_STRIDE,
 *     - TYPE.
 *
 * Test following cases:
 *     - captured varyings are declared with API, TransformFeedbackVaryings
 *     with INTERLEAVED_ATTRIBS,
 *     - captured varyings are declared with API, TransformFeedbackVaryings
 *     with SEPARATE_ATTRIBS,
 *     - captured varyings are declared with "xfb" qualifiers.
 *
 * Following layout should be used in cases of INTERLEAVED_ATTRIBS and "xfb"
 * qualifiers:
 *              | var 0 | var 1 | var 2 | var 3
 *     buffer 0 | used  | used  | empty | used
 *     buffer 1 | empty | used  | empty | empty
 *
 * In "xfb" qualifiers case, use following snippet:
 *
 *     layout (xfb_buffer = 1, xfb_stride = 4 * sizeof(type)) out;
 *
 * Declaration in shader should use following order:
 *     buffer_0_var_1
 *     buffer_1_var_1
 *     buffer_0_var_3
 *     buffer_0_var_0
 *
 * To make sure that captured varyings are active, they should be assigned.
 *
 * Test all types. Test all shader stages.
 **/
class XFBGetProgramResourceAPITest : public TestBase
{
public:
	/* Public methods */
	XFBGetProgramResourceAPITest(deqp::Context& context);

	virtual ~XFBGetProgramResourceAPITest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool testCase(glw::GLuint test_case_index);
	virtual void testInit();
	virtual void insertSkipComponents(int num_components, Utils::Program::NameVector& varyings);

private:
	/* Private enums */
	enum CASES
	{
		INTERLEAVED,
		SEPARATED,
		XFB,
	};

	/* Private types */
	struct test_Case
	{
		CASES				  m_case;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private methods */
	std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	bool inspectProgram(glw::GLuint test_case_index, Utils::Program& program);

	/* Private fields */
	std::vector<test_Case> m_test_cases;
};

/** Implementation of test XFBOverrideQualifiersWithAPI. Description follows:
 *
 * Test verifies that API is ignored, when qualifiers are in use.
 *
 * This test follows Buffer algorithm. Rasterization can disabled. Test
 * following code snippet:
 *
 *     layout (xfb_offset = 2 * sizeof(type)) out type vegeta;
 *                                            out type trunks;
 *     layout (xfb_offset = 0)                out type goku;
 *                                            out type gohan;
 *
 *     vegeta = EXPECTED_VALUE;
 *     trunks = EXPECTED_VALUE;
 *     goku   = EXPECTED_VALUE;
 *     gohan  = EXPECTED_VALUE;
 *
 * Use API, TransformFeedbackVaryings, to specify trunks and gohan as outputs
 * to be captured.
 *
 * Test pass if:
 *     - correct values are captured for vegeta and goku,
 *     - trunks and gohan are not captured,
 *     - correct stride is reported.
 *
 * Test all types. Test all shader stages.
 **/
class XFBOverrideQualifiersWithAPITest : public BufferTestBase
{
public:
	XFBOverrideQualifiersWithAPITest(deqp::Context& context);

	~XFBOverrideQualifiersWithAPITest()
	{
	}

protected:
	/* Protected methods */
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getCapturedVaryings(glw::GLuint test_case_index, Utils::Program::NameVector& captured_varyings, glw::GLint* xfb_components);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual bool inspectProgram(glw::GLuint test_case_index, Utils::Program& program, std::stringstream& out_stream);
};

/** Implementation of test XFBVertexStreams. Description follows:
 *
 * Test verifies that "xfb" qualifiers work as expected with multiple output
 * streams.
 *
 * Test implements Buffer algorithm. Rasterization can be discarded.
 *
 * Test following code snippet:
 *
 *     layout (xfb_buffer = 1, xfb_stride = 64) out;
 *     layout (xfb_buffer = 2, xfb_stride = 64) out;
 *     layout (xfb_buffer = 3, xfb_stride = 64) out;
 *
 *     layout (stream = 0, xfb_buffer = 1, xfb_offset = 48) out vec4 goku;
 *     layout (stream = 0, xfb_buffer = 1, xfb_offset = 32) out vec4 gohan;
 *     layout (stream = 0, xfb_buffer = 1, xfb_offset = 16) out vec4 goten;
 *     layout (stream = 1, xfb_buffer = 3, xfb_offset = 48) out vec4 picolo;
 *     layout (stream = 1, xfb_buffer = 3, xfb_offset = 32) out vec4 vegeta;
 *     layout (stream = 2, xfb_buffer = 2, xfb_offset = 32) out vec4 bulma;
 *
 *     goku   = EXPECTED_VALUE;
 *     gohan  = EXPECTED_VALUE;
 *     goten  = EXPECTED_VALUE;
 *     picolo = EXPECTED_VALUE;
 *     vegeta = EXPECTED_VALUE;
 *     bulma  = EXPECTED_VALUE;
 *
 * Test pass if all captured outputs have expected values.
 **/
class XFBVertexStreamsTest : public BufferTestBase
{
public:
	XFBVertexStreamsTest(deqp::Context& context);

	~XFBVertexStreamsTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);
};

/** Implementation of test XFBMultipleVertexStreams. Description follows:
 *
 * Test verifies that outputs from single stream must be captured with single
 * xfb binding.
 *
 * Test following code snippet:
 *
 *     layout (xfb_buffer = 1, xfb_stride = 64) out;
 *     layout (xfb_buffer = 3, xfb_stride = 64) out;
 *
 *     layout (stream = 0, xfb_buffer = 1, xfb_offset = 48) out vec4 goku;
 *     layout (stream = 1, xfb_buffer = 1, xfb_offset = 32) out vec4 gohan;
 *     layout (stream = 2, xfb_buffer = 1, xfb_offset = 16) out vec4 goten;
 *
 * It is expected that linking of program will fail.
 **/
class XFBMultipleVertexStreamsTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBMultipleVertexStreamsTest(deqp::Context& context);

	virtual ~XFBMultipleVertexStreamsTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual bool isComputeRelevant(glw::GLuint test_case_index);
};

/** Implementation of test XFBExceedBufferLimit. Description follows:
 *
 * Test verifies that MAX_TRANSFORM_FEEDBACK_BUFFERS limit is respected.
 *
 * Test following code snippets:
 *
 *     layout (xfb_buffer = MAX_TRANSFORM_FEEDBACK_BUFFERS) out;
 *
 * ,
 *
 *     layout (xfb_buffer = MAX_TRANSFORM_FEEDBACK_BUFFERS) out vec4 output;
 *
 * and
 *
 *     layout (xfb_buffer = MAX_TRANSFORM_FEEDBACK_BUFFERS) out Block {
 *         vec4 member;
 *     };
 *
 * It is expected that build process will fail.
 *
 * Test all shader stages.
 **/
class XFBExceedBufferLimitTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBExceedBufferLimitTest(deqp::Context& context);
	virtual ~XFBExceedBufferLimitTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum CASES
	{
		BLOCK = 0,
		GLOBAL,
		VECTOR,

		/* */
		CASE_MAX
	};

	/* Private types */
	struct testCase
	{
		CASES				  m_case;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBExceedOffsetLimit. Description follows:
 *
 * Test verifies that MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS limit is
 * respected.
 *
 * Test following code snippets:
 *
 *     layout (xfb_buffer = 0, xfb_stride = MAX_SIZE + 16) out;
 *
 * ,
 *
 *     layout (xfb_buffer = 0, xfb_offset = MAX_SIZE) out vec4 output;
 *
 * and
 *
 *     layout (xfb_buffer = 0, xfb_offset = MAX_SIZE) out Block
 *     {
 *         vec4 member;
 *     };
 *
 * where MAX_SIZE is the maximum supported size of transform feedback buffer,
 * which should be equal to:
 *
 *     MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS * sizeof(float)
 *
 * It is expected that build process will fail.
 *
 * Test all shader stages.
 **/
class XFBExceedOffsetLimitTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBExceedOffsetLimitTest(deqp::Context& context);
	virtual ~XFBExceedOffsetLimitTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	enum CASES
	{
		BLOCK = 0,
		GLOBAL,
		VECTOR,

		/* */
		CASE_MAX
	};

	/* Private types */
	struct testCase
	{
		CASES				  m_case;
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBGlobalBuffer. Description follows:
 *
 * Test verifies that global setting of xfb_buffer qualifier work as expected.
 *
 * This test implements Buffer algorithm. Rasterization can be discarded. Test
 * following code snippet:
 *
 *     layout (xfb_buffer = 3) out;
 *
 *     layout (                xfb_offset = 2 * sizeof(type)) out type chichi;
 *     layout (xfb_buffer = 1, xfb_offset = 0)                out type bulma;
 *     layout (xfb_buffer = 1, xfb_offset = sizeof(type))     out Vegeta {
 *         type trunks;
 *         type bra;
 *     };
 *     layout (                xfb_offset = 0)                out Goku {
 *         type gohan;
 *         type goten;
 *     };
 *
 *     chichi = EXPECTED_VALUE;
 *     bulma  = EXPECTED_VALUE;
 *     trunks = EXPECTED_VALUE;
 *     bra    = EXPECTED_VALUE;
 *     gohan  = EXPECTED_VALUE;
 *     goten  = EXPECTED_VALUE;
 *
 * Test pass if all captured outputs have expected values.
 *
 * Test all shader stages. Test all types.
 **/
class XFBGlobalBufferTest : public BufferTestBase
{
public:
	XFBGlobalBufferTest(deqp::Context& context);

	~XFBGlobalBufferTest()
	{
	}

protected:
	/* Protected methods */
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual void		testInit();

private:
	/* Private types */
	struct _testCase
	{
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<_testCase> m_test_cases;
};

/** Implementation of test XFBStride. Description follows:
 *
 * Test verifies that expected stride values are used.
 *
 * Test following code snippet:
 *
 *     layout (xfb_offset = 0) out type output;
 *
 *     output = EXPECTED_VALUE;
 *
 * Test all types. Test all shader stages.
 **/
class XFBStrideTest : public BufferTestBase
{
public:
	XFBStrideTest(deqp::Context& context);
	~XFBStrideTest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;

	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual void		testInit();

private:
	/* Private types */
	struct testCase
	{
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBBlockMemberBuffer. Description follows:
 *
 * Test verifies that member of block have to use same buffer as block.
 *
 * Test following code snippet:
 *
 *     layout (xfb_offset = 0) out Goku
 *     {
 *                                 vec4 gohan;
 *         layout (xfb_buffer = 1) vec4 goten;
 *     };
 *
 * It is expected that compilation will fail.
 *
 * Test all shader stages.
 **/
class XFBBlockMemberBufferTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBBlockMemberBufferTest(deqp::Context& context);

	virtual ~XFBBlockMemberBufferTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBOutputOverlapping. Description follows:
 *
 * Test verifies that overlapped outputs are reported as errors by compiler.
 *
 * Test following code snippet:
 *
 *     layout (xfb_offset = sizeof(type))       out type gohan;
 *     layout (xfb_offset = 1.5 * sizeof(type)) out type goten;
 *
 *     gohan = EXPECTED_VALUE;
 *     goten = EXPECTED_VALUE;
 *
 * It is expected that compilation will fail.
 *
 * Test all shader stages. Test all types.
 **/
class XFBOutputOverlappingTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBOutputOverlappingTest(deqp::Context& context);

	virtual ~XFBOutputOverlappingTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_offset_gohan;
		glw::GLuint			  m_offset_goten;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBInvalidOffsetAlignment. Description follows:
 *
 * Test verifies that invalidly aligned outputs cause a failure to build
 * process.
 *
 * Test following code snippet:
 *
 *     layout (xfb_offset = OFFSET) out type goku;
 *
 *     goku = EXPECTED_VALUE;
 *
 * Select OFFSET values so as to cause invalid alignment. Inspect program to
 * verify offset of goku.
 *
 * Test all shader stages. Test all types. Test all offsets in range:
 * (sizeof(type), 2 * sizeof(type)).
 **/
class XFBInvalidOffsetAlignmentTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBInvalidOffsetAlignmentTest(deqp::Context& context);

	virtual ~XFBInvalidOffsetAlignmentTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint			  m_offset;
		Utils::Shader::STAGES m_stage;
		Utils::Type			  m_type;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

/** Implementation of test XFBCaptureInactiveOutputVariable. Description follows:
 *
 * Test verifies behaviour of inactive outputs.
 *
 * This test implements Buffer algorithm. Rasterization can be disabled. Draw
 * two vertices instead of one. Test following code snippet:
 *
 *     layout (xfb_offset = 16) out vec4 goku;
 *     layout (xfb_offset = 32) out vec4 gohan;
 *     layout (xfb_offset = 0)  out vec4 goten;
 *
 *     gohan = EXPECTED_VALUE;
 *     goten = EXPECTED_VALUE;
 *
 * Test pass if:
 *     - values captured for goten and gohan are as expected,
 *     - goku value is undefined
 *     - stride is 3 * sizeof(vec4) - 48
 *
 * Test all shader stages.
 **/
class XFBCaptureInactiveOutputVariableTest : public BufferTestBase
{
public:
	XFBCaptureInactiveOutputVariableTest(deqp::Context& context);
	~XFBCaptureInactiveOutputVariableTest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;
	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();

	virtual bool inspectProgram(glw::GLuint test_case_index, Utils::Program& program, std::stringstream& out_stream);

	virtual bool verifyBuffers(bufferCollection& buffers);

private:
	enum test_cases
	{
		TEST_VS = 0,
		TEST_TES,
		TEST_GS,

		/* */
		TEST_MAX
	};
};

/** Implementation of test XFBCaptureInactiveOutputComponent. Description follows:
 *
 * Test verifies behaviour of inactive component.
 *
 * This test implements Buffer algorithm. Rasterization can be disabled. Draw
 * two vertices instead of one. Test following code snippet:
 *
 *     layout (xfb_offset = 32)  out vec4 goku;
 *     layout (xfb_offset = 0)   out vec4 gohan;
 *     layout (xfb_offset = 16)  out vec4 goten;
 *     layout (xfb_offset = 48)  out vec4 chichi;
 *     layout (xfb_offset = 112) out vec4 vegeta;
 *     layout (xfb_offset = 96)  out vec4 trunks;
 *     layout (xfb_offset = 80)  out vec4 bra;
 *     layout (xfb_offset = 64)  out vec4 bulma;
 *
 *     goku.x   = EXPECTED_VALUE;
 *     goku.z   = EXPECTED_VALUE;
 *     gohan.y  = EXPECTED_VALUE;
 *     gohan.w  = EXPECTED_VALUE;
 *     goten.x  = EXPECTED_VALUE;
 *     goten.y  = EXPECTED_VALUE;
 *     chichi.z = EXPECTED_VALUE;
 *     chichi.w = EXPECTED_VALUE;
 *     vegeta.x = EXPECTED_VALUE;
 *     trunks.y = EXPECTED_VALUE;
 *     bra.z    = EXPECTED_VALUE;
 *     bulma.w  = EXPECTED_VALUE;
 *
 * Test pass when captured values of all assigned components match expected
 * values, while not assigned ones are undefined.
 *
 * Test all shader stages.
 **/
class XFBCaptureInactiveOutputComponentTest : public BufferTestBase
{
public:
	XFBCaptureInactiveOutputComponentTest(deqp::Context& context);
	~XFBCaptureInactiveOutputComponentTest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;
	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);

	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool verifyBuffers(bufferCollection& buffers);

private:
	enum test_cases
	{
		TEST_VS = 0,
		TEST_TES,
		TEST_GS,

		/* */
		TEST_MAX
	};
};

/** Implementation of test XFBCaptureInactiveOutputBlockMember. Description follows:
 *
 * Test verifies behaviour of inactive block member.
 *
 * This test implements Buffer algorithm. Rasterization can be disabled. Draw
 * two vertices instead of one. Test following code snippet:
 *
 *     layout (xfb_offset = 16)  out Goku {
 *         vec4 gohan;
 *         vec4 goten;
 *         vec4 chichi;
 *     };
 *
 *     gohan  = EXPECTED_VALUE;
 *     chichi = EXPECTED_VALUE;
 *
 * Test pass when captured values of gohan and chichi match expected values.
 * It is expected that goten will receive undefined value.
 *
 * Test all shader stages.
 **/
class XFBCaptureInactiveOutputBlockMemberTest : public BufferTestBase
{
public:
	XFBCaptureInactiveOutputBlockMemberTest(deqp::Context& context);

	~XFBCaptureInactiveOutputBlockMemberTest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;
	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);
	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool verifyBuffers(bufferCollection& buffers);

private:
	enum test_cases
	{
		TEST_VS = 0,
		TEST_TES,
		TEST_GS,

		/* */
		TEST_MAX
	};
};

/** Implementation of test XFBCaptureStruct. Description follows:
 *
 * Test verifies that structures are captured as expected.
 *
 * This test implements Buffer algorithm. Rasterization can be disabled. Draw
 * two vertices instead of one. Test following code snippet:
 *
 *     struct Goku {
 *         vec4 gohan;
 *         vec4 goten;
 *         vec4 chichi;
 *     };
 *
 *     layout (xfb_offset = 16) out Goku goku;
 *
 *     goku.gohan  = EXPECTED_VALUE;
 *     goku.chichi = EXPECTED_VALUE;
 *
 * Test pass when captured values of gohan and chichi match expected values.
 * It is expected that goten will receive undefined value.
 *
 * Test all shader stages.
 **/
class XFBCaptureStructTest : public BufferTestBase
{
public:
	XFBCaptureStructTest(deqp::Context& context);

	~XFBCaptureStructTest()
	{
	}

protected:
	/* Protected methods */
	using BufferTestBase::executeDrawCall;
	virtual bool executeDrawCall(bool tesEnabled, glw::GLuint test_case_index);

	virtual void getBufferDescriptors(glw::GLuint test_case_index, bufferDescriptor::Vector& out_descriptors);

	virtual void getShaderBody(glw::GLuint test_case_index, Utils::Shader::STAGES stage, std::string& out_assignments,
							   std::string& out_calculations);

	virtual void getShaderInterface(glw::GLuint test_case_index, Utils::Shader::STAGES stage,
									std::string& out_interface);

	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool verifyBuffers(bufferCollection& buffers);

private:
	enum test_cases
	{
		TEST_VS = 0,
		TEST_TES,
		TEST_GS,

		/* */
		TEST_MAX
	};
};

/** Implementation of test XFBCaptureUnsizedArray. Description follows:
 *
 * Test verifies this is not allowed to qualify unsized array with "xfb".
 *
 * Test following code snippet:
 *
 *     layout (xfb_offset = 0) out vec4 goku[];
 *
 *     goku[0] = EXPECTED_VALUE;
 *
 * It is expected that compilation will fail.
 *
 * Test all shader stages.
 **/
class XFBCaptureUnsizedArrayTest : public NegativeTestBase
{
public:
	/* Public methods */
	XFBCaptureUnsizedArrayTest(deqp::Context& context);
	virtual ~XFBCaptureUnsizedArrayTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual std::string getShaderSource(glw::GLuint test_case_index, Utils::Shader::STAGES stage);

	virtual std::string getTestCaseName(glw::GLuint test_case_index);
	virtual glw::GLuint getTestCaseNumber();
	virtual bool isComputeRelevant(glw::GLuint test_case_index);
	virtual void testInit();

private:
	/* Private types */
	struct testCase
	{
		Utils::Shader::STAGES m_stage;
	};

	/* Private fields */
	std::vector<testCase> m_test_cases;
};

} /* EnhancedLayouts namespace */

/** Group class for Shader Language 420Pack conformance tests */
class EnhancedLayoutsTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	EnhancedLayoutsTests(deqp::Context& context);

	virtual ~EnhancedLayoutsTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	EnhancedLayoutsTests(const EnhancedLayoutsTests& other);
	EnhancedLayoutsTests& operator=(const EnhancedLayoutsTests& other);
};

} // gl4cts

#endif // _GL4CENHANCEDLAYOUTSTESTS_HPP
