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
#include "es31cLayoutBindingTests.hpp"

#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"

#include "deRandom.hpp"
#include "deStringUtil.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "gluTexture.hpp"
#include "gluTextureUtil.hpp"

namespace glcts
{

//=========================================================================
//= typedefs
//=========================================================================
typedef std::string String;
typedef std::map<String, String>		 StringMap;
typedef std::map<String, glw::GLint>	 StringIntMap;
typedef std::map<glw::GLint, glw::GLint> IntIntMap;
typedef std::vector<String> StringVector;
typedef std::vector<int>	IntVector;

typedef std::map<int, glu::Texture2D*>		Texture2DMap;
typedef std::map<int, glu::Texture2DArray*> Texture2DArrayMap;
typedef std::map<int, glu::Texture3D*>		Texture3DMap;

//=========================================================================
//= utility classes
//=========================================================================

//= string stream that saves some typing
class StringStream : public std::ostringstream
{
public:
	void reset()
	{
		clear();
		str("");
	}
};

class LayoutBindingProgram;

class IProgramContextSupplier
{
public:
	virtual ~IProgramContextSupplier()
	{
	}
	virtual Context&					   getContext()		   = 0;
	virtual const LayoutBindingParameters& getTestParameters() = 0;
	virtual eStageType					   getStage()		   = 0;
	virtual const String& getSource(eStageType stage)		   = 0;
	virtual LayoutBindingProgram* createProgram()			   = 0;
};

class LayoutBindingProgram
{
public:
	LayoutBindingProgram(IProgramContextSupplier& contextSupplier)
		: m_contextSupplier(contextSupplier)
		, m_context(contextSupplier.getContext().getRenderContext())
		, m_stage(contextSupplier.getStage())
		, m_testParams(contextSupplier.getTestParameters())
		, m_gl(contextSupplier.getContext().getRenderContext().getFunctions())
	{
		if (getStage() != ComputeShader)
			m_program = new glu::ShaderProgram(
				m_context, glu::makeVtxFragSources(m_contextSupplier.getSource(VertexShader).c_str(),
												   m_contextSupplier.getSource(FragmentShader).c_str()));
		else
			m_program = new glu::ShaderProgram(
				m_context,
				glu::ProgramSources() << glu::ComputeSource(m_contextSupplier.getSource(ComputeShader).c_str()));
	}
	virtual ~LayoutBindingProgram()
	{
		delete m_program;
	}

	class LayoutBindingProgramAutoPtr
	{
	public:
		LayoutBindingProgramAutoPtr(IProgramContextSupplier& contextSupplier)
		{
			m_program = contextSupplier.createProgram();
		}
		~LayoutBindingProgramAutoPtr()
		{
			delete m_program;
		}
		LayoutBindingProgram* operator->()
		{
			return m_program;
		}

	private:
		LayoutBindingProgram* m_program;
	};

	String getErrorLog(bool dumpShaders = false)
	{
		StringStream errLog;

		if (getStage() != ComputeShader)
		{
			const glu::ShaderInfo& fragmentShaderInfo = m_program->getShaderInfo(glu::SHADERTYPE_FRAGMENT);
			const glu::ShaderInfo& vertexShaderInfo   = m_program->getShaderInfo(glu::SHADERTYPE_VERTEX);

			if (!fragmentShaderInfo.compileOk || !m_program->getProgramInfo().linkOk || dumpShaders)
			{
				errLog << "### dump of " << stageToName(FragmentShader) << "###\n";
				String msg((dumpShaders ? "Fragment shader should not have compiled" :
										  "Vertex shader compile failed while testing "));
				errLog << "Fragment shader compile failed while testing " << stageToName(getStage()) << ": "
					   << fragmentShaderInfo.infoLog << "\n";
				errLog << m_contextSupplier.getSource(FragmentShader);
			}
			if (!vertexShaderInfo.compileOk || !m_program->getProgramInfo().linkOk || dumpShaders)
			{
				errLog << "### dump of " << stageToName(VertexShader) << "###\n";
				String msg((dumpShaders ? "Vertex shader should not have compiled" :
										  "Vertex shader compile failed while testing "));
				errLog << msg << stageToName(getStage()) << ": " << vertexShaderInfo.infoLog << "\n";
				errLog << m_contextSupplier.getSource(VertexShader);
			}
		}
		else
		{
			const glu::ShaderInfo& computeShaderInfo = m_program->getShaderInfo(glu::SHADERTYPE_COMPUTE);

			if (!computeShaderInfo.compileOk || !m_program->getProgramInfo().linkOk || dumpShaders)
			{
				errLog << "### dump of " << stageToName(ComputeShader) << "###\n";
				String msg((dumpShaders ? "Compute shader should not have compiled" :
										  "Compute shader compile failed while testing "));
				errLog << msg << stageToName(ComputeShader) << ": " << computeShaderInfo.infoLog << "\n";
				errLog << m_contextSupplier.getSource(ComputeShader);
			}
		}
		if (!m_program->getProgramInfo().linkOk)
		{
			getStage();
			errLog << "Linking failed while testing " << stageToName(getStage()) << ": "
				   << m_program->getProgramInfo().infoLog << "\n";
			errLog << "### other stages ###\n";
			switch (getStage())
			{
			case FragmentShader:
				errLog << "### dump of " << stageToName(VertexShader) << "###\n";
				errLog << m_contextSupplier.getSource(VertexShader);
				break;
			case VertexShader:
				errLog << "### dump of " << stageToName(FragmentShader) << "###\n";
				errLog << stageToName(FragmentShader) << "\n";
				errLog << m_contextSupplier.getSource(FragmentShader);
				break;
			case ComputeShader:
				errLog << "### dump of " << stageToName(ComputeShader) << "###\n";
				errLog << stageToName(ComputeShader) << "\n";
				errLog << m_contextSupplier.getSource(ComputeShader);
				break;
			default:
				DE_ASSERT(0);
				break;
			}
		}
		return errLog.str();
	}

private:
	IProgramContextSupplier& m_contextSupplier;

	const glu::RenderContext&	  m_context;
	const eStageType			   m_stage;		 // shader stage currently tested
	const LayoutBindingParameters& m_testParams; // parameters for shader generation (table at end of file)
	const glw::Functions&		   m_gl;

	glu::ShaderProgram* m_program;

private:
	StringIntMap getUniformLocations(StringVector args) const
	{
		StringVector::iterator it;
		StringIntMap		   locations;
		bool				   passed = true;

		for (it = args.begin(); it != args.end(); it++)
		{
			const char* name	 = (*it).c_str();
			glw::GLint  location = m_gl.getUniformLocation(getProgram(), name);
			passed &= (0 <= location);
			if (passed)
			{
				locations[name] = location;
			}
		}

		return locations;
	}

public:
	glw::GLint getProgram() const
	{
		return m_program->getProgram();
	}

	const glw::Functions& gl() const
	{
		return m_gl;
	}

	virtual eStageType getStage() const
	{
		return m_stage;
	}

	String stageToName(eStageType stage) const
	{
		switch (stage)
		{
		case FragmentShader:
			return "FragmentShader";
		case VertexShader:
			return "VertexShader";
		case ComputeShader:
			return "ComputeShader";
		default:
			DE_ASSERT(0);
			break;
		}
		return String();
	}

	bool error() const
	{
		return (m_gl.getError() == GL_NO_ERROR);
	}

	bool compiledAndLinked() const
	{
		if (getStage() != ComputeShader)
			return m_program->getShaderInfo(glu::SHADERTYPE_FRAGMENT).compileOk &&
				   m_program->getShaderInfo(glu::SHADERTYPE_FRAGMENT).compileOk && m_program->getProgramInfo().linkOk;

		return m_program->getShaderInfo(glu::SHADERTYPE_COMPUTE).compileOk && m_program->getProgramInfo().linkOk;
	}

	virtual StringIntMap getBindingPoints(StringVector args) const
	{
		StringIntMap bindingPoints;

		StringIntMap locations = getUniformLocations(args);
		if (!locations.empty())
		{
			glw::GLint bindingPoint;
			for (StringIntMap::iterator it = locations.begin(); it != locations.end(); it++)
			{
				glw::GLint location = it->second;
				m_gl.getUniformiv(getProgram(), location, &bindingPoint);
				bool hasNoError = (GL_NO_ERROR == m_gl.getError());
				if (hasNoError)
				{
					bindingPoints[it->first] = bindingPoint;
				}
			}
		}
		return bindingPoints;
	}

	virtual bool setBindingPoints(StringVector list, glw::GLint bindingPoint) const
	{
		bool bNoError = true;

		StringIntMap locations = getUniformLocations(list);
		if (!locations.empty())
		{
			for (StringIntMap::iterator it = locations.begin(); it != locations.end(); it++)
			{
				m_gl.uniform1i(it->second, bindingPoint);
				bNoError &= (GL_NO_ERROR == m_gl.getError());
			}
		}
		return bNoError;
	}

	virtual StringIntMap getOffsets(StringVector /*args*/) const
	{
		return StringIntMap();
	}
};

class LayoutBindingTestResult
{
public:
	LayoutBindingTestResult(bool passed = true, const String& reason = String(), bool notRunforThisContext = false)
		: m_passed(passed), m_notRunForThisContext(notRunforThisContext), m_reason(reason)
	{
	}

public:
	bool testPassed() const
	{
		return m_passed;
	}

	String getReason() const
	{
		return m_reason;
	}

	bool runForThisContext() const
	{
		return !m_notRunForThisContext;
	}

private:
	bool   m_passed;
	bool   m_notRunForThisContext;
	String m_reason;
};

class IntegerConstant
{
public:
	enum Literals
	{
		decimal = 0,
		decimal_u,
		decimal_U,
		octal,
		octal_u,
		octal_U,
		hex_x,
		hex_X,
		hex_u,
		hex_u_X,
		hex_U,
		hex_U_X,
		last
	};

public:
	IntegerConstant(Literals lit, int ai) : asInt(ai)
	{
		StringStream s;
		switch (lit)
		{
		case decimal:
			s << asInt;
			break;
		case decimal_u:
			s << asInt << "u";
			break;
		case decimal_U:
			s << asInt << "U";
			break;
		case octal:
			s << "0" << std::oct << asInt;
			break;
		case octal_u:
			s << "0" << std::oct << asInt << "u";
			break;
		case octal_U:
			s << "0" << std::oct << asInt << "U";
			break;
		case hex_x:
			s << "0x" << std::hex << asInt;
			break;
		case hex_X:
			s << "0X" << std::hex << asInt;
			break;
		case hex_u:
			s << "0x" << std::hex << asInt << "u";
			break;
		case hex_u_X:
			s << "0X" << std::hex << asInt << "u";
			break;
		case hex_U:
			s << "0x" << std::hex << asInt << "U";
			break;
		case hex_U_X:
			s << "0X" << std::hex << asInt << "U";
			break;
		case last:
		default:
			DE_ASSERT(0);
		}

		asString = s.str();
	}

public:
	String asString;
	int	asInt;
};

//*****************************************************************************
class LayoutBindingBaseCase : public TestCase, public IProgramContextSupplier
{
public:
	LayoutBindingBaseCase(Context& context, const char* name, const char* description, StageType stage,
						  LayoutBindingParameters& samplerType, glu::GLSLVersion glslVersion);
	virtual ~LayoutBindingBaseCase(void);

	IterateResult iterate(void);

