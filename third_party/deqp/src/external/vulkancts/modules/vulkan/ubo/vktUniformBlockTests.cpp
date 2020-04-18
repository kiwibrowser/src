/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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

#include "vktUniformBlockTests.hpp"

#include "vktUniformBlockCase.hpp"
#include "vktRandomUniformBlockCase.hpp"

#include "tcuCommandLine.hpp"
#include "deStringUtil.hpp"

namespace vkt
{
namespace ubo
{

namespace
{

class BlockBasicTypeCase : public UniformBlockCase
{
public:
	BlockBasicTypeCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const VarType& type, deUint32 layoutFlags, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase(testCtx, name, description, BUFFERMODE_PER_BLOCK, matrixLoadFlag)
	{
		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("var", type, 0));
		block.setFlags(layoutFlags);

		if (numInstances > 0)
		{
			block.setArraySize(numInstances);
			block.setInstanceName("block");
		}

		init();
	}
};

void createBlockBasicTypeCases (tcu::TestCaseGroup& group, tcu::TestContext& testCtx, const std::string& name, const VarType& type, deUint32 layoutFlags, int numInstances = 0)
{
	de::MovePtr<tcu::TestCaseGroup>	typeGroup(new tcu::TestCaseGroup(group.getTestContext(), name.c_str(), ""));

	typeGroup->addChild(new BlockBasicTypeCase(testCtx, "vertex",				"", type, layoutFlags|DECLARE_VERTEX,					numInstances, LOAD_FULL_MATRIX));
	typeGroup->addChild(new BlockBasicTypeCase(testCtx, "fragment",				"", type, layoutFlags|DECLARE_FRAGMENT,					numInstances, LOAD_FULL_MATRIX));
	typeGroup->addChild(new BlockBasicTypeCase(testCtx, "both",					"",	type, layoutFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	numInstances, LOAD_FULL_MATRIX));
	typeGroup->addChild(new BlockBasicTypeCase(testCtx, "vertex_comp_access",	"", type, layoutFlags|DECLARE_VERTEX,					numInstances, LOAD_MATRIX_COMPONENTS));
	typeGroup->addChild(new BlockBasicTypeCase(testCtx, "fragment_comp_access",	"", type, layoutFlags|DECLARE_FRAGMENT,					numInstances, LOAD_MATRIX_COMPONENTS));
	typeGroup->addChild(new BlockBasicTypeCase(testCtx, "both_comp_access",		"",	type, layoutFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	numInstances, LOAD_MATRIX_COMPONENTS));

	group.addChild(typeGroup.release());
}

class BlockSingleStructCase : public UniformBlockCase
{
public:
	BlockSingleStructCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 layoutFlags, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_INT_VEC3, PRECISION_HIGH), UNUSED_BOTH); // First member is unused.
		typeS.addMember("b", VarType(VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("s", VarType(&typeS), 0));
		block.setFlags(layoutFlags);

		if (numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(numInstances);
		}

		init();
	}
};

class BlockSingleStructArrayCase : public UniformBlockCase
{
public:
	BlockSingleStructArrayCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 layoutFlags, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_INT_VEC3, PRECISION_HIGH), UNUSED_BOTH);
		typeS.addMember("b", VarType(VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("u", VarType(glu::TYPE_UINT, PRECISION_LOW)));
		block.addUniform(Uniform("s", VarType(VarType(&typeS), 3)));
		block.addUniform(Uniform("v", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_MEDIUM)));
		block.setFlags(layoutFlags);

		if (numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(numInstances);
		}

		init();
	}
};

class BlockSingleNestedStructCase : public UniformBlockCase
{
public:
	BlockSingleNestedStructCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 layoutFlags, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_INT_VEC3, PRECISION_HIGH));
		typeS.addMember("b", VarType(VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH), UNUSED_BOTH);

		StructType& typeT = m_interface.allocStruct("T");
		typeT.addMember("a", VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM));
		typeT.addMember("b", VarType(&typeS));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("s", VarType(&typeS), 0));
		block.addUniform(Uniform("v", VarType(glu::TYPE_FLOAT_VEC2, PRECISION_LOW), UNUSED_BOTH));
		block.addUniform(Uniform("t", VarType(&typeT), 0));
		block.addUniform(Uniform("u", VarType(glu::TYPE_UINT, PRECISION_HIGH), 0));
		block.setFlags(layoutFlags);

		if (numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(numInstances);
		}

		init();
	}
};

