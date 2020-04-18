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
 * \brief Synchronization Tests
 *//*--------------------------------------------------------------------*/

#include "es31fSynchronizationTests.hpp"
#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuRenderTarget.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deMemory.h"
#include "deRandom.hpp"

#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static bool validateSortedAtomicRampAdditionValueChain (const std::vector<deUint32>& valueChain, deUint32 sumValue, int& invalidOperationNdx, deUint32& errorDelta, deUint32& errorExpected)
{
	std::vector<deUint32> chainDelta(valueChain.size());

	for (int callNdx = 0; callNdx < (int)valueChain.size(); ++callNdx)
		chainDelta[callNdx] = ((callNdx + 1 == (int)valueChain.size()) ? (sumValue) : (valueChain[callNdx+1])) - valueChain[callNdx];

	// chainDelta contains now the actual additions applied to the value
	// check there exists an addition ramp form 1 to ...
	std::sort(chainDelta.begin(), chainDelta.end());

	for (int callNdx = 0; callNdx < (int)valueChain.size(); ++callNdx)
	{
		if ((int)chainDelta[callNdx] != callNdx+1)
		{
			invalidOperationNdx = callNdx;
			errorDelta = chainDelta[callNdx];
			errorExpected = callNdx+1;

			return false;
		}
	}

	return true;
}

static void readBuffer (const glw::Functions& gl, deUint32 target, int numElements, std::vector<deUint32>& result)
{
	const void* ptr = gl.mapBufferRange(target, 0, (int)(sizeof(deUint32) * numElements), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "map");

	if (!ptr)
		throw tcu::TestError("mapBufferRange returned NULL");

	result.resize(numElements);
	memcpy(&result[0], ptr, sizeof(deUint32) * numElements);

	if (gl.unmapBuffer(target) == GL_FALSE)
		throw tcu::TestError("unmapBuffer returned false");
}

static deUint32 readBufferUint32 (const glw::Functions& gl, deUint32 target)
{
	std::vector<deUint32> vec;

	readBuffer(gl, target, 1, vec);

	return vec[0];
}

//! Generate a ramp of values from 1 to numElements, and shuffle it
void generateShuffledRamp (int numElements, std::vector<int>& ramp)
{
	de::Random rng(0xabcd);

	// some positive (non-zero) unique values
	ramp.resize(numElements);
	for (int callNdx = 0; callNdx < numElements; ++callNdx)
		ramp[callNdx] = callNdx + 1;

	rng.shuffle(ramp.begin(), ramp.end());
}

static std::string specializeShader(Context& context, const char* code)
{
	const glu::GLSLVersion				glslVersion			= glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	std::map<std::string, std::string>	specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (glu::contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		specializationMap["SHADER_IMAGE_ATOMIC_REQUIRE"] = "";
	else
		specializationMap["SHADER_IMAGE_ATOMIC_REQUIRE"] = "#extension GL_OES_shader_image_atomic : require";

	return tcu::StringTemplate(code).specialize(specializationMap);
}

class InterInvocationTestCase : public TestCase
{
public:
	enum StorageType
	{
		STORAGE_BUFFER = 0,
		STORAGE_IMAGE,

		STORAGE_LAST
	};
	enum CaseFlags
	{
		FLAG_ATOMIC				= 0x1,
		FLAG_ALIASING_STORAGES	= 0x2,
		FLAG_IN_GROUP			= 0x4,
	};

						InterInvocationTestCase		(Context& context, const char* name, const char* desc, StorageType storage, int flags = 0);
						~InterInvocationTestCase	(void);

private:
	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);

	void				runCompute					(void);
	bool				verifyResults				(void);
	virtual std::string	genShaderSource				(void) const = 0;

protected:
	std::string			genBarrierSource			(void) const;

	const StorageType	m_storage;
	const bool			m_useAtomic;
	const bool			m_aliasingStorages;
	const bool			m_syncWithGroup;
	const int			m_workWidth;				// !< total work width
	const int			m_workHeight;				// !<     ...    height
	const int			m_localWidth;				// !< group width
	const int			m_localHeight;				// !< group height
	const int			m_elementsPerInvocation;	// !< elements accessed by a single invocation

private:
	glw::GLuint			m_storageBuf;
	glw::GLuint			m_storageTex;
	glw::GLuint			m_resultBuf;
	glu::ShaderProgram*	m_program;
};

InterInvocationTestCase::InterInvocationTestCase (Context& context, const char* name, const char* desc, StorageType storage, int flags)
	: TestCase					(context, name, desc)
	, m_storage					(storage)
	, m_useAtomic				((flags & FLAG_ATOMIC) != 0)
	, m_aliasingStorages		((flags & FLAG_ALIASING_STORAGES) != 0)
	, m_syncWithGroup			((flags & FLAG_IN_GROUP) != 0)
	, m_workWidth				(256)
	, m_workHeight				(256)
	, m_localWidth				(16)
	, m_localHeight				(8)
	, m_elementsPerInvocation	(8)
	, m_storageBuf				(0)
	, m_storageTex				(0)
	, m_resultBuf				(0)
	, m_program					(DE_NULL)
{
	DE_ASSERT(m_storage < STORAGE_LAST);
	DE_ASSERT(m_localWidth*m_localHeight <= 128); // minimum MAX_COMPUTE_WORK_GROUP_INVOCATIONS value
}

InterInvocationTestCase::~InterInvocationTestCase (void)
{
	deinit();
}

void InterInvocationTestCase::init (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// requirements

	if (m_useAtomic && m_storage == STORAGE_IMAGE && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic"))
		throw tcu::NotSupportedError("Test requires GL_OES_shader_image_atomic extension");

	// program

	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genShaderSource()));
	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		throw tcu::TestError("could not build program");

	// source

	if (m_storage == STORAGE_BUFFER)
	{
		const int				bufferElements	= m_workWidth * m_workHeight * m_elementsPerInvocation;
		const int				bufferSize		= bufferElements * (int)sizeof(deUint32);
		std::vector<deUint32>	zeroBuffer		(bufferElements, 0);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating zero-filled buffer for storage, size " << bufferElements << " elements, " << bufferSize << " bytes." << tcu::TestLog::EndMessage;

		gl.genBuffers(1, &m_storageBuf);
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_storageBuf);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, &zeroBuffer[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen storage buf");
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		const int				bufferElements	= m_workWidth * m_workHeight * m_elementsPerInvocation;
		const int				bufferSize		= bufferElements * (int)sizeof(deUint32);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating image for storage, size " << m_workWidth << "x" << m_workHeight * m_elementsPerInvocation << ", " << bufferSize << " bytes." << tcu::TestLog::EndMessage;

		gl.genTextures(1, &m_storageTex);
		gl.bindTexture(GL_TEXTURE_2D, m_storageTex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32I, m_workWidth, m_workHeight * m_elementsPerInvocation);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen storage image");

		// Zero-fill
		m_testCtx.getLog() << tcu::TestLog::Message << "Filling image with 0." << tcu::TestLog::EndMessage;

		{
			const std::vector<deInt32> zeroBuffer(m_workWidth * m_workHeight * m_elementsPerInvocation, 0);
			gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_workWidth, m_workHeight * m_elementsPerInvocation, GL_RED_INTEGER, GL_INT, &zeroBuffer[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "specify image contents");
		}
	}
	else
		DE_ASSERT(DE_FALSE);

	// destination

	{
		const int				bufferElements	= m_workWidth * m_workHeight;
		const int				bufferSize		= bufferElements * (int)sizeof(deUint32);
		std::vector<deInt32>	negativeBuffer	(bufferElements, -1);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating -1 filled buffer for results, size " << bufferElements << " elements, " << bufferSize << " bytes." << tcu::TestLog::EndMessage;

		gl.genBuffers(1, &m_resultBuf);
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_resultBuf);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, &negativeBuffer[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen storage buf");
	}
}

void InterInvocationTestCase::deinit (void)
{
	if (m_storageBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_storageBuf);
		m_storageBuf = DE_NULL;
	}

	if (m_storageTex)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_storageTex);
		m_storageTex = DE_NULL;
	}

	if (m_resultBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_resultBuf);
		m_resultBuf = DE_NULL;
	}

	delete m_program;
	m_program = DE_NULL;
}

InterInvocationTestCase::IterateResult InterInvocationTestCase::iterate (void)
{
	// Dispatch
	runCompute();

	// Verify buffer contents
	if (verifyResults())
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, (std::string((m_storage == STORAGE_BUFFER) ? ("buffer") : ("image")) + " content verification failed").c_str());

	return STOP;
}

void InterInvocationTestCase::runCompute (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	const int				groupsX	= m_workWidth / m_localWidth;
	const int				groupsY	= m_workHeight / m_localHeight;

	DE_ASSERT((m_workWidth % m_localWidth) == 0);
	DE_ASSERT((m_workHeight % m_localHeight) == 0);

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Dispatching compute.\n"
		<< "	group size: " << m_localWidth << "x" << m_localHeight << "\n"
		<< "	dispatch size: " << groupsX << "x" << groupsY << "\n"
		<< "	total work size: " << m_workWidth << "x" << m_workHeight << "\n"
		<< tcu::TestLog::EndMessage;

	gl.useProgram(m_program->getProgram());

	// source
	if (m_storage == STORAGE_BUFFER && !m_aliasingStorages)
	{
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storageBuf);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind source buf");
	}
	else if (m_storage == STORAGE_BUFFER && m_aliasingStorages)
	{
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storageBuf);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storageBuf);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind source buf");

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding same buffer object to buffer storages." << tcu::TestLog::EndMessage;
	}
	else if (m_storage == STORAGE_IMAGE && !m_aliasingStorages)
	{
		gl.bindImageTexture(1, m_storageTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind result buf");
	}
	else if (m_storage == STORAGE_IMAGE && m_aliasingStorages)
	{
		gl.bindImageTexture(1, m_storageTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		gl.bindImageTexture(2, m_storageTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

		GLU_EXPECT_NO_ERROR(gl.getError(), "bind result buf");

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding same texture level to image storages." << tcu::TestLog::EndMessage;
	}
	else
		DE_ASSERT(DE_FALSE);

	// destination
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_resultBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind result buf");

	// dispatch
	gl.dispatchCompute(groupsX, groupsY, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatchCompute");
}

bool InterInvocationTestCase::verifyResults (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const int				errorFloodThreshold	= 5;
	int						numErrorsLogged		= 0;
	const void*				mapped				= DE_NULL;
	std::vector<deInt32>	results				(m_workWidth * m_workHeight);
	bool					error				= false;

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_resultBuf);
	mapped = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_workWidth * m_workHeight * sizeof(deInt32), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "map buffer");

	// copy to properly aligned array
	deMemcpy(&results[0], mapped, m_workWidth * m_workHeight * sizeof(deUint32));

	if (gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER) != GL_TRUE)
		throw tcu::TestError("memory map store corrupted");

	// check the results
	for (int ndx = 0; ndx < (int)results.size(); ++ndx)
	{
		if (results[ndx] != 1)
		{
			error = true;

			if (numErrorsLogged == 0)
				m_testCtx.getLog() << tcu::TestLog::Message << "Result buffer failed, got unexpected values.\n" << tcu::TestLog::EndMessage;
			if (numErrorsLogged++ < errorFloodThreshold)
				m_testCtx.getLog() << tcu::TestLog::Message << "	Error at index " << ndx << ": expected 1, got " << results[ndx] << ".\n" << tcu::TestLog::EndMessage;
			else
			{
				// after N errors, no point continuing verification
				m_testCtx.getLog() << tcu::TestLog::Message << "	-- too many errors, skipping verification --\n" << tcu::TestLog::EndMessage;
				break;
			}
		}
	}

	if (!error)
		m_testCtx.getLog() << tcu::TestLog::Message << "Result buffer ok." << tcu::TestLog::EndMessage;
	return !error;
}

std::string InterInvocationTestCase::genBarrierSource (void) const
{
	std::ostringstream buf;

	if (m_syncWithGroup)
	{
		// Wait until all invocations in this work group have their texture/buffer read/write operations complete
		// \note We could also use memoryBarrierBuffer() or memoryBarrierImage() in place of groupMemoryBarrier() but
		//       we only require intra-workgroup synchronization.
		buf << "\n"
			<< "	groupMemoryBarrier();\n"
			<< "	barrier();\n"
			<< "\n";
	}
	else if (m_storage == STORAGE_BUFFER)
	{
		DE_ASSERT(!m_syncWithGroup);

		// Waiting only for data written by this invocation. Since all buffer reads and writes are
		// processed in order (within a single invocation), we don't have to do anything.
		buf << "\n";
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		DE_ASSERT(!m_syncWithGroup);

		// Waiting only for data written by this invocation. But since operations complete in undefined
		// order, we have to wait for them to complete.
		buf << "\n"
			<< "	memoryBarrierImage();\n"
			<< "\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	return buf.str();
}

class InvocationBasicCase : public InterInvocationTestCase
{
public:
							InvocationBasicCase		(Context& context, const char* name, const char* desc, StorageType storage, int flags);
private:
	std::string				genShaderSource			(void) const;
	virtual std::string		genShaderMainBlock		(void) const = 0;
};

InvocationBasicCase::InvocationBasicCase (Context& context, const char* name, const char* desc, StorageType storage, int flags)
	: InterInvocationTestCase(context, name, desc, storage, flags)
{
}

std::string InvocationBasicCase::genShaderSource (void) const
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x=" << m_localWidth << ", local_size_y=" << m_localHeight << ") in;\n"
		<< "layout(binding=0, std430) buffer Output\n"
		<< "{\n"
		<< "	highp int values[];\n"
		<< "} sb_result;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=1, std430) coherent buffer Storage\n"
			<< "{\n"
			<< "	highp int values[];\n"
			<< "} sb_store;\n"
			<< "\n"
			<< "highp int getIndex (in highp uvec2 localID, in highp int element)\n"
			<< "{\n"
			<< "	highp uint groupNdx = gl_NumWorkGroups.x * gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
			<< "	return int((localID.y * gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x) + (groupNdx * gl_WorkGroupSize.x) + localID.x) * " << m_elementsPerInvocation << " + element;\n"
			<< "}\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(r32i, binding=1) coherent uniform highp iimage2D u_image;\n"
			<< "\n"
			<< "highp ivec2 getCoord (in highp uvec2 localID, in highp int element)\n"
			<< "{\n"
			<< "	return ivec2(int(gl_WorkGroupID.x * gl_WorkGroupSize.x + localID.x), int(gl_WorkGroupID.y * gl_WorkGroupSize.y + localID.y) + element * " << m_workHeight << ");\n"
			<< "}\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	int resultNdx   = int(gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x);\n"
		<< "	int groupNdx    = int(gl_NumWorkGroups.x * gl_WorkGroupID.y + gl_WorkGroupID.x);\n"
		<< "	bool allOk      = true;\n"
		<< "\n"
		<< genShaderMainBlock()
		<< "\n"
		<< "	sb_result.values[resultNdx] = (allOk) ? (1) : (0);\n"
		<< "}\n";

	return specializeShader(m_context, buf.str().c_str());
}

