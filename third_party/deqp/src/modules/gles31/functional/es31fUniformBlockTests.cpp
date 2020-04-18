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
 * \brief Uniform block tests.
 *//*--------------------------------------------------------------------*/

#include "es31fUniformBlockTests.hpp"
#include "glsUniformBlockCase.hpp"
#include "glsRandomUniformBlockCase.hpp"
#include "tcuCommandLine.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"

using std::string;
using std::vector;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using gls::UniformBlockCase;
using gls::RandomUniformBlockCase;
using namespace gls::ub;

void createRandomCaseGroup (tcu::TestCaseGroup* parentGroup, Context& context, const char* groupName, const char* description, UniformBlockCase::BufferMode bufferMode, deUint32 features, int numCases, deUint32 baseSeed)
{
	tcu::TestCaseGroup* group = new tcu::TestCaseGroup(context.getTestContext(), groupName, description);
	parentGroup->addChild(group);

	baseSeed += (deUint32)context.getTestContext().getCommandLine().getBaseSeed();

	for (int ndx = 0; ndx < numCases; ndx++)
		group->addChild(new RandomUniformBlockCase(context.getTestContext(), context.getRenderContext(), glu::GLSL_VERSION_310_ES,
												   de::toString(ndx).c_str(), "", bufferMode, features, (deUint32)ndx+baseSeed));
}

class BlockBasicTypeCase : public UniformBlockCase
{
public:
	BlockBasicTypeCase (Context& context, const char* name, const char* description, const VarType& type, deUint32 layoutFlags, int numInstances)
		: UniformBlockCase(context.getTestContext(), context.getRenderContext(), name, description, glu::GLSL_VERSION_310_ES, BUFFERMODE_PER_BLOCK)
	{
		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("var", type, 0));
		block.setFlags(layoutFlags);

		if (numInstances > 0)
		{
			block.setArraySize(numInstances);
			block.setInstanceName("block");
		}
	}
};

static void createBlockBasicTypeCases (tcu::TestCaseGroup* group, Context& context, const char* name, const VarType& type, deUint32 layoutFlags, int numInstances = 0)
{
	group->addChild(new BlockBasicTypeCase(context, (string(name) + "_vertex").c_str(),		"", type, layoutFlags|DECLARE_VERTEX,					numInstances));
	group->addChild(new BlockBasicTypeCase(context, (string(name) + "_fragment").c_str(),	"", type, layoutFlags|DECLARE_FRAGMENT,					numInstances));

	if (!(layoutFlags & LAYOUT_PACKED))
		group->addChild(new BlockBasicTypeCase(context, (string(name) + "_both").c_str(),	"", type, layoutFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	numInstances));
}

class Block2LevelStructArrayCase : public UniformBlockCase
{
public:
	Block2LevelStructArrayCase (Context& context, const char* name, const char* description, deUint32 layoutFlags, BufferMode bufferMode, int numInstances)
		: UniformBlockCase	(context.getTestContext(), context.getRenderContext(), name, description, glu::GLSL_VERSION_310_ES, bufferMode)
		, m_layoutFlags		(layoutFlags)
		, m_numInstances	(numInstances)
	{
	}

	void init (void)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_UINT_VEC3, PRECISION_HIGH), UNUSED_BOTH);
		typeS.addMember("b", VarType(VarType(glu::TYPE_FLOAT_MAT2, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_UINT, PRECISION_LOW));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("u", VarType(glu::TYPE_INT, PRECISION_MEDIUM)));
		block.addUniform(Uniform("s", VarType(VarType(VarType(&typeS), 3), 2)));
		block.addUniform(Uniform("v", VarType(glu::TYPE_FLOAT_VEC2, PRECISION_MEDIUM)));
		block.setFlags(m_layoutFlags);

		if (m_numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(m_numInstances);
		}
	}

private:
	deUint32	m_layoutFlags;
	int			m_numInstances;
};

} // anonymous

UniformBlockTests::UniformBlockTests (Context& context)
	: TestCaseGroup(context, "ubo", "Uniform Block tests")
{
}

UniformBlockTests::~UniformBlockTests (void)
{
}