	// overrideable subtests
	virtual LayoutBindingTestResult binding_basic_default(void);
	virtual LayoutBindingTestResult binding_basic_explicit(void);
	virtual LayoutBindingTestResult binding_basic_multiple(void);
	virtual LayoutBindingTestResult binding_basic_render(void);
	virtual LayoutBindingTestResult binding_integer_constant(void);
	virtual LayoutBindingTestResult binding_integer_constant_expression(void);
	virtual LayoutBindingTestResult binding_array_size(void);
	virtual LayoutBindingTestResult binding_array_implicit(void);
	virtual LayoutBindingTestResult binding_array_multiple(void);
	virtual LayoutBindingTestResult binding_api_update(void);
	virtual LayoutBindingTestResult binding_compilation_errors(void);
	virtual LayoutBindingTestResult binding_link_errors(void);
	virtual LayoutBindingTestResult binding_examples(void);
	virtual LayoutBindingTestResult binding_mixed_order(void);

private:
	// drawTest normal vs. compute
	typedef LayoutBindingTestResult (LayoutBindingBaseCase::*LayoutBindingDrawTestPtr)(glw::GLint program, int binding);

	LayoutBindingDrawTestPtr m_drawTest;

	// pointer type for subtests
	typedef LayoutBindingTestResult (LayoutBindingBaseCase::*LayoutBindingSubTestPtr)();

	// test table entry
	struct LayoutBindingSubTest
	{
		char const*				name;
		char const*				description;
		LayoutBindingSubTestPtr test;
	};

	// IProgramContextSupplier interface
protected:
	const LayoutBindingParameters& getTestParameters()
	{
		return m_testParams;
	}

	const glu::RenderContext& getRenderContext()
	{
		return m_context.getRenderContext();
	}

	virtual eStageType getStage()
	{
		return m_stage.type;
	}

	const String& getSource(eStageType stage)
	{
		return m_sources[stage];
	}

	Context& getContext()
	{
		return m_context;
	}

	const glw::Functions& gl()
	{
		return m_context.getRenderContext().getFunctions();
	}

	bool needsPrecision() const
	{
		if (isContextTypeES(m_context.getRenderContext().getType()) || m_glslVersion == glu::GLSL_VERSION_450)
		{
			return (m_testParams.surface_type != UniformBlock) && (m_testParams.surface_type != ShaderStorageBuffer);
		}
		else
		{
			return (m_testParams.surface_type != UniformBlock) && (m_testParams.surface_type != ShaderStorageBuffer) &&
				   (m_testParams.surface_type != AtomicCounter) && (m_testParams.surface_type != Image);
		}
	}

protected:
	std::vector<int> makeSparseRange(int maxElement, int minElement = 0) const
	{
		static float	   rangeT[]		 = { 0.0f, 0.1f, 0.5f, 0.6f, 0.9f, 1.0f };
		std::vector<float> rangeTemplate = makeVector(rangeT);
		float			   max			 = rangeTemplate.back();
		float			   range		 = (float)((maxElement - 1) - minElement);

		std::vector<int> result;
		for (std::vector<float>::iterator it = rangeTemplate.begin(); it != rangeTemplate.end(); it++)
		{
			float e = *it;
			e		= (e * range) / max;
			result.insert(result.end(), minElement + (int)e);
		}
		return result;
	}

	glu::GLSLVersion getGLSLVersion()
	{
		return m_glslVersion;
	}

	bool isStage(eStageType stage)
	{
		return (stage == m_stage.type);
	}

	void setTemplateParam(eStageType stage, const char* param, const String& value)
	{
		m_templateParams[stage][param] = value.c_str();
	}

	void setTemplateParam(const char* param, const String& value)
	{
		setTemplateParam(m_stage.type, param, value);
	}

	void updateTemplate(eStageType stage)
	{
		m_sources[stage] = tcu::StringTemplate(m_templates[stage]).specialize(m_templateParams[stage]);
	}

	void updateTemplate()
	{
		updateTemplate(m_stage.type);
	}

	template <class T0, class T1>
	String generateLog(const String& msg, T0 result, T1 expected)
	{
		StringStream s;
		s << msg << " expected: " << expected << " actual: " << result << "\n";
		s << getSource(VertexShader) << "\n";
		s << getSource(FragmentShader) << "\n";
		return s.str();
	}

private:
	std::vector<LayoutBindingSubTest> m_tests;

	void init(void)
	{
		m_drawTest =
			(getStage() == ComputeShader) ? &LayoutBindingBaseCase::drawTestCompute : &LayoutBindingBaseCase::drawTest;

#define MAKE_TEST_ENTRY(__subtest_name__) { #__subtest_name__, "", &LayoutBindingBaseCase::__subtest_name__ }
		LayoutBindingSubTest tests[] = {
			MAKE_TEST_ENTRY(binding_basic_default),		 MAKE_TEST_ENTRY(binding_basic_explicit),
			MAKE_TEST_ENTRY(binding_basic_multiple),	 MAKE_TEST_ENTRY(binding_basic_render),
			MAKE_TEST_ENTRY(binding_integer_constant),   MAKE_TEST_ENTRY(binding_integer_constant_expression),
			MAKE_TEST_ENTRY(binding_array_size),		 MAKE_TEST_ENTRY(binding_array_implicit),
			MAKE_TEST_ENTRY(binding_array_multiple),	 MAKE_TEST_ENTRY(binding_api_update),
			MAKE_TEST_ENTRY(binding_compilation_errors), MAKE_TEST_ENTRY(binding_link_errors),
			MAKE_TEST_ENTRY(binding_examples),			 MAKE_TEST_ENTRY(binding_mixed_order)
		};
		m_tests = makeVector(tests);

		m_uniformDeclTemplate = "${LAYOUT}${KEYWORD}${UNIFORM_TYPE}${UNIFORM_BLOCK_NAME}${UNIFORM_BLOCK}${UNIFORM_"
								"INSTANCE_NAME}${UNIFORM_ARRAY};\n";

		m_expectedColor = tcu::Vec4(0.0, 1.0f, 0.0f, 1.0f);

		switch (getTestParameters().texture_type)
		{
		case TwoD:
		{
			// 2D
			glu::ImmutableTexture2D* texture2D =
				new glu::ImmutableTexture2D(getContext().getRenderContext(), GL_RGBA8, 2, 2);

			texture2D->getRefTexture().allocLevel(0);
			tcu::clear(texture2D->getRefTexture().getLevel(0), m_expectedColor);
			texture2D->upload();

			if (m_textures2D.find(0) != m_textures2D.end())
			{
				delete m_textures2D[0];
			}

			m_textures2D[0] = texture2D;
		}
		break;
		case TwoDArray:
		{
			// 2DArray
			glu::Texture2DArray* texture2DArray =
				new glu::Texture2DArray(getContext().getRenderContext(), GL_RGBA8, 2, 2, 1);

			texture2DArray->getRefTexture().allocLevel(0);
			tcu::clear(texture2DArray->getRefTexture().getLevel(0), m_expectedColor);
			texture2DArray->upload();

			if (m_textures2DArray.find(0) != m_textures2DArray.end())
			{
				delete m_textures2DArray[0];
			}

			m_textures2DArray[0] = texture2DArray;
		}
		break;
		// 3D
		case ThreeD:
		{
			glu::Texture3D* texture3D = new glu::Texture3D(getContext().getRenderContext(), GL_RGBA8, 2, 2, 1);

			texture3D->getRefTexture().allocLevel(0);
			tcu::clear(texture3D->getRefTexture().getLevel(0), m_expectedColor);
			texture3D->upload();

			if (m_textures3D.find(0) != m_textures3D.end())
			{
				delete m_textures3D[0];
			}

			m_textures3D[0] = texture3D;
		}
		break;
		case None:
			// test case where no texture allocation is needed
			break;
		default:
			DE_ASSERT(0);
			break;
		}
	}

	String initDefaultVSContext()
	{
		m_templates[VertexShader] = "${VERSION}"
									"layout(location=0) in vec2 inPosition;\n"
									"${UNIFORM_DECL}\n"
									"flat out ${OUT_VAR_TYPE} fragColor;\n"
									"${OPTIONAL_FUNCTION_BLOCK}\n"
									"void main(void)\n"
									"{\n"
									"  ${OUT_ASSIGNMENT} ${UNIFORM_ACCESS}\n"
									"  gl_Position = vec4(inPosition, 0.0, 1.0);\n"
									"}\n";

		StringMap& args = m_templateParams[VertexShader];
		// some samplers and all images don't have default precision qualifier (sampler3D)
		// so append a precision default in all sampler and image cases.
		StringStream s;
		s << glu::getGLSLVersionDeclaration(m_glslVersion) << "\n";
		s << "precision highp float;\n";
		if (needsPrecision())
		{
			s << "precision highp " << getTestParameters().uniform_type << ";\n";
		}

		args["VERSION"]					= s.str();
		args["UNIFORM_DECL"]			= "";
		args["OPTIONAL_FUNCTION_BLOCK"] = "";
		args["UNIFORM_ACCESS"]			= "";
		args["OUT_ASSIGNMENT"]			= "fragColor =";
		args["OUT_VAR_TYPE"]			= getTestParameters().vector_type;
		args["OUT_VAR"]					= "fragColor";
		if (m_stage.type != VertexShader)
		{
			args["OUT_ASSIGNMENT"] = "";
		}
		return tcu::StringTemplate(m_templates[VertexShader]).specialize(args);
	}

	String initDefaultFSContext()
	{
		// build fragment shader
		m_templates[FragmentShader] = "${VERSION}"
									  "layout(location=0) out ${OUT_VAR_TYPE} ${OUT_VAR};\n"
									  "flat in ${OUT_VAR_TYPE} fragColor;\n"
									  "${UNIFORM_DECL}\n"
									  "${OPTIONAL_FUNCTION_BLOCK}\n"
									  "void main(void)\n"
									  "{\n"
									  "  ${OUT_ASSIGNMENT} ${UNIFORM_ACCESS}\n"
									  "}\n";

		StringMap& args = m_templateParams[FragmentShader];
		// samplers and images don't have default precision qualifier
		StringStream s;
		s << glu::getGLSLVersionDeclaration(m_glslVersion) << "\n";
		s << "precision highp float;\n";
		if (needsPrecision())
		{
			s << "precision highp " << getTestParameters().uniform_type << ";\n";
		}
		args["VERSION"]					= s.str();
		args["OUT_VAR_TYPE"]			= getTestParameters().vector_type;
		args["UNIFORM_ACCESS"]			= "vec4(0.0,0.0,0.0,0.0);";
		args["UNIFORM_DECL"]			= "";
		args["OPTIONAL_FUNCTION_BLOCK"] = "";
		args["OUT_ASSIGNMENT"]			= "outColor =";
		args["OUT_VAR"]					= "outColor";
		// must have a linkage between stage and fragment shader
		// that the compiler can't optimize away
		if (FragmentShader != m_stage.type)
		{
			args["UNIFORM_ACCESS"] = "fragColor;";
		}
		return tcu::StringTemplate(m_templates[FragmentShader]).specialize(args);
	}