class InvocationWriteReadCase : public InvocationBasicCase
{
public:
					InvocationWriteReadCase		(Context& context, const char* name, const char* desc, StorageType storage, int flags);
private:
	std::string		genShaderMainBlock			(void) const;
};

InvocationWriteReadCase::InvocationWriteReadCase (Context& context, const char* name, const char* desc, StorageType storage, int flags)
	: InvocationBasicCase(context, name, desc, storage, flags)
{
}

std::string InvocationWriteReadCase::genShaderMainBlock (void) const
{
	std::ostringstream buf;

	// write

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tatomicAdd(sb_store.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")], groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tsb_store.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")] = groupNdx;\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\timageAtomicAdd(u_image, getCoord(gl_LocalInvocationID.xy, " << ndx << "), int(groupNdx));\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\timageStore(u_image, getCoord(gl_LocalInvocationID.xy, " << ndx << "), ivec4(int(groupNdx), 0, 0, 0));\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	// barrier

	buf << genBarrierSource();

	// read

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		const std::string localID = (m_syncWithGroup) ? ("(gl_LocalInvocationID.xy + uvec2(" + de::toString(ndx+1) + ", " + de::toString(2*ndx) + ")) % gl_WorkGroupSize.xy") : ("gl_LocalInvocationID.xy");

		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tallOk = allOk && (atomicExchange(sb_store.values[getIndex(" << localID << ", " << ndx << ")], 0) == groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tallOk = allOk && (sb_store.values[getIndex(" << localID << ", " << ndx << ")] == groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\tallOk = allOk && (imageAtomicExchange(u_image, getCoord(" << localID << ", " << ndx << "), 0) == groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\tallOk = allOk && (imageLoad(u_image, getCoord(" << localID << ", " << ndx << ")).x == groupNdx);\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	return buf.str();
}

class InvocationReadWriteCase : public InvocationBasicCase
{
public:
					InvocationReadWriteCase		(Context& context, const char* name, const char* desc, StorageType storage, int flags);
private:
	std::string		genShaderMainBlock			(void) const;
};

InvocationReadWriteCase::InvocationReadWriteCase (Context& context, const char* name, const char* desc, StorageType storage, int flags)
	: InvocationBasicCase(context, name, desc, storage, flags)
{
}

std::string InvocationReadWriteCase::genShaderMainBlock (void) const
{
	std::ostringstream buf;

	// read

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		const std::string localID = (m_syncWithGroup) ? ("(gl_LocalInvocationID.xy + uvec2(" + de::toString(ndx+1) + ", " + de::toString(2*ndx) + ")) % gl_WorkGroupSize.xy") : ("gl_LocalInvocationID.xy");

		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tallOk = allOk && (atomicExchange(sb_store.values[getIndex(" << localID << ", " << ndx << ")], 123) == 0);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tallOk = allOk && (sb_store.values[getIndex(" << localID << ", " << ndx << ")] == 0);\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\tallOk = allOk && (imageAtomicExchange(u_image, getCoord(" << localID << ", " << ndx << "), 123) == 0);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\tallOk = allOk && (imageLoad(u_image, getCoord(" << localID << ", " << ndx << ")).x == 0);\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	// barrier

	buf << genBarrierSource();

	// write

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tatomicAdd(sb_store.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")], groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tsb_store.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")] = groupNdx;\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\timageAtomicAdd(u_image, getCoord(gl_LocalInvocationID.xy, " << ndx << "), int(groupNdx));\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\timageStore(u_image, getCoord(gl_LocalInvocationID.xy, " << ndx << "), ivec4(int(groupNdx), 0, 0, 0));\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	return buf.str();
}

class InvocationOverWriteCase : public InvocationBasicCase
{
public:
					InvocationOverWriteCase		(Context& context, const char* name, const char* desc, StorageType storage, int flags);
private:
	std::string		genShaderMainBlock			(void) const;
};

InvocationOverWriteCase::InvocationOverWriteCase (Context& context, const char* name, const char* desc, StorageType storage, int flags)
	: InvocationBasicCase(context, name, desc, storage, flags)
{
}

std::string InvocationOverWriteCase::genShaderMainBlock (void) const
{
	std::ostringstream buf;

	// write

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tatomicAdd(sb_store.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")], 456);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tsb_store.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")] = 456;\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\timageAtomicAdd(u_image, getCoord(gl_LocalInvocationID.xy, " << ndx << "), 456);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\timageStore(u_image, getCoord(gl_LocalInvocationID.xy, " << ndx << "), ivec4(456, 0, 0, 0));\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	// barrier

	buf << genBarrierSource();

	// write over

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		// write another invocation's value or our own value depending on test type
		const std::string localID = (m_syncWithGroup) ? ("(gl_LocalInvocationID.xy + uvec2(" + de::toString(ndx+4) + ", " + de::toString(3*ndx) + ")) % gl_WorkGroupSize.xy") : ("gl_LocalInvocationID.xy");

		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tatomicExchange(sb_store.values[getIndex(" << localID << ", " << ndx << ")], groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tsb_store.values[getIndex(" << localID << ", " << ndx << ")] = groupNdx;\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\timageAtomicExchange(u_image, getCoord(" << localID << ", " << ndx << "), groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\timageStore(u_image, getCoord(" << localID << ", " << ndx << "), ivec4(groupNdx, 0, 0, 0));\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	// barrier

	buf << genBarrierSource();

	// read

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		// check another invocation's value or our own value depending on test type
		const std::string localID = (m_syncWithGroup) ? ("(gl_LocalInvocationID.xy + uvec2(" + de::toString(ndx+1) + ", " + de::toString(2*ndx) + ")) % gl_WorkGroupSize.xy") : ("gl_LocalInvocationID.xy");

		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tallOk = allOk && (atomicExchange(sb_store.values[getIndex(" << localID << ", " << ndx << ")], 123) == groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tallOk = allOk && (sb_store.values[getIndex(" << localID << ", " << ndx << ")] == groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\tallOk = allOk && (imageAtomicExchange(u_image, getCoord(" << localID << ", " << ndx << "), 123) == groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\tallOk = allOk && (imageLoad(u_image, getCoord(" << localID << ", " << ndx << ")).x == groupNdx);\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	return buf.str();
}

class InvocationAliasWriteCase : public InterInvocationTestCase
{
public:
	enum TestType
	{
		TYPE_WRITE = 0,
		TYPE_OVERWRITE,

		TYPE_LAST
	};

					InvocationAliasWriteCase	(Context& context, const char* name, const char* desc, TestType type, StorageType storage, int flags);
private:
	std::string		genShaderSource				(void) const;

	const TestType	m_type;
};

InvocationAliasWriteCase::InvocationAliasWriteCase (Context& context, const char* name, const char* desc, TestType type, StorageType storage, int flags)
	: InterInvocationTestCase	(context, name, desc, storage, flags | FLAG_ALIASING_STORAGES)
	, m_type					(type)
{
	DE_ASSERT(type < TYPE_LAST);
}

std::string InvocationAliasWriteCase::genShaderSource (void) const
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x=" << m_localWidth << ", local_size_y=" << m_localHeight << ") in;\n"
		<< "layout(binding=0, std430) buffer Output\n"
		<< "{\n"
		<< "	highp int values[];\n"
		<< "} sb_result;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=1, std430) coherent buffer Storage0\n"
			<< "{\n"
			<< "	highp int values[];\n"
			<< "} sb_store0;\n"
			<< "layout(binding=2, std430) coherent buffer Storage1\n"
			<< "{\n"
			<< "	highp int values[];\n"
			<< "} sb_store1;\n"
			<< "\n"
			<< "highp int getIndex (in highp uvec2 localID, in highp int element)\n"
			<< "{\n"
			<< "	highp uint groupNdx = gl_NumWorkGroups.x * gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
			<< "	return int((localID.y * gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x) + (groupNdx * gl_WorkGroupSize.x) + localID.x) * " << m_elementsPerInvocation << " + element;\n"
			<< "}\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(r32i, binding=1) coherent uniform highp iimage2D u_image0;\n"
			<< "layout(r32i, binding=2) coherent uniform highp iimage2D u_image1;\n"
			<< "\n"
			<< "highp ivec2 getCoord (in highp uvec2 localID, in highp int element)\n"
			<< "{\n"
			<< "	return ivec2(int(gl_WorkGroupID.x * gl_WorkGroupSize.x + localID.x), int(gl_WorkGroupID.y * gl_WorkGroupSize.y + localID.y) + element * " << m_workHeight << ");\n"
			<< "}\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	int resultNdx   = int(gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x);\n"
		<< "	int groupNdx    = int(gl_NumWorkGroups.x * gl_WorkGroupID.y + gl_WorkGroupID.x);\n"
		<< "	bool allOk      = true;\n"
		<< "\n";

	if (m_type == TYPE_OVERWRITE)
	{
		// write

		for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
		{
			if (m_storage == STORAGE_BUFFER && m_useAtomic)
				buf << "\tatomicAdd(sb_store0.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")], 456);\n";
			else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
				buf << "\tsb_store0.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")] = 456;\n";
			else if (m_storage == STORAGE_IMAGE && m_useAtomic)
				buf << "\timageAtomicAdd(u_image0, getCoord(gl_LocalInvocationID.xy, " << ndx << "), 456);\n";
			else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
				buf << "\timageStore(u_image0, getCoord(gl_LocalInvocationID.xy, " << ndx << "), ivec4(456, 0, 0, 0));\n";
			else
				DE_ASSERT(DE_FALSE);
		}

		// barrier

		buf << genBarrierSource();
	}
	else
		DE_ASSERT(m_type == TYPE_WRITE);

	// write (again)

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		const std::string localID = (m_syncWithGroup) ? ("(gl_LocalInvocationID.xy + uvec2(" + de::toString(ndx+2) + ", " + de::toString(2*ndx) + ")) % gl_WorkGroupSize.xy") : ("gl_LocalInvocationID.xy");

		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tatomicExchange(sb_store1.values[getIndex(" << localID << ", " << ndx << ")], groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tsb_store1.values[getIndex(" << localID << ", " << ndx << ")] = groupNdx;\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\timageAtomicExchange(u_image1, getCoord(" << localID << ", " << ndx << "), groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\timageStore(u_image1, getCoord(" << localID << ", " << ndx << "), ivec4(groupNdx, 0, 0, 0));\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	// barrier

	buf << genBarrierSource();

	// read

	for (int ndx = 0; ndx < m_elementsPerInvocation; ++ndx)
	{
		if (m_storage == STORAGE_BUFFER && m_useAtomic)
			buf << "\tallOk = allOk && (atomicExchange(sb_store0.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")], 123) == groupNdx);\n";
		else if (m_storage == STORAGE_BUFFER && !m_useAtomic)
			buf << "\tallOk = allOk && (sb_store0.values[getIndex(gl_LocalInvocationID.xy, " << ndx << ")] == groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && m_useAtomic)
			buf << "\tallOk = allOk && (imageAtomicExchange(u_image0, getCoord(gl_LocalInvocationID.xy, " << ndx << "), 123) == groupNdx);\n";
		else if (m_storage == STORAGE_IMAGE && !m_useAtomic)
			buf << "\tallOk = allOk && (imageLoad(u_image0, getCoord(gl_LocalInvocationID.xy, " << ndx << ")).x == groupNdx);\n";
		else
			DE_ASSERT(DE_FALSE);
	}

	// return result

	buf << "\n"
		<< "	sb_result.values[resultNdx] = (allOk) ? (1) : (0);\n"
		<< "}\n";

	return specializeShader(m_context, buf.str().c_str());
}

namespace op
{

struct WriteData
{
	int targetHandle;
	int seed;

	static WriteData Generate(int targetHandle, int seed)
	{
		WriteData retVal;

		retVal.targetHandle = targetHandle;
		retVal.seed = seed;

		return retVal;
	}
};

struct ReadData
{
	int targetHandle;
	int seed;

	static ReadData Generate(int targetHandle, int seed)
	{
		ReadData retVal;

		retVal.targetHandle = targetHandle;
		retVal.seed = seed;

		return retVal;
	}
};

struct Barrier
{
};

struct WriteDataInterleaved
{
	int		targetHandle;
	int		seed;
	bool	evenOdd;

