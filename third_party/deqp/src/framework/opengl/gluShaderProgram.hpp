#ifndef _GLUSHADERPROGRAM_HPP
#define _GLUSHADERPROGRAM_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES Utilities
 * ------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Shader and Program helpers.
 *//*--------------------------------------------------------------------*/

#include "gluDefs.hpp"
#include "gluShaderUtil.hpp"
#include "glwEnums.hpp"
#include "qpTestLog.h"

#include <string>
#include <vector>

namespace tcu
{
class TestLog;
}

namespace glu
{

class RenderContext;

typedef std::vector<deUint32> ShaderBinaryDataType;

/*--------------------------------------------------------------------*//*!
 * \brief Shader information (compile status, log, etc.).
 *//*--------------------------------------------------------------------*/
struct ShaderInfo
{
	ShaderType				type;			//!< Shader type.
	std::string				source;			//!< Shader source.
	std::string				infoLog;		//!< Compile info log.
	bool					compileOk;		//!< Did compilation succeed?
	deUint64				compileTimeUs;	//!< Compile time in microseconds (us).

	ShaderInfo (void) : compileOk(false), compileTimeUs(0) {}
};

/*--------------------------------------------------------------------*//*!
 * \brief Program information (link status, log).
 *//*--------------------------------------------------------------------*/
struct ProgramInfo
{
	std::string				infoLog;		//!< Link info log.
	bool					linkOk;			//!< Did link succeed?
	deUint64				linkTimeUs;		//!< Link time in microseconds (us).

	ProgramInfo (void) : linkOk(false), linkTimeUs(0) {}
};

/*--------------------------------------------------------------------*//*!
 * \brief Combined shader compilation and program linking info.
 *//*--------------------------------------------------------------------*/
struct ShaderProgramInfo
{
	glu::ProgramInfo				program;
	std::vector<glu::ShaderInfo>	shaders;
};

/*--------------------------------------------------------------------*//*!
 * \brief Shader object.
 *//*--------------------------------------------------------------------*/
class Shader
{
public:
							Shader				(const glw::Functions& gl, ShaderType shaderType);
							Shader				(const RenderContext& renderCtx, ShaderType shaderType);
							~Shader				(void);

	void					setSources			(int numSourceStrings, const char* const* sourceStrings, const int* lengths);
	void					compile				(void);
	void					specialize			(const char* entryPoint, glw::GLuint numSpecializationConstants,
												 const glw::GLuint* constantIndex, const glw::GLuint* constantValue);

	deUint32				getShader			(void) const { return m_shader;				}
	const ShaderInfo&		getInfo				(void) const { return m_info;				}

	glu::ShaderType			getType				(void) const { return getInfo().type;		}
	bool					getCompileStatus	(void) const { return getInfo().compileOk;	}
	const std::string&		getSource			(void) const { return getInfo().source;		}
	const std::string&		getInfoLog			(void) const { return getInfo().infoLog;	}

	deUint32				operator*			(void) const { return getShader();			}

private:
							Shader				(const Shader& other);
	Shader&					operator=			(const Shader& other);

	const glw::Functions&	m_gl;
	deUint32				m_shader;	//!< Shader handle.
	ShaderInfo				m_info;		//!< Client-side clone of state for debug / perf reasons.
};

/*--------------------------------------------------------------------*//*!
 * \brief Program object.
 *//*--------------------------------------------------------------------*/
class Program
{
public:
							Program						(const glw::Functions& gl);
							Program						(const RenderContext& renderCtx);
							Program						(const RenderContext& renderCtx, deUint32 program);
							~Program					(void);

	void					attachShader				(deUint32 shader);
	void					detachShader				(deUint32 shader);

	void					bindAttribLocation			(deUint32 location, const char* name);
	void					transformFeedbackVaryings	(int count, const char* const* varyings, deUint32 bufferMode);

	void					link						(void);

	deUint32				getProgram					(void) const { return m_program;			}
	const ProgramInfo&		getInfo						(void) const { return m_info;				}