	String initDefaultCSContext()
	{
		// build compute shader
		m_templates[ComputeShader] = "${VERSION}"
									 "${UNIFORM_DECL}\n"
									 "${OPTIONAL_FUNCTION_BLOCK}\n"
									 "void main(void)\n"
									 "{\n"
									 "  ${OUT_VAR_TYPE} tmp = ${UNIFORM_ACCESS}\n"
									 "  ${OUT_ASSIGNMENT} tmp ${OUT_END}\n"
									 "}\n";

		StringMap& args = m_templateParams[ComputeShader];
		// images don't have default precision qualifier
		StringStream s;
		s << glu::getGLSLVersionDeclaration(m_glslVersion) << "\n";
		s << "layout (local_size_x = 1) in;\n"
			 "precision highp float;\n";
		if (needsPrecision())
		{
			s << "precision highp " << getTestParameters().uniform_type << ";\n";
		}

		// bindings are per uniform type...
		if (getTestParameters().surface_type == Image)
		{
			s << "layout(binding=0, std430) buffer outData {\n"
				 "    "
			  << getTestParameters().vector_type << " outColor;\n"
													"};\n";
			args["OUT_ASSIGNMENT"] = "outColor =";
			args["OUT_END"]		   = ";";
		}
		else
		{
			s << "layout(binding=0, rgba8) uniform highp writeonly image2D outImage;\n";
			args["OUT_ASSIGNMENT"] = "imageStore(outImage, ivec2(0), ";
			args["OUT_END"]		   = ");";
		}
		args["VERSION"]					= s.str();
		args["OUT_VAR_TYPE"]			= getTestParameters().vector_type;
		args["UNIFORM_ACCESS"]			= "vec4(0.0,0.0,0.0,0.0);";
		args["UNIFORM_DECL"]			= "";
		args["OPTIONAL_FUNCTION_BLOCK"] = "";

		return tcu::StringTemplate(m_templates[ComputeShader]).specialize(args);
	}

protected:
	String buildUniformDecl(const String& keyword, const String& layout, const String& uniform_type,
							const String& uniform_block_name, const String& uniform_block,
							const String& uniform_instance, const String& uniform_array) const
	{
		StringMap args;
		setArg(args["LAYOUT"], layout);
		setArg(args["KEYWORD"], keyword);
		if (uniform_block_name.empty())
			setArg(args["UNIFORM_TYPE"], uniform_type);
		else
			args["UNIFORM_TYPE"] = "";
		if (!uniform_instance.empty() && !uniform_block_name.empty())
			setArg(args["UNIFORM_BLOCK_NAME"], uniform_block_name + "_block");
		else
			setArg(args["UNIFORM_BLOCK_NAME"], uniform_block_name);
		setArg(args["UNIFORM_BLOCK"], uniform_block);
		if ((uniform_block_name.empty() && uniform_block.empty()) || !uniform_array.empty())
			args["UNIFORM_INSTANCE_NAME"] = uniform_instance;
		else
			args["UNIFORM_INSTANCE_NAME"] = "";
		args["UNIFORM_ARRAY"]			  = uniform_array;
		return tcu::StringTemplate(m_uniformDeclTemplate).specialize(args);
	}

	virtual String getDefaultUniformName(int idx = 0)
	{
		StringStream s;
		s << "uniform" << idx;
		return s.str();
	}

	virtual String buildUniformName(String& var)
	{
		std::ostringstream s;
		s << var;
		return s.str();
	}

	virtual String buildLayout(const String& binding)
	{
		std::ostringstream s;
		if (!binding.empty())
			s << "layout(binding=" << binding << ") ";
		return s.str();
	}

	virtual String buildLayout(int binding)
	{
		std::ostringstream bindingStr;
		bindingStr << binding;
		return buildLayout(bindingStr.str());
	}

	virtual String buildAccess(const String& var)
	{
		std::ostringstream s;
		s << getTestParameters().access_function << "(" << var << "," << getTestParameters().coord_vector_type << "(0)"
		  << ")";
		return s.str();
	}

	virtual String buildBlockName(const String& /*name*/)
	{
		return String();
	}

	virtual String buildBlock(const String& /*name*/, const String& /*type*/ = String("float"))
	{
		return String();
	}

	virtual String buildArray(int idx)
	{
		StringStream s;
		s << "[" << idx << "]";
		return s.str();
	}

	virtual String buildArrayAccess(int uniform, int idx)
	{
		StringStream s;
		s << getDefaultUniformName(uniform) << buildArray(idx);
		if (!buildBlockName(getDefaultUniformName()).empty())
		{
			s << "." << getDefaultUniformName(uniform);
		}
		return s.str();
	}

	// return max. binding point allowed
	virtual int maxBindings()
	{
		int units = 0;

		if (getTestParameters().surface_type == Image)
		{
			gl().getIntegerv(GL_MAX_IMAGE_UNITS, &units);
		}
		else
		{
			gl().getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &units);
		}

		return units;
	}

	// return max. array size allowed
	virtual int maxArraySize()
	{
		int units = 0;

		if (getTestParameters().surface_type == Image)
		{
			switch (m_stage.type)
			{
			case VertexShader:
				gl().getIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &units);
				break;
			case FragmentShader:
				gl().getIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &units);
				break;
			default:
				DE_ASSERT(0);
				break;
			}
		}
		else
		{
			switch (m_stage.type)
			{
			case VertexShader:
				gl().getIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &units);
				break;
			case FragmentShader:
				gl().getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
				break;
			default:
				DE_ASSERT(0);
				break;
			}
		}

		return units;
	}

	virtual bool isSupported()
	{
		return (maxArraySize() > 0);
	}

	virtual void bind(int binding)
	{
		glw::GLint texTarget = 0;
		glw::GLint texName   = 0;

		switch (getTestParameters().texture_type)
		{
		case TwoD:
			texTarget = GL_TEXTURE_2D;
			texName   = m_textures2D[0]->getGLTexture();
			break;
		case TwoDArray:
			texTarget = GL_TEXTURE_2D_ARRAY;
			texName   = m_textures2DArray[0]->getGLTexture();
			break;
		case ThreeD:
			texTarget = GL_TEXTURE_3D;
			texName   = m_textures3D[0]->getGLTexture();
			break;
		default:
			DE_ASSERT(0);
			break;
		}

		switch (getTestParameters().surface_type)
		{
		case Texture:
			gl().activeTexture(GL_TEXTURE0 + binding);
			gl().bindTexture(texTarget, texName);
			gl().texParameteri(texTarget, GL_TEXTURE_WRAP_S, glu::getGLWrapMode(tcu::Sampler::CLAMP_TO_EDGE));
			gl().texParameteri(texTarget, GL_TEXTURE_WRAP_T, glu::getGLWrapMode(tcu::Sampler::CLAMP_TO_EDGE));
			gl().texParameteri(texTarget, GL_TEXTURE_MIN_FILTER, glu::getGLFilterMode(tcu::Sampler::NEAREST));
			gl().texParameteri(texTarget, GL_TEXTURE_MAG_FILTER, glu::getGLFilterMode(tcu::Sampler::NEAREST));
			break;
		case Image:
			gl().bindImageTexture(binding, texName, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
			break;
		default:
			DE_ASSERT(0);
			break;
		}
	}

	virtual void unbind(int binding)
	{
		glw::GLint texTarget = 0;

		switch (getTestParameters().texture_type)
		{
		case TwoD:
			texTarget = GL_TEXTURE_2D;
			break;
		case TwoDArray:
			texTarget = GL_TEXTURE_2D_ARRAY;
			break;
		case ThreeD:
			texTarget = GL_TEXTURE_3D;
			break;
		default:
			DE_ASSERT(0);
			break;
		}

		switch (getTestParameters().surface_type)
		{
		case Texture:
			gl().activeTexture(GL_TEXTURE0 + binding);
			gl().bindTexture(texTarget, 0);
			break;
		case Image:
			gl().bindImageTexture(binding, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
			break;
		default:
			DE_ASSERT(0);
			break;
		}
	}

	virtual LayoutBindingTestResult drawTest(glw::GLint program, int binding);
	virtual LayoutBindingTestResult drawTestCompute(glw::GLint program, int binding);

	// allocate resources needed for all subtests, i.e. textures
	virtual void setupTest()
	{
	}

	// cleanup resources needed for all subtests
	virtual void teardownTest()
	{
		for (Texture2DMap::iterator it = m_textures2D.begin(); it != m_textures2D.end(); it++)
		{
			delete it->second;
		}
		m_textures2D.clear();

		for (Texture2DArrayMap::iterator it = m_textures2DArray.begin(); it != m_textures2DArray.end(); it++)
		{
			delete it->second;
		}
		m_textures2DArray.clear();

		for (Texture3DMap::iterator it = m_textures3D.begin(); it != m_textures3D.end(); it++)
		{
			delete it->second;
		}
		m_textures3D.clear();
	}

private:
	// appends a space to the argument (make shader source pretty)
	void setArg(String& arg, const String& value) const
	{
		if (!value.empty())
			arg = value + String(" ");
		else
			arg = String();
	}

private:
	LayoutBindingParameters m_testParams;
	StageType				m_stage;

	std::map<eStageType, String>	  m_sources;
	std::map<eStageType, StringMap>   m_templateParams;
	std::map<eStageType, const char*> m_templates;

	Texture2DMap	  m_textures2D;
	Texture2DArrayMap m_textures2DArray;
	Texture3DMap	  m_textures3D;
	tcu::Vec4		  m_expectedColor;

	const char* m_uniformDeclTemplate;

	glu::GLSLVersion m_glslVersion;
};

LayoutBindingBaseCase::LayoutBindingBaseCase(Context& context, const char* name, const char* description,
											 StageType stage, LayoutBindingParameters& samplerType,
											 glu::GLSLVersion glslVersion)
	: TestCase(context, name, description)
	, m_drawTest(DE_NULL)
	, m_testParams(samplerType)
	, m_stage(stage)
	, m_uniformDeclTemplate(DE_NULL)
	, m_glslVersion(glslVersion)
{
}

LayoutBindingBaseCase::~LayoutBindingBaseCase(void)
{
	teardownTest();
}

LayoutBindingBaseCase::IterateResult LayoutBindingBaseCase::iterate(void)
{
	tcu::TestLog& log	= m_context.getTestContext().getLog();
	bool		  passed = true;

	if (!isSupported())
	{
		log << tcu::TestLog::Section("NotSupported", "");
		log << tcu::TestLog::Message << "This test was not run as minimum requirements were not met."
			<< tcu::TestLog::EndMessage;
		log << tcu::TestLog::EndSection;
		getContext().getTestContext().setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "NotSupported");
		return STOP;
	}

	// init test case (create shader templates and textures)
	init();

	// allocate resources for all subtests
	setupTest();

	for (std::vector<LayoutBindingSubTest>::iterator it = m_tests.begin(); it != m_tests.end(); it++)
	{
		// need to reset templates and their args to a clean state before every
		// test to avoid bleeding.
		m_sources[VertexShader]   = initDefaultVSContext();
		m_sources[FragmentShader] = initDefaultFSContext();
		m_sources[ComputeShader]  = initDefaultCSContext();

		LayoutBindingTestResult result = ((*this).*((*it).test))();
		if (!result.testPassed())
		{
			log << tcu::TestLog::Section((*it).name, (*it).description);
			log << tcu::TestLog::Message << result.getReason() << tcu::TestLog::EndMessage;
			log << tcu::TestLog::EndSection;
		}
		if (!result.runForThisContext())
		{
			log << tcu::TestLog::Section((*it).name, (*it).description);
			log << tcu::TestLog::Message << "This test was not run for this context as it does not apply."
				<< tcu::TestLog::EndMessage;
			log << tcu::TestLog::EndSection;
		}
		passed &= result.testPassed();
	}

	// cleanup resources
	teardownTest();

	/*=========================================================================
	 TEST results
	 =========================================================================*/

	getContext().getTestContext().setTestResult(passed ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
												passed ? "Pass" : "Fail");

	return STOP;
}

/*=========================================================================
 // bind resource to specified binding point and program and
 // dispatch a computation, read back image at (0,0)
 =========================================================================*/