	static WriteDataInterleaved Generate(int targetHandle, int seed, bool evenOdd)
	{
		WriteDataInterleaved retVal;

		retVal.targetHandle = targetHandle;
		retVal.seed = seed;
		retVal.evenOdd = evenOdd;

		return retVal;
	}
};

struct ReadDataInterleaved
{
	int targetHandle;
	int seed0;
	int seed1;

	static ReadDataInterleaved Generate(int targetHandle, int seed0, int seed1)
	{
		ReadDataInterleaved retVal;

		retVal.targetHandle = targetHandle;
		retVal.seed0 = seed0;
		retVal.seed1 = seed1;

		return retVal;
	}
};

struct ReadMultipleData
{
	int targetHandle0;
	int seed0;
	int targetHandle1;
	int seed1;

	static ReadMultipleData Generate(int targetHandle0, int seed0, int targetHandle1, int seed1)
	{
		ReadMultipleData retVal;

		retVal.targetHandle0 = targetHandle0;
		retVal.seed0 = seed0;
		retVal.targetHandle1 = targetHandle1;
		retVal.seed1 = seed1;

		return retVal;
	}
};

struct ReadZeroData
{
	int targetHandle;

	static ReadZeroData Generate(int targetHandle)
	{
		ReadZeroData retVal;

		retVal.targetHandle = targetHandle;

		return retVal;
	}
};

} // namespace op

class InterCallTestCase;

class InterCallOperations
{
public:
	InterCallOperations& operator<< (const op::WriteData&);
	InterCallOperations& operator<< (const op::ReadData&);
	InterCallOperations& operator<< (const op::Barrier&);
	InterCallOperations& operator<< (const op::ReadMultipleData&);
	InterCallOperations& operator<< (const op::WriteDataInterleaved&);
	InterCallOperations& operator<< (const op::ReadDataInterleaved&);
	InterCallOperations& operator<< (const op::ReadZeroData&);

private:
	struct Command
	{
		enum CommandType
		{
			TYPE_WRITE = 0,
			TYPE_READ,
			TYPE_BARRIER,
			TYPE_READ_MULTIPLE,
			TYPE_WRITE_INTERLEAVE,
			TYPE_READ_INTERLEAVE,
			TYPE_READ_ZERO,

			TYPE_LAST
		};

		CommandType type;

		union CommandUnion
		{
			op::WriteData				write;
			op::ReadData				read;
			op::Barrier					barrier;
			op::ReadMultipleData		readMulti;
			op::WriteDataInterleaved	writeInterleave;
			op::ReadDataInterleaved		readInterleave;
			op::ReadZeroData			readZero;
		} u_cmd;
	};

	friend class InterCallTestCase;

	std::vector<Command> m_cmds;
};

InterCallOperations& InterCallOperations::operator<< (const op::WriteData& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_WRITE;
	m_cmds.back().u_cmd.write = cmd;

	return *this;
}

InterCallOperations& InterCallOperations::operator<< (const op::ReadData& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_READ;
	m_cmds.back().u_cmd.read = cmd;

	return *this;
}

InterCallOperations& InterCallOperations::operator<< (const op::Barrier& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_BARRIER;
	m_cmds.back().u_cmd.barrier = cmd;

	return *this;
}

InterCallOperations& InterCallOperations::operator<< (const op::ReadMultipleData& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_READ_MULTIPLE;
	m_cmds.back().u_cmd.readMulti = cmd;

	return *this;
}

InterCallOperations& InterCallOperations::operator<< (const op::WriteDataInterleaved& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_WRITE_INTERLEAVE;
	m_cmds.back().u_cmd.writeInterleave = cmd;

	return *this;
}

InterCallOperations& InterCallOperations::operator<< (const op::ReadDataInterleaved& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_READ_INTERLEAVE;
	m_cmds.back().u_cmd.readInterleave = cmd;

	return *this;
}

InterCallOperations& InterCallOperations::operator<< (const op::ReadZeroData& cmd)
{
	m_cmds.push_back(Command());
	m_cmds.back().type = Command::TYPE_READ_ZERO;
	m_cmds.back().u_cmd.readZero = cmd;

	return *this;
}

class InterCallTestCase : public TestCase
{
public:
	enum StorageType
	{
		STORAGE_BUFFER = 0,
		STORAGE_IMAGE,

		STORAGE_LAST
	};
	enum Flags
	{
		FLAG_USE_ATOMIC	= 1,
		FLAG_USE_INT	= 2,
	};
													InterCallTestCase			(Context& context, const char* name, const char* desc, StorageType storage, int flags, const InterCallOperations& ops);
													~InterCallTestCase			(void);

private:
	void											init						(void);
	void											deinit						(void);
	IterateResult									iterate						(void);
	bool											verifyResults				(void);

	void											runCommand					(const op::WriteData& cmd, int stepNdx, int& programFriendlyName);
	void											runCommand					(const op::ReadData& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName);
	void											runCommand					(const op::Barrier&);
	void											runCommand					(const op::ReadMultipleData& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName);
	void											runCommand					(const op::WriteDataInterleaved& cmd, int stepNdx, int& programFriendlyName);
	void											runCommand					(const op::ReadDataInterleaved& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName);
	void											runCommand					(const op::ReadZeroData& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName);
	void											runSingleRead				(int targetHandle, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName);

	glw::GLuint										genStorage					(int friendlyName);
	glw::GLuint										genResultStorage			(void);
	glu::ShaderProgram*								genWriteProgram				(int seed);
	glu::ShaderProgram*								genReadProgram				(int seed);
	glu::ShaderProgram*								genReadMultipleProgram		(int seed0, int seed1);
	glu::ShaderProgram*								genWriteInterleavedProgram	(int seed, bool evenOdd);
	glu::ShaderProgram*								genReadInterleavedProgram	(int seed0, int seed1);
	glu::ShaderProgram*								genReadZeroProgram			(void);

	const StorageType								m_storage;
	const int										m_invocationGridSize;	// !< width and height of the two dimensional work dispatch
	const int										m_perInvocationSize;	// !< number of elements accessed in single invocation
	const std::vector<InterCallOperations::Command>	m_cmds;
	const bool										m_useAtomic;
	const bool										m_formatInteger;

	std::vector<glu::ShaderProgram*>				m_operationPrograms;
	std::vector<glw::GLuint>						m_operationResultStorages;
	std::map<int, glw::GLuint>						m_storageIDs;
};

InterCallTestCase::InterCallTestCase (Context& context, const char* name, const char* desc, StorageType storage, int flags, const InterCallOperations& ops)
	: TestCase					(context, name, desc)
	, m_storage					(storage)
	, m_invocationGridSize		(512)
	, m_perInvocationSize		(2)
	, m_cmds					(ops.m_cmds)
	, m_useAtomic				((flags & FLAG_USE_ATOMIC) != 0)
	, m_formatInteger			((flags & FLAG_USE_INT) != 0)
{
}

InterCallTestCase::~InterCallTestCase (void)
{
	deinit();
}

void InterCallTestCase::init (void)
{
	int			programFriendlyName = 0;
	const bool	supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// requirements

	if (m_useAtomic && m_storage == STORAGE_IMAGE && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic"))
		throw tcu::NotSupportedError("Test requires GL_OES_shader_image_atomic extension");

	// generate resources and validate command list

	m_operationPrograms.resize(m_cmds.size(), DE_NULL);
	m_operationResultStorages.resize(m_cmds.size(), 0);

	for (int step = 0; step < (int)m_cmds.size(); ++step)
	{
		switch (m_cmds[step].type)
		{
			case InterCallOperations::Command::TYPE_WRITE:
			{
				const op::WriteData& cmd = m_cmds[step].u_cmd.write;

				// new storage handle?
				if (m_storageIDs.find(cmd.targetHandle) == m_storageIDs.end())
					m_storageIDs[cmd.targetHandle] = genStorage(cmd.targetHandle);

				// program
				{
					glu::ShaderProgram* program = genWriteProgram(cmd.seed);

					m_testCtx.getLog() << tcu::TestLog::Message << "Program #" << ++programFriendlyName << tcu::TestLog::EndMessage;
					m_testCtx.getLog() << *program;

					if (!program->isOk())
						throw tcu::TestError("could not build program");

					m_operationPrograms[step] = program;
				}
				break;
			}

			case InterCallOperations::Command::TYPE_READ:
			{
				const op::ReadData& cmd = m_cmds[step].u_cmd.read;
				DE_ASSERT(m_storageIDs.find(cmd.targetHandle) != m_storageIDs.end());

				// program and result storage
				{
					glu::ShaderProgram* program = genReadProgram(cmd.seed);

					m_testCtx.getLog() << tcu::TestLog::Message << "Program #" << ++programFriendlyName << tcu::TestLog::EndMessage;
					m_testCtx.getLog() << *program;

					if (!program->isOk())
						throw tcu::TestError("could not build program");

					m_operationPrograms[step] = program;
					m_operationResultStorages[step] = genResultStorage();
				}
				break;
			}

			case InterCallOperations::Command::TYPE_BARRIER:
			{
				break;
			}

			case InterCallOperations::Command::TYPE_READ_MULTIPLE:
			{
				const op::ReadMultipleData& cmd = m_cmds[step].u_cmd.readMulti;
				DE_ASSERT(m_storageIDs.find(cmd.targetHandle0) != m_storageIDs.end());
				DE_ASSERT(m_storageIDs.find(cmd.targetHandle1) != m_storageIDs.end());

				// program
				{
					glu::ShaderProgram* program = genReadMultipleProgram(cmd.seed0, cmd.seed1);

					m_testCtx.getLog() << tcu::TestLog::Message << "Program #" << ++programFriendlyName << tcu::TestLog::EndMessage;
					m_testCtx.getLog() << *program;

					if (!program->isOk())
						throw tcu::TestError("could not build program");

					m_operationPrograms[step] = program;
					m_operationResultStorages[step] = genResultStorage();
				}
				break;
			}

			case InterCallOperations::Command::TYPE_WRITE_INTERLEAVE:
			{
				const op::WriteDataInterleaved& cmd = m_cmds[step].u_cmd.writeInterleave;

				// new storage handle?
				if (m_storageIDs.find(cmd.targetHandle) == m_storageIDs.end())
					m_storageIDs[cmd.targetHandle] = genStorage(cmd.targetHandle);

				// program
				{
					glu::ShaderProgram* program = genWriteInterleavedProgram(cmd.seed, cmd.evenOdd);

					m_testCtx.getLog() << tcu::TestLog::Message << "Program #" << ++programFriendlyName << tcu::TestLog::EndMessage;
					m_testCtx.getLog() << *program;

					if (!program->isOk())
						throw tcu::TestError("could not build program");

					m_operationPrograms[step] = program;
				}
				break;
			}

			case InterCallOperations::Command::TYPE_READ_INTERLEAVE:
			{
				const op::ReadDataInterleaved& cmd = m_cmds[step].u_cmd.readInterleave;
				DE_ASSERT(m_storageIDs.find(cmd.targetHandle) != m_storageIDs.end());

				// program
				{
					glu::ShaderProgram* program = genReadInterleavedProgram(cmd.seed0, cmd.seed1);

					m_testCtx.getLog() << tcu::TestLog::Message << "Program #" << ++programFriendlyName << tcu::TestLog::EndMessage;
					m_testCtx.getLog() << *program;

					if (!program->isOk())
						throw tcu::TestError("could not build program");

					m_operationPrograms[step] = program;
					m_operationResultStorages[step] = genResultStorage();
				}
				break;
			}

			case InterCallOperations::Command::TYPE_READ_ZERO:
			{
				const op::ReadZeroData& cmd = m_cmds[step].u_cmd.readZero;

				// new storage handle?
				if (m_storageIDs.find(cmd.targetHandle) == m_storageIDs.end())
					m_storageIDs[cmd.targetHandle] = genStorage(cmd.targetHandle);

				// program
				{
					glu::ShaderProgram* program = genReadZeroProgram();

					m_testCtx.getLog() << tcu::TestLog::Message << "Program #" << ++programFriendlyName << tcu::TestLog::EndMessage;
					m_testCtx.getLog() << *program;

					if (!program->isOk())
						throw tcu::TestError("could not build program");

					m_operationPrograms[step] = program;
					m_operationResultStorages[step] = genResultStorage();
				}
				break;
			}

			default:
				DE_ASSERT(DE_FALSE);
		}
	}
}

void InterCallTestCase::deinit (void)
{
	// programs
	for (int ndx = 0; ndx < (int)m_operationPrograms.size(); ++ndx)
		delete m_operationPrograms[ndx];
	m_operationPrograms.clear();

	// result storages
	for (int ndx = 0; ndx < (int)m_operationResultStorages.size(); ++ndx)
	{
		if (m_operationResultStorages[ndx])
			m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_operationResultStorages[ndx]);
	}
	m_operationResultStorages.clear();

	// storage
	for (std::map<int, glw::GLuint>::const_iterator it = m_storageIDs.begin(); it != m_storageIDs.end(); ++it)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (m_storage == STORAGE_BUFFER)
			gl.deleteBuffers(1, &it->second);
		else if (m_storage == STORAGE_IMAGE)
			gl.deleteTextures(1, &it->second);
		else
			DE_ASSERT(DE_FALSE);
	}
	m_storageIDs.clear();
}

