/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
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
 * \brief Geometry shader tests.
 *//*--------------------------------------------------------------------*/

#include "es31fGeometryShaderTests.hpp"

#include "gluRenderContext.hpp"
#include "gluTextureUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "gluContextInfo.hpp"
#include "gluCallLogWrapper.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuStringTemplate.hpp"
#include "glsStateQueryUtil.hpp"

#include "gluStrUtil.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deMemory.h"

#include "sglrContext.hpp"
#include "sglrReferenceContext.hpp"
#include "sglrGLContext.hpp"
#include "sglrReferenceUtils.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include <algorithm>

using namespace glw;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;

const int TEST_CANVAS_SIZE = 256;

static const char* const s_commonShaderSourceVertex =		"${GLSL_VERSION_DECL}\n"
															"in highp vec4 a_position;\n"
															"in highp vec4 a_color;\n"
															"out highp vec4 v_geom_FragColor;\n"
															"void main (void)\n"
															"{\n"
															"	gl_Position = a_position;\n"
															"	gl_PointSize = 1.0;\n"
															"	v_geom_FragColor = a_color;\n"
															"}\n";
static const char* const s_commonShaderSourceFragment =		"${GLSL_VERSION_DECL}\n"
															"layout(location = 0) out mediump vec4 fragColor;\n"
															"in mediump vec4 v_frag_FragColor;\n"
															"void main (void)\n"
															"{\n"
															"	fragColor = v_frag_FragColor;\n"
															"}\n";
static const char* const s_expandShaderSourceGeometryBody =	"in highp vec4 v_geom_FragColor[];\n"
															"out highp vec4 v_frag_FragColor;\n"
															"\n"
															"void main (void)\n"
															"{\n"
															"	const highp vec4 offset0 = vec4(-0.07, -0.01, 0.0, 0.0);\n"
															"	const highp vec4 offset1 = vec4( 0.03, -0.03, 0.0, 0.0);\n"
															"	const highp vec4 offset2 = vec4(-0.01,  0.08, 0.0, 0.0);\n"
															"	      highp vec4 yoffset = float(gl_PrimitiveIDIn) * vec4(0.02, 0.1, 0.0, 0.0);\n"
															"\n"
															"	for (highp int ndx = 0; ndx < gl_in.length(); ndx++)\n"
															"	{\n"
															"		gl_Position = gl_in[ndx].gl_Position + offset0 + yoffset;\n"
															"		gl_PrimitiveID = gl_PrimitiveIDIn;\n"
															"		v_frag_FragColor = v_geom_FragColor[ndx];\n"
															"		EmitVertex();\n"
															"\n"
															"		gl_Position = gl_in[ndx].gl_Position + offset1 + yoffset;\n"
															"		gl_PrimitiveID = gl_PrimitiveIDIn;\n"
															"		v_frag_FragColor = v_geom_FragColor[ndx];\n"
															"		EmitVertex();\n"
															"\n"
															"		gl_Position = gl_in[ndx].gl_Position + offset2 + yoffset;\n"
															"		gl_PrimitiveID = gl_PrimitiveIDIn;\n"
															"		v_frag_FragColor = v_geom_FragColor[ndx];\n"
															"		EmitVertex();\n"
															"		EndPrimitive();\n"
															"	}\n"
															"}\n";

static std::string specializeShader (const std::string& shaderSource, const glu::ContextType& contextType)
{
	const bool							supportsES32	= glu::contextSupports(contextType, glu::ApiType::es(3, 2));
	std::map<std::string, std::string>	args;
	args["GLSL_VERSION_DECL"]					= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(contextType));
	args["GLSL_EXT_GEOMETRY_SHADER"]			= supportsES32 ? "" : "#extension GL_EXT_geometry_shader : require\n";
	args["GLSL_OES_TEXTURE_STORAGE_MULTISAMPLE"]= supportsES32 ? "" : "#extension GL_OES_texture_storage_multisample_2d_array : require\n";

	return tcu::StringTemplate(shaderSource).specialize(args);
}

std::string inputTypeToGLString (rr::GeometryShaderInputType inputType)
{
	switch (inputType)
	{
		case rr::GEOMETRYSHADERINPUTTYPE_POINTS:				return "points";
		case rr::GEOMETRYSHADERINPUTTYPE_LINES:					return "lines";
		case rr::GEOMETRYSHADERINPUTTYPE_LINES_ADJACENCY:		return "lines_adjacency";
		case rr::GEOMETRYSHADERINPUTTYPE_TRIANGLES:				return "triangles";
		case rr::GEOMETRYSHADERINPUTTYPE_TRIANGLES_ADJACENCY:	return "triangles_adjacency";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

std::string outputTypeToGLString (rr::GeometryShaderOutputType outputType)
{
	switch (outputType)
	{
		case rr::GEOMETRYSHADEROUTPUTTYPE_POINTS:				return "points";
		case rr::GEOMETRYSHADEROUTPUTTYPE_LINE_STRIP:			return "line_strip";
		case rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP:		return "triangle_strip";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

std::string primitiveTypeToString (GLenum primitive)
{
	switch (primitive)
	{
		case GL_POINTS:						 return "points";
		case GL_LINES:						 return "lines";
		case GL_LINE_LOOP:					 return "line_loop";
		case GL_LINE_STRIP:					 return "line_strip";
		case GL_LINES_ADJACENCY:			 return "lines_adjacency";
		case GL_LINE_STRIP_ADJACENCY:		 return "line_strip_adjacency";
		case GL_TRIANGLES:					 return "triangles";
		case GL_TRIANGLE_STRIP:				 return "triangle_strip";
		case GL_TRIANGLE_FAN:				 return "triangle_fan";
		case GL_TRIANGLES_ADJACENCY:		 return "triangles_adjacency";
		case GL_TRIANGLE_STRIP_ADJACENCY:	 return "triangle_strip_adjacency";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

struct OutputCountPatternSpec
{
						OutputCountPatternSpec (int count);
						OutputCountPatternSpec (int count0, int count1);

	std::vector<int>	pattern;
};

OutputCountPatternSpec::OutputCountPatternSpec (int count)
{
	pattern.push_back(count);
}

OutputCountPatternSpec::OutputCountPatternSpec (int count0, int count1)
{
	pattern.push_back(count0);
	pattern.push_back(count1);
}

class VertexExpanderShader : public sglr::ShaderProgram
{
public:
				VertexExpanderShader	(const glu::ContextType& contextType, rr::GeometryShaderInputType inputType, rr::GeometryShaderOutputType outputType);

	void		shadeVertices			(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void		shadeFragments			(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void		shadePrimitives			(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

private:
	size_t		calcOutputVertices		(rr::GeometryShaderInputType inputType) const;
	std::string	genGeometrySource		(const glu::ContextType& contextType, rr::GeometryShaderInputType inputType, rr::GeometryShaderOutputType outputType) const;
};

VertexExpanderShader::VertexExpanderShader (const glu::ContextType& contextType, rr::GeometryShaderInputType inputType, rr::GeometryShaderOutputType outputType)
	: sglr::ShaderProgram(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_color", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(specializeShader(s_commonShaderSourceVertex, contextType))
							<< sglr::pdec::FragmentSource(specializeShader(s_commonShaderSourceFragment, contextType))
							<< sglr::pdec::GeometryShaderDeclaration(inputType, outputType, calcOutputVertices(inputType))
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, inputType, outputType)))
{
}

void VertexExpanderShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->pointSize = 1.0f;
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void VertexExpanderShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packets[packetNdx], context, 0, fragNdx));
}

void VertexExpanderShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(invocationID);

	for (int ndx = 0; ndx < numPackets; ++ndx)
	for (int verticeNdx = 0; verticeNdx < verticesIn; ++verticeNdx)
	{
		const tcu::Vec4 offsets[] =
		{
			tcu::Vec4(-0.07f, -0.01f, 0.0f, 0.0f),
			tcu::Vec4( 0.03f, -0.03f, 0.0f, 0.0f),
			tcu::Vec4(-0.01f,  0.08f, 0.0f, 0.0f)
		};
		const tcu::Vec4 yoffset = float(packets[ndx].primitiveIDIn) * tcu::Vec4(0.02f, 0.1f, 0, 0);

		// Create new primitive at every input vertice
		const rr::VertexPacket* vertex = packets[ndx].vertices[verticeNdx];

		output.EmitVertex(vertex->position + offsets[0] + yoffset, vertex->pointSize, vertex->outputs, packets[ndx].primitiveIDIn);
		output.EmitVertex(vertex->position + offsets[1] + yoffset, vertex->pointSize, vertex->outputs, packets[ndx].primitiveIDIn);
		output.EmitVertex(vertex->position + offsets[2] + yoffset, vertex->pointSize, vertex->outputs, packets[ndx].primitiveIDIn);
		output.EndPrimitive();
	}
}

size_t VertexExpanderShader::calcOutputVertices (rr::GeometryShaderInputType inputType) const
{
	switch (inputType)
	{
		case rr::GEOMETRYSHADERINPUTTYPE_POINTS:				return 1 * 3;
		case rr::GEOMETRYSHADERINPUTTYPE_LINES:					return 2 * 3;
		case rr::GEOMETRYSHADERINPUTTYPE_LINES_ADJACENCY:		return 4 * 3;
		case rr::GEOMETRYSHADERINPUTTYPE_TRIANGLES:				return 3 * 3;
		case rr::GEOMETRYSHADERINPUTTYPE_TRIANGLES_ADJACENCY:	return 6 * 3;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

std::string	VertexExpanderShader::genGeometrySource (const glu::ContextType& contextType, rr::GeometryShaderInputType inputType, rr::GeometryShaderOutputType outputType) const
{
	std::ostringstream str;

	str << "${GLSL_VERSION_DECL}\n";
	str << "${GLSL_EXT_GEOMETRY_SHADER}";
	str << "layout(" << inputTypeToGLString(inputType) << ") in;\n";
	str << "layout(" << outputTypeToGLString(outputType) << ", max_vertices = " << calcOutputVertices(inputType) << ") out;";
	str << "\n";
	str << s_expandShaderSourceGeometryBody;

	return specializeShader(str.str(), contextType);
}

class VertexEmitterShader : public sglr::ShaderProgram
{
public:
				VertexEmitterShader		(const glu::ContextType& contextType, int emitCountA, int endCountA, int emitCountB, int endCountB, rr::GeometryShaderOutputType outputType);

	void		shadeVertices			(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void		shadeFragments			(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void		shadePrimitives			(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

private:
	std::string	genGeometrySource		(const glu::ContextType& contextType, int emitCountA, int endCountA, int emitCountB, int endCountB, rr::GeometryShaderOutputType outputType) const;

	int			m_emitCountA;
	int			m_endCountA;
	int			m_emitCountB;
	int			m_endCountB;
};

VertexEmitterShader::VertexEmitterShader (const glu::ContextType& contextType, int emitCountA, int endCountA, int emitCountB, int endCountB, rr::GeometryShaderOutputType outputType)
	: sglr::ShaderProgram(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_color", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(specializeShader(s_commonShaderSourceVertex, contextType))
							<< sglr::pdec::FragmentSource(specializeShader(s_commonShaderSourceFragment, contextType))
							<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_POINTS, outputType, emitCountA + emitCountB)
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, emitCountA, endCountA, emitCountB, endCountB, outputType)))
	, m_emitCountA		(emitCountA)
	, m_endCountA		(endCountA)
	, m_emitCountB		(emitCountB)
	, m_endCountB		(endCountB)
{
}

void VertexEmitterShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->pointSize = 1.0f;
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void VertexEmitterShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packets[packetNdx], context, 0, fragNdx));
}

void VertexEmitterShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(verticesIn);
	DE_UNREF(invocationID);

	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		const tcu::Vec4 positions[] =
		{
			tcu::Vec4(-0.5f,   0.5f, 0.0f, 0.0f),
			tcu::Vec4( 0.0f,   0.1f, 0.0f, 0.0f),
			tcu::Vec4( 0.5f,   0.5f, 0.0f, 0.0f),
			tcu::Vec4( 0.7f,  -0.2f, 0.0f, 0.0f),
			tcu::Vec4( 0.2f,   0.2f, 0.0f, 0.0f),
			tcu::Vec4( 0.4f,  -0.3f, 0.0f, 0.0f),
		};

		// Create new primitive at this point
		const rr::VertexPacket* vertex = packets[ndx].vertices[0];

		for (int i = 0; i < m_emitCountA; ++i)
			output.EmitVertex(vertex->position + positions[i], vertex->pointSize, vertex->outputs, packets[ndx].primitiveIDIn);

		for (int i = 0; i < m_endCountA; ++i)
			output.EndPrimitive();

		for (int i = 0; i < m_emitCountB; ++i)
			output.EmitVertex(vertex->position + positions[m_emitCountA + i], vertex->pointSize, vertex->outputs, packets[ndx].primitiveIDIn);

		for (int i = 0; i < m_endCountB; ++i)
			output.EndPrimitive();
	}
}

std::string	VertexEmitterShader::genGeometrySource (const glu::ContextType& contextType, int emitCountA, int endCountA, int emitCountB, int endCountB, rr::GeometryShaderOutputType outputType) const
{
	std::ostringstream str;

	str << "${GLSL_VERSION_DECL}\n";
	str << "${GLSL_EXT_GEOMETRY_SHADER}";
	str << "layout(points) in;\n";
	str << "layout(" << outputTypeToGLString(outputType) << ", max_vertices = " << (emitCountA+emitCountB) << ") out;";
	str << "\n";

	str <<	"in highp vec4 v_geom_FragColor[];\n"
			"out highp vec4 v_frag_FragColor;\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	const highp vec4 position0 = vec4(-0.5,  0.5, 0.0, 0.0);\n"
			"	const highp vec4 position1 = vec4( 0.0,  0.1, 0.0, 0.0);\n"
			"	const highp vec4 position2 = vec4( 0.5,  0.5, 0.0, 0.0);\n"
			"	const highp vec4 position3 = vec4( 0.7, -0.2, 0.0, 0.0);\n"
			"	const highp vec4 position4 = vec4( 0.2,  0.2, 0.0, 0.0);\n"
			"	const highp vec4 position5 = vec4( 0.4, -0.3, 0.0, 0.0);\n"
			"\n";

	for (int i = 0; i < emitCountA; ++i)
		str <<	"	gl_Position = gl_in[0].gl_Position + position" << i << ";\n"
				"	gl_PrimitiveID = gl_PrimitiveIDIn;\n"
				"	v_frag_FragColor = v_geom_FragColor[0];\n"
				"	EmitVertex();\n"
				"\n";

	for (int i = 0; i < endCountA; ++i)
		str << "	EndPrimitive();\n";

	for (int i = 0; i < emitCountB; ++i)
		str <<	"	gl_Position = gl_in[0].gl_Position + position" << (emitCountA + i) << ";\n"
				"	gl_PrimitiveID = gl_PrimitiveIDIn;\n"
				"	v_frag_FragColor = v_geom_FragColor[0];\n"
				"	EmitVertex();\n"
				"\n";

	for (int i = 0; i < endCountB; ++i)
		str << "	EndPrimitive();\n";

	str << "}\n";

	return specializeShader(str.str(), contextType);
}

class VertexVaryingShader : public sglr::ShaderProgram
{
public:
												VertexVaryingShader		(const glu::ContextType& contextType, int vertexOut, int geometryOut);

	void										shadeVertices			(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void										shadeFragments			(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void										shadePrimitives			(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

private:
	static sglr::pdec::ShaderProgramDeclaration genProgramDeclaration	(const glu::ContextType& contextType, int vertexOut, int geometryOut);

	const int									m_vertexOut;
	const int									m_geometryOut;
};

VertexVaryingShader::VertexVaryingShader (const glu::ContextType& contextType, int vertexOut, int geometryOut)
	: sglr::ShaderProgram	(genProgramDeclaration(contextType, vertexOut, geometryOut))
	, m_vertexOut			(vertexOut)
	, m_geometryOut			(geometryOut)
{
}

void VertexVaryingShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	// vertex shader is no-op
	if (m_vertexOut == -1)
		return;

	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		const tcu::Vec4 color = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);

		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->pointSize = 1.0f;

		switch (m_vertexOut)
		{
			case 0:
				break;

			case 1:
				packets[ndx]->outputs[0] = color;
				break;

			case 2:
				packets[ndx]->outputs[0] = color * 0.5f;
				packets[ndx]->outputs[1] = color.swizzle(2,1,0,3) * 0.5f;
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
	}
}

void VertexVaryingShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	{
		switch (m_geometryOut)
		{
			case 0:
				for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
					rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
				break;

			case 1:
				for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
					rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readTriangleVarying<float>(packets[packetNdx], context, 0, fragNdx));
				break;

			case 2:
				for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
					rr::writeFragmentOutput(context, packetNdx, fragNdx, 0,   rr::readTriangleVarying<float>(packets[packetNdx], context, 0, fragNdx)
					                                                        + rr::readTriangleVarying<float>(packets[packetNdx], context, 1, fragNdx).swizzle(1, 0, 2, 3));
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
	}
}

void VertexVaryingShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(invocationID);

	const tcu::Vec4 vertexOffset(-0.2f, -0.2f, 0, 0);

	if (m_vertexOut == -1)
	{
		// vertex is a no-op
		const tcu::Vec4 inputColor = tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
		rr::GenericVec4	outputs[2];

		// output color
		switch (m_geometryOut)
		{
			case 0:
				break;

			case 1:
				outputs[0] = inputColor;
				break;

			case 2:
				outputs[0] = inputColor * 0.5f;
				outputs[1] = inputColor.swizzle(1, 0, 2, 3) * 0.5f;
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}

		for (int ndx = 0; ndx < numPackets; ++ndx)
		{
			output.EmitVertex(tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f) + vertexOffset, 1.0f, outputs, packets[ndx].primitiveIDIn);
			output.EmitVertex(tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f) + vertexOffset, 1.0f, outputs, packets[ndx].primitiveIDIn);
			output.EmitVertex(tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f) + vertexOffset, 1.0f, outputs, packets[ndx].primitiveIDIn);
			output.EndPrimitive();
		}
	}
	else
	{
		// vertex is not a no-op
		for (int ndx = 0; ndx < numPackets; ++ndx)
		{
			for (int verticeNdx = 0; verticeNdx < verticesIn; ++verticeNdx)
			{
				tcu::Vec4		inputColor;
				rr::GenericVec4	outputs[2];

				// input color
				switch (m_vertexOut)
				{
					case 0:
						inputColor = tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
						break;

					case 1:
						inputColor = packets[ndx].vertices[verticeNdx]->outputs[0].get<float>();
						break;

					case 2:
						inputColor = (packets[ndx].vertices[verticeNdx]->outputs[0].get<float>() * 0.5f)
								   + (packets[ndx].vertices[verticeNdx]->outputs[1].get<float>().swizzle(2, 1, 0, 3) * 0.5f);
						break;

					default:
						DE_ASSERT(DE_FALSE);
				}

				// output color
				switch (m_geometryOut)
				{
					case 0:
						break;

					case 1:
						outputs[0] = inputColor;
						break;

					case 2:
						outputs[0] = inputColor * 0.5f;
						outputs[1] = inputColor.swizzle(1, 0, 2, 3) * 0.5f;
						break;

					default:
						DE_ASSERT(DE_FALSE);
				}

				output.EmitVertex(packets[ndx].vertices[verticeNdx]->position + vertexOffset, packets[ndx].vertices[verticeNdx]->pointSize, outputs, packets[ndx].primitiveIDIn);
			}
			output.EndPrimitive();
		}
	}
}

sglr::pdec::ShaderProgramDeclaration VertexVaryingShader::genProgramDeclaration	(const glu::ContextType& contextType, int vertexOut, int geometryOut)
{
	sglr::pdec::ShaderProgramDeclaration	decl;
	std::ostringstream						vertexSource;
	std::ostringstream						fragmentSource;
	std::ostringstream						geometrySource;

	decl
		<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
		<< sglr::pdec::VertexAttribute("a_color", rr::GENERICVECTYPE_FLOAT);

	for (int i = 0; i < vertexOut; ++i)
		decl << sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT);
	for (int i = 0; i < geometryOut; ++i)
		decl << sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT);

	decl
		<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
		<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_TRIANGLES, rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP, 3);

	// vertexSource

	vertexSource << "${GLSL_VERSION_DECL}\n"
					"in highp vec4 a_position;\n"
					"in highp vec4 a_color;\n";

	// no-op case?
	if (vertexOut == -1)
	{
		vertexSource << "void main (void)\n"
						"{\n"
						"}\n";
	}
	else
	{
		for (int i = 0; i < vertexOut; ++i)
			vertexSource << "out highp vec4 v_geom_" << i << ";\n";

		vertexSource << "void main (void)\n"
						"{\n"
						"\tgl_Position = a_position;\n"
						"\tgl_PointSize = 1.0;\n";
		switch (vertexOut)
		{
			case 0:
				break;

			case 1:
				vertexSource << "\tv_geom_0 = a_color;\n";
				break;

			case 2:
				vertexSource << "\tv_geom_0 = a_color * 0.5;\n";
				vertexSource << "\tv_geom_1 = a_color.zyxw * 0.5;\n";
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
		vertexSource << "}\n";
	}

	// fragmentSource

	fragmentSource <<	"${GLSL_VERSION_DECL}\n"
						"layout(location = 0) out mediump vec4 fragColor;\n";

	for (int i = 0; i < geometryOut; ++i)
		fragmentSource << "in mediump vec4 v_frag_" << i << ";\n";

	fragmentSource <<	"void main (void)\n"
						"{\n";
	switch (geometryOut)
	{
		case 0:
			fragmentSource << "\tfragColor = vec4(1.0, 0.0, 0.0, 1.0);\n";
			break;

		case 1:
			fragmentSource << "\tfragColor = v_frag_0;\n";
			break;

		case 2:
			fragmentSource << "\tfragColor = v_frag_0 + v_frag_1.yxzw;\n";
			break;

		default:
			DE_ASSERT(DE_FALSE);
	}
	fragmentSource << "}\n";

	// geometrySource

	geometrySource <<	"${GLSL_VERSION_DECL}\n"
						"${GLSL_EXT_GEOMETRY_SHADER}"
						"layout(triangles) in;\n"
						"layout(triangle_strip, max_vertices = 3) out;\n";

	for (int i = 0; i < vertexOut; ++i)
		geometrySource << "in highp vec4 v_geom_" << i << "[];\n";
	for (int i = 0; i < geometryOut; ++i)
		geometrySource << "out highp vec4 v_frag_" << i << ";\n";

	geometrySource <<	"void main (void)\n"
						"{\n"
						"\thighp vec4 offset = vec4(-0.2, -0.2, 0.0, 0.0);\n"
						"\thighp vec4 inputColor;\n\n";

	for (int vertexNdx = 0; vertexNdx < 3; ++vertexNdx)
	{
		if (vertexOut == -1)
		{
			// vertex is a no-op
			geometrySource <<	"\tinputColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
								"\tgl_Position = vec4(" << ((vertexNdx==0) ? ("0.0, 0.0") : ((vertexNdx==1) ? ("1.0, 0.0") : ("1.0, 1.0"))) << ", 0.0, 1.0) + offset;\n"
								"\tgl_PrimitiveID = gl_PrimitiveIDIn;\n";
		}
		else
		{
			switch (vertexOut)
			{
				case 0:
					geometrySource << "\tinputColor = vec4(1.0, 0.0, 0.0, 1.0);\n";
					break;

				case 1:
					geometrySource << "\tinputColor = v_geom_0[" << vertexNdx << "];\n";
					break;

				case 2:
					geometrySource << "\tinputColor = v_geom_0[" << vertexNdx << "] * 0.5 + v_geom_1[" << vertexNdx << "].zyxw * 0.5;\n";
					break;

				default:
					DE_ASSERT(DE_FALSE);
			}
			geometrySource <<	"\tgl_Position = gl_in[" << vertexNdx << "].gl_Position + offset;\n"
								"\tgl_PrimitiveID = gl_PrimitiveIDIn;\n";
		}

		switch (geometryOut)
		{
			case 0:
				break;

			case 1:
				geometrySource << "\tv_frag_0 = inputColor;\n";
				break;

			case 2:
				geometrySource << "\tv_frag_0 = inputColor * 0.5;\n";
				geometrySource << "\tv_frag_1 = inputColor.yxzw * 0.5;\n";
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}

		geometrySource << "\tEmitVertex();\n\n";
	}

	geometrySource <<	"\tEndPrimitive();\n"
						"}\n";

	decl
		<< sglr::pdec::VertexSource(specializeShader(vertexSource.str(), contextType))
		<< sglr::pdec::FragmentSource(specializeShader(fragmentSource.str(), contextType))
		<< sglr::pdec::GeometrySource(specializeShader(geometrySource.str(), contextType));
	return decl;
}

class OutputCountShader : public sglr::ShaderProgram
{
public:
									OutputCountShader		(const glu::ContextType& contextType, const OutputCountPatternSpec& spec);

	void							shadeVertices			(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void							shadeFragments			(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void							shadePrimitives			(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

private:
	std::string						genGeometrySource		(const glu::ContextType& contextType, const OutputCountPatternSpec& spec) const;
	size_t							getPatternEmitCount		(const OutputCountPatternSpec& spec) const;

	const int						m_patternLength;
	const int						m_patternMaxEmitCount;
	const OutputCountPatternSpec	m_spec;
};

OutputCountShader::OutputCountShader (const glu::ContextType& contextType, const OutputCountPatternSpec& spec)
	: sglr::ShaderProgram	(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_color", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(specializeShader(s_commonShaderSourceVertex, contextType))
							<< sglr::pdec::FragmentSource(specializeShader(s_commonShaderSourceFragment, contextType))
							<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_POINTS, rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP, getPatternEmitCount(spec))
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, spec)))
	, m_patternLength		((int)spec.pattern.size())
	, m_patternMaxEmitCount	((int)getPatternEmitCount(spec))
	, m_spec				(spec)
{
}

void OutputCountShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->pointSize = 1.0f;
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void OutputCountShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packets[packetNdx], context, 0, fragNdx));
}

void OutputCountShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(verticesIn);
	DE_UNREF(invocationID);

	const float rowHeight	= 2.0f / (float)m_patternLength;
	const float colWidth	= 2.0f / (float)m_patternMaxEmitCount;

	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	{
		// Create triangle strip at this point
		const rr::VertexPacket*	vertex		= packets[packetNdx].vertices[0];
		const int				emitCount	= m_spec.pattern[packets[packetNdx].primitiveIDIn];

		for (int ndx = 0; ndx < emitCount / 2; ++ndx)
		{
			output.EmitVertex(vertex->position + tcu::Vec4(2 * (float)ndx * colWidth, 0.0,       0.0, 0.0), vertex->pointSize, vertex->outputs, packets[packetNdx].primitiveIDIn);
			output.EmitVertex(vertex->position + tcu::Vec4(2 * (float)ndx * colWidth, rowHeight, 0.0, 0.0), vertex->pointSize, vertex->outputs, packets[packetNdx].primitiveIDIn);
		}
		output.EndPrimitive();
	}
}

std::string	OutputCountShader::genGeometrySource (const glu::ContextType& contextType, const OutputCountPatternSpec& spec) const
{
	std::ostringstream str;

	// draw row with a triangle strip, always make rectangles
	for (int ndx = 0; ndx < (int)spec.pattern.size(); ++ndx)
		DE_ASSERT(spec.pattern[ndx] % 2 == 0);

	str << "${GLSL_VERSION_DECL}\n";
	str << "${GLSL_EXT_GEOMETRY_SHADER}";
	str << "layout(points) in;\n";
	str << "layout(triangle_strip, max_vertices = " << getPatternEmitCount(spec) << ") out;";
	str << "\n";

	str <<	"in highp vec4 v_geom_FragColor[];\n"
			"out highp vec4 v_frag_FragColor;\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	const highp float rowHeight = 2.0 / float(" << spec.pattern.size() << ");\n"
			"	const highp float colWidth = 2.0 / float(" << getPatternEmitCount(spec) << ");\n"
			"\n";

	str <<	"	highp int emitCount = ";
	for (int ndx = 0; ndx < (int)spec.pattern.size() - 1; ++ndx)
		str << "(gl_PrimitiveIDIn == " << ndx << ") ? (" << spec.pattern[ndx] << ") : (";
	str <<	spec.pattern[(int)spec.pattern.size() - 1]
		<<	((spec.pattern.size() == 1) ? ("") : (")"))
		<<	";\n";

	str <<	"	for (highp int ndx = 0; ndx < emitCount / 2; ndx++)\n"
			"	{\n"
			"		gl_Position = gl_in[0].gl_Position + vec4(float(ndx) * 2.0 * colWidth, 0.0, 0.0, 0.0);\n"
			"		v_frag_FragColor = v_geom_FragColor[0];\n"
			"		EmitVertex();\n"
			"\n"
			"		gl_Position = gl_in[0].gl_Position + vec4(float(ndx) * 2.0 * colWidth, rowHeight, 0.0, 0.0);\n"
			"		v_frag_FragColor = v_geom_FragColor[0];\n"
			"		EmitVertex();\n"
			"	}\n"
			"}\n";

	return specializeShader(str.str(), contextType);
}

size_t OutputCountShader::getPatternEmitCount (const OutputCountPatternSpec& spec) const
{
	return *std::max_element(spec.pattern.begin(), spec.pattern.end());
}