LayoutBindingTestResult LayoutBindingBaseCase::drawTestCompute(glw::GLint program, int binding)
{
	const glw::Functions& l_gl   = getContext().getRenderContext().getFunctions();
	bool				  passed = true;

	DE_TEST_ASSERT(getStage() == ComputeShader);

	l_gl.useProgram(program);

	deUint32 fb_or_ssb;

	if (getTestParameters().surface_type == Image)
	{
		bind(binding);

		glw::GLfloat buffer[4] = { 0.1f, 0.2f, 0.3f, 0.4f };
		l_gl.genBuffers(1, &fb_or_ssb);
		l_gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, fb_or_ssb);

		l_gl.bufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(glw::GLfloat), buffer, GL_DYNAMIC_COPY);

		l_gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fb_or_ssb);

		l_gl.dispatchCompute(1, 1, 1);
		l_gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		tcu::Vec4 pixel =
			*(tcu::Vec4*)l_gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(glw::GLfloat), GL_MAP_READ_BIT);
		l_gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);

		l_gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		l_gl.deleteBuffers(1, &fb_or_ssb);

		unbind(binding);

		tcu::Vec4 expected(0.0f, 1.0f, 0.0f, 1.0f);
		passed = (pixel == expected);
		if (!passed)
		{
			return LayoutBindingTestResult(passed, generateLog(String("drawTestCompute failed"), expected, pixel));
		}
	}
	else
	{
		deUint32 something = 0x01020304, tex;

		l_gl.genTextures(1, &tex);
		l_gl.bindTexture(GL_TEXTURE_2D, tex);
		l_gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		l_gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
		l_gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &something);

		bind(binding);

		l_gl.bindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

		l_gl.dispatchCompute(1, 1, 1);
		l_gl.memoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		l_gl.bindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

		l_gl.genFramebuffers(1, &fb_or_ssb);
		l_gl.bindFramebuffer(GL_FRAMEBUFFER, fb_or_ssb);
		l_gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		l_gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &something);

		l_gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		l_gl.deleteFramebuffers(1, &fb_or_ssb);
		l_gl.deleteTextures(1, &tex);

		unbind(binding);

		const deUint32 expected = 0xff00ff00;
		passed					= (expected == something);
		if (!passed)
		{
			return LayoutBindingTestResult(
				passed, generateLog(String("drawTestCompute failed"), tcu::RGBA(expected), tcu::RGBA(something)));
		}
	}

	return passed;
}

/*=========================================================================
 // bind resource to specified binding point and program and
 // return result of comparison of rendered pixel at (0,0) with expected
 =========================================================================*/
LayoutBindingTestResult LayoutBindingBaseCase::drawTest(glw::GLint program, int binding)
{
	const glw::Functions&	GL			  = getContext().getRenderContext().getFunctions();
	const tcu::RenderTarget& renderTarget = getContext().getRenderContext().getRenderTarget();
	glw::GLuint				 viewportW	= renderTarget.getWidth();
	glw::GLuint				 viewportH	= renderTarget.getHeight();
	tcu::Surface			 renderedFrame(viewportW, viewportH);
	bool					 passed = true;

	DE_TEST_ASSERT(getStage() != ComputeShader);

	GL.viewport(0, 0, viewportW, viewportH);
	GL.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GL.clear(GL_COLOR_BUFFER_BIT);

	static const float position[] = {
		-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
	};
	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	GL.useProgram(program);

	bind(binding);

	static const glu::VertexArrayBinding vertexArrays[] = {
		glu::va::Float("inPosition", 2, 4, 0, &position[0]),
	};
	glu::draw(getContext().getRenderContext(), program, DE_LENGTH_OF_ARRAY(vertexArrays), &vertexArrays[0],
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

	glu::readPixels(getContext().getRenderContext(), 0, 0, renderedFrame.getAccess());

	tcu::RGBA pixel = renderedFrame.getPixel(0, 0);

	passed = (pixel == tcu::RGBA(m_expectedColor));

	unbind(binding);

	if (!passed)
	{
		return LayoutBindingTestResult(passed,
									   generateLog(String("drawTest failed"), m_expectedColor, pixel.getPacked()));
	}

	return true;
}

//== verify that binding point is default w/o layout binding
LayoutBindingTestResult LayoutBindingBaseCase::binding_basic_default()
{
	bool passed = true;

	StringStream s;
	s << buildAccess(getDefaultUniformName()) << ";\n";
	setTemplateParam("UNIFORM_ACCESS", s.str());

	String decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(String()),
								   String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								   buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
	setTemplateParam("UNIFORM_DECL", decl);
	updateTemplate();

	LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
	passed &= program->compiledAndLinked();
	if (!passed)
	{
		return LayoutBindingTestResult(passed, program->getErrorLog());
	}

	StringVector  list;
	const String& u = buildBlockName(getDefaultUniformName());
	if (!u.empty())
		list.push_back(u + "_block");
	else
		list.push_back(getDefaultUniformName());

	StringIntMap bindingPoints = program->getBindingPoints(list);

	passed &= bindingPoints.size() == list.size() && (bindingPoints[u] == 0);
	if (!passed)
	{
		return LayoutBindingTestResult(passed,
									   generateLog(String("binding point did not match default"), bindingPoints[u], 0));
	}

	return true;
}

//== verify that binding point has specified value
LayoutBindingTestResult LayoutBindingBaseCase::binding_basic_explicit()
{
	bool passed = true;

	{
		StringStream s;
		s << buildAccess(getDefaultUniformName()) << ";\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
	}

	std::vector<int> bindings = makeSparseRange(maxBindings());
	for (std::vector<int>::iterator it = bindings.begin(); it < bindings.end(); it++)
	{
		int	binding = *it;
		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(binding),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}

		StringVector  list;
		const String& s = buildBlockName(getDefaultUniformName());
		if (!s.empty())
			list.push_back(s + "_block");
		else
			list.push_back(getDefaultUniformName());

		StringIntMap bindingPoints = program->getBindingPoints(list);
		passed &= bindingPoints.size() == list.size() && (binding == bindingPoints[list[0]]);
		if (!passed)
		{
			return LayoutBindingTestResult(
				passed, generateLog(String("binding point did not match default"), bindingPoints[list[0]], binding));
		}
	}
	return true;
}

//== verify that binding works with multiple samplers (same binding points)
LayoutBindingTestResult LayoutBindingBaseCase::binding_basic_multiple()
{
	bool passed = true;

	glw::GLint baseBindingPoint = maxBindings() - 1;

	String decl0 = buildUniformDecl(String(getTestParameters().keyword), buildLayout(baseBindingPoint),
									String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
									buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
	String decl1 = buildUniformDecl(String(getTestParameters().keyword), buildLayout(baseBindingPoint),
									String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName(1)),
									buildBlock(getDefaultUniformName(1)), getDefaultUniformName(1), String());
	setTemplateParam("UNIFORM_DECL", decl0 + decl1);

	StringStream s;
	s << buildAccess(getDefaultUniformName()) << " + " << buildAccess(getDefaultUniformName(1)) << ";\n";
	setTemplateParam("UNIFORM_ACCESS", s.str());
	updateTemplate();

	LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
	passed &= program->compiledAndLinked();
	if (!passed)
	{
		return LayoutBindingTestResult(passed, program->getErrorLog());
	}

	StringVector list;
	String		 u = buildBlockName(getDefaultUniformName());
	if (!u.empty())
		list.push_back(u + "_block");
	else
		list.push_back(getDefaultUniformName());

	u = buildBlockName(getDefaultUniformName(1));
	if (!u.empty())
		list.push_back(u + "_block");
	else
		list.push_back(getDefaultUniformName(1));

	StringIntMap bindingPoints = program->getBindingPoints(list);
	passed &= (baseBindingPoint == bindingPoints[list[0]]) && (baseBindingPoint == bindingPoints[list[1]]);
	if (!passed)
	{
		String err;
		err = generateLog(String("binding point did not match default"), bindingPoints[list[0]], baseBindingPoint);
		err += generateLog(String("binding point did not match default"), bindingPoints[list[1]], baseBindingPoint);
		return LayoutBindingTestResult(passed, err);
	}

	return true;
}

//== verify that binding point has specified value
LayoutBindingTestResult LayoutBindingBaseCase::binding_basic_render()
{
	bool passed = true;

	StringStream s;
	s << buildAccess(getDefaultUniformName()) << ";\n";
	setTemplateParam("UNIFORM_ACCESS", s.str());

	std::vector<int> bindings = makeSparseRange(maxBindings());
	for (std::vector<int>::iterator it = bindings.begin(); it < bindings.end(); it++)
	{
		int	binding = *it;
		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(binding),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}

		LayoutBindingTestResult drawTestResult = ((*this).*(m_drawTest))(program->getProgram(), binding);
		if (!drawTestResult.testPassed())
		{
			return LayoutBindingTestResult(drawTestResult.testPassed(), drawTestResult.getReason());
		}
	}
	return true;
}

LayoutBindingTestResult LayoutBindingBaseCase::binding_integer_constant()
{
	bool			 passed   = true;
	std::vector<int> integers = makeSparseRange(maxBindings(), 0);

	std::vector<IntegerConstant> integerConstants;
	for (int idx = 0; idx < IntegerConstant::last; idx++)
	{
		for (IntVector::iterator it = integers.begin(); it != integers.end(); it++)
		{
			integerConstants.push_back(IntegerConstant((IntegerConstant::Literals)idx, (*it)));
		}
	}

	//== verify that binding point can be set with integer constant
	for (std::vector<IntegerConstant>::iterator it = integerConstants.begin(); it != integerConstants.end(); it++)
	{
		String& intConst = (*it).asString;

		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(intConst),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);

		StringStream s;
		s << buildAccess(getDefaultUniformName()) << ";\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}

		StringVector  list;
		const String& u = buildBlockName(getDefaultUniformName());
		if (!u.empty())
			list.push_back(u + "_block");
		else
			list.push_back(getDefaultUniformName());

		StringIntMap bindingPoints = program->getBindingPoints(list);
		passed &= ((*it).asInt == bindingPoints[list[0]]);
		if (!passed)
		{
			return LayoutBindingTestResult(passed, generateLog(String("binding point did not match default"),
															   bindingPoints[list[0]], (*it).asInt));
		}
	}

	//== verify that binding point can be set with integer constant resulting from a preprocessor substitution
	for (std::vector<IntegerConstant>::iterator it = integerConstants.begin(); it != integerConstants.end(); it++)
	{
		String& intConst = (*it).asString;

		StringStream s;
		s << "#define INT_CONST " << intConst << std::endl;

		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(String("INT_CONST")),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", s.str() + decl);

		s.reset();
		s << buildAccess(getDefaultUniformName()) << ";\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}

		StringVector  list;
		const String& u = buildBlockName(getDefaultUniformName());
		if (!u.empty())
			list.push_back(u + "_block");
		else
			list.push_back(getDefaultUniformName());

		StringIntMap bindingPoints = program->getBindingPoints(list);
		passed &= ((*it).asInt == bindingPoints[list[0]]);
		if (!passed)
		{
			return LayoutBindingTestResult(passed, generateLog(String("binding point did not match default"),
															   bindingPoints[list[0]], (*it).asInt));
		}
	}

	return true;
}

//== test integer constant expressions
//== only for GL
LayoutBindingTestResult LayoutBindingBaseCase::binding_integer_constant_expression()
{
	bool passed = true;
	if (getGLSLVersion() == glu::GLSL_VERSION_310_ES)
		return LayoutBindingTestResult(passed, String(), true);

	return LayoutBindingTestResult(passed, String(), true);
}