	bool					getLinkStatus				(void) const { return getInfo().linkOk;		}
	const std::string&		getInfoLog					(void) const { return getInfo().infoLog;	}

	bool					isSeparable					(void) const;
	void					setSeparable				(bool separable);

	int						getUniformLocation			(const std::string& name);

	deUint32				operator*					(void) const { return getProgram();			}

private:
							Program						(const Program& other);
	Program&				operator=					(const Program& other);

	const glw::Functions&	m_gl;
	deUint32				m_program;
	ProgramInfo				m_info;
};


/*--------------------------------------------------------------------*//*!
 * \brief Program pipeline object.
 *//*--------------------------------------------------------------------*/
class ProgramPipeline
{
public:
							ProgramPipeline				(const RenderContext& renderCtx);
							ProgramPipeline				(const glw::Functions& gl);
							~ProgramPipeline			(void);

	deUint32				getPipeline					(void) const { return m_pipeline; }
	void					useProgramStages			(deUint32 stages, deUint32 program);
	void					activeShaderProgram			(deUint32 program);
	bool					isValid						(void);

private:
							ProgramPipeline				(const ProgramPipeline& other);
	ProgramPipeline&		operator=					(const ProgramPipeline& other);

	const glw::Functions&	m_gl;
	deUint32				m_pipeline;
};

struct ProgramSources;
struct ProgramBinaries;

/*--------------------------------------------------------------------*//*!
 * \brief Shader program manager.
 *
 * ShaderProgram manages both Shader and Program objects, and provides
 * convenient API for constructing such programs.
 *//*--------------------------------------------------------------------*/
class ShaderProgram
{
public:
							ShaderProgram				(const glw::Functions& gl, const ProgramSources& sources);
							ShaderProgram				(const glw::Functions& gl, const ProgramBinaries& binaries);
							ShaderProgram				(const RenderContext& renderCtx, const ProgramSources& sources);
							ShaderProgram				(const RenderContext& renderCtx, const ProgramBinaries& binaries);
							~ShaderProgram				(void);

	bool					isOk						(void) const											{ return m_program.getLinkStatus();						}
	deUint32				getProgram					(void) const											{ return m_program.getProgram();						}

	bool					hasShader					(glu::ShaderType shaderType) const						{ return !m_shaders[shaderType].empty();				}
	Shader*					getShader					(glu::ShaderType shaderType, int shaderNdx = 0) const	{ return m_shaders[shaderType][shaderNdx];	}
	int						getNumShaders				(glu::ShaderType shaderType) const						{ return (int)m_shaders[shaderType].size();				}
	const ShaderInfo&		getShaderInfo				(glu::ShaderType shaderType, int shaderNdx = 0) const	{ return m_shaders[shaderType][shaderNdx]->getInfo();	}
	const ProgramInfo&		getProgramInfo				(void) const											{ return m_program.getInfo();							}

private:
							ShaderProgram				(const ShaderProgram& other);
	ShaderProgram&			operator=					(const ShaderProgram& other);
	void					init						(const glw::Functions& gl, const ProgramSources& sources);
	void					init						(const glw::Functions& gl, const ProgramBinaries& binaries);
	void					setBinary					(const glw::Functions& gl, std::vector<Shader*>& shaders, glw::GLenum binaryFormat, const void* binaryData, const int length);

	std::vector<Shader*>	m_shaders[SHADERTYPE_LAST];
	Program					m_program;
};

// Utilities.

deUint32		getGLShaderType		(ShaderType shaderType);
deUint32		getGLShaderTypeBit	(ShaderType shaderType);
qpShaderType	getLogShaderType	(ShaderType shaderType);

tcu::TestLog&	operator<<			(tcu::TestLog& log, const ShaderInfo& shaderInfo);
tcu::TestLog&	operator<<			(tcu::TestLog& log, const ShaderProgramInfo& shaderProgramInfo);
tcu::TestLog&	operator<<			(tcu::TestLog& log, const ProgramSources& sources);
tcu::TestLog&	operator<<			(tcu::TestLog& log, const Shader& shader);
tcu::TestLog&	operator<<			(tcu::TestLog& log, const ShaderProgram& program);

// ProgramSources utilities and implementation.

struct AttribLocationBinding
{
	std::string			name;
	deUint32			location;

