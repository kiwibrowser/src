/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief ShaderLibrary Vulkan implementation
 *//*--------------------------------------------------------------------*/

#include "vktShaderLibrary.hpp"
#include "vktTestCase.hpp"

#include "vkPrograms.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"

#include "gluShaderLibrary.hpp"
#include "gluShaderUtil.hpp"

#include "tcuStringTemplate.hpp"
#include "tcuTexture.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"

#include "deStringUtil.hpp"
#include "deArrayUtil.hpp"
#include "deMemory.h"

#include <sstream>
#include <map>

namespace vkt
{

using std::string;
using std::vector;
using std::map;
using std::pair;
using std::ostringstream;

using de::MovePtr;
using de::UniquePtr;

using glu::ShaderType;
using glu::ProgramSources;
using glu::DataType;

using glu::sl::ShaderCaseSpecification;
using glu::sl::ProgramSpecializationParams;
using glu::sl::RequiredExtension;
using glu::sl::Value;
using glu::sl::ValueBlock;

using tcu::TestStatus;
using tcu::StringTemplate;
using tcu::Vec2;
using tcu::ConstPixelBufferAccess;
using tcu::TextureFormat;
using tcu::TestLog;

using vk::SourceCollections;
using vk::Move;
using vk::Unique;

namespace
{

enum
{
	REFERENCE_UNIFORM_BINDING	= 0,
	USER_UNIFORM_BINDING		= 1
};

string getShaderName (ShaderType shaderType, size_t progNdx)
{
	ostringstream str;
	str << glu::getShaderTypeName(shaderType);
	if (progNdx > 0)
		str << "_" << progNdx;
	return str.str();
}

void genUniformBlock (ostringstream& out, const string& blockName, const string& instanceName, int setNdx, int bindingNdx, const vector<Value>& uniforms)
{
	out << "layout(";

	if (setNdx != 0)
		out << "set = " << setNdx << ", ";

	out << "binding = " << bindingNdx << ", std140) uniform " << blockName << "\n"
		<< "{\n";

	for (vector<Value>::const_iterator val = uniforms.begin(); val != uniforms.end(); ++val)
		out << "\t" << glu::declare(val->type, val->name, 1) << ";\n";

	out << "}";

	if (!instanceName.empty())
		out << " " << instanceName;

	out << ";\n";
}

void declareReferenceBlock (ostringstream& out, const ValueBlock& valueBlock)
{
	if (!valueBlock.outputs.empty())
		genUniformBlock(out, "Reference", "ref", 0, REFERENCE_UNIFORM_BINDING, valueBlock.outputs);
}

void declareUniforms (ostringstream& out, const ValueBlock& valueBlock)
{
	if (!valueBlock.uniforms.empty())
		genUniformBlock(out, "Uniforms", "", 0, USER_UNIFORM_BINDING, valueBlock.uniforms);
}

DataType getTransportType (DataType valueType)
{
	if (isDataTypeBoolOrBVec(valueType))
		return glu::getDataTypeUintVec(getDataTypeScalarSize(valueType));
	else
		return valueType;
}

int getNumTransportLocations (DataType valueType)
{
	return isDataTypeMatrix(valueType) ? getDataTypeMatrixNumColumns(valueType) : 1;
}

// This functions builds a matching vertex shader for a 'both' case, when
// the fragment shader is being tested.
// We need to build attributes and varyings for each 'input'.
string genVertexShader (const ShaderCaseSpecification& spec)
{
	ostringstream	res;
	int				curInputLoc		= 0;
	int				curOutputLoc	= 0;

	res << glu::getGLSLVersionDeclaration(spec.targetVersion) << "\n";

	// Declarations (position + attribute/varying for each input).
	res << "precision highp float;\n";
	res << "precision highp int;\n";
	res << "\n";
	res << "layout(location = 0) in highp vec4 dEQP_Position;\n";
	curInputLoc += 1;

	for (size_t ndx = 0; ndx < spec.values.inputs.size(); ndx++)
	{
		const Value&		val					= spec.values.inputs[ndx];
		const DataType		valueType			= val.type.getBasicType();
		const DataType		transportType		= getTransportType(valueType);
		const char* const	transportTypeStr	= getDataTypeName(transportType);
		const int			numLocs				= getNumTransportLocations(valueType);

		res << "layout(location = " << curInputLoc << ") in " << transportTypeStr << " a_" << val.name << ";\n";
		res << "layout(location = " << curOutputLoc << ") flat out " << transportTypeStr << " " << (transportType != valueType ? "v_" : "") << val.name << ";\n";

		curInputLoc		+= numLocs;
		curOutputLoc	+= numLocs;
	}
	res << "\n";

	// Main function.
	// - gl_Position = dEQP_Position;
	// - for each input: write attribute directly to varying
	res << "void main()\n";
	res << "{\n";
	res << "	gl_Position = dEQP_Position;\n";
	for (size_t ndx = 0; ndx < spec.values.inputs.size(); ndx++)
	{
		const Value&	val		= spec.values.inputs[ndx];
		const string&	name	= val.name;

		res << "	" << (getTransportType(val.type.getBasicType()) != val.type.getBasicType() ? "v_" : "")
			<< name << " = a_" << name << ";\n";
	}

	res << "}\n";
	return res.str();
}

void genCompareOp (ostringstream& output, const char* dstVec4Var, const ValueBlock& valueBlock, const char* checkVarName)
{
	bool isFirstOutput = true;

	for (size_t ndx = 0; ndx < valueBlock.outputs.size(); ndx++)
	{
		const Value&	val		= valueBlock.outputs[ndx];

		// Check if we're only interested in one variable (then skip if not the right one).
		if (checkVarName && val.name != checkVarName)
			continue;

		// Prefix.
		if (isFirstOutput)
		{
			output << "bool RES = ";
			isFirstOutput = false;
		}
		else
			output << "RES = RES && ";

		// Generate actual comparison.
		if (getDataTypeScalarType(val.type.getBasicType()) == glu::TYPE_FLOAT)
			output << "isOk(" << val.name << ", ref." << val.name << ", 0.05);\n";
		else
			output << "isOk(" << val.name << ", ref." << val.name << ");\n";
	}

	if (isFirstOutput)
		output << dstVec4Var << " = vec4(1.0);\n";
	else
		output << dstVec4Var << " = vec4(RES, RES, RES, 1.0);\n";
}

string genFragmentShader (const ShaderCaseSpecification& spec)
{
	ostringstream	shader;
	ostringstream	setup;
	int				curInLoc	= 0;

	shader << glu::getGLSLVersionDeclaration(spec.targetVersion) << "\n";

	shader << "precision highp float;\n";
	shader << "precision highp int;\n";
	shader << "\n";

	shader << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";
	shader << "\n";

	genCompareFunctions(shader, spec.values, false);
	shader << "\n";

	// Declarations (varying, reference for each output).
	for (size_t ndx = 0; ndx < spec.values.outputs.size(); ndx++)
	{
		const Value&		val					= spec.values.outputs[ndx];
		const DataType		valueType			= val.type.getBasicType();
		const char*	const	valueTypeStr		= getDataTypeName(valueType);
		const DataType		transportType		= getTransportType(valueType);
		const char* const	transportTypeStr	= getDataTypeName(transportType);
		const int			numLocs				= getNumTransportLocations(valueType);

		shader << "layout(location = " << curInLoc << ") flat in " << transportTypeStr << " " << (valueType != transportType ? "v_" : "") << val.name << ";\n";

		if (valueType != transportType)
			setup << "	" << valueTypeStr << " " << val.name << " = " << valueTypeStr << "(v_" << val.name << ");\n";

		curInLoc += numLocs;
	}

	declareReferenceBlock(shader, spec.values);

	shader << "\n";
	shader << "void main()\n";
	shader << "{\n";

	shader << setup.str();

	shader << "	";
	genCompareOp(shader, "dEQP_FragColor", spec.values, DE_NULL);

	shader << "}\n";
	return shader.str();
}

// Specialize a shader for the vertex shader test case.
string specializeVertexShader (const ShaderCaseSpecification& spec, const string& src)
{
	ostringstream		decl;
	ostringstream		setup;
	ostringstream		output;
	int					curInputLoc		= 0;
	int					curOutputLoc	= 0;

	// generated from "both" case
	DE_ASSERT(spec.caseType == glu::sl::CASETYPE_VERTEX_ONLY);

	// Output (write out position).
	output << "gl_Position = dEQP_Position;\n";

	// Declarations (position + attribute for each input, varying for each output).
	decl << "layout(location = 0) in highp vec4 dEQP_Position;\n";
	curInputLoc += 1;

	for (size_t ndx = 0; ndx < spec.values.inputs.size(); ndx++)
	{
		const Value&		val					= spec.values.inputs[ndx];
		const DataType		valueType			= val.type.getBasicType();
		const char*	const	valueTypeStr		= getDataTypeName(valueType);
		const DataType		transportType		= getTransportType(valueType);
		const char* const	transportTypeStr	= getDataTypeName(transportType);
		const int			numLocs				= getNumTransportLocations(valueType);

		decl << "layout(location = " << curInputLoc << ") in ";

		curInputLoc += numLocs;

		if (valueType == transportType)
			decl << transportTypeStr << " " << val.name << ";\n";
		else
		{
			decl << transportTypeStr << " a_" << val.name << ";\n";
			setup << valueTypeStr << " " << val.name << " = " << valueTypeStr << "(a_" << val.name << ");\n";
		}
	}

	declareUniforms(decl, spec.values);

	for (size_t ndx = 0; ndx < spec.values.outputs.size(); ndx++)
	{
		const Value&		val					= spec.values.outputs[ndx];
		const DataType		valueType			= val.type.getBasicType();
		const char*	const	valueTypeStr		= getDataTypeName(valueType);
		const DataType		transportType		= getTransportType(valueType);
		const char* const	transportTypeStr	= getDataTypeName(transportType);
		const int			numLocs				= getNumTransportLocations(valueType);

		decl << "layout(location = " << curOutputLoc << ") flat out ";

		curOutputLoc += numLocs;

		if (valueType == transportType)
			decl << transportTypeStr << " " << val.name << ";\n";
		else
		{
			decl << transportTypeStr << " v_" << val.name << ";\n";
			decl << valueTypeStr << " " << val.name << ";\n";

			output << "v_" << val.name << " = " << transportTypeStr << "(" << val.name << ");\n";
		}
	}

	// Shader specialization.
	map<string, string> params;
	params.insert(pair<string, string>("DECLARATIONS", decl.str()));
	params.insert(pair<string, string>("SETUP", setup.str()));
	params.insert(pair<string, string>("OUTPUT", output.str()));
	params.insert(pair<string, string>("POSITION_FRAG_COLOR", "gl_Position"));

	StringTemplate	tmpl	(src);
	const string	baseSrc	= tmpl.specialize(params);
	const string	withExt	= injectExtensionRequirements(baseSrc, spec.programs[0].requiredExtensions, glu::SHADERTYPE_VERTEX);

	return withExt;
}

// Specialize a shader for the fragment shader test case.
string specializeFragmentShader (const ShaderCaseSpecification& spec, const string& src)
{
	ostringstream		decl;
	ostringstream		setup;
	ostringstream		output;
	int					curInputLoc	= 0;

	// generated from "both" case
	DE_ASSERT(spec.caseType == glu::sl::CASETYPE_FRAGMENT_ONLY);

	genCompareFunctions(decl, spec.values, false);
	genCompareOp(output, "dEQP_FragColor", spec.values, DE_NULL);

	decl << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";

	for (size_t ndx = 0; ndx < spec.values.inputs.size(); ndx++)
	{
		const Value&		val					= spec.values.inputs[ndx];
		const DataType		valueType			= val.type.getBasicType();
		const char*	const	valueTypeStr		= getDataTypeName(valueType);
		const DataType		transportType		= getTransportType(valueType);
		const char* const	transportTypeStr	= getDataTypeName(transportType);
		const int			numLocs				= getNumTransportLocations(valueType);

		decl << "layout(location = " << curInputLoc << ") flat in ";

		curInputLoc += numLocs;

		if (valueType == transportType)
			decl << transportTypeStr << " " << val.name << ";\n";
		else
		{
			decl << transportTypeStr << " v_" << val.name << ";\n";
			setup << valueTypeStr << " " << val.name << " = " << valueTypeStr << "(v_" << val.name << ");\n";
		}
	}

	declareUniforms(decl, spec.values);
	declareReferenceBlock(decl, spec.values);

	for (size_t ndx = 0; ndx < spec.values.outputs.size(); ndx++)
	{
		const Value&		val				= spec.values.outputs[ndx];
		const DataType		basicType		= val.type.getBasicType();
		const char* const	refTypeStr		= getDataTypeName(basicType);

		decl << refTypeStr << " " << val.name << ";\n";
	}

	// Shader specialization.
	map<string, string> params;
	params.insert(pair<string, string>("DECLARATIONS", decl.str()));
	params.insert(pair<string, string>("SETUP", setup.str()));
	params.insert(pair<string, string>("OUTPUT", output.str()));
	params.insert(pair<string, string>("POSITION_FRAG_COLOR", "dEQP_FragColor"));

	StringTemplate	tmpl	(src);
	const string	baseSrc	= tmpl.specialize(params);
	const string	withExt	= injectExtensionRequirements(baseSrc, spec.programs[0].requiredExtensions, glu::SHADERTYPE_FRAGMENT);

	return withExt;
}

map<string, string> generateVertexSpecialization (const ProgramSpecializationParams& specParams)
{
	ostringstream			decl;
	ostringstream			setup;
	map<string, string>		params;
	int						curInputLoc		= 0;

	decl << "layout(location = 0) in highp vec4 dEQP_Position;\n";
	curInputLoc += 1;

	for (size_t ndx = 0; ndx < specParams.caseSpec.values.inputs.size(); ndx++)
	{
		const Value&		val					= specParams.caseSpec.values.inputs[ndx];
		const DataType		valueType			= val.type.getBasicType();
		const char*	const	valueTypeStr		= getDataTypeName(valueType);
		const DataType		transportType		= getTransportType(valueType);
		const char* const	transportTypeStr	= getDataTypeName(transportType);
		const int			numLocs				= getNumTransportLocations(valueType);

		decl << "layout(location = " << curInputLoc << ") in ";

		curInputLoc += numLocs;

		if (valueType == transportType)
			decl << transportTypeStr << " " << val.name << ";\n";
		else
		{
			decl << transportTypeStr << " a_" << val.name << ";\n";
			setup << valueTypeStr << " " << val.name << " = " << valueTypeStr << "(a_" << val.name << ");\n";
		}
	}

	declareUniforms(decl, specParams.caseSpec.values);

	params.insert(pair<string, string>("VERTEX_DECLARATIONS",	decl.str()));
	params.insert(pair<string, string>("VERTEX_SETUP",			setup.str()));
	params.insert(pair<string, string>("VERTEX_OUTPUT",			string("gl_Position = dEQP_Position;\n")));

	return params;
}

map<string, string> generateFragmentSpecialization (const ProgramSpecializationParams& specParams)
{
	ostringstream		decl;
	ostringstream		output;
	map<string, string>	params;

	genCompareFunctions(decl, specParams.caseSpec.values, false);
	genCompareOp(output, "dEQP_FragColor", specParams.caseSpec.values, DE_NULL);

	decl << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";

	for (size_t ndx = 0; ndx < specParams.caseSpec.values.outputs.size(); ndx++)
	{
		const Value&		val			= specParams.caseSpec.values.outputs[ndx];
		const char*	const	refTypeStr	= getDataTypeName(val.type.getBasicType());

		decl << refTypeStr << " " << val.name << ";\n";
	}

	declareReferenceBlock(decl, specParams.caseSpec.values);
	declareUniforms(decl, specParams.caseSpec.values);

	params.insert(pair<string, string>("FRAGMENT_DECLARATIONS",	decl.str()));
	params.insert(pair<string, string>("FRAGMENT_OUTPUT",		output.str()));
	params.insert(pair<string, string>("FRAG_COLOR",			"dEQP_FragColor"));

	return params;
}

map<string, string> generateGeometrySpecialization (const ProgramSpecializationParams& specParams)
{
	ostringstream		decl;
	map<string, string>	params;

	decl << "layout (triangles) in;\n";
	decl << "layout (triangle_strip, max_vertices=3) out;\n";
	decl << "\n";

	declareUniforms(decl, specParams.caseSpec.values);

	params.insert(pair<string, string>("GEOMETRY_DECLARATIONS",		decl.str()));

	return params;
}

map<string, string> generateTessControlSpecialization (const ProgramSpecializationParams& specParams)
{
	ostringstream		decl;
	ostringstream		output;
	map<string, string>	params;

	decl << "layout (vertices=3) out;\n";
	decl << "\n";

	declareUniforms(decl, specParams.caseSpec.values);

	output <<	"gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
				"gl_TessLevelInner[0] = 2.0;\n"
				"gl_TessLevelInner[1] = 2.0;\n"
				"gl_TessLevelOuter[0] = 2.0;\n"
				"gl_TessLevelOuter[1] = 2.0;\n"
				"gl_TessLevelOuter[2] = 2.0;\n"
				"gl_TessLevelOuter[3] = 2.0;";

	params.insert(pair<string, string>("TESSELLATION_CONTROL_DECLARATIONS",	decl.str()));
	params.insert(pair<string, string>("TESSELLATION_CONTROL_OUTPUT",		output.str()));
	params.insert(pair<string, string>("GL_MAX_PATCH_VERTICES",				de::toString(specParams.maxPatchVertices)));

	return params;
}

map<string, string> generateTessEvalSpecialization (const ProgramSpecializationParams& specParams)
{
	ostringstream		decl;
	ostringstream		output;
	map<string, string>	params;

	decl << "layout (triangles) in;\n";
	decl << "\n";

	declareUniforms(decl, specParams.caseSpec.values);

	output <<	"gl_Position = gl_TessCoord[0] * gl_in[0].gl_Position + gl_TessCoord[1] * gl_in[1].gl_Position + gl_TessCoord[2] * gl_in[2].gl_Position;\n";

	params.insert(pair<string, string>("TESSELLATION_EVALUATION_DECLARATIONS",	decl.str()));
	params.insert(pair<string, string>("TESSELLATION_EVALUATION_OUTPUT",		output.str()));
	params.insert(pair<string, string>("GL_MAX_PATCH_VERTICES",					de::toString(specParams.maxPatchVertices)));

	return params;
}

void specializeShaderSources (ProgramSources&						dst,
							  const ProgramSources&					src,
							  const ProgramSpecializationParams&	specParams,
							  glu::ShaderType						shaderType,
							  map<string, string>					(*specializationGenerator) (const ProgramSpecializationParams& specParams))
{
	if (!src.sources[shaderType].empty())
	{
		const map<string, string>	tmplParams	= specializationGenerator(specParams);

		for (size_t ndx = 0; ndx < src.sources[shaderType].size(); ++ndx)
		{
			const StringTemplate	tmpl			(src.sources[shaderType][ndx]);
			const string			baseGLSLCode	= tmpl.specialize(tmplParams);
			const string			sourceWithExts	= injectExtensionRequirements(baseGLSLCode, specParams.requiredExtensions, shaderType);

			dst << glu::ShaderSource(shaderType, sourceWithExts);
		}
	}
}

void specializeProgramSources (glu::ProgramSources&					dst,
							   const glu::ProgramSources&			src,
							   const ProgramSpecializationParams&	specParams)
{
	specializeShaderSources(dst, src, specParams, glu::SHADERTYPE_VERTEX,					generateVertexSpecialization);
	specializeShaderSources(dst, src, specParams, glu::SHADERTYPE_FRAGMENT,					generateFragmentSpecialization);
	specializeShaderSources(dst, src, specParams, glu::SHADERTYPE_GEOMETRY,					generateGeometrySpecialization);
	specializeShaderSources(dst, src, specParams, glu::SHADERTYPE_TESSELLATION_CONTROL,		generateTessControlSpecialization);
	specializeShaderSources(dst, src, specParams, glu::SHADERTYPE_TESSELLATION_EVALUATION,	generateTessEvalSpecialization);

	dst << glu::ProgramSeparable(src.separable);
}

struct ValueBufferLayout
{
	struct Entry
	{
		int		offset;
		int		vecStride;	//! Applies to matrices only

