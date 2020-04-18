/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Uniform block tests.
 */ /*-------------------------------------------------------------------*/

#include "glcUniformBlockTests.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "glcUniformBlockCase.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"

namespace deqp
{

using std::string;
using std::vector;
using deqp::Context;

using namespace ub;

enum FeatureBits
{
	FEATURE_VECTORS			= (1 << 0),
	FEATURE_MATRICES		= (1 << 1),
	FEATURE_ARRAYS			= (1 << 2),
	FEATURE_STRUCTS			= (1 << 3),
	FEATURE_NESTED_STRUCTS  = (1 << 4),
	FEATURE_INSTANCE_ARRAYS = (1 << 5),
	FEATURE_VERTEX_BLOCKS   = (1 << 6),
	FEATURE_FRAGMENT_BLOCKS = (1 << 7),
	FEATURE_SHARED_BLOCKS   = (1 << 8),
	FEATURE_UNUSED_UNIFORMS = (1 << 9),
	FEATURE_UNUSED_MEMBERS  = (1 << 10),
	FEATURE_PACKED_LAYOUT   = (1 << 11),
	FEATURE_SHARED_LAYOUT   = (1 << 12),
	FEATURE_STD140_LAYOUT   = (1 << 13),
	FEATURE_MATRIX_LAYOUT   = (1 << 14) //!< Matrix layout flags.
};

class RandomUniformBlockCase : public UniformBlockCase
{
public:
	RandomUniformBlockCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
						   BufferMode bufferMode, deUint32 features, deUint32 seed);

	void init(void);

private:
	void generateBlock(de::Random& rnd, deUint32 layoutFlags);
	void generateUniform(de::Random& rnd, UniformBlock& block);
	VarType generateType(de::Random& rnd, int typeDepth, bool arrayOk);

	deUint32 m_features;
	int		 m_maxVertexBlocks;
	int		 m_maxFragmentBlocks;
	int		 m_maxSharedBlocks;
	int		 m_maxInstances;
	int		 m_maxArrayLength;
	int		 m_maxStructDepth;
	int		 m_maxBlockMembers;
	int		 m_maxStructMembers;
	deUint32 m_seed;

	int m_blockNdx;
	int m_uniformNdx;
	int m_structNdx;
};

RandomUniformBlockCase::RandomUniformBlockCase(Context& context, const char* name, const char* description,
											   glu::GLSLVersion glslVersion, BufferMode bufferMode, deUint32 features,
											   deUint32 seed)
	: UniformBlockCase(context, name, description, glslVersion, bufferMode)
	, m_features(features)
	, m_maxVertexBlocks((features & FEATURE_VERTEX_BLOCKS) ? 4 : 0)
	, m_maxFragmentBlocks((features & FEATURE_FRAGMENT_BLOCKS) ? 4 : 0)
	, m_maxSharedBlocks((features & FEATURE_SHARED_BLOCKS) ? 4 : 0)
	, m_maxInstances((features & FEATURE_INSTANCE_ARRAYS) ? 3 : 0)
	, m_maxArrayLength((features & FEATURE_ARRAYS) ? 8 : 0)
	, m_maxStructDepth((features & FEATURE_STRUCTS) ? 2 : 0)
	, m_maxBlockMembers(5)
	, m_maxStructMembers(4)
	, m_seed(seed)
	, m_blockNdx(1)
	, m_uniformNdx(1)
	, m_structNdx(1)
{
}

void RandomUniformBlockCase::init(void)
{
	de::Random rnd(m_seed);

	int numShared	 = m_maxSharedBlocks > 0 ? rnd.getInt(1, m_maxSharedBlocks) : 0;
	int numVtxBlocks  = m_maxVertexBlocks - numShared > 0 ? rnd.getInt(1, m_maxVertexBlocks - numShared) : 0;
	int numFragBlocks = m_maxFragmentBlocks - numShared > 0 ? rnd.getInt(1, m_maxFragmentBlocks - numShared) : 0;

	for (int ndx = 0; ndx < numShared; ndx++)
		generateBlock(rnd, DECLARE_VERTEX | DECLARE_FRAGMENT);

	for (int ndx = 0; ndx < numVtxBlocks; ndx++)
		generateBlock(rnd, DECLARE_VERTEX);

	for (int ndx = 0; ndx < numFragBlocks; ndx++)
		generateBlock(rnd, DECLARE_FRAGMENT);
}

void RandomUniformBlockCase::generateBlock(de::Random& rnd, deUint32 layoutFlags)
{
	DE_ASSERT(m_blockNdx <= 'z' - 'a');

	const float   instanceArrayWeight = 0.3f;
	UniformBlock& block				  = m_interface.allocBlock((string("Block") + (char)('A' + m_blockNdx)).c_str());
	int numInstances = (m_maxInstances > 0 && rnd.getFloat() < instanceArrayWeight) ? rnd.getInt(0, m_maxInstances) : 0;
	int numUniforms  = rnd.getInt(1, m_maxBlockMembers);

	if (numInstances > 0)
		block.setArraySize(numInstances);

	if (numInstances > 0 || rnd.getBool())
		block.setInstanceName((string("block") + (char)('A' + m_blockNdx)).c_str());

	// Layout flag candidates.
	vector<deUint32> layoutFlagCandidates;
	layoutFlagCandidates.push_back(0);
	if (m_features & FEATURE_PACKED_LAYOUT)
		layoutFlagCandidates.push_back(LAYOUT_SHARED);
	if ((m_features & FEATURE_SHARED_LAYOUT) && ((layoutFlags & DECLARE_BOTH) != DECLARE_BOTH))
		layoutFlagCandidates.push_back(LAYOUT_PACKED); // \note packed layout can only be used in a single shader stage.
	if (m_features & FEATURE_STD140_LAYOUT)
		layoutFlagCandidates.push_back(LAYOUT_STD140);

	layoutFlags |= rnd.choose<deUint32>(layoutFlagCandidates.begin(), layoutFlagCandidates.end());

	if (m_features & FEATURE_MATRIX_LAYOUT)
	{
		static const deUint32 matrixCandidates[] = { 0, LAYOUT_ROW_MAJOR, LAYOUT_COLUMN_MAJOR };
		layoutFlags |=
			rnd.choose<deUint32>(&matrixCandidates[0], &matrixCandidates[DE_LENGTH_OF_ARRAY(matrixCandidates)]);
	}

	block.setFlags(layoutFlags);

	for (int ndx = 0; ndx < numUniforms; ndx++)
		generateUniform(rnd, block);

	m_blockNdx += 1;
}