	AttribLocationBinding (void) : location(0) {}
	AttribLocationBinding (const std::string& name_, deUint32 location_) : name(name_), location(location_) {}
};

struct TransformFeedbackMode
{
	deUint32			mode;

	TransformFeedbackMode (void) : mode(0) {}
	TransformFeedbackMode (deUint32 mode_) : mode(mode_) {}
};

struct TransformFeedbackVarying
{
	std::string			name;

	explicit TransformFeedbackVarying (const std::string& name_) : name(name_) {}
};

struct ProgramSeparable
{
	bool				separable;
	explicit ProgramSeparable (bool separable_) : separable(separable_) {}
};

template<typename Iterator>
struct TransformFeedbackVaryings
{
	Iterator			begin;
	Iterator			end;

	TransformFeedbackVaryings (Iterator begin_, Iterator end_) : begin(begin_), end(end_) {}
};

struct ShaderSource
{
	ShaderType			shaderType;
	std::string			source;

	ShaderSource (void) : shaderType(SHADERTYPE_LAST) {}
	ShaderSource (glu::ShaderType shaderType_, const std::string& source_) : shaderType(shaderType_), source(source_) { DE_ASSERT(!source_.empty()); }
};

struct VertexSource : public ShaderSource
{
	VertexSource (const std::string& source_) : ShaderSource(glu::SHADERTYPE_VERTEX, source_) {}
};

struct FragmentSource : public ShaderSource
{
	FragmentSource (const std::string& source_) : ShaderSource(glu::SHADERTYPE_FRAGMENT, source_) {}
};

struct GeometrySource : public ShaderSource
{
	GeometrySource (const std::string& source_) : ShaderSource(glu::SHADERTYPE_GEOMETRY, source_) {}
};

struct ComputeSource : public ShaderSource
{
	ComputeSource (const std::string& source_) : ShaderSource(glu::SHADERTYPE_COMPUTE, source_) {}
};

struct TessellationControlSource : public ShaderSource
{
	TessellationControlSource (const std::string& source_) : ShaderSource(glu::SHADERTYPE_TESSELLATION_CONTROL, source_) {}
};

struct TessellationEvaluationSource : public ShaderSource
{
	TessellationEvaluationSource (const std::string& source_) : ShaderSource(glu::SHADERTYPE_TESSELLATION_EVALUATION, source_) {}
};

struct ProgramSources
{
	std::vector<std::string>			sources[SHADERTYPE_LAST];
	std::vector<AttribLocationBinding>	attribLocationBindings;

	deUint32							transformFeedbackBufferMode;		//!< TF buffer mode, or GL_NONE.
	std::vector<std::string>			transformFeedbackVaryings;
	bool								separable;

	ProgramSources (void) : transformFeedbackBufferMode(0), separable(false) {}

	ProgramSources&						operator<<			(const AttribLocationBinding& binding)		{ attribLocationBindings.push_back(binding);						return *this;	}
	ProgramSources&						operator<<			(const TransformFeedbackMode& mode)			{ transformFeedbackBufferMode = mode.mode;							return *this;	}
	ProgramSources&						operator<<			(const TransformFeedbackVarying& varying)	{ transformFeedbackVaryings.push_back(varying.name);				return *this;	}
	ProgramSources&						operator<<			(const ShaderSource& shaderSource)			{ sources[shaderSource.shaderType].push_back(shaderSource.source);	return *this;	}
	ProgramSources&						operator<<			(const ProgramSeparable& progSeparable)		{ separable = progSeparable.separable;								return *this;	}

	template<typename Iterator>
	ProgramSources&						operator<<			(const TransformFeedbackVaryings<Iterator>& varyings);
};

struct SpecializationData
{
	deUint32 index;
	deUint32 value;