		Entry (void) : offset(0), vecStride(0) {}
		Entry (int offset_, int vecStride_) : offset(offset_), vecStride(vecStride_) {}
	};

	vector<Entry>	entries;
	int				size;

	ValueBufferLayout (void) : size(0) {}
};

ValueBufferLayout computeStd140Layout (const vector<Value>& values)
{
	ValueBufferLayout layout;

	layout.entries.resize(values.size());

	for (size_t ndx = 0; ndx < values.size(); ++ndx)
	{
		const DataType	basicType	= values[ndx].type.getBasicType();
		const bool		isMatrix	= isDataTypeMatrix(basicType);
		const int		numVecs		= isMatrix ? getDataTypeMatrixNumColumns(basicType) : 1;
		const DataType	vecType		= isMatrix ? glu::getDataTypeFloatVec(getDataTypeMatrixNumRows(basicType)) : basicType;
		const int		vecSize		= getDataTypeScalarSize(vecType);
		const int		alignment	= ((isMatrix || vecSize == 3) ? 4 : vecSize)*int(sizeof(deUint32));

		layout.size			= deAlign32(layout.size, alignment);
		layout.entries[ndx] = ValueBufferLayout::Entry(layout.size, alignment);
		layout.size			+= alignment*(numVecs-1) + vecSize*int(sizeof(deUint32));
	}

	return layout;
}

ValueBufferLayout computeStd430Layout (const vector<Value>& values)
{
	ValueBufferLayout layout;

	layout.entries.resize(values.size());

	for (size_t ndx = 0; ndx < values.size(); ++ndx)
	{
		const DataType	basicType	= values[ndx].type.getBasicType();
		const int		numVecs		= isDataTypeMatrix(basicType) ? getDataTypeMatrixNumColumns(basicType) : 1;
		const DataType	vecType		= isDataTypeMatrix(basicType) ? glu::getDataTypeFloatVec(getDataTypeMatrixNumRows(basicType)) : basicType;
		const int		vecSize		= getDataTypeScalarSize(vecType);
		const int		alignment	= (vecSize == 3 ? 4 : vecSize)*int(sizeof(deUint32));

		layout.size			= deAlign32(layout.size, alignment);
		layout.entries[ndx] = ValueBufferLayout::Entry(layout.size, alignment);
		layout.size			+= alignment*(numVecs-1) + vecSize*int(sizeof(deUint32));
	}

	return layout;
}

void copyToLayout (void* dst, const ValueBufferLayout::Entry& entryLayout, const Value& value, int arrayNdx)
{
	const DataType	basicType	= value.type.getBasicType();
	const int		scalarSize	= getDataTypeScalarSize(basicType);
	const int		numVecs		= isDataTypeMatrix(basicType) ? getDataTypeMatrixNumColumns(basicType) : 1;
	const int		numComps	= isDataTypeMatrix(basicType) ? getDataTypeMatrixNumRows(basicType) : scalarSize;

	DE_ASSERT(size_t((arrayNdx+1)*scalarSize) <= value.elements.size());

	if (isDataTypeBoolOrBVec(basicType))
	{
		for (int vecNdx = 0; vecNdx < numVecs; vecNdx++)
		{
			for (int compNdx = 0; compNdx < numComps; compNdx++)
			{
				const deUint32 data = value.elements[arrayNdx*scalarSize + vecNdx*numComps + compNdx].bool32 ? ~0u : 0u;

				deMemcpy((deUint8*)dst + entryLayout.offset + vecNdx*entryLayout.vecStride + compNdx * sizeof(deUint32),
						 &data,
						 sizeof(deUint32));
			}
		}
	}
	else
	{
		for (int vecNdx = 0; vecNdx < numVecs; vecNdx++)
			deMemcpy((deUint8*)dst + entryLayout.offset + vecNdx*entryLayout.vecStride,
					 &value.elements[arrayNdx*scalarSize + vecNdx*numComps],
					 numComps*sizeof(deUint32));
	}
}

void copyToLayout (void* dst, const ValueBufferLayout& layout, const vector<Value>& values, int arrayNdx)
{
	DE_ASSERT(layout.entries.size() == values.size());

	for (size_t ndx = 0; ndx < values.size(); ndx++)
		copyToLayout(dst, layout.entries[ndx], values[ndx], arrayNdx);
}

deUint32 getShaderStages (const ShaderCaseSpecification& spec)
{
	if (spec.caseType == glu::sl::CASETYPE_COMPLETE)
	{
		deUint32	stages	= 0u;

		for (size_t progNdx = 0; progNdx < spec.programs.size(); progNdx++)
		{
			for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
			{
				if (!spec.programs[progNdx].sources.sources[shaderType].empty())
					stages |= (1u << shaderType);
			}
		}

		return stages;
	}
	else
		return (1u << glu::SHADERTYPE_VERTEX) | (1u << glu::SHADERTYPE_FRAGMENT);
}

class PipelineProgram
{
public:
								PipelineProgram		(Context& context, const ShaderCaseSpecification& spec);