class BuiltinVariableShader : public sglr::ShaderProgram
{
public:
	enum VariableTest
	{
		TEST_POINT_SIZE = 0,
		TEST_PRIMITIVE_ID_IN,
		TEST_PRIMITIVE_ID,

		TEST_LAST
	};

						BuiltinVariableShader	(const glu::ContextType& contextType, VariableTest test);

	void				shadeVertices			(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void				shadeFragments			(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void				shadePrimitives			(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

	static const char*	getTestAttributeName	(VariableTest test);

private:
	std::string			genGeometrySource		(const glu::ContextType& contextType, VariableTest test) const;
	std::string			genVertexSource			(const glu::ContextType& contextType, VariableTest test) const;
	std::string			genFragmentSource		(const glu::ContextType& contextType, VariableTest test) const;

	const VariableTest	m_test;
};

BuiltinVariableShader::BuiltinVariableShader (const glu::ContextType& contextType, VariableTest test)
	: sglr::ShaderProgram	(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute(getTestAttributeName(test), rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(genVertexSource(contextType, test))
							<< sglr::pdec::FragmentSource(genFragmentSource(contextType, test))
							<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_POINTS,
																	 ((test == TEST_POINT_SIZE) ? (rr::GEOMETRYSHADEROUTPUTTYPE_POINTS) : (rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP)),
																	 ((test == TEST_POINT_SIZE) ? (1) : (3)))
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, test)))
	, m_test				(test)
{
}

void BuiltinVariableShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->pointSize = 1.0f;
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void BuiltinVariableShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	const tcu::Vec4 red			= tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const tcu::Vec4 green		= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 blue		= tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f);
	const tcu::Vec4 yellow		= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 colors[4]	= { yellow, red, green, blue };

	if (m_test == TEST_POINT_SIZE || m_test == TEST_PRIMITIVE_ID_IN)
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
			rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packets[packetNdx], context, 0, fragNdx));
	}
	else if (m_test == TEST_PRIMITIVE_ID)
	{
		const tcu::Vec4 color = colors[context.primitiveID % 4];

		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
			rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, color);
	}
	else
		DE_ASSERT(DE_FALSE);
}

void BuiltinVariableShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(verticesIn);
	DE_UNREF(invocationID);

	const tcu::Vec4 red			= tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const tcu::Vec4 green		= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 blue		= tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f);
	const tcu::Vec4 yellow		= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 colors[4]	= { red, green, blue, yellow };

	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	{
		const rr::VertexPacket*	vertex = packets[packetNdx].vertices[0];

		if (m_test == TEST_POINT_SIZE)
		{
			rr::GenericVec4	fragColor;
			const float		pointSize = vertex->outputs[0].get<float>().x() + 1.0f;

			fragColor = tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			output.EmitVertex(vertex->position, pointSize, &fragColor, packets[packetNdx].primitiveIDIn);
		}
		else if (m_test == TEST_PRIMITIVE_ID_IN)
		{
			rr::GenericVec4	fragColor;
			fragColor = colors[packets[packetNdx].primitiveIDIn % 4];

			output.EmitVertex(vertex->position + tcu::Vec4(0.05f, 0.0f,  0.0f, 0.0f), 1.0f, &fragColor, packets[packetNdx].primitiveIDIn);
			output.EmitVertex(vertex->position - tcu::Vec4(0.05f, 0.0f,  0.0f, 0.0f), 1.0f, &fragColor, packets[packetNdx].primitiveIDIn);
			output.EmitVertex(vertex->position + tcu::Vec4(0.0f,  0.05f, 0.0f, 0.0f), 1.0f, &fragColor, packets[packetNdx].primitiveIDIn);
		}
		else if (m_test == TEST_PRIMITIVE_ID)
		{
			const int primitiveID = (int)deFloatFloor(vertex->outputs[0].get<float>().x()) + 3;

			output.EmitVertex(vertex->position + tcu::Vec4(0.05f, 0.0f,  0.0f, 0.0f), 1.0f, vertex->outputs, primitiveID);
			output.EmitVertex(vertex->position - tcu::Vec4(0.05f, 0.0f,  0.0f, 0.0f), 1.0f, vertex->outputs, primitiveID);
			output.EmitVertex(vertex->position + tcu::Vec4(0.0f,  0.05f, 0.0f, 0.0f), 1.0f, vertex->outputs, primitiveID);
		}
		else
			DE_ASSERT(DE_FALSE);

		output.EndPrimitive();
	}
}

const char* BuiltinVariableShader::getTestAttributeName (VariableTest test)
{
	switch (test)
	{
		case TEST_POINT_SIZE:			return "a_pointSize";
		case TEST_PRIMITIVE_ID_IN:		return "";
		case TEST_PRIMITIVE_ID:			return "a_primitiveID";
		default:
			DE_ASSERT(DE_FALSE);
			return "";
	}
}

std::string BuiltinVariableShader::genGeometrySource (const glu::ContextType& contextType, VariableTest test) const
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_GEOMETRY_SHADER}";

	if (test == TEST_POINT_SIZE)
		buf << "#extension GL_EXT_geometry_point_size : require\n";

	buf << "layout(points) in;\n";

	if (test == TEST_POINT_SIZE)
		buf << "layout(points, max_vertices = 1) out;\n";
	else
		buf << "layout(triangle_strip, max_vertices = 3) out;\n";

	if (test == TEST_POINT_SIZE)
		buf << "in highp vec4 v_geom_pointSize[];\n";
	else if (test == TEST_PRIMITIVE_ID)
		buf << "in highp vec4 v_geom_primitiveID[];\n";

	if (test != TEST_PRIMITIVE_ID)
		buf << "out highp vec4 v_frag_FragColor;\n";

	buf <<	"\n"
			"void main (void)\n"
			"{\n";

	if (test == TEST_POINT_SIZE)
	{
		buf <<	"	gl_Position = gl_in[0].gl_Position;\n"
				"	gl_PointSize = v_geom_pointSize[0].x + 1.0;\n"
				"	v_frag_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
				"	EmitVertex();\n";
	}
	else if (test == TEST_PRIMITIVE_ID_IN)
	{
		buf <<	"	const highp vec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	const highp vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);\n"
				"	const highp vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 colors[4] = vec4[4](red, green, blue, yellow);\n"
				"\n"
				"	gl_Position = gl_in[0].gl_Position + vec4(0.05, 0.0, 0.0, 0.0);\n"
				"	v_frag_FragColor = colors[gl_PrimitiveIDIn % 4];\n"
				"	EmitVertex();\n"
				"\n"
				"	gl_Position = gl_in[0].gl_Position - vec4(0.05, 0.0, 0.0, 0.0);\n"
				"	v_frag_FragColor = colors[gl_PrimitiveIDIn % 4];\n"
				"	EmitVertex();\n"
				"\n"
				"	gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.05, 0.0, 0.0);\n"
				"	v_frag_FragColor = colors[gl_PrimitiveIDIn % 4];\n"
				"	EmitVertex();\n";
	}
	else if (test == TEST_PRIMITIVE_ID)
	{
		buf <<	"	gl_Position = gl_in[0].gl_Position + vec4(0.05, 0.0, 0.0, 0.0);\n"
				"	gl_PrimitiveID = int(floor(v_geom_primitiveID[0].x)) + 3;\n"
				"	EmitVertex();\n"
				"\n"
				"	gl_Position = gl_in[0].gl_Position - vec4(0.05, 0.0, 0.0, 0.0);\n"
				"	gl_PrimitiveID = int(floor(v_geom_primitiveID[0].x)) + 3;\n"
				"	EmitVertex();\n"
				"\n"
				"	gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.05, 0.0, 0.0);\n"
				"	gl_PrimitiveID = int(floor(v_geom_primitiveID[0].x)) + 3;\n"
				"	EmitVertex();\n"
				"\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "}\n";

	return specializeShader(buf.str(), contextType);
}

std::string BuiltinVariableShader::genVertexSource (const glu::ContextType& contextType, VariableTest test) const
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n";

	if (test == TEST_POINT_SIZE)
		buf << "in highp vec4 a_pointSize;\n";
	else if (test == TEST_PRIMITIVE_ID)
		buf << "in highp vec4 a_primitiveID;\n";

	if (test == TEST_POINT_SIZE)
		buf << "out highp vec4 v_geom_pointSize;\n";
	else if (test == TEST_PRIMITIVE_ID)
		buf << "out highp vec4 v_geom_primitiveID;\n";

	buf <<	"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	gl_PointSize = 1.0;\n";

	if (test == TEST_POINT_SIZE)
		buf << "	v_geom_pointSize = a_pointSize;\n";
	else if (test == TEST_PRIMITIVE_ID)
		buf << "	v_geom_primitiveID = a_primitiveID;\n";

	buf <<	"}\n";

	return specializeShader(buf.str(), contextType);
}

std::string BuiltinVariableShader::genFragmentSource (const glu::ContextType& contextType, VariableTest test) const
{
	std::ostringstream buf;

	if (test == TEST_POINT_SIZE || test == TEST_PRIMITIVE_ID_IN)
		return specializeShader(s_commonShaderSourceFragment, contextType);
	else if (test == TEST_PRIMITIVE_ID)
	{
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXT_GEOMETRY_SHADER}"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	const mediump vec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	const mediump vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	const mediump vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);\n"
				"	const mediump vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"	const mediump vec4 colors[4] = vec4[4](yellow, red, green, blue);\n"
				"	fragColor = colors[gl_PrimitiveID % 4];\n"
				"}\n";

		return specializeShader(buf.str(), contextType);
	}
	else
	{
		DE_ASSERT(DE_FALSE);
		return DE_NULL;
	}
}

class VaryingOutputCountShader : public sglr::ShaderProgram
{
public:
	enum VaryingSource
	{
		READ_ATTRIBUTE = 0,
		READ_UNIFORM,
		READ_TEXTURE,

		READ_LAST
	};

	enum
	{
		EMIT_COUNT_VERTEX_0 = 6,
		EMIT_COUNT_VERTEX_1 = 0,
		EMIT_COUNT_VERTEX_2 = -1,
		EMIT_COUNT_VERTEX_3 = 10,
	};

								VaryingOutputCountShader	(const glu::ContextType& contextType, VaryingSource source, int maxEmitCount, bool instanced);

	void						shadeVertices				(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void						shadeFragments				(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void						shadePrimitives				(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

	static const char*			getAttributeName			(VaryingSource test);

private:
	static std::string			genGeometrySource			(const glu::ContextType& contextType, VaryingSource test, int maxEmitCount, bool instanced);
	static std::string			genVertexSource				(const glu::ContextType& contextType, VaryingSource test);

	const VaryingSource			m_test;
	const sglr::UniformSlot&	m_sampler;
	const sglr::UniformSlot&	m_emitCount;
	const int					m_maxEmitCount;
	const bool					m_instanced;
};

VaryingOutputCountShader::VaryingOutputCountShader (const glu::ContextType& contextType, VaryingSource source, int maxEmitCount, bool instanced)
	: sglr::ShaderProgram	(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::Uniform("u_sampler", glu::TYPE_SAMPLER_2D)
							<< sglr::pdec::Uniform("u_emitCount", glu::TYPE_INT_VEC4)
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute(getAttributeName(source), rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(genVertexSource(contextType, source))
							<< sglr::pdec::FragmentSource(specializeShader(s_commonShaderSourceFragment, contextType))
							<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_POINTS,
																	 rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP,
																	 maxEmitCount,
																	 (instanced) ? (4) : (1))
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, source, maxEmitCount, instanced)))
	, m_test				(source)
	, m_sampler				(getUniformByName("u_sampler"))
	, m_emitCount			(getUniformByName("u_emitCount"))
	, m_maxEmitCount		(maxEmitCount)
	, m_instanced			(instanced)
{
}

void VaryingOutputCountShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void VaryingOutputCountShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packets[packetNdx], context, 0, fragNdx));
}

void VaryingOutputCountShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(verticesIn);

	const tcu::Vec4 red			= tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const tcu::Vec4 green		= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 blue		= tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f);
	const tcu::Vec4 yellow		= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 colors[4]	= { red, green, blue, yellow };

	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	{
		const rr::VertexPacket*	vertex		= packets[packetNdx].vertices[0];
		int						emitCount	= 0;
		tcu::Vec4				color		= tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);

		if (m_test == READ_ATTRIBUTE)
		{
			emitCount = (int)vertex->outputs[0].get<float>()[(m_instanced) ? (invocationID) : (0)];
			color = tcu::Vec4((emitCount < 10) ? (0.0f) : (1.0f), (emitCount > 10) ? (0.0f) : (1.0f), 1.0f, 1.0f);
		}
		else if (m_test == READ_UNIFORM)
		{
			const int primitiveNdx = (m_instanced) ? (invocationID) : ((int)vertex->outputs[0].get<float>().x());

			DE_ASSERT(primitiveNdx >= 0);
			DE_ASSERT(primitiveNdx < 4);

			emitCount = m_emitCount.value.i4[primitiveNdx];
			color = colors[primitiveNdx];
		}
		else if (m_test == READ_TEXTURE)
		{
			const int			primitiveNdx	= (m_instanced) ? (invocationID) : ((int)vertex->outputs[0].get<float>().x());
			const tcu::Vec2		texCoord		= tcu::Vec2(1.0f / 8.0f + (float)primitiveNdx / 4.0f, 0.5f);
			const tcu::Vec4		texColor		= m_sampler.sampler.tex2D->sample(texCoord.x(), texCoord.y(), 0.0f);

			DE_ASSERT(primitiveNdx >= 0);
			DE_ASSERT(primitiveNdx < 4);

			color = colors[primitiveNdx];
			emitCount = 0;

			if (texColor.x() > 0.0f)
				emitCount += (EMIT_COUNT_VERTEX_0 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_0);
			if (texColor.y() > 0.0f)
				emitCount += (EMIT_COUNT_VERTEX_1 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_1);
			if (texColor.z() > 0.0f)
				emitCount += (EMIT_COUNT_VERTEX_2 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_2);
			if (texColor.w() > 0.0f)
				emitCount += (EMIT_COUNT_VERTEX_3 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_3);
		}
		else
			DE_ASSERT(DE_FALSE);

		for (int ndx = 0; ndx < (int)emitCount / 2; ++ndx)
		{
			const float		angle			= (float(ndx) + 0.5f) / float(emitCount / 2) * 3.142f;
			const tcu::Vec4 basePosition	= (m_instanced) ?
												(vertex->position + tcu::Vec4(deFloatCos(float(invocationID)), deFloatSin(float(invocationID)), 0.0f, 0.0f) * 0.5f) :
												(vertex->position);
			const tcu::Vec4	position0		= basePosition + tcu::Vec4(deFloatCos(angle),  deFloatSin(angle), 0.0f, 0.0f) * 0.15f;
			const tcu::Vec4	position1		= basePosition + tcu::Vec4(deFloatCos(angle), -deFloatSin(angle), 0.0f, 0.0f) * 0.15f;
			rr::GenericVec4	fragColor;

			fragColor = color;

			output.EmitVertex(position0, 0.0f, &fragColor, packets[packetNdx].primitiveIDIn);
			output.EmitVertex(position1, 0.0f, &fragColor, packets[packetNdx].primitiveIDIn);
		}

		output.EndPrimitive();
	}
}

const char* VaryingOutputCountShader::getAttributeName (VaryingSource test)
{
	switch (test)
	{
		case READ_ATTRIBUTE:	return "a_emitCount";
		case READ_UNIFORM:		return "a_vertexNdx";
		case READ_TEXTURE:		return "a_vertexNdx";
		default:
			DE_ASSERT(DE_FALSE);
			return "";
	}
}

std::string VaryingOutputCountShader::genGeometrySource (const glu::ContextType& contextType, VaryingSource test, int maxEmitCount, bool instanced)
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_GEOMETRY_SHADER}"
			"layout(points" << ((instanced) ? (",invocations=4") : ("")) << ") in;\n"
			"layout(triangle_strip, max_vertices = " << maxEmitCount << ") out;\n";

	if (test == READ_ATTRIBUTE)
		buf <<	"in highp vec4 v_geom_emitCount[];\n";
	else if (test == READ_UNIFORM)
		buf <<	"in highp vec4 v_geom_vertexNdx[];\n"
				"uniform highp ivec4 u_emitCount;\n";
	else
		buf <<	"in highp vec4 v_geom_vertexNdx[];\n"
				"uniform highp sampler2D u_sampler;\n";

	buf <<	"out highp vec4 v_frag_FragColor;\n"
			"\n"
			"void main (void)\n"
			"{\n";

	// emit count

	if (test == READ_ATTRIBUTE)
	{
		buf <<	"	highp vec4 attrEmitCounts = v_geom_emitCount[0];\n"
				"	mediump int emitCount = int(attrEmitCounts[" << ((instanced) ? ("gl_InvocationID") : ("0")) << "]);\n";
	}
	else if (test == READ_UNIFORM)
	{
		buf <<	"	mediump int primitiveNdx = " << ((instanced) ? ("gl_InvocationID") : ("int(v_geom_vertexNdx[0].x)")) << ";\n"
				"	mediump int emitCount = u_emitCount[primitiveNdx];\n";
	}
	else if (test == READ_TEXTURE)
	{
		buf <<	"	highp float primitiveNdx = " << ((instanced) ? ("float(gl_InvocationID)") : ("v_geom_vertexNdx[0].x")) << ";\n"
				"	highp vec2 texCoord = vec2(1.0 / 8.0 + primitiveNdx / 4.0, 0.5);\n"
				"	highp vec4 texColor = texture(u_sampler, texCoord);\n"
				"	mediump int emitCount = 0;\n"
				"	if (texColor.x > 0.0)\n"
				"		emitCount += " << ((EMIT_COUNT_VERTEX_0 == -1) ? (maxEmitCount) : (EMIT_COUNT_VERTEX_0)) << ";\n"
				"	if (texColor.y > 0.0)\n"
				"		emitCount += " << ((EMIT_COUNT_VERTEX_1 == -1) ? (maxEmitCount) : (EMIT_COUNT_VERTEX_1)) << ";\n"
				"	if (texColor.z > 0.0)\n"
				"		emitCount += " << ((EMIT_COUNT_VERTEX_2 == -1) ? (maxEmitCount) : (EMIT_COUNT_VERTEX_2)) << ";\n"
				"	if (texColor.w > 0.0)\n"
				"		emitCount += " << ((EMIT_COUNT_VERTEX_3 == -1) ? (maxEmitCount) : (EMIT_COUNT_VERTEX_3)) << ";\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	// color

	if (test == READ_ATTRIBUTE)
	{
		// We don't want color to be compile time constant
		buf <<	"	highp vec4 color = vec4((emitCount < 10) ? (0.0) : (1.0), (emitCount > 10) ? (0.0) : (1.0), 1.0, 1.0);\n";
	}
	else if (test == READ_UNIFORM || test == READ_TEXTURE)
	{
		buf <<	"\n"
				"	const highp vec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	const highp vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);\n"
				"	const highp vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 colors[4] = vec4[4](red, green, blue, yellow);\n"
				"	highp vec4 color = colors[int(primitiveNdx)];\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	buf <<	"\n"
			"	highp vec4 basePos = " << ((instanced) ? ("gl_in[0].gl_Position + 0.5 * vec4(cos(float(gl_InvocationID)), sin(float(gl_InvocationID)), 0.0, 0.0)") : ("gl_in[0].gl_Position")) << ";\n"
			"	for (mediump int i = 0; i < emitCount / 2; i++)\n"
			"	{\n"
			"		highp float angle = (float(i) + 0.5) / float(emitCount / 2) * 3.142;\n"
			"		gl_Position = basePos + vec4(cos(angle),  sin(angle), 0.0, 0.0) * 0.15;\n"
			"		v_frag_FragColor = color;\n"
			"		EmitVertex();\n"
			"		gl_Position = basePos + vec4(cos(angle), -sin(angle), 0.0, 0.0) * 0.15;\n"
			"		v_frag_FragColor = color;\n"
			"		EmitVertex();\n"
			"	}"
			"}\n";

	return specializeShader(buf.str(), contextType);
}

std::string VaryingOutputCountShader::genVertexSource (const glu::ContextType& contextType, VaryingSource test)
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n";

	if (test == READ_ATTRIBUTE)
	{
		buf << "in highp vec4 a_emitCount;\n";
		buf << "out highp vec4 v_geom_emitCount;\n";
	}
	else if (test == READ_UNIFORM || test == READ_TEXTURE)
	{
		buf << "in highp vec4 a_vertexNdx;\n";
		buf << "out highp vec4 v_geom_vertexNdx;\n";
	}

	buf <<	"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n";

	if (test == READ_ATTRIBUTE)
		buf << "	v_geom_emitCount = a_emitCount;\n";
	else if (test == READ_UNIFORM || test == READ_TEXTURE)
		buf << "	v_geom_vertexNdx = a_vertexNdx;\n";

	buf <<	"}\n";

	return specializeShader(buf.str(), contextType);
}

class InvocationCountShader : public sglr::ShaderProgram
{
public:
	enum OutputCase
	{
		CASE_FIXED_OUTPUT_COUNTS = 0,
		CASE_DIFFERENT_OUTPUT_COUNTS,

		CASE_LAST
	};

						InvocationCountShader		(const glu::ContextType& contextType, int numInvocations, OutputCase testCase);

private:
	void				shadeVertices				(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void				shadeFragments				(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void				shadePrimitives				(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

	static std::string	genGeometrySource			(const glu::ContextType& contextType, int numInvocations, OutputCase testCase);
	static size_t		getNumVertices				(int numInvocations, OutputCase testCase);

	const int			m_numInvocations;
	const OutputCase	m_testCase;
};

InvocationCountShader::InvocationCountShader (const glu::ContextType& contextType, int numInvocations, OutputCase testCase)
	: sglr::ShaderProgram	(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_color", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToGeometryVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::GeometryToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(specializeShader(s_commonShaderSourceVertex, contextType))
							<< sglr::pdec::FragmentSource(specializeShader(s_commonShaderSourceFragment, contextType))
							<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_POINTS,
																	 rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP,
																	 getNumVertices(numInvocations, testCase),
																	 numInvocations)
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, numInvocations, testCase)))
	, m_numInvocations		(numInvocations)
	, m_testCase			(testCase)
{
}

void InvocationCountShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void InvocationCountShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packets[packetNdx], context, 0, fragNdx));
}

void InvocationCountShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(verticesIn);

	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	{
		const float				l_angle		= float(invocationID) / float(m_numInvocations) * 5.5f;
		const float				l_radius	= 0.6f;

		const rr::VertexPacket*	vertex		= packets[packetNdx].vertices[0];

		if (m_testCase == CASE_FIXED_OUTPUT_COUNTS)
		{
			const tcu::Vec4			position0	= vertex->position + tcu::Vec4(deFloatCos(l_angle)      * (l_radius - 0.1f), deFloatSin(l_angle)      * (l_radius - 0.1f), 0.0f, 0.0f);
			const tcu::Vec4			position1	= vertex->position + tcu::Vec4(deFloatCos(l_angle+0.1f) * l_radius,          deFloatSin(l_angle+0.1f) * l_radius,          0.0f, 0.0f);
			const tcu::Vec4			position2	= vertex->position + tcu::Vec4(deFloatCos(l_angle-0.1f) * l_radius,          deFloatSin(l_angle-0.1f) * l_radius,          0.0f, 0.0f);

			rr::GenericVec4			tipColor;
			rr::GenericVec4			baseColor;

			tipColor  = tcu::Vec4(1.0, 1.0, 0.0, 1.0) * packets[packetNdx].vertices[0]->outputs[0].get<float>();
			baseColor = tcu::Vec4(1.0, 0.0, 0.0, 1.0) * packets[packetNdx].vertices[0]->outputs[0].get<float>();

			output.EmitVertex(position0, 0.0f, &tipColor, packets[packetNdx].primitiveIDIn);
			output.EmitVertex(position1, 0.0f, &baseColor, packets[packetNdx].primitiveIDIn);
			output.EmitVertex(position2, 0.0f, &baseColor, packets[packetNdx].primitiveIDIn);
			output.EndPrimitive();
		}
		else if (m_testCase == CASE_DIFFERENT_OUTPUT_COUNTS)
		{
			const tcu::Vec4 color			= tcu::Vec4(float(invocationID % 2), (((invocationID / 2) % 2) == 0) ? (1.0f) : (0.0f), 1.0f, 1.0f);
			const tcu::Vec4 basePosition	= vertex->position + tcu::Vec4(deFloatCos(l_angle) * l_radius, deFloatSin(l_angle) * l_radius, 0.0f, 0.0f);
			const int		numNgonVtx		= invocationID + 3;

			rr::GenericVec4	outColor;
			outColor = color;

			for (int ndx = 0; ndx + 1 < numNgonVtx; ndx += 2)
			{
				const float subAngle = (float(ndx) + 1.0f) / float(numNgonVtx) * 3.141f;

				output.EmitVertex(basePosition + tcu::Vec4(deFloatCos(subAngle) * 0.1f, deFloatSin(subAngle) *  0.1f, 0.0f, 0.0f), 0.0f, &outColor, packets[packetNdx].primitiveIDIn);
				output.EmitVertex(basePosition + tcu::Vec4(deFloatCos(subAngle) * 0.1f, deFloatSin(subAngle) * -0.1f, 0.0f, 0.0f), 0.0f, &outColor, packets[packetNdx].primitiveIDIn);
			}

			if ((numNgonVtx % 2) == 1)
				output.EmitVertex(basePosition + tcu::Vec4(-0.1f, 0.0f, 0.0f, 0.0f), 0.0f, &outColor, packets[packetNdx].primitiveIDIn);

			output.EndPrimitive();
		}
	}
}

std::string InvocationCountShader::genGeometrySource (const glu::ContextType& contextType, int numInvocations, OutputCase testCase)
{
	const int			maxVertices = (int)getNumVertices(numInvocations, testCase);
	std::ostringstream	buf;

	buf	<<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_GEOMETRY_SHADER}"
			"layout(points, invocations = " << numInvocations << ") in;\n"
			"layout(triangle_strip, max_vertices = " << maxVertices << ") out;\n"
			"\n"
			"in highp vec4 v_geom_FragColor[];\n"
			"out highp vec4 v_frag_FragColor;\n"
			"\n"
			"void main ()\n"
			"{\n"
			"	highp float l_angle = float(gl_InvocationID) / float(" << numInvocations << ") * 5.5;\n"
			"	highp float l_radius = 0.6;\n"
			"\n";

	if (testCase == CASE_FIXED_OUTPUT_COUNTS)
	{
		buf <<	"	v_frag_FragColor = vec4(1.0, 1.0, 0.0, 1.0) * v_geom_FragColor[0];\n"
				"	gl_Position = gl_in[0].gl_Position + vec4(cos(l_angle) * (l_radius - 0.1), sin(l_angle) * (l_radius - 0.1), 0.0, 0.0);\n"
				"	EmitVertex();\n"
				"\n"
				"	v_frag_FragColor = vec4(1.0, 0.0, 0.0, 1.0) * v_geom_FragColor[0];\n"
				"	gl_Position = gl_in[0].gl_Position + vec4(cos(l_angle+0.1) * l_radius, sin(l_angle+0.1) * l_radius, 0.0, 0.0);\n"
				"	EmitVertex();\n"
				"\n"
				"	v_frag_FragColor = vec4(1.0, 0.0, 0.0, 1.0) * v_geom_FragColor[0];\n"
				"	gl_Position = gl_in[0].gl_Position + vec4(cos(l_angle-0.1) * l_radius, sin(l_angle-0.1) * l_radius, 0.0, 0.0);\n"
				"	EmitVertex();\n";
	}
	else if (testCase == CASE_DIFFERENT_OUTPUT_COUNTS)
	{
		buf <<	"	highp vec4 l_color = vec4(float(gl_InvocationID % 2), (((gl_InvocationID / 2) % 2) == 0) ? (1.0) : (0.0), 1.0, 1.0);\n"
				"	highp vec4 basePosition = gl_in[0].gl_Position + vec4(cos(l_angle) * l_radius, sin(l_angle) * l_radius, 0.0, 0.0);\n"
				"	mediump int numNgonVtx = gl_InvocationID + 3;\n"
				"\n"
				"	for (int ndx = 0; ndx + 1 < numNgonVtx; ndx += 2)\n"
				"	{\n"
				"		highp float sub_angle = (float(ndx) + 1.0) / float(numNgonVtx) * 3.141;\n"
				"\n"
				"		v_frag_FragColor = l_color;\n"
				"		gl_Position = basePosition + vec4(cos(sub_angle) * 0.1, sin(sub_angle) * 0.1, 0.0, 0.0);\n"
				"		EmitVertex();\n"
				"\n"
				"		v_frag_FragColor = l_color;\n"
				"		gl_Position = basePosition + vec4(cos(sub_angle) * 0.1, sin(sub_angle) * -0.1, 0.0, 0.0);\n"
				"		EmitVertex();\n"
				"	}\n"
				"	if ((numNgonVtx % 2) == 1)\n"
				"	{\n"
				"		v_frag_FragColor = l_color;\n"
				"		gl_Position = basePosition + vec4(-0.1, 0.0, 0.0, 0.0);\n"
				"		EmitVertex();\n"
				"	}\n";
	}
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), contextType);
}