static std::string genName(char first, char last, int ndx)
{
	std::string str			= "";
	int			alphabetLen = last - first + 1;

	while (ndx > alphabetLen)
	{
		str.insert(str.begin(), (char)(first + ((ndx - 1) % alphabetLen)));
		ndx = ((ndx - 1) / alphabetLen);
	}

	str.insert(str.begin(), (char)(first + (ndx % (alphabetLen + 1)) - 1));

	return str;
}

void RandomUniformBlockCase::generateUniform(de::Random& rnd, UniformBlock& block)
{
	const float unusedVtxWeight  = 0.15f;
	const float unusedFragWeight = 0.15f;
	bool		unusedOk		 = (m_features & FEATURE_UNUSED_UNIFORMS) != 0;
	deUint32	flags			 = 0;
	std::string name			 = genName('a', 'z', m_uniformNdx);
	VarType		type			 = generateType(rnd, 0, true);

	flags |= (unusedOk && rnd.getFloat() < unusedVtxWeight) ? UNUSED_VERTEX : 0;
	flags |= (unusedOk && rnd.getFloat() < unusedFragWeight) ? UNUSED_FRAGMENT : 0;

	block.addUniform(Uniform(name.c_str(), type, flags));

	m_uniformNdx += 1;
}

VarType RandomUniformBlockCase::generateType(de::Random& rnd, int typeDepth, bool arrayOk)
{
	const float structWeight = 0.1f;
	const float arrayWeight  = 0.1f;

	if (typeDepth < m_maxStructDepth && rnd.getFloat() < structWeight)
	{
		const float		unusedVtxWeight  = 0.15f;
		const float		unusedFragWeight = 0.15f;
		bool			unusedOk		 = (m_features & FEATURE_UNUSED_MEMBERS) != 0;
		vector<VarType> memberTypes;
		int				numMembers = rnd.getInt(1, m_maxStructMembers);

		// Generate members first so nested struct declarations are in correct order.
		for (int ndx = 0; ndx < numMembers; ndx++)
			memberTypes.push_back(generateType(rnd, typeDepth + 1, true));

		StructType& structType = m_interface.allocStruct((string("s") + genName('A', 'Z', m_structNdx)).c_str());
		m_structNdx += 1;

		DE_ASSERT(numMembers <= 'Z' - 'A');
		for (int ndx = 0; ndx < numMembers; ndx++)
		{
			deUint32 flags = 0;

			flags |= (unusedOk && rnd.getFloat() < unusedVtxWeight) ? UNUSED_VERTEX : 0;
			flags |= (unusedOk && rnd.getFloat() < unusedFragWeight) ? UNUSED_FRAGMENT : 0;

			structType.addMember((string("m") + (char)('A' + ndx)).c_str(), memberTypes[ndx], flags);
		}

		return VarType(&structType);
	}
	else if (m_maxArrayLength > 0 && arrayOk && rnd.getFloat() < arrayWeight)
	{
		int		arrayLength = rnd.getInt(1, m_maxArrayLength);
		VarType elementType = generateType(rnd, typeDepth, false /* nested arrays are not allowed */);
		return VarType(elementType, arrayLength);
	}
	else
	{
		vector<glu::DataType> typeCandidates;

		typeCandidates.push_back(glu::TYPE_FLOAT);
		typeCandidates.push_back(glu::TYPE_INT);
		typeCandidates.push_back(glu::TYPE_UINT);
		typeCandidates.push_back(glu::TYPE_BOOL);

		if (m_features & FEATURE_VECTORS)
		{
			typeCandidates.push_back(glu::TYPE_FLOAT_VEC2);
			typeCandidates.push_back(glu::TYPE_FLOAT_VEC3);
			typeCandidates.push_back(glu::TYPE_FLOAT_VEC4);
			typeCandidates.push_back(glu::TYPE_INT_VEC2);
			typeCandidates.push_back(glu::TYPE_INT_VEC3);
			typeCandidates.push_back(glu::TYPE_INT_VEC4);
			typeCandidates.push_back(glu::TYPE_UINT_VEC2);
			typeCandidates.push_back(glu::TYPE_UINT_VEC3);
			typeCandidates.push_back(glu::TYPE_UINT_VEC4);
			typeCandidates.push_back(glu::TYPE_BOOL_VEC2);
			typeCandidates.push_back(glu::TYPE_BOOL_VEC3);
			typeCandidates.push_back(glu::TYPE_BOOL_VEC4);
		}

		if (m_features & FEATURE_MATRICES)
		{
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT2);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT2X3);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT3X2);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT3);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT3X4);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT4X2);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT4X3);
			typeCandidates.push_back(glu::TYPE_FLOAT_MAT4);
		}

		glu::DataType type  = rnd.choose<glu::DataType>(typeCandidates.begin(), typeCandidates.end());
		deUint32	  flags = 0;

		if (!glu::isDataTypeBoolOrBVec(type))
		{
			// Precision.
			static const deUint32 precisionCandidates[] = { PRECISION_LOW, PRECISION_MEDIUM, PRECISION_HIGH };
			flags |= rnd.choose<deUint32>(&precisionCandidates[0],
										  &precisionCandidates[DE_LENGTH_OF_ARRAY(precisionCandidates)]);
		}

		return VarType(type, flags);
	}
}