//== test different sized arrays
LayoutBindingTestResult LayoutBindingBaseCase::binding_array_size(void)
{
	bool passed = true;

	std::vector<int> arraySizes = makeSparseRange(maxArraySize(), 1);
	for (std::vector<int>::iterator it = arraySizes.begin(); it < arraySizes.end(); it++)
	{
		int	arraySize = *it;
		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(maxBindings() - arraySize - 1),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), buildArray(arraySize));
		setTemplateParam("UNIFORM_DECL", decl);

		StringStream s;
		for (int idx = 0; idx < arraySize; idx++)
		{
			s << (idx ? " + " : "") << buildAccess(buildArrayAccess(0, idx));
		}
		s << ";\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}

		StringVector list;
		for (int idx = 0; idx < arraySize; idx++)
		{
			std::ostringstream texUnitStr;
			texUnitStr << getDefaultUniformName();
			const String& u = buildBlockName(getDefaultUniformName());
			if (!u.empty())
				texUnitStr << "_block";
			texUnitStr << buildArray(idx);
			list.push_back(texUnitStr.str());
		}

		StringIntMap bindingPoints = program->getBindingPoints(list);
		for (int idx = 0; idx < arraySize; idx++)
		{
			passed &= (((maxBindings() - arraySize - 1) + idx) == bindingPoints[list[idx]]);
			if (!passed)
			{
				return LayoutBindingTestResult(passed, generateLog(String("binding point did not match default"),
																   bindingPoints[list[idx]],
																   (maxBindings() - arraySize - 1) + idx));
			}
		}
	}

	return LayoutBindingTestResult(passed, String());
}

//== verify first element takes binding point specified in binding and
//== subsequent entries take the next consecutive units
LayoutBindingTestResult LayoutBindingBaseCase::binding_array_implicit(void)
{
	bool passed = true;

	std::vector<int> bindings = makeSparseRange(maxBindings(), 0);
	for (std::vector<int>::iterator it = bindings.begin(); it < bindings.end(); it++)
	{
		int baseBindingPoint = *it;
		int arraySize		 = std::min(maxBindings() - baseBindingPoint, 4);

		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(baseBindingPoint),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), buildArray(arraySize));
		setTemplateParam("UNIFORM_DECL", decl);

		StringStream s;
		for (int idx = 0; idx < arraySize; idx++)
		{
			s << (idx ? " + " : "") << buildAccess(buildArrayAccess(0, idx));
		}
		s << ";\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}
		StringVector list;
		for (int idx = 0; idx < arraySize; idx++)
		{
			std::ostringstream texUnitStr;
			texUnitStr << getDefaultUniformName();
			const String& u = buildBlockName(getDefaultUniformName());
			if (!u.empty())
				texUnitStr << "_block";
			texUnitStr << buildArray(idx);
			list.push_back(texUnitStr.str());
		}

		StringIntMap bindingPoints = program->getBindingPoints(list);
		for (int idx = 0; idx < arraySize; idx++)
		{
			passed &= ((baseBindingPoint + idx) == bindingPoints[list[idx]]);
			if (!passed)
			{
				return LayoutBindingTestResult(passed, generateLog(String("binding point did not match default"),
																   bindingPoints[list[idx]], (baseBindingPoint + idx)));
			}
		}
	}
	return LayoutBindingTestResult(passed, String());
}

//== multiple arrays :verify first element takes binding point specified in binding and
//== subsequent entries take the next consecutive units
LayoutBindingTestResult LayoutBindingBaseCase::binding_array_multiple(void)
{
	bool passed = true;

	// two arrays, limit max. binding to one
	std::vector<int> bindings = makeSparseRange(maxBindings() - 2, 0);
	for (std::vector<int>::iterator it = bindings.begin(); it < bindings.end(); it++)
	{
		int baseBindingPoint = *it;

		// total distance from current binding point to end of binding range
		// split over two arrays, making sure that the array sizes don't
		// exceed max. array sizes per stage
		int arraySize = (maxBindings() - baseBindingPoint - 1) / 2;
		arraySize	 = std::min(arraySize, maxArraySize() / 2);

		StringStream s;
		String		 decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(baseBindingPoint),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), buildArray(arraySize));
		String another_decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(baseBindingPoint),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName(1)),
							 buildBlock(getDefaultUniformName(1)), getDefaultUniformName(1), buildArray(arraySize));
		setTemplateParam("UNIFORM_DECL", decl + another_decl);

		s.reset();
		for (int uniform = 0; uniform < 2; uniform++)
		{
			for (int idx = 0; idx < arraySize; idx++)
			{
				s << ((idx | uniform) ? " + " : "") << buildAccess(buildArrayAccess(uniform, idx));
			}
		}
		s << ";\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog());
		}

		StringVector list;
		for (int uniform = 0; uniform < 2; uniform++)
		{
			list.clear();
			for (int idx = 0; idx < arraySize; idx++)
			{
				std::ostringstream texUnitStr;
				texUnitStr << getDefaultUniformName(uniform);
				const String& u = buildBlockName(getDefaultUniformName(uniform));
				if (!u.empty())
					texUnitStr << "_block";
				texUnitStr << buildArray(idx);
				list.push_back(texUnitStr.str());
			}

			StringIntMap bindingPoints = program->getBindingPoints(list);
			for (int idx = 0; idx < arraySize; idx++)
			{
				passed &= ((baseBindingPoint + idx) == bindingPoints[list[idx]]);
				if (!passed)
				{
					return LayoutBindingTestResult(passed,
												   generateLog(String("binding point did not match default"),
															   bindingPoints[list[idx]], (baseBindingPoint + idx)));
				}
			}
		}
	}
	return LayoutBindingTestResult(passed, String());
}

//== verify that explicit binding point can be changed via API
LayoutBindingTestResult LayoutBindingBaseCase::binding_api_update(void)
{
	bool passed = true;

	String decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(1),
								   String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								   buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
	setTemplateParam("UNIFORM_DECL", decl);

	StringStream s;
	s << buildAccess(getDefaultUniformName()) << ";\n";
	setTemplateParam("UNIFORM_ACCESS", s.str());
	updateTemplate();

	LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
	passed &= program->compiledAndLinked();
	if (!passed)
	{
		return LayoutBindingTestResult(passed, program->getErrorLog());
	}

	StringVector  list;
	const String& u = buildBlockName(getDefaultUniformName());
	if (!u.empty())
		list.push_back(u + "_block");
	else
		list.push_back(getDefaultUniformName());

	StringIntMap bindingPoints = program->getBindingPoints(list);

	gl().useProgram(program->getProgram());
	program->setBindingPoints(list, maxBindings() - 1);
	gl().useProgram(0);

	bindingPoints = program->getBindingPoints(list);

	passed &= bindingPoints[list[0]] == (maxBindings() - 1);
	if (!passed)
	{
		return LayoutBindingTestResult(passed, generateLog(String("binding point did not match default"),
														   bindingPoints[list[0]], maxBindings() - 1));
	}

	return LayoutBindingTestResult(passed, String());
}

LayoutBindingTestResult LayoutBindingBaseCase::binding_compilation_errors(void)
{
	bool   passed = true;
	String decl;

	// verify "uniform float var;" doesn't compile
	{
		StringStream s;
		s << getTestParameters().vector_type << "(0.0);";

		setTemplateParam("UNIFORM_ACCESS", s.str());
		s.reset();
		s << "layout(binding=0) "
		  << "uniform float tex0;";
		setTemplateParam("UNIFORM_DECL", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= !program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog(true));
		}
	}

	// verify that non-constant integer expression in binding fails
	{
		decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(String("0.0")),
								String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= !program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog(true));
		}
	}

	{
		decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(String("-1")),
								String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= !program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog(true));
		}
	}
	{
		decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(maxBindings()),
								String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= !program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog(true));
		}
	}
	{
		decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(maxBindings()),
								String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam("UNIFORM_DECL", decl);

		StringStream s;
		s << "vec4(0.0);\n";
		setTemplateParam("UNIFORM_ACCESS", s.str());
		updateTemplate();

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed &= !program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog(true));
		}
	}
	return LayoutBindingTestResult(passed, String());
}

LayoutBindingTestResult LayoutBindingBaseCase::binding_link_errors(void)
{
	bool passed = true;

	// same sampler with different binding in two compilation units
	if (isStage(VertexShader))
	{
		String decl =
			buildUniformDecl(String(getTestParameters().keyword), buildLayout(1),
							 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
							 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam(VertexShader, "UNIFORM_DECL", decl);

		setTemplateParam(VertexShader, "OUT_ASSIGNMENT", String("fragColor ="));

		StringStream s;
		s << buildAccess(getDefaultUniformName()) << ";\n";
		setTemplateParam(VertexShader, "UNIFORM_ACCESS", s.str());
		updateTemplate(VertexShader);

		decl = buildUniformDecl(String(getTestParameters().keyword), buildLayout(3),
								String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
		setTemplateParam(FragmentShader, "UNIFORM_DECL", decl);

		s.reset();
		s << "fragColor + " << buildAccess(getDefaultUniformName()) << ";\n";
		setTemplateParam(FragmentShader, "UNIFORM_ACCESS", s.str());
		updateTemplate(FragmentShader);

		LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
		passed = !program->compiledAndLinked();
		if (!passed)
		{
			return LayoutBindingTestResult(passed, program->getErrorLog(true));
		}
	}

	return LayoutBindingTestResult(passed, String());
}

// this subtest is generically empty. Overwritten in test cases as needed.
LayoutBindingTestResult LayoutBindingBaseCase::binding_examples(void)
{
	return true;
}

// just for image and atomic counter cases
LayoutBindingTestResult LayoutBindingBaseCase::binding_mixed_order(void)
{
	return true;
}

//=========================================================================
// test case Sampler layout binding
//=========================================================================
class SamplerLayoutBindingCase : public LayoutBindingBaseCase

{
public:
	SamplerLayoutBindingCase(Context& context, const char* name, const char* description, StageType stage,
							 LayoutBindingParameters& samplerType, glu::GLSLVersion glslVersion);
	~SamplerLayoutBindingCase(void);

private:
	/*virtual*/
	LayoutBindingProgram* createProgram()
	{
		return new LayoutBindingProgram(*this);
	}

	/*virtual*/
	String getDefaultUniformName(int idx = 0)
	{
		StringStream s;

		s << "sampler" << idx;
		return s.str();
	}

	/*virtual*/
	String buildLayout(const String& binding)
	{
		std::ostringstream s;
		if (!binding.empty())
			s << "layout(binding=" << binding << ") ";
		return s.str();
	}

	virtual int maxBindings()
	{
		int units = 0;
		gl().getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &units);
		return units;
	}

	// return max. array size allowed
	virtual int maxArraySize()
	{
		int units = 0;

		switch (getStage())
		{
		case VertexShader:
			gl().getIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &units);
			break;
		case FragmentShader:
			gl().getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
			break;
		case ComputeShader:
			gl().getIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &units);
			break;
		default:
			DE_ASSERT(0);
			break;
		}

		return units;
	}
};

SamplerLayoutBindingCase::SamplerLayoutBindingCase(Context& context, const char* name, const char* description,
												   StageType stage, LayoutBindingParameters& samplerType,
												   glu::GLSLVersion glslVersion)
	: LayoutBindingBaseCase(context, name, description, stage, samplerType, glslVersion)
{
}

SamplerLayoutBindingCase::~SamplerLayoutBindingCase(void)
{
}

//=========================================================================
// test case Image layout binding
//=========================================================================
class ImageLayoutBindingCase : public LayoutBindingBaseCase

{
public:
	ImageLayoutBindingCase(Context& context, const char* name, const char* description, StageType stage,
						   LayoutBindingParameters& samplerType, glu::GLSLVersion glslVersion);
	~ImageLayoutBindingCase(void);

private:
	class ImageLayoutBindingProgram : public LayoutBindingProgram
	{
	public:
		ImageLayoutBindingProgram(IProgramContextSupplier& contextSupplier) : LayoutBindingProgram(contextSupplier)
		{
		}
	};

private:
	// IProgramContextSupplier
	/*virtual*/
	LayoutBindingProgram* createProgram()
	{
		return new ImageLayoutBindingProgram(*this);
	}

private:
	/*virtual*/
	String getDefaultUniformName(int idx = 0)
	{
		StringStream s;
		s << "image" << idx;
		return s.str();
	}