	deUint32					getStages			(void) const					{ return m_stages;							}

	bool						hasShader			(glu::ShaderType type) const	{ return (m_stages & (1u << type)) != 0;	}
	vk::VkShaderModule			getShader			(glu::ShaderType type) const	{ return *m_shaderModules[type];			}

private:
	const deUint32				m_stages;
	Move<vk::VkShaderModule>	m_shaderModules[glu::SHADERTYPE_LAST];
};

PipelineProgram::PipelineProgram (Context& context, const ShaderCaseSpecification& spec)
	: m_stages(getShaderStages(spec))
{
	// \note Currently only a single source program is supported as framework lacks SPIR-V linking capability
	TCU_CHECK_INTERNAL(spec.programs.size() == 1);

	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
	{
		if ((m_stages & (1u << shaderType)) != 0)
		{
			m_shaderModules[shaderType]	= vk::createShaderModule(context.getDeviceInterface(), context.getDevice(),
																 context.getBinaryCollection().get(getShaderName((glu::ShaderType)shaderType, 0)), 0u);
		}
	}
}

vector<vk::VkPipelineShaderStageCreateInfo> getPipelineShaderStageCreateInfo (const PipelineProgram& program)
{
	vector<vk::VkPipelineShaderStageCreateInfo>	infos;

	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
	{
		if (program.hasShader((glu::ShaderType)shaderType))
		{
			const vk::VkPipelineShaderStageCreateInfo info =
			{
				vk::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// sType
				DE_NULL,													// pNext
				(vk::VkPipelineShaderStageCreateFlags)0,
				vk::getVkShaderStage((glu::ShaderType)shaderType),			// stage
				program.getShader((glu::ShaderType)shaderType),				// module
				"main",
				DE_NULL,													// pSpecializationInfo
			};

			infos.push_back(info);
		}
	}

	return infos;
}

Move<vk::VkBuffer> createBuffer (Context& context, vk::VkDeviceSize size, vk::VkBufferUsageFlags usageFlags)
{
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const vk::VkBufferCreateInfo	params				=
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// sType
		DE_NULL,									// pNext
		0u,											// flags
		size,										// size
		usageFlags,									// usage
		vk::VK_SHARING_MODE_EXCLUSIVE,				// sharingMode
		1u,											// queueFamilyCount
		&queueFamilyIndex,							// pQueueFamilyIndices
	};

	return vk::createBuffer(context.getDeviceInterface(), context.getDevice(), &params);
}

Move<vk::VkImage> createImage2D (Context& context, deUint32 width, deUint32 height, vk::VkFormat format, vk::VkImageTiling tiling, vk::VkImageUsageFlags usageFlags)
{
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const vk::VkImageCreateInfo		params				=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// sType
		DE_NULL,									// pNext
		0u,											// flags
		vk::VK_IMAGE_TYPE_2D,						// imageType
		format,										// format
		{ width, height, 1u },						// extent
		1u,											// mipLevels
		1u,											// arraySize
		vk::VK_SAMPLE_COUNT_1_BIT,					// samples
		tiling,										// tiling
		usageFlags,									// usage
		vk::VK_SHARING_MODE_EXCLUSIVE,				// sharingMode
		1u,											// queueFamilyCount
		&queueFamilyIndex,							// pQueueFamilyIndices
		vk::VK_IMAGE_LAYOUT_UNDEFINED,				// initialLayout
	};