class BlockBasicTypeCase : public UniformBlockCase
{
public:
	BlockBasicTypeCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
					   const VarType& type, deUint32 layoutFlags, int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, BUFFERMODE_PER_BLOCK)
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

static void createRandomCaseGroup(tcu::TestCaseGroup* parentGroup, Context& context, const char* groupName,
								  const char* description, glu::GLSLVersion glslVersion,
								  UniformBlockCase::BufferMode bufferMode, deUint32 features, int numCases,
								  deUint32 baseSeed)
{
	tcu::TestCaseGroup* group = new tcu::TestCaseGroup(context.getTestContext(), groupName, description);
	parentGroup->addChild(group);

	baseSeed += (deUint32)context.getTestContext().getCommandLine().getBaseSeed();

	for (int ndx = 0; ndx < numCases; ndx++)
		group->addChild(new RandomUniformBlockCase(context, de::toString(ndx).c_str(), "", glslVersion, bufferMode,
												   features, (deUint32)deInt32Hash(ndx + baseSeed)));
}

class BlockSingleStructCase : public UniformBlockCase
{
public:
	BlockSingleStructCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
						  deUint32 layoutFlags, BufferMode bufferMode, int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, bufferMode)
		, m_layoutFlags(layoutFlags)
		, m_numInstances(numInstances)
	{
	}

	void init(void)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_INT_VEC3, PRECISION_HIGH), UNUSED_BOTH); // First member is unused.
		typeS.addMember("b", VarType(VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("s", VarType(&typeS), 0));
		block.setFlags(m_layoutFlags);

		if (m_numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(m_numInstances);
		}
	}

private:
	deUint32 m_layoutFlags;
	int		 m_numInstances;
};

class BlockSingleStructArrayCase : public UniformBlockCase
{
public:
	BlockSingleStructArrayCase(Context& context, const char* name, const char* description,
							   glu::GLSLVersion glslVersion, deUint32 layoutFlags, BufferMode bufferMode,
							   int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, bufferMode)
		, m_layoutFlags(layoutFlags)
		, m_numInstances(numInstances)
	{
	}

	void init(void)
	{
		StructType& typeS = m_interface.allocStruct("S");
		typeS.addMember("a", VarType(glu::TYPE_INT_VEC3, PRECISION_HIGH), UNUSED_BOTH);
		typeS.addMember("b", VarType(VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM), 4));
		typeS.addMember("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH));

		UniformBlock& block = m_interface.allocBlock("Block");
		block.addUniform(Uniform("u", VarType(glu::TYPE_UINT, PRECISION_LOW)));
		block.addUniform(Uniform("s", VarType(VarType(&typeS), 3)));
		block.addUniform(Uniform("v", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_MEDIUM)));
		block.setFlags(m_layoutFlags);

		if (m_numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(m_numInstances);
		}
	}

private:
	deUint32 m_layoutFlags;
	int		 m_numInstances;
};

class BlockSingleNestedStructCase : public UniformBlockCase
{
public:
	BlockSingleNestedStructCase(Context& context, const char* name, const char* description,
								glu::GLSLVersion glslVersion, deUint32 layoutFlags, BufferMode bufferMode,
								int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, bufferMode)
		, m_layoutFlags(layoutFlags)
		, m_numInstances(numInstances)
	{
	}

	void init(void)
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
		block.setFlags(m_layoutFlags);

		if (m_numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(m_numInstances);
		}
	}

private:
	deUint32 m_layoutFlags;
	int		 m_numInstances;
};

class BlockSingleNestedStructArrayCase : public UniformBlockCase
{
public:
	BlockSingleNestedStructArrayCase(Context& context, const char* name, const char* description,
									 glu::GLSLVersion glslVersion, deUint32 layoutFlags, BufferMode bufferMode,
									 int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, bufferMode)
		, m_layoutFlags(layoutFlags)
		, m_numInstances(numInstances)
	{
	}

	void init(void)
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
		block.setFlags(m_layoutFlags);

		if (m_numInstances > 0)
		{
			block.setInstanceName("block");
			block.setArraySize(m_numInstances);
		}
	}

private:
	deUint32 m_layoutFlags;
	int		 m_numInstances;
};

class BlockMultiBasicTypesCase : public UniformBlockCase
{
public:
	BlockMultiBasicTypesCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
							 deUint32 flagsA, deUint32 flagsB, BufferMode bufferMode, int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, bufferMode)
		, m_flagsA(flagsA)
		, m_flagsB(flagsB)
		, m_numInstances(numInstances)
	{
	}

	void init(void)
	{
		UniformBlock& blockA = m_interface.allocBlock("BlockA");
		blockA.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT, PRECISION_HIGH)));
		blockA.addUniform(Uniform("b", VarType(glu::TYPE_UINT_VEC3, PRECISION_LOW), UNUSED_BOTH));
		blockA.addUniform(Uniform("c", VarType(glu::TYPE_FLOAT_MAT2, PRECISION_MEDIUM)));
		blockA.setInstanceName("blockA");
		blockA.setFlags(m_flagsA);

		UniformBlock& blockB = m_interface.allocBlock("BlockB");
		blockB.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT_MAT3, PRECISION_MEDIUM)));
		blockB.addUniform(Uniform("b", VarType(glu::TYPE_INT_VEC2, PRECISION_LOW)));
		blockB.addUniform(Uniform("c", VarType(glu::TYPE_FLOAT_VEC4, PRECISION_HIGH), UNUSED_BOTH));
		blockB.addUniform(Uniform("d", VarType(glu::TYPE_BOOL, 0)));
		blockB.setInstanceName("blockB");
		blockB.setFlags(m_flagsB);

		if (m_numInstances > 0)
		{
			blockA.setArraySize(m_numInstances);
			blockB.setArraySize(m_numInstances);
		}
	}

private:
	deUint32 m_flagsA;
	deUint32 m_flagsB;
	int		 m_numInstances;
};