size_t InvocationCountShader::getNumVertices (int numInvocations, OutputCase testCase)
{
	switch (testCase)
	{
		case CASE_FIXED_OUTPUT_COUNTS:			return 3;
		case CASE_DIFFERENT_OUTPUT_COUNTS:		return (size_t)(2 + numInvocations);
		default:
			DE_ASSERT(false);
			return 0;
	}
}

class InstancedExpansionShader : public sglr::ShaderProgram
{
public:
								InstancedExpansionShader	(const glu::ContextType& contextType, int numInvocations);

private:
	void						shadeVertices				(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void						shadeFragments				(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
	void						shadePrimitives				(rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const;

	static std::string			genVertexSource				(const glu::ContextType& contextType);
	static std::string			genFragmentSource			(const glu::ContextType& contextType);
	static std::string			genGeometrySource			(const glu::ContextType& contextType, int numInvocations);

	const int					m_numInvocations;
};

InstancedExpansionShader::InstancedExpansionShader (const glu::ContextType& contextType, int numInvocations)
	: sglr::ShaderProgram	(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_offset", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource(genVertexSource(contextType))
							<< sglr::pdec::FragmentSource(genFragmentSource(contextType))
							<< sglr::pdec::GeometryShaderDeclaration(rr::GEOMETRYSHADERINPUTTYPE_POINTS,
																	 rr::GEOMETRYSHADEROUTPUTTYPE_TRIANGLE_STRIP,
																	 4,
																	 numInvocations)
							<< sglr::pdec::GeometrySource(genGeometrySource(contextType, numInvocations)))
	, m_numInvocations		(numInvocations)
{
}

void InstancedExpansionShader::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position =	rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx) +
									rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void InstancedExpansionShader::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	DE_UNREF(packets);

	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void InstancedExpansionShader::shadePrimitives (rr::GeometryEmitter& output, int verticesIn, const rr::PrimitivePacket* packets, const int numPackets, int invocationID) const
{
	DE_UNREF(verticesIn);

	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	{
		const rr::VertexPacket*	vertex			= packets[packetNdx].vertices[0];
		const tcu::Vec4			basePosition	= vertex->position;
		const float				phase			= float(invocationID) / float(m_numInvocations) * 6.3f;
		const tcu::Vec4			centerPosition	= basePosition + tcu::Vec4(deFloatCos(phase), deFloatSin(phase), 0.0f, 0.0f) * 0.1f;

		output.EmitVertex(centerPosition + tcu::Vec4( 0.0f,  -0.1f, 0.0f, 0.0f), 0.0f, DE_NULL, packets[packetNdx].primitiveIDIn);
		output.EmitVertex(centerPosition + tcu::Vec4(-0.05f,  0.0f, 0.0f, 0.0f), 0.0f, DE_NULL, packets[packetNdx].primitiveIDIn);
		output.EmitVertex(centerPosition + tcu::Vec4( 0.05f,  0.0f, 0.0f, 0.0f), 0.0f, DE_NULL, packets[packetNdx].primitiveIDIn);
		output.EndPrimitive();
	}
}

std::string InstancedExpansionShader::genVertexSource (const glu::ContextType& contextType)
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"in highp vec4 a_offset;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position + a_offset;\n"
			"}\n";

	return specializeShader(buf.str(), contextType);
}

std::string InstancedExpansionShader::genFragmentSource (const glu::ContextType& contextType)
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
			"}\n";

	return specializeShader(buf.str(), contextType);
}

std::string InstancedExpansionShader::genGeometrySource (const glu::ContextType& contextType, int numInvocations)
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_GEOMETRY_SHADER}"
			"layout(points,invocations=" << numInvocations << ") in;\n"
			"layout(triangle_strip, max_vertices = 3) out;\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	highp vec4 basePosition = gl_in[0].gl_Position;\n"
			"	highp float phase = float(gl_InvocationID) / float(" << numInvocations << ") * 6.3;\n"
			"	highp vec4 centerPosition = basePosition + 0.1 * vec4(cos(phase), sin(phase), 0.0, 0.0);\n"
			"\n"
			"	gl_Position = centerPosition + vec4( 0.00, -0.1, 0.0, 0.0);\n"
			"	EmitVertex();\n"
			"	gl_Position = centerPosition + vec4(-0.05,  0.0, 0.0, 0.0);\n"
			"	EmitVertex();\n"
			"	gl_Position = centerPosition + vec4( 0.05,  0.0, 0.0, 0.0);\n"
			"	EmitVertex();\n"
			"}\n";

	return specializeShader(buf.str(), contextType);
}

class GeometryShaderRenderTest : public TestCase
{
public:
	enum Flag
	{
		FLAG_DRAW_INSTANCED		= 1,
		FLAG_USE_INDICES		= 2,
		FLAG_USE_RESTART_INDEX	= 4,
	};

									GeometryShaderRenderTest	(Context& context, const char* name, const char* desc, GLenum inputPrimitives, GLenum outputPrimitives, const char* dataAttributeName, int flags = 0);
	virtual							~GeometryShaderRenderTest	(void);

	void							init						(void);
	void							deinit						(void);

	IterateResult					iterate						(void);
	bool							compare						(void);

	virtual sglr::ShaderProgram&	getProgram					(void) = 0;

protected:
	virtual void					genVertexAttribData			(void);
	void							renderWithContext			(sglr::Context& ctx, sglr::ShaderProgram& program, tcu::Surface& dstSurface);
	virtual void					preRender					(sglr::Context& ctx, GLuint programID);
	virtual void					postRender					(sglr::Context& ctx, GLuint programID);

	int								m_numDrawVertices;
	int								m_numDrawInstances;
	int								m_vertexAttrDivisor;

	const GLenum					m_inputPrimitives;
	const GLenum					m_outputPrimitives;
	const char* const				m_dataAttributeName;
	const int						m_flags;

	tcu::IVec2						m_viewportSize;
	int								m_interationCount;

	tcu::Surface*					m_glResult;
	tcu::Surface*					m_refResult;

	sglr::ReferenceContextBuffers*	m_refBuffers;
	sglr::ReferenceContext*			m_refContext;
	sglr::Context*					m_glContext;

	std::vector<tcu::Vec4>			m_vertexPosData;
	std::vector<tcu::Vec4>			m_vertexAttrData;
	std::vector<deUint16>			m_indices;
};

GeometryShaderRenderTest::GeometryShaderRenderTest (Context& context, const char* name, const char* desc, GLenum inputPrimitives, GLenum outputPrimitives, const char* dataAttributeName, int flags)
	: TestCase				(context, name, desc)
	, m_numDrawVertices		(0)
	, m_numDrawInstances	(0)
	, m_vertexAttrDivisor	(0)
	, m_inputPrimitives		(inputPrimitives)
	, m_outputPrimitives	(outputPrimitives)
	, m_dataAttributeName	(dataAttributeName)
	, m_flags				(flags)
	, m_viewportSize		(TEST_CANVAS_SIZE, TEST_CANVAS_SIZE)
	, m_interationCount		(0)
	, m_glResult			(DE_NULL)
	, m_refResult			(DE_NULL)
	, m_refBuffers			(DE_NULL)
	, m_refContext			(DE_NULL)
	, m_glContext			(DE_NULL)
{
	// Disallow instanced drawElements
	DE_ASSERT(((m_flags & FLAG_DRAW_INSTANCED) == 0) || ((m_flags & FLAG_USE_INDICES) == 0));
	// Disallow restart without indices
	DE_ASSERT(!(((m_flags & FLAG_USE_RESTART_INDEX) != 0) && ((m_flags & FLAG_USE_INDICES) == 0)));
}

GeometryShaderRenderTest::~GeometryShaderRenderTest (void)
{
	deinit();
}

void GeometryShaderRenderTest::init (void)
{
	// requirements
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	// gen resources
	{
		sglr::ReferenceContextLimits limits;

		m_glResult		= new tcu::Surface(m_viewportSize.x(), m_viewportSize.y());
		m_refResult		= new tcu::Surface(m_viewportSize.x(), m_viewportSize.y());

		m_refBuffers	= new sglr::ReferenceContextBuffers(m_context.getRenderTarget().getPixelFormat(), m_context.getRenderTarget().getDepthBits(), 0, m_viewportSize.x(), m_viewportSize.y());
		m_refContext	= new sglr::ReferenceContext(limits, m_refBuffers->getColorbuffer(), m_refBuffers->getDepthbuffer(), m_refBuffers->getStencilbuffer());
		m_glContext		= new sglr::GLContext(m_context.getRenderContext(), m_testCtx.getLog(), sglr::GLCONTEXT_LOG_CALLS | sglr::GLCONTEXT_LOG_PROGRAMS, tcu::IVec4(0, 0, m_viewportSize.x(), m_viewportSize.y()));
	}
}

void GeometryShaderRenderTest::deinit (void)
{
	delete m_glResult;
	delete m_refResult;

	m_glResult = DE_NULL;
	m_refResult = DE_NULL;

	delete m_refContext;
	delete m_glContext;
	delete m_refBuffers;

	m_refBuffers = DE_NULL;
	m_refContext = DE_NULL;
	m_glContext = DE_NULL;
}

tcu::TestCase::IterateResult GeometryShaderRenderTest::iterate (void)
{
	// init() must be called
	DE_ASSERT(m_glContext);
	DE_ASSERT(m_refContext);

	const int iteration = m_interationCount++;

	if (iteration == 0)
	{
		// Check requirements
		const int width	 = m_context.getRenderTarget().getWidth();
		const int height = m_context.getRenderTarget().getHeight();

		if (width < m_viewportSize.x() || height < m_viewportSize.y())
			throw tcu::NotSupportedError(std::string("Render target size must be at least ") + de::toString(m_viewportSize.x()) + "x" + de::toString(m_viewportSize.y()));

		// Gen data
		genVertexAttribData();

		return CONTINUE;
	}
	else if (iteration == 1)
	{
		// Render
		sglr::ShaderProgram& program = getProgram();

		renderWithContext(*m_glContext, program, *m_glResult);
		renderWithContext(*m_refContext, program, *m_refResult);

		return CONTINUE;
	}
	else
	{
		if (compare())
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");

		return STOP;
	}
}

bool GeometryShaderRenderTest::compare (void)
{
	using tcu::TestLog;

	if (m_context.getRenderTarget().getNumSamples() > 1)
	{
		return tcu::fuzzyCompare(m_testCtx.getLog(), "Compare Results", "Compare Results", m_refResult->getAccess(), m_glResult->getAccess(), 0.02f, tcu::COMPARE_LOG_RESULT);
	}
	else
	{
		tcu::Surface	errorMask				(m_viewportSize.x(), m_viewportSize.y());
		const tcu::RGBA	green					(0, 255, 0, 255);
		const tcu::RGBA	red						(255, 0, 0, 255);
		const int		colorComponentThreshold	= 20;
		bool			testResult				= true;

		for (int x = 0; x < m_viewportSize.x(); ++x)
		for (int y = 0; y < m_viewportSize.y(); ++y)
		{
			if (x == 0 || y == 0 || x + 1 == m_viewportSize.x() || y + 1 == m_viewportSize.y())
			{
				// Mark edge pixels as correct since their neighbourhood is undefined
				errorMask.setPixel(x, y, green);
			}
			else
			{
				const tcu::RGBA	refcolor	= m_refResult->getPixel(x, y);
				bool			found		= false;

				// Got to find similar pixel near this pixel (3x3 kernel)
				for (int dx = -1; dx <= 1; ++dx)
				for (int dy = -1; dy <= 1; ++dy)
				{
					const tcu::RGBA		testColor	= m_glResult->getPixel(x + dx, y + dy);
					const tcu::IVec4	colDiff		= tcu::abs(testColor.toIVec() - refcolor.toIVec());

					const int			maxColDiff	= de::max(de::max(colDiff.x(), colDiff.y()), colDiff.z()); // check RGB channels

					if (maxColDiff <= colorComponentThreshold)
						found = true;
				}

				if (!found)
					testResult = false;

				errorMask.setPixel(x, y, (found) ? (green) : (red));
			}
		}

		if (testResult)
		{
			m_testCtx.getLog()	<< TestLog::ImageSet("Compare result", "Result of rendering")
								<< TestLog::Image("Result", "Result", *m_glResult)
								<< TestLog::EndImageSet;
			m_testCtx.getLog() << TestLog::Message << "Image compare ok." << TestLog::EndMessage;
		}
		else
		{
			m_testCtx.getLog()	<< TestLog::ImageSet("Compare result", "Result of rendering")
								<< TestLog::Image("Result",		"Result",		*m_glResult)
								<< TestLog::Image("Reference",	"Reference",	*m_refResult)
								<< TestLog::Image("ErrorMask",	"Error mask",	errorMask)
								<< TestLog::EndImageSet;
			m_testCtx.getLog() << TestLog::Message << "Image compare failed." << TestLog::EndMessage;
		}

		return testResult;
	}
}

void GeometryShaderRenderTest::genVertexAttribData (void)
{
	// Create 1 X 2 grid in triangle strip adjacent - order
	const float scale = 0.3f;
	const tcu::Vec4 offset(-0.5f, -0.2f, 0.0f, 1.0f);

	m_vertexPosData.resize(12);
	m_vertexPosData[ 0] = tcu::Vec4( 0,  0, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 1] = tcu::Vec4(-1, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 2] = tcu::Vec4( 0, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 3] = tcu::Vec4( 1,  1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 4] = tcu::Vec4( 1,  0, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 5] = tcu::Vec4( 0, -2, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 6] = tcu::Vec4( 1, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 7] = tcu::Vec4( 2,  1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 8] = tcu::Vec4( 2,  0, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 9] = tcu::Vec4( 1, -2, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[10] = tcu::Vec4( 2, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[11] = tcu::Vec4( 3,  0, 0.0f, 0.0f) * scale + offset;

	// Red and white
	m_vertexAttrData.resize(12);
	for (int i = 0; i < 12; ++i)
		m_vertexAttrData[i] = (i % 2 == 0) ? tcu::Vec4(1, 1, 1, 1) : tcu::Vec4(1, 0, 0, 1);

	m_numDrawVertices = 12;
}

void GeometryShaderRenderTest::renderWithContext (sglr::Context& ctx, sglr::ShaderProgram& program, tcu::Surface& dstSurface)
{
#define CHECK_GL_CTX_ERRORS() glu::checkError(ctx.getError(), DE_NULL, __FILE__, __LINE__)

	const GLuint	programId		= ctx.createProgram(&program);
	const GLint		attrPosLoc		= ctx.getAttribLocation(programId, "a_position");
	const GLint		attrColLoc		= ctx.getAttribLocation(programId, m_dataAttributeName);
	GLuint			vaoId			= 0;
	GLuint			vertexPosBuf	= 0;
	GLuint			vertexAttrBuf	= 0;
	GLuint			elementArrayBuf	= 0;

	ctx.genVertexArrays(1, &vaoId);
	ctx.bindVertexArray(vaoId);

	if (attrPosLoc != -1)
	{
		ctx.genBuffers(1, &vertexPosBuf);
		ctx.bindBuffer(GL_ARRAY_BUFFER, vertexPosBuf);
		ctx.bufferData(GL_ARRAY_BUFFER, m_vertexPosData.size() * sizeof(tcu::Vec4), &m_vertexPosData[0], GL_STATIC_DRAW);
		ctx.vertexAttribPointer(attrPosLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		ctx.enableVertexAttribArray(attrPosLoc);
	}

	if (attrColLoc != -1)
	{
		ctx.genBuffers(1, &vertexAttrBuf);
		ctx.bindBuffer(GL_ARRAY_BUFFER, vertexAttrBuf);
		ctx.bufferData(GL_ARRAY_BUFFER, m_vertexAttrData.size() * sizeof(tcu::Vec4), &m_vertexAttrData[0], GL_STATIC_DRAW);
		ctx.vertexAttribPointer(attrColLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		ctx.enableVertexAttribArray(attrColLoc);

		if (m_vertexAttrDivisor)
			ctx.vertexAttribDivisor(attrColLoc, m_vertexAttrDivisor);
	}

	if (m_flags & FLAG_USE_INDICES)
	{
		ctx.genBuffers(1, &elementArrayBuf);
		ctx.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuf);
		ctx.bufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(deUint16), &m_indices[0], GL_STATIC_DRAW);
	}

	ctx.clearColor(0, 0, 0, 1);
	ctx.clear(GL_COLOR_BUFFER_BIT);

	ctx.viewport(0, 0, m_viewportSize.x(), m_viewportSize.y());
	CHECK_GL_CTX_ERRORS();

	ctx.useProgram(programId);
	CHECK_GL_CTX_ERRORS();

	preRender(ctx, programId);
	CHECK_GL_CTX_ERRORS();

	if (m_flags & FLAG_USE_RESTART_INDEX)
	{
		ctx.enable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		CHECK_GL_CTX_ERRORS();
	}

	if (m_flags & FLAG_USE_INDICES)
		ctx.drawElements(m_inputPrimitives, m_numDrawVertices, GL_UNSIGNED_SHORT, DE_NULL);
	else if (m_flags & FLAG_DRAW_INSTANCED)
		ctx.drawArraysInstanced(m_inputPrimitives, 0, m_numDrawVertices, m_numDrawInstances);
	else
		ctx.drawArrays(m_inputPrimitives, 0, m_numDrawVertices);

	CHECK_GL_CTX_ERRORS();

	if (m_flags & FLAG_USE_RESTART_INDEX)
	{
		ctx.disable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		CHECK_GL_CTX_ERRORS();
	}

	postRender(ctx, programId);
	CHECK_GL_CTX_ERRORS();

	ctx.useProgram(0);

	if (attrPosLoc != -1)
		ctx.disableVertexAttribArray(attrPosLoc);
	if (attrColLoc != -1)
		ctx.disableVertexAttribArray(attrColLoc);

	if (vertexPosBuf)
		ctx.deleteBuffers(1, &vertexPosBuf);
	if (vertexAttrBuf)
		ctx.deleteBuffers(1, &vertexAttrBuf);
	if (elementArrayBuf)
		ctx.deleteBuffers(1, &elementArrayBuf);

	ctx.deleteVertexArrays(1, &vaoId);

	CHECK_GL_CTX_ERRORS();

	ctx.finish();
	ctx.readPixels(dstSurface, 0, 0, m_viewportSize.x(), m_viewportSize.y());

#undef CHECK_GL_CTX_ERRORS
}

void GeometryShaderRenderTest::preRender (sglr::Context& ctx, GLuint programID)
{
	DE_UNREF(ctx);
	DE_UNREF(programID);
}

void GeometryShaderRenderTest::postRender (sglr::Context& ctx, GLuint programID)
{
	DE_UNREF(ctx);
	DE_UNREF(programID);
}

class GeometryExpanderRenderTest : public GeometryShaderRenderTest
{
public:
									GeometryExpanderRenderTest	(Context& context, const char* name, const char* desc, GLenum inputPrimitives, GLenum outputPrimitives);
	virtual							~GeometryExpanderRenderTest	(void);

	sglr::ShaderProgram&			getProgram					(void);

private:
	void							init						(void);
	void							deinit						(void);
	VertexExpanderShader*			m_program;
};

GeometryExpanderRenderTest::GeometryExpanderRenderTest (Context& context, const char* name, const char* desc, GLenum inputPrimitives, GLenum outputPrimitives)
	: GeometryShaderRenderTest	(context, name, desc, inputPrimitives, outputPrimitives, "a_color")
	, m_program					(DE_NULL)
{
}

GeometryExpanderRenderTest::~GeometryExpanderRenderTest (void)
{
}

void GeometryExpanderRenderTest::init (void)
{
	m_program = new VertexExpanderShader(m_context.getRenderContext().getType(), sglr::rr_util::mapGLGeometryShaderInputType(m_inputPrimitives), sglr::rr_util::mapGLGeometryShaderOutputType(m_outputPrimitives));

	GeometryShaderRenderTest::init();
}

void GeometryExpanderRenderTest::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

sglr::ShaderProgram& GeometryExpanderRenderTest::getProgram (void)
{
	return *m_program;
}

class EmitTest : public GeometryShaderRenderTest
{
public:
							EmitTest				(Context& context, const char* name, const char* desc, int emitCountA, int endCountA, int emitCountB, int endCountB, GLenum outputType);

	sglr::ShaderProgram&	getProgram				(void);
private:
	void					init					(void);
	void					deinit					(void);
	void					genVertexAttribData		(void);

	VertexEmitterShader*	m_program;
	int						m_emitCountA;
	int						m_endCountA;
	int						m_emitCountB;
	int						m_endCountB;
	GLenum					m_outputType;
};

EmitTest::EmitTest (Context& context, const char* name, const char* desc, int emitCountA, int endCountA, int emitCountB, int endCountB, GLenum outputType)
	: GeometryShaderRenderTest	(context, name, desc, GL_POINTS, outputType, "a_color")
	, m_program					(DE_NULL)
	, m_emitCountA				(emitCountA)
	, m_endCountA				(endCountA)
	, m_emitCountB				(emitCountB)
	, m_endCountB				(endCountB)
	, m_outputType				(outputType)
{
}

void EmitTest::init(void)
{
	m_program = new VertexEmitterShader(m_context.getRenderContext().getType(), m_emitCountA, m_endCountA, m_emitCountB, m_endCountB, sglr::rr_util::mapGLGeometryShaderOutputType(m_outputType));

	GeometryShaderRenderTest::init();
}

void EmitTest::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

sglr::ShaderProgram& EmitTest::getProgram (void)
{
	return *m_program;
}

void EmitTest::genVertexAttribData (void)
{
	m_vertexPosData.resize(1);
	m_vertexPosData[0] = tcu::Vec4(0, 0, 0, 1);

	m_vertexAttrData.resize(1);
	m_vertexAttrData[0] = tcu::Vec4(1, 1, 1, 1);

	m_numDrawVertices = 1;
}

class VaryingTest : public GeometryShaderRenderTest
{
public:
							VaryingTest				(Context& context, const char* name, const char* desc, int vertexOut, int geometryOut);

	sglr::ShaderProgram&	getProgram				(void);
private:
	void					init					(void);
	void					deinit					(void);
	void					genVertexAttribData		(void);

	VertexVaryingShader*	m_program;
	int						m_vertexOut;
	int						m_geometryOut;
};

VaryingTest::VaryingTest (Context& context, const char* name, const char* desc, int vertexOut, int geometryOut)
	: GeometryShaderRenderTest	(context, name, desc, GL_TRIANGLES, GL_TRIANGLE_STRIP, "a_color")
	, m_program					(DE_NULL)
	, m_vertexOut				(vertexOut)
	, m_geometryOut				(geometryOut)
{
}

void VaryingTest::init (void)
{
	m_program = new VertexVaryingShader(m_context.getRenderContext().getType(), m_vertexOut, m_geometryOut);

	GeometryShaderRenderTest::init();
}

void VaryingTest::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

sglr::ShaderProgram& VaryingTest::getProgram (void)
{
	return *m_program;
}

void VaryingTest::genVertexAttribData (void)
{
	m_vertexPosData.resize(3);
	m_vertexPosData[0] = tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4(0.0f, 0.5f, 0.0f, 1.0f);
	m_vertexPosData[2] = tcu::Vec4(0.1f, 0.0f, 0.0f, 1.0f);

	m_vertexAttrData.resize(3);
	m_vertexAttrData[0] = tcu::Vec4(0.7f, 0.4f, 0.6f, 1.0f);
	m_vertexAttrData[1] = tcu::Vec4(0.9f, 0.2f, 0.5f, 1.0f);
	m_vertexAttrData[2] = tcu::Vec4(0.1f, 0.8f, 0.3f, 1.0f);

	m_numDrawVertices = 3;
}

class TriangleStripAdjacencyVertexCountTest : public GeometryExpanderRenderTest
{
public:
				TriangleStripAdjacencyVertexCountTest	(Context& context, const char* name, const char* desc, int numInputVertices);

private:
	void		genVertexAttribData						(void);

	int			m_numInputVertices;
};

TriangleStripAdjacencyVertexCountTest::TriangleStripAdjacencyVertexCountTest (Context& context, const char* name, const char* desc, int numInputVertices)
	: GeometryExpanderRenderTest	(context, name, desc, GL_TRIANGLE_STRIP_ADJACENCY, GL_TRIANGLE_STRIP)
	, m_numInputVertices			(numInputVertices)
{
}

void TriangleStripAdjacencyVertexCountTest::genVertexAttribData (void)
{
	this->GeometryShaderRenderTest::genVertexAttribData();
	m_numDrawVertices = m_numInputVertices;
}

class NegativeDrawCase : public TestCase
{
public:
							NegativeDrawCase	(Context& context, const char* name, const char* desc, GLenum inputType, GLenum inputPrimitives);
							~NegativeDrawCase	(void);

	void					init				(void);
	void					deinit				(void);

	IterateResult			iterate				(void);

private:
	sglr::Context*			m_ctx;
	VertexExpanderShader*	m_program;
	GLenum					m_inputType;
	GLenum					m_inputPrimitives;
};

NegativeDrawCase::NegativeDrawCase (Context& context, const char* name, const char* desc, GLenum inputType, GLenum inputPrimitives)
	: TestCase			(context, name, desc)
	, m_ctx				(DE_NULL)
	, m_program			(DE_NULL)
	, m_inputType		(inputType)
	, m_inputPrimitives	(inputPrimitives)
{
}

NegativeDrawCase::~NegativeDrawCase (void)
{
	deinit();
}

void NegativeDrawCase::init (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	m_ctx		= new sglr::GLContext(m_context.getRenderContext(), m_testCtx.getLog(), sglr::GLCONTEXT_LOG_CALLS | sglr::GLCONTEXT_LOG_PROGRAMS, tcu::IVec4(0, 0, 1, 1));
	m_program	= new VertexExpanderShader(m_context.getRenderContext().getType() , sglr::rr_util::mapGLGeometryShaderInputType(m_inputType), rr::GEOMETRYSHADEROUTPUTTYPE_POINTS);
}

void NegativeDrawCase::deinit (void)
{
	delete m_ctx;
	delete m_program;

	m_ctx = NULL;
	m_program = DE_NULL;
}

NegativeDrawCase::IterateResult NegativeDrawCase::iterate (void)
{
	const GLuint	programId		= m_ctx->createProgram(m_program);
	const GLint		attrPosLoc		= m_ctx->getAttribLocation(programId, "a_position");
	const tcu::Vec4 vertexPosData	(0, 0, 0, 1);

	GLuint vaoId		= 0;
	GLuint vertexPosBuf = 0;
	GLenum errorCode	= 0;

	m_ctx->genVertexArrays(1, &vaoId);
	m_ctx->bindVertexArray(vaoId);

	m_ctx->genBuffers(1, &vertexPosBuf);
	m_ctx->bindBuffer(GL_ARRAY_BUFFER, vertexPosBuf);
	m_ctx->bufferData(GL_ARRAY_BUFFER, sizeof(tcu::Vec4), vertexPosData.m_data, GL_STATIC_DRAW);
	m_ctx->vertexAttribPointer(attrPosLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	m_ctx->enableVertexAttribArray(attrPosLoc);

	m_ctx->clearColor(0, 0, 0, 1);
	m_ctx->clear(GL_COLOR_BUFFER_BIT);

	m_ctx->viewport(0, 0, 1, 1);

	m_ctx->useProgram(programId);

	// no errors before
	glu::checkError(m_ctx->getError(), "", __FILE__, __LINE__);

	m_ctx->drawArrays(m_inputPrimitives, 0, 1);

	errorCode = m_ctx->getError();
	if (errorCode != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Expected GL_INVALID_OPERATION, got " << glu::getErrorStr(errorCode) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got wrong error code");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	m_ctx->useProgram(0);

	m_ctx->disableVertexAttribArray(attrPosLoc);
	m_ctx->deleteBuffers(1, &vertexPosBuf);

	m_ctx->deleteVertexArrays(1, &vaoId);

	return STOP;
}

class OutputCountCase : public GeometryShaderRenderTest
{
public:
									OutputCountCase			(Context& context, const char* name, const char* desc, const OutputCountPatternSpec&);
private:
	void							init					(void);
	void							deinit					(void);

	sglr::ShaderProgram&			getProgram				(void);
	void							genVertexAttribData		(void);

	const int						m_primitiveCount;
	OutputCountShader*				m_program;
	OutputCountPatternSpec			m_spec;
};

OutputCountCase::OutputCountCase (Context& context, const char* name, const char* desc, const OutputCountPatternSpec& spec)
	: GeometryShaderRenderTest	(context, name, desc, GL_POINTS, GL_TRIANGLE_STRIP, "a_color")
	, m_primitiveCount			((int)spec.pattern.size())
	, m_program					(DE_NULL)
	, m_spec					(spec)
{
}

void OutputCountCase::init (void)
{
	// Check requirements and adapt to them
	{
		const int	componentsPerVertex	= 4 + 4; // vec4 pos, vec4 color
		const int	testVertices		= *std::max_element(m_spec.pattern.begin(), m_spec.pattern.end());
		glw::GLint	maxVertices			= 0;
		glw::GLint	maxComponents		= 0;

		// check the extension before querying anything
		if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
			TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

		m_context.getRenderContext().getFunctions().getIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxVertices);
		m_context.getRenderContext().getFunctions().getIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &maxComponents);

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_OUTPUT_VERTICES = " << maxVertices << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS = " << maxComponents << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Components per vertex = " << componentsPerVertex << tcu::TestLog::EndMessage;

		if (testVertices == -1)
		{
			// "max vertices"-case
			DE_ASSERT((int)m_spec.pattern.size() == 1);
			m_spec.pattern[0] = de::min(maxVertices, maxComponents / componentsPerVertex);

			// make sure size is dividable by 2, as OutputShader requires
			m_spec.pattern[0] = m_spec.pattern[0] & ~0x00000001;

			if (m_spec.pattern[0] == 0)
				throw tcu::InternalError("Pattern size is invalid.");
		}
		else
		{
			// normal case
			if (testVertices > maxVertices)
				throw tcu::NotSupportedError(de::toString(testVertices) + " output vertices required.");
			if (testVertices * componentsPerVertex > maxComponents)
				throw tcu::NotSupportedError(de::toString(testVertices * componentsPerVertex) + " output components required.");
		}
	}

	// Log what the test tries to do

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering " << (int)m_spec.pattern.size() << " row(s).\nOne geometry shader invocation generates one row.\nRow sizes:" << tcu::TestLog::EndMessage;
	for (int ndx = 0; ndx < (int)m_spec.pattern.size(); ++ndx)
		m_testCtx.getLog() << tcu::TestLog::Message << "Row " << ndx << ": " << m_spec.pattern[ndx] << " vertices." << tcu::TestLog::EndMessage;

	// Gen shader
	DE_ASSERT(!m_program);
	m_program = new OutputCountShader(m_context.getRenderContext().getType(), m_spec);

	// Case init
	GeometryShaderRenderTest::init();
}

void OutputCountCase::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

sglr::ShaderProgram& OutputCountCase::getProgram (void)
{
	return *m_program;
}

void OutputCountCase::genVertexAttribData (void)
{
	m_vertexPosData.resize(m_primitiveCount);
	m_vertexAttrData.resize(m_primitiveCount);

	for (int ndx = 0; ndx < m_primitiveCount; ++ndx)
	{
		m_vertexPosData[ndx] = tcu::Vec4(-1.0f, ((float)ndx) / (float)m_primitiveCount * 2.0f - 1.0f, 0.0f, 1.0f);
		m_vertexAttrData[ndx] = (ndx % 2 == 0) ? tcu::Vec4(1, 1, 1, 1) : tcu::Vec4(1, 0, 0, 1);
	}

	m_numDrawVertices = m_primitiveCount;
}

class BuiltinVariableRenderTest : public GeometryShaderRenderTest
{
public:
												BuiltinVariableRenderTest	(Context& context, const char* name, const char* desc, BuiltinVariableShader::VariableTest test, int flags = 0);

private:
	void										init						(void);
	void										deinit						(void);

	sglr::ShaderProgram&						getProgram					(void);
	void										genVertexAttribData			(void);

	BuiltinVariableShader*						m_program;
	const BuiltinVariableShader::VariableTest	m_test;
};

BuiltinVariableRenderTest::BuiltinVariableRenderTest (Context& context, const char* name, const char* desc, BuiltinVariableShader::VariableTest test, int flags)
	: GeometryShaderRenderTest	(context, name, desc, GL_POINTS, GL_POINTS, BuiltinVariableShader::getTestAttributeName(test), flags)
	, m_program					(DE_NULL)
	, m_test					(test)
{
}

void BuiltinVariableRenderTest::init (void)
{
	// Requirements
	if (m_test == BuiltinVariableShader::TEST_POINT_SIZE)
	{
		const float requiredPointSize = 5.0f;

		tcu::Vec2 range = tcu::Vec2(1.0f, 1.0f);

		if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 4)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_point_size"))
			TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_point_size extension.");

		m_context.getRenderContext().getFunctions().getFloatv(GL_ALIASED_POINT_SIZE_RANGE, range.getPtr());
		if (range.y() < requiredPointSize)
			throw tcu::NotSupportedError("Test case requires point size " + de::toString(requiredPointSize));
	}

	m_program = new BuiltinVariableShader(m_context.getRenderContext().getType(), m_test);

	// Shader init
	GeometryShaderRenderTest::init();
}