	return vk::createImage(context.getDeviceInterface(), context.getDevice(), &params);
}

Move<vk::VkImageView> createAttachmentView (Context& context, vk::VkImage image, vk::VkFormat format)
{
	const vk::VkImageViewCreateInfo	params				=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// sType
		DE_NULL,											// pNext
		0u,													// flags
		image,												// image
		vk::VK_IMAGE_VIEW_TYPE_2D,							// viewType
		format,												// format
		vk::makeComponentMappingRGBA(),						// channels
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,						// aspectMask
			0u,													// baseMipLevel
			1u,													// mipLevels
			0u,													// baseArrayLayer
			1u,													// arraySize
		},													// subresourceRange
	};

	return vk::createImageView(context.getDeviceInterface(), context.getDevice(), &params);
}

Move<vk::VkRenderPass> createRenderPass (Context& context, vk::VkFormat colorAttFormat, deUint32 size)
{
	vk::VkAttachmentDescription	colorAttDesc[4];
	vk::VkAttachmentReference	colorAttRef[4];

	for (deUint32 i = 0; i < size; i++)
	{
		vk::VkAttachmentDescription	desc =
		{
			0u,														// flags
			colorAttFormat,											// format
			vk::VK_SAMPLE_COUNT_1_BIT,								// samples
			vk::VK_ATTACHMENT_LOAD_OP_CLEAR,						// loadOp
			vk::VK_ATTACHMENT_STORE_OP_STORE,						// storeOp
			vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// stencilLoadOp
			vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,					// stencilStoreOp
			vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// initialLayout
			vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// finalLayout
		};
		colorAttDesc[i] = desc;

		vk::VkAttachmentReference	ref =
		{
			i,														// attachment
			vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// layout
		};
		colorAttRef[i] = ref;
	}

	const vk::VkAttachmentReference		dsAttRef			=
	{
		VK_ATTACHMENT_UNUSED,									// attachment
		vk::VK_IMAGE_LAYOUT_GENERAL,							// layout
	};
	const vk::VkSubpassDescription		subpassDesc			=
	{
		(vk::VkSubpassDescriptionFlags)0,
		vk::VK_PIPELINE_BIND_POINT_GRAPHICS,					// pipelineBindPoint
		0u,														// inputCount
		DE_NULL,												// pInputAttachments
		size,													// colorCount
		&colorAttRef[0],										// pColorAttachments
		DE_NULL,												// pResolveAttachments
		&dsAttRef,												// depthStencilAttachment
		0u,														// preserveCount
		DE_NULL,												// pPreserveAttachments

	};
	const vk::VkRenderPassCreateInfo	renderPassParams	=
	{
		vk::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// sType
		DE_NULL,												// pNext
		(vk::VkRenderPassCreateFlags)0,
		size,													// attachmentCount
		&colorAttDesc[0],										// pAttachments
		1u,														// subpassCount
		&subpassDesc,											// pSubpasses
		0u,														// dependencyCount
		DE_NULL,												// pDependencies
	};

	return vk::createRenderPass(context.getDeviceInterface(), context.getDevice(), &renderPassParams);
}

vk::VkShaderStageFlags getVkStageFlags (deUint32 stages)
{
	vk::VkShaderStageFlags	vkStages	= 0u;

	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
	{
		if ((stages & (1u << shaderType)) != 0)
			vkStages |= vk::getVkShaderStage((glu::ShaderType)shaderType);
	}

	return vkStages;
}

Move<vk::VkDescriptorSetLayout> createDescriptorSetLayout (Context& context, deUint32 shaderStages)
{
	DE_STATIC_ASSERT(REFERENCE_UNIFORM_BINDING	== 0);
	DE_STATIC_ASSERT(USER_UNIFORM_BINDING		== 1);

	return vk::DescriptorSetLayoutBuilder()
				.addSingleBinding(vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vk::VK_SHADER_STAGE_FRAGMENT_BIT)
				.addSingleBinding(vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, getVkStageFlags(shaderStages))
				.build(context.getDeviceInterface(), context.getDevice());
}

Move<vk::VkPipelineLayout> createPipelineLayout (Context& context, vk::VkDescriptorSetLayout descriptorSetLayout)
{
	const vk::VkPipelineLayoutCreateInfo	params	=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// sType
		DE_NULL,											// pNext
		(vk::VkPipelineLayoutCreateFlags)0,
		1u,													// descriptorSetCount
		&descriptorSetLayout,								// pSetLayouts
		0u,													// pushConstantRangeCount
		DE_NULL,											// pPushConstantRanges
	};

	return vk::createPipelineLayout(context.getDeviceInterface(), context.getDevice(), &params);
}

vk::VkFormat getVecFormat (DataType scalarType, int scalarSize)
{
	switch (scalarType)
	{
		case glu::TYPE_FLOAT:
		{
			const vk::VkFormat vecFmts[] =
			{
				vk::VK_FORMAT_R32_SFLOAT,
				vk::VK_FORMAT_R32G32_SFLOAT,
				vk::VK_FORMAT_R32G32B32_SFLOAT,
				vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			};
			return de::getSizedArrayElement<4>(vecFmts, scalarSize-1);
		}

		case glu::TYPE_INT:
		{
			const vk::VkFormat vecFmts[] =
			{
				vk::VK_FORMAT_R32_SINT,
				vk::VK_FORMAT_R32G32_SINT,
				vk::VK_FORMAT_R32G32B32_SINT,
				vk::VK_FORMAT_R32G32B32A32_SINT,
			};
			return de::getSizedArrayElement<4>(vecFmts, scalarSize-1);
		}

		case glu::TYPE_UINT:
		{
			const vk::VkFormat vecFmts[] =
			{
				vk::VK_FORMAT_R32_UINT,
				vk::VK_FORMAT_R32G32_UINT,
				vk::VK_FORMAT_R32G32B32_UINT,
				vk::VK_FORMAT_R32G32B32A32_UINT,
			};
			return de::getSizedArrayElement<4>(vecFmts, scalarSize-1);
		}

		case glu::TYPE_BOOL:
		{
			const vk::VkFormat vecFmts[] =
			{
				vk::VK_FORMAT_R32_UINT,
				vk::VK_FORMAT_R32G32_UINT,
				vk::VK_FORMAT_R32G32B32_UINT,
				vk::VK_FORMAT_R32G32B32A32_UINT,
			};
			return de::getSizedArrayElement<4>(vecFmts, scalarSize-1);
		}

		default:
			DE_FATAL("Unknown scalar type");
			return vk::VK_FORMAT_R8G8B8A8_UINT;
	}
}

vector<vk::VkVertexInputAttributeDescription> getVertexAttributeDescriptions (const vector<Value>& inputValues, const ValueBufferLayout& layout)
{
	vector<vk::VkVertexInputAttributeDescription>	attribs;

	// Position
	{
		const vk::VkVertexInputAttributeDescription	posDesc	=
		{
			0u,								// location
			0u,								// binding
			vk::VK_FORMAT_R32G32_SFLOAT,	// format
			0u,								// offset
		};

		attribs.push_back(posDesc);
	}

	// Input values
	for (size_t inputNdx = 0; inputNdx < inputValues.size(); inputNdx++)
	{
		const Value&					input		= inputValues[inputNdx];
		const ValueBufferLayout::Entry&	layoutEntry	= layout.entries[inputNdx];
		const DataType					basicType	= input.type.getBasicType();
		const int						numVecs		= isDataTypeMatrix(basicType)
													? getDataTypeMatrixNumColumns(basicType)
													: 1;
		const int						vecSize		= isDataTypeMatrix(basicType)
													? getDataTypeMatrixNumRows(basicType)
													: getDataTypeScalarSize(basicType);
		const DataType					scalarType	= getDataTypeScalarType(basicType);
		const vk::VkFormat				vecFmt		= getVecFormat(scalarType, vecSize);

		for (int vecNdx = 0; vecNdx < numVecs; vecNdx++)
		{
			const deUint32								curLoc	= (deUint32)attribs.size();
			const deUint32								offset	= (deUint32)(layoutEntry.offset + layoutEntry.vecStride*vecNdx);
			const vk::VkVertexInputAttributeDescription	desc	=
			{
				curLoc,		// location
				1u,			// binding
				vecFmt,		// format
				offset,		// offset
			};

			attribs.push_back(desc);
		}
	}

	return attribs;
}