class BlockMultiNestedStructCase : public UniformBlockCase
{
public:
	BlockMultiNestedStructCase(Context& context, const char* name, const char* description,
							   glu::GLSLVersion glslVersion, deUint32 flagsA, deUint32 flagsB, BufferMode bufferMode,
							   int numInstances)
		: UniformBlockCase(context, name, description, glslVersion, bufferMode)
		, m_flagsA(flagsA)
		, m_flagsB(flagsB)
		, m_numInstances(numInstances)
	{
	}

	void init(void)
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
		blockA.setFlags(m_flagsA);

		UniformBlock& blockB = m_interface.allocBlock("BlockB");
		blockB.addUniform(Uniform("a", VarType(glu::TYPE_FLOAT_MAT2, PRECISION_MEDIUM)));
		blockB.addUniform(Uniform("b", VarType(&typeT)));
		blockB.addUniform(Uniform("c", VarType(glu::TYPE_BOOL_VEC4, 0), UNUSED_BOTH));
		blockB.addUniform(Uniform("d", VarType(glu::TYPE_BOOL, 0)));
		blockB.setInstanceName("blockB");
		blockB.setFlags(m_flagsB);

		if (m_numInstances > 0)
		{
			blockA.setArraySize(m_numInstances);
			blockB.setArraySize(m_numInstances);
		}
	}

private:
	deUint32 m_flagsA;
	deUint32 m_flagsB;
	int		 m_numInstances;
};

class UniformBlockPrecisionMatching : public TestCase
{
public:
	UniformBlockPrecisionMatching(Context& context, glu::GLSLVersion glslVersion)
		: TestCase(context, "precision_matching", ""), m_glslVersion(glslVersion)
	{
	}