void BuiltinVariableRenderTest::deinit(void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}


sglr::ShaderProgram& BuiltinVariableRenderTest::getProgram (void)
{
	return *m_program;
}

void BuiltinVariableRenderTest::genVertexAttribData (void)
{
	m_vertexPosData.resize(4);
	m_vertexPosData[0] = tcu::Vec4( 0.5f,  0.0f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4( 0.0f,  0.5f, 0.0f, 1.0f);
	m_vertexPosData[2] = tcu::Vec4(-0.7f, -0.1f, 0.0f, 1.0f);
	m_vertexPosData[3] = tcu::Vec4(-0.1f, -0.7f, 0.0f, 1.0f);

	m_vertexAttrData.resize(4);
	m_vertexAttrData[0] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[1] = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[2] = tcu::Vec4(2.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[3] = tcu::Vec4(3.0f, 0.0f, 0.0f, 0.0f);

	// Only used by primitive ID restart test
	m_indices.resize(4);
	m_indices[0] = 3;
	m_indices[1] = 2;
	m_indices[2] = 0xFFFF; // restart
	m_indices[3] = 1;

	m_numDrawVertices = 4;
}

class LayeredRenderCase : public TestCase
{
public:
	enum LayeredRenderTargetType
	{
		TARGET_CUBE = 0,
		TARGET_3D,
		TARGET_1D_ARRAY,
		TARGET_2D_ARRAY,
		TARGET_2D_MS_ARRAY,

		TARGET_LAST
	};
	enum TestType
	{
		TEST_DEFAULT_LAYER,						// !< draw to default layer
		TEST_SINGLE_LAYER,						// !< draw to single layer
		TEST_ALL_LAYERS,						// !< draw all layers
		TEST_DIFFERENT_LAYERS,					// !< draw different content to different layers
		TEST_INVOCATION_PER_LAYER,				// !< draw to all layers, one invocation per layer
		TEST_MULTIPLE_LAYERS_PER_INVOCATION,	// !< draw to all layers, multiple invocations write to multiple layers
		TEST_LAYER_ID,							// !< draw to all layers, verify gl_Layer fragment input
		TEST_LAYER_PROVOKING_VERTEX,			// !< draw primitive with vertices in different layers, check which layer it was drawn to

		TEST_LAST
	};
										LayeredRenderCase			(Context& context, const char* name, const char* desc, LayeredRenderTargetType target, TestType test);
										~LayeredRenderCase			(void);

	void								init						(void);
	void								deinit						(void);
	IterateResult						iterate						(void);

private:
	void								initTexture					(void);
	void								initFbo						(void);
	void								initRenderShader			(void);
	void								initSamplerShader			(void);

	std::string							genFragmentSource			(const glu::ContextType& contextType) const;
	std::string							genGeometrySource			(const glu::ContextType& contextType) const;
	std::string							genSamplerFragmentSource	(const glu::ContextType& contextType) const;

	void								renderToTexture				(void);
	void								sampleTextureLayer			(tcu::Surface& dst, int layer);
	bool								verifyLayerContent			(const tcu::Surface& layer, int layerNdx);
	bool								verifyImageSingleColoredRow (const tcu::Surface& layer, float rowWidthRatio, const tcu::Vec4& color, bool logging = true);
	bool								verifyEmptyImage			(const tcu::Surface& layer, bool logging = true);
	bool								verifyProvokingVertexLayers	(const tcu::Surface& layer0, const tcu::Surface& layer1);

	static int							getTargetLayers				(LayeredRenderTargetType target);
	static glw::GLenum					getTargetTextureTarget		(LayeredRenderTargetType target);
	static tcu::IVec3					getTargetDimensions			(LayeredRenderTargetType target);
	static tcu::IVec2					getResolveDimensions		(LayeredRenderTargetType target);

	const LayeredRenderTargetType		m_target;
	const TestType						m_test;
	const int							m_numLayers;
	const int							m_targetLayer;
	const tcu::IVec2					m_resolveDimensions;

	int									m_iteration;
	bool								m_allLayersOk;

	glw::GLuint							m_texture;
	glw::GLuint							m_fbo;
	glu::ShaderProgram*					m_renderShader;
	glu::ShaderProgram*					m_samplerShader;

	glw::GLint							m_samplerSamplerLoc;
	glw::GLint							m_samplerLayerLoc;

	glw::GLenum							m_provokingVertex;
};

LayeredRenderCase::LayeredRenderCase (Context& context, const char* name, const char* desc, LayeredRenderTargetType target, TestType test)
	: TestCase				(context, name, desc)
	, m_target				(target)
	, m_test				(test)
	, m_numLayers			(getTargetLayers(target))
	, m_targetLayer			(m_numLayers / 2)
	, m_resolveDimensions	(getResolveDimensions(target))
	, m_iteration			(0)
	, m_allLayersOk			(true)
	, m_texture				(0)
	, m_fbo					(0)
	, m_renderShader		(DE_NULL)
	, m_samplerShader		(DE_NULL)
	, m_samplerSamplerLoc	(-1)
	, m_samplerLayerLoc		(-1)
	, m_provokingVertex		(0)
{
}

LayeredRenderCase::~LayeredRenderCase (void)
{
	deinit();
}

void LayeredRenderCase::init (void)
{
	// Requirements

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	if (m_target == TARGET_2D_MS_ARRAY && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
		TCU_THROW(NotSupportedError, "Test requires OES_texture_storage_multisample_2d_array extension or higher context version.");

	if (m_context.getRenderTarget().getWidth() < m_resolveDimensions.x() || m_context.getRenderTarget().getHeight() < m_resolveDimensions.y())
		throw tcu::NotSupportedError("Render target size must be at least " + de::toString(m_resolveDimensions.x()) + "x" + de::toString(m_resolveDimensions.y()));

	// log what the test tries to do

	if (m_test == TEST_DEFAULT_LAYER)
		m_testCtx.getLog() << tcu::TestLog::Message << "Rendering to the default layer." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_SINGLE_LAYER)
		m_testCtx.getLog() << tcu::TestLog::Message << "Rendering to a single layer." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_ALL_LAYERS)
		m_testCtx.getLog() << tcu::TestLog::Message << "Rendering to all layers." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_DIFFERENT_LAYERS)
		m_testCtx.getLog() << tcu::TestLog::Message << "Outputting different number of vertices to each layer." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_INVOCATION_PER_LAYER)
		m_testCtx.getLog() << tcu::TestLog::Message << "Using a different invocation to output to each layer." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_MULTIPLE_LAYERS_PER_INVOCATION)
		m_testCtx.getLog() << tcu::TestLog::Message << "Outputting to each layer from multiple invocations." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_LAYER_ID)
		m_testCtx.getLog() << tcu::TestLog::Message << "Using gl_Layer in fragment shader." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_LAYER_PROVOKING_VERTEX)
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying LAYER_PROVOKING_VERTEX." << tcu::TestLog::EndMessage;
	else
		DE_ASSERT(false);

	// init resources

	initTexture();
	initFbo();
	initRenderShader();
	initSamplerShader();
}

void LayeredRenderCase::deinit (void)
{
	if (m_texture)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texture);
		m_texture = 0;
	}

	if (m_fbo)
	{
		m_context.getRenderContext().getFunctions().deleteFramebuffers(1, &m_fbo);
		m_fbo = 0;
	}

	delete m_renderShader;
	delete m_samplerShader;

	m_renderShader = DE_NULL;
	m_samplerShader = DE_NULL;
}

LayeredRenderCase::IterateResult LayeredRenderCase::iterate (void)
{
	++m_iteration;

	if (m_iteration == 1)
	{
		if (m_test == TEST_LAYER_PROVOKING_VERTEX)
		{
			// which layer the implementation claims to render to

			gls::StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLint> state;

			m_context.getRenderContext().getFunctions().getIntegerv(GL_LAYER_PROVOKING_VERTEX, &state);
			GLU_EXPECT_NO_ERROR(m_context.getRenderContext().getFunctions().getError(), "getInteger(GL_LAYER_PROVOKING_VERTEX)");

			if (!state.verifyValidity(m_testCtx))
				return STOP;

			m_testCtx.getLog() << tcu::TestLog::Message << "GL_LAYER_PROVOKING_VERTEX = " << glu::getProvokingVertexStr(state) << tcu::TestLog::EndMessage;

			if (state != GL_FIRST_VERTEX_CONVENTION &&
				state != GL_LAST_VERTEX_CONVENTION &&
				state != GL_UNDEFINED_VERTEX)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "getInteger(GL_LAYER_PROVOKING_VERTEX) returned illegal value. Got " << state << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected provoking vertex value");
				return STOP;
			}

			m_provokingVertex = (glw::GLenum)state;
		}

		// render to texture
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "RenderToTexture", "Render to layered texture");

			// render to layered texture with the geometry shader
			renderToTexture();
		}

		return CONTINUE;
	}
	else if (m_test == TEST_LAYER_PROVOKING_VERTEX && m_provokingVertex == GL_UNDEFINED_VERTEX)
	{
		// Verification requires information from another layers, layers not independent
		{
			const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "VerifyLayers", "Verify layers 0 and 1");
			tcu::Surface				layer0		(m_resolveDimensions.x(), m_resolveDimensions.y());
			tcu::Surface				layer1		(m_resolveDimensions.x(), m_resolveDimensions.y());

			// sample layer to frame buffer
			sampleTextureLayer(layer0, 0);
			sampleTextureLayer(layer1, 1);

			m_allLayersOk &= verifyProvokingVertexLayers(layer0, layer1);
		}

		// Other layers empty
		for (int layerNdx = 2; layerNdx < m_numLayers; ++layerNdx)
		{
			const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "VerifyLayer", "Verify layer " + de::toString(layerNdx));
			tcu::Surface				layer		(m_resolveDimensions.x(), m_resolveDimensions.y());

			// sample layer to frame buffer
			sampleTextureLayer(layer, layerNdx);

			// verify
			m_allLayersOk &= verifyEmptyImage(layer);
		}
	}
	else
	{
		// Layers independent

		const int					layerNdx	= m_iteration - 2;
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "VerifyLayer", "Verify layer " + de::toString(layerNdx));
		tcu::Surface				layer		(m_resolveDimensions.x(), m_resolveDimensions.y());

		// sample layer to frame buffer
		sampleTextureLayer(layer, layerNdx);

		// verify
		m_allLayersOk &= verifyLayerContent(layer, layerNdx);

		if (layerNdx < m_numLayers-1)
			return CONTINUE;
	}

	// last iteration
	if (m_allLayersOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Detected invalid layer content");

	return STOP;
}

void LayeredRenderCase::initTexture (void)
{
	DE_ASSERT(!m_texture);

	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const tcu::IVec3			texSize			= getTargetDimensions(m_target);
	const tcu::TextureFormat	texFormat		= glu::mapGLInternalFormat(GL_RGBA8);
	const glu::TransferFormat	transferFormat	= glu::getTransferFormat(texFormat);

	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gen texture");

	switch (m_target)
	{
		case TARGET_CUBE:
			m_testCtx.getLog() << tcu::TestLog::Message << "Creating cubemap texture, size = " << texSize.x() << "x" << texSize.y() << tcu::TestLog::EndMessage;
			gl.bindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			break;

		case TARGET_3D:
			m_testCtx.getLog() << tcu::TestLog::Message << "Creating 3d texture, size = " << texSize.x() << "x" << texSize.y() << "x" << texSize.z() << tcu::TestLog::EndMessage;
			gl.bindTexture(GL_TEXTURE_3D, m_texture);
			gl.texImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, texSize.x(), texSize.y(), texSize.z(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			break;

		case TARGET_1D_ARRAY:
			m_testCtx.getLog() << tcu::TestLog::Message << "Creating 1d texture array, size = " << texSize.x() << ", layers = " << texSize.y() << tcu::TestLog::EndMessage;
			gl.bindTexture(GL_TEXTURE_1D_ARRAY, m_texture);
			gl.texImage2D(GL_TEXTURE_1D_ARRAY, 0, GL_RGBA8, texSize.x(), texSize.y(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			break;

		case TARGET_2D_ARRAY:
			m_testCtx.getLog() << tcu::TestLog::Message << "Creating 2d texture array, size = " << texSize.x() << "x" << texSize.y() << ", layers = " << texSize.z() << tcu::TestLog::EndMessage;
			gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
			gl.texImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, texSize.x(), texSize.y(), texSize.z(), 0, transferFormat.format, transferFormat.dataType, DE_NULL);
			break;

		case TARGET_2D_MS_ARRAY:
		{
			const int numSamples = 2;

			int maxSamples = 0;
			gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);

			m_testCtx.getLog() << tcu::TestLog::Message << "Creating 2d multisample texture array, size = " << texSize.x() << "x" << texSize.y() << ", layers = " << texSize.z() << ", samples = " << numSamples << tcu::TestLog::EndMessage;

			if (numSamples > maxSamples)
				throw tcu::NotSupportedError("Test requires " + de::toString(numSamples) + " color texture samples." );

			gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_texture);
			gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numSamples, GL_RGBA8, texSize.x(), texSize.y(), texSize.z(), GL_TRUE);
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "tex image");

	// Multisample textures don't use filters
	if (getTargetTextureTarget(m_target) != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		gl.texParameteri(getTargetTextureTarget(m_target), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.texParameteri(getTargetTextureTarget(m_target), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(getTargetTextureTarget(m_target), GL_TEXTURE_WRAP_S, GL_REPEAT);
		gl.texParameteri(getTargetTextureTarget(m_target), GL_TEXTURE_WRAP_T, GL_REPEAT);
		gl.texParameteri(getTargetTextureTarget(m_target), GL_TEXTURE_WRAP_R, GL_REPEAT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "tex filter");
	}
}

void LayeredRenderCase::initFbo (void)
{
	DE_ASSERT(!m_fbo);

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog() << tcu::TestLog::Message << "Creating FBO" << tcu::TestLog::EndMessage;

	gl.genFramebuffers(1, &m_fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "setup fbo");
}

void LayeredRenderCase::initRenderShader (void)
{
	const tcu::ScopedLogSection section(m_testCtx.getLog(), "RenderToTextureShader", "Create layered rendering shader program");

	static const char* const positionVertex =	"${GLSL_VERSION_DECL}\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
												"}\n";

	m_renderShader = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource(specializeShader(positionVertex, m_context.getRenderContext().getType()))
		<< glu::FragmentSource(genFragmentSource(m_context.getRenderContext().getType()))
		<< glu::GeometrySource(genGeometrySource(m_context.getRenderContext().getType())));
	m_testCtx.getLog() << *m_renderShader;

	if (!m_renderShader->isOk())
		throw tcu::TestError("failed to build render shader");
}

void LayeredRenderCase::initSamplerShader (void)
{
	const tcu::ScopedLogSection section(m_testCtx.getLog(), "TextureSamplerShader", "Create shader sampler program");

	static const char* const positionVertex =	"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"}\n";

	m_samplerShader = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
																			<< glu::VertexSource(specializeShader(positionVertex, m_context.getRenderContext().getType()))
																			<< glu::FragmentSource(genSamplerFragmentSource(m_context.getRenderContext().getType())));

	m_testCtx.getLog() << *m_samplerShader;

	if (!m_samplerShader->isOk())
		throw tcu::TestError("failed to build sampler shader");

	m_samplerSamplerLoc = m_context.getRenderContext().getFunctions().getUniformLocation(m_samplerShader->getProgram(), "u_sampler");
	if (m_samplerSamplerLoc == -1)
		throw tcu::TestError("u_sampler uniform location = -1");

	m_samplerLayerLoc = m_context.getRenderContext().getFunctions().getUniformLocation(m_samplerShader->getProgram(), "u_layer");
	if (m_samplerLayerLoc == -1)
		throw tcu::TestError("u_layer uniform location = -1");
}

std::string LayeredRenderCase::genFragmentSource (const glu::ContextType& contextType) const
{
	static const char* const fragmentLayerIdShader =	"${GLSL_VERSION_DECL}\n"
														"${GLSL_EXT_GEOMETRY_SHADER}"
														"layout(location = 0) out mediump vec4 fragColor;\n"
														"void main (void)\n"
														"{\n"
														"	fragColor = vec4(((gl_Layer % 2) == 1) ? 1.0 : 0.5,\n"
														"	                 (((gl_Layer / 2) % 2) == 1) ? 1.0 : 0.5,\n"
														"	                 (gl_Layer == 0) ? 1.0 : 0.0,\n"
														"	                 1.0);\n"
														"}\n";

	if (m_test != TEST_LAYER_ID)
		return specializeShader(s_commonShaderSourceFragment, contextType);
	else
		return specializeShader(fragmentLayerIdShader, contextType);
}

std::string LayeredRenderCase::genGeometrySource (const glu::ContextType& contextType) const
{
	// TEST_DIFFERENT_LAYERS:				draw 0 quad to first layer, 1 to second, etc.
	// TEST_ALL_LAYERS:						draw 1 quad to all layers
	// TEST_MULTIPLE_LAYERS_PER_INVOCATION:	draw 1 triangle to "current layer" and 1 triangle to another layer
	// else:								draw 1 quad to some single layer
	const int			maxVertices =		(m_test == TEST_DIFFERENT_LAYERS) ? ((2 + m_numLayers-1) * m_numLayers) :
											(m_test == TEST_ALL_LAYERS || m_test == TEST_LAYER_ID) ? (m_numLayers * 4) :
											(m_test == TEST_MULTIPLE_LAYERS_PER_INVOCATION) ? (6) :
											(m_test == TEST_LAYER_PROVOKING_VERTEX) ? (6) :
											(4);
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_GEOMETRY_SHADER}";

	if (m_test == TEST_INVOCATION_PER_LAYER || m_test == TEST_MULTIPLE_LAYERS_PER_INVOCATION)
		buf << "layout(points, invocations=" << m_numLayers << ") in;\n";
	else
		buf << "layout(points) in;\n";

	buf <<	"layout(triangle_strip, max_vertices = " << maxVertices << ") out;\n"
			"out highp vec4 v_frag_FragColor;\n"
			"\n"
			"void main (void)\n"
			"{\n";

	if (m_test == TEST_DEFAULT_LAYER)
	{
		buf <<	"	const highp vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n\n"
				"	gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n";
	}
	else if (m_test == TEST_SINGLE_LAYER)
	{
		buf <<	"	const highp vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n\n"
				"	gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = " << m_targetLayer << ";\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = " << m_targetLayer << ";\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = " << m_targetLayer << ";\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = " << m_targetLayer << ";\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n";
	}
	else if (m_test == TEST_ALL_LAYERS || m_test == TEST_LAYER_ID)
	{
		DE_ASSERT(m_numLayers <= 6);

		buf <<	"	const highp vec4 white   = vec4(1.0, 1.0, 1.0, 1.0);\n"
				"	const highp vec4 red     = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	const highp vec4 green   = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 blue    = vec4(0.0, 0.0, 1.0, 1.0);\n"
				"	const highp vec4 yellow  = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 magenta = vec4(1.0, 0.0, 1.0, 1.0);\n"
				"	const highp vec4 colors[6] = vec4[6](white, red, green, blue, yellow, magenta);\n\n"
				"	for (mediump int layerNdx = 0; layerNdx < " << m_numLayers << "; ++layerNdx)\n"
				"	{\n"
				"		gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"		gl_Layer = layerNdx;\n"
				"		v_frag_FragColor = colors[layerNdx];\n"
				"		EmitVertex();\n\n"
				"		gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"		gl_Layer = layerNdx;\n"
				"		v_frag_FragColor = colors[layerNdx];\n"
				"		EmitVertex();\n\n"
				"		gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				"		gl_Layer = layerNdx;\n"
				"		v_frag_FragColor = colors[layerNdx];\n"
				"		EmitVertex();\n\n"
				"		gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				"		gl_Layer = layerNdx;\n"
				"		v_frag_FragColor = colors[layerNdx];\n"
				"		EmitVertex();\n"
				"		EndPrimitive();\n"
				"	}\n";
	}
	else if (m_test == TEST_DIFFERENT_LAYERS)
	{
		DE_ASSERT(m_numLayers <= 6);

		buf <<	"	const highp vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n\n"
				"	for (mediump int layerNdx = 0; layerNdx < " << m_numLayers << "; ++layerNdx)\n"
				"	{\n"
				"		for (mediump int colNdx = 0; colNdx <= layerNdx; ++colNdx)\n"
				"		{\n"
				"			highp float posX = float(colNdx) / float(" << m_numLayers << ") * 2.0 - 1.0;\n\n"
				"			gl_Position = vec4(posX,  1.0, 0.0, 1.0);\n"
				"			gl_Layer = layerNdx;\n"
				"			v_frag_FragColor = white;\n"
				"			EmitVertex();\n\n"
				"			gl_Position = vec4(posX, -1.0, 0.0, 1.0);\n"
				"			gl_Layer = layerNdx;\n"
				"			v_frag_FragColor = white;\n"
				"			EmitVertex();\n"
				"		}\n"
				"		EndPrimitive();\n"
				"	}\n";
	}
	else if (m_test == TEST_INVOCATION_PER_LAYER)
	{
		buf <<	"	const highp vec4 white   = vec4(1.0, 1.0, 1.0, 1.0);\n"
				"	const highp vec4 red     = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	const highp vec4 green   = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 blue    = vec4(0.0, 0.0, 1.0, 1.0);\n"
				"	const highp vec4 yellow  = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"	const highp vec4 magenta = vec4(1.0, 0.0, 1.0, 1.0);\n"
				"	const highp vec4 colors[6] = vec4[6](white, red, green, blue, yellow, magenta);\n"
				"\n"
				"	gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = gl_InvocationID;\n"
				"	v_frag_FragColor = colors[gl_InvocationID];\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = gl_InvocationID;\n"
				"	v_frag_FragColor = colors[gl_InvocationID];\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = gl_InvocationID;\n"
				"	v_frag_FragColor = colors[gl_InvocationID];\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = gl_InvocationID;\n"
				"	v_frag_FragColor = colors[gl_InvocationID];\n"
				"	EmitVertex();\n"
				"	EndPrimitive();\n";
	}
	else if (m_test == TEST_MULTIPLE_LAYERS_PER_INVOCATION)
	{
		buf <<	"	const highp vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n"
				"\n"
				"	mediump int layerA = gl_InvocationID;\n"
				"	mediump int layerB = (gl_InvocationID + 1) % " << m_numLayers << ";\n"
				"	highp float aEnd = float(layerA) / float(" << m_numLayers << ") * 2.0 - 1.0;\n"
				"	highp float bEnd = float(layerB) / float(" << m_numLayers << ") * 2.0 - 1.0;\n"
				"\n"
				"	gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = layerA;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = layerA;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(aEnd, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = layerA;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	EndPrimitive();\n"
				"\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = layerB;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(bEnd,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = layerB;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(bEnd, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = layerB;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	EndPrimitive();\n";
	}
	else if (m_test == TEST_LAYER_PROVOKING_VERTEX)
	{
		buf <<	"	const highp vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n\n"
				"	gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = 0;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = 1;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = 1;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	EndPrimitive();\n\n"
				"	gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = 0;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				"	gl_Layer = 1;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n\n"
				"	gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				"	gl_Layer = 1;\n"
				"	v_frag_FragColor = white;\n"
				"	EmitVertex();\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	buf <<	"}\n";

	return specializeShader(buf.str(), contextType);
}

std::string LayeredRenderCase::genSamplerFragmentSource (const glu::ContextType& contextType) const
{
	std::ostringstream buf;

	buf << "${GLSL_VERSION_DECL}\n";
	if (m_target == TARGET_2D_MS_ARRAY)
		buf << "${GLSL_OES_TEXTURE_STORAGE_MULTISAMPLE}";
	buf << "layout(location = 0) out mediump vec4 fragColor;\n";

	switch (m_target)
	{
		case TARGET_CUBE:			buf << "uniform highp samplerCube u_sampler;\n";		break;
		case TARGET_3D:				buf << "uniform highp sampler3D u_sampler;\n";			break;
		case TARGET_2D_ARRAY:		buf << "uniform highp sampler2DArray u_sampler;\n";		break;
		case TARGET_1D_ARRAY:		buf << "uniform highp sampler1DArray u_sampler;\n";		break;
		case TARGET_2D_MS_ARRAY:	buf << "uniform highp sampler2DMSArray u_sampler;\n";	break;
		default:
			DE_ASSERT(DE_FALSE);
	}

	buf <<	"uniform highp int u_layer;\n"
			"void main (void)\n"
			"{\n";

	switch (m_target)
	{
		case TARGET_CUBE:
			buf <<	"	highp vec2 facepos = 2.0 * gl_FragCoord.xy / vec2(ivec2(" << m_resolveDimensions.x() << ", " << m_resolveDimensions.y() << ")) - vec2(1.0, 1.0);\n"
					"	if (u_layer == 0)\n"
					"		fragColor = textureLod(u_sampler, vec3(1.0, -facepos.y, -facepos.x), 0.0);\n"
					"	else if (u_layer == 1)\n"
					"		fragColor = textureLod(u_sampler, vec3(-1.0, -facepos.y, facepos.x), 0.0);\n"
					"	else if (u_layer == 2)\n"
					"		fragColor = textureLod(u_sampler, vec3(facepos.x, 1.0, facepos.y), 0.0);\n"
					"	else if (u_layer == 3)\n"
					"		fragColor = textureLod(u_sampler, vec3(facepos.x, -1.0, -facepos.y), 0.0);\n"
					"	else if (u_layer == 4)\n"
					"		fragColor = textureLod(u_sampler, vec3(facepos.x, -facepos.y, 1.0), 0.0);\n"
					"	else if (u_layer == 5)\n"
					"		fragColor = textureLod(u_sampler, vec3(-facepos.x, -facepos.y, -1.0), 0.0);\n"
					"	else\n"
					"		fragColor = vec4(1.0, 0.0, 1.0, 1.0);\n";
			break;

		case TARGET_3D:
		case TARGET_2D_ARRAY:
		case TARGET_2D_MS_ARRAY:
			buf <<	"	highp ivec2 screenpos = ivec2(floor(gl_FragCoord.xy));\n"
					"	fragColor = texelFetch(u_sampler, ivec3(screenpos, u_layer), 0);\n";
			break;

		case TARGET_1D_ARRAY:
			buf <<	"	highp ivec2 screenpos = ivec2(floor(gl_FragCoord.xy));\n"
					"	fragColor = texelFetch(u_sampler, ivec2(screenpos.x, u_layer), 0);\n";
			break;

		default:
			DE_ASSERT(DE_FALSE);
	}
	buf <<	"}\n";
	return specializeShader(buf.str(), contextType);
}