Move<vk::VkPipeline> createPipeline (Context&					context,
									 const vector<Value>&		inputValues,
									 const ValueBufferLayout&	inputLayout,
									 const PipelineProgram&		program,
									 vk::VkRenderPass			renderPass,
									 vk::VkPipelineLayout		pipelineLayout,
									 tcu::UVec2					renderSize,
									 deUint32					size)
{
	const vector<vk::VkPipelineShaderStageCreateInfo>	shaderStageParams		(getPipelineShaderStageCreateInfo(program));
	const vector<vk::VkVertexInputAttributeDescription>	vertexAttribParams		(getVertexAttributeDescriptions(inputValues, inputLayout));
	const vk::VkPipelineDepthStencilStateCreateInfo		depthStencilParams		=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,		// sType
		DE_NULL,															// pNext
		(vk::VkPipelineDepthStencilStateCreateFlags)0,
		VK_FALSE,															// depthTestEnable
		VK_FALSE,															// depthWriteEnable
		vk::VK_COMPARE_OP_ALWAYS,											// depthCompareOp
		VK_FALSE,															// depthBoundsTestEnable
		VK_FALSE,															// stencilTestEnable
		{
			vk::VK_STENCIL_OP_KEEP,												// stencilFailOp;
			vk::VK_STENCIL_OP_KEEP,												// stencilPassOp;
			vk::VK_STENCIL_OP_KEEP,												// stencilDepthFailOp;
			vk::VK_COMPARE_OP_ALWAYS,											// stencilCompareOp;
			0u,																	// stencilCompareMask
			0u,																	// stencilWriteMask
			0u,																	// stencilReference
		},																	// front;
		{
			vk::VK_STENCIL_OP_KEEP,												// stencilFailOp;
			vk::VK_STENCIL_OP_KEEP,												// stencilPassOp;
			vk::VK_STENCIL_OP_KEEP,												// stencilDepthFailOp;
			vk::VK_COMPARE_OP_ALWAYS,											// stencilCompareOp;
			0u,																	// stencilCompareMask
			0u,																	// stencilWriteMask
			0u,																	// stencilReference
		},																	// back;
		-1.0f,																// minDepthBounds
		+1.0f,																// maxDepthBounds
	};
	const vk::VkViewport								viewport0				=
	{
		0.0f,																// originX
		0.0f,																// originY
		(float)renderSize.x(),												// width
		(float)renderSize.y(),												// height
		0.0f,																// minDepth
		1.0f,																// maxDepth
	};
	const vk::VkRect2D									scissor0				=
	{
		{ 0u, 0u },															// offset
		{ renderSize.x(), renderSize.y() }									// extent
	};
	const vk::VkPipelineViewportStateCreateInfo			viewportParams			=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// sType
		DE_NULL,															// pNext
		(vk::VkPipelineViewportStateCreateFlags)0,
		1u,																	// viewportCount
		&viewport0,															// pViewports
		1u,																	// scissorCount
		&scissor0,															// pScissors
	};
	const vk::VkPipelineMultisampleStateCreateInfo		multisampleParams		=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// sType
		DE_NULL,															// pNext
		(vk::VkPipelineMultisampleStateCreateFlags)0,
		vk::VK_SAMPLE_COUNT_1_BIT,											// rasterSamples
		DE_FALSE,															// sampleShadingEnable
		0.0f,																// minSampleShading
		DE_NULL,															// pSampleMask
		VK_FALSE,															// alphaToCoverageEnable
		VK_FALSE,															// alphaToOneEnable
	};
	const vk::VkPipelineRasterizationStateCreateInfo	rasterParams			=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// sType
		DE_NULL,															// pNext
		(vk::VkPipelineRasterizationStateCreateFlags)0,
		DE_FALSE,															// depthClipEnable
		DE_FALSE,															// rasterizerDiscardEnable
		vk::VK_POLYGON_MODE_FILL,											// fillMode
		vk::VK_CULL_MODE_NONE,												// cullMode;
		vk::VK_FRONT_FACE_COUNTER_CLOCKWISE,								// frontFace;
		VK_FALSE,															// depthBiasEnable
		0.0f,																// depthBiasConstantFactor
		0.0f,																// depthBiasClamp
		0.0f,																// depthBiasSlopeFactor
		1.0f,																// lineWidth
	};
	const vk::VkPipelineInputAssemblyStateCreateInfo	inputAssemblyParams		=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// sType
		DE_NULL,															// pNext
		(vk::VkPipelineInputAssemblyStateCreateFlags)0,
		vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,							// topology
		DE_FALSE,															// primitiveRestartEnable
	};
	const vk::VkVertexInputBindingDescription			vertexBindings[]		=
	{
		{
			0u,																	// binding
			(deUint32)sizeof(tcu::Vec2),										// stride
			vk::VK_VERTEX_INPUT_RATE_VERTEX,									// stepRate
		},
		{
			1u,																	// binding
			0u,																	// stride
			vk::VK_VERTEX_INPUT_RATE_INSTANCE,									// stepRate
		},
	};
	const vk::VkPipelineVertexInputStateCreateInfo		vertexInputStateParams	=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// sType
		DE_NULL,															// pNext
		(vk::VkPipelineVertexInputStateCreateFlags)0,
		(inputValues.empty() ? 1u : 2u),									// bindingCount
		vertexBindings,														// pVertexBindingDescriptions
		(deUint32)vertexAttribParams.size(),								// attributeCount
		&vertexAttribParams[0],												// pVertexAttributeDescriptions
	};
	const vk::VkColorComponentFlags						allCompMask				= vk::VK_COLOR_COMPONENT_R_BIT
																				| vk::VK_COLOR_COMPONENT_G_BIT
																				| vk::VK_COLOR_COMPONENT_B_BIT
																				| vk::VK_COLOR_COMPONENT_A_BIT;
	vk::VkPipelineColorBlendAttachmentState		attBlendParams[4];
	for (deUint32 i = 0; i < size; i++)
	{
		vk::VkPipelineColorBlendAttachmentState blend =
		{
			VK_FALSE,															// blendEnable
			vk::VK_BLEND_FACTOR_ONE,											// srcBlendColor
			vk::VK_BLEND_FACTOR_ZERO,											// destBlendColor
			vk::VK_BLEND_OP_ADD,												// blendOpColor
			vk::VK_BLEND_FACTOR_ONE,											// srcBlendAlpha
			vk::VK_BLEND_FACTOR_ZERO,											// destBlendAlpha
			vk::VK_BLEND_OP_ADD,												// blendOpAlpha
			allCompMask,														// componentWriteMask
		};
		attBlendParams[i] = blend;
	}

	const vk::VkPipelineColorBlendStateCreateInfo		blendParams				=
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,		// sType
		DE_NULL,															// pNext
		(vk::VkPipelineColorBlendStateCreateFlags)0,
		VK_FALSE,															// logicOpEnable
		vk::VK_LOGIC_OP_COPY,												// logicOp
		size,																// attachmentCount
		&attBlendParams[0],													// pAttachments
		{ 0.0f, 0.0f, 0.0f, 0.0f },											// blendConstants
	};
	const vk::VkGraphicsPipelineCreateInfo				pipelineParams			=
	{
		vk::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,				// sType
		DE_NULL,															// pNext
		0u,																	// flags
		(deUint32)shaderStageParams.size(),									// stageCount
		&shaderStageParams[0],												// pStages
		&vertexInputStateParams,											// pVertexInputState
		&inputAssemblyParams,												// pInputAssemblyState
		DE_NULL,															// pTessellationState
		&viewportParams,													// pViewportState
		&rasterParams,														// pRasterState
		&multisampleParams,													// pMultisampleState
		&depthStencilParams,												// pDepthStencilState
		&blendParams,														// pColorBlendState
		(const vk::VkPipelineDynamicStateCreateInfo*)DE_NULL,				// pDynamicState
		pipelineLayout,														// layout
		renderPass,															// renderPass
		0u,																	// subpass
		DE_NULL,															// basePipelineHandle
		0u,																	// basePipelineIndex
	};

	return vk::createGraphicsPipeline(context.getDeviceInterface(), context.getDevice(), DE_NULL, &pipelineParams);
}

Move<vk::VkFramebuffer> createFramebuffer (Context& context, vk::VkRenderPass renderPass, Move<vk::VkImageView> colorAttView[4], deUint32 size, int width, int height)
{
	vk::VkImageView att[4];
	for (deUint32 i = 0; i < size; i++)
	{
		att[i] = *colorAttView[i];
	}
	const vk::VkFramebufferCreateInfo	framebufferParams	=
	{
		vk::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// sType
		DE_NULL,										// pNext
		(vk::VkFramebufferCreateFlags)0,
		renderPass,										// renderPass
		size,											// attachmentCount
		&att[0],										// pAttachments
		(deUint32)width,								// width
		(deUint32)height,								// height
		1u,												// layers
	};

	return vk::createFramebuffer(context.getDeviceInterface(), context.getDevice(), &framebufferParams);
}

Move<vk::VkCommandPool> createCommandPool (Context& context)
{
	const deUint32						queueFamilyIndex	= context.getUniversalQueueFamilyIndex();

	return vk::createCommandPool(context.getDeviceInterface(), context.getDevice(), (vk::VkCommandPoolCreateFlags)0u, queueFamilyIndex);
}

Move<vk::VkDescriptorPool> createDescriptorPool (Context& context)
{
	return vk::DescriptorPoolBuilder()
				.addType(vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u)
				.build(context.getDeviceInterface(), context.getDevice(), vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);
}

Move<vk::VkDescriptorSet> allocateDescriptorSet (Context& context, vk::VkDescriptorPool descriptorPool, vk::VkDescriptorSetLayout setLayout)
{
	const vk::VkDescriptorSetAllocateInfo	params	=
	{
		vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,
		descriptorPool,
		1u,
		&setLayout
	};

	return vk::allocateDescriptorSet(context.getDeviceInterface(), context.getDevice(), &params);
}