InterCallTestCase::IterateResult InterCallTestCase::iterate (void)
{
	int programFriendlyName			= 0;
	int resultStorageFriendlyName	= 0;

	m_testCtx.getLog() << tcu::TestLog::Message << "Running operations:" << tcu::TestLog::EndMessage;

	// run steps

	for (int step = 0; step < (int)m_cmds.size(); ++step)
	{
		switch (m_cmds[step].type)
		{
			case InterCallOperations::Command::TYPE_WRITE:				runCommand(m_cmds[step].u_cmd.write,			step,	programFriendlyName);								break;
			case InterCallOperations::Command::TYPE_READ:				runCommand(m_cmds[step].u_cmd.read,				step,	programFriendlyName, resultStorageFriendlyName);	break;
			case InterCallOperations::Command::TYPE_BARRIER:			runCommand(m_cmds[step].u_cmd.barrier);																		break;
			case InterCallOperations::Command::TYPE_READ_MULTIPLE:		runCommand(m_cmds[step].u_cmd.readMulti,		step,	programFriendlyName, resultStorageFriendlyName);	break;
			case InterCallOperations::Command::TYPE_WRITE_INTERLEAVE:	runCommand(m_cmds[step].u_cmd.writeInterleave,	step,	programFriendlyName);								break;
			case InterCallOperations::Command::TYPE_READ_INTERLEAVE:	runCommand(m_cmds[step].u_cmd.readInterleave,	step,	programFriendlyName, resultStorageFriendlyName);	break;
			case InterCallOperations::Command::TYPE_READ_ZERO:			runCommand(m_cmds[step].u_cmd.readZero,			step,	programFriendlyName, resultStorageFriendlyName);	break;
			default:
				DE_ASSERT(DE_FALSE);
		}
	}

	// read results from result buffers
	if (verifyResults())
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, (std::string((m_storage == STORAGE_BUFFER) ? ("buffer") : ("image")) + " content verification failed").c_str());

	return STOP;
}

bool InterCallTestCase::verifyResults (void)
{
	int		resultBufferFriendlyName	= 0;
	bool	allResultsOk				= true;
	bool	anyResult					= false;

	m_testCtx.getLog() << tcu::TestLog::Message << "Reading verifier program results" << tcu::TestLog::EndMessage;

	for (int step = 0; step < (int)m_cmds.size(); ++step)
	{
		const int	errorFloodThreshold	= 5;
		int			numErrorsLogged		= 0;

		if (m_operationResultStorages[step])
		{
			const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
			const void*				mapped	= DE_NULL;
			std::vector<deInt32>	results	(m_invocationGridSize * m_invocationGridSize);
			bool					error	= false;

			anyResult = true;

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_operationResultStorages[step]);
			mapped = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_invocationGridSize * m_invocationGridSize * sizeof(deUint32), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "map buffer");

			// copy to properly aligned array
			deMemcpy(&results[0], mapped, m_invocationGridSize * m_invocationGridSize * sizeof(deUint32));

			if (gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER) != GL_TRUE)
				throw tcu::TestError("memory map store corrupted");

			// check the results
			for (int ndx = 0; ndx < (int)results.size(); ++ndx)
			{
				if (results[ndx] != 1)
				{
					error = true;

					if (numErrorsLogged == 0)
						m_testCtx.getLog() << tcu::TestLog::Message << "Result storage #" << ++resultBufferFriendlyName << " failed, got unexpected values.\n" << tcu::TestLog::EndMessage;
					if (numErrorsLogged++ < errorFloodThreshold)
						m_testCtx.getLog() << tcu::TestLog::Message << "	Error at index " << ndx << ": expected 1, got " << results[ndx] << ".\n" << tcu::TestLog::EndMessage;
					else
					{
						// after N errors, no point continuing verification
						m_testCtx.getLog() << tcu::TestLog::Message << "	-- too many errors, skipping verification --\n" << tcu::TestLog::EndMessage;
						break;
					}
				}
			}

			if (error)
			{
				allResultsOk = false;
			}
			else
				m_testCtx.getLog() << tcu::TestLog::Message << "Result storage #" << ++resultBufferFriendlyName << " ok." << tcu::TestLog::EndMessage;
		}
	}

	DE_ASSERT(anyResult);
	DE_UNREF(anyResult);

	return allResultsOk;
}

void InterCallTestCase::runCommand (const op::WriteData& cmd, int stepNdx, int& programFriendlyName)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Running program #" << ++programFriendlyName << " to write " << ((m_storage == STORAGE_BUFFER) ? ("buffer") : ("image")) << " #" << cmd.targetHandle << ".\n"
		<< "	Dispatch size: " << m_invocationGridSize << "x" << m_invocationGridSize << "."
		<< tcu::TestLog::EndMessage;

	gl.useProgram(m_operationPrograms[stepNdx]->getProgram());

	// set destination
	if (m_storage == STORAGE_BUFFER)
	{
		DE_ASSERT(m_storageIDs[cmd.targetHandle]);

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storageIDs[cmd.targetHandle]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind destination buffer");
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		DE_ASSERT(m_storageIDs[cmd.targetHandle]);

		gl.bindImageTexture(0, m_storageIDs[cmd.targetHandle], 0, GL_FALSE, 0, (m_useAtomic) ? (GL_READ_WRITE) : (GL_WRITE_ONLY), (m_formatInteger) ? (GL_R32I) : (GL_R32F));
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind destination image");
	}
	else
		DE_ASSERT(DE_FALSE);

	// calc
	gl.dispatchCompute(m_invocationGridSize, m_invocationGridSize, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatch write");
}

void InterCallTestCase::runCommand (const op::ReadData& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName)
{
	runSingleRead(cmd.targetHandle, stepNdx, programFriendlyName, resultStorageFriendlyName);
}

void InterCallTestCase::runCommand (const op::Barrier& cmd)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	DE_UNREF(cmd);

	if (m_storage == STORAGE_BUFFER)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Memory Barrier\n\tbits = GL_SHADER_STORAGE_BARRIER_BIT" << tcu::TestLog::EndMessage;
		gl.memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Memory Barrier\n\tbits = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT" << tcu::TestLog::EndMessage;
		gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	else
		DE_ASSERT(DE_FALSE);
}

void InterCallTestCase::runCommand (const op::ReadMultipleData& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Running program #" << ++programFriendlyName << " to verify " << ((m_storage == STORAGE_BUFFER) ? ("buffers") : ("images")) << " #" << cmd.targetHandle0 << " and #" << cmd.targetHandle1 << ".\n"
		<< "	Writing results to result storage #" << ++resultStorageFriendlyName << ".\n"
		<< "	Dispatch size: " << m_invocationGridSize << "x" << m_invocationGridSize << "."
		<< tcu::TestLog::EndMessage;

	gl.useProgram(m_operationPrograms[stepNdx]->getProgram());

	// set sources
	if (m_storage == STORAGE_BUFFER)
	{
		DE_ASSERT(m_storageIDs[cmd.targetHandle0]);
		DE_ASSERT(m_storageIDs[cmd.targetHandle1]);

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storageIDs[cmd.targetHandle0]);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storageIDs[cmd.targetHandle1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind source buffers");
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		DE_ASSERT(m_storageIDs[cmd.targetHandle0]);
		DE_ASSERT(m_storageIDs[cmd.targetHandle1]);

		gl.bindImageTexture(1, m_storageIDs[cmd.targetHandle0], 0, GL_FALSE, 0, (m_useAtomic) ? (GL_READ_WRITE) : (GL_READ_ONLY), (m_formatInteger) ? (GL_R32I) : (GL_R32F));
		gl.bindImageTexture(2, m_storageIDs[cmd.targetHandle1], 0, GL_FALSE, 0, (m_useAtomic) ? (GL_READ_WRITE) : (GL_READ_ONLY), (m_formatInteger) ? (GL_R32I) : (GL_R32F));
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind source images");
	}
	else
		DE_ASSERT(DE_FALSE);

	// set destination
	DE_ASSERT(m_operationResultStorages[stepNdx]);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_operationResultStorages[stepNdx]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind result buffer");

	// calc
	gl.dispatchCompute(m_invocationGridSize, m_invocationGridSize, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatch read multi");
}

void InterCallTestCase::runCommand (const op::WriteDataInterleaved& cmd, int stepNdx, int& programFriendlyName)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Running program #" << ++programFriendlyName << " to write " << ((m_storage == STORAGE_BUFFER) ? ("buffer") : ("image")) << " #" << cmd.targetHandle << ".\n"
		<< "	Writing to every " << ((cmd.evenOdd) ? ("even") : ("odd")) << " " << ((m_storage == STORAGE_BUFFER) ? ("element") : ("column")) << ".\n"
		<< "	Dispatch size: " << m_invocationGridSize / 2 << "x" << m_invocationGridSize << "."
		<< tcu::TestLog::EndMessage;

	gl.useProgram(m_operationPrograms[stepNdx]->getProgram());

	// set destination
	if (m_storage == STORAGE_BUFFER)
	{
		DE_ASSERT(m_storageIDs[cmd.targetHandle]);

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storageIDs[cmd.targetHandle]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind destination buffer");
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		DE_ASSERT(m_storageIDs[cmd.targetHandle]);

		gl.bindImageTexture(0, m_storageIDs[cmd.targetHandle], 0, GL_FALSE, 0, (m_useAtomic) ? (GL_READ_WRITE) : (GL_WRITE_ONLY), (m_formatInteger) ? (GL_R32I) : (GL_R32F));
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind destination image");
	}
	else
		DE_ASSERT(DE_FALSE);

	// calc
	gl.dispatchCompute(m_invocationGridSize / 2, m_invocationGridSize, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatch write");
}

void InterCallTestCase::runCommand (const op::ReadDataInterleaved& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName)
{
	runSingleRead(cmd.targetHandle, stepNdx, programFriendlyName, resultStorageFriendlyName);
}

void InterCallTestCase::runCommand (const op::ReadZeroData& cmd, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName)
{
	runSingleRead(cmd.targetHandle, stepNdx, programFriendlyName, resultStorageFriendlyName);
}

void InterCallTestCase::runSingleRead (int targetHandle, int stepNdx, int& programFriendlyName, int& resultStorageFriendlyName)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Running program #" << ++programFriendlyName << " to verify " << ((m_storage == STORAGE_BUFFER) ? ("buffer") : ("image")) << " #" << targetHandle << ".\n"
		<< "	Writing results to result storage #" << ++resultStorageFriendlyName << ".\n"
		<< "	Dispatch size: " << m_invocationGridSize << "x" << m_invocationGridSize << "."
		<< tcu::TestLog::EndMessage;

	gl.useProgram(m_operationPrograms[stepNdx]->getProgram());

	// set source
	if (m_storage == STORAGE_BUFFER)
	{
		DE_ASSERT(m_storageIDs[targetHandle]);

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storageIDs[targetHandle]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind source buffer");
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		DE_ASSERT(m_storageIDs[targetHandle]);

		gl.bindImageTexture(1, m_storageIDs[targetHandle], 0, GL_FALSE, 0, (m_useAtomic) ? (GL_READ_WRITE) : (GL_READ_ONLY), (m_formatInteger) ? (GL_R32I) : (GL_R32F));
		GLU_EXPECT_NO_ERROR(gl.getError(), "bind source image");
	}
	else
		DE_ASSERT(DE_FALSE);

	// set destination
	DE_ASSERT(m_operationResultStorages[stepNdx]);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_operationResultStorages[stepNdx]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind result buffer");

	// calc
	gl.dispatchCompute(m_invocationGridSize, m_invocationGridSize, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatch read");
}

glw::GLuint InterCallTestCase::genStorage (int friendlyName)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_storage == STORAGE_BUFFER)
	{
		const int		numElements		= m_invocationGridSize * m_invocationGridSize * m_perInvocationSize;
		const int		bufferSize		= numElements * (int)((m_formatInteger) ? (sizeof(deInt32)) : (sizeof(glw::GLfloat)));
		glw::GLuint		retVal			= 0;

		m_testCtx.getLog() << tcu::TestLog::Message << "Creating buffer #" << friendlyName << ", size " << bufferSize << " bytes." << tcu::TestLog::EndMessage;

		gl.genBuffers(1, &retVal);
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, retVal);

		if (m_formatInteger)
		{
			const std::vector<deUint32> zeroBuffer(numElements, 0);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, &zeroBuffer[0], GL_STATIC_DRAW);
		}
		else
		{
			const std::vector<float> zeroBuffer(numElements, 0.0f);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, &zeroBuffer[0], GL_STATIC_DRAW);
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffer");

		return retVal;
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		const int	imageWidth	= m_invocationGridSize;
		const int	imageHeight	= m_invocationGridSize * m_perInvocationSize;
		glw::GLuint	retVal		= 0;

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Creating image #" << friendlyName << ", size " << imageWidth << "x" << imageHeight
			<< ", internalformat = " << ((m_formatInteger) ? ("r32i") : ("r32f"))
			<< ", size = " << (imageWidth*imageHeight*sizeof(deUint32)) << " bytes."
			<< tcu::TestLog::EndMessage;

		gl.genTextures(1, &retVal);
		gl.bindTexture(GL_TEXTURE_2D, retVal);

		if (m_formatInteger)
			gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32I, imageWidth, imageHeight);
		else
			gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32F, imageWidth, imageHeight);

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen image");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Filling image with 0"
			<< tcu::TestLog::EndMessage;

		if (m_formatInteger)
		{
			const std::vector<deInt32> zeroBuffer(imageWidth * imageHeight, 0);
			gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, GL_RED_INTEGER, GL_INT, &zeroBuffer[0]);
		}
		else
		{
			const std::vector<float> zeroBuffer(imageWidth * imageHeight, 0.0f);
			gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, GL_RED, GL_FLOAT, &zeroBuffer[0]);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "specify image contents");

		return retVal;
	}
	else
	{
		DE_ASSERT(DE_FALSE);
		return 0;
	}
}

glw::GLuint InterCallTestCase::genResultStorage (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	glw::GLuint				retVal	= 0;

	gl.genBuffers(1, &retVal);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, retVal);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, m_invocationGridSize * m_invocationGridSize * sizeof(deUint32), DE_NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffer");

	return retVal;
}