class BlockSingleNestedStructArrayCase : public UniformBlockCase
{
public:
	BlockSingleNestedStructArrayCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 layoutFlags, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_INT_VEC3, PRECISION_HIGH));
		typeS.addMember("b", VarType(VarType(glu::TYPE_INT_VEC2, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH), UNUSED_BOTH);

		StructType& typeT = m_interface.allocStruct("T");
		typeT.addMember("a", VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM));
		typeT.addMember("b", VarType(VarType(&typeS), 3));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("s", VarType(&typeS), 0));
		block.addUniform(Uniform("v", VarType(glu::TYPE_FLOAT_VEC2, PRECISION_LOW), UNUSED_BOTH));
		block.addUniform(Uniform("t", VarType(VarType(&typeT), 2), 0));
		block.addUniform(Uniform("u", VarType(glu::TYPE_UINT, PRECISION_HIGH), 0));
		block.setFlags(layoutFlags);

		if (numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(numInstances);
		}

		init();
	}
};

class BlockMultiBasicTypesCase : public UniformBlockCase
{
public:
	BlockMultiBasicTypesCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 flagsA, deUint32 flagsB, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
	{
		UniformBlock& blockA = m_interface.allocBlock("BlockA");
		blockA.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT, PRECISION_HIGH)));
		blockA.addUniform(Uniform("b", VarType(glu::TYPE_UINT_VEC3, PRECISION_LOW), UNUSED_BOTH));
		blockA.addUniform(Uniform("c", VarType(glu::TYPE_FLOAT_MAT2, PRECISION_MEDIUM)));
		blockA.setInstanceName("blockA");
		blockA.setFlags(flagsA);

		UniformBlock& blockB = m_interface.allocBlock("BlockB");
		blockB.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM)));
		blockB.addUniform(Uniform("b", VarType(glu::TYPE_INT_VEC2, PRECISION_LOW)));
		blockB.addUniform(Uniform("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH), UNUSED_BOTH));
		blockB.addUniform(Uniform("d", VarType(glu::TYPE_BOOL, 0)));
		blockB.setInstanceName("blockB");
		blockB.setFlags(flagsB);

		if (numInstances > 0)
		{
			blockA.setArraySize(numInstances);
			blockB.setArraySize(numInstances);
		}

		init();
	}
};

class BlockMultiNestedStructCase : public UniformBlockCase
{
public:
	BlockMultiNestedStructCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 flagsA, deUint32 flagsB, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_FLOAT_MAT3, PRECISION_LOW));
		typeS.addMember("b", VarType(VarType(glu::TYPE_INT_VEC2, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH));

		StructType& typeT = m_interface.allocStruct("T");
		typeT.addMember("a", VarType(glu::TYPE_UINT, PRECISION_MEDIUM), UNUSED_BOTH);
		typeT.addMember("b", VarType(&typeS));
		typeT.addMember("c", VarType(glu::TYPE_BOOL_VEC4, 0));

		UniformBlock& blockA = m_interface.allocBlock("BlockA");
		blockA.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT, PRECISION_HIGH)));
		blockA.addUniform(Uniform("b", VarType(&typeS)));
		blockA.addUniform(Uniform("c", VarType(glu::TYPE_UINT_VEC3, PRECISION_LOW), UNUSED_BOTH));
		blockA.setInstanceName("blockA");
		blockA.setFlags(flagsA);

		UniformBlock& blockB = m_interface.allocBlock("BlockB");
		blockB.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT_MAT2, PRECISION_MEDIUM)));
		blockB.addUniform(Uniform("b", VarType(&typeT)));
		blockB.addUniform(Uniform("c", VarType(glu::TYPE_BOOL_VEC4, 0), UNUSED_BOTH));
		blockB.addUniform(Uniform("d", VarType(glu::TYPE_BOOL, 0)));
		blockB.setInstanceName("blockB");
		blockB.setFlags(flagsB);

		if (numInstances > 0)
		{
			blockA.setArraySize(numInstances);
			blockB.setArraySize(numInstances);
		}

		init();
	}
};