Move<vk::VkCommandBuffer> allocateCommandBuffer (Context& context, vk::VkCommandPool cmdPool)
{
	return vk::allocateCommandBuffer(context.getDeviceInterface(), context.getDevice(), cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

MovePtr<vk::Allocation> allocateAndBindMemory (Context& context, vk::VkBuffer buffer, vk::MemoryRequirement memReqs)
{
	const vk::DeviceInterface&		vkd		= context.getDeviceInterface();
	const vk::VkMemoryRequirements	bufReqs	= vk::getBufferMemoryRequirements(vkd, context.getDevice(), buffer);
	MovePtr<vk::Allocation>			memory	= context.getDefaultAllocator().allocate(bufReqs, memReqs);

	vkd.bindBufferMemory(context.getDevice(), buffer, memory->getMemory(), memory->getOffset());

	return memory;
}

vk::VkFormat getRenderTargetFormat (DataType dataType)
{
	switch (dataType)
	{
		case glu::TYPE_FLOAT_VEC2:
			return vk::VK_FORMAT_R8G8_UNORM;
		case glu::TYPE_FLOAT_VEC3:
			return vk::VK_FORMAT_R5G6B5_UNORM_PACK16;
		case glu::TYPE_FLOAT_VEC4:
			return vk::VK_FORMAT_R8G8B8A8_UNORM;
		case glu::TYPE_INT_VEC2:
			return vk::VK_FORMAT_R8G8_SINT;
		case glu::TYPE_INT_VEC4:
			return vk::VK_FORMAT_R8G8B8A8_SINT;
		default:
			return vk::VK_FORMAT_R8G8B8A8_UNORM;
	}
}

MovePtr<vk::Allocation> allocateAndBindMemory (Context& context, vk::VkImage image, vk::MemoryRequirement memReqs)
{
	const vk::DeviceInterface&		vkd		= context.getDeviceInterface();
	const vk::VkMemoryRequirements	imgReqs	= vk::getImageMemoryRequirements(vkd, context.getDevice(), image);
	MovePtr<vk::Allocation>			memory	= context.getDefaultAllocator().allocate(imgReqs, memReqs);

	vkd.bindImageMemory(context.getDevice(), image, memory->getMemory(), memory->getOffset());

	return memory;
}

void writeValuesToMem (Context& context, const vk::Allocation& dst, const ValueBufferLayout& layout, const vector<Value>& values, int arrayNdx)
{
	copyToLayout(dst.getHostPtr(), layout, values, arrayNdx);

	// \note Buffers are not allocated with coherency / uncached requirement so we need to manually flush CPU write caches
	flushMappedMemoryRange(context.getDeviceInterface(), context.getDevice(), dst.getMemory(), dst.getOffset(), (vk::VkDeviceSize)layout.size);
}

class ShaderCaseInstance : public TestInstance
{
public:
													ShaderCaseInstance              (Context& context, const ShaderCaseSpecification& spec);
													~ShaderCaseInstance		(void);

	TestStatus										iterate					(void);

private:
	enum
	{
		RENDER_WIDTH		= 64,
		RENDER_HEIGHT		= 64,

		POSITIONS_OFFSET	= 0,
		POSITIONS_SIZE		= (int)sizeof(Vec2)*4,

		INDICES_OFFSET		= POSITIONS_SIZE,
		INDICES_SIZE		= (int)sizeof(deUint16)*6,

		TOTAL_POS_NDX_SIZE	= POSITIONS_SIZE+INDICES_SIZE
	};

	const ShaderCaseSpecification&					m_spec;

	const Unique<vk::VkBuffer>						m_posNdxBuffer;
	const UniquePtr<vk::Allocation>					m_posNdxMem;

	const ValueBufferLayout							m_inputLayout;
	const Unique<vk::VkBuffer>						m_inputBuffer;			// Input values (attributes). Can be NULL if no inputs present
	const UniquePtr<vk::Allocation>					m_inputMem;				// Input memory, can be NULL if no input buffer exists

	const ValueBufferLayout							m_referenceLayout;
	const Unique<vk::VkBuffer>						m_referenceBuffer;		// Output (reference) values. Can be NULL if no outputs present
	const UniquePtr<vk::Allocation>					m_referenceMem;			// Output (reference) memory, can be NULL if no reference buffer exists

	const ValueBufferLayout							m_uniformLayout;
	const Unique<vk::VkBuffer>						m_uniformBuffer;		// Uniform values. Can be NULL if no uniforms present
	const UniquePtr<vk::Allocation>					m_uniformMem;			// Uniform memory, can be NULL if no uniform buffer exists

	const vk::VkFormat								m_rtFormat;
	deUint32										m_outputCount;
	Move<vk::VkImage>								m_rtImage [4];
	MovePtr<vk::Allocation>							m_rtMem[4];
	Move<vk::VkImageView>							m_rtView[4];

	Move<vk::VkBuffer>								m_readImageBuffer[4];
	MovePtr<vk::Allocation>							m_readImageMem[4];

	const Unique<vk::VkRenderPass>					m_renderPass;
	Move<vk::VkFramebuffer>							m_framebuffer;
	const PipelineProgram							m_program;
	const Unique<vk::VkDescriptorSetLayout>			m_descriptorSetLayout;
	const Unique<vk::VkPipelineLayout>				m_pipelineLayout;
	const Unique<vk::VkPipeline>					m_pipeline;

	const Unique<vk::VkDescriptorPool>				m_descriptorPool;
	const Unique<vk::VkDescriptorSet>				m_descriptorSet;

	const Unique<vk::VkCommandPool>					m_cmdPool;
	const Unique<vk::VkCommandBuffer>				m_cmdBuffer;

	int												m_subCaseNdx;
};

ShaderCaseInstance::ShaderCaseInstance (Context& context, const ShaderCaseSpecification& spec)
	: TestInstance			(context)
	, m_spec				(spec)

	, m_posNdxBuffer		(createBuffer(context, (vk::VkDeviceSize)TOTAL_POS_NDX_SIZE, vk::VK_BUFFER_USAGE_INDEX_BUFFER_BIT|vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
	, m_posNdxMem			(allocateAndBindMemory(context, *m_posNdxBuffer, vk::MemoryRequirement::HostVisible))

	, m_inputLayout			(computeStd430Layout(spec.values.inputs))
	, m_inputBuffer			(m_inputLayout.size > 0 ? createBuffer(context, (vk::VkDeviceSize)m_inputLayout.size, vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) : Move<vk::VkBuffer>())
	, m_inputMem			(m_inputLayout.size > 0 ? allocateAndBindMemory(context, *m_inputBuffer, vk::MemoryRequirement::HostVisible) : MovePtr<vk::Allocation>())

	, m_referenceLayout		(computeStd140Layout(spec.values.outputs))
	, m_referenceBuffer		(m_referenceLayout.size > 0 ? createBuffer(context, (vk::VkDeviceSize)m_referenceLayout.size, vk::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) : Move<vk::VkBuffer>())
	, m_referenceMem		(m_referenceLayout.size > 0 ? allocateAndBindMemory(context, *m_referenceBuffer, vk::MemoryRequirement::HostVisible) : MovePtr<vk::Allocation>())

	, m_uniformLayout		(computeStd140Layout(spec.values.uniforms))
	, m_uniformBuffer		(m_uniformLayout.size > 0 ? createBuffer(context, (vk::VkDeviceSize)m_uniformLayout.size, vk::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) : Move<vk::VkBuffer>())
	, m_uniformMem			(m_uniformLayout.size > 0 ? allocateAndBindMemory(context, *m_uniformBuffer, vk::MemoryRequirement::HostVisible) : MovePtr<vk::Allocation>())

	, m_rtFormat			(getRenderTargetFormat(spec.outputFormat))
	, m_outputCount			((deUint32)m_spec.values.outputs.size() == 0 ? 1 : (deUint32)m_spec.values.outputs.size())
	, m_rtImage				()
	, m_rtMem				()
	, m_rtView				()

	, m_readImageBuffer		()
	, m_readImageMem		()

	, m_renderPass			(createRenderPass(context, m_rtFormat, m_outputCount))
	, m_framebuffer			()
	, m_program				(context, spec)
	, m_descriptorSetLayout	(createDescriptorSetLayout(context, m_program.getStages()))
	, m_pipelineLayout		(createPipelineLayout(context, *m_descriptorSetLayout))
	, m_pipeline			(createPipeline(context, spec.values.inputs, m_inputLayout, m_program, *m_renderPass, *m_pipelineLayout, tcu::UVec2(RENDER_WIDTH, RENDER_HEIGHT), m_outputCount))

	, m_descriptorPool		(createDescriptorPool(context))
	, m_descriptorSet		(allocateDescriptorSet(context, *m_descriptorPool, *m_descriptorSetLayout))

	, m_cmdPool				(createCommandPool(context))
	, m_cmdBuffer			(allocateCommandBuffer(context, *m_cmdPool))

	, m_subCaseNdx			(0)
{
	{
		// Initialize the resources for each color attachment needed by the shader
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			m_rtImage[outNdx] = createImage2D(context, RENDER_WIDTH, RENDER_HEIGHT, m_rtFormat, vk::VK_IMAGE_TILING_OPTIMAL, vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|   vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			m_rtMem[outNdx] = allocateAndBindMemory(context, *m_rtImage[outNdx], vk::MemoryRequirement::Any);
			m_rtView[outNdx] = createAttachmentView(context, *m_rtImage[outNdx], m_rtFormat);

			m_readImageBuffer[outNdx] = createBuffer(context, (vk::VkDeviceSize)(RENDER_WIDTH * RENDER_HEIGHT * tcu::getPixelSize(vk::mapVkFormat(m_rtFormat))), vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			m_readImageMem[outNdx] = allocateAndBindMemory(context, *m_readImageBuffer[outNdx], vk::MemoryRequirement::HostVisible);
		}
		m_framebuffer = createFramebuffer(context, *m_renderPass, m_rtView, m_outputCount, RENDER_WIDTH, RENDER_HEIGHT);
	}

	const vk::DeviceInterface&	vkd					= context.getDeviceInterface();
	const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();

	{
		const Vec2			s_positions[]	=
		{
			Vec2(-1.0f, -1.0f),
			Vec2(-1.0f, +1.0f),
			Vec2(+1.0f, -1.0f),
			Vec2(+1.0f, +1.0f)
		};
		const deUint16		s_indices[]		=
		{
			0, 1, 2,
			1, 3, 2
		};

		DE_STATIC_ASSERT(sizeof(s_positions) == POSITIONS_SIZE);
		DE_STATIC_ASSERT(sizeof(s_indices) == INDICES_SIZE);

		deMemcpy((deUint8*)m_posNdxMem->getHostPtr() + POSITIONS_OFFSET,	&s_positions[0],	sizeof(s_positions));
		deMemcpy((deUint8*)m_posNdxMem->getHostPtr() + INDICES_OFFSET,		&s_indices[0],		sizeof(s_indices));

		flushMappedMemoryRange(m_context.getDeviceInterface(), context.getDevice(), m_posNdxMem->getMemory(), m_posNdxMem->getOffset(), sizeof(s_positions)+sizeof(s_indices));
	}

	if (!m_spec.values.uniforms.empty())
	{
		const vk::VkDescriptorBufferInfo	bufInfo	=
		{
			*m_uniformBuffer,
			(vk::VkDeviceSize)0,	// offset
			(vk::VkDeviceSize)m_uniformLayout.size
		};

		vk::DescriptorSetUpdateBuilder()
			.writeSingle(*m_descriptorSet, vk::DescriptorSetUpdateBuilder::Location::binding(USER_UNIFORM_BINDING),
						 vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufInfo)
			.update(vkd, m_context.getDevice());
	}

	if (!m_spec.values.outputs.empty())
	{
		const vk::VkDescriptorBufferInfo	bufInfo	=
		{
			*m_referenceBuffer,
			(vk::VkDeviceSize)0,	// offset
			(vk::VkDeviceSize)m_referenceLayout.size
		};

		vk::DescriptorSetUpdateBuilder()
			.writeSingle(*m_descriptorSet, vk::DescriptorSetUpdateBuilder::Location::binding(REFERENCE_UNIFORM_BINDING),
						 vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufInfo)
			.update(vkd, m_context.getDevice());
	}

	// Record command buffer

	{
		const vk::VkCommandBufferBeginInfo beginInfo	=
		{
			vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType
			DE_NULL,											// pNext
			0u,													// flags
			(const vk::VkCommandBufferInheritanceInfo*)DE_NULL,
		};

		VK_CHECK(vkd.beginCommandBuffer(*m_cmdBuffer, &beginInfo));
	}

	{
		const vk::VkMemoryBarrier		vertFlushBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_MEMORY_BARRIER,													// sType
			DE_NULL,																				// pNext
			vk::VK_ACCESS_HOST_WRITE_BIT,															// srcAccessMask
			vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|vk::VK_ACCESS_UNIFORM_READ_BIT,					// dstAccessMask
		};
		vk::VkImageMemoryBarrier	colorAttBarrier	[4];
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			vk::VkImageMemoryBarrier barrier =
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// sType
				DE_NULL,										// pNext
				0u,												// srcAccessMask
				vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		// dstAccessMask
				vk::VK_IMAGE_LAYOUT_UNDEFINED,					// oldLayout
				vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// newLayout
				queueFamilyIndex,								// srcQueueFamilyIndex
				queueFamilyIndex,								// destQueueFamilyIndex
				*m_rtImage[outNdx],								// image
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,				// aspectMask
					0u,											// baseMipLevel
					1u,											// mipLevels
					0u,											// baseArraySlice
					1u,											// arraySize
				}												// subresourceRange
			};
			colorAttBarrier[outNdx]	= barrier;
		}
		vkd.cmdPipelineBarrier(*m_cmdBuffer, vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, (vk::VkDependencyFlags)0,
							   1, &vertFlushBarrier,
							   0, (const vk::VkBufferMemoryBarrier*)DE_NULL,
							   m_outputCount, &colorAttBarrier[0]);
	}

	{
		vk::VkClearValue			clearValue[4];
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			vk::VkClearValue value = vk::makeClearValueColorF32(0.125f, 0.25f, 0.75f, 1.0f);
			clearValue[outNdx] = value;
		}

		const vk::VkRenderPassBeginInfo	passBeginInfo	=
		{
			vk::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// sType
			DE_NULL,										// pNext
			*m_renderPass,									// renderPass
			*m_framebuffer,									// framebuffer
			{ { 0, 0 }, { RENDER_WIDTH, RENDER_HEIGHT } },	// renderArea
			m_outputCount,			// clearValueCount
			&clearValue[0],									// pClearValues
		};

		vkd.cmdBeginRenderPass(*m_cmdBuffer, &passBeginInfo, vk::VK_SUBPASS_CONTENTS_INLINE);
	}

	vkd.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

	if (!m_spec.values.uniforms.empty() || !m_spec.values.outputs.empty())
		vkd.cmdBindDescriptorSets(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelineLayout, 0u, 1u, &*m_descriptorSet, 0u, DE_NULL);

	{
		const vk::VkBuffer		buffers[]	= { *m_posNdxBuffer, *m_inputBuffer };
		const vk::VkDeviceSize	offsets[]	= { POSITIONS_OFFSET, 0u };
		const deUint32			numBuffers	= buffers[1] != 0 ? 2u : 1u;
		vkd.cmdBindVertexBuffers(*m_cmdBuffer, 0u, numBuffers, buffers, offsets);
	}

	vkd.cmdBindIndexBuffer	(*m_cmdBuffer, *m_posNdxBuffer, (vk::VkDeviceSize)INDICES_OFFSET, vk::VK_INDEX_TYPE_UINT16);
	vkd.cmdDrawIndexed		(*m_cmdBuffer, 6u, 1u, 0u, 0u, 0u);
	vkd.cmdEndRenderPass	(*m_cmdBuffer);

	{
		vk::VkImageMemoryBarrier	renderFinishBarrier[4];
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			vk::VkImageMemoryBarrier	barrier =
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// sType
				DE_NULL,										// pNext
				vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		// srcAccessMask
				vk::VK_ACCESS_TRANSFER_READ_BIT,				// dstAccessMask
				vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// oldLayout
				vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// newLayout
				queueFamilyIndex,								// srcQueueFamilyIndex
				queueFamilyIndex,								// destQueueFamilyIndex
				*m_rtImage[outNdx],								// image
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,				// aspectMask
					0u,											// baseMipLevel
					1u,											// mipLevels
					0u,											// baseArraySlice
					1u,											// arraySize
				}												// subresourceRange
			};
			renderFinishBarrier[outNdx] = barrier;
        }

		vkd.cmdPipelineBarrier(*m_cmdBuffer, vk::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0,
							   0, (const vk::VkMemoryBarrier*)DE_NULL,
							   0, (const vk::VkBufferMemoryBarrier*)DE_NULL,
							   m_outputCount, &renderFinishBarrier[0]);
	}

	{
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			const vk::VkBufferImageCopy	copyParams	=
			{
				(vk::VkDeviceSize)0u,					// bufferOffset
				(deUint32)RENDER_WIDTH,					// bufferRowLength
				(deUint32)RENDER_HEIGHT,				// bufferImageHeight
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,			// aspect
					0u,										// mipLevel
					0u,										// arrayLayer
					1u,										// arraySize
				},										// imageSubresource
				{ 0u, 0u, 0u },							// imageOffset
				{ RENDER_WIDTH, RENDER_HEIGHT, 1u }		// imageExtent
			};

			vkd.cmdCopyImageToBuffer(*m_cmdBuffer, *m_rtImage[outNdx], vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_readImageBuffer[outNdx], 1u, &copyParams);
		}
	}

	{
		const vk::VkDeviceSize			size				= (vk::VkDeviceSize)(RENDER_WIDTH * RENDER_HEIGHT * tcu::getPixelSize(vk::mapVkFormat(m_rtFormat)));
		vk::VkBufferMemoryBarrier	copyFinishBarrier[4];
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			vk::VkBufferMemoryBarrier barrier =
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,		// sType
				DE_NULL,											// pNext
				vk::VK_ACCESS_TRANSFER_WRITE_BIT,					// srcAccessMask
				vk::VK_ACCESS_HOST_READ_BIT,						// dstAccessMask
				queueFamilyIndex,									// srcQueueFamilyIndex
				queueFamilyIndex,									// destQueueFamilyIndex
				*m_readImageBuffer[outNdx],									// buffer
				0u,													// offset
				size												// size
			};
			copyFinishBarrier[outNdx] = barrier;
		}
		vkd.cmdPipelineBarrier(*m_cmdBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0,
							   0, (const vk::VkMemoryBarrier*)DE_NULL,
							   m_outputCount, &copyFinishBarrier[0],
							   0, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}

	VK_CHECK(vkd.endCommandBuffer(*m_cmdBuffer));
}