	IterateResult iterate(void)
	{
		std::string vs1("layout (std140) uniform Data { lowp float x; } myData;\n"
						"void main() {\n"
						"  gl_Position = vec4(myData.x, 0.0, 0.0, 1.0);\n"
						"}");
		std::string fs1("precision highp float;\n"
						"out vec4 color;\n"
						"layout (std140) uniform Data { float x; } myData;\n"
						"void main() {\n"
						"  color = vec4(myData.x);\n"
						"}");

		std::string vs2("layout (std140) uniform Data { highp int x; mediump int y; } myData;\n"
						"void main() {\n"
						"  gl_Position = vec4(float(myData.x), 0.0, 0.0, 1.0);\n"
						"}");
		std::string fs2("precision highp float;\n"
						"out vec4 color;\n"
						"layout (std140) uniform Data { mediump int x; highp int y; } myData;\n"
						"void main() {\n"
						"  color = vec4(float(myData.y));\n"
						"}");

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		if (!Link(vs1, fs1) || !Link(vs2, fs2))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	bool Link(const std::string& vs, const std::string& fs)
	{
		const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();
		const glw::GLuint	 p		  = gl.createProgram();
		const std::string	 version = glu::getGLSLVersionDeclaration(m_glslVersion);

		const struct
		{
			const char*		   name;
			const std::string& body;
			glw::GLenum		   type;
		} shaderDefinition[] = { { "VS", vs, GL_VERTEX_SHADER }, { "FS", fs, GL_FRAGMENT_SHADER } };

		for (unsigned int index = 0; index < 2; ++index)
		{
			std::string shaderSource	= version + "\n" + shaderDefinition[index].body;
			const char* shaderSourcePtr = shaderSource.c_str();

			glw::GLuint sh = gl.createShader(shaderDefinition[index].type);
			gl.attachShader(p, sh);
			gl.deleteShader(sh);
			gl.shaderSource(sh, 1, &shaderSourcePtr, NULL);
			gl.compileShader(sh);

			glw::GLint status;
			gl.getShaderiv(sh, GL_COMPILE_STATUS, &status);
			if (status == GL_FALSE)
			{
				glw::GLint length;
				gl.getShaderiv(sh, GL_INFO_LOG_LENGTH, &length);
				if (length > 0)
				{
					std::vector<glw::GLchar> log(length);
					gl.getShaderInfoLog(sh, length, NULL, &log[0]);
					m_context.getTestContext().getLog() << tcu::TestLog::Message << shaderDefinition[index].name
														<< " compilation should succed. Info Log:\n"
														<< &log[0] << tcu::TestLog::EndMessage;
				}
				gl.deleteProgram(p);
				return false;
			}
		}

		gl.linkProgram(p);

		bool	   result = true;
		glw::GLint status;
		gl.getProgramiv(p, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			glw::GLchar log[1024];
			gl.getProgramInfoLog(p, sizeof(log), NULL, log);
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Link operation should succed. Info Log:\n"
												<< log << tcu::TestLog::EndMessage;
			result = false;
		}

		gl.deleteProgram(p);
		return result;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

class UniformBlockNameMatching : public TestCase
{
public:
	UniformBlockNameMatching(Context& context, glu::GLSLVersion glslVersion)
		: TestCase(context, "name_matching", ""), m_glslVersion(glslVersion)
	{
	}

	IterateResult iterate(void)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		std::string vs1("precision highp float;\n"
						"layout (std140) uniform Data { vec4 v; };\n"
						"void main() {\n"
						"  gl_Position = v;\n"
						"}");
		std::string fs1("precision highp float;\n"
						"out vec4 color;\n"
						"layout (std140) uniform Data { vec4 v; } myData;\n"
						"void main() {\n"
						"  color = vec4(myData.v);\n"
						"}");

		// check if link error is generated when one of matched blocks has instance name and other doesn't
		if (!Test(vs1, fs1, GL_FALSE))
			return STOP;

		std::string vs2("precision highp float;\n"
						"uniform Data { vec4 v; };\n"
						"void main() {\n"
						"  gl_Position = v;\n"
						"}");
		std::string fs2("precision highp float;\n"
						"out vec4 color;\n"
						"uniform Data { vec4 v; };\n"
						"void main() {\n"
						"  color = v;\n"
						"}");

		// check if linking succeeds when both matched blocks are lacking an instance name
		if (!Test(vs2, fs2, GL_TRUE))
			return STOP;

		std::string vs3("precision highp float;\n"
						"layout (std140) uniform Data { vec4 v; } a;\n"
						"void main() {\n"
						"  gl_Position = a.v;\n"
						"}");
		std::string fs3("precision highp float;\n"
						"out vec4 color;\n"
						"layout (std140) uniform Data { vec4 v; } b;\n"
						"void main() {\n"
						"  color = b.v;\n"
						"}");

		// check if linking succeeds when both matched blocks are lacking an instance name
		if (!Test(vs3, fs3, GL_TRUE))
			return STOP;

		std::string vs4("precision highp float;\n"
						"layout (std140) uniform Data { float f; };\n"
						"void main() {\n"
						"  gl_Position = vec4(f);\n"
						"}\n");
		std::string fs4("precision highp float;\n"
						"uniform float f;\n"
						"out vec4 color;\n"
						"void main() {\n"
						"  color = vec4(f);\n"
						"}\n");

		// check if link error is generated when the same name is used for block and non-block uniform
		if (!Test(vs4, fs4, GL_FALSE))
			return STOP;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

	bool Test(const std::string& vs, const std::string& fs, glw::GLint expectedLinkStatus)
	{
		const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();
		const glw::GLuint	 p		  = gl.createProgram();
		const std::string	 version = glu::getGLSLVersionDeclaration(m_glslVersion);

		const struct
		{
			const char*		   name;
			const std::string& body;
			glw::GLenum		   type;
		} shaderDefinition[] = { { "VS", vs, GL_VERTEX_SHADER }, { "FS", fs, GL_FRAGMENT_SHADER } };

		for (unsigned int index = 0; index < 2; ++index)
		{
			std::string shaderSource	= version + "\n" + shaderDefinition[index].body;
			const char* shaderSourcePtr = shaderSource.c_str();

			glw::GLuint sh = gl.createShader(shaderDefinition[index].type);
			gl.attachShader(p, sh);
			gl.deleteShader(sh);
			gl.shaderSource(sh, 1, &shaderSourcePtr, NULL);
			gl.compileShader(sh);

			glw::GLint status;
			gl.getShaderiv(sh, GL_COMPILE_STATUS, &status);
			if (status == GL_FALSE)
			{
				glw::GLint length;
				gl.getShaderiv(sh, GL_INFO_LOG_LENGTH, &length);
				if (length > 0)
				{
					std::vector<glw::GLchar> log(length);
					gl.getShaderInfoLog(sh, length, NULL, &log[0]);
					m_context.getTestContext().getLog() << tcu::TestLog::Message << shaderDefinition[index].name
														<< " compilation should succed. Info Log:\n"
														<< &log[0] << tcu::TestLog::EndMessage;
				}
				gl.deleteProgram(p);
				return false;
			}
		}

		gl.linkProgram(p);

		bool	   result = true;
		glw::GLint status;
		gl.getProgramiv(p, GL_LINK_STATUS, &status);
		if (status != expectedLinkStatus)
		{
			if (status == GL_TRUE)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Link operation should fail.\n"
													<< tcu::TestLog::EndMessage;
			}
			else
			{
				glw::GLchar log[1024];
				gl.getProgramInfoLog(p, sizeof(log), NULL, log);
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Link operation should succed. Info Log:\n"
					<< log << tcu::TestLog::EndMessage;
			}
			result = false;
		}

		gl.deleteProgram(p);
		return result;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

UniformBlockTests::UniformBlockTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "uniform_block", "Uniform Block tests"), m_glslVersion(glslVersion)
{
}

UniformBlockTests::~UniformBlockTests(void)
{
}

void UniformBlockTests::init(void)
{
	static const glu::DataType basicTypes[] = { glu::TYPE_FLOAT,		glu::TYPE_FLOAT_VEC2,   glu::TYPE_FLOAT_VEC3,
												glu::TYPE_FLOAT_VEC4,   glu::TYPE_INT,			glu::TYPE_INT_VEC2,
												glu::TYPE_INT_VEC3,		glu::TYPE_INT_VEC4,		glu::TYPE_UINT,
												glu::TYPE_UINT_VEC2,	glu::TYPE_UINT_VEC3,	glu::TYPE_UINT_VEC4,
												glu::TYPE_BOOL,			glu::TYPE_BOOL_VEC2,	glu::TYPE_BOOL_VEC3,
												glu::TYPE_BOOL_VEC4,	glu::TYPE_FLOAT_MAT2,   glu::TYPE_FLOAT_MAT3,
												glu::TYPE_FLOAT_MAT4,   glu::TYPE_FLOAT_MAT2X3, glu::TYPE_FLOAT_MAT2X4,
												glu::TYPE_FLOAT_MAT3X2, glu::TYPE_FLOAT_MAT3X4, glu::TYPE_FLOAT_MAT4X2,
												glu::TYPE_FLOAT_MAT4X3 };

	static const struct
	{
		const char* name;
		deUint32	flags;
	} precisionFlags[] = { { "lowp", PRECISION_LOW }, { "mediump", PRECISION_MEDIUM }, { "highp", PRECISION_HIGH } };

	static const struct
	{
		const char* name;
		deUint32	flags;
	} layoutFlags[] = { { "shared", LAYOUT_SHARED }, { "packed", LAYOUT_PACKED }, { "std140", LAYOUT_STD140 } };

	static const struct
	{
		const char* name;
		deUint32	flags;
	} matrixFlags[] = { { "row_major", LAYOUT_ROW_MAJOR }, { "column_major", LAYOUT_COLUMN_MAJOR } };

	static const struct
	{
		const char*					 name;
		UniformBlockCase::BufferMode mode;
	} bufferModes[] = { { "per_block_buffer", UniformBlockCase::BUFFERMODE_PER_BLOCK },
						{ "single_buffer", UniformBlockCase::BUFFERMODE_SINGLE } };

	// ubo.single_basic_type
	{
		tcu::TestCaseGroup* singleBasicTypeGroup =
			new tcu::TestCaseGroup(m_testCtx, "single_basic_type", "Single basic variable in single buffer");
		addChild(singleBasicTypeGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			tcu::TestCaseGroup* layoutGroup = new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, "");
			singleBasicTypeGroup->addChild(layoutGroup);

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				glu::DataType type		= basicTypes[basicTypeNdx];
				const char*   typeName  = glu::getDataTypeName(type);
				deUint32	  baseFlags = layoutFlags[layoutFlagNdx].flags;
				deUint32	  flags		= baseFlags | ((baseFlags & LAYOUT_PACKED) ?
												  (basicTypeNdx % 2 == 0 ? DECLARE_VERTEX : DECLARE_FRAGMENT) :
												  DECLARE_BOTH);

				if (glu::isDataTypeBoolOrBVec(type))
					layoutGroup->addChild(new BlockBasicTypeCase(m_context, typeName, "", m_glslVersion,
																 VarType(type, 0), flags, 0 /* no instance array */));
				else
				{
					for (int precNdx = 0; precNdx < DE_LENGTH_OF_ARRAY(precisionFlags); precNdx++)
						layoutGroup->addChild(new BlockBasicTypeCase(
							m_context, (string(precisionFlags[precNdx].name) + "_" + typeName).c_str(), "",
							m_glslVersion, VarType(type, precisionFlags[precNdx].flags), flags,
							0 /* no instance array */));
				}

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
					{
						for (int precNdx = 0; precNdx < DE_LENGTH_OF_ARRAY(precisionFlags); precNdx++)
							layoutGroup->addChild(new BlockBasicTypeCase(
								m_context,
								(string(matrixFlags[matFlagNdx].name) + "_" + precisionFlags[precNdx].name + "_" +
								 typeName)
									.c_str(),
								"", m_glslVersion, VarType(type, precisionFlags[precNdx].flags),
								flags | matrixFlags[matFlagNdx].flags, 0 /* no instance array */));
					}
				}
			}
		}
	}