	/*virtual*/
	String buildLayout(const String& binding)
	{
		std::ostringstream s;
		if (!binding.empty())
			s << "layout(binding=" << binding << ", rgba8) readonly ";
		else
			s << "layout(rgba8) readonly ";
		return s.str();
	}

	/*virtual*/
	int maxBindings()
	{
		int units = 0;
		gl().getIntegerv(GL_MAX_IMAGE_UNITS, &units);
		return units;
	}

	/*virtual*/
	int maxArraySize()
	{
		int units = 0;
		switch (getStage())
		{
		case VertexShader:
			gl().getIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &units);
			break;
		case FragmentShader:
			gl().getIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &units);
			break;
		case ComputeShader:
			gl().getIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &units);
			break;
		default:
			DE_ASSERT(0);
			break;
		}
		return units;
	}

private:
	//virtual LayoutBindingTestResult        binding_basic_default               (void);
	//virtual LayoutBindingTestResult        binding_basic_explicit              (void);
	//virtual LayoutBindingTestResult        binding_basic_multiple              (void);
	//virtual LayoutBindingTestResult        binding_basic_render                (void);
	//virtual LayoutBindingTestResult        binding_integer_constant            (void);
	//virtual LayoutBindingTestResult        binding_integer_constant_expression (void);
	//virtual LayoutBindingTestResult        binding_array_size                  (void);
	//virtual LayoutBindingTestResult        binding_array_implicit              (void);
	//virtual LayoutBindingTestResult        binding_array_multiple              (void);
	/*virtual*/
	LayoutBindingTestResult binding_api_update(void)
	{
		// only for GL
		if (getGLSLVersion() == glu::GLSL_VERSION_310_ES)
			return LayoutBindingTestResult(true, String(), true);

		return LayoutBindingBaseCase::binding_api_update();
	}
	//virtual LayoutBindingTestResult        binding_compilation_errors          (void);
	//virtual LayoutBindingTestResult        binding_link_errors                 (void);
	//virtual LayoutBindingTestResult        binding_link_examples               (void);
	/*virtual*/
	LayoutBindingTestResult binding_mixed_order(void)
	{
		bool passed = true;

		{
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			String decl =
				buildUniformDecl(String(getTestParameters().keyword), String("layout(binding=0, rgba8) readonly"),
								 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
			setTemplateParam("UNIFORM_DECL", decl);
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}
		}
		{
			String decl =
				buildUniformDecl(String(getTestParameters().keyword), String("layout(r32f, binding=0) readonly"),
								 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
			setTemplateParam("UNIFORM_DECL", decl);
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}
		}

		return true;
	}
};

ImageLayoutBindingCase::ImageLayoutBindingCase(Context& context, const char* name, const char* description,
											   StageType stage, LayoutBindingParameters& samplerType,
											   glu::GLSLVersion glslVersion)
	: LayoutBindingBaseCase(context, name, description, stage, samplerType, glslVersion)
{
}

ImageLayoutBindingCase::~ImageLayoutBindingCase(void)
{
}

//=========================================================================
// test case Atomic counter binding
//=========================================================================
class AtomicCounterLayoutBindingCase : public LayoutBindingBaseCase

{
public:
	AtomicCounterLayoutBindingCase(Context& context, const char* name, const char* description, StageType stage,
								   LayoutBindingParameters& samplerType, glu::GLSLVersion glslVersion);
	~AtomicCounterLayoutBindingCase(void)
	{
	}

private:
	class AtomicCounterLayoutBindingProgram : public LayoutBindingProgram
	{
	public:
		AtomicCounterLayoutBindingProgram(IProgramContextSupplier& contextSupplier)
			: LayoutBindingProgram(contextSupplier)
		{
		}

	private:
		/*virtual*/
		StringIntMap getBindingPoints(StringVector args) const
		{
			StringIntMap bindingPoints;

			for (StringVector::iterator it = args.begin(); it != args.end(); it++)
			{
				glw::GLuint idx = gl().getProgramResourceIndex(getProgram(), GL_UNIFORM, (*it).c_str());
				if (idx != GL_INVALID_INDEX)
				{
					glw::GLenum param					  = GL_ATOMIC_COUNTER_BUFFER_INDEX;
					glw::GLint  atomic_counter_buffer_idx = -1;
					gl().getProgramResourceiv(getProgram(), GL_UNIFORM, idx, 1, &param, 1, NULL,
											  &atomic_counter_buffer_idx);
					bool hasNoError = (GL_NO_ERROR == gl().getError()) && (-1 != atomic_counter_buffer_idx);
					if (!hasNoError)
						continue;

					param			 = GL_BUFFER_BINDING;
					glw::GLint value = -1;
					gl().getProgramResourceiv(getProgram(), GL_ATOMIC_COUNTER_BUFFER, atomic_counter_buffer_idx, 1,
											  &param, 1, NULL, &value);
					hasNoError = (GL_NO_ERROR == gl().getError()) && (-1 != value);
					if (!hasNoError)
						continue;

					bindingPoints[*it] = value;
				}
			}
			return bindingPoints;
		}

		/*virtual*/
		StringIntMap getOffsets(StringVector args) const
		{
			StringIntMap bindingPoints;

			for (StringVector::iterator it = args.begin(); it != args.end(); it++)
			{
				glw::GLuint idx = gl().getProgramResourceIndex(getProgram(), GL_UNIFORM, (*it).c_str());
				if (idx != GL_INVALID_INDEX)
				{
					glw::GLenum param = GL_OFFSET;
					glw::GLint  value = -1;
					gl().getProgramResourceiv(getProgram(), GL_UNIFORM, idx, 1, &param, 1, NULL, &value);
					bool hasNoError = (GL_NO_ERROR == gl().getError());
					if (hasNoError)
					{
						bindingPoints[*it] = value;
					}
				}
			}
			return bindingPoints;
		}
	};

private:
	// IProgramContextSupplier
	/*virtual*/
	LayoutBindingProgram* createProgram()
	{
		return new AtomicCounterLayoutBindingProgram(*this);
	}

private:
	/*virtual*/
	String getDefaultUniformName(int idx = 0)
	{
		StringStream s;
		s << "atomic" << idx;
		return s.str();
	}

	/*virtual*/
	String buildAccess(const String& var)
	{
		std::ostringstream s;
		s << "vec4(float(atomicCounter(" << var << ")), 1.0, 0.0, 1.0)";
		return s.str();
	}

	int maxBindings()
	{
		int units = 0;
		gl().getIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &units);
		return units;
	}

	// return max. array size allowed
	int maxArraySize()
	{
		int units = 0;
		switch (getStage())
		{
		case FragmentShader:
			gl().getIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &units);
			break;
		case VertexShader:
			gl().getIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &units);
			break;
		case ComputeShader:
			gl().getIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &units);
			break;
		default:
			DE_ASSERT(0);
			break;
		}
		return units;
	}

	//=========================================================================
	// sub-tests overrides
	//=========================================================================