glu::ShaderProgram* InterCallTestCase::genWriteProgram (int seed)
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x = 1, local_size_y = 1) in;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=0, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_out;\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=0) " << ((m_useAtomic) ? ("coherent ") : ("writeonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageOut;\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uvec3 size    = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "	int groupNdx  = int(size.x * size.y * gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);\n"
		<< "\n";

	// Write to buffer/image m_perInvocationSize elements
	if (m_storage == STORAGE_BUFFER)
	{
		for (int writeNdx = 0; writeNdx < m_perInvocationSize; ++writeNdx)
		{
			if (m_useAtomic)
				buf << "	atomicExchange(";
			else
				buf << "	";

			buf << "sb_out.values[(groupNdx + " << seed + writeNdx*m_invocationGridSize*m_invocationGridSize << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize << "]";

			if (m_useAtomic)
				buf << ", " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
			else
				buf << " = " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx);\n";
		}
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		for (int writeNdx = 0; writeNdx < m_perInvocationSize; ++writeNdx)
		{
			if (m_useAtomic)
				buf << "	imageAtomicExchange";
			else
				buf << "	imageStore";

			buf << "(u_imageOut, ivec2((int(gl_GlobalInvocationID.x) + " << (seed + writeNdx*100) << ") % " << m_invocationGridSize << ", int(gl_GlobalInvocationID.y) + " << writeNdx*m_invocationGridSize << "), ";

			if (m_useAtomic)
				buf << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
			else
				buf << ((m_formatInteger) ? ("ivec4(int(groupNdx), 0, 0, 0)") : ("vec4(float(groupNdx), 0.0, 0.0, 0.0)")) << ");\n";
		}
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, buf.str().c_str())));
}

glu::ShaderProgram* InterCallTestCase::genReadProgram (int seed)
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x = 1, local_size_y = 1) in;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=1, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_in;\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=1) " << ((m_useAtomic) ? ("coherent ") : ("readonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageIn;\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "layout(binding=0, std430) buffer ResultBuffer\n"
		<< "{\n"
		<< "	highp int resultOk[];\n"
		<< "} sb_result;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uvec3 size = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "	int groupNdx = int(size.x * size.y * gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);\n"
		<< "	" << ((m_formatInteger) ? ("int") : ("float")) << " zero = " << ((m_formatInteger) ? ("0") : ("0.0")) << ";\n"
		<< "	bool allOk = true;\n"
		<< "\n";

	// Verify data

	if (m_storage == STORAGE_BUFFER)
	{
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
		{
			if (!m_useAtomic)
				buf << "	allOk = allOk && (sb_in.values[(groupNdx + "
					<< seed + readNdx*m_invocationGridSize*m_invocationGridSize << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize << "] == "
					<< ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
			else
				buf << "	allOk = allOk && (atomicExchange(sb_in.values[(groupNdx + "
					<< seed + readNdx*m_invocationGridSize*m_invocationGridSize << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize << "], zero) == "
					<< ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
		}
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
		{
			if (!m_useAtomic)
				buf	<< "	allOk = allOk && (imageLoad(u_imageIn, ivec2((gl_GlobalInvocationID.x + "
					<< (seed + readNdx*100) << "u) % " << m_invocationGridSize << "u, gl_GlobalInvocationID.y + " << readNdx*m_invocationGridSize << "u)).x == "
					<< ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
			else
				buf << "	allOk = allOk && (imageAtomicExchange(u_imageIn, ivec2((gl_GlobalInvocationID.x + "
					<< (seed + readNdx*100) << "u) % " << m_invocationGridSize << "u, gl_GlobalInvocationID.y + " << readNdx*m_invocationGridSize << "u), zero) == "
					<< ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
		}
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "	sb_result.resultOk[groupNdx] = (allOk) ? (1) : (0);\n"
		<< "}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, buf.str().c_str())));
}

glu::ShaderProgram* InterCallTestCase::genReadMultipleProgram (int seed0, int seed1)
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x = 1, local_size_y = 1) in;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=1, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer0\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_in0;\n"
			<< "layout(binding=2, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer1\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_in1;\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=1) " << ((m_useAtomic) ? ("coherent ") : ("readonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageIn0;\n"
			<< "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=2) " << ((m_useAtomic) ? ("coherent ") : ("readonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageIn1;\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "layout(binding=0, std430) buffer ResultBuffer\n"
		<< "{\n"
		<< "	highp int resultOk[];\n"
		<< "} sb_result;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uvec3 size = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "	int groupNdx = int(size.x * size.y * gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);\n"
		<< "	" << ((m_formatInteger) ? ("int") : ("float")) << " zero = " << ((m_formatInteger) ? ("0") : ("0.0")) << ";\n"
		<< "	bool allOk = true;\n"
		<< "\n";

	// Verify data

	if (m_storage == STORAGE_BUFFER)
	{
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "	allOk = allOk && (" << ((m_useAtomic) ? ("atomicExchange(") : ("")) << "sb_in0.values[(groupNdx + " << seed0 + readNdx*m_invocationGridSize*m_invocationGridSize << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize << "]" << ((m_useAtomic) ? (", zero)") : ("")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n"
				<< "	allOk = allOk && (" << ((m_useAtomic) ? ("atomicExchange(") : ("")) << "sb_in1.values[(groupNdx + " << seed1 + readNdx*m_invocationGridSize*m_invocationGridSize << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize << "]" << ((m_useAtomic) ? (", zero)") : ("")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "	allOk = allOk && (" << ((m_useAtomic) ? ("imageAtomicExchange") : ("imageLoad")) << "(u_imageIn0, ivec2((gl_GlobalInvocationID.x + " << (seed0 + readNdx*100) << "u) % " << m_invocationGridSize << "u, gl_GlobalInvocationID.y + " << readNdx*m_invocationGridSize << "u)" << ((m_useAtomic) ? (", zero)") : (").x")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n"
				<< "	allOk = allOk && (" << ((m_useAtomic) ? ("imageAtomicExchange") : ("imageLoad")) << "(u_imageIn1, ivec2((gl_GlobalInvocationID.x + " << (seed1 + readNdx*100) << "u) % " << m_invocationGridSize << "u, gl_GlobalInvocationID.y + " << readNdx*m_invocationGridSize << "u)" << ((m_useAtomic) ? (", zero)") : (").x")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "	sb_result.resultOk[groupNdx] = (allOk) ? (1) : (0);\n"
		<< "}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, buf.str().c_str())));
}

glu::ShaderProgram* InterCallTestCase::genWriteInterleavedProgram (int seed, bool evenOdd)
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x = 1, local_size_y = 1) in;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=0, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_out;\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=0) " << ((m_useAtomic) ? ("coherent ") : ("writeonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageOut;\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uvec3 size    = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "	int groupNdx  = int(size.x * size.y * gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);\n"
		<< "\n";

	// Write to buffer/image m_perInvocationSize elements
	if (m_storage == STORAGE_BUFFER)
	{
		for (int writeNdx = 0; writeNdx < m_perInvocationSize; ++writeNdx)
		{
			if (m_useAtomic)
				buf << "	atomicExchange(";
			else
				buf << "	";

			buf << "sb_out.values[((groupNdx + " << seed + writeNdx*m_invocationGridSize*m_invocationGridSize / 2 << ") % " << m_invocationGridSize*m_invocationGridSize / 2 * m_perInvocationSize  << ") * 2 + " << ((evenOdd) ? (0) : (1)) << "]";

			if (m_useAtomic)
				buf << ", " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
			else
				buf << "= " << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx);\n";
		}
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		for (int writeNdx = 0; writeNdx < m_perInvocationSize; ++writeNdx)
		{
			if (m_useAtomic)
				buf << "	imageAtomicExchange";
			else
				buf << "	imageStore";

			buf << "(u_imageOut, ivec2(((int(gl_GlobalInvocationID.x) + " << (seed + writeNdx*100) << ") % " << m_invocationGridSize / 2 << ") * 2 + " << ((evenOdd) ? (0) : (1)) << ", int(gl_GlobalInvocationID.y) + " << writeNdx*m_invocationGridSize << "), ";

			if (m_useAtomic)
				buf << ((m_formatInteger) ? ("int") : ("float")) << "(groupNdx));\n";
			else
				buf << ((m_formatInteger) ? ("ivec4(int(groupNdx), 0, 0, 0)") : ("vec4(float(groupNdx), 0.0, 0.0, 0.0)")) << ");\n";
		}
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, buf.str().c_str())));
}

glu::ShaderProgram* InterCallTestCase::genReadInterleavedProgram (int seed0, int seed1)
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x = 1, local_size_y = 1) in;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=1, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_in;\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=1) " << ((m_useAtomic) ? ("coherent ") : ("readonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageIn;\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "layout(binding=0, std430) buffer ResultBuffer\n"
		<< "{\n"
		<< "	highp int resultOk[];\n"
		<< "} sb_result;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uvec3 size = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "	int groupNdx = int(size.x * size.y * gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);\n"
		<< "	int interleavedGroupNdx = int((size.x >> 1U) * size.y * gl_GlobalInvocationID.z + (size.x >> 1U) * gl_GlobalInvocationID.y + (gl_GlobalInvocationID.x >> 1U));\n"
		<< "	" << ((m_formatInteger) ? ("int") : ("float")) << " zero = " << ((m_formatInteger) ? ("0") : ("0.0")) << ";\n"
		<< "	bool allOk = true;\n"
		<< "\n";

	// Verify data

	if (m_storage == STORAGE_BUFFER)
	{
		buf << "	if (groupNdx % 2 == 0)\n"
			<< "	{\n";
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "		allOk = allOk && ("
				<< ((m_useAtomic) ? ("atomicExchange(") : ("")) << "sb_in.values[((interleavedGroupNdx + " << seed0 + readNdx*m_invocationGridSize*m_invocationGridSize / 2 << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize / 2 << ") * 2 + 0]"
				<< ((m_useAtomic) ? (", zero)") : ("")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(interleavedGroupNdx));\n";
		buf << "	}\n"
			<< "	else\n"
			<< "	{\n";
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "		allOk = allOk && ("
				<< ((m_useAtomic) ? ("atomicExchange(") : ("")) << "sb_in.values[((interleavedGroupNdx + " << seed1 + readNdx*m_invocationGridSize*m_invocationGridSize / 2 << ") % " << m_invocationGridSize*m_invocationGridSize*m_perInvocationSize / 2 << ") * 2 + 1]"
				<< ((m_useAtomic) ? (", zero)") : ("")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(interleavedGroupNdx));\n";
		buf << "	}\n";
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		buf << "	if (groupNdx % 2 == 0)\n"
			<< "	{\n";
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "		allOk = allOk && ("
				<< ((m_useAtomic) ? ("imageAtomicExchange") : ("imageLoad"))
				<< "(u_imageIn, ivec2(((int(gl_GlobalInvocationID.x >> 1U) + " << (seed0 + readNdx*100) << ") % " << m_invocationGridSize / 2 << ") * 2 + 0, int(gl_GlobalInvocationID.y) + " << readNdx*m_invocationGridSize << ")"
				<< ((m_useAtomic) ? (", zero)") : (").x")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(interleavedGroupNdx));\n";
		buf << "	}\n"
			<< "	else\n"
			<< "	{\n";
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "		allOk = allOk && ("
				<< ((m_useAtomic) ? ("imageAtomicExchange") : ("imageLoad"))
				<< "(u_imageIn, ivec2(((int(gl_GlobalInvocationID.x >> 1U) + " << (seed1 + readNdx*100) << ") % " << m_invocationGridSize / 2 << ") * 2 + 1, int(gl_GlobalInvocationID.y) + " << readNdx*m_invocationGridSize << ")"
				<< ((m_useAtomic) ? (", zero)") : (").x")) << " == " << ((m_formatInteger) ? ("int") : ("float")) << "(interleavedGroupNdx));\n";
		buf << "	}\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "	sb_result.resultOk[groupNdx] = (allOk) ? (1) : (0);\n"
		<< "}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, buf.str().c_str())));
}

glu::ShaderProgram*	InterCallTestCase::genReadZeroProgram (void)
{
	const bool			useImageAtomics = m_useAtomic && m_storage == STORAGE_IMAGE;
	std::ostringstream	buf;

	buf << "${GLSL_VERSION_DECL}\n"
		<< ((useImageAtomics) ? ("${SHADER_IMAGE_ATOMIC_REQUIRE}\n") : (""))
		<< "layout (local_size_x = 1, local_size_y = 1) in;\n";

	if (m_storage == STORAGE_BUFFER)
		buf << "layout(binding=1, std430) " << ((m_useAtomic) ? ("coherent ") : ("")) << "buffer Buffer\n"
			<< "{\n"
			<< "	highp " << ((m_formatInteger) ? ("int") : ("float")) << " values[];\n"
			<< "} sb_in;\n";
	else if (m_storage == STORAGE_IMAGE)
		buf << "layout(" << ((m_formatInteger) ? ("r32i") : ("r32f")) << ", binding=1) " << ((m_useAtomic) ? ("coherent ") : ("readonly ")) << "uniform highp " << ((m_formatInteger) ? ("iimage2D") : ("image2D")) << " u_imageIn;\n";
	else
		DE_ASSERT(DE_FALSE);

	buf << "layout(binding=0, std430) buffer ResultBuffer\n"
		<< "{\n"
		<< "	highp int resultOk[];\n"
		<< "} sb_result;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uvec3 size = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "	int groupNdx = int(size.x * size.y * gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);\n"
		<< "	" << ((m_formatInteger) ? ("int") : ("float")) << " anything = " << ((m_formatInteger) ? ("5") : ("5.0")) << ";\n"
		<< "	bool allOk = true;\n"
		<< "\n";

	// Verify data

	if (m_storage == STORAGE_BUFFER)
	{
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "	allOk = allOk && ("
				<< ((m_useAtomic) ? ("atomicExchange(") : ("")) << "sb_in.values[groupNdx * " << m_perInvocationSize << " + " << readNdx << "]"
				<< ((m_useAtomic) ? (", anything)") : ("")) << " == " << ((m_formatInteger) ? ("0") : ("0.0")) << ");\n";
	}
	else if (m_storage == STORAGE_IMAGE)
	{
		for (int readNdx = 0; readNdx < m_perInvocationSize; ++readNdx)
			buf << "	allOk = allOk && ("
			<< ((m_useAtomic) ? ("imageAtomicExchange") : ("imageLoad")) << "(u_imageIn, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y + " << (readNdx*m_invocationGridSize) << "u)"
			<< ((m_useAtomic) ? (", anything)") : (").x")) << " == " << ((m_formatInteger) ? ("0") : ("0.0")) << ");\n";
	}
	else
		DE_ASSERT(DE_FALSE);

	buf << "	sb_result.resultOk[groupNdx] = (allOk) ? (1) : (0);\n"
		<< "}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, buf.str().c_str())));
}