class Block2LevelStructArrayCase : public UniformBlockCase
{
public:
	Block2LevelStructArrayCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, deUint32 layoutFlags, BufferMode bufferMode, int numInstances, MatrixLoadFlags matrixLoadFlag)
		: UniformBlockCase	(testCtx, name, description, bufferMode, matrixLoadFlag)
		, m_layoutFlags		(layoutFlags)
		, m_numInstances	(numInstances)
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

		init();
	}

private:
	deUint32	m_layoutFlags;
	int			m_numInstances;
};

class LinkByBindingCase : public UniformBlockCase
{
public:
	LinkByBindingCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, BufferMode bufferMode, int numInstances)
		: UniformBlockCase	(testCtx, name, description, bufferMode, LOAD_FULL_MATRIX)
	{
		UniformBlock& blockA = m_interface.allocBlock("TestBlock");
		blockA.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT, PRECISION_HIGH)));
		blockA.addUniform(Uniform("b", VarType(glu::TYPE_UINT_VEC3, PRECISION_LOW), UNUSED_BOTH));
		blockA.addUniform(Uniform("c", VarType(glu::TYPE_FLOAT_MAT2, PRECISION_MEDIUM)));
		blockA.setFlags(LAYOUT_STD140|DECLARE_VERTEX);

		UniformBlock& blockB = m_interface.allocBlock("TestBlock");
		blockB.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM)));
		blockB.addUniform(Uniform("b", VarType(glu::TYPE_INT_VEC2, PRECISION_LOW)));
		blockB.addUniform(Uniform("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH), UNUSED_BOTH));
		blockB.addUniform(Uniform("d", VarType(glu::TYPE_BOOL, 0)));
		blockB.setFlags(LAYOUT_STD140|DECLARE_FRAGMENT);

		if (numInstances > 0)
		{
			blockA.setInstanceName("testBlock");
			blockA.setArraySize(numInstances);
			blockB.setInstanceName("testBlock");
			blockB.setArraySize(numInstances);
		}

		init();
	}
};

void createRandomCaseGroup (tcu::TestCaseGroup* parentGroup, tcu::TestContext& testCtx, const char* groupName, const char* description, UniformBlockCase::BufferMode bufferMode, deUint32 features, int numCases, deUint32 baseSeed)
{
	tcu::TestCaseGroup* group = new tcu::TestCaseGroup(testCtx, groupName, description);
	parentGroup->addChild(group);

	baseSeed += (deUint32)testCtx.getCommandLine().getBaseSeed();

	for (int ndx = 0; ndx < numCases; ndx++)
		group->addChild(new RandomUniformBlockCase(testCtx, de::toString(ndx), "", bufferMode, features, (deUint32)ndx + baseSeed));
}

// UniformBlockTests

class UniformBlockTests : public tcu::TestCaseGroup
{
public:
							UniformBlockTests		(tcu::TestContext& testCtx);
							~UniformBlockTests		(void);

	void					init					(void);

private:
							UniformBlockTests		(const UniformBlockTests& other);
	UniformBlockTests&		operator=				(const UniformBlockTests& other);
};