private:
	LayoutBindingTestResult binding_basic_default(void)
	{
		return true;
	}
	//virtual LayoutBindingTestResult binding_basic_explicit          (void);
	//virtual LayoutBindingTestResult binding_basic_multiple          (void);
	LayoutBindingTestResult binding_basic_render(void)
	{
		return true;
	}
	//virtual LayoutBindingTestResult binding_integer_constant        (void);
	/*virtual*/
	LayoutBindingTestResult binding_array_size(void)
	{
		bool passed = true;

		//== test different sized arrays
		std::vector<int> arraySizes = makeSparseRange(maxArraySize(), 1);
		for (std::vector<int>::iterator it = arraySizes.begin(); it < arraySizes.end(); it++)
		{
			int			 arraySize = *it;
			StringStream s;
			s << "[" << arraySize << "]";
			String decl =
				buildUniformDecl(String(getTestParameters().keyword), buildLayout(0),
								 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								 buildBlock(getDefaultUniformName()), getDefaultUniformName(), buildArray(arraySize));
			setTemplateParam("UNIFORM_DECL", decl);

			s.reset();
			// build a function that accesses the whole array
			s << "float accumulate(void)\n";
			s << "{\n";
			s << "  float acc = 0.0;\n";
			s << "  for(int i=0; i < " << arraySize << " ; i++)\n";
			s << "    acc = float(atomicCounter(" << getDefaultUniformName() << "[i]));\n";
			s << "  return acc;\n";
			s << "}\n";

			setTemplateParam("OPTIONAL_FUNCTION_BLOCK", s.str());

			s.reset();
			s << "vec4(accumulate(), 1.0, 0.0, 1.0);\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());
			updateTemplate();

			setTemplateParam("OPTIONAL_FUNCTION_BLOCK", String());

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}

			StringVector	   list;
			std::ostringstream texUnitStr;
			texUnitStr << getDefaultUniformName() << "[0]";
			list.push_back(texUnitStr.str());

			StringIntMap bindingPoints = program->getBindingPoints(list);
			passed &= (0 == bindingPoints[list[0]]);
			if (!passed)
			{
				return LayoutBindingTestResult(
					passed, generateLog(String("binding point did not match default"), bindingPoints[list[0]], 1));
			}
		}

		return LayoutBindingTestResult(passed, String());
	}
	LayoutBindingTestResult binding_array_implicit(void)
	{
		return true;
	}
	LayoutBindingTestResult binding_array_multiple(void)
	{
		return true;
	}
	LayoutBindingTestResult binding_api_update(void)
	{
		return true;
	}
	//virtual LayoutBindingTestResult binding_compilation_errors      (void);
	//virtual LayoutBindingTestResult binding_link_errors             (void);

	LayoutBindingTestResult binding_examples_many_bindings(void)
	{
		int max_bindings = 0;

		if (isStage(VertexShader))
		{
			gl().getIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &max_bindings);
		}
		else if (isStage(FragmentShader))
		{
			gl().getIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &max_bindings);
		}
		else
		{
			gl().getIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, &max_bindings);
		}

		// example 1 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=2, offset=4) uniform atomic_uint;\n";
			s << "layout(binding=2) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}

			StringVector list;
			list.push_back(getDefaultUniformName());

			StringIntMap offsets = program->getOffsets(list);
			passed &= (4 == offsets[list[0]]);
			if (!passed)
			{
				return LayoutBindingTestResult(
					passed, generateLog(String("offset did not match requested"), offsets[list[0]], 1));
			}
		}

		// example 2 in atomic counter CTS spec ac-binding-examples
		if (max_bindings >= 2)
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			s << "+" << buildAccess(getDefaultUniformName(1)) << ";\n";
			s << "+" << buildAccess(getDefaultUniformName(2)) << ";\n";
			s << "+" << buildAccess(getDefaultUniformName(3)) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=3, offset=4) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			s << "layout(binding=2) uniform atomic_uint " << getDefaultUniformName(1) << ";\n";
			s << "layout(binding=3) uniform atomic_uint " << getDefaultUniformName(2) << ";\n";
			s << "layout(binding=2) uniform atomic_uint " << getDefaultUniformName(3) << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}

			StringVector list;
			list.push_back(getDefaultUniformName());
			list.push_back(getDefaultUniformName(1));
			list.push_back(getDefaultUniformName(2));
			list.push_back(getDefaultUniformName(3));

			StringIntMap offsets = program->getOffsets(list);
			IntVector	expected;
			expected.insert(expected.end(), 4);
			expected.insert(expected.end(), 0);
			expected.insert(expected.end(), 8);
			expected.insert(expected.end(), 4);
			for (unsigned int idx = 0; idx < list.size(); idx++)
			{
				passed &= (expected[idx] == offsets[list[idx]]);
				if (!passed)
				{
					return LayoutBindingTestResult(
						passed, generateLog(String("offset of") + String(list[idx]) + String("did not match requested"),
											offsets[list[idx]], 4));
				}
			}
		}

		// example 3 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=2, offset=4) uniform atomic_uint;\n";
			s << "layout(offset=8) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (passed)
			{
				return LayoutBindingTestResult(!passed, String("should not compile"));
			}
		}

		// example 4 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(offset=4) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (passed)
			{
				return LayoutBindingTestResult(!passed, String("should not compile"));
			}
		}

		// example 5 in atomic counter CTS spec ac-binding-examples
		// first check working example, then amend it with an error
		if (max_bindings >= 2)
		{
			for (int pass = 0; pass < 2; pass++)
			{
				bool passed = true;

				StringStream s;
				s << buildAccess(getDefaultUniformName()) << ";\n";
				s << "+" << buildAccess(getDefaultUniformName(1)) << ";\n";
				if (pass)
					s << "+" << buildAccess(getDefaultUniformName(2)) << ";\n";
				setTemplateParam("UNIFORM_ACCESS", s.str());

				s.reset();
				s << "layout(binding=1, offset=0) uniform atomic_uint " << getDefaultUniformName() << ";\n";
				s << "layout(binding=2, offset=0) uniform atomic_uint " << getDefaultUniformName(1) << ";\n";
				if (pass)
					s << "layout(binding=1, offset=0) uniform atomic_uint " << getDefaultUniformName(2) << ";\n";
				setTemplateParam("UNIFORM_DECL", s.str());
				updateTemplate();

				LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
				passed &= program->compiledAndLinked();
				if (pass != 0)
				{
					if (passed)
					{
						return LayoutBindingTestResult(passed, program->getErrorLog());
					}
				}
				else
				{
					if (!passed)
					{
						return LayoutBindingTestResult(passed, String("should not compile"));
					}
				}
			}
		}

		// example 6 in atomic counter CTS spec ac-binding-examples
		// first check working example, then amend it with an error
		if (max_bindings >= 2)
		{
			for (int pass = 0; pass < 2; pass++)
			{
				bool passed = true;

				StringStream s;
				s << buildAccess(getDefaultUniformName()) << ";\n";
				s << "+" << buildAccess(getDefaultUniformName(1)) << ";\n";
				if (pass)
					s << "+" << buildAccess(getDefaultUniformName(2)) << ";\n";
				setTemplateParam("UNIFORM_ACCESS", s.str());

				s.reset();
				s << "layout(binding=1, offset=0) uniform atomic_uint " << getDefaultUniformName() << ";\n";
				s << "layout(binding=2, offset=0) uniform atomic_uint " << getDefaultUniformName(1) << ";\n";
				if (pass)
					s << "layout(binding=1, offset=2) uniform atomic_uint " << getDefaultUniformName(2) << ";\n";
				setTemplateParam("UNIFORM_DECL", s.str());
				updateTemplate();

				LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
				passed &= program->compiledAndLinked();
				if (pass != 0)
				{
					if (passed)
					{
						return LayoutBindingTestResult(passed, program->getErrorLog());
					}
				}
				else
				{
					if (!passed)
					{
						return LayoutBindingTestResult(passed, String("should not compile"));
					}
				}
			}
		}

		return true;
	}

	LayoutBindingTestResult binding_examples_one_binding(void)
	{
		// example 1 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=0, offset=4) uniform atomic_uint;\n";
			s << "layout(binding=0) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}

			StringVector list;
			list.push_back(getDefaultUniformName());

			StringIntMap offsets = program->getOffsets(list);
			passed &= (4 == offsets[list[0]]);
			if (!passed)
			{
				return LayoutBindingTestResult(
					passed, generateLog(String("offset did not match requested"), offsets[list[0]], 1));
			}
		}

		// example 2 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			s << "+" << buildAccess(getDefaultUniformName(1)) << ";\n";
			s << "+" << buildAccess(getDefaultUniformName(2)) << ";\n";
			s << "+" << buildAccess(getDefaultUniformName(3)) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=0, offset=4) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			s << "layout(binding=0) uniform atomic_uint " << getDefaultUniformName(1) << ";\n";
			s << "layout(binding=0) uniform atomic_uint " << getDefaultUniformName(2) << ";\n";
			s << "layout(binding=0) uniform atomic_uint " << getDefaultUniformName(3) << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}

			StringVector list;
			list.push_back(getDefaultUniformName());
			list.push_back(getDefaultUniformName(1));
			list.push_back(getDefaultUniformName(2));
			list.push_back(getDefaultUniformName(3));

			StringIntMap offsets = program->getOffsets(list);
			IntVector	expected;
			expected.insert(expected.end(), 4);
			expected.insert(expected.end(), 8);
			expected.insert(expected.end(), 12);
			expected.insert(expected.end(), 16);
			for (unsigned int idx = 0; idx < list.size(); idx++)
			{
				passed &= (expected[idx] == offsets[list[idx]]);
				if (!passed)
				{
					return LayoutBindingTestResult(
						passed, generateLog(String("offset of") + String(list[idx]) + String("did not match requested"),
											offsets[list[idx]], expected[idx]));
				}
			}
		}

		// example 3 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=0, offset=4) uniform atomic_uint;\n";
			s << "layout(offset=8) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (passed)
			{
				return LayoutBindingTestResult(!passed, String("should not compile"));
			}
		}

		// example 4 in atomic counter CTS spec ac-binding-examples
		{
			bool		 passed = true;
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(offset=4) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (passed)
			{
				return LayoutBindingTestResult(!passed, String("should not compile"));
			}
		}

		// example 5 in atomic counter CTS spec ac-binding-examples
		// first check working example, then amend it with an error
		for (int pass = 0; pass < 2; pass++)
		{
			bool passed = true;

			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			if (pass)
				s << "+" << buildAccess(getDefaultUniformName(1)) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=0, offset=0) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			if (pass)
				s << "layout(binding=0, offset=0) uniform atomic_uint " << getDefaultUniformName(1) << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (pass != 0)
			{
				if (passed)
				{
					return LayoutBindingTestResult(passed, program->getErrorLog());
				}
			}
			else
			{
				if (!passed)
				{
					return LayoutBindingTestResult(passed, String("should not compile"));
				}
			}
		}

		// example 6 in atomic counter CTS spec ac-binding-examples
		// first check working example, then amend it with an error
		for (int pass = 0; pass < 2; pass++)
		{
			bool passed = true;

			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			if (pass)
				s << "+" << buildAccess(getDefaultUniformName(1)) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			s.reset();
			s << "layout(binding=0, offset=0) uniform atomic_uint " << getDefaultUniformName() << ";\n";
			if (pass)
				s << "layout(binding=0, offset=2) uniform atomic_uint " << getDefaultUniformName(1) << ";\n";
			setTemplateParam("UNIFORM_DECL", s.str());
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (pass != 0)
			{
				if (passed)
				{
					return LayoutBindingTestResult(passed, program->getErrorLog());
				}
			}
			else
			{
				if (!passed)
				{
					return LayoutBindingTestResult(passed, String("should not compile"));
				}
			}
		}

		return true;
	}

	/*virtual*/
	LayoutBindingTestResult binding_examples(void)
	{
		if (maxBindings() >= 4)
		{
			return binding_examples_many_bindings();
		}
		else
		{
			return binding_examples_one_binding();
		}
	}

	/*virtual*/
	LayoutBindingTestResult binding_mixed_order(void)
	{
		bool passed = true;

		{
			StringStream s;
			s << buildAccess(getDefaultUniformName()) << ";\n";
			setTemplateParam("UNIFORM_ACCESS", s.str());

			String decl =
				buildUniformDecl(String(getTestParameters().keyword), String("layout(binding=0, offset=0)"),
								 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
			setTemplateParam("UNIFORM_DECL", decl);
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}
		}
		{
			String decl =
				buildUniformDecl(String(getTestParameters().keyword), String("layout(offset=0, binding=0)"),
								 String(getTestParameters().uniform_type), buildBlockName(getDefaultUniformName()),
								 buildBlock(getDefaultUniformName()), getDefaultUniformName(), String());
			setTemplateParam("UNIFORM_DECL", decl);
			updateTemplate();

			LayoutBindingProgram::LayoutBindingProgramAutoPtr program(*this);
			passed &= program->compiledAndLinked();
			if (!passed)
			{
				return LayoutBindingTestResult(passed, program->getErrorLog());
			}
		}

		return true;
	}
};

AtomicCounterLayoutBindingCase::AtomicCounterLayoutBindingCase(Context& context, const char* name,
															   const char* description, StageType stage,
															   LayoutBindingParameters& samplerType,
															   glu::GLSLVersion			glslVersion)
	: LayoutBindingBaseCase(context, name, description, stage, samplerType, glslVersion)
{
}

//=========================================================================
// test case Uniform blocks binding
//=========================================================================
class UniformBlocksLayoutBindingCase : public LayoutBindingBaseCase

{
public:
	UniformBlocksLayoutBindingCase(Context& context, const char* name, const char* description, StageType stage,
								   LayoutBindingParameters& samplerType, glu::GLSLVersion glslVersion);
	~UniformBlocksLayoutBindingCase(void);

private:
	class UniformBlocksLayoutBindingProgram : public LayoutBindingProgram
	{
	public:
		UniformBlocksLayoutBindingProgram(IProgramContextSupplier& contextSupplier)
			: LayoutBindingProgram(contextSupplier)
		{
		}

		~UniformBlocksLayoutBindingProgram()
		{
		}

	private:
		/*virtual*/
		StringIntMap getBindingPoints(StringVector args) const
		{
			StringIntMap bindingPoints;

			for (StringVector::iterator it = args.begin(); it != args.end(); it++)
			{
				glw::GLuint idx = gl().getProgramResourceIndex(getProgram(), GL_UNIFORM_BLOCK, (*it).c_str());
				if (idx != glw::GLuint(-1))
				{
					glw::GLenum param = GL_BUFFER_BINDING;
					glw::GLint  value = -1;
					gl().getProgramResourceiv(getProgram(), GL_UNIFORM_BLOCK, idx, 1, &param, 1, NULL, &value);
					bool hasNoError = (GL_NO_ERROR == gl().getError());
					if (hasNoError)
					{
						bindingPoints[*it] = value;
					}
				}
			}

			return bindingPoints;
		}

		/*virtual*/
		bool setBindingPoints(StringVector list, glw::GLint bindingPoint) const
		{
			bool bNoError = true;

			for (StringVector::iterator it = list.begin(); it != list.end(); it++)
			{
				glw::GLuint blockIndex = gl().getUniformBlockIndex(getProgram(), (*it).c_str());
				if (blockIndex == GL_INVALID_INDEX)

				{
					return false;
				}
				gl().uniformBlockBinding(getProgram(), blockIndex, bindingPoint);
			}
			return bNoError;
		}
	};

private:
	// IProgramContextSupplier
	/*virtual*/
	LayoutBindingProgram* createProgram()
	{
		return new UniformBlocksLayoutBindingProgram(*this);
	}

private:
	/*virtual*/
	String buildBlockName(const String& name)
	{

		return name;
	}

	/*virtual*/
	String buildBlock(const String& name, const String& type)
	{

		std::ostringstream s;
		s << "{";
		s << type << " " << name << "_a; ";
		s << type << " " << name << "_b; ";
		s << "}";
		return s.str();
	}

	/*virtual*/
	String buildAccess(const String& var)
	{
		std::ostringstream s;
		s << "vec4(0.0, " << var << "_a, 0.0, 1.0) + vec4(0.0, " << var << "_b, 0.0, 1.0)";
		return s.str();
	}

	/*virtual*/
	String buildLayout(const String& binding)
	{
		std::ostringstream s;
		if (!binding.empty())
			s << "layout(binding=" << binding << ", std140) ";
		else
			s << "layout(std140) ";
		return s.str();
	}