	// ubo.single_basic_array
	{
		tcu::TestCaseGroup* singleBasicArrayGroup =
			new tcu::TestCaseGroup(m_testCtx, "single_basic_array", "Single basic array variable in single buffer");
		addChild(singleBasicArrayGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			tcu::TestCaseGroup* layoutGroup = new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, "");
			singleBasicArrayGroup->addChild(layoutGroup);

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				glu::DataType type		= basicTypes[basicTypeNdx];
				const char*   typeName  = glu::getDataTypeName(type);
				const int	 arraySize = 3;
				deUint32	  baseFlags = layoutFlags[layoutFlagNdx].flags;
				deUint32	  flags		= baseFlags | ((baseFlags & LAYOUT_PACKED) ?
												  (basicTypeNdx % 2 == 0 ? DECLARE_VERTEX : DECLARE_FRAGMENT) :
												  DECLARE_BOTH);

				layoutGroup->addChild(new BlockBasicTypeCase(
					m_context, typeName, "", m_glslVersion,
					VarType(VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), arraySize), flags,
					0 /* no instance array */));

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						layoutGroup->addChild(new BlockBasicTypeCase(
							m_context, (string(matrixFlags[matFlagNdx].name) + "_" + typeName).c_str(), "",
							m_glslVersion, VarType(VarType(type, PRECISION_HIGH), arraySize),
							flags | matrixFlags[matFlagNdx].flags, 0 /* no instance array */));
				}
			}
		}
	}

	// ubo.single_struct
	{
		tcu::TestCaseGroup* singleStructGroup =
			new tcu::TestCaseGroup(m_testCtx, "single_struct", "Single struct in uniform block");
		addChild(singleStructGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string name = std::string(bufferModes[modeNdx].name) + "_" + layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags = layoutFlags[layoutFlagNdx].flags;
					deUint32	flags	 = baseFlags | ((baseFlags & LAYOUT_PACKED) ? DECLARE_VERTEX : DECLARE_BOTH);

					if (bufferModes[modeNdx].mode == UniformBlockCase::BUFFERMODE_SINGLE && isArray == 0)
						continue; // Doesn't make sense to add this variant.

					if (isArray)
						name += "_instance_array";

					singleStructGroup->addChild(new BlockSingleStructCase(
						m_context, name.c_str(), "", m_glslVersion, flags, bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.single_struct_array
	{
		tcu::TestCaseGroup* singleStructArrayGroup =
			new tcu::TestCaseGroup(m_testCtx, "single_struct_array", "Struct array in one uniform block");
		addChild(singleStructArrayGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string name = std::string(bufferModes[modeNdx].name) + "_" + layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags = layoutFlags[layoutFlagNdx].flags;
					deUint32	flags	 = baseFlags | ((baseFlags & LAYOUT_PACKED) ? DECLARE_FRAGMENT : DECLARE_BOTH);

					if (bufferModes[modeNdx].mode == UniformBlockCase::BUFFERMODE_SINGLE && isArray == 0)
						continue; // Doesn't make sense to add this variant.

					if (isArray)
						name += "_instance_array";

					singleStructArrayGroup->addChild(new BlockSingleStructArrayCase(
						m_context, name.c_str(), "", m_glslVersion, flags, bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.single_nested_struct
	{
		tcu::TestCaseGroup* singleNestedStructGroup =
			new tcu::TestCaseGroup(m_testCtx, "single_nested_struct", "Nested struct in one uniform block");
		addChild(singleNestedStructGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string name = std::string(bufferModes[modeNdx].name) + "_" + layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags = layoutFlags[layoutFlagNdx].flags;
					deUint32	flags	 = baseFlags | ((baseFlags & LAYOUT_PACKED) ? DECLARE_VERTEX : DECLARE_BOTH);

					if (bufferModes[modeNdx].mode == UniformBlockCase::BUFFERMODE_SINGLE && isArray == 0)
						continue; // Doesn't make sense to add this variant.

					if (isArray)
						name += "_instance_array";

					singleNestedStructGroup->addChild(new BlockSingleNestedStructCase(
						m_context, name.c_str(), "", m_glslVersion, flags, bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.single_nested_struct_array
	{
		tcu::TestCaseGroup* singleNestedStructArrayGroup =
			new tcu::TestCaseGroup(m_testCtx, "single_nested_struct_array", "Nested struct array in one uniform block");
		addChild(singleNestedStructArrayGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string name = std::string(bufferModes[modeNdx].name) + "_" + layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags = layoutFlags[layoutFlagNdx].flags;
					deUint32	flags	 = baseFlags | ((baseFlags & LAYOUT_PACKED) ? DECLARE_FRAGMENT : DECLARE_BOTH);

					if (bufferModes[modeNdx].mode == UniformBlockCase::BUFFERMODE_SINGLE && isArray == 0)
						continue; // Doesn't make sense to add this variant.

					if (isArray)
						name += "_instance_array";

					singleNestedStructArrayGroup->addChild(new BlockSingleNestedStructArrayCase(
						m_context, name.c_str(), "", m_glslVersion, flags, bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.instance_array_basic_type
	{
		tcu::TestCaseGroup* instanceArrayBasicTypeGroup =
			new tcu::TestCaseGroup(m_testCtx, "instance_array_basic_type", "Single basic variable in instance array");
		addChild(instanceArrayBasicTypeGroup);

		for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
		{
			tcu::TestCaseGroup* layoutGroup = new tcu::TestCaseGroup(m_testCtx, layoutFlags[layoutFlagNdx].name, "");
			instanceArrayBasicTypeGroup->addChild(layoutGroup);

			for (int basicTypeNdx = 0; basicTypeNdx < DE_LENGTH_OF_ARRAY(basicTypes); basicTypeNdx++)
			{
				glu::DataType type		   = basicTypes[basicTypeNdx];
				const char*   typeName	 = glu::getDataTypeName(type);
				const int	 numInstances = 3;
				deUint32	  baseFlags	= layoutFlags[layoutFlagNdx].flags;
				deUint32	  flags		   = baseFlags | ((baseFlags & LAYOUT_PACKED) ?
												  (basicTypeNdx % 2 == 0 ? DECLARE_VERTEX : DECLARE_FRAGMENT) :
												  DECLARE_BOTH);

				layoutGroup->addChild(new BlockBasicTypeCase(
					m_context, typeName, "", m_glslVersion,
					VarType(type, glu::isDataTypeBoolOrBVec(type) ? 0 : PRECISION_HIGH), flags, numInstances));

				if (glu::isDataTypeMatrix(type))
				{
					for (int matFlagNdx = 0; matFlagNdx < DE_LENGTH_OF_ARRAY(matrixFlags); matFlagNdx++)
						layoutGroup->addChild(new BlockBasicTypeCase(
							m_context, (string(matrixFlags[matFlagNdx].name) + "_" + typeName).c_str(), "",
							m_glslVersion, VarType(type, PRECISION_HIGH), flags | matrixFlags[matFlagNdx].flags,
							numInstances));
				}
			}
		}
	}

	// ubo.multi_basic_types
	{
		tcu::TestCaseGroup* multiBasicTypesGroup =
			new tcu::TestCaseGroup(m_testCtx, "multi_basic_types", "Multiple buffers with basic types");
		addChild(multiBasicTypesGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			multiBasicTypesGroup->addChild(modeGroup);

			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string baseName  = layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags = layoutFlags[layoutFlagNdx].flags;

					if (isArray)
						baseName += "_instance_array";

					modeGroup->addChild(new BlockMultiBasicTypesCase(
						m_context, (baseName + "_mixed").c_str(), "", m_glslVersion, baseFlags | DECLARE_VERTEX,
						baseFlags | DECLARE_FRAGMENT, bufferModes[modeNdx].mode, isArray ? 3 : 0));

					if (!(baseFlags & LAYOUT_PACKED))
						modeGroup->addChild(new BlockMultiBasicTypesCase(
							m_context, (baseName + "_both").c_str(), "", m_glslVersion,
							baseFlags | DECLARE_VERTEX | DECLARE_FRAGMENT,
							baseFlags | DECLARE_VERTEX | DECLARE_FRAGMENT, bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.multi_nested_struct
	{
		tcu::TestCaseGroup* multiNestedStructGroup =
			new tcu::TestCaseGroup(m_testCtx, "multi_nested_struct", "Multiple buffers with nested structs");
		addChild(multiNestedStructGroup);

		for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(bufferModes); modeNdx++)
		{
			tcu::TestCaseGroup* modeGroup = new tcu::TestCaseGroup(m_testCtx, bufferModes[modeNdx].name, "");
			multiNestedStructGroup->addChild(modeGroup);

			for (int layoutFlagNdx = 0; layoutFlagNdx < DE_LENGTH_OF_ARRAY(layoutFlags); layoutFlagNdx++)
			{
				for (int isArray = 0; isArray < 2; isArray++)
				{
					std::string baseName  = layoutFlags[layoutFlagNdx].name;
					deUint32	baseFlags = layoutFlags[layoutFlagNdx].flags;

					if (isArray)
						baseName += "_instance_array";

					modeGroup->addChild(new BlockMultiNestedStructCase(
						m_context, (baseName + "_mixed").c_str(), "", m_glslVersion, baseFlags | DECLARE_VERTEX,
						baseFlags | DECLARE_FRAGMENT, bufferModes[modeNdx].mode, isArray ? 3 : 0));

					if (!(baseFlags & LAYOUT_PACKED))
						modeGroup->addChild(new BlockMultiNestedStructCase(
							m_context, (baseName + "_both").c_str(), "", m_glslVersion,
							baseFlags | DECLARE_VERTEX | DECLARE_FRAGMENT,
							baseFlags | DECLARE_VERTEX | DECLARE_FRAGMENT, bufferModes[modeNdx].mode, isArray ? 3 : 0));
				}
			}
		}
	}

	// ubo.random
	{
		const deUint32 allShaders	= FEATURE_VERTEX_BLOCKS | FEATURE_FRAGMENT_BLOCKS | FEATURE_SHARED_BLOCKS;
		const deUint32 allLayouts	= FEATURE_PACKED_LAYOUT | FEATURE_SHARED_LAYOUT | FEATURE_STD140_LAYOUT;
		const deUint32 allBasicTypes = FEATURE_VECTORS | FEATURE_MATRICES;
		const deUint32 unused		 = FEATURE_UNUSED_MEMBERS | FEATURE_UNUSED_UNIFORMS;
		const deUint32 matFlags		 = FEATURE_MATRIX_LAYOUT;

		tcu::TestCaseGroup* randomGroup = new tcu::TestCaseGroup(m_testCtx, "random", "Random Uniform Block cases");
		addChild(randomGroup);

		// Basic types.
		createRandomCaseGroup(randomGroup, m_context, "scalar_types", "Scalar types only, per-block buffers",
							  m_glslVersion, UniformBlockCase::BUFFERMODE_PER_BLOCK, allShaders | allLayouts | unused,
							  10, 0);
		createRandomCaseGroup(randomGroup, m_context, "vector_types", "Scalar and vector types only, per-block buffers",
							  m_glslVersion, UniformBlockCase::BUFFERMODE_PER_BLOCK,
							  allShaders | allLayouts | unused | FEATURE_VECTORS, 10, 25);
		createRandomCaseGroup(randomGroup, m_context, "basic_types", "All basic types, per-block buffers",
							  m_glslVersion, UniformBlockCase::BUFFERMODE_PER_BLOCK,
							  allShaders | allLayouts | unused | allBasicTypes | matFlags, 10, 50);
		createRandomCaseGroup(randomGroup, m_context, "basic_arrays", "Arrays, per-block buffers", m_glslVersion,
							  UniformBlockCase::BUFFERMODE_PER_BLOCK,
							  allShaders | allLayouts | unused | allBasicTypes | matFlags | FEATURE_ARRAYS, 10, 50);

		createRandomCaseGroup(
			randomGroup, m_context, "basic_instance_arrays", "Basic instance arrays, per-block buffers", m_glslVersion,
			UniformBlockCase::BUFFERMODE_PER_BLOCK,
			allShaders | allLayouts | unused | allBasicTypes | matFlags | FEATURE_INSTANCE_ARRAYS, 10, 75);
		createRandomCaseGroup(randomGroup, m_context, "nested_structs", "Nested structs, per-block buffers",
							  m_glslVersion, UniformBlockCase::BUFFERMODE_PER_BLOCK,
							  allShaders | allLayouts | unused | allBasicTypes | matFlags | FEATURE_STRUCTS, 10, 100);
		createRandomCaseGroup(
			randomGroup, m_context, "nested_structs_arrays", "Nested structs, arrays, per-block buffers", m_glslVersion,
			UniformBlockCase::BUFFERMODE_PER_BLOCK,
			allShaders | allLayouts | unused | allBasicTypes | matFlags | FEATURE_STRUCTS | FEATURE_ARRAYS, 10, 150);
		createRandomCaseGroup(
			randomGroup, m_context, "nested_structs_instance_arrays",
			"Nested structs, instance arrays, per-block buffers", m_glslVersion, UniformBlockCase::BUFFERMODE_PER_BLOCK,
			allShaders | allLayouts | unused | allBasicTypes | matFlags | FEATURE_STRUCTS | FEATURE_INSTANCE_ARRAYS, 10,
			125);
		createRandomCaseGroup(randomGroup, m_context, "nested_structs_arrays_instance_arrays",
							  "Nested structs, instance arrays, per-block buffers", m_glslVersion,
							  UniformBlockCase::BUFFERMODE_PER_BLOCK,
							  allShaders | allLayouts | unused | allBasicTypes | matFlags | FEATURE_STRUCTS |
								  FEATURE_ARRAYS | FEATURE_INSTANCE_ARRAYS,
							  10, 175);

		createRandomCaseGroup(randomGroup, m_context, "all_per_block_buffers", "All random features, per-block buffers",
							  m_glslVersion, UniformBlockCase::BUFFERMODE_PER_BLOCK, ~0u, 20, 200);
		createRandomCaseGroup(randomGroup, m_context, "all_shared_buffer", "All random features, shared buffer",
							  m_glslVersion, UniformBlockCase::BUFFERMODE_SINGLE, ~0u, 20, 250);
	}

	// ubo.common
	if (glu::isGLSLVersionSupported(m_context.getRenderContext().getType(), glu::GLSL_VERSION_300_ES))
	{
		tcu::TestCaseGroup* commonGroup = new tcu::TestCaseGroup(m_testCtx, "common", "Common Uniform Block cases");
		addChild(commonGroup);
		commonGroup->addChild(new UniformBlockPrecisionMatching(m_context, m_glslVersion));
		commonGroup->addChild(new UniformBlockNameMatching(m_context, m_glslVersion));
	}
	else if (glu::isGLSLVersionSupported(m_context.getRenderContext().getType(), glu::GLSL_VERSION_150))
	{
		tcu::TestCaseGroup* commonGroup = new tcu::TestCaseGroup(m_testCtx, "common", "Common Uniform Block cases");
		addChild(commonGroup);
		commonGroup->addChild(new UniformBlockNameMatching(m_context, m_glslVersion));
	}
}

} // deqp