UniformBlockTests::UniformBlockTests (tcu::TestContext& testCtx)
	: TestCaseGroup(testCtx, "ubo", "Uniform Block tests")
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
		const std::string	name;
		deUint32			flags;
	} precisionFlags[] =
	{
		// TODO remove PRECISION_LOW because both PRECISION_LOW and PRECISION_MEDIUM means relaxed precision?
		{ "lowp",		PRECISION_LOW		},
		{ "mediump",	PRECISION_MEDIUM	},
		{ "highp",		PRECISION_HIGH		}
	};

	static const struct
	{
		const char*			name;
		deUint32			flags;
	} layoutFlags[] =
	{
		{ "std140",		LAYOUT_STD140	}
	};

	static const struct
	{
		const std::string	name;
		deUint32			flags;
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
			de::MovePtr<tcu::TestCaseGroup> layoutGroup(new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, ""));

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				const glu::DataType	type		= basicTypes[basicTypeNdx];
				const char* const	typeName	= glu::getDataTypeName(type);
				const int			childSize	= 4;
				const int			parentSize	= 3;
				const VarType		childType	(VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), childSize);
				const VarType		parentType	(childType, parentSize);

				createBlockBasicTypeCases(*layoutGroup, m_testCtx, typeName, parentType, layoutFlags[layoutFlagNdx].flags);

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						createBlockBasicTypeCases(*layoutGroup, m_testCtx, (std::string(matrixFlags[matFlagNdx].name) + "_" + typeName),
												  parentType, layoutFlags[layoutFlagNdx].flags|matrixFlags[matFlagNdx].flags);
				}
			}
			nestedArrayGroup->addChild(layoutGroup.release());
		}
	}

	// ubo.3_level_array
	{
		tcu::TestCaseGroup* nestedArrayGroup = new tcu::TestCaseGroup(m_testCtx, "3_level_array", "3-level basic array variable in single buffer");
		addChild(nestedArrayGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> layoutGroup(new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, ""));

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				const glu::DataType	type		= basicTypes[basicTypeNdx];
				const char* const	typeName	= glu::getDataTypeName(type);
				const int			childSize0	= 2;
				const int			childSize1	= 4;
				const int			parentSize	= 3;
				const VarType		childType0	(VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), childSize0);
				const VarType		childType1	(childType0, childSize1);
				const VarType		parentType	(childType1, parentSize);

				createBlockBasicTypeCases(*layoutGroup, m_testCtx, typeName, parentType, layoutFlags[layoutFlagNdx].flags);

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						createBlockBasicTypeCases(*layoutGroup, m_testCtx, (std::string(matrixFlags[matFlagNdx].name) + "_" + typeName),
												  parentType, layoutFlags[layoutFlagNdx].flags|matrixFlags[matFlagNdx].flags);
				}
			}
			nestedArrayGroup->addChild(layoutGroup.release());
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

					modeGroup->addChild(new Block2LevelStructArrayCase(m_testCtx, (baseName + "_vertex"),				"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new Block2LevelStructArrayCase(m_testCtx, (baseName + "_fragment"),				"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new Block2LevelStructArrayCase(m_testCtx, (baseName + "_both"),					"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new Block2LevelStructArrayCase(m_testCtx, (baseName + "_vertex_comp_access"),	"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new Block2LevelStructArrayCase(m_testCtx, (baseName + "_fragment_comp_access"),	"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new Block2LevelStructArrayCase(m_testCtx, (baseName + "_both_comp_access"),		"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// ubo.single_basic_type
	{
		tcu::TestCaseGroup* singleBasicTypeGroup = new tcu::TestCaseGroup(m_testCtx, "single_basic_type", "Single basic variable in single buffer");
		addChild(singleBasicTypeGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> layoutGroup(new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, ""));

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				glu::DataType		type = basicTypes[basicTypeNdx];
				const char* const	typeName = glu::getDataTypeName(type);

				if (glu::isDataTypeBoolOrBVec(type))
					createBlockBasicTypeCases(*layoutGroup, m_testCtx, typeName, VarType(type, 0), layoutFlags[layoutFlagNdx].flags);
			}

			for (int precNdx = 0; precNdx < DE_LENGTH_OF_ARRAY(precisionFlags); precNdx++)
			{
				de::MovePtr<tcu::TestCaseGroup>	precGroup(new tcu::TestCaseGroup(m_testCtx, precisionFlags[precNdx].name.c_str(), ""));

				for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
				{
					glu::DataType		type		= basicTypes[basicTypeNdx];
					const char* const	typeName	= glu::getDataTypeName(type);

					if (!glu::isDataTypeBoolOrBVec(type))
						createBlockBasicTypeCases(*precGroup, m_testCtx, typeName,
												  VarType(type, precisionFlags[precNdx].flags), layoutFlags[layoutFlagNdx].flags);

					if (glu::isDataTypeMatrix(type))
					{
						for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						{
							createBlockBasicTypeCases(*precGroup, m_testCtx, matrixFlags[matFlagNdx].name + "_" + typeName,
													  VarType(type, precisionFlags[precNdx].flags), layoutFlags[layoutFlagNdx].flags | matrixFlags[matFlagNdx].flags);

						}
					}
				}
				layoutGroup->addChild(precGroup.release());
			}
			singleBasicTypeGroup->addChild(layoutGroup.release());
		}
	}

	// ubo.single_basic_array
	{
		tcu::TestCaseGroup* singleBasicArrayGroup = new tcu::TestCaseGroup(m_testCtx, "single_basic_array", "Single basic array variable in single buffer");
		addChild(singleBasicArrayGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> layoutGroup(new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, ""));

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				glu::DataType		type		= basicTypes[basicTypeNdx];
				const char* const	typeName	= glu::getDataTypeName(type);
				const int			arraySize	= 3;

				createBlockBasicTypeCases(*layoutGroup, m_testCtx, typeName,
										  VarType(VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), arraySize),
										  layoutFlags[layoutFlagNdx].flags);

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						createBlockBasicTypeCases(*layoutGroup, m_testCtx, matrixFlags[matFlagNdx].name + "_" + typeName,
												  VarType(VarType(type, PRECISION_HIGH), arraySize),
												  layoutFlags[layoutFlagNdx].flags|matrixFlags[matFlagNdx].flags);
				}
			}
			singleBasicArrayGroup->addChild(layoutGroup.release());
		}
	}

	// ubo.single_struct
	{
		tcu::TestCaseGroup* singleStructGroup = new tcu::TestCaseGroup(m_testCtx, "single_struct", "Single struct in uniform block");
		addChild(singleStructGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			singleStructGroup->addChild(modeGroup);

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

					modeGroup->addChild(new BlockSingleStructCase(m_testCtx, baseName + "_vertex",				 "", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleStructCase(m_testCtx, baseName + "_fragment",			 "", baseFlags|DECLARE_FRAGMENT,				bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleStructCase(m_testCtx, baseName + "_both",				 "", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleStructCase(m_testCtx, baseName + "_vertex_comp_access",	 "", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleStructCase(m_testCtx, baseName + "_fragment_comp_access", "", baseFlags|DECLARE_FRAGMENT,				bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleStructCase(m_testCtx, baseName + "_both_comp_access",	 "", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// ubo.single_struct_array
	{
		tcu::TestCaseGroup* singleStructArrayGroup = new tcu::TestCaseGroup(m_testCtx, "single_struct_array", "Struct array in one uniform block");
		addChild(singleStructArrayGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			singleStructArrayGroup->addChild(modeGroup);

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

					modeGroup->addChild(new BlockSingleStructArrayCase(m_testCtx, baseName + "_vertex",				  "", baseFlags|DECLARE_VERTEX,						bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleStructArrayCase(m_testCtx, baseName + "_fragment",			  "", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleStructArrayCase(m_testCtx, baseName + "_both",				  "",	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleStructArrayCase(m_testCtx, baseName + "_vertex_comp_access",	  "", baseFlags|DECLARE_VERTEX,						bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleStructArrayCase(m_testCtx, baseName + "_fragment_comp_access", "", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleStructArrayCase(m_testCtx, baseName + "_both_comp_access",	  "",	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// ubo.single_nested_struct
	{
		tcu::TestCaseGroup* singleNestedStructGroup = new tcu::TestCaseGroup(m_testCtx, "single_nested_struct", "Nested struct in one uniform block");
		addChild(singleNestedStructGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			singleNestedStructGroup->addChild(modeGroup);

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

					modeGroup->addChild(new BlockSingleNestedStructCase(m_testCtx, baseName + "_vertex",				"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleNestedStructCase(m_testCtx, baseName + "_fragment",				"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleNestedStructCase(m_testCtx, baseName + "_both",					"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleNestedStructCase(m_testCtx, baseName + "_vertex_comp_access",	"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleNestedStructCase(m_testCtx, baseName + "_fragment_comp_access",	"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleNestedStructCase(m_testCtx, baseName + "_both_comp_access",		"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// ubo.single_nested_struct_array
	{
		tcu::TestCaseGroup* singleNestedStructArrayGroup = new tcu::TestCaseGroup(m_testCtx, "single_nested_struct_array", "Nested struct array in one uniform block");
		addChild(singleNestedStructArrayGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			singleNestedStructArrayGroup->addChild(modeGroup);

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

					modeGroup->addChild(new BlockSingleNestedStructArrayCase(m_testCtx, baseName + "_vertex",				"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleNestedStructArrayCase(m_testCtx, baseName + "_fragment",				"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleNestedStructArrayCase(m_testCtx, baseName + "_both",					"",	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockSingleNestedStructArrayCase(m_testCtx, baseName + "_vertex_comp_access",	"", baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleNestedStructArrayCase(m_testCtx, baseName + "_fragment_comp_access",	"", baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockSingleNestedStructArrayCase(m_testCtx, baseName + "_both_comp_access",		"",	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// ubo.instance_array_basic_type
	{
		tcu::TestCaseGroup* instanceArrayBasicTypeGroup = new tcu::TestCaseGroup(m_testCtx, "instance_array_basic_type", "Single basic variable in instance array");
		addChild(instanceArrayBasicTypeGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> layoutGroup(new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, ""));

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				glu::DataType		type			= basicTypes[basicTypeNdx];
				const char* const	typeName		= glu::getDataTypeName(type);
				const int			numInstances	= 3;

				createBlockBasicTypeCases(*layoutGroup, m_testCtx, typeName,
										  VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH),
										  layoutFlags[layoutFlagNdx].flags, numInstances);

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						createBlockBasicTypeCases(*layoutGroup, m_testCtx, matrixFlags[matFlagNdx].name + "_" + typeName,
												  VarType(type, PRECISION_HIGH), layoutFlags[layoutFlagNdx].flags|matrixFlags[matFlagNdx].flags,
												  numInstances);
				}
			}
			instanceArrayBasicTypeGroup->addChild(layoutGroup.release());
		}
	}

	// ubo.multi_basic_types
	{
		tcu::TestCaseGroup* multiBasicTypesGroup = new tcu::TestCaseGroup(m_testCtx, "multi_basic_types", "Multiple buffers with basic types");
		addChild(multiBasicTypesGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			multiBasicTypesGroup->addChild(modeGroup);

			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string	baseName	= layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags	= layoutFlags[layoutFlagNdx].flags;

					if (isArray)
						baseName += "_instance_array";

					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_vertex",				"", baseFlags|DECLARE_VERTEX,					baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_fragment",				"", baseFlags|DECLARE_FRAGMENT,					baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_both",					"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_mixed",				"", baseFlags|DECLARE_VERTEX,					baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_vertex_comp_access",	"", baseFlags|DECLARE_VERTEX,					baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_fragment_comp_access",	"", baseFlags|DECLARE_FRAGMENT,					baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_both_comp_access",		"", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockMultiBasicTypesCase(m_testCtx, baseName + "_mixed_comp_access",	"", baseFlags|DECLARE_VERTEX,					baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// ubo.multi_nested_struct
	{
		tcu::TestCaseGroup* multiNestedStructGroup = new tcu::TestCaseGroup(m_testCtx, "multi_nested_struct", "Multiple buffers with nested structs");
		addChild(multiNestedStructGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			multiNestedStructGroup->addChild(modeGroup);

			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string	baseName	= layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags	= layoutFlags[layoutFlagNdx].flags;

					if (isArray)
						baseName += "_instance_array";

					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_vertex",				  "", baseFlags|DECLARE_VERTEX,						baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_fragment",			  "", baseFlags|DECLARE_FRAGMENT,					baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_both",				  "", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_mixed",				  "", baseFlags|DECLARE_VERTEX,						baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_FULL_MATRIX));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_vertex_comp_access",	  "", baseFlags|DECLARE_VERTEX,						baseFlags|DECLARE_VERTEX,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_fragment_comp_access", "", baseFlags|DECLARE_FRAGMENT,					baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_both_comp_access",	  "", baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	baseFlags|DECLARE_VERTEX|DECLARE_FRAGMENT,	bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
					modeGroup->addChild(new BlockMultiNestedStructCase(m_testCtx, baseName + "_mixed_comp_access",	  "", baseFlags|DECLARE_VERTEX,						baseFlags|DECLARE_FRAGMENT,					bufferModes[modeNdx].mode, isArray ? 3 : 0, LOAD_MATRIX_COMPONENTS));
				}
			}
		}
	}

	// .link_by_binding
	{
		tcu::TestCaseGroup* linkByBindingGroup = new tcu::TestCaseGroup(m_testCtx, "link_by_binding", "Blocks with same name but different binding");
		addChild(linkByBindingGroup);

		linkByBindingGroup->addChild(new LinkByBindingCase(m_testCtx, "single_buf_single_instance",		"", UniformBlockCase::BUFFERMODE_SINGLE, 0));
		linkByBindingGroup->addChild(new LinkByBindingCase(m_testCtx, "single_buf_instance_array",		"", UniformBlockCase::BUFFERMODE_SINGLE, 2));
		linkByBindingGroup->addChild(new LinkByBindingCase(m_testCtx, "per_block_buf_single_instance",	"", UniformBlockCase::BUFFERMODE_PER_BLOCK, 0));
		linkByBindingGroup->addChild(new LinkByBindingCase(m_testCtx, "per_block_buf_instance_array",	"", UniformBlockCase::BUFFERMODE_PER_BLOCK, 2));
	}

	// ubo.random
	{
		const deUint32	allShaders		= FEATURE_VERTEX_BLOCKS|FEATURE_FRAGMENT_BLOCKS|FEATURE_SHARED_BLOCKS;
		const deUint32	allLayouts		= FEATURE_STD140_LAYOUT;
		const deUint32	allBasicTypes	= FEATURE_VECTORS|FEATURE_MATRICES;
		const deUint32	unused			= FEATURE_UNUSED_MEMBERS|FEATURE_UNUSED_UNIFORMS;
		const deUint32	matFlags		= FEATURE_MATRIX_LAYOUT;
		const deUint32	allFeatures		= ~FEATURE_OUT_OF_ORDER_OFFSETS;  // OOO offsets handled in a dedicated case group

		tcu::TestCaseGroup* randomGroup = new tcu::TestCaseGroup(m_testCtx, "random", "Random Uniform Block cases");
		addChild(randomGroup);

		// Basic types.
		createRandomCaseGroup(randomGroup, m_testCtx, "scalar_types",	"Scalar types only, per-block buffers",				UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused,										25, 0);
		createRandomCaseGroup(randomGroup, m_testCtx, "vector_types",	"Scalar and vector types only, per-block buffers",	UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|FEATURE_VECTORS,						25, 25);
		createRandomCaseGroup(randomGroup, m_testCtx, "basic_types",	"All basic types, per-block buffers",				UniformBlockCase::BUFFERMODE_PER_BLOCK, allShaders|allLayouts|unused|allBasicTypes|matFlags,				25, 50);
		createRandomCaseGroup(randomGroup, m_testCtx, "basic_arrays",	"Arrays, per-block buffers",						UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_ARRAYS,	25, 50);

		createRandomCaseGroup(randomGroup, m_testCtx, "basic_instance_arrays",					"Basic instance arrays, per-block buffers",				UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_INSTANCE_ARRAYS,								25, 75);
		createRandomCaseGroup(randomGroup, m_testCtx, "nested_structs",							"Nested structs, per-block buffers",					UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_STRUCTS,										25, 100);
		createRandomCaseGroup(randomGroup, m_testCtx, "nested_structs_arrays",					"Nested structs, arrays, per-block buffers",			UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_STRUCTS|FEATURE_ARRAYS,							25, 150);
		createRandomCaseGroup(randomGroup, m_testCtx, "nested_structs_instance_arrays",			"Nested structs, instance arrays, per-block buffers",	UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_STRUCTS|FEATURE_INSTANCE_ARRAYS,				25, 125);
		createRandomCaseGroup(randomGroup, m_testCtx, "nested_structs_arrays_instance_arrays",	"Nested structs, instance arrays, per-block buffers",	UniformBlockCase::BUFFERMODE_PER_BLOCK,	allShaders|allLayouts|unused|allBasicTypes|matFlags|FEATURE_STRUCTS|FEATURE_ARRAYS|FEATURE_INSTANCE_ARRAYS,	25, 175);

		createRandomCaseGroup(randomGroup, m_testCtx, "all_per_block_buffers",	"All random features, per-block buffers",	UniformBlockCase::BUFFERMODE_PER_BLOCK,	allFeatures,	50, 200);
		createRandomCaseGroup(randomGroup, m_testCtx, "all_shared_buffer",		"All random features, shared buffer",		UniformBlockCase::BUFFERMODE_SINGLE,	allFeatures,	50, 250);

		createRandomCaseGroup(randomGroup, m_testCtx, "all_out_of_order_offsets",	"All random features, out of order member offsets",		UniformBlockCase::BUFFERMODE_PER_BLOCK,	allFeatures | FEATURE_OUT_OF_ORDER_OFFSETS,	50, 300);
	}
}

} // anonymous

tcu::TestCaseGroup*	createTests	(tcu::TestContext& testCtx)
{
	return new UniformBlockTests(testCtx);
}

} // ubo
} // vkt