class SSBOConcurrentAtomicCase : public TestCase
{
public:

							SSBOConcurrentAtomicCase	(Context& context, const char* name, const char* description, int numCalls, int workSize);
							~SSBOConcurrentAtomicCase	(void);

	void					init						(void);
	void					deinit						(void);
	IterateResult			iterate						(void);

private:
	std::string				genComputeSource			(void) const;

	const int				m_numCalls;
	const int				m_workSize;
	glu::ShaderProgram*		m_program;
	deUint32				m_bufferID;
	std::vector<deUint32>	m_intermediateResultBuffers;
};

SSBOConcurrentAtomicCase::SSBOConcurrentAtomicCase (Context& context, const char* name, const char* description, int numCalls, int workSize)
	: TestCase		(context, name, description)
	, m_numCalls	(numCalls)
	, m_workSize	(workSize)
	, m_program		(DE_NULL)
	, m_bufferID	(DE_NULL)
{
}

SSBOConcurrentAtomicCase::~SSBOConcurrentAtomicCase (void)
{
	deinit();
}

void SSBOConcurrentAtomicCase::init (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	std::vector<deUint32>	zeroData			(m_workSize, 0);

	// gen buffers

	gl.genBuffers(1, &m_bufferID);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_bufferID);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(deUint32) * m_workSize, &zeroData[0], GL_DYNAMIC_COPY);

	for (int ndx = 0; ndx < m_numCalls; ++ndx)
	{
		deUint32 buffer = 0;

		gl.genBuffers(1, &buffer);
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(deUint32) * m_workSize, &zeroData[0], GL_DYNAMIC_COPY);

		m_intermediateResultBuffers.push_back(buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffers");
	}

	// gen program

	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genComputeSource()));
	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		throw tcu::TestError("could not build program");
}

void SSBOConcurrentAtomicCase::deinit (void)
{
	if (m_bufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_bufferID);
		m_bufferID = 0;
	}

	for (int ndx = 0; ndx < (int)m_intermediateResultBuffers.size(); ++ndx)
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_intermediateResultBuffers[ndx]);
	m_intermediateResultBuffers.clear();

	delete m_program;
	m_program = DE_NULL;
}

TestCase::IterateResult SSBOConcurrentAtomicCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const deUint32			sumValue		= (deUint32)(m_numCalls * (m_numCalls + 1) / 2);
	std::vector<int>		deltas;

	// generate unique deltas
	generateShuffledRamp(m_numCalls, deltas);

	// invoke program N times, each with a different delta
	{
		const int deltaLocation = gl.getUniformLocation(m_program->getProgram(), "u_atomicDelta");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Running shader " << m_numCalls << " times.\n"
			<< "Num groups = (" << m_workSize << ", 1, 1)\n"
			<< "Setting u_atomicDelta to a unique value for each call.\n"
			<< tcu::TestLog::EndMessage;

		if (deltaLocation == -1)
			throw tcu::TestError("u_atomicDelta location was -1");

		gl.useProgram(m_program->getProgram());
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_bufferID);

		for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Call " << callNdx << ": u_atomicDelta = " << deltas[callNdx]
				<< tcu::TestLog::EndMessage;

			gl.uniform1ui(deltaLocation, deltas[callNdx]);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_intermediateResultBuffers[callNdx]);
			gl.dispatchCompute(m_workSize, 1, 1);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "post dispatch");
	}

	// Verify result
	{
		std::vector<deUint32> result;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying work buffer, it should be filled with value " << sumValue << tcu::TestLog::EndMessage;

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_bufferID);
		readBuffer(gl, GL_SHADER_STORAGE_BUFFER, m_workSize, result);

		for (int ndx = 0; ndx < m_workSize; ++ndx)
		{
			if (result[ndx] != sumValue)
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Work buffer error, at index " << ndx << " expected value " << (sumValue) << ", got " << result[ndx] << "\n"
					<< "Work buffer contains invalid values."
					<< tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents invalid");
				return STOP;
			}
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Work buffer contents are valid." << tcu::TestLog::EndMessage;
	}

	// verify steps
	{
		std::vector<std::vector<deUint32> >	intermediateResults	(m_numCalls);
		std::vector<deUint32>				valueChain			(m_numCalls);

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying intermediate results. " << tcu::TestLog::EndMessage;

		// collect results

		for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
		{
			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_intermediateResultBuffers[callNdx]);
			readBuffer(gl, GL_SHADER_STORAGE_BUFFER, m_workSize, intermediateResults[callNdx]);
		}

		// verify values

		for (int valueNdx = 0; valueNdx < m_workSize; ++valueNdx)
		{
			int			invalidOperationNdx;
			deUint32	errorDelta;
			deUint32	errorExpected;

			// collect result chain for each element
			for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
				valueChain[callNdx] = intermediateResults[callNdx][valueNdx];

			// check there exists a path from 0 to sumValue using each addition once
			// decompose cumulative results to addition operations (all additions positive => this works)

			std::sort(valueChain.begin(), valueChain.end());

			// validate chain
			if (!validateSortedAtomicRampAdditionValueChain(valueChain, sumValue, invalidOperationNdx, errorDelta, errorExpected))
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Intermediate buffer error, at value index " << valueNdx << ", applied operation index " << invalidOperationNdx << ", value was increased by " << errorDelta << ", but expected " << errorExpected << ".\n"
					<< "Intermediate buffer contains invalid values. Values at index " << valueNdx << "\n"
					<< tcu::TestLog::EndMessage;

				for (int logCallNdx = 0; logCallNdx < m_numCalls; ++logCallNdx)
					m_testCtx.getLog() << tcu::TestLog::Message << "Value[" << logCallNdx << "] = " << intermediateResults[logCallNdx][valueNdx] << tcu::TestLog::EndMessage;
				m_testCtx.getLog() << tcu::TestLog::Message << "Result = " << sumValue << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents invalid");
				return STOP;
			}
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Intermediate buffers are valid." << tcu::TestLog::EndMessage;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

std::string SSBOConcurrentAtomicCase::genComputeSource (void) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout (binding = 1, std430) writeonly buffer IntermediateResults\n"
		<< "{\n"
		<< "	highp uint values[" << m_workSize << "];\n"
		<< "} sb_ires;\n"
		<< "\n"
		<< "layout (binding = 2, std430) volatile buffer WorkBuffer\n"
		<< "{\n"
		<< "	highp uint values[" << m_workSize << "];\n"
		<< "} sb_work;\n"
		<< "uniform highp uint u_atomicDelta;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	highp uint invocationIndex = gl_GlobalInvocationID.x;\n"
		<< "	sb_ires.values[invocationIndex] = atomicAdd(sb_work.values[invocationIndex], u_atomicDelta);\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

class ConcurrentAtomicCounterCase : public TestCase
{
public:

							ConcurrentAtomicCounterCase		(Context& context, const char* name, const char* description, int numCalls, int workSize);
							~ConcurrentAtomicCounterCase	(void);

	void					init							(void);
	void					deinit							(void);
	IterateResult			iterate							(void);

private:
	std::string				genComputeSource				(bool evenOdd) const;

	const int				m_numCalls;
	const int				m_workSize;
	glu::ShaderProgram*		m_evenProgram;
	glu::ShaderProgram*		m_oddProgram;
	deUint32				m_counterBuffer;
	deUint32				m_intermediateResultBuffer;
};

ConcurrentAtomicCounterCase::ConcurrentAtomicCounterCase (Context& context, const char* name, const char* description, int numCalls, int workSize)
	: TestCase					(context, name, description)
	, m_numCalls				(numCalls)
	, m_workSize				(workSize)
	, m_evenProgram				(DE_NULL)
	, m_oddProgram				(DE_NULL)
	, m_counterBuffer			(DE_NULL)
	, m_intermediateResultBuffer(DE_NULL)
{
}

ConcurrentAtomicCounterCase::~ConcurrentAtomicCounterCase (void)
{
	deinit();
}

void ConcurrentAtomicCounterCase::init (void)
{
	const glw::Functions&		gl			= m_context.getRenderContext().getFunctions();
	const std::vector<deUint32>	zeroData	(m_numCalls * m_workSize, 0);

	// gen buffer

	gl.genBuffers(1, &m_counterBuffer);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_counterBuffer);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(deUint32), &zeroData[0], GL_DYNAMIC_COPY);

	gl.genBuffers(1, &m_intermediateResultBuffer);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_intermediateResultBuffer);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(deUint32) * m_numCalls * m_workSize, &zeroData[0], GL_DYNAMIC_COPY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffers");

	// gen programs

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "EvenProgram", "Even program");

		m_evenProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genComputeSource(true)));
		m_testCtx.getLog() << *m_evenProgram;
		if (!m_evenProgram->isOk())
			throw tcu::TestError("could not build program");
	}
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "OddProgram", "Odd program");

		m_oddProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genComputeSource(false)));
		m_testCtx.getLog() << *m_oddProgram;
		if (!m_oddProgram->isOk())
			throw tcu::TestError("could not build program");
	}
}

void ConcurrentAtomicCounterCase::deinit (void)
{
	if (m_counterBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_counterBuffer);
		m_counterBuffer = 0;
	}
	if (m_intermediateResultBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_intermediateResultBuffer);
		m_intermediateResultBuffer = 0;
	}

	delete m_evenProgram;
	m_evenProgram = DE_NULL;

	delete m_oddProgram;
	m_oddProgram = DE_NULL;
}

TestCase::IterateResult ConcurrentAtomicCounterCase::iterate (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// invoke program N times, each with a different delta
	{
		const int evenCallNdxLocation	= gl.getUniformLocation(m_evenProgram->getProgram(), "u_callNdx");
		const int oddCallNdxLocation	= gl.getUniformLocation(m_oddProgram->getProgram(), "u_callNdx");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Running shader pair (even & odd) " << m_numCalls << " times.\n"
			<< "Num groups = (" << m_workSize << ", 1, 1)\n"
			<< tcu::TestLog::EndMessage;

		if (evenCallNdxLocation == -1)
			throw tcu::TestError("u_callNdx location was -1");
		if (oddCallNdxLocation == -1)
			throw tcu::TestError("u_callNdx location was -1");

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_intermediateResultBuffer);
		gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_counterBuffer);

		for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
		{
			gl.useProgram(m_evenProgram->getProgram());
			gl.uniform1ui(evenCallNdxLocation, (deUint32)callNdx);
			gl.dispatchCompute(m_workSize, 1, 1);

			gl.useProgram(m_oddProgram->getProgram());
			gl.uniform1ui(oddCallNdxLocation, (deUint32)callNdx);
			gl.dispatchCompute(m_workSize, 1, 1);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "post dispatch");
	}

	// Verify result
	{
		deUint32 result;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying work buffer, it should be " << m_numCalls*m_workSize << tcu::TestLog::EndMessage;

		gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_counterBuffer);
		result = readBufferUint32(gl, GL_ATOMIC_COUNTER_BUFFER);

		if ((int)result != m_numCalls*m_workSize)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Counter buffer error, expected value " << (m_numCalls*m_workSize) << ", got " << result << "\n"
				<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents invalid");
			return STOP;
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Counter buffer is valid." << tcu::TestLog::EndMessage;
	}

	// verify steps
	{
		std::vector<deUint32> intermediateResults;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying intermediate results. " << tcu::TestLog::EndMessage;

		// collect results

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_intermediateResultBuffer);
		readBuffer(gl, GL_SHADER_STORAGE_BUFFER, m_numCalls * m_workSize, intermediateResults);

		// verify values

		std::sort(intermediateResults.begin(), intermediateResults.end());

		for (int valueNdx = 0; valueNdx < m_workSize * m_numCalls; ++valueNdx)
		{
			if ((int)intermediateResults[valueNdx] != valueNdx)
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Intermediate buffer error, at value index " << valueNdx << ", expected " << valueNdx << ", got " << intermediateResults[valueNdx] << ".\n"
					<< "Intermediate buffer contains invalid values. Intermediate results:\n"
					<< tcu::TestLog::EndMessage;

				for (int logCallNdx = 0; logCallNdx < m_workSize * m_numCalls; ++logCallNdx)
					m_testCtx.getLog() << tcu::TestLog::Message << "Value[" << logCallNdx << "] = " << intermediateResults[logCallNdx] << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents invalid");
				return STOP;
			}
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Intermediate buffers are valid." << tcu::TestLog::EndMessage;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