void LayeredRenderCase::renderToTexture (void)
{
	const tcu::IVec3		texSize		= getTargetDimensions(m_target);
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	glu::VertexArray		vao			(m_context.getRenderContext());

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering to texture" << tcu::TestLog::EndMessage;

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.viewport(0, 0, texSize.x(), texSize.y());
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.bindVertexArray(*vao);
	gl.useProgram(m_renderShader->getProgram());
	gl.drawArrays(GL_POINTS, 0, 1);
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "render");
}

void LayeredRenderCase::sampleTextureLayer (tcu::Surface& dst, int layer)
{
	DE_ASSERT(dst.getWidth() == m_resolveDimensions.x());
	DE_ASSERT(dst.getHeight() == m_resolveDimensions.y());

	static const tcu::Vec4 fullscreenQuad[4] =
	{
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
	};

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const int				positionLoc	= gl.getAttribLocation(m_samplerShader->getProgram(), "a_position");
	glu::VertexArray		vao			(m_context.getRenderContext());
	glu::Buffer				buf			(m_context.getRenderContext());

	m_testCtx.getLog() << tcu::TestLog::Message << "Sampling from texture layer " << layer << tcu::TestLog::EndMessage;

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.viewport(0, 0, m_resolveDimensions.x(), m_resolveDimensions.y());
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.bindBuffer(GL_ARRAY_BUFFER, *buf);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "buf");

	gl.bindVertexArray(*vao);
	gl.vertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(positionLoc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup attribs");

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(getTargetTextureTarget(m_target), m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind texture");

	gl.useProgram(m_samplerShader->getProgram());
	gl.uniform1i(m_samplerLayerLoc, layer);
	gl.uniform1i(m_samplerSamplerLoc, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup program");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

	gl.useProgram(0);
	gl.bindVertexArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clean");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

bool LayeredRenderCase::verifyLayerContent (const tcu::Surface& layer, int layerNdx)
{
	const tcu::Vec4 white   = tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	const tcu::Vec4 red     = tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const tcu::Vec4 green   = tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 blue    = tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f);
	const tcu::Vec4 yellow  = tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4 magenta = tcu::Vec4(1.0f, 0.0f, 1.0f, 1.0f);
	const tcu::Vec4 colors[6] = { white, red, green, blue, yellow, magenta };

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying layer contents" << tcu::TestLog::EndMessage;

	switch (m_test)
	{
		case TEST_DEFAULT_LAYER:
			if (layerNdx == 0)
				return verifyImageSingleColoredRow(layer, 0.5f, white);
			else
				return verifyEmptyImage(layer);

		case TEST_SINGLE_LAYER:
			if (layerNdx == m_targetLayer)
				return verifyImageSingleColoredRow(layer, 0.5f, white);
			else
				return verifyEmptyImage(layer);

		case TEST_ALL_LAYERS:
		case TEST_INVOCATION_PER_LAYER:
			return verifyImageSingleColoredRow(layer, 0.5f, colors[layerNdx]);

		case TEST_DIFFERENT_LAYERS:
		case TEST_MULTIPLE_LAYERS_PER_INVOCATION:
			if (layerNdx == 0)
				return verifyEmptyImage(layer);
			else
				return verifyImageSingleColoredRow(layer, (float)layerNdx / (float)m_numLayers, white);

		case TEST_LAYER_ID:
		{
			const tcu::Vec4 layerColor((layerNdx % 2 == 1) ? (1.0f) : (0.5f),
									   ((layerNdx/2) % 2 == 1) ? (1.0f) : (0.5f),
									   (layerNdx == 0) ? (1.0f) : (0.0f),
									   1.0f);
			return verifyImageSingleColoredRow(layer, 0.5f, layerColor);
		}

		case TEST_LAYER_PROVOKING_VERTEX:
			if (m_provokingVertex == GL_FIRST_VERTEX_CONVENTION)
			{
				if (layerNdx == 0)
					return verifyImageSingleColoredRow(layer, 0.5f, white);
				else
					return verifyEmptyImage(layer);
			}
			else if (m_provokingVertex == GL_LAST_VERTEX_CONVENTION)
			{
				if (layerNdx == 1)
					return verifyImageSingleColoredRow(layer, 0.5f, white);
				else
					return verifyEmptyImage(layer);
			}
			else
			{
				DE_ASSERT(false);
				return false;
			}

		default:
			DE_ASSERT(DE_FALSE);
			return false;
	};
}

bool LayeredRenderCase::verifyImageSingleColoredRow (const tcu::Surface& layer, float rowWidthRatio, const tcu::Vec4& barColor, bool logging)
{
	DE_ASSERT(rowWidthRatio > 0.0f);

	const int		barLength			= (int)(rowWidthRatio * (float)layer.getWidth());
	const int		barLengthThreshold	= 1;
	tcu::Surface	errorMask			(layer.getWidth(), layer.getHeight());
	bool			allPixelsOk			= true;

	if (logging)
		m_testCtx.getLog() << tcu::TestLog::Message << "Expecting all pixels with distance less or equal to (about) " << barLength << " pixels from left border to be of color " << barColor.swizzle(0,1,2) << "." << tcu::TestLog::EndMessage;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toIVec());

	for (int y = 0; y < layer.getHeight(); ++y)
	for (int x = 0; x < layer.getWidth(); ++x)
	{
		const tcu::RGBA color		= layer.getPixel(x, y);
		const tcu::RGBA refColor	= tcu::RGBA(barColor);
		const int		threshold	= 8;
		const bool		isBlack		= color.getRed() <= threshold || color.getGreen() <= threshold || color.getBlue() <= threshold;
		const bool		isColor		= tcu::allEqual(tcu::lessThan(tcu::abs(color.toIVec().swizzle(0, 1, 2) - refColor.toIVec().swizzle(0, 1, 2)), tcu::IVec3(threshold, threshold, threshold)), tcu::BVec3(true, true, true));

		bool			isOk;

		if (x <= barLength - barLengthThreshold)
			isOk = isColor;
		else if (x >= barLength + barLengthThreshold)
			isOk = isBlack;
		else
			isOk = isColor || isBlack;

		allPixelsOk &= isOk;

		if (!isOk)
			errorMask.setPixel(x, y, tcu::RGBA::red());
	}

	if (allPixelsOk)
	{
		if (logging)
			m_testCtx.getLog()	<< tcu::TestLog::Message << "Image is valid." << tcu::TestLog::EndMessage
								<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
								<< tcu::TestLog::Image("Layer", "Layer", layer)
								<< tcu::TestLog::EndImageSet;
		return true;
	}
	else
	{
		if (logging)
			m_testCtx.getLog()	<< tcu::TestLog::Message << "Image verification failed. Got unexpected pixels." << tcu::TestLog::EndMessage
								<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
								<< tcu::TestLog::Image("Layer",		"Layer",	layer)
								<< tcu::TestLog::Image("ErrorMask",	"Errors",	errorMask)
								<< tcu::TestLog::EndImageSet;
		return false;
	}

	if (logging)
		m_testCtx.getLog() << tcu::TestLog::Image("LayerContent", "Layer content", layer);

	return allPixelsOk;
}

bool LayeredRenderCase::verifyEmptyImage (const tcu::Surface& layer, bool logging)
{
	// Expect black
	if (logging)
		m_testCtx.getLog() << tcu::TestLog::Message << "Expecting empty image" << tcu::TestLog::EndMessage;

	for (int y = 0; y < layer.getHeight(); ++y)
	for (int x = 0; x < layer.getWidth(); ++x)
	{
		const tcu::RGBA color		= layer.getPixel(x, y);
		const int		threshold	= 8;
		const bool		isBlack		= color.getRed() <= threshold || color.getGreen() <= threshold || color.getBlue() <= threshold;

		if (!isBlack)
		{
			if (logging)
				m_testCtx.getLog()	<< tcu::TestLog::Message
									<< "Found (at least) one bad pixel at " << x << "," << y << ". Pixel color is not background color."
									<< tcu::TestLog::EndMessage
									<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
									<< tcu::TestLog::Image("Layer", "Layer", layer)
									<< tcu::TestLog::EndImageSet;
			return false;
		}
	}

	if (logging)
		m_testCtx.getLog() << tcu::TestLog::Message << "Image is valid" << tcu::TestLog::EndMessage;

	return true;
}

bool LayeredRenderCase::verifyProvokingVertexLayers (const tcu::Surface& layer0, const tcu::Surface& layer1)
{
	const bool		layer0Empty		= verifyEmptyImage(layer0, false);
	const bool		layer1Empty		= verifyEmptyImage(layer1, false);
	bool			error			= false;

	// Both images could contain something if the quad triangles get assigned to different layers
	m_testCtx.getLog() << tcu::TestLog::Message << "Expecting non-empty layers, or non-empty layer." << tcu::TestLog::EndMessage;

	if (layer0Empty == true && layer1Empty == true)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Got empty images." << tcu::TestLog::EndMessage;
		error = true;
	}

	// log images always
	m_testCtx.getLog()
		<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
		<< tcu::TestLog::Image("Layer", "Layer0", layer0)
		<< tcu::TestLog::Image("Layer", "Layer1", layer1)
		<< tcu::TestLog::EndImageSet;

	if (error)
		m_testCtx.getLog() << tcu::TestLog::Message << "Image verification failed." << tcu::TestLog::EndMessage;
	else
		m_testCtx.getLog() << tcu::TestLog::Message << "Image is valid." << tcu::TestLog::EndMessage;

	return !error;
}

int LayeredRenderCase::getTargetLayers (LayeredRenderTargetType target)
{
	switch (target)
	{
		case TARGET_CUBE:			return 6;
		case TARGET_3D:				return 4;
		case TARGET_1D_ARRAY:		return 4;
		case TARGET_2D_ARRAY:		return 4;
		case TARGET_2D_MS_ARRAY:	return 2;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

glw::GLenum LayeredRenderCase::getTargetTextureTarget (LayeredRenderTargetType target)
{
	switch (target)
	{
		case TARGET_CUBE:			return GL_TEXTURE_CUBE_MAP;
		case TARGET_3D:				return GL_TEXTURE_3D;
		case TARGET_1D_ARRAY:		return GL_TEXTURE_1D_ARRAY;
		case TARGET_2D_ARRAY:		return GL_TEXTURE_2D_ARRAY;
		case TARGET_2D_MS_ARRAY:	return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

tcu::IVec3 LayeredRenderCase::getTargetDimensions (LayeredRenderTargetType target)
{
	switch (target)
	{
		case TARGET_CUBE:			return tcu::IVec3(64, 64, 0);
		case TARGET_3D:				return tcu::IVec3(64, 64, 4);
		case TARGET_1D_ARRAY:		return tcu::IVec3(64, 4, 0);
		case TARGET_2D_ARRAY:		return tcu::IVec3(64, 64, 4);
		case TARGET_2D_MS_ARRAY:	return tcu::IVec3(64, 64, 2);
		default:
			DE_ASSERT(DE_FALSE);
			return tcu::IVec3(0, 0, 0);
	}
}

tcu::IVec2 LayeredRenderCase::getResolveDimensions (LayeredRenderTargetType target)
{
	switch (target)
	{
		case TARGET_CUBE:			return tcu::IVec2(64, 64);
		case TARGET_3D:				return tcu::IVec2(64, 64);
		case TARGET_1D_ARRAY:		return tcu::IVec2(64, 1);
		case TARGET_2D_ARRAY:		return tcu::IVec2(64, 64);
		case TARGET_2D_MS_ARRAY:	return tcu::IVec2(64, 64);
		default:
			DE_ASSERT(DE_FALSE);
			return tcu::IVec2(0, 0);
	}
}

class VaryingOutputCountCase : public GeometryShaderRenderTest
{
public:
	enum ShaderInstancingMode
	{
		MODE_WITHOUT_INSTANCING = 0,
		MODE_WITH_INSTANCING,

		MODE_LAST
	};
													VaryingOutputCountCase			(Context& context, const char* name, const char* desc, VaryingOutputCountShader::VaryingSource test, ShaderInstancingMode mode);
private:
	void											init							(void);
	void											deinit							(void);
	void											preRender						(sglr::Context& ctx, GLuint programID);

	sglr::ShaderProgram&							getProgram						(void);
	void											genVertexAttribData				(void);
	void											genVertexDataWithoutInstancing	(void);
	void											genVertexDataWithInstancing		(void);

	VaryingOutputCountShader*						m_program;
	const VaryingOutputCountShader::VaryingSource	m_test;
	const ShaderInstancingMode						m_mode;
	int												m_maxEmitCount;
};

VaryingOutputCountCase::VaryingOutputCountCase (Context& context, const char* name, const char* desc, VaryingOutputCountShader::VaryingSource test, ShaderInstancingMode mode)
	: GeometryShaderRenderTest	(context, name, desc, GL_POINTS, GL_TRIANGLE_STRIP, VaryingOutputCountShader::getAttributeName(test))
	, m_program					(DE_NULL)
	, m_test					(test)
	, m_mode					(mode)
	, m_maxEmitCount			(0)
{
	DE_ASSERT(mode < MODE_LAST);
}

void VaryingOutputCountCase::init (void)
{
	// Check requirements

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	if (m_test == VaryingOutputCountShader::READ_TEXTURE)
	{
		glw::GLint maxTextures = 0;

		m_context.getRenderContext().getFunctions().getIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &maxTextures);

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS = " << maxTextures << tcu::TestLog::EndMessage;

		if (maxTextures < 1)
			throw tcu::NotSupportedError("Geometry shader texture units required");
	}

	// Get max emit count
	{
		const int	componentsPerVertex	= 4 + 4; // vec4 pos, vec4 color
		glw::GLint	maxVertices			= 0;
		glw::GLint	maxComponents		= 0;

		m_context.getRenderContext().getFunctions().getIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxVertices);
		m_context.getRenderContext().getFunctions().getIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &maxComponents);

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_OUTPUT_VERTICES = " << maxVertices << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS = " << maxComponents << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Components per vertex = " << componentsPerVertex << tcu::TestLog::EndMessage;

		if (maxVertices < 256)
			throw tcu::TestError("MAX_GEOMETRY_OUTPUT_VERTICES was less than minimum required (256)");
		if (maxComponents < 1024)
			throw tcu::TestError("MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS was less than minimum required (1024)");

		m_maxEmitCount = de::min(maxVertices, maxComponents / componentsPerVertex);
	}

	// Log what the test tries to do

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering 4 n-gons with n = "
		<< ((VaryingOutputCountShader::EMIT_COUNT_VERTEX_0 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_0)) << ", "
		<< ((VaryingOutputCountShader::EMIT_COUNT_VERTEX_1 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_1)) << ", "
		<< ((VaryingOutputCountShader::EMIT_COUNT_VERTEX_2 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_2)) << ", and "
		<< ((VaryingOutputCountShader::EMIT_COUNT_VERTEX_3 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_3)) << ".\n"
		<< "N is supplied to the geomery shader with "
		<< ((m_test == VaryingOutputCountShader::READ_ATTRIBUTE) ? ("attribute") : (m_test == VaryingOutputCountShader::READ_UNIFORM) ? ("uniform") : ("texture"))
		<< tcu::TestLog::EndMessage;

	// Gen shader
	{
		const bool instanced = (m_mode == MODE_WITH_INSTANCING);

		DE_ASSERT(!m_program);
		m_program = new VaryingOutputCountShader(m_context.getRenderContext().getType(), m_test, m_maxEmitCount, instanced);
	}

	// Case init
	GeometryShaderRenderTest::init();
}

void VaryingOutputCountCase::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

void VaryingOutputCountCase::preRender (sglr::Context& ctx, GLuint programID)
{
	if (m_test == VaryingOutputCountShader::READ_UNIFORM)
	{
		const int		location		= ctx.getUniformLocation(programID, "u_emitCount");
		const deInt32	emitCount[4]	= { 6, 0, m_maxEmitCount, 10 };

		if (location == -1)
			throw tcu::TestError("uniform location of u_emitCount was -1.");

		ctx.uniform4iv(location, 1, emitCount);
	}
	else if (m_test == VaryingOutputCountShader::READ_TEXTURE)
	{
		const deUint8 data[4*4] =
		{
			255,   0,   0,   0,
			  0, 255,   0,   0,
			  0,   0, 255,   0,
			  0,   0,   0, 255,
		};
		const int	location	= ctx.getUniformLocation(programID, "u_sampler");
		GLuint		texID		= 0;

		if (location == -1)
			throw tcu::TestError("uniform location of u_sampler was -1.");
		ctx.uniform1i(location, 0);

		// \note we don't need to explicitly delete the texture, the sglr context will delete it
		ctx.genTextures(1, &texID);
		ctx.bindTexture(GL_TEXTURE_2D, texID);
		ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		ctx.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		ctx.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
}

sglr::ShaderProgram& VaryingOutputCountCase::getProgram (void)
{
	return *m_program;
}

void VaryingOutputCountCase::genVertexAttribData (void)
{
	if (m_mode == MODE_WITHOUT_INSTANCING)
		genVertexDataWithoutInstancing();
	else if (m_mode == MODE_WITH_INSTANCING)
		genVertexDataWithInstancing();
	else
		DE_ASSERT(false);
}

void VaryingOutputCountCase::genVertexDataWithoutInstancing (void)
{
	m_numDrawVertices = 4;

	m_vertexPosData.resize(4);
	m_vertexAttrData.resize(4);

	m_vertexPosData[0] = tcu::Vec4( 0.5f,  0.0f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4( 0.0f,  0.5f, 0.0f, 1.0f);
	m_vertexPosData[2] = tcu::Vec4(-0.7f, -0.1f, 0.0f, 1.0f);
	m_vertexPosData[3] = tcu::Vec4(-0.1f, -0.7f, 0.0f, 1.0f);

	if (m_test == VaryingOutputCountShader::READ_ATTRIBUTE)
	{
		m_vertexAttrData[0] = tcu::Vec4(((VaryingOutputCountShader::EMIT_COUNT_VERTEX_0 == -1) ? ((float)m_maxEmitCount) : ((float)VaryingOutputCountShader::EMIT_COUNT_VERTEX_0)), 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[1] = tcu::Vec4(((VaryingOutputCountShader::EMIT_COUNT_VERTEX_1 == -1) ? ((float)m_maxEmitCount) : ((float)VaryingOutputCountShader::EMIT_COUNT_VERTEX_1)), 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[2] = tcu::Vec4(((VaryingOutputCountShader::EMIT_COUNT_VERTEX_2 == -1) ? ((float)m_maxEmitCount) : ((float)VaryingOutputCountShader::EMIT_COUNT_VERTEX_2)), 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[3] = tcu::Vec4(((VaryingOutputCountShader::EMIT_COUNT_VERTEX_3 == -1) ? ((float)m_maxEmitCount) : ((float)VaryingOutputCountShader::EMIT_COUNT_VERTEX_3)), 0.0f, 0.0f, 0.0f);
	}
	else
	{
		m_vertexAttrData[0] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[1] = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[2] = tcu::Vec4(2.0f, 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[3] = tcu::Vec4(3.0f, 0.0f, 0.0f, 0.0f);
	}
}

void VaryingOutputCountCase::genVertexDataWithInstancing (void)
{
	m_numDrawVertices = 1;

	m_vertexPosData.resize(1);
	m_vertexAttrData.resize(1);

	m_vertexPosData[0] = tcu::Vec4(0.0f,  0.0f, 0.0f, 1.0f);

	if (m_test == VaryingOutputCountShader::READ_ATTRIBUTE)
	{
		const int emitCounts[] =
		{
			(VaryingOutputCountShader::EMIT_COUNT_VERTEX_0 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_0),
			(VaryingOutputCountShader::EMIT_COUNT_VERTEX_1 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_1),
			(VaryingOutputCountShader::EMIT_COUNT_VERTEX_2 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_2),
			(VaryingOutputCountShader::EMIT_COUNT_VERTEX_3 == -1) ? (m_maxEmitCount) : (VaryingOutputCountShader::EMIT_COUNT_VERTEX_3),
		};

		m_vertexAttrData[0] = tcu::Vec4((float)emitCounts[0], (float)emitCounts[1], (float)emitCounts[2], (float)emitCounts[3]);
	}
	else
	{
		// not used
		m_vertexAttrData[0] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

class GeometryProgramQueryCase : public TestCase
{
public:
	struct ProgramCase
	{
		const char*	description;
		const char*	header;
		int			value;
	};

						GeometryProgramQueryCase			(Context& context, const char* name, const char* description, glw::GLenum target);

	void				init								(void);
	IterateResult		iterate								(void);

private:
	void				expectProgramValue					(deUint32 program, int value);
	void				expectQueryError					(deUint32 program);

	const glw::GLenum	m_target;

protected:
	std::vector<ProgramCase> m_cases;
};

GeometryProgramQueryCase::GeometryProgramQueryCase (Context& context, const char* name, const char* description, glw::GLenum target)
	: TestCase	(context, name, description)
	, m_target	(target)
{
}

void GeometryProgramQueryCase::init (void)
{
	if (!(m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader") || glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2))))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");
}

GeometryProgramQueryCase::IterateResult GeometryProgramQueryCase::iterate (void)
{
	const bool			supportsES32			= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	const std::string	vertexSource			= std::string(glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType()))) + "\n"
												  "void main ()\n"
												  "{\n"
												  "	gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
												  "}\n";
	const std::string	fragmentSource			= std::string(glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType()))) + "\n"
												  "layout(location = 0) out mediump vec4 fragColor;\n"
												  "void main ()\n"
												  "{\n"
												  "	fragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
												  "}\n";
	static const char*	s_geometryBody			="void main ()\n"
												  "{\n"
												  "	gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
												  "	EmitVertex();\n"
												  "}\n";

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// default cases
	for (int ndx = 0; ndx < (int)m_cases.size(); ++ndx)
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "Case", m_cases[ndx].description);
		const std::string			geometrySource	= m_cases[ndx].header + std::string(s_geometryBody);
		const glu::ShaderProgram	program			(m_context.getRenderContext(),
														glu::ProgramSources()
														<< glu::VertexSource(vertexSource)
														<< glu::FragmentSource(fragmentSource)
														<< glu::GeometrySource(specializeShader(geometrySource, m_context.getRenderContext().getType())));

		m_testCtx.getLog() << program;
		expectProgramValue(program.getProgram(), m_cases[ndx].value);
	}

	// no geometry shader -case (INVALID OP)
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "NoGeometryShader", "No geometry shader");
		const glu::ShaderProgram	program			(m_context.getRenderContext(),
														glu::ProgramSources()
														<< glu::VertexSource(vertexSource)
														<< glu::FragmentSource(fragmentSource));

		m_testCtx.getLog() << program;
		expectQueryError(program.getProgram());
	}

	// not linked -case (INVALID OP)
	{
		const tcu::ScopedLogSection section				(m_testCtx.getLog(), "NotLinkedProgram", "Shader program not linked");
		const std::string	geometrySource				= std::string(glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType()))) + "\n"
														+ std::string(supportsES32 ? "" : "#extension GL_EXT_geometry_shader : require\n")
														+ "layout (triangles) in;\n"
														"layout (points, max_vertices = 3) out;\n"
														+ std::string(s_geometryBody);


		const char* const			vtxSourcePtr	= vertexSource.c_str();
		const char* const			fragSourcePtr	= fragmentSource.c_str();
		const char* const			geomSourcePtr	= geometrySource.c_str();

		glu::Shader					vertexShader	(m_context.getRenderContext(), glu::SHADERTYPE_VERTEX);
		glu::Shader					fragmentShader	(m_context.getRenderContext(), glu::SHADERTYPE_FRAGMENT);
		glu::Shader					geometryShader	(m_context.getRenderContext(), glu::SHADERTYPE_GEOMETRY);
		glu::Program				program			(m_context.getRenderContext());

		vertexShader.setSources(1, &vtxSourcePtr, DE_NULL);
		fragmentShader.setSources(1, &fragSourcePtr, DE_NULL);
		geometryShader.setSources(1, &geomSourcePtr, DE_NULL);

		vertexShader.compile();
		fragmentShader.compile();
		geometryShader.compile();

		if (!vertexShader.getCompileStatus()   ||
			!fragmentShader.getCompileStatus() ||
			!geometryShader.getCompileStatus())
			throw tcu::TestError("Failed to compile shader");

		program.attachShader(vertexShader.getShader());
		program.attachShader(fragmentShader.getShader());
		program.attachShader(geometryShader.getShader());

		m_testCtx.getLog() << tcu::TestLog::Message << "Creating a program with geometry shader, but not linking it" << tcu::TestLog::EndMessage;

		expectQueryError(program.getProgram());
	}

	return STOP;
}

void GeometryProgramQueryCase::expectProgramValue (deUint32 program, int value)
{
	const glw::Functions&										gl		= m_context.getRenderContext().getFunctions();
	gls::StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLint>	state;

	gl.getProgramiv(program, m_target, &state);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

	m_testCtx.getLog() << tcu::TestLog::Message << glu::getProgramParamStr(m_target) << " = " << state << tcu::TestLog::EndMessage;

	if (state != value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "// ERROR: Expected " << value << ", got " << state << tcu::TestLog::EndMessage;

		// don't overwrite error
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got invalid value");
	}
}

void GeometryProgramQueryCase::expectQueryError (deUint32 program)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	glw::GLint				dummy;
	glw::GLenum				errorCode;

	m_testCtx.getLog() << tcu::TestLog::Message << "Querying " << glu::getProgramParamStr(m_target) << ", expecting INVALID_OPERATION" << tcu::TestLog::EndMessage;
	gl.getProgramiv(program, m_target, &dummy);

	errorCode = gl.getError();

	if (errorCode != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "// ERROR: Expected INVALID_OPERATION, got " << glu::getErrorStr(errorCode) << tcu::TestLog::EndMessage;

		// don't overwrite error
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected error code");
	}
}

class GeometryShaderInvocationsQueryCase : public GeometryProgramQueryCase
{
public:
	GeometryShaderInvocationsQueryCase(Context& context, const char* name, const char* description);
};

GeometryShaderInvocationsQueryCase::GeometryShaderInvocationsQueryCase(Context& context, const char* name, const char* description)
	: GeometryProgramQueryCase(context, name, description, GL_GEOMETRY_SHADER_INVOCATIONS)
{
	// 2 normal cases
	m_cases.resize(2);

	m_cases[0].description	= "Default value";
	m_cases[0].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles) in;\nlayout (points, max_vertices = 3) out;\n";
	m_cases[0].value		= 1;

	m_cases[1].description	= "Value declared";
	m_cases[1].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles, invocations=2) in;\nlayout (points, max_vertices = 3) out;\n";
	m_cases[1].value		= 2;
}

class GeometryShaderVerticesQueryCase : public GeometryProgramQueryCase
{
public:
	GeometryShaderVerticesQueryCase(Context& context, const char* name, const char* description);
};

GeometryShaderVerticesQueryCase::GeometryShaderVerticesQueryCase (Context& context, const char* name, const char* description)
	: GeometryProgramQueryCase(context, name, description, GL_GEOMETRY_LINKED_VERTICES_OUT_EXT)
{
	m_cases.resize(1);

	m_cases[0].description	= "max_vertices = 1";
	m_cases[0].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles) in;\nlayout (points, max_vertices = 1) out;\n";
	m_cases[0].value		= 1;
}

class GeometryShaderInputQueryCase : public GeometryProgramQueryCase
{
public:
	GeometryShaderInputQueryCase(Context& context, const char* name, const char* description);
};

GeometryShaderInputQueryCase::GeometryShaderInputQueryCase(Context& context, const char* name, const char* description)
	: GeometryProgramQueryCase(context, name, description, GL_GEOMETRY_LINKED_INPUT_TYPE_EXT)
{
	m_cases.resize(3);

	m_cases[0].description	= "Triangles";
	m_cases[0].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles) in;\nlayout (points, max_vertices = 3) out;\n";
	m_cases[0].value		= GL_TRIANGLES;

	m_cases[1].description	= "Lines";
	m_cases[1].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (lines) in;\nlayout (points, max_vertices = 3) out;\n";
	m_cases[1].value		= GL_LINES;

	m_cases[2].description	= "Points";
	m_cases[2].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (points) in;\nlayout (points, max_vertices = 3) out;\n";
	m_cases[2].value		= GL_POINTS;
}