ShaderCaseInstance::~ShaderCaseInstance (void)
{
}

int getNumSubCases (const ValueBlock& values)
{
	if (!values.outputs.empty())
		return int(values.outputs[0].elements.size() / values.outputs[0].type.getScalarSize());
	else
		return 1; // Always run at least one iteration even if no output values are specified
}

bool checkResultImage (const ConstPixelBufferAccess& result)
{
	const tcu::IVec4	refPix	(255, 255, 255, 255);

	for (int y = 0; y < result.getHeight(); y++)
	{
		for (int x = 0; x < result.getWidth(); x++)
		{
			const tcu::IVec4	resPix	= result.getPixelInt(x, y);

			if (boolAny(notEqual(resPix, refPix)))
				return false;
		}
	}

	return true;
}

bool checkResultImageWithReference (const ConstPixelBufferAccess& result, tcu::IVec4 refPix)
{
	for (int y = 0; y < result.getHeight(); y++)
	{
		for (int x = 0; x < result.getWidth(); x++)
		{
			const tcu::IVec4	resPix	= result.getPixelInt(x, y);

			if (boolAny(notEqual(resPix, refPix)))
				return false;
		}
	}

	return true;
}
TestStatus ShaderCaseInstance::iterate (void)
{
	const vk::DeviceInterface&	vkd		= m_context.getDeviceInterface();
	const vk::VkDevice			device	= m_context.getDevice();
	const vk::VkQueue			queue	= m_context.getUniversalQueue();

	if (!m_spec.values.inputs.empty())
		writeValuesToMem(m_context, *m_inputMem, m_inputLayout, m_spec.values.inputs, m_subCaseNdx);

	if (!m_spec.values.outputs.empty())
		writeValuesToMem(m_context, *m_referenceMem, m_referenceLayout, m_spec.values.outputs, m_subCaseNdx);

	if (!m_spec.values.uniforms.empty())
		writeValuesToMem(m_context, *m_uniformMem, m_uniformLayout, m_spec.values.uniforms, m_subCaseNdx);

	{
		const vk::VkSubmitInfo		submitInfo	=
		{
			vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,
			0u,											// waitSemaphoreCount
			(const vk::VkSemaphore*)0,					// pWaitSemaphores
			(const vk::VkPipelineStageFlags*)DE_NULL,
			1u,
			&m_cmdBuffer.get(),
			0u,											// signalSemaphoreCount
			(const vk::VkSemaphore*)0,					// pSignalSemaphores
		};
		const Unique<vk::VkFence>	fence		(vk::createFence(vkd, device));

		VK_CHECK(vkd.queueSubmit	(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vkd.waitForFences	(device, 1u, &fence.get(), DE_TRUE, ~0ull));
	}

	// Result was checked in fragment shader
	if (m_spec.outputType == glu::sl::OUTPUT_RESULT)
	{
		const ConstPixelBufferAccess	imgAccess	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), RENDER_WIDTH, RENDER_HEIGHT, 1, m_readImageMem[0]->getHostPtr());

		invalidateMappedMemoryRange(vkd, device, m_readImageMem[0]->getMemory(), m_readImageMem[0]->getOffset(), (vk::VkDeviceSize)(RENDER_WIDTH*RENDER_HEIGHT*4));

		if (!checkResultImage(imgAccess))
		{
			TestLog&	log		= m_context.getTestContext().getLog();

			log << TestLog::Message << "ERROR: Got non-white pixels on sub-case " << m_subCaseNdx << TestLog::EndMessage
				<< TestLog::Image("Result", "Result", imgAccess);

			dumpValues(log, m_spec.values, m_subCaseNdx);

			return TestStatus::fail(string("Got invalid pixels at sub-case ") + de::toString(m_subCaseNdx));
		}
	}
	// Result was written to color buffer
	else
	{
		for (deUint32 outNdx = 0; outNdx < m_outputCount; outNdx++)
		{
			const ConstPixelBufferAccess	imgAccess		(vk::mapVkFormat(m_rtFormat), RENDER_WIDTH, RENDER_HEIGHT, 1, m_readImageMem[outNdx]->getHostPtr());
			const DataType					dataType		= m_spec.values.outputs[outNdx].type.getBasicType();
			const int						numComponents	= getDataTypeScalarSize(dataType);
			tcu::IVec4						reference		(0, 0, 0, 1);

			for (int refNdx = 0; refNdx < numComponents; refNdx++)
			{
				if (isDataTypeFloatOrVec(dataType))
					reference[refNdx] = (int)m_spec.values.outputs[outNdx].elements[m_subCaseNdx * numComponents + refNdx].float32;
				else if (isDataTypeIntOrIVec(dataType))
					reference[refNdx] = m_spec.values.outputs[outNdx].elements[m_subCaseNdx * numComponents + refNdx].int32;
				else
					DE_FATAL("Unknown data type");
			}

			invalidateMappedMemoryRange(vkd, device, m_readImageMem[outNdx]->getMemory(), m_readImageMem[outNdx]->getOffset(), (vk::VkDeviceSize)(RENDER_WIDTH * RENDER_HEIGHT * tcu::getPixelSize(vk::mapVkFormat(m_rtFormat))));

			if (!checkResultImageWithReference(imgAccess, reference))
			{
				TestLog&	log		= m_context.getTestContext().getLog();

				log << TestLog::Message << "ERROR: Got nonmatching pixels on sub-case " << m_subCaseNdx << " output " << outNdx << TestLog::EndMessage
					<< TestLog::Image("Result", "Result", imgAccess);

				dumpValues(log, m_spec.values, m_subCaseNdx);

				return TestStatus::fail(string("Got invalid pixels at sub-case ") + de::toString(m_subCaseNdx));
			}
		}
	}

	if (++m_subCaseNdx < getNumSubCases(m_spec.values))
		return TestStatus::incomplete();
	else
		return TestStatus::pass("All sub-cases passed");
}