std::string ConcurrentAtomicCounterCase::genComputeSource (bool evenOdd) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout (binding = 1, std430) writeonly buffer IntermediateResults\n"
		<< "{\n"
		<< "	highp uint values[" << m_workSize * m_numCalls << "];\n"
		<< "} sb_ires;\n"
		<< "\n"
		<< "layout (binding = 2, offset = 0) uniform atomic_uint u_counter;\n"
		<< "uniform highp uint u_callNdx;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	highp uint dataNdx = u_callNdx * " << m_workSize << "u + gl_GlobalInvocationID.x;\n"
		<< "	if ((dataNdx % 2u) == " << ((evenOdd) ? (0) : (1)) << "u)\n"
		<< "		sb_ires.values[dataNdx] = atomicCounterIncrement(u_counter);\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

class ConcurrentImageAtomicCase : public TestCase
{
public:

							ConcurrentImageAtomicCase	(Context& context, const char* name, const char* description, int numCalls, int workSize);
							~ConcurrentImageAtomicCase	(void);

	void					init						(void);
	void					deinit						(void);
	IterateResult			iterate						(void);

private:
	void					readWorkImage				(std::vector<deUint32>& result);

	std::string				genComputeSource			(void) const;
	std::string				genImageReadSource			(void) const;
	std::string				genImageClearSource			(void) const;

	const int				m_numCalls;
	const int				m_workSize;
	glu::ShaderProgram*		m_program;
	glu::ShaderProgram*		m_imageReadProgram;
	glu::ShaderProgram*		m_imageClearProgram;
	deUint32				m_imageID;
	std::vector<deUint32>	m_intermediateResultBuffers;
};

ConcurrentImageAtomicCase::ConcurrentImageAtomicCase (Context& context, const char* name, const char* description, int numCalls, int workSize)
	: TestCase				(context, name, description)
	, m_numCalls			(numCalls)
	, m_workSize			(workSize)
	, m_program				(DE_NULL)
	, m_imageReadProgram	(DE_NULL)
	, m_imageClearProgram	(DE_NULL)
	, m_imageID				(DE_NULL)
{
}

ConcurrentImageAtomicCase::~ConcurrentImageAtomicCase (void)
{
	deinit();
}

void ConcurrentImageAtomicCase::init (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	std::vector<deUint32>	zeroData			(m_workSize * m_workSize, 0);
	const bool				supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic"))
		throw tcu::NotSupportedError("Test requires GL_OES_shader_image_atomic");

	// gen image

	gl.genTextures(1, &m_imageID);
	gl.bindTexture(GL_TEXTURE_2D, m_imageID);
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_workSize, m_workSize);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gen tex");

	// gen buffers

	for (int ndx = 0; ndx < m_numCalls; ++ndx)
	{
		deUint32 buffer = 0;

		gl.genBuffers(1, &buffer);
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(deUint32) * m_workSize * m_workSize, &zeroData[0], GL_DYNAMIC_COPY);

		m_intermediateResultBuffers.push_back(buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffers");
	}

	// gen programs

	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genComputeSource()));
	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		throw tcu::TestError("could not build program");

	m_imageReadProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genImageReadSource()));
	if (!m_imageReadProgram->isOk())
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "ImageReadProgram", "Image read program");

		m_testCtx.getLog() << *m_imageReadProgram;
		throw tcu::TestError("could not build program");
	}

	m_imageClearProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genImageClearSource()));
	if (!m_imageClearProgram->isOk())
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "ImageClearProgram", "Image read program");

		m_testCtx.getLog() << *m_imageClearProgram;
		throw tcu::TestError("could not build program");
	}
}

void ConcurrentImageAtomicCase::deinit (void)
{
	if (m_imageID)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_imageID);
		m_imageID = 0;
	}

	for (int ndx = 0; ndx < (int)m_intermediateResultBuffers.size(); ++ndx)
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_intermediateResultBuffers[ndx]);
	m_intermediateResultBuffers.clear();

	delete m_program;
	m_program = DE_NULL;

	delete m_imageReadProgram;
	m_imageReadProgram = DE_NULL;

	delete m_imageClearProgram;
	m_imageClearProgram = DE_NULL;
}

TestCase::IterateResult ConcurrentImageAtomicCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const deUint32			sumValue		= (deUint32)(m_numCalls * (m_numCalls + 1) / 2);
	std::vector<int>		deltas;

	// generate unique deltas
	generateShuffledRamp(m_numCalls, deltas);

	// clear image
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Clearing image contents" << tcu::TestLog::EndMessage;

		gl.useProgram(m_imageClearProgram->getProgram());
		gl.bindImageTexture(2, m_imageID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
		gl.dispatchCompute(m_workSize, m_workSize, 1);
		gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "clear");
	}

	// invoke program N times, each with a different delta
	{
		const int deltaLocation = gl.getUniformLocation(m_program->getProgram(), "u_atomicDelta");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Running shader " << m_numCalls << " times.\n"
			<< "Num groups = (" << m_workSize << ", " << m_workSize << ", 1)\n"
			<< "Setting u_atomicDelta to a unique value for each call.\n"
			<< tcu::TestLog::EndMessage;

		if (deltaLocation == -1)
			throw tcu::TestError("u_atomicDelta location was -1");

		gl.useProgram(m_program->getProgram());
		gl.bindImageTexture(2, m_imageID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Call " << callNdx << ": u_atomicDelta = " << deltas[callNdx]
				<< tcu::TestLog::EndMessage;

			gl.uniform1ui(deltaLocation, deltas[callNdx]);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_intermediateResultBuffers[callNdx]);
			gl.dispatchCompute(m_workSize, m_workSize, 1);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "post dispatch");
	}

	// Verify result
	{
		std::vector<deUint32> result;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying work image, it should be filled with value " << sumValue << tcu::TestLog::EndMessage;

		readWorkImage(result);

		for (int ndx = 0; ndx < m_workSize * m_workSize; ++ndx)
		{
			if (result[ndx] != sumValue)
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Work image error, at index (" << ndx % m_workSize << ", " << ndx / m_workSize << ") expected value " << (sumValue) << ", got " << result[ndx] << "\n"
					<< "Work image contains invalid values."
					<< tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image contents invalid");
				return STOP;
			}
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Work image contents are valid." << tcu::TestLog::EndMessage;
	}

	// verify steps
	{
		std::vector<std::vector<deUint32> >	intermediateResults	(m_numCalls);
		std::vector<deUint32>				valueChain			(m_numCalls);
		std::vector<deUint32>				chainDelta			(m_numCalls);

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying intermediate results. " << tcu::TestLog::EndMessage;

		// collect results

		for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
		{
			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_intermediateResultBuffers[callNdx]);
			readBuffer(gl, GL_SHADER_STORAGE_BUFFER, m_workSize * m_workSize, intermediateResults[callNdx]);
		}

		// verify values

		for (int valueNdx = 0; valueNdx < m_workSize; ++valueNdx)
		{
			int			invalidOperationNdx;
			deUint32	errorDelta;
			deUint32	errorExpected;

			// collect result chain for each element
			for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
				valueChain[callNdx] = intermediateResults[callNdx][valueNdx];

			// check there exists a path from 0 to sumValue using each addition once
			// decompose cumulative results to addition operations (all additions positive => this works)

			std::sort(valueChain.begin(), valueChain.end());

			for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
				chainDelta[callNdx] = ((callNdx + 1 == m_numCalls) ? (sumValue) : (valueChain[callNdx+1])) - valueChain[callNdx];

			// chainDelta contains now the actual additions applied to the value
			std::sort(chainDelta.begin(), chainDelta.end());

			// validate chain
			if (!validateSortedAtomicRampAdditionValueChain(valueChain, sumValue, invalidOperationNdx, errorDelta, errorExpected))
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Intermediate buffer error, at index (" << valueNdx % m_workSize << ", " << valueNdx / m_workSize << "), applied operation index "
					<< invalidOperationNdx << ", value was increased by " << errorDelta << ", but expected " << errorExpected << ".\n"
					<< "Intermediate buffer contains invalid values. Values at index (" << valueNdx % m_workSize << ", " << valueNdx / m_workSize << ")\n"
					<< tcu::TestLog::EndMessage;

				for (int logCallNdx = 0; logCallNdx < m_numCalls; ++logCallNdx)
					m_testCtx.getLog() << tcu::TestLog::Message << "Value[" << logCallNdx << "] = " << intermediateResults[logCallNdx][valueNdx] << tcu::TestLog::EndMessage;
				m_testCtx.getLog() << tcu::TestLog::Message << "Result = " << sumValue << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents invalid");
				return STOP;
			}
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Intermediate buffers are valid." << tcu::TestLog::EndMessage;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

void ConcurrentImageAtomicCase::readWorkImage (std::vector<deUint32>& result)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	glu::Buffer				resultBuffer	(m_context.getRenderContext());

	// Read image to an ssbo

	{
		const std::vector<deUint32> zeroData(m_workSize*m_workSize, 0);

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *resultBuffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, (int)(sizeof(deUint32) * m_workSize * m_workSize), &zeroData[0], GL_DYNAMIC_COPY);

		gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		gl.useProgram(m_imageReadProgram->getProgram());

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *resultBuffer);
		gl.bindImageTexture(2, m_imageID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
		gl.dispatchCompute(m_workSize, m_workSize, 1);

		GLU_EXPECT_NO_ERROR(gl.getError(), "read");
	}

	// Read ssbo
	{
		const void* ptr = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (int)(sizeof(deUint32) * m_workSize * m_workSize), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "map");

		if (!ptr)
			throw tcu::TestError("mapBufferRange returned NULL");

		result.resize(m_workSize * m_workSize);
		memcpy(&result[0], ptr, sizeof(deUint32) * m_workSize * m_workSize);

		if (gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER) == GL_FALSE)
			throw tcu::TestError("unmapBuffer returned false");
	}
}

std::string ConcurrentImageAtomicCase::genComputeSource (void) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "${SHADER_IMAGE_ATOMIC_REQUIRE}\n"
		<< "\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout (binding = 1, std430) writeonly buffer IntermediateResults\n"
		<< "{\n"
		<< "	highp uint values[" << m_workSize * m_workSize << "];\n"
		<< "} sb_ires;\n"
		<< "\n"
		<< "layout (binding = 2, r32ui) volatile uniform highp uimage2D u_workImage;\n"
		<< "uniform highp uint u_atomicDelta;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	highp uint invocationIndex = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * uint(" << m_workSize <<");\n"
		<< "	sb_ires.values[invocationIndex] = imageAtomicAdd(u_workImage, ivec2(gl_GlobalInvocationID.xy), u_atomicDelta);\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

std::string ConcurrentImageAtomicCase::genImageReadSource (void) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout (binding = 1, std430) writeonly buffer ImageValues\n"
		<< "{\n"
		<< "	highp uint values[" << m_workSize * m_workSize << "];\n"
		<< "} sb_res;\n"
		<< "\n"
		<< "layout (binding = 2, r32ui) readonly uniform highp uimage2D u_workImage;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	highp uint invocationIndex = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * uint(" << m_workSize <<");\n"
		<< "	sb_res.values[invocationIndex] = imageLoad(u_workImage, ivec2(gl_GlobalInvocationID.xy)).x;\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

std::string ConcurrentImageAtomicCase::genImageClearSource (void) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout (binding = 2, r32ui) writeonly uniform highp uimage2D u_workImage;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	imageStore(u_workImage, ivec2(gl_GlobalInvocationID.xy), uvec4(0, 0, 0, 0));\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

class ConcurrentSSBOAtomicCounterMixedCase : public TestCase
{
public:
							ConcurrentSSBOAtomicCounterMixedCase	(Context& context, const char* name, const char* description, int numCalls, int workSize);
							~ConcurrentSSBOAtomicCounterMixedCase	(void);

	void					init									(void);
	void					deinit									(void);
	IterateResult			iterate									(void);

private:
	std::string				genSSBOComputeSource					(void) const;
	std::string				genAtomicCounterComputeSource			(void) const;

	const int				m_numCalls;
	const int				m_workSize;
	deUint32				m_bufferID;
	glu::ShaderProgram*		m_ssboAtomicProgram;
	glu::ShaderProgram*		m_atomicCounterProgram;
};

ConcurrentSSBOAtomicCounterMixedCase::ConcurrentSSBOAtomicCounterMixedCase (Context& context, const char* name, const char* description, int numCalls, int workSize)
	: TestCase					(context, name, description)
	, m_numCalls				(numCalls)
	, m_workSize				(workSize)
	, m_bufferID				(DE_NULL)
	, m_ssboAtomicProgram		(DE_NULL)
	, m_atomicCounterProgram	(DE_NULL)
{
	// SSBO atomic XORs cancel out
	DE_ASSERT((workSize * numCalls) % (16 * 2) == 0);
}

ConcurrentSSBOAtomicCounterMixedCase::~ConcurrentSSBOAtomicCounterMixedCase (void)
{
	deinit();
}

void ConcurrentSSBOAtomicCounterMixedCase::init (void)
{
	const glw::Functions&		gl			= m_context.getRenderContext().getFunctions();
	const deUint32				zeroBuf[2]	= { 0, 0 };

	// gen buffer

	gl.genBuffers(1, &m_bufferID);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_bufferID);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, (int)(sizeof(deUint32) * 2), zeroBuf, GL_DYNAMIC_COPY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffers");

	// gen programs

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "SSBOProgram", "SSBO atomic program");

		m_ssboAtomicProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genSSBOComputeSource()));
		m_testCtx.getLog() << *m_ssboAtomicProgram;
		if (!m_ssboAtomicProgram->isOk())
			throw tcu::TestError("could not build program");
	}
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "AtomicCounterProgram", "Atomic counter program");

		m_atomicCounterProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genAtomicCounterComputeSource()));
		m_testCtx.getLog() << *m_atomicCounterProgram;
		if (!m_atomicCounterProgram->isOk())
			throw tcu::TestError("could not build program");
	}
}