class GeometryShaderOutputQueryCase : public GeometryProgramQueryCase
{
public:
	GeometryShaderOutputQueryCase(Context& context, const char* name, const char* description);
};

GeometryShaderOutputQueryCase::GeometryShaderOutputQueryCase(Context& context, const char* name, const char* description)
	: GeometryProgramQueryCase(context, name, description, GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT)
{
	m_cases.resize(3);

	m_cases[0].description	= "Triangle strip";
	m_cases[0].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles) in;\nlayout (triangle_strip, max_vertices = 3) out;\n";
	m_cases[0].value		= GL_TRIANGLE_STRIP;

	m_cases[1].description	= "Lines";
	m_cases[1].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles) in;\nlayout (line_strip, max_vertices = 3) out;\n";
	m_cases[1].value		= GL_LINE_STRIP;

	m_cases[2].description	= "Points";
	m_cases[2].header		= "${GLSL_VERSION_DECL}\n${GLSL_EXT_GEOMETRY_SHADER}layout (triangles) in;\nlayout (points, max_vertices = 3) out;\n";
	m_cases[2].value		= GL_POINTS;
}

class ImplementationLimitCase : public TestCase
{
public:
						ImplementationLimitCase	(Context& context, const char* name, const char* description, glw::GLenum target, int minValue);

	void				init					(void);
	IterateResult		iterate					(void);

	const glw::GLenum	m_target;
	const int			m_minValue;
};

ImplementationLimitCase::ImplementationLimitCase (Context& context, const char* name, const char* description, glw::GLenum target, int minValue)
	: TestCase		(context, name, description)
	, m_target		(target)
	, m_minValue	(minValue)
{
}

void ImplementationLimitCase::init (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");
}

ImplementationLimitCase::IterateResult ImplementationLimitCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_INTEGER);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Types", "Alternative queries");
		verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_BOOLEAN);
		verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_INTEGER64);
		verifyStateIntegerMin(result, gl, m_target, m_minValue, QUERY_FLOAT);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class LayerProvokingVertexQueryCase : public TestCase
{
public:
					LayerProvokingVertexQueryCase	(Context& context, const char* name, const char* description);

	void			init							(void);
	IterateResult	iterate							(void);
};

LayerProvokingVertexQueryCase::LayerProvokingVertexQueryCase (Context& context, const char* name, const char* description)
	: TestCase(context, name, description)
{
}

void LayerProvokingVertexQueryCase::init (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");
}

LayerProvokingVertexQueryCase::IterateResult LayerProvokingVertexQueryCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	QueriedState			state;

	gl.enableLogging(true);
	queryState(result, gl, QUERY_INTEGER, GL_LAYER_PROVOKING_VERTEX, state);

	if (!state.isUndefined())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "LAYER_PROVOKING_VERTEX = " << glu::getProvokingVertexStr(state.getIntAccess()) << tcu::TestLog::EndMessage;

		if (state.getIntAccess() != GL_FIRST_VERTEX_CONVENTION &&
			state.getIntAccess() != GL_LAST_VERTEX_CONVENTION &&
			state.getIntAccess() != GL_UNDEFINED_VERTEX)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "getInteger(GL_LAYER_PROVOKING_VERTEX) returned illegal value. Got "
				<< state.getIntAccess() << "\n"
				<< "Expected any of {FIRST_VERTEX_CONVENTION, LAST_VERTEX_CONVENTION, UNDEFINED_VERTEX}."
				<< tcu::TestLog::EndMessage;

			result.fail("got unexpected provoking vertex value");
		}

		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Types", "Alternative queries");
			verifyStateInteger(result, gl, GL_LAYER_PROVOKING_VERTEX, state.getIntAccess(), QUERY_BOOLEAN);
			verifyStateInteger(result, gl, GL_LAYER_PROVOKING_VERTEX, state.getIntAccess(), QUERY_INTEGER64);
			verifyStateInteger(result, gl, GL_LAYER_PROVOKING_VERTEX, state.getIntAccess(), QUERY_FLOAT);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class GeometryInvocationCase : public GeometryShaderRenderTest
{
public:
	enum OutputCase
	{
		CASE_FIXED_OUTPUT_COUNTS = 0,
		CASE_DIFFERENT_OUTPUT_COUNTS,

		CASE_LAST
	};

								GeometryInvocationCase	(Context& context, const char* name, const char* description, int numInvocations, OutputCase testCase);
								~GeometryInvocationCase	(void);

	void						init					(void);
	void						deinit					(void);

private:
	sglr::ShaderProgram&		getProgram				(void);
	void						genVertexAttribData		(void);

	static InvocationCountShader::OutputCase mapToShaderCaseType (OutputCase testCase);

	const OutputCase			m_testCase;
	int							m_numInvocations;
	InvocationCountShader*		m_program;
};

GeometryInvocationCase::GeometryInvocationCase (Context& context, const char* name, const char* description, int numInvocations, OutputCase testCase)
	: GeometryShaderRenderTest	(context, name, description, GL_POINTS, GL_TRIANGLE_STRIP, "a_color")
	, m_testCase				(testCase)
	, m_numInvocations			(numInvocations)
	, m_program					(DE_NULL)
{
	DE_ASSERT(m_testCase < CASE_LAST);
}

GeometryInvocationCase::~GeometryInvocationCase	(void)
{
	deinit();
}

void GeometryInvocationCase::init (void)
{
	const glw::Functions&	gl								= m_context.getRenderContext().getFunctions();
	int						maxGeometryShaderInvocations	= 0;
	int						maxComponents					= 0;

	// requirements

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	gl.getIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &maxGeometryShaderInvocations);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS)");

	gl.getIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &maxComponents);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS)");

	m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_SHADER_INVOCATIONS = " << maxGeometryShaderInvocations << tcu::TestLog::EndMessage;

	// set target num invocations

	if (m_numInvocations == -1)
		m_numInvocations = maxGeometryShaderInvocations;
	else if (maxGeometryShaderInvocations < m_numInvocations)
		throw tcu::NotSupportedError("Test requires larger GL_MAX_GEOMETRY_SHADER_INVOCATIONS");

	if (m_testCase == CASE_DIFFERENT_OUTPUT_COUNTS)
	{
		const int maxEmitCount	= m_numInvocations + 2;
		const int numComponents	= 8; // pos + color
		if (maxEmitCount * numComponents > maxComponents)
			throw tcu::NotSupportedError("Test requires larger GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS");
	}

	// Log what the test tries to do

	if (m_testCase == CASE_FIXED_OUTPUT_COUNTS)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Rendering triangles in a partial circle formation with a geometry shader. Each triangle is generated by a separate invocation.\n"
			<< "Drawing 2 points, each generating " << m_numInvocations << " triangles."
			<< tcu::TestLog::EndMessage;
	}
	else if (m_testCase == CASE_DIFFERENT_OUTPUT_COUNTS)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Rendering n-gons in a partial circle formation with a geometry shader. Each n-gon is generated by a separate invocation.\n"
			<< "Drawing 2 points, each generating " << m_numInvocations << " n-gons."
			<< tcu::TestLog::EndMessage;
	}
	else
		DE_ASSERT(false);

	// resources

	m_program = new InvocationCountShader(m_context.getRenderContext().getType(), m_numInvocations, mapToShaderCaseType(m_testCase));

	GeometryShaderRenderTest::init();
}

void GeometryInvocationCase::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

sglr::ShaderProgram& GeometryInvocationCase::getProgram (void)
{
	return *m_program;
}

void GeometryInvocationCase::genVertexAttribData (void)
{
	m_vertexPosData.resize(2);
	m_vertexPosData[0] = tcu::Vec4(0.0f,-0.3f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4(0.2f, 0.3f, 0.0f, 1.0f);

	m_vertexAttrData.resize(2);
	m_vertexAttrData[0] = tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_vertexAttrData[1] = tcu::Vec4(0.8f, 0.8f, 0.8f, 1.0f);
	m_numDrawVertices = 2;
}

InvocationCountShader::OutputCase GeometryInvocationCase::mapToShaderCaseType (OutputCase testCase)
{
	switch (testCase)
	{
		case CASE_FIXED_OUTPUT_COUNTS:			return InvocationCountShader::CASE_FIXED_OUTPUT_COUNTS;
		case CASE_DIFFERENT_OUTPUT_COUNTS:		return InvocationCountShader::CASE_DIFFERENT_OUTPUT_COUNTS;
		default:
			DE_ASSERT(false);
			return InvocationCountShader::CASE_LAST;
	}
}

class DrawInstancedGeometryInstancedCase : public GeometryShaderRenderTest
{
public:
								DrawInstancedGeometryInstancedCase	(Context& context, const char* name, const char* description, int numInstances, int numInvocations);
								~DrawInstancedGeometryInstancedCase	(void);

private:
	void						init								(void);
	void						deinit								(void);
	sglr::ShaderProgram&		getProgram							(void);
	void						genVertexAttribData					(void);

	const int					m_numInstances;
	const int					m_numInvocations;
	InstancedExpansionShader*	m_program;
};

DrawInstancedGeometryInstancedCase::DrawInstancedGeometryInstancedCase (Context& context, const char* name, const char* description, int numInstances, int numInvocations)
	: GeometryShaderRenderTest	(context, name, description, GL_POINTS, GL_TRIANGLE_STRIP, "a_offset", FLAG_DRAW_INSTANCED)
	, m_numInstances			(numInstances)
	, m_numInvocations			(numInvocations)
	, m_program					(DE_NULL)
{
}

DrawInstancedGeometryInstancedCase::~DrawInstancedGeometryInstancedCase (void)
{
}

void DrawInstancedGeometryInstancedCase::init (void)
{
	m_program = new InstancedExpansionShader(m_context.getRenderContext().getType(), m_numInvocations);

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Rendering a single point with " << m_numInstances << " instances. "
		<< "Each geometry shader is invoked " << m_numInvocations << " times for each primitive. "
		<< tcu::TestLog::EndMessage;

	GeometryShaderRenderTest::init();
}

void DrawInstancedGeometryInstancedCase::deinit(void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	GeometryShaderRenderTest::deinit();
}

sglr::ShaderProgram& DrawInstancedGeometryInstancedCase::getProgram (void)
{
	return *m_program;
}

void DrawInstancedGeometryInstancedCase::genVertexAttribData (void)
{
	m_numDrawVertices = 1;
	m_numDrawInstances = m_numInstances;
	m_vertexAttrDivisor = 1;

	m_vertexPosData.resize(1);
	m_vertexAttrData.resize(8);

	m_vertexPosData[0] = tcu::Vec4( 0.0f,  0.0f, 0.0f, 1.0f);

	m_vertexAttrData[0] = tcu::Vec4( 0.5f,  0.0f, 0.0f, 0.0f);
	m_vertexAttrData[1] = tcu::Vec4( 0.0f,  0.5f, 0.0f, 0.0f);
	m_vertexAttrData[2] = tcu::Vec4(-0.7f, -0.1f, 0.0f, 0.0f);
	m_vertexAttrData[3] = tcu::Vec4(-0.1f, -0.7f, 0.0f, 0.0f);
	m_vertexAttrData[4] = tcu::Vec4(-0.8f, -0.7f, 0.0f, 0.0f);
	m_vertexAttrData[5] = tcu::Vec4(-0.9f,  0.6f, 0.0f, 0.0f);
	m_vertexAttrData[6] = tcu::Vec4(-0.8f,  0.3f, 0.0f, 0.0f);
	m_vertexAttrData[7] = tcu::Vec4(-0.1f,  0.1f, 0.0f, 0.0f);

	DE_ASSERT(m_numInstances <= (int)m_vertexAttrData.size());
}

class GeometryProgramLimitCase : public TestCase
{
public:
						GeometryProgramLimitCase	(Context& context, const char* name, const char* description, glw::GLenum apiName, const std::string& glslName, int limit);

private:
	void				init						(void);
	IterateResult		iterate						(void);

	const glw::GLenum	m_apiName;
	const std::string	m_glslName;
	const int			m_limit;
};

GeometryProgramLimitCase::GeometryProgramLimitCase (Context& context, const char* name, const char* description, glw::GLenum apiName, const std::string& glslName, int limit)
	: TestCase		(context, name, description)
	, m_apiName		(apiName)
	, m_glslName	(glslName)
	, m_limit		(limit)
{
}

void GeometryProgramLimitCase::init (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");
}

GeometryProgramLimitCase::IterateResult GeometryProgramLimitCase::iterate (void)
{
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	int						limit;

	// query limit
	{
		gls::StateQueryUtil::StateQueryMemoryWriteGuard<glw::GLint>	state;
		glu::CallLogWrapper											gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

		gl.enableLogging(true);
		gl.glGetIntegerv(m_apiName, &state);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "getIntegerv()");

		m_testCtx.getLog() << tcu::TestLog::Message << glu::getGettableStateStr(m_apiName) << " = " << state << tcu::TestLog::EndMessage;

		if (!state.verifyValidity(result))
		{
			result.setTestContextResult(m_testCtx);
			return STOP;
		}

		if (state < m_limit)
		{
			result.fail("Minimum value = " + de::toString(m_limit) + ", got " + de::toString(state.get()));
			result.setTestContextResult(m_testCtx);
			return STOP;
		}

		limit = state;

		// verify other getters
		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Types", "Alternative queries");
			verifyStateInteger(result, gl, m_apiName, limit, QUERY_BOOLEAN);
			verifyStateInteger(result, gl, m_apiName, limit, QUERY_INTEGER64);
			verifyStateInteger(result, gl, m_apiName, limit, QUERY_FLOAT);
		}
	}

	// verify limit is the same in GLSL
	{
		static const char* const vertexSource =		"${GLSL_VERSION_DECL}\n"
													"void main ()\n"
													"{\n"
													"	gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
													"}\n";
		static const char* const fragmentSource =	"${GLSL_VERSION_DECL}\n"
													"layout(location = 0) out mediump vec4 fragColor;\n"
													"void main ()\n"
													"{\n"
													"	fragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
													"}\n";
		const std::string geometrySource =			"${GLSL_VERSION_DECL}\n"
													"${GLSL_EXT_GEOMETRY_SHADER}"
													"layout(points) in;\n"
													"layout(points, max_vertices = 1) out;\n"
													"void main ()\n"
													"{\n"
													"	// Building the shader will fail if the constant value is not the expected\n"
													"	const mediump int cArraySize = (gl_" + m_glslName + " == " + de::toString(limit) + ") ? (1) : (-1);\n"
													"	float[cArraySize] fArray;\n"
													"	fArray[0] = 0.0f;\n"
													"	gl_Position = vec4(0.0, 0.0, 0.0, fArray[0]);\n"
													"	EmitVertex();\n"
													"}\n";

		const de::UniquePtr<glu::ShaderProgram> program(new glu::ShaderProgram(m_context.getRenderContext(),
																			   glu::ProgramSources()
																			   << glu::VertexSource(specializeShader(vertexSource, m_context.getRenderContext().getType()))
																			   << glu::FragmentSource(specializeShader(fragmentSource, m_context.getRenderContext().getType()))
																			   << glu::GeometrySource(specializeShader(geometrySource, m_context.getRenderContext().getType()))));

		m_testCtx.getLog() << tcu::TestLog::Message << "Building a test shader to verify GLSL constant " << m_glslName << " value." << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << *program;

		if (!program->isOk())
		{
			// compile failed, assume static assert failed
			result.fail("Shader build failed");
			result.setTestContextResult(m_testCtx);
			return STOP;
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Build ok" << tcu::TestLog::EndMessage;
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class PrimitivesGeneratedQueryCase : public TestCase
{
public:
	enum QueryTest
	{
		TEST_NO_GEOMETRY			= 0,
		TEST_NO_AMPLIFICATION,
		TEST_AMPLIFICATION,
		TEST_PARTIAL_PRIMITIVES,
		TEST_INSTANCED,

		TEST_LAST
	};

						PrimitivesGeneratedQueryCase	(Context& context, const char* name, const char* description, QueryTest test);
						~PrimitivesGeneratedQueryCase	(void);

private:
	void				init							(void);
	void				deinit							(void);
	IterateResult		iterate							(void);

	glu::ShaderProgram*	genProgram						(void);

	const QueryTest		m_test;
	glu::ShaderProgram*	m_program;
};

PrimitivesGeneratedQueryCase::PrimitivesGeneratedQueryCase (Context& context, const char* name, const char* description, QueryTest test)
	: TestCase	(context, name, description)
	, m_test	(test)
	, m_program	(DE_NULL)
{
	DE_ASSERT(m_test < TEST_LAST);
}

PrimitivesGeneratedQueryCase::~PrimitivesGeneratedQueryCase (void)
{
	deinit();
}

void PrimitivesGeneratedQueryCase::init (void)
{
	// requirements

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	// log what test tries to do

	if (m_test == TEST_NO_GEOMETRY)
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying PRIMITIVES_GENERATED while rendering without a geometry shader." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_NO_AMPLIFICATION)
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying PRIMITIVES_GENERATED while rendering with a non-amplifying geometry shader." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_AMPLIFICATION)
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying PRIMITIVES_GENERATED while rendering with a (3x) amplifying geometry shader." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_PARTIAL_PRIMITIVES)
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying PRIMITIVES_GENERATED while rendering with a geometry shader that emits also partial primitives." << tcu::TestLog::EndMessage;
	else if (m_test == TEST_INSTANCED)
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying PRIMITIVES_GENERATED while rendering with a instanced geometry shader." << tcu::TestLog::EndMessage;
	else
		DE_ASSERT(false);

	// resources

	m_program = genProgram();
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("could not build program");
}

void PrimitivesGeneratedQueryCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

PrimitivesGeneratedQueryCase::IterateResult PrimitivesGeneratedQueryCase::iterate (void)
{
	glw::GLuint primitivesGenerated = 0xDEBADBAD;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Drawing 8 points, setting a_one for each to value (1.0, 1.0, 1.0, 1.0)"
		<< tcu::TestLog::EndMessage;

	{
		static const tcu::Vec4 vertexData[8*2] =
		{
			tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.1f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.2f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.3f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.4f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.6f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(0.7f, 0.0f, 0.0f, 1.0f),	tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		};

		const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
		const glu::VertexArray	vao					(m_context.getRenderContext());
		const glu::Buffer		buffer				(m_context.getRenderContext());
		const glu::Query		query				(m_context.getRenderContext());
		const int				positionLocation	= gl.getAttribLocation(m_program->getProgram(), "a_position");
		const int				oneLocation			= gl.getAttribLocation(m_program->getProgram(), "a_one");

		gl.bindVertexArray(*vao);

		gl.bindBuffer(GL_ARRAY_BUFFER, *buffer);
		gl.bufferData(GL_ARRAY_BUFFER, (int)sizeof(vertexData), vertexData, GL_STATIC_DRAW);

		gl.vertexAttribPointer(positionLocation, 4, GL_FLOAT, GL_FALSE, 2 * (int)sizeof(tcu::Vec4), DE_NULL);
		gl.enableVertexAttribArray(positionLocation);

		if (oneLocation != -1)
		{
			gl.vertexAttribPointer(oneLocation, 4, GL_FLOAT, GL_FALSE, 2 * (int)sizeof(tcu::Vec4), (const tcu::Vec4*)DE_NULL + 1);
			gl.enableVertexAttribArray(oneLocation);
		}

		gl.useProgram(m_program->getProgram());

		GLU_EXPECT_NO_ERROR(gl.getError(), "setup render");

		gl.beginQuery(GL_PRIMITIVES_GENERATED, *query);
		gl.drawArrays(GL_POINTS, 0, 8);
		gl.endQuery(GL_PRIMITIVES_GENERATED);

		GLU_EXPECT_NO_ERROR(gl.getError(), "render and query");

		gl.getQueryObjectuiv(*query, GL_QUERY_RESULT, &primitivesGenerated);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get query result");
	}

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "GL_PRIMITIVES_GENERATED = " << primitivesGenerated
		<< tcu::TestLog::EndMessage;

	{
		const deUint32 expectedGenerated = (m_test == TEST_AMPLIFICATION) ? (3*8) : (m_test == TEST_INSTANCED) ? (8*(3+1)) : (8);

		if (expectedGenerated == primitivesGenerated)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got wrong result for GL_PRIMITIVES_GENERATED");
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Got unexpected result for GL_PRIMITIVES_GENERATED. Expected " << expectedGenerated << ", got " << primitivesGenerated
				<< tcu::TestLog::EndMessage;
		}
	}

	return STOP;
}

glu::ShaderProgram* PrimitivesGeneratedQueryCase::genProgram (void)
{
	static const char* const vertexSource =		"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"in highp vec4 a_one;\n"
												"out highp vec4 v_one;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"	v_one = a_one;\n"
												"}\n";
	static const char* const fragmentSource =	"${GLSL_VERSION_DECL}\n"
												"layout(location = 0) out mediump vec4 fragColor;\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
												"}\n";
	std::ostringstream geometrySource;
	glu::ProgramSources sources;

	if (m_test != TEST_NO_GEOMETRY)
	{
		geometrySource <<	"${GLSL_VERSION_DECL}\n"
							"${GLSL_EXT_GEOMETRY_SHADER}"
							"layout(points" << ((m_test == TEST_INSTANCED) ? (", invocations = 3") : ("")) << ") in;\n"
							"layout(triangle_strip, max_vertices = 7) out;\n"
							"in highp vec4 v_one[];\n"
							"void main (void)\n"
							"{\n"
							"	// always taken\n"
							"	if (v_one[0].x != 0.0)\n"
							"	{\n"
							"		gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.1, 0.0, 0.0);\n"
							"		EmitVertex();\n"
							"		gl_Position = gl_in[0].gl_Position + vec4(0.1, 0.0, 0.0, 0.0);\n"
							"		EmitVertex();\n"
							"		gl_Position = gl_in[0].gl_Position - vec4(0.1, 0.0, 0.0, 0.0);\n"
							"		EmitVertex();\n"
							"		EndPrimitive();\n"
							"	}\n";

		if (m_test == TEST_AMPLIFICATION)
		{
			geometrySource <<	"\n"
								"	// always taken\n"
								"	if (v_one[0].y != 0.0)\n"
								"	{\n"
								"		gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.1, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"		gl_Position = gl_in[0].gl_Position + vec4(0.1, 0.0, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"		gl_Position = gl_in[0].gl_Position - vec4(0.0, 0.1, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"		gl_Position = gl_in[0].gl_Position - vec4(0.1, 0.0, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"	}\n";
		}
		else if (m_test == TEST_PARTIAL_PRIMITIVES)
		{
			geometrySource <<	"\n"
								"	// always taken\n"
								"	if (v_one[0].y != 0.0)\n"
								"	{\n"
								"		gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.1, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"		gl_Position = gl_in[0].gl_Position + vec4(0.1, 0.0, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"\n"
								"		// never taken\n"
								"		if (v_one[0].z < 0.0)\n"
								"		{\n"
								"			gl_Position = gl_in[0].gl_Position - vec4(0.1, 0.0, 0.0, 0.0);\n"
								"			EmitVertex();\n"
								"		}\n"
								"	}\n";
		}
		else if (m_test == TEST_INSTANCED)
		{
			geometrySource <<	"\n"
								"	// taken once\n"
								"	if (v_one[0].y > float(gl_InvocationID) + 0.5)\n"
								"	{\n"
								"		gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.1, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"		gl_Position = gl_in[0].gl_Position + vec4(0.1, 0.0, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"		gl_Position = gl_in[0].gl_Position - vec4(0.1, 0.0, 0.0, 0.0);\n"
								"		EmitVertex();\n"
								"	}\n";
		}

		geometrySource <<	"}\n";
	}

	sources << glu::VertexSource(specializeShader(vertexSource, m_context.getRenderContext().getType()));
	sources << glu::FragmentSource(specializeShader(fragmentSource, m_context.getRenderContext().getType()));

	if (!geometrySource.str().empty())
		sources << glu::GeometrySource(specializeShader(geometrySource.str(), m_context.getRenderContext().getType()));

	return new glu::ShaderProgram(m_context.getRenderContext(), sources);
}

class PrimitivesGeneratedQueryObjectQueryCase : public TestCase
{
public:
					PrimitivesGeneratedQueryObjectQueryCase	(Context& context, const char* name, const char* description);

	void			init									(void);
	IterateResult	iterate									(void);
};

PrimitivesGeneratedQueryObjectQueryCase::PrimitivesGeneratedQueryObjectQueryCase (Context& context, const char* name, const char* description)
	: TestCase(context, name, description)
{
}

void PrimitivesGeneratedQueryObjectQueryCase::init (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");
}

PrimitivesGeneratedQueryObjectQueryCase::IterateResult PrimitivesGeneratedQueryObjectQueryCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		glw::GLuint query = 0;

		verifyStateQueryInteger(result, gl, GL_PRIMITIVES_GENERATED, GL_CURRENT_QUERY, 0, QUERY_QUERY);

		gl.glGenQueries(1, &query);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGenQueries");

		gl.glBeginQuery(GL_PRIMITIVES_GENERATED, query);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "beginQuery");

		verifyStateQueryInteger(result, gl, GL_PRIMITIVES_GENERATED, GL_CURRENT_QUERY, (int)query, QUERY_QUERY);

		gl.glEndQuery(GL_PRIMITIVES_GENERATED);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "endQuery");
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class GeometryShaderFeartureTestCase : public TestCase
{
public:
					GeometryShaderFeartureTestCase	(Context& context, const char* name, const char* description);

	void			init							(void);
};

GeometryShaderFeartureTestCase::GeometryShaderFeartureTestCase (Context& context, const char* name, const char* description)
	: TestCase(context, name, description)
{
}

void GeometryShaderFeartureTestCase::init (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");
}

class FramebufferDefaultLayersCase : public GeometryShaderFeartureTestCase
{
public:
					FramebufferDefaultLayersCase	(Context& context, const char* name, const char* description);
	IterateResult	iterate							(void);
};

FramebufferDefaultLayersCase::FramebufferDefaultLayersCase (Context& context, const char* name, const char* description)
	: GeometryShaderFeartureTestCase(context, name, description)
{
}

FramebufferDefaultLayersCase::IterateResult FramebufferDefaultLayersCase::iterate (void)
{
	glu::CallLogWrapper gl(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	gl.enableLogging(true);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "Default", "Default value");
		const glu::Framebuffer		fbo				(m_context.getRenderContext());
		glw::GLint					defaultLayers	= -1;

		gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fbo);
		gl.glGetFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, &defaultLayers);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "getFramebufferParameteriv");

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_FRAMEBUFFER_DEFAULT_LAYERS = " << defaultLayers << tcu::TestLog::EndMessage;

		if (defaultLayers != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected 0, got " << defaultLayers << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid layer count");
		}
	}

	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "SetTo12", "Set default layers to 12");
		const glu::Framebuffer		fbo				(m_context.getRenderContext());
		glw::GLint					defaultLayers	= -1;

		gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fbo);
		gl.glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 12);
		gl.glGetFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, &defaultLayers);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "getFramebufferParameteriv");

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_FRAMEBUFFER_DEFAULT_LAYERS = " << defaultLayers << tcu::TestLog::EndMessage;

		if (defaultLayers != 12)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected 12, got " << defaultLayers << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid layer count");
		}
	}

	return STOP;
}

class FramebufferAttachmentLayeredCase : public GeometryShaderFeartureTestCase
{
public:
					FramebufferAttachmentLayeredCase	(Context& context, const char* name, const char* description);
	IterateResult	iterate								(void);
};

FramebufferAttachmentLayeredCase::FramebufferAttachmentLayeredCase (Context& context, const char* name, const char* description)
	: GeometryShaderFeartureTestCase(context, name, description)
{
}