class ShaderCase : public TestCase
{
public:
									ShaderCase		(tcu::TestContext& testCtx, const string& name, const string& description, const ShaderCaseSpecification& spec);


	void							initPrograms	(SourceCollections& programCollection) const;
	TestInstance*					createInstance	(Context& context) const;

private:
	const ShaderCaseSpecification	m_spec;
};

ShaderCase::ShaderCase (tcu::TestContext& testCtx, const string& name, const string& description, const ShaderCaseSpecification& spec)
	: TestCase	(testCtx, name, description)
	, m_spec	(spec)
{
}

void ShaderCase::initPrograms (SourceCollections& sourceCollection) const
{
	vector<ProgramSources>	specializedSources	(m_spec.programs.size());

	DE_ASSERT(isValid(m_spec));

	if (m_spec.expectResult != glu::sl::EXPECT_PASS)
		TCU_THROW(InternalError, "Only EXPECT_PASS is supported");

	if (m_spec.caseType == glu::sl::CASETYPE_VERTEX_ONLY)
	{
		DE_ASSERT(m_spec.programs.size() == 1 && m_spec.programs[0].sources.sources[glu::SHADERTYPE_VERTEX].size() == 1);
		specializedSources[0] << glu::VertexSource(specializeVertexShader(m_spec, m_spec.programs[0].sources.sources[glu::SHADERTYPE_VERTEX][0]))
							  << glu::FragmentSource(genFragmentShader(m_spec));
	}
	else if (m_spec.caseType == glu::sl::CASETYPE_FRAGMENT_ONLY)
	{
		DE_ASSERT(m_spec.programs.size() == 1 && m_spec.programs[0].sources.sources[glu::SHADERTYPE_FRAGMENT].size() == 1);
		specializedSources[0] << glu::VertexSource(genVertexShader(m_spec))
							  << glu::FragmentSource(specializeFragmentShader(m_spec, m_spec.programs[0].sources.sources[glu::SHADERTYPE_FRAGMENT][0]));
	}
	else
	{
		DE_ASSERT(m_spec.caseType == glu::sl::CASETYPE_COMPLETE);

		const int	maxPatchVertices	= 4; // \todo [2015-08-05 pyry] Query

		for (size_t progNdx = 0; progNdx < m_spec.programs.size(); progNdx++)
		{
			const ProgramSpecializationParams	progSpecParams	(m_spec, m_spec.programs[progNdx].requiredExtensions, maxPatchVertices);

			specializeProgramSources(specializedSources[progNdx], m_spec.programs[progNdx].sources, progSpecParams);
		}
	}

	for (size_t progNdx = 0; progNdx < specializedSources.size(); progNdx++)
	{
		for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
		{
			if (!specializedSources[progNdx].sources[shaderType].empty())
			{
				vk::GlslSource& curSrc	= sourceCollection.glslSources.add(getShaderName((glu::ShaderType)shaderType, progNdx));
				curSrc.sources[shaderType] = specializedSources[progNdx].sources[shaderType];
			}
		}
	}
}

TestInstance* ShaderCase::createInstance (Context& context) const
{
	return new ShaderCaseInstance(context, m_spec);
}

class ShaderCaseFactory : public glu::sl::ShaderCaseFactory
{
public:
	ShaderCaseFactory (tcu::TestContext& testCtx)
		: m_testCtx(testCtx)
	{
	}

	tcu::TestCaseGroup* createGroup (const string& name, const string& description, const vector<tcu::TestNode*>& children)
	{
		return new tcu::TestCaseGroup(m_testCtx, name.c_str(), description.c_str(), children);
	}

	tcu::TestCase* createCase (const string& name, const string& description, const ShaderCaseSpecification& spec)
	{
		return new ShaderCase(m_testCtx, name, description, spec);
	}

private:
	tcu::TestContext&	m_testCtx;
};

class ShaderLibraryGroup : public tcu::TestCaseGroup
{
public:
	ShaderLibraryGroup (tcu::TestContext& testCtx, const string& name, const string& description, const string& filename)
		 : tcu::TestCaseGroup	(testCtx, name.c_str(), description.c_str())
		 , m_filename			(filename)
	{
	}

	void init (void)
	{
		ShaderCaseFactory				caseFactory	(m_testCtx);
		const vector<tcu::TestNode*>	children	= glu::sl::parseFile(m_testCtx.getArchive(), m_filename, &caseFactory);

		for (size_t ndx = 0; ndx < children.size(); ndx++)
		{
			try
			{
				addChild(children[ndx]);
			}
			catch (...)
			{
				for (; ndx < children.size(); ndx++)
					delete children[ndx];
				throw;
			}
		}
	}

private:
	const string	m_filename;
};

} // anonymous

MovePtr<tcu::TestCaseGroup> createShaderLibraryGroup (tcu::TestContext& testCtx, const string& name, const string& description, const string& filename)
{
	return MovePtr<tcu::TestCaseGroup>(new ShaderLibraryGroup(testCtx, name, description, filename));
}

} // vkt