void ConcurrentSSBOAtomicCounterMixedCase::deinit (void)
{
	if (m_bufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_bufferID);
		m_bufferID = 0;
	}

	delete m_ssboAtomicProgram;
	m_ssboAtomicProgram = DE_NULL;

	delete m_atomicCounterProgram;
	m_atomicCounterProgram = DE_NULL;
}

TestCase::IterateResult ConcurrentSSBOAtomicCounterMixedCase::iterate (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog() << tcu::TestLog::Message << "Testing atomic counters and SSBO atomic operations with both backed by the same buffer." << tcu::TestLog::EndMessage;

	// invoke programs N times
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Running SSBO atomic program and atomic counter program " << m_numCalls << " times. (interleaved)\n"
			<< "Num groups = (" << m_workSize << ", 1, 1)\n"
			<< tcu::TestLog::EndMessage;

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_bufferID);
		gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_bufferID);

		for (int callNdx = 0; callNdx < m_numCalls; ++callNdx)
		{
			gl.useProgram(m_atomicCounterProgram->getProgram());
			gl.dispatchCompute(m_workSize, 1, 1);

			gl.useProgram(m_ssboAtomicProgram->getProgram());
			gl.dispatchCompute(m_workSize, 1, 1);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "post dispatch");
	}

	// Verify result
	{
		deUint32 result;

		// XORs cancel out, only addition is left
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying work buffer, it should be " << m_numCalls*m_workSize << tcu::TestLog::EndMessage;

		gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_bufferID);
		result = readBufferUint32(gl, GL_ATOMIC_COUNTER_BUFFER);

		if ((int)result != m_numCalls*m_workSize)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Buffer value error, expected value " << (m_numCalls*m_workSize) << ", got " << result << "\n"
				<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents invalid");
			return STOP;
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Buffer is valid." << tcu::TestLog::EndMessage;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

std::string ConcurrentSSBOAtomicCounterMixedCase::genSSBOComputeSource (void) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout (binding = 1, std430) volatile buffer WorkBuffer\n"
		<< "{\n"
		<< "	highp uint targetValue;\n"
		<< "	highp uint dummy;\n"
		<< "} sb_work;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	// flip high bits\n"
		<< "	highp uint mask = uint(1) << (24u + (gl_GlobalInvocationID.x % 8u));\n"
		<< "	sb_work.dummy = atomicXor(sb_work.targetValue, mask);\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

std::string ConcurrentSSBOAtomicCounterMixedCase::genAtomicCounterComputeSource (void) const
{
	std::ostringstream buf;

	buf	<< "${GLSL_VERSION_DECL}\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "\n"
		<< "layout (binding = 2, offset = 0) uniform atomic_uint u_counter;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	atomicCounterIncrement(u_counter);\n"
		<< "}";

	return specializeShader(m_context, buf.str().c_str());
}

} // anonymous

SynchronizationTests::SynchronizationTests (Context& context)
	: TestCaseGroup(context, "synchronization", "Synchronization tests")
{
}

SynchronizationTests::~SynchronizationTests (void)
{
}

void SynchronizationTests::init (void)
{
	tcu::TestCaseGroup* const inInvocationGroup		= new tcu::TestCaseGroup(m_testCtx, "in_invocation",	"Test intra-invocation synchronization");
	tcu::TestCaseGroup* const interInvocationGroup	= new tcu::TestCaseGroup(m_testCtx, "inter_invocation", "Test inter-invocation synchronization");
	tcu::TestCaseGroup* const interCallGroup		= new tcu::TestCaseGroup(m_testCtx, "inter_call",       "Test inter-call synchronization");

	addChild(inInvocationGroup);
	addChild(interInvocationGroup);
	addChild(interCallGroup);

	// .in_invocation & .inter_invocation
	{
		static const struct CaseConfig
		{
			const char*									namePrefix;
			const InterInvocationTestCase::StorageType	storage;
			const int									flags;
		} configs[] =
		{
			{ "image",			InterInvocationTestCase::STORAGE_IMAGE,		0										},
			{ "image_atomic",	InterInvocationTestCase::STORAGE_IMAGE,		InterInvocationTestCase::FLAG_ATOMIC	},
			{ "ssbo",			InterInvocationTestCase::STORAGE_BUFFER,	0										},
			{ "ssbo_atomic",	InterInvocationTestCase::STORAGE_BUFFER,	InterInvocationTestCase::FLAG_ATOMIC	},
		};

		for (int groupNdx = 0; groupNdx < 2; ++groupNdx)
		{
			tcu::TestCaseGroup* const	targetGroup	= (groupNdx == 0) ? (inInvocationGroup) : (interInvocationGroup);
			const int					extraFlags	= (groupNdx == 0) ? (0) : (InterInvocationTestCase::FLAG_IN_GROUP);

			for (int configNdx = 0; configNdx < DE_LENGTH_OF_ARRAY(configs); ++configNdx)
			{
				const char* const target = (configs[configNdx].storage == InterInvocationTestCase::STORAGE_BUFFER) ? ("buffer") : ("image");

				targetGroup->addChild(new InvocationWriteReadCase(m_context,
																  (std::string(configs[configNdx].namePrefix) + "_write_read").c_str(),
																  (std::string("Write to ") + target + " and read it").c_str(),
																  configs[configNdx].storage,
																  configs[configNdx].flags | extraFlags));

				targetGroup->addChild(new InvocationReadWriteCase(m_context,
																  (std::string(configs[configNdx].namePrefix) + "_read_write").c_str(),
																  (std::string("Read form ") + target + " and then write to it").c_str(),
																  configs[configNdx].storage,
																  configs[configNdx].flags | extraFlags));

				targetGroup->addChild(new InvocationOverWriteCase(m_context,
																  (std::string(configs[configNdx].namePrefix) + "_overwrite").c_str(),
																  (std::string("Write to ") + target + " twice and read it").c_str(),
																  configs[configNdx].storage,
																  configs[configNdx].flags | extraFlags));

				targetGroup->addChild(new InvocationAliasWriteCase(m_context,
																   (std::string(configs[configNdx].namePrefix) + "_alias_write").c_str(),
																   (std::string("Write to aliasing ") + target + " and read it").c_str(),
																   InvocationAliasWriteCase::TYPE_WRITE,
																   configs[configNdx].storage,
																   configs[configNdx].flags | extraFlags));

				targetGroup->addChild(new InvocationAliasWriteCase(m_context,
																   (std::string(configs[configNdx].namePrefix) + "_alias_overwrite").c_str(),
																   (std::string("Write to aliasing ") + target + "s and read it").c_str(),
																   InvocationAliasWriteCase::TYPE_OVERWRITE,
																   configs[configNdx].storage,
																   configs[configNdx].flags | extraFlags));
			}
		}
	}

	// .inter_call
	{
		tcu::TestCaseGroup* const withBarrierGroup		= new tcu::TestCaseGroup(m_testCtx, "with_memory_barrier", "Synchronize with memory barrier");
		tcu::TestCaseGroup* const withoutBarrierGroup	= new tcu::TestCaseGroup(m_testCtx, "without_memory_barrier", "Synchronize without memory barrier");

		interCallGroup->addChild(withBarrierGroup);
		interCallGroup->addChild(withoutBarrierGroup);

		// .with_memory_barrier
		{
			static const struct CaseConfig
			{
				const char*								namePrefix;
				const InterCallTestCase::StorageType	storage;
				const int								flags;
			} configs[] =
			{
				{ "image",			InterCallTestCase::STORAGE_IMAGE,	0																		},
				{ "image_atomic",	InterCallTestCase::STORAGE_IMAGE,	InterCallTestCase::FLAG_USE_ATOMIC | InterCallTestCase::FLAG_USE_INT	},
				{ "ssbo",			InterCallTestCase::STORAGE_BUFFER,	0																		},
				{ "ssbo_atomic",	InterCallTestCase::STORAGE_BUFFER,	InterCallTestCase::FLAG_USE_ATOMIC | InterCallTestCase::FLAG_USE_INT	},
			};

			const int seed0 = 123;
			const int seed1 = 457;

			for (int configNdx = 0; configNdx < DE_LENGTH_OF_ARRAY(configs); ++configNdx)
			{
				const char* const target = (configs[configNdx].storage == InterCallTestCase::STORAGE_BUFFER) ? ("buffer") : ("image");

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_write_read").c_str(),
																 (std::string("Write to ") + target + " and read it").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::WriteData::Generate(1, seed0)
																	<< op::Barrier()
																	<< op::ReadData::Generate(1, seed0)));

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_read_write").c_str(),
																 (std::string("Read from ") + target + " and then write to it").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::ReadZeroData::Generate(1)
																	<< op::Barrier()
																	<< op::WriteData::Generate(1, seed0)));

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_overwrite").c_str(),
																 (std::string("Write to ") + target + " twice and read it").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::WriteData::Generate(1, seed0)
																	<< op::Barrier()
																	<< op::WriteData::Generate(1, seed1)
																	<< op::Barrier()
																	<< op::ReadData::Generate(1, seed1)));

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_multiple_write_read").c_str(),
																 (std::string("Write to multiple ") + target + "s and read them").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::WriteData::Generate(1, seed0)
																	<< op::WriteData::Generate(2, seed1)
																	<< op::Barrier()
																	<< op::ReadMultipleData::Generate(1, seed0, 2, seed1)));

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_multiple_interleaved_write_read").c_str(),
																 (std::string("Write to same ") + target + " in multiple calls and read it").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::WriteDataInterleaved::Generate(1, seed0, true)
																	<< op::WriteDataInterleaved::Generate(1, seed1, false)
																	<< op::Barrier()
																	<< op::ReadDataInterleaved::Generate(1, seed0, seed1)));

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_multiple_unrelated_write_read_ordered").c_str(),
																 (std::string("Two unrelated ") + target + " write-reads").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::WriteData::Generate(1, seed0)
																	<< op::WriteData::Generate(2, seed1)
																	<< op::Barrier()
																	<< op::ReadData::Generate(1, seed0)
																	<< op::ReadData::Generate(2, seed1)));

				withBarrierGroup->addChild(new InterCallTestCase(m_context,
																 (std::string(configs[configNdx].namePrefix) + "_multiple_unrelated_write_read_non_ordered").c_str(),
																 (std::string("Two unrelated ") + target + " write-reads").c_str(),
																 configs[configNdx].storage,
																 configs[configNdx].flags,
																 InterCallOperations()
																	<< op::WriteData::Generate(1, seed0)
																	<< op::WriteData::Generate(2, seed1)
																	<< op::Barrier()
																	<< op::ReadData::Generate(2, seed1)
																	<< op::ReadData::Generate(1, seed0)));
			}

			// .without_memory_barrier
			{
				struct InvocationConfig
				{
					const char*	name;
					int			count;
				};

				static const InvocationConfig ssboInvocations[] =
				{
					{ "1k",		1024	},
					{ "4k",		4096	},
					{ "32k",	32768	},
				};
				static const InvocationConfig imageInvocations[] =
				{
					{ "8x8",		8	},
					{ "32x32",		32	},
					{ "128x128",	128	},
				};
				static const InvocationConfig counterInvocations[] =
				{
					{ "32",		32		},
					{ "128",	128		},
					{ "1k",		1024	},
				};
				static const int callCounts[] = { 2, 5, 100 };

				for (int invocationNdx = 0; invocationNdx < DE_LENGTH_OF_ARRAY(ssboInvocations); ++invocationNdx)
					for (int callCountNdx = 0; callCountNdx < DE_LENGTH_OF_ARRAY(callCounts); ++callCountNdx)
						withoutBarrierGroup->addChild(new SSBOConcurrentAtomicCase(m_context, (std::string("ssbo_atomic_dispatch_") + de::toString(callCounts[callCountNdx]) + "_calls_" + ssboInvocations[invocationNdx].name + "_invocations").c_str(),	"", callCounts[callCountNdx], ssboInvocations[invocationNdx].count));

				for (int invocationNdx = 0; invocationNdx < DE_LENGTH_OF_ARRAY(imageInvocations); ++invocationNdx)
					for (int callCountNdx = 0; callCountNdx < DE_LENGTH_OF_ARRAY(callCounts); ++callCountNdx)
						withoutBarrierGroup->addChild(new ConcurrentImageAtomicCase(m_context, (std::string("image_atomic_dispatch_") + de::toString(callCounts[callCountNdx]) + "_calls_" + imageInvocations[invocationNdx].name + "_invocations").c_str(),	"", callCounts[callCountNdx], imageInvocations[invocationNdx].count));

				for (int invocationNdx = 0; invocationNdx < DE_LENGTH_OF_ARRAY(counterInvocations); ++invocationNdx)
					for (int callCountNdx = 0; callCountNdx < DE_LENGTH_OF_ARRAY(callCounts); ++callCountNdx)
						withoutBarrierGroup->addChild(new ConcurrentAtomicCounterCase(m_context, (std::string("atomic_counter_dispatch_") + de::toString(callCounts[callCountNdx]) + "_calls_" + counterInvocations[invocationNdx].name + "_invocations").c_str(),	"", callCounts[callCountNdx], counterInvocations[invocationNdx].count));

				for (int invocationNdx = 0; invocationNdx < DE_LENGTH_OF_ARRAY(counterInvocations); ++invocationNdx)
					for (int callCountNdx = 0; callCountNdx < DE_LENGTH_OF_ARRAY(callCounts); ++callCountNdx)
						withoutBarrierGroup->addChild(new ConcurrentSSBOAtomicCounterMixedCase(m_context, (std::string("ssbo_atomic_counter_mixed_dispatch_") + de::toString(callCounts[callCountNdx]) + "_calls_" + counterInvocations[invocationNdx].name + "_invocations").c_str(),	"", callCounts[callCountNdx], counterInvocations[invocationNdx].count));
			}
		}
	}
}

} // Functional
} // gles31
} // deqp