FramebufferAttachmentLayeredCase::IterateResult FramebufferAttachmentLayeredCase::iterate (void)
{
	enum CaseType
	{
		TEXTURE_3D,
		TEXTURE_2D_ARRAY,
		TEXTURE_CUBE,
		TEXTURE_2D_MS_ARRAY,
		TEXTURE_3D_LAYER,
		TEXTURE_2D_ARRAY_LAYER,
	};

	static const struct TextureType
	{
		const char*	name;
		const char*	description;
		bool		layered;
		CaseType	type;
	} textureTypes[] =
	{
		{ "3D",				"3D texture",			true,	TEXTURE_3D				},
		{ "2DArray",		"2D array",				true,	TEXTURE_2D_ARRAY		},
		{ "Cube",			"Cube map",				true,	TEXTURE_CUBE			},
		{ "2DMSArray",		"2D multisample array",	true,	TEXTURE_2D_MS_ARRAY		},
		{ "3DLayer",		"3D texture layer ",	false,	TEXTURE_3D_LAYER		},
		{ "2DArrayLayer",	"2D array layer ",		false,	TEXTURE_2D_ARRAY_LAYER	},
	};

	glu::CallLogWrapper gl(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	gl.enableLogging(true);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(textureTypes); ++ndx)
	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), textureTypes[ndx].name, textureTypes[ndx].description);
		const glu::Framebuffer		fbo				(m_context.getRenderContext());
		const glu::Texture			texture			(m_context.getRenderContext());
		glw::GLint					layered			= -1;

		gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fbo);

		if (textureTypes[ndx].type == TEXTURE_3D || textureTypes[ndx].type == TEXTURE_3D_LAYER)
		{
			gl.glBindTexture(GL_TEXTURE_3D, *texture);
			gl.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
			gl.glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			if (textureTypes[ndx].type == TEXTURE_3D)
				gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture, 0);
			else
				gl.glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture, 0, 2);
		}
		else if (textureTypes[ndx].type == TEXTURE_2D_ARRAY || textureTypes[ndx].type == TEXTURE_2D_ARRAY_LAYER)
		{
			gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *texture);
			gl.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
			gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			if (textureTypes[ndx].type == TEXTURE_2D_ARRAY)
				gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture, 0);
			else
				gl.glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture, 0, 3);
		}
		else if (textureTypes[ndx].type == TEXTURE_CUBE)
		{
			gl.glBindTexture(GL_TEXTURE_CUBE_MAP, *texture);
			for (int face = 0; face < 6; ++face)
				gl.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
			gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture, 0);
		}
		else if (textureTypes[ndx].type == TEXTURE_2D_MS_ARRAY)
		{
			// check extension
			if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Context is not equal or greather than 3.2 and GL_OES_texture_storage_multisample_2d_array not supported, skipping." << tcu::TestLog::EndMessage;
				continue;
			}

			gl.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, *texture);
			gl.glTexStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, GL_RGBA8, 32, 32, 32, GL_FALSE);
			gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture, 0);
		}

		GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup attachment");

		gl.glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_LAYERED, &layered);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "getFramebufferParameteriv");

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_FRAMEBUFFER_ATTACHMENT_LAYERED = " << glu::getBooleanStr(layered) << tcu::TestLog::EndMessage;

		if (layered != GL_TRUE && layered != GL_FALSE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected boolean, got " << layered << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid boolean");
		}
		else if ((layered == GL_TRUE) != textureTypes[ndx].layered)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected " << ((textureTypes[ndx].layered) ? ("GL_TRUE") : ("GL_FALSE")) << ", got " << glu::getBooleanStr(layered) << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid layer count");
		}
	}

	return STOP;
}

class FramebufferIncompleteLayereTargetsCase : public GeometryShaderFeartureTestCase
{
public:
					FramebufferIncompleteLayereTargetsCase	(Context& context, const char* name, const char* description);
	IterateResult	iterate									(void);
};

FramebufferIncompleteLayereTargetsCase::FramebufferIncompleteLayereTargetsCase (Context& context, const char* name, const char* description)
	: GeometryShaderFeartureTestCase(context, name, description)
{
}

FramebufferIncompleteLayereTargetsCase::IterateResult FramebufferIncompleteLayereTargetsCase::iterate (void)
{
	glu::CallLogWrapper gl(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	gl.enableLogging(true);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "LayerAndNonLayer", "Layered and non-layered");
		const glu::Framebuffer		fbo				(m_context.getRenderContext());
		const glu::Texture			texture0		(m_context.getRenderContext());
		const glu::Texture			texture1		(m_context.getRenderContext());

		glw::GLint					fboStatus;

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *texture0);
		gl.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
		gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *texture1);
		gl.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
		gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fbo);
		gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture0, 0);
		gl.glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, *texture1, 0, 0);

		GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup fbo");

		fboStatus = gl.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer status: " << glu::getFramebufferStatusStr(fboStatus) << tcu::TestLog::EndMessage;

		if (fboStatus != GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, got " << glu::getFramebufferStatusStr(fboStatus) << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid layer count");
		}
	}

	{
		const tcu::ScopedLogSection section			(m_testCtx.getLog(), "DifferentTarget", "Different target");
		const glu::Framebuffer		fbo				(m_context.getRenderContext());
		const glu::Texture			texture0		(m_context.getRenderContext());
		const glu::Texture			texture1		(m_context.getRenderContext());

		glw::GLint					fboStatus;

		gl.glBindTexture(GL_TEXTURE_2D_ARRAY, *texture0);
		gl.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
		gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		gl.glBindTexture(GL_TEXTURE_3D, *texture1);
		gl.glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
		gl.glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fbo);
		gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture0, 0);
		gl.glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, *texture1, 0);

		GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup fbo");

		fboStatus = gl.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer status: " << glu::getFramebufferStatusStr(fboStatus) << tcu::TestLog::EndMessage;

		if (fboStatus != GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, got " << glu::getFramebufferStatusStr(fboStatus) << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid layer count");
		}
	}

	return STOP;
}

class ReferencedByGeometryShaderCase : public GeometryShaderFeartureTestCase
{
public:
					ReferencedByGeometryShaderCase	(Context& context, const char* name, const char* description);
	IterateResult	iterate							(void);
};

ReferencedByGeometryShaderCase::ReferencedByGeometryShaderCase (Context& context, const char* name, const char* description)
	: GeometryShaderFeartureTestCase(context, name, description)
{
}

ReferencedByGeometryShaderCase::IterateResult ReferencedByGeometryShaderCase::iterate (void)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	{
		static const char* const vertexSource =		"${GLSL_VERSION_DECL}\n"
													"uniform highp vec4 u_position;\n"
													"void main (void)\n"
													"{\n"
													"	gl_Position = u_position;\n"
													"}\n";
		static const char* const fragmentSource =	"${GLSL_VERSION_DECL}\n"
													"layout(location = 0) out mediump vec4 fragColor;\n"
													"void main (void)\n"
													"{\n"
													"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
													"}\n";
		static const char* const geometrySource =	"${GLSL_VERSION_DECL}\n"
													"${GLSL_EXT_GEOMETRY_SHADER}"
													"layout(points) in;\n"
													"layout(points, max_vertices=1) out;\n"
													"uniform highp vec4 u_offset;\n"
													"void main (void)\n"
													"{\n"
													"	gl_Position = gl_in[0].gl_Position + u_offset;\n"
													"	EmitVertex();\n"
													"}\n";

		const glu::ShaderProgram program(m_context.getRenderContext(), glu::ProgramSources()
																		<< glu::VertexSource(specializeShader(vertexSource, m_context.getRenderContext().getType()))
																		<< glu::FragmentSource(specializeShader(fragmentSource, m_context.getRenderContext().getType()))
																		<< glu::GeometrySource(specializeShader(geometrySource, m_context.getRenderContext().getType())));
		m_testCtx.getLog() << program;

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "UnreferencedUniform", "Unreferenced uniform u_position");
			glu::CallLogWrapper			gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
			const deUint32				props[1]	= { GL_REFERENCED_BY_GEOMETRY_SHADER };
			deUint32					resourcePos;
			glw::GLsizei				length		= 0;
			glw::GLint					referenced	= 0;

			gl.enableLogging(true);

			resourcePos = gl.glGetProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_position");
			m_testCtx.getLog() << tcu::TestLog::Message << "u_position resource index: " << resourcePos << tcu::TestLog::EndMessage;

			gl.glGetProgramResourceiv(program.getProgram(), GL_UNIFORM, resourcePos, 1, props, 1, &length, &referenced);
			m_testCtx.getLog() << tcu::TestLog::Message << "Query GL_REFERENCED_BY_GEOMETRY_SHADER, got " << length << " value(s), value[0] = " << glu::getBooleanStr(referenced) << tcu::TestLog::EndMessage;

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "query resource");

			if (length == 0 || referenced != GL_FALSE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected GL_FALSE." << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected value");
			}
		}

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "ReferencedUniform", "Referenced uniform u_offset");
			glu::CallLogWrapper			gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
			const deUint32				props[1]	= { GL_REFERENCED_BY_GEOMETRY_SHADER };
			deUint32					resourcePos;
			glw::GLsizei				length		= 0;
			glw::GLint					referenced	= 0;

			gl.enableLogging(true);

			resourcePos = gl.glGetProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_offset");
			m_testCtx.getLog() << tcu::TestLog::Message << "u_offset resource index: " << resourcePos << tcu::TestLog::EndMessage;

			gl.glGetProgramResourceiv(program.getProgram(), GL_UNIFORM, resourcePos, 1, props, 1, &length, &referenced);
			m_testCtx.getLog() << tcu::TestLog::Message << "Query GL_REFERENCED_BY_GEOMETRY_SHADER, got " << length << " value(s), value[0] = " << glu::getBooleanStr(referenced) << tcu::TestLog::EndMessage;

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "query resource");

			if (length == 0 || referenced != GL_TRUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected GL_TRUE." << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected value");
			}
		}
	}

	return STOP;
}

class CombinedGeometryUniformLimitCase : public GeometryShaderFeartureTestCase
{
public:
						CombinedGeometryUniformLimitCase	(Context& context, const char* name, const char* desc);
private:
	IterateResult		iterate								(void);
};

CombinedGeometryUniformLimitCase::CombinedGeometryUniformLimitCase (Context& context, const char* name, const char* desc)
	: GeometryShaderFeartureTestCase(context, name, desc)
{
}

CombinedGeometryUniformLimitCase::IterateResult CombinedGeometryUniformLimitCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "The minimum value of MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS is MAX_GEOMETRY_UNIFORM_BLOCKS x MAX_UNIFORM_BLOCK_SIZE / 4 + MAX_GEOMETRY_UNIFORM_COMPONENTS"
						<< tcu::TestLog::EndMessage;

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformBlocks;
	gl.glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &maxUniformBlocks);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformBlockSize;
	gl.glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	StateQueryMemoryWriteGuard<glw::GLint> maxUniformComponents;
	gl.glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, &maxUniformComponents);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glGetIntegerv");

	if (maxUniformBlocks.verifyValidity(result) && maxUniformBlockSize.verifyValidity(result) && maxUniformComponents.verifyValidity(result))
	{
		const int limit = ((int)maxUniformBlocks) * ((int)maxUniformBlockSize) / 4 + (int)maxUniformComponents;
		verifyStateIntegerMin(result, gl, GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, limit, QUERY_INTEGER);

		{
			const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Types", "Alternative queries");
			verifyStateIntegerMin(result, gl, GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, limit, QUERY_BOOLEAN);
			verifyStateIntegerMin(result, gl, GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, limit, QUERY_INTEGER64);
			verifyStateIntegerMin(result, gl, GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, limit, QUERY_FLOAT);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class VertexFeedbackCase : public TestCase
{
public:
	enum DrawMethod
	{
		METHOD_DRAW_ARRAYS = 0,
		METHOD_DRAW_ARRAYS_INSTANCED,
		METHOD_DRAW_ARRAYS_INDIRECT,
		METHOD_DRAW_ELEMENTS,
		METHOD_DRAW_ELEMENTS_INSTANCED,
		METHOD_DRAW_ELEMENTS_INDIRECT,

		METHOD_LAST
	};
	enum PrimitiveType
	{
		PRIMITIVE_LINE_LOOP = 0,
		PRIMITIVE_LINE_STRIP,
		PRIMITIVE_TRIANGLE_STRIP,
		PRIMITIVE_TRIANGLE_FAN,
		PRIMITIVE_POINTS,

		PRIMITIVE_LAST
	};

						VertexFeedbackCase	(Context& context, const char* name, const char* description, DrawMethod method, PrimitiveType output);
						~VertexFeedbackCase	(void);
private:
	void				init				(void);
	void				deinit				(void);
	IterateResult		iterate				(void);

	glu::ShaderProgram*	genProgram			(void);
	deUint32			getOutputPrimitive	(void);
	deUint32			getBasePrimitive	(void);

	const DrawMethod	m_method;
	const PrimitiveType	m_output;

	deUint32			m_elementBuf;
	deUint32			m_arrayBuf;
	deUint32			m_offsetBuf;
	deUint32			m_feedbackBuf;
	deUint32			m_indirectBuffer;
	glu::ShaderProgram*	m_program;
	glu::VertexArray*	m_vao;
};

VertexFeedbackCase::VertexFeedbackCase (Context& context, const char* name, const char* description, DrawMethod method, PrimitiveType output)
	: TestCase			(context, name, description)
	, m_method			(method)
	, m_output			(output)
	, m_elementBuf		(0)
	, m_arrayBuf		(0)
	, m_offsetBuf		(0)
	, m_feedbackBuf		(0)
	, m_indirectBuffer	(0)
	, m_program			(DE_NULL)
	, m_vao				(DE_NULL)
{
	DE_ASSERT(method < METHOD_LAST);
	DE_ASSERT(output < PRIMITIVE_LAST);
}

VertexFeedbackCase::~VertexFeedbackCase (void)
{
	deinit();
}

void VertexFeedbackCase::init (void)
{
	// requirements

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	// log what test tries to do

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing GL_EXT_geometry_shader transform feedback relaxations.\n"
		<< "Capturing vertex shader varying, no geometry shader. Invoke with:"
		<< tcu::TestLog::EndMessage;

	switch (m_method)
	{
		case METHOD_DRAW_ARRAYS:				m_testCtx.getLog() << tcu::TestLog::Message << "Draw method: drawArrays"			<< tcu::TestLog::EndMessage;	break;
		case METHOD_DRAW_ARRAYS_INSTANCED:		m_testCtx.getLog() << tcu::TestLog::Message << "Draw method: drawArraysInstanced"	<< tcu::TestLog::EndMessage;	break;
		case METHOD_DRAW_ARRAYS_INDIRECT:		m_testCtx.getLog() << tcu::TestLog::Message << "Draw method: drawArraysIndirect"	<< tcu::TestLog::EndMessage;	break;
		case METHOD_DRAW_ELEMENTS:				m_testCtx.getLog() << tcu::TestLog::Message << "Draw method: drawElements"			<< tcu::TestLog::EndMessage;	break;
		case METHOD_DRAW_ELEMENTS_INSTANCED:	m_testCtx.getLog() << tcu::TestLog::Message << "Draw method: drawElementsInstanced" << tcu::TestLog::EndMessage;	break;
		case METHOD_DRAW_ELEMENTS_INDIRECT:		m_testCtx.getLog() << tcu::TestLog::Message << "Draw method: drawElementsIndirect"	<< tcu::TestLog::EndMessage;	break;
		default:
			DE_ASSERT(false);
	}
	switch (m_output)
	{
		case PRIMITIVE_LINE_LOOP:				m_testCtx.getLog() << tcu::TestLog::Message << "Draw primitive: line loop"			<< tcu::TestLog::EndMessage;	break;
		case PRIMITIVE_LINE_STRIP:				m_testCtx.getLog() << tcu::TestLog::Message << "Draw primitive: line strip"			<< tcu::TestLog::EndMessage;	break;
		case PRIMITIVE_TRIANGLE_STRIP:			m_testCtx.getLog() << tcu::TestLog::Message << "Draw primitive: triangle strip"		<< tcu::TestLog::EndMessage;	break;
		case PRIMITIVE_TRIANGLE_FAN:			m_testCtx.getLog() << tcu::TestLog::Message << "Draw primitive: triangle fan"		<< tcu::TestLog::EndMessage;	break;
		case PRIMITIVE_POINTS:					m_testCtx.getLog() << tcu::TestLog::Message << "Draw primitive: points"				<< tcu::TestLog::EndMessage;	break;
		default:
			DE_ASSERT(false);
	}

	// resources

	{
		static const deUint16 elementData[] =
		{
			0, 1, 2, 3,
		};
		static const tcu::Vec4 arrayData[] =
		{
			tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
			tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f),
			tcu::Vec4(2.0f, 0.0f, 0.0f, 0.0f),
			tcu::Vec4(3.0f, 0.0f, 0.0f, 0.0f),
		};
		static const tcu::Vec4 offsetData[] =
		{
			tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
			tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
			tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
			tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
		};

		const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
		const int				feedbackSize	= 8 * (int)sizeof(float[4]);

		m_vao = new glu::VertexArray(m_context.getRenderContext());
		gl.bindVertexArray(**m_vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set up vao");

		gl.genBuffers(1, &m_elementBuf);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuf);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elementData), &elementData[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");

		gl.genBuffers(1, &m_arrayBuf);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuf);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(arrayData), &arrayData[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");

		gl.genBuffers(1, &m_offsetBuf);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_offsetBuf);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(offsetData), &offsetData[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");

		gl.genBuffers(1, &m_feedbackBuf);
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_feedbackBuf);
		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackSize, DE_NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");

		m_program = genProgram();

		if (!m_program->isOk())
		{
			m_testCtx.getLog() << *m_program;
			throw tcu::TestError("could not build program");
		}
	}
}

void VertexFeedbackCase::deinit (void)
{
	if (m_elementBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_elementBuf);
		m_elementBuf = 0;
	}

	if (m_arrayBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_arrayBuf);
		m_arrayBuf = 0;
	}

	if (m_offsetBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_offsetBuf);
		m_offsetBuf = 0;
	}

	if (m_feedbackBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_feedbackBuf);
		m_feedbackBuf = 0;
	}

	if (m_indirectBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_indirectBuffer);
		m_indirectBuffer = 0;
	}

	delete m_program;
	m_program = DE_NULL;

	delete m_vao;
	m_vao = DE_NULL;
}

VertexFeedbackCase::IterateResult VertexFeedbackCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const deUint32			outputPrimitive	= getOutputPrimitive();
	const deUint32			basePrimitive	= getBasePrimitive();

	const int				posLocation		= gl.getAttribLocation(m_program->getProgram(), "a_position");
	const int				offsetLocation	= gl.getAttribLocation(m_program->getProgram(), "a_offset");

	if (posLocation == -1)
		throw tcu::TestError("a_position location was -1");
	if (offsetLocation == -1)
		throw tcu::TestError("a_offset location was -1");

	gl.useProgram(m_program->getProgram());

	gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuf);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);

	gl.bindBuffer(GL_ARRAY_BUFFER, m_offsetBuf);
	gl.vertexAttribPointer(offsetLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(offsetLocation);

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_feedbackBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind buffer base");

	m_testCtx.getLog() << tcu::TestLog::Message << "Calling BeginTransformFeedback(" << glu::getPrimitiveTypeStr(basePrimitive) << ")" << tcu::TestLog::EndMessage;
	gl.beginTransformFeedback(basePrimitive);
	GLU_EXPECT_NO_ERROR(gl.getError(), "beginTransformFeedback");

	switch (m_method)
	{
		case METHOD_DRAW_ARRAYS:
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Calling DrawArrays(" << glu::getPrimitiveTypeStr(outputPrimitive) << ", ...)" << tcu::TestLog::EndMessage;
			gl.drawArrays(outputPrimitive, 0, 4);
			break;
		}

		case METHOD_DRAW_ARRAYS_INSTANCED:
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Calling DrawArraysInstanced(" << glu::getPrimitiveTypeStr(outputPrimitive) << ", ...)" << tcu::TestLog::EndMessage;
			gl.vertexAttribDivisor(offsetLocation, 2);
			gl.drawArraysInstanced(outputPrimitive, 0, 3, 2);
			break;
		}

		case METHOD_DRAW_ELEMENTS:
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Calling DrawElements(" << glu::getPrimitiveTypeStr(outputPrimitive) << ", ...)" << tcu::TestLog::EndMessage;
			gl.drawElements(outputPrimitive, 4, GL_UNSIGNED_SHORT, DE_NULL);
			break;
		}

		case METHOD_DRAW_ELEMENTS_INSTANCED:
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Calling DrawElementsInstanced(" << glu::getPrimitiveTypeStr(outputPrimitive) << ", ...)" << tcu::TestLog::EndMessage;
			gl.drawElementsInstanced(outputPrimitive, 3, GL_UNSIGNED_SHORT, DE_NULL, 2);
			break;
		}

		case METHOD_DRAW_ARRAYS_INDIRECT:
		{
			struct DrawArraysIndirectCommand
			{
				deUint32 count;
				deUint32 instanceCount;
				deUint32 first;
				deUint32 reservedMustBeZero;
			} params;

			DE_STATIC_ASSERT(sizeof(DrawArraysIndirectCommand) == sizeof(deUint32[4]));

			params.count = 4;
			params.instanceCount = 1;
			params.first = 0;
			params.reservedMustBeZero = 0;

			gl.genBuffers(1, &m_indirectBuffer);
			gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
			gl.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(params), &params, GL_STATIC_DRAW);

			m_testCtx.getLog() << tcu::TestLog::Message << "Calling DrawElementsIndirect(" << glu::getPrimitiveTypeStr(outputPrimitive) << ", ...)" << tcu::TestLog::EndMessage;
			gl.drawArraysIndirect(outputPrimitive, DE_NULL);
			break;
		}

		case METHOD_DRAW_ELEMENTS_INDIRECT:
		{
			struct DrawElementsIndirectCommand
			{
				deUint32	count;
				deUint32	instanceCount;
				deUint32	firstIndex;
				deInt32		baseVertex;
				deUint32	reservedMustBeZero;
			} params;

			DE_STATIC_ASSERT(sizeof(DrawElementsIndirectCommand) == sizeof(deUint32[5]));

			params.count = 4;
			params.instanceCount = 1;
			params.firstIndex = 0;
			params.baseVertex = 0;
			params.reservedMustBeZero = 0;

			gl.genBuffers(1, &m_indirectBuffer);
			gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
			gl.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(params), &params, GL_STATIC_DRAW);

			m_testCtx.getLog() << tcu::TestLog::Message << "Calling DrawElementsIndirect(" << glu::getPrimitiveTypeStr(outputPrimitive) << ", ...)" << tcu::TestLog::EndMessage;
			gl.drawElementsIndirect(outputPrimitive, GL_UNSIGNED_SHORT, DE_NULL);
			break;
		}

		default:
			DE_ASSERT(false);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "endTransformFeedback");

	m_testCtx.getLog() << tcu::TestLog::Message << "No errors." << tcu::TestLog::EndMessage;
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

glu::ShaderProgram* VertexFeedbackCase::genProgram (void)
{
	static const char* const vertexSource =		"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"in highp vec4 a_offset;\n"
												"out highp vec4 tf_value;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"	tf_value = a_position + a_offset;\n"
												"}\n";
	static const char* const fragmentSource =	"${GLSL_VERSION_DECL}\n"
												"layout(location = 0) out mediump vec4 fragColor;\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0);\n"
												"}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
																<< glu::VertexSource(specializeShader(vertexSource, m_context.getRenderContext().getType()))
																<< glu::FragmentSource(specializeShader(fragmentSource, m_context.getRenderContext().getType()))
																<< glu::TransformFeedbackVarying("tf_value")
																<< glu::TransformFeedbackMode(GL_INTERLEAVED_ATTRIBS));
}

deUint32 VertexFeedbackCase::getOutputPrimitive (void)
{
	switch(m_output)
	{
		case PRIMITIVE_LINE_LOOP:		return GL_LINE_LOOP;
		case PRIMITIVE_LINE_STRIP:		return GL_LINE_STRIP;
		case PRIMITIVE_TRIANGLE_STRIP:	return GL_TRIANGLE_STRIP;
		case PRIMITIVE_TRIANGLE_FAN:	return GL_TRIANGLE_FAN;
		case PRIMITIVE_POINTS:			return GL_POINTS;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

deUint32 VertexFeedbackCase::getBasePrimitive (void)
{
	switch(m_output)
	{
		case PRIMITIVE_LINE_LOOP:		return GL_LINES;
		case PRIMITIVE_LINE_STRIP:		return GL_LINES;
		case PRIMITIVE_TRIANGLE_STRIP:	return GL_TRIANGLES;
		case PRIMITIVE_TRIANGLE_FAN:	return GL_TRIANGLES;
		case PRIMITIVE_POINTS:			return GL_POINTS;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

class VertexFeedbackOverflowCase : public TestCase
{
public:
	enum Method
	{
		METHOD_DRAW_ARRAYS = 0,
		METHOD_DRAW_ELEMENTS,
	};

						VertexFeedbackOverflowCase	(Context& context, const char* name, const char* description, Method method);
						~VertexFeedbackOverflowCase	(void);

private:
	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);
	glu::ShaderProgram*	genProgram					(void);

	const Method		m_method;

	deUint32			m_elementBuf;
	deUint32			m_arrayBuf;
	deUint32			m_feedbackBuf;
	glu::ShaderProgram*	m_program;
	glu::VertexArray*	m_vao;
};

VertexFeedbackOverflowCase::VertexFeedbackOverflowCase (Context& context, const char* name, const char* description, Method method)
	: TestCase		(context, name, description)
	, m_method		(method)
	, m_elementBuf	(0)
	, m_arrayBuf	(0)
	, m_feedbackBuf	(0)
	, m_program		(DE_NULL)
	, m_vao			(DE_NULL)
{
}

VertexFeedbackOverflowCase::~VertexFeedbackOverflowCase (void)
{
	deinit();
}

void VertexFeedbackOverflowCase::init (void)
{
	// requirements

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Tests require GL_EXT_geometry_shader extension or higher context version.");

	// log what test tries to do

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing GL_EXT_geometry_shader transform feedback overflow behavior.\n"
		<< "Capturing vertex shader varying, rendering 2 triangles. Allocating feedback buffer for 5 vertices."
		<< tcu::TestLog::EndMessage;

	// resources

	{
		static const deUint16	elementData[] =
		{
			0, 1, 2,
			0, 1, 2,
		};
		static const tcu::Vec4	arrayData[] =
		{
			tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
			tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		};

		const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();

		m_vao = new glu::VertexArray(m_context.getRenderContext());
		gl.bindVertexArray(**m_vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set up vao");

		if (m_method == METHOD_DRAW_ELEMENTS)
		{
			gl.genBuffers(1, &m_elementBuf);
			gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuf);
			gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elementData), &elementData[0], GL_STATIC_DRAW);
			GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");
		}

		gl.genBuffers(1, &m_arrayBuf);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuf);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(arrayData), &arrayData[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");

		{
			const int					feedbackCount			= 5 * 4; // 5x vec4
			const std::vector<float>	initialBufferContents	(feedbackCount, -1.0f);

			m_testCtx.getLog() << tcu::TestLog::Message << "Filling feeback buffer with dummy value (-1.0)." << tcu::TestLog::EndMessage;

			gl.genBuffers(1, &m_feedbackBuf);
			gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_feedbackBuf);
			gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, (int)(sizeof(float) * initialBufferContents.size()), &initialBufferContents[0], GL_DYNAMIC_COPY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");
		}

		m_program = genProgram();

		if (!m_program->isOk())
		{
			m_testCtx.getLog() << *m_program;
			throw tcu::TestError("could not build program");
		}
	}
}

void VertexFeedbackOverflowCase::deinit (void)
{
	if (m_elementBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_elementBuf);
		m_elementBuf = 0;
	}

	if (m_arrayBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_arrayBuf);
		m_arrayBuf = 0;
	}

	if (m_feedbackBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_feedbackBuf);
		m_feedbackBuf = 0;
	}

	delete m_program;
	m_program = DE_NULL;

	delete m_vao;
	m_vao = DE_NULL;
}

VertexFeedbackOverflowCase::IterateResult VertexFeedbackOverflowCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const int				posLocation		= gl.getAttribLocation(m_program->getProgram(), "a_position");

	if (posLocation == -1)
		throw tcu::TestError("a_position location was -1");

	gl.useProgram(m_program->getProgram());

	gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuf);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);

	if (m_method == METHOD_DRAW_ELEMENTS)
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuf);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind buffers");
	}

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_feedbackBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind buffer base");

	m_testCtx.getLog() << tcu::TestLog::Message << "Capturing 2 triangles." << tcu::TestLog::EndMessage;

	gl.beginTransformFeedback(GL_TRIANGLES);

	if (m_method == METHOD_DRAW_ELEMENTS)
		gl.drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, DE_NULL);
	else if (m_method == METHOD_DRAW_ARRAYS)
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	else
		DE_ASSERT(false);

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "capture");

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying final triangle was not partially written to the feedback buffer." << tcu::TestLog::EndMessage;

	{
		const void*				ptr		= gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(float[4]) * 5, GL_MAP_READ_BIT);
		std::vector<float>		feedback;
		bool					error	= false;

		GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange");
		if (!ptr)
			throw tcu::TestError("mapBufferRange returned null");

		feedback.resize(5*4);
		deMemcpy(&feedback[0], ptr, sizeof(float[4]) * 5);

		if (gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER) != GL_TRUE)
			throw tcu::TestError("unmapBuffer returned false");

		// Verify vertices 0 - 2
		for (int vertex = 0; vertex < 3; ++vertex)
		{
			for (int component = 0; component < 4; ++component)
			{
				if (feedback[vertex*4 + component] != 1.0f)
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "Feedback buffer vertex " << vertex << ", component " << component << ": unexpected value, expected 1.0, got " << feedback[vertex*4 + component]
						<< tcu::TestLog::EndMessage;
					error = true;
				}
			}
		}

		// Verify vertices 3 - 4
		for (int vertex = 3; vertex < 5; ++vertex)
		{
			for (int component = 0; component < 4; ++component)
			{
				if (feedback[vertex*4 + component] != -1.0f)
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "Feedback buffer vertex " << vertex << ", component " << component << ": unexpected value, expected -1.0, got " << feedback[vertex*4 + component]
						<< tcu::TestLog::EndMessage;
					error = true;
				}
			}
		}

		if (error)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Feedback result validation failed");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