	SpecializationData (void) : index(0), value(0) {}
	SpecializationData (const deUint32 index_, const deUint32 value_) : index(index_), value(value_) {}
};

struct ShaderBinary
{
	ShaderBinaryDataType		binary;
	std::vector<ShaderType>		shaderTypes;
	std::vector<std::string>	shaderEntryPoints;
	std::vector<deUint32>		specializationIndices;
	std::vector<deUint32>		specializationValues;

	ShaderBinary (void) {}
	ShaderBinary (const ShaderBinaryDataType binary_) : binary(binary_)
	{
		DE_ASSERT(!binary_.empty());
	}
	ShaderBinary (const ShaderBinaryDataType binary_, glu::ShaderType shaderType_) : binary(binary_)
	{
		DE_ASSERT(!binary_.empty());
		shaderTypes.push_back(shaderType_);
		shaderEntryPoints.push_back("main");
	}

	ShaderBinary& operator<< (const ShaderType& shaderType)
	{
		shaderTypes.push_back(shaderType);
		return *this;
	}

	ShaderBinary& operator<< (const std::string& entryPoint)
	{
		shaderEntryPoints.push_back(entryPoint);
		return *this;
	}

	ShaderBinary& operator<< (const SpecializationData& specData)
	{
		specializationIndices.push_back(specData.index);
		specializationValues.push_back(specData.value);
		return *this;
	}
};

struct VertexBinary : public ShaderBinary
{
	VertexBinary (const ShaderBinaryDataType binary_) : ShaderBinary(binary_, glu::SHADERTYPE_VERTEX) {}
};

struct FragmentBinary : public ShaderBinary
{
	FragmentBinary (const ShaderBinaryDataType binary_) : ShaderBinary(binary_, glu::SHADERTYPE_FRAGMENT) {}
};

struct GeometryBinary : public ShaderBinary
{
	GeometryBinary (const ShaderBinaryDataType binary_) : ShaderBinary(binary_, glu::SHADERTYPE_GEOMETRY) {}
};

struct ComputeBinary : public ShaderBinary
{
	ComputeBinary (const ShaderBinaryDataType binary_) : ShaderBinary(binary_, glu::SHADERTYPE_COMPUTE) {}
};

struct TessellationControlBinary : public ShaderBinary
{
	TessellationControlBinary (const ShaderBinaryDataType binary_) : ShaderBinary(binary_, glu::SHADERTYPE_TESSELLATION_CONTROL) {}
};

struct TessellationEvaluationBinary : public ShaderBinary
{
	TessellationEvaluationBinary (const ShaderBinaryDataType binary_) : ShaderBinary(binary_, glu::SHADERTYPE_TESSELLATION_EVALUATION) {}
};

struct ProgramBinaries
{
	std::vector<ShaderBinary>	binaries;

	glw::GLenum					binaryFormat;

	ProgramBinaries (void) : binaryFormat(GL_SHADER_BINARY_FORMAT_SPIR_V_ARB) {}
	ProgramBinaries (glw::GLenum binaryFormat_) : binaryFormat(binaryFormat_) {}

	ProgramBinaries& operator<< (const ShaderBinary& shaderBinary)	{ binaries.push_back(shaderBinary);	return *this;	}
};

template<typename Iterator>
inline ProgramSources& ProgramSources::operator<< (const TransformFeedbackVaryings<Iterator>& varyings)
{
	for (Iterator cur = varyings.begin; cur != varyings.end; ++cur)
		transformFeedbackVaryings.push_back(*cur);
	return *this;
}

//! Helper for constructing vertex-fragment source pair.
inline ProgramSources makeVtxFragSources (const std::string& vertexSrc, const std::string& fragmentSrc)
{
	ProgramSources sources;
	sources.sources[SHADERTYPE_VERTEX].push_back(vertexSrc);
	sources.sources[SHADERTYPE_FRAGMENT].push_back(fragmentSrc);
	return sources;
}

} // glu

#endif // _GLUSHADERPROGRAM_HPP