void UniformBlockTests::init (void)
{
	static const glu::DataType basicTypes[] =
	{
		glu::TYPE_FLOAT,
		glu::TYPE_FLOAT_VEC2,
		glu::TYPE_FLOAT_VEC3,
		glu::TYPE_FLOAT_VEC4,
		glu::TYPE_INT,
		glu::TYPE_INT_VEC2,
		glu::TYPE_INT_VEC3,
		glu::TYPE_INT_VEC4,
		glu::TYPE_UINT,
		glu::TYPE_UINT_VEC2,
		glu::TYPE_UINT_VEC3,
		glu::TYPE_UINT_VEC4,
		glu::TYPE_BOOL,
		glu::TYPE_BOOL_VEC2,
		glu::TYPE_BOOL_VEC3,
		glu::TYPE_BOOL_VEC4,
		glu::TYPE_FLOAT_MAT2,
		glu::TYPE_FLOAT_MAT3,
		glu::TYPE_FLOAT_MAT4,
		glu::TYPE_FLOAT_MAT2X3,
		glu::TYPE_FLOAT_MAT2X4,
		glu::TYPE_FLOAT_MAT3X2,
		glu::TYPE_FLOAT_MAT3X4,
		glu::TYPE_FLOAT_MAT4X2,
		glu::TYPE_FLOAT_MAT4X3
	};

	static const struct
	{
		const char*		name;
		deUint32		flags;
	} layoutFlags[] =
	{
		{ "shared",		LAYOUT_SHARED	},
		{ "packed",		LAYOUT_PACKED	},
		{ "std140",		LAYOUT_STD140	}
	};

	static const struct
	{
		const char*		name;
		deUint32		flags;
	} matrixFlags[] =
	{
		{ "row_major",		LAYOUT_ROW_MAJOR	},
		{ "column_major",	LAYOUT_COLUMN_MAJOR }
	};

	static const struct
	{
		const char*							name;
		UniformBlockCase::BufferMode		mode;
	} bufferModes[] =
	{
		{ "per_block_buffer",	UniformBlockCase::BUFFERMODE_PER_BLOCK },
		{ "single_buffer",		UniformBlockCase::BUFFERMODE_SINGLE	}
	};

	// ubo.2_level_array
	{
		tcu::TestCaseGroup* nestedArrayGroup = new tcu::TestCaseGroup(m_testCtx, "2_level_array", "2-level basic array variable in single buffer");
		addChild(nestedArrayGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			tcu::TestCaseGroup* layoutGroup = new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, "");
			nestedArrayGroup->addChild(layoutGroup);

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				const glu::DataType	type		= basicTypes[basicTypeNdx];
				const char*			typeName	= glu::getDataTypeName(type);
				const int			childSize	= 4;
				const int			parentSize	= 3;
				const VarType		childType	(VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), childSize);
				const VarType		parentType	(childType, parentSize);

				createBlockBasicTypeCases(layoutGroup, m_context, typeName, parentType, layoutFlags[layoutFlagNdx].flags);

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						createBlockBasicTypeCases(layoutGroup, m_context, (string(matrixFlags[matFlagNdx].name) + "_" + typeName).c_str(),
												  parentType, layoutFlags[layoutFlagNdx].flags|matrixFlags[matFlagNdx].flags);
				}
			}
		}
	}

	// ubo.3_level_array
	{
		tcu::TestCaseGroup* nestedArrayGroup = new tcu::TestCaseGroup(m_testCtx, "3_level_array", "3-level basic array variable in single buffer");
		addChild(nestedArrayGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			tcu::TestCaseGroup* layoutGroup = new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, "");
			nestedArrayGroup->addChild(layoutGroup);

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				const glu::DataType	type		= basicTypes[basicTypeNdx];
				const char*			typeName	= glu::getDataTypeName(type);
				const int			childSize0	= 2;
				const int			childSize1	= 4;
				const int			parentSize	= 3;
				const VarType		childType0	(VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), childSize0);
				const VarType		childType1	(childType0, childSize1);
				const VarType		parentType	(childType1, parentSize);

				createBlockBasicTypeCases(layoutGroup, m_context, typeName, parentType, layoutFlags[layoutFlagNdx].flags);

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						createBlockBasicTypeCases(layoutGroup, m_context, (string(matrixFlags[matFlagNdx].name) + "_" + typeName).c_str(),
												  parentType, layoutFlags[layoutFlagNdx].flags|matrixFlags[matFlagNdx].flags);
				}
			}
		}
	}

	// ubo.2_level_struct_array
	{
		tcu::TestCaseGroup* structArrayArrayGroup = new tcu::TestCaseGroup(m_testCtx, "2_level_struct_array", "Struct array in one uniform block");
		addChild(structArrayArrayGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			structArrayArrayGroup->addChild(modeGroup);

			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string	baseName	= layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags	= layoutFlags[layoutFlagNdx].flags;

					if (bufferModes[modeNdx].mode == UniformBlockCase::BUFFERMODE_SINGLE && isArray == 0)
						continue; // Doesn't make sense to add this variant.

					if (isArray)
						baseName += "_instance_array";

					modeGroup->addChild(new Block2LevelStructArrayCase(m_context, (baseName + "_vertex").c_str(),	"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0));
					modeGroup->addChild(new Block2LevelStructArrayCase(m_context, (baseName + "_fragment").c_str(),	"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0));

					if (!(baseFlags & LAYOUT_PACKED))
						modeGroup->addChild(new Block2LevelStructArrayCase(m_context, (baseName + "_both").c_str(),	"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.random
	{
		const deUint32	allShaders		= FEATURE_VERTEX_BLOCKS|FEATURE_FRAGMENT_BLOCKS|FEATURE_SHARED_BLOCKS;
		const deUint32	allLayouts		= FEATURE_PACKED_LAYOUT|FEATURE_SHARED_LAYOUT|FEATURE_STD140_LAYOUT;
		const deUint32	allBasicTypes	= FEATURE_VECTORS|FEATURE_MATRICES;
		const deUint32	unused			= FEATURE_UNUSED_MEMBERS|FEATURE_UNUSED_UNIFORMS;
		const deUint32	matFlags		= FEATURE_MATRIX_LAYOUT;
		const deUint32	basicTypeArrays	= allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_ARRAYS|FEATURE_ARRAYS_OF_ARRAYS;
		const deUint32	allFeatures		= ~0u;

		tcu::TestCaseGroup* randomGroup = new tcu::TestCaseGroup(m_testCtx, "random", "Random Uniform Block cases");
		addChild(randomGroup);

		createRandomCaseGroup(randomGroup, m_context, "basic_type_arrays",		"Arrays, per-block buffers",				UniformBlockCase::BUFFERMODE_PER_BLOCK,	basicTypeArrays,	25, 1150);
		createRandomCaseGroup(randomGroup, m_context, "all_per_block_buffers",	"All random features, per-block buffers",	UniformBlockCase::BUFFERMODE_PER_BLOCK,	allFeatures,		50, 11200);
		createRandomCaseGroup(randomGroup, m_context, "all_shared_buffer",		"All random features, shared buffer",		UniformBlockCase::BUFFERMODE_SINGLE,	allFeatures,		50, 11250);
	}
}

} // Functional
} // gles31
} // deqp