glu::ShaderProgram* VertexFeedbackOverflowCase::genProgram (void)
{
	static const char* const vertexSource =		"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"}\n";
	static const char* const fragmentSource =	"${GLSL_VERSION_DECL}\n"
												"layout(location = 0) out mediump vec4 fragColor;\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0);\n"
												"}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
																<< glu::VertexSource(specializeShader(vertexSource, m_context.getRenderContext().getType()))
																<< glu::FragmentSource(specializeShader(fragmentSource, m_context.getRenderContext().getType()))
																<< glu::TransformFeedbackVarying("gl_Position")
																<< glu::TransformFeedbackMode(GL_INTERLEAVED_ATTRIBS));
}

} // anonymous

GeometryShaderTests::GeometryShaderTests (Context& context)
	: TestCaseGroup(context, "geometry_shading", "Geometry shader tests")
{
}

GeometryShaderTests::~GeometryShaderTests (void)
{
}

void GeometryShaderTests::init (void)
{
	struct PrimitiveTestSpec
	{
		deUint32	primitiveType;
		const char* name;
		deUint32	outputType;
	};

	struct EmitTestSpec
	{
		deUint32	outputType;
		int			emitCountA;		//!< primitive A emit count
		int			endCountA;		//!< primitive A end count
		int			emitCountB;		//!<
		int			endCountB;		//!<
		const char* name;
	};

	static const struct LayeredTarget
	{
		LayeredRenderCase::LayeredRenderTargetType	target;
		const char*									name;
		const char*									desc;
	} layerTargets[] =
	{
		{ LayeredRenderCase::TARGET_CUBE,			"cubemap",				"cubemap"						},
		{ LayeredRenderCase::TARGET_3D,				"3d",					"3D texture"					},
		{ LayeredRenderCase::TARGET_2D_ARRAY,		"2d_array",				"2D array texture"				},
		{ LayeredRenderCase::TARGET_2D_MS_ARRAY,	"2d_multisample_array",	"2D multisample array texture"	},
	};

	tcu::TestCaseGroup* const queryGroup				= new tcu::TestCaseGroup(m_testCtx, "query", "Query tests.");
	tcu::TestCaseGroup* const basicGroup				= new tcu::TestCaseGroup(m_testCtx, "basic", "Basic tests.");
	tcu::TestCaseGroup* const inputPrimitiveGroup		= new tcu::TestCaseGroup(m_testCtx, "input", "Different input primitives.");
	tcu::TestCaseGroup* const conversionPrimitiveGroup	= new tcu::TestCaseGroup(m_testCtx, "conversion", "Different input and output primitives.");
	tcu::TestCaseGroup* const emitGroup					= new tcu::TestCaseGroup(m_testCtx, "emit", "Different emit counts.");
	tcu::TestCaseGroup* const varyingGroup				= new tcu::TestCaseGroup(m_testCtx, "varying", "Test varyings.");
	tcu::TestCaseGroup* const layeredGroup				= new tcu::TestCaseGroup(m_testCtx, "layered", "Layered rendering.");
	tcu::TestCaseGroup* const instancedGroup			= new tcu::TestCaseGroup(m_testCtx, "instanced", "Instanced rendering.");
	tcu::TestCaseGroup* const negativeGroup				= new tcu::TestCaseGroup(m_testCtx, "negative", "Negative tests.");
	tcu::TestCaseGroup* const feedbackGroup				= new tcu::TestCaseGroup(m_testCtx, "vertex_transform_feedback", "Transform feedback.");

	this->addChild(queryGroup);
	this->addChild(basicGroup);
	this->addChild(inputPrimitiveGroup);
	this->addChild(conversionPrimitiveGroup);
	this->addChild(emitGroup);
	this->addChild(varyingGroup);
	this->addChild(layeredGroup);
	this->addChild(instancedGroup);
	this->addChild(negativeGroup);
	this->addChild(feedbackGroup);

	// query test
	{
		// limits with a corresponding glsl constant
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_input_components",				"", GL_MAX_GEOMETRY_INPUT_COMPONENTS,				"MaxGeometryInputComponents",		64));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_output_components",				"", GL_MAX_GEOMETRY_OUTPUT_COMPONENTS,				"MaxGeometryOutputComponents",		64));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_image_uniforms",					"", GL_MAX_GEOMETRY_IMAGE_UNIFORMS,					"MaxGeometryImageUniforms",			0));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_texture_image_units",			"", GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,			"MaxGeometryTextureImageUnits",		16));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_output_vertices",				"", GL_MAX_GEOMETRY_OUTPUT_VERTICES,				"MaxGeometryOutputVertices",		256));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_total_output_components",		"", GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,		"MaxGeometryTotalOutputComponents",	1024));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_uniform_components",				"", GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,				"MaxGeometryUniformComponents",		1024));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_atomic_counters",				"", GL_MAX_GEOMETRY_ATOMIC_COUNTERS,				"MaxGeometryAtomicCounters",		0));
		queryGroup->addChild(new GeometryProgramLimitCase(m_context, "max_geometry_atomic_counter_buffers",			"", GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS,			"MaxGeometryAtomicCounterBuffers",	0));

		// program queries
		queryGroup->addChild(new GeometryShaderVerticesQueryCase	(m_context, "geometry_linked_vertices_out",	"GL_GEOMETRY_LINKED_VERTICES_OUT"));
		queryGroup->addChild(new GeometryShaderInputQueryCase		(m_context, "geometry_linked_input_type",	"GL_GEOMETRY_LINKED_INPUT_TYPE"));
		queryGroup->addChild(new GeometryShaderOutputQueryCase		(m_context, "geometry_linked_output_type",	"GL_GEOMETRY_LINKED_OUTPUT_TYPE"));
		queryGroup->addChild(new GeometryShaderInvocationsQueryCase	(m_context, "geometry_shader_invocations",	"GL_GEOMETRY_SHADER_INVOCATIONS"));

		// limits
		queryGroup->addChild(new ImplementationLimitCase(m_context, "max_geometry_shader_invocations",		"", GL_MAX_GEOMETRY_SHADER_INVOCATIONS,		32));
		queryGroup->addChild(new ImplementationLimitCase(m_context, "max_geometry_uniform_blocks",			"", GL_MAX_GEOMETRY_UNIFORM_BLOCKS,			12));
		queryGroup->addChild(new ImplementationLimitCase(m_context, "max_geometry_shader_storage_blocks",	"", GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,	0));

		// layer_provoking_vertex_ext
		queryGroup->addChild(new LayerProvokingVertexQueryCase(m_context, "layer_provoking_vertex", "GL_LAYER_PROVOKING_VERTEX"));

		// primitives_generated
		queryGroup->addChild(new PrimitivesGeneratedQueryCase(m_context, "primitives_generated_no_geometry",		"PRIMITIVES_GENERATED query with no geometry shader",								PrimitivesGeneratedQueryCase::TEST_NO_GEOMETRY));
		queryGroup->addChild(new PrimitivesGeneratedQueryCase(m_context, "primitives_generated_no_amplification",	"PRIMITIVES_GENERATED query with non amplifying geometry shader",					PrimitivesGeneratedQueryCase::TEST_NO_AMPLIFICATION));
		queryGroup->addChild(new PrimitivesGeneratedQueryCase(m_context, "primitives_generated_amplification",		"PRIMITIVES_GENERATED query with amplifying geometry shader",						PrimitivesGeneratedQueryCase::TEST_AMPLIFICATION));
		queryGroup->addChild(new PrimitivesGeneratedQueryCase(m_context, "primitives_generated_partial_primitives", "PRIMITIVES_GENERATED query with geometry shader emitting partial primitives",		PrimitivesGeneratedQueryCase::TEST_PARTIAL_PRIMITIVES));
		queryGroup->addChild(new PrimitivesGeneratedQueryCase(m_context, "primitives_generated_instanced",			"PRIMITIVES_GENERATED query with instanced geometry shader",						PrimitivesGeneratedQueryCase::TEST_INSTANCED));

		queryGroup->addChild(new PrimitivesGeneratedQueryObjectQueryCase(m_context, "primitives_generated", "Query bound PRIMITIVES_GENERATED query"));

		// fbo
		queryGroup->addChild(new ImplementationLimitCase				(m_context, "max_framebuffer_layers",				"", GL_MAX_FRAMEBUFFER_LAYERS,	256));
		queryGroup->addChild(new FramebufferDefaultLayersCase			(m_context, "framebuffer_default_layers",			""));
		queryGroup->addChild(new FramebufferAttachmentLayeredCase		(m_context, "framebuffer_attachment_layered",		""));
		queryGroup->addChild(new FramebufferIncompleteLayereTargetsCase	(m_context, "framebuffer_incomplete_layer_targets",	""));

		// resource query
		queryGroup->addChild(new ReferencedByGeometryShaderCase			(m_context, "referenced_by_geometry_shader",	""));

		// combined limits
		queryGroup->addChild(new CombinedGeometryUniformLimitCase		(m_context, "max_combined_geometry_uniform_components", "MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS"));
	}

	// basic tests
	{
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_10",				"Output 10 vertices",								OutputCountPatternSpec(10)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_128",				"Output 128 vertices",								OutputCountPatternSpec(128)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_256",				"Output 256 vertices",								OutputCountPatternSpec(256)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_max",				"Output max vertices",								OutputCountPatternSpec(-1)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_10_and_100",		"Output 10 and 100 vertices in two invocations",	OutputCountPatternSpec(10, 100)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_100_and_10",		"Output 100 and 10 vertices in two invocations",	OutputCountPatternSpec(100, 10)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_0_and_128",			"Output 0 and 128 vertices in two invocations",		OutputCountPatternSpec(0, 128)));
		basicGroup->addChild(new OutputCountCase			(m_context,	"output_128_and_0",			"Output 128 and 0 vertices in two invocations",		OutputCountPatternSpec(128, 0)));

		basicGroup->addChild(new VaryingOutputCountCase		(m_context,	"output_vary_by_attribute",	"Output varying number of vertices",				VaryingOutputCountShader::READ_ATTRIBUTE,	VaryingOutputCountCase::MODE_WITHOUT_INSTANCING));
		basicGroup->addChild(new VaryingOutputCountCase		(m_context,	"output_vary_by_uniform",	"Output varying number of vertices",				VaryingOutputCountShader::READ_UNIFORM,		VaryingOutputCountCase::MODE_WITHOUT_INSTANCING));
		basicGroup->addChild(new VaryingOutputCountCase		(m_context,	"output_vary_by_texture",	"Output varying number of vertices",				VaryingOutputCountShader::READ_TEXTURE,		VaryingOutputCountCase::MODE_WITHOUT_INSTANCING));

		basicGroup->addChild(new BuiltinVariableRenderTest	(m_context,	"point_size",				"test gl_PointSize",								BuiltinVariableShader::TEST_POINT_SIZE));
		basicGroup->addChild(new BuiltinVariableRenderTest	(m_context,	"primitive_id_in",			"test gl_PrimitiveIDIn",							BuiltinVariableShader::TEST_PRIMITIVE_ID_IN));
		basicGroup->addChild(new BuiltinVariableRenderTest	(m_context,	"primitive_id_in_restarted","test gl_PrimitiveIDIn with primitive restart",		BuiltinVariableShader::TEST_PRIMITIVE_ID_IN, GeometryShaderRenderTest::FLAG_USE_RESTART_INDEX | GeometryShaderRenderTest::FLAG_USE_INDICES));
		basicGroup->addChild(new BuiltinVariableRenderTest	(m_context,	"primitive_id",				"test gl_PrimitiveID",								BuiltinVariableShader::TEST_PRIMITIVE_ID));
	}

	// input primitives
	{
		static const PrimitiveTestSpec inputPrimitives[] =
		{
			{ GL_POINTS,					"points",					GL_POINTS			},
			{ GL_LINES,						"lines",					GL_LINE_STRIP		},
			{ GL_LINE_LOOP,					"line_loop",				GL_LINE_STRIP		},
			{ GL_LINE_STRIP,				"line_strip",				GL_LINE_STRIP		},
			{ GL_TRIANGLES,					"triangles",				GL_TRIANGLE_STRIP	},
			{ GL_TRIANGLE_STRIP,			"triangle_strip",			GL_TRIANGLE_STRIP	},
			{ GL_TRIANGLE_FAN,				"triangle_fan",				GL_TRIANGLE_STRIP	},
			{ GL_LINES_ADJACENCY,			"lines_adjacency",			GL_LINE_STRIP		},
			{ GL_LINE_STRIP_ADJACENCY,		"line_strip_adjacency",		GL_LINE_STRIP		},
			{ GL_TRIANGLES_ADJACENCY,		"triangles_adjacency",		GL_TRIANGLE_STRIP	}
		};

		tcu::TestCaseGroup* const basicPrimitiveGroup		= new tcu::TestCaseGroup(m_testCtx, "basic_primitive",			"Different input and output primitives.");
		tcu::TestCaseGroup* const triStripAdjacencyGroup	= new tcu::TestCaseGroup(m_testCtx, "triangle_strip_adjacency",	"Different triangle_strip_adjacency vertex counts.");

		// more basic types
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(inputPrimitives); ++ndx)
			basicPrimitiveGroup->addChild(new GeometryExpanderRenderTest(m_context, inputPrimitives[ndx].name, inputPrimitives[ndx].name, inputPrimitives[ndx].primitiveType, inputPrimitives[ndx].outputType));

		// triangle strip adjacency with different vtx counts
		for (int vtxCount = 0; vtxCount <= 12; ++vtxCount)
		{
			const std::string name = "vertex_count_" + de::toString(vtxCount);
			const std::string desc = "Vertex count is " + de::toString(vtxCount);

			triStripAdjacencyGroup->addChild(new TriangleStripAdjacencyVertexCountTest(m_context, name.c_str(), desc.c_str(), vtxCount));
		}

		inputPrimitiveGroup->addChild(basicPrimitiveGroup);
		inputPrimitiveGroup->addChild(triStripAdjacencyGroup);
	}

	// different type conversions
	{
		static const PrimitiveTestSpec conversionPrimitives[] =
		{
			{ GL_TRIANGLES,		"triangles_to_points",	GL_POINTS			},
			{ GL_LINES,			"lines_to_points",		GL_POINTS			},
			{ GL_POINTS,		"points_to_lines",		GL_LINE_STRIP		},
			{ GL_TRIANGLES,		"triangles_to_lines",	GL_LINE_STRIP		},
			{ GL_POINTS,		"points_to_triangles",	GL_TRIANGLE_STRIP	},
			{ GL_LINES,			"lines_to_triangles",	GL_TRIANGLE_STRIP	}
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(conversionPrimitives); ++ndx)
			conversionPrimitiveGroup->addChild(new GeometryExpanderRenderTest(m_context, conversionPrimitives[ndx].name, conversionPrimitives[ndx].name, conversionPrimitives[ndx].primitiveType, conversionPrimitives[ndx].outputType));
	}

	// emit different amounts
	{
		static const EmitTestSpec emitTests[] =
		{
			{ GL_POINTS,			 0,		0,	0,	0,	"points"			},
			{ GL_POINTS,			 0,		1,	0,	0,	"points"			},
			{ GL_POINTS,			 1,		1,	0,	0,	"points"			},
			{ GL_POINTS,			 0,		2,	0,	0,	"points"			},
			{ GL_POINTS,			 1,		2,	0,	0,	"points"			},
			{ GL_LINE_STRIP,		 0,		0,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 0,		1,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 1,		1,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 2,		1,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 0,		2,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 1,		2,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 2,		2,	0,	0,	"line_strip"		},
			{ GL_LINE_STRIP,		 2,		2,	2,	0,	"line_strip"		},
			{ GL_TRIANGLE_STRIP,	 0,		0,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 0,		1,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 1,		1,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 2,		1,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 3,		1,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 0,		2,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 1,		2,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 2,		2,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 3,		2,	0,	0,	"triangle_strip"	},
			{ GL_TRIANGLE_STRIP,	 3,		2,	3,	0,	"triangle_strip"	},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(emitTests); ++ndx)
		{
			std::string name = std::string(emitTests[ndx].name) + "_emit_" + de::toString(emitTests[ndx].emitCountA) + "_end_" + de::toString(emitTests[ndx].endCountA);
			std::string desc = std::string(emitTests[ndx].name) + " output, emit " + de::toString(emitTests[ndx].emitCountA) + " vertices, call EndPrimitive " + de::toString(emitTests[ndx].endCountA) + " times";

			if (emitTests[ndx].emitCountB)
			{
				name += "_emit_" + de::toString(emitTests[ndx].emitCountB) + "_end_" + de::toString(emitTests[ndx].endCountB);
				desc += ", emit " + de::toString(emitTests[ndx].emitCountB) + " vertices, call EndPrimitive " + de::toString(emitTests[ndx].endCountB) + " times";
			}

			emitGroup->addChild(new EmitTest(m_context, name.c_str(), desc.c_str(), emitTests[ndx].emitCountA, emitTests[ndx].endCountA, emitTests[ndx].emitCountB, emitTests[ndx].endCountB, emitTests[ndx].outputType));
		}
	}

	// varying
	{
		struct VaryingTestSpec
		{
			int			vertexOutputs;
			int			geometryOutputs;
			const char*	name;
			const char*	desc;
		};

		static const VaryingTestSpec varyingTests[] =
		{
			{ -1, 1, "vertex_no_op_geometry_out_1", "vertex_no_op_geometry_out_1" },
			{  0, 1, "vertex_out_0_geometry_out_1", "vertex_out_0_geometry_out_1" },
			{  0, 2, "vertex_out_0_geometry_out_2", "vertex_out_0_geometry_out_2" },
			{  1, 0, "vertex_out_1_geometry_out_0", "vertex_out_1_geometry_out_0" },
			{  1, 2, "vertex_out_1_geometry_out_2", "vertex_out_1_geometry_out_2" },
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(varyingTests); ++ndx)
			varyingGroup->addChild(new VaryingTest(m_context, varyingTests[ndx].name, varyingTests[ndx].desc, varyingTests[ndx].vertexOutputs, varyingTests[ndx].geometryOutputs));
	}

	// layered
	{
		static const struct TestType
		{
			LayeredRenderCase::TestType	test;
			const char*					testPrefix;
			const char*					descPrefix;
		} tests[] =
		{
			{ LayeredRenderCase::TEST_DEFAULT_LAYER,			"render_with_default_layer_",	"Render to all layers of "					},
			{ LayeredRenderCase::TEST_SINGLE_LAYER,				"render_to_one_",				"Render to one layer of "					},
			{ LayeredRenderCase::TEST_ALL_LAYERS,				"render_to_all_",				"Render to all layers of "					},
			{ LayeredRenderCase::TEST_DIFFERENT_LAYERS,			"render_different_to_",			"Render different data to different layers"	},
			{ LayeredRenderCase::TEST_LAYER_ID,					"fragment_layer_",				"Read gl_Layer in fragment shader"			},
			{ LayeredRenderCase::TEST_LAYER_PROVOKING_VERTEX,	"layer_provoking_vertex_",		"Verify LAYER_PROVOKING_VERTEX"				},
		};

		for (int testNdx = 0; testNdx < DE_LENGTH_OF_ARRAY(tests); ++testNdx)
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(layerTargets); ++targetNdx)
		{
			const std::string name = std::string(tests[testNdx].testPrefix) + layerTargets[targetNdx].name;
			const std::string desc = std::string(tests[testNdx].descPrefix) + layerTargets[targetNdx].desc;

			layeredGroup->addChild(new LayeredRenderCase(m_context, name.c_str(), desc.c_str(), layerTargets[targetNdx].target, tests[testNdx].test));
		}
	}

	// instanced
	{
		static const struct InvocationCase
		{
			const char* name;
			int			numInvocations;
		} invocationCases[] =
		{
			{ "1",		1  },
			{ "2",		2  },
			{ "8",		8  },
			{ "32",		32 },
			{ "max",	-1 },
		};
		static const int numDrawInstances[] = { 2, 4, 8 };
		static const int numDrawInvocations[] = { 2, 8 };

		// same amount of content to all invocations
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(invocationCases); ++ndx)
			instancedGroup->addChild(new GeometryInvocationCase(m_context,
																(std::string("geometry_") + invocationCases[ndx].name + "_invocations").c_str(),
																(std::string("Geometry shader with ") + invocationCases[ndx].name + " invocation(s)").c_str(),
																invocationCases[ndx].numInvocations,
																GeometryInvocationCase::CASE_FIXED_OUTPUT_COUNTS));

		// different amount of content to each invocation
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(invocationCases); ++ndx)
			if (invocationCases[ndx].numInvocations != 1)
				instancedGroup->addChild(new GeometryInvocationCase(m_context,
																	(std::string("geometry_output_different_") + invocationCases[ndx].name + "_invocations").c_str(),
																	"Geometry shader invocation(s) with different emit counts",
																	invocationCases[ndx].numInvocations,
																	GeometryInvocationCase::CASE_DIFFERENT_OUTPUT_COUNTS));

		// invocation per layer
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(layerTargets); ++targetNdx)
		{
			const std::string name = std::string("invocation_per_layer_") + layerTargets[targetNdx].name;
			const std::string desc = std::string("Render to multiple layers with multiple invocations, one invocation per layer, target ") + layerTargets[targetNdx].desc;

			instancedGroup->addChild(new LayeredRenderCase(m_context, name.c_str(), desc.c_str(), layerTargets[targetNdx].target, LayeredRenderCase::TEST_INVOCATION_PER_LAYER));
		}

		// multiple layers per invocation
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(layerTargets); ++targetNdx)
		{
			const std::string name = std::string("multiple_layers_per_invocation_") + layerTargets[targetNdx].name;
			const std::string desc = std::string("Render to multiple layers with multiple invocations, multiple layers per invocation, target ") + layerTargets[targetNdx].desc;

			instancedGroup->addChild(new LayeredRenderCase(m_context, name.c_str(), desc.c_str(), layerTargets[targetNdx].target, LayeredRenderCase::TEST_MULTIPLE_LAYERS_PER_INVOCATION));
		}

		// different invocation output counts depending on {uniform, attrib, texture}
		instancedGroup->addChild(new VaryingOutputCountCase(m_context,	"invocation_output_vary_by_attribute",	"Output varying number of vertices", VaryingOutputCountShader::READ_ATTRIBUTE,	VaryingOutputCountCase::MODE_WITH_INSTANCING));
		instancedGroup->addChild(new VaryingOutputCountCase(m_context,	"invocation_output_vary_by_uniform",	"Output varying number of vertices", VaryingOutputCountShader::READ_UNIFORM,	VaryingOutputCountCase::MODE_WITH_INSTANCING));
		instancedGroup->addChild(new VaryingOutputCountCase(m_context,	"invocation_output_vary_by_texture",	"Output varying number of vertices", VaryingOutputCountShader::READ_TEXTURE,	VaryingOutputCountCase::MODE_WITH_INSTANCING));

		// with drawInstanced
		for (int instanceNdx = 0; instanceNdx < DE_LENGTH_OF_ARRAY(numDrawInstances); ++instanceNdx)
		for (int invocationNdx = 0; invocationNdx < DE_LENGTH_OF_ARRAY(numDrawInvocations); ++invocationNdx)
		{
			const std::string name = std::string("draw_") + de::toString(numDrawInstances[instanceNdx]) + "_instances_geometry_" + de::toString(numDrawInvocations[invocationNdx]) + "_invocations";
			const std::string desc = std::string("Draw ") + de::toString(numDrawInstances[instanceNdx]) + " instances, with " + de::toString(numDrawInvocations[invocationNdx]) + " geometry shader invocations.";

			instancedGroup->addChild(new DrawInstancedGeometryInstancedCase(m_context, name.c_str(), desc.c_str(), numDrawInstances[instanceNdx], numDrawInvocations[invocationNdx]));
		}
	}

	// negative (wrong types)
	{
		struct PrimitiveToInputTypeConversion
		{
			GLenum inputType;
			GLenum primitiveType;
		};

		static const PrimitiveToInputTypeConversion legalConversions[] =
		{
			{ GL_POINTS,				GL_POINTS					},
			{ GL_LINES,					GL_LINES					},
			{ GL_LINES,					GL_LINE_LOOP				},
			{ GL_LINES,					GL_LINE_STRIP				},
			{ GL_LINES_ADJACENCY,		GL_LINES_ADJACENCY			},
			{ GL_LINES_ADJACENCY,		GL_LINE_STRIP_ADJACENCY		},
			{ GL_TRIANGLES,				GL_TRIANGLES				},
			{ GL_TRIANGLES,				GL_TRIANGLE_STRIP			},
			{ GL_TRIANGLES,				GL_TRIANGLE_FAN				},
			{ GL_TRIANGLES_ADJACENCY,	GL_TRIANGLES_ADJACENCY		},
			{ GL_TRIANGLES_ADJACENCY,	GL_TRIANGLE_STRIP_ADJACENCY	},
		};

		static const GLenum inputTypes[] =
		{
			GL_POINTS,
			GL_LINES,
			GL_LINES_ADJACENCY,
			GL_TRIANGLES,
			GL_TRIANGLES_ADJACENCY
		};

		static const GLenum primitiveTypes[] =
		{
			GL_POINTS,
			GL_LINES,
			GL_LINE_LOOP,
			GL_LINE_STRIP,
			GL_LINES_ADJACENCY,
			GL_LINE_STRIP_ADJACENCY,
			GL_TRIANGLES,
			GL_TRIANGLE_STRIP,
			GL_TRIANGLE_FAN,
			GL_TRIANGLES_ADJACENCY,
			GL_TRIANGLE_STRIP_ADJACENCY
		};

		for (int inputTypeNdx = 0; inputTypeNdx < DE_LENGTH_OF_ARRAY(inputTypes); ++inputTypeNdx)
		for (int primitiveTypeNdx = 0; primitiveTypeNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); ++primitiveTypeNdx)
		{
			const GLenum		inputType		= inputTypes[inputTypeNdx];
			const GLenum		primitiveType	= primitiveTypes[primitiveTypeNdx];
			const std::string	name			= std::string("type_") + inputTypeToGLString(sglr::rr_util::mapGLGeometryShaderInputType(inputType)) + "_primitive_" + primitiveTypeToString(primitiveType);
			const std::string	desc			= std::string("Shader input type ") + inputTypeToGLString(sglr::rr_util::mapGLGeometryShaderInputType(inputType)) + ", draw primitive type " + primitiveTypeToString(primitiveType);

			bool isLegal = false;

			for (int legalNdx = 0; legalNdx < DE_LENGTH_OF_ARRAY(legalConversions); ++legalNdx)
				if (legalConversions[legalNdx].inputType == inputType && legalConversions[legalNdx].primitiveType == primitiveType)
					isLegal = true;

			// only illegal
			if (!isLegal)
				negativeGroup->addChild(new NegativeDrawCase(m_context, name.c_str(), desc.c_str(), inputType, primitiveType));
		}
	}

	// vertex transform feedback
	{
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_line_loop",				"Capture line loop lines",									VertexFeedbackCase::METHOD_DRAW_ARRAYS,				VertexFeedbackCase::PRIMITIVE_LINE_LOOP));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_line_strip",				"Capture line strip lines",									VertexFeedbackCase::METHOD_DRAW_ARRAYS,				VertexFeedbackCase::PRIMITIVE_LINE_STRIP));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_triangle_strip",			"Capture triangle strip triangles",							VertexFeedbackCase::METHOD_DRAW_ARRAYS,				VertexFeedbackCase::PRIMITIVE_TRIANGLE_STRIP));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_triangle_fan",			"Capture triangle fan triangles",							VertexFeedbackCase::METHOD_DRAW_ARRAYS,				VertexFeedbackCase::PRIMITIVE_TRIANGLE_FAN));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_draw_arrays",				"Capture primitives generated with drawArrays",				VertexFeedbackCase::METHOD_DRAW_ARRAYS,				VertexFeedbackCase::PRIMITIVE_POINTS));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_draw_arrays_instanced",	"Capture primitives generated with drawArraysInstanced",	VertexFeedbackCase::METHOD_DRAW_ARRAYS_INSTANCED,	VertexFeedbackCase::PRIMITIVE_POINTS));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_draw_arrays_indirect",	"Capture primitives generated with drawArraysIndirect",		VertexFeedbackCase::METHOD_DRAW_ARRAYS_INDIRECT,	VertexFeedbackCase::PRIMITIVE_POINTS));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_draw_elements",			"Capture primitives generated with drawElements",			VertexFeedbackCase::METHOD_DRAW_ELEMENTS,			VertexFeedbackCase::PRIMITIVE_POINTS));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_draw_elements_instanced",	"Capture primitives generated with drawElementsInstanced",	VertexFeedbackCase::METHOD_DRAW_ELEMENTS_INSTANCED,	VertexFeedbackCase::PRIMITIVE_POINTS));
		feedbackGroup->addChild(new VertexFeedbackCase(m_context, "capture_vertex_draw_elements_indirect",	"Capture primitives generated with drawElementsIndirect",	VertexFeedbackCase::METHOD_DRAW_ELEMENTS_INDIRECT,	VertexFeedbackCase::PRIMITIVE_POINTS));

		feedbackGroup->addChild(new VertexFeedbackOverflowCase(m_context, "capture_vertex_draw_arrays_overflow_single_buffer",		"Capture triangles to too small a buffer", VertexFeedbackOverflowCase::METHOD_DRAW_ARRAYS));
		feedbackGroup->addChild(new VertexFeedbackOverflowCase(m_context, "capture_vertex_draw_elements_overflow_single_buffer",	"Capture triangles to too small a buffer", VertexFeedbackOverflowCase::METHOD_DRAW_ELEMENTS));
	}
}

} // Functional
} // gles31
} // deqp