	/*virtual*/
	int maxBindings()
	{
		int units = 0;
		gl().getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &units);
		return units;
	}

	// return max. array size allowed
	/*virtual*/
	int maxArraySize()
	{
		int units = 0;
		switch (getStage())
		{
		case FragmentShader:
			gl().getIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &units);
			break;
		case VertexShader:
			gl().getIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &units);
			break;
		case ComputeShader:
			gl().getIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &units);
			break;
		default:
			DE_ASSERT(0);
			break;
		}
		return units;
	}

	/*virtual*/
	void bind(int binding)
	{
		gl().bindBufferBase(GL_UNIFORM_BUFFER, binding, m_buffername);
	}

	/*virtual*/
	void unbind(int binding)
	{
		gl().bindBufferBase(GL_UNIFORM_BUFFER, binding, 0);
	}

	/*virtual*/
	void setupTest(void)
	{
		const float f[2] = { 0.25f, 0.75f };
		gl().genBuffers(1, &m_buffername);
		gl().bindBuffer(GL_UNIFORM_BUFFER, m_buffername);
		gl().bufferData(GL_UNIFORM_BUFFER, sizeof(f), f, GL_STATIC_DRAW);
	}

	/*virtual*/
	void teardownTest(void)
	{
		gl().bindBuffer(GL_UNIFORM_BUFFER, 0);
		gl().deleteBuffers(1, &m_buffername);
	}

private:
	glw::GLuint m_buffername;
};

UniformBlocksLayoutBindingCase::UniformBlocksLayoutBindingCase(Context& context, const char* name,
															   const char* description, StageType stage,
															   LayoutBindingParameters& samplerType,
															   glu::GLSLVersion			glslVersion)
	: LayoutBindingBaseCase(context, name, description, stage, samplerType, glslVersion), m_buffername(0)
{
}

UniformBlocksLayoutBindingCase::~UniformBlocksLayoutBindingCase()
{
}

//*****************************************************************************

//=========================================================================
// test case Shader storage buffer binding
//=========================================================================
class ShaderStorageBufferLayoutBindingCase : public LayoutBindingBaseCase
{
public:
	ShaderStorageBufferLayoutBindingCase(Context& context, const char* name, const char* description, StageType stage,
										 LayoutBindingParameters& samplerType, glu::GLSLVersion glslVersion);
	~ShaderStorageBufferLayoutBindingCase(void);

private:
	class ShaderStorageBufferLayoutBindingProgram : public LayoutBindingProgram
	{
	public:
		ShaderStorageBufferLayoutBindingProgram(IProgramContextSupplier& contextSupplier)
			: LayoutBindingProgram(contextSupplier)
		{
		}

		~ShaderStorageBufferLayoutBindingProgram()
		{
		}
		//*******************************************************************************
		// overwritten virtual methods
	private:
		/*virtual*/
		StringIntMap getBindingPoints(StringVector args) const
		{
			StringIntMap bindingPoints;

			for (StringVector::iterator it = args.begin(); it != args.end(); it++)
			{
				glw::GLuint idx = gl().getProgramResourceIndex(getProgram(), GL_SHADER_STORAGE_BLOCK, (*it).c_str());
				if (idx != GL_INVALID_INDEX)
				{
					glw::GLenum param = GL_BUFFER_BINDING;
					glw::GLint  value = -1;
					gl().getProgramResourceiv(getProgram(), GL_SHADER_STORAGE_BLOCK, idx, 1, &param, 1, NULL, &value);
					bool hasNoError = (GL_NO_ERROR == gl().getError());
					if (hasNoError)
					{
						bindingPoints[*it] = value;
					}
				}
			}

			return bindingPoints;
		}

		/*virtual*/
		bool setBindingPoints(StringVector list, glw::GLint bindingPoint) const
		{
			for (StringVector::iterator it = list.begin(); it != list.end(); it++)
			{
				glw::GLuint blockIndex =
					gl().getProgramResourceIndex(getProgram(), GL_SHADER_STORAGE_BLOCK, (*it).c_str());
				if (blockIndex == GL_INVALID_INDEX)
				{
					return false;
				}
				gl().shaderStorageBlockBinding(getProgram(), blockIndex, bindingPoint);
			}
			return true;
		}
	};

private:
	// IProgramContextSupplier
	/*virtual*/
	LayoutBindingProgram* createProgram()
	{
		return new ShaderStorageBufferLayoutBindingProgram(*this);
	}

private:
	/*virtual*/
	String buildLayout(const String& binding)
	{
		std::ostringstream s;
		if (!binding.empty())
			s << "layout(binding=" << binding << ", std430) ";
		else
			s << "layout(std430) ";
		return s.str();
	}

	/*virtual*/
	String getDefaultUniformName(int idx = 0)
	{
		StringStream s;

		s << "buffer" << idx;
		return s.str();
	}
	/*virtual*/
	String buildBlockName(const String& name)
	{
		return name;
	}

	/*virtual*/
	String buildBlock(const String& name, const String& type)
	{
		std::ostringstream s;
		s << "{";
		s << type << " " << name << "_a[2];\n";
		s << "}";
		return s.str();
	}

	/*virtual*/
	String buildAccess(const String& var)
	{
		std::ostringstream s;
		s << "vec4(0.0, " << var << "_a[0] + " << var << "_a[1], 0.0, 1.0)";
		return s.str();
	}

	int maxBindings()
	{
		int units = 0;
		gl().getIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &units);
		return units;
	}

	// return max. array size allowed
	int maxArraySize()
	{
		int units = 0;
		switch (getStage())
		{
		case FragmentShader:
			gl().getIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &units);
			break;
		case VertexShader:
			gl().getIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &units);
			break;
		case ComputeShader:
			gl().getIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &units);
			break;
		default:
			DE_ASSERT(0);
			break;
		}
		return units;
	}

	/*virtual*/
	void bind(int binding)
	{
		gl().bindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_buffername);
	}

	/*virtual*/
	void unbind(int binding)
	{
		gl().bindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0);
	}

	/*virtual*/
	void setupTest(void)
	{
		const float f[2] = { 0.25f, 0.75f };
		gl().genBuffers(1, &m_buffername);
		gl().bindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffername);
		gl().bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(f), f, GL_STATIC_DRAW);
	}

	/*virtual*/
	void teardownTest(void)
	{
		gl().bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		gl().deleteBuffers(1, &m_buffername);
	}

	//=========================================================================
	// sub-tests overrides
	//=========================================================================
private:
private:
	//virtual LayoutBindingTestResult        binding_basic_default               (void);
	//virtual LayoutBindingTestResult        binding_basic_explicit              (void);
	//virtual LayoutBindingTestResult        binding_basic_multiple              (void);
	//virtual LayoutBindingTestResult        binding_basic_render                (void);
	//virtual LayoutBindingTestResult        binding_integer_constant            (void);
	//virtual LayoutBindingTestResult        binding_integer_constant_expression (void);
	//virtual LayoutBindingTestResult        binding_array_size                  (void);
	//virtual LayoutBindingTestResult        binding_array_implicit              (void);
	//virtual LayoutBindingTestResult        binding_array_multiple              (void);
	/*virtual*/
	LayoutBindingTestResult binding_api_update(void)
	{
		// only for GL
		if (getGLSLVersion() == glu::GLSL_VERSION_310_ES)
			return LayoutBindingTestResult(true, String(), true);

		return LayoutBindingBaseCase::binding_api_update();
	}
	//virtual LayoutBindingTestResult        binding_compilation_errors          (void);
	//virtual LayoutBindingTestResult        binding_link_errors                 (void);
	//virtual LayoutBindingTestResult        binding_examples                    (void);

private:
	glw::GLuint m_buffername;
};

ShaderStorageBufferLayoutBindingCase::ShaderStorageBufferLayoutBindingCase(Context& context, const char* name,
																		   const char* description, StageType stage,
																		   LayoutBindingParameters& samplerType,
																		   glu::GLSLVersion			glslVersion)
	: LayoutBindingBaseCase(context, name, description, stage, samplerType, glslVersion), m_buffername(0)
{
}

ShaderStorageBufferLayoutBindingCase::~ShaderStorageBufferLayoutBindingCase()
{
}

//*****************************************************************************

LayoutBindingTests::LayoutBindingTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "layout_binding", "Layout Binding LayoutBindingSubTest"), m_glslVersion(glslVersion)
{
}

LayoutBindingTests::~LayoutBindingTests(void)
{
}

StageType LayoutBindingTests::stageTypes[] = {
	{ "ComputeShader", ComputeShader },
	{ "FragmentShader", FragmentShader },
	{ "VertexShader", VertexShader },
};

// uniform_type must match vector_type, i.e. isampler2D=>ivec4
LayoutBindingParameters LayoutBindingTests::test_args[] = {
	{ "uniform", Texture, TwoD, "vec4", "sampler2D", "vec2", "texture" },
	{ "uniform", Texture, ThreeD, "vec4", "sampler3D", "vec3", "texture" },
	{ "uniform", Texture, TwoDArray, "vec4", "sampler2DArray", "vec3", "texture" },
	{ "uniform", Image, TwoD, "vec4", "image2D", "ivec2", "imageLoad" },
	{ "uniform", AtomicCounter, TwoD, "vec4", "atomic_uint", "vec3", "atomic" },
	{ "uniform", UniformBlock, None, "vec4", "block", "vec3", "block" },
	{ "buffer", ShaderStorageBuffer, None, "vec4", "buffer", "vec3", "atomicAdd" },
};

// create test name which must be unique or dEQP framework will throw
// example: sampler2D_layout_binding_vec4_texture_0
String LayoutBindingTests::createTestName(const StageType& stageType, const LayoutBindingParameters& testArgs)
{
	StringStream s;
	s << testArgs.uniform_type;
	s << "_layout_binding_";
	s << testArgs.access_function << "_";
	s << stageType.name;
	return s.str();
}

void LayoutBindingTests::init(void)
{
	std::vector<StageType>				 stages   = makeVector(stageTypes);
	std::vector<LayoutBindingParameters> samplers = makeVector(test_args);
	for (std::vector<StageType>::iterator stagesIter = stages.begin(); stagesIter != stages.end(); stagesIter++)
	{
		for (std::vector<LayoutBindingParameters>::iterator testArgsIter = samplers.begin();
			 testArgsIter != samplers.end(); testArgsIter++)
		{
			String testName = createTestName(*stagesIter, *testArgsIter);
			switch ((*testArgsIter).surface_type)
			{
			case Texture:
				addChild(new SamplerLayoutBindingCase(m_context, testName.c_str(),
													  "test sampler layout binding functionality", *stagesIter,
													  *testArgsIter, m_glslVersion));
				break;
			case Image:
				addChild(new ImageLayoutBindingCase(m_context, testName.c_str(),
													"test image layout binding functionality", *stagesIter,
													*testArgsIter, m_glslVersion));
				break;
			case AtomicCounter:
				addChild(new AtomicCounterLayoutBindingCase(m_context, testName.c_str(),
															"test atomic counters layout binding functionality",
															*stagesIter, *testArgsIter, m_glslVersion));
				break;
			case UniformBlock:
				addChild(new UniformBlocksLayoutBindingCase(m_context, testName.c_str(),
															"test uniform block layout binding functionality",
															*stagesIter, *testArgsIter, m_glslVersion));
				break;
			case ShaderStorageBuffer:
				addChild(new ShaderStorageBufferLayoutBindingCase(
					m_context, testName.c_str(), "test shader storage buffer layout binding functionality", *stagesIter,
					*testArgsIter, m_glslVersion));
				break;
			default:
				DE_ASSERT(0);
				break;
			}
		}
	}
}

} // glcts
