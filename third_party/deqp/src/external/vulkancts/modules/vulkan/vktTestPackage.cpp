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
 * \brief Vulkan Test Package
 *//*--------------------------------------------------------------------*/

#include "vktTestPackage.hpp"

#include "tcuPlatform.hpp"
#include "tcuTestCase.hpp"
#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"

#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkBinaryRegistry.hpp"
#include "vkShaderToSpirV.hpp"
#include "vkDebugReportUtil.hpp"
#include "vkQueryUtil.hpp"

#include "deUniquePtr.hpp"

#include "vktTestGroupUtil.hpp"
#include "vktApiTests.hpp"
#include "vktPipelineTests.hpp"
#include "vktBindingModelTests.hpp"
#include "vktSpvAsmTests.hpp"
#include "vktShaderLibrary.hpp"
#include "vktRenderPassTests.hpp"
#include "vktMemoryTests.hpp"
#include "vktShaderRenderBuiltinVarTests.hpp"
#include "vktShaderRenderDerivateTests.hpp"
#include "vktShaderRenderDiscardTests.hpp"
#include "vktShaderRenderIndexingTests.hpp"
#include "vktShaderRenderLoopTests.hpp"
#include "vktShaderRenderMatrixTests.hpp"
#include "vktShaderRenderOperatorTests.hpp"
#include "vktShaderRenderReturnTests.hpp"
#include "vktShaderRenderStructTests.hpp"
#include "vktShaderRenderSwitchTests.hpp"
#include "vktShaderRenderTextureFunctionTests.hpp"
#include "vktShaderRenderTextureGatherTests.hpp"
#include "vktShaderBuiltinTests.hpp"
#include "vktOpaqueTypeIndexingTests.hpp"
#include "vktAtomicOperationTests.hpp"
#include "vktUniformBlockTests.hpp"
#include "vktDynamicStateTests.hpp"
#include "vktSSBOLayoutTests.hpp"
#include "vktQueryPoolTests.hpp"
#include "vktDrawTests.hpp"
#include "vktComputeTests.hpp"
#include "vktImageTests.hpp"
#include "vktInfoTests.hpp"
#include "vktWsiTests.hpp"
#include "vktSynchronizationTests.hpp"
#include "vktSparseResourcesTests.hpp"
#include "vktTessellationTests.hpp"
#include "vktRasterizationTests.hpp"
#include "vktClippingTests.hpp"
#include "vktFragmentOperationsTests.hpp"
#include "vktTextureTests.hpp"
#include "vktGeometryTests.hpp"
#include "vktRobustnessTests.hpp"
#include "vktYCbCrTests.hpp"

#include <vector>
#include <sstream>

namespace // compilation
{

vk::ProgramBinary* compileProgram (const vk::GlslSource& source, glu::ShaderProgramInfo* buildInfo)
{
	return vk::buildProgram(source, buildInfo);
}

vk::ProgramBinary* compileProgram (const vk::HlslSource& source, glu::ShaderProgramInfo* buildInfo)
{
	return vk::buildProgram(source, buildInfo);
}

vk::ProgramBinary* compileProgram (const vk::SpirVAsmSource& source, vk::SpirVProgramInfo* buildInfo)
{
	return vk::assembleProgram(source, buildInfo);
}

template <typename InfoType, typename IteratorType>
vk::ProgramBinary* buildProgram (const std::string&					casePath,
								 IteratorType						iter,
								 const vk::BinaryRegistryReader&	prebuiltBinRegistry,
								 tcu::TestLog&						log,
								 vk::BinaryCollection*				progCollection)
{
	const vk::ProgramIdentifier		progId		(casePath, iter.getName());
	const tcu::ScopedLogSection		progSection	(log, iter.getName(), "Program: " + iter.getName());
	de::MovePtr<vk::ProgramBinary>	binProg;
	InfoType						buildInfo;

	try
	{
		binProg	= de::MovePtr<vk::ProgramBinary>(compileProgram(iter.getProgram(), &buildInfo));
		log << buildInfo;
	}
	catch (const tcu::NotSupportedError& err)
	{
		// Try to load from cache
		log << err << tcu::TestLog::Message << "Building from source not supported, loading stored binary instead" << tcu::TestLog::EndMessage;

		binProg = de::MovePtr<vk::ProgramBinary>(prebuiltBinRegistry.loadProgram(progId));

		log << iter.getProgram();
	}
	catch (const tcu::Exception&)
	{
		// Build failed for other reason
		log << buildInfo;
		throw;
	}

	TCU_CHECK_INTERNAL(binProg);

	{
		vk::ProgramBinary* const	returnBinary	= binProg.get();

		progCollection->add(progId.programName, binProg);

		return returnBinary;
	}
}

} // anonymous(compilation)

namespace vkt
{

using std::vector;
using de::UniquePtr;
using de::MovePtr;
using tcu::TestLog;

namespace
{

MovePtr<vk::DebugReportRecorder> createDebugReportRecorder (const vk::PlatformInterface& vkp, const vk::InstanceInterface& vki, vk::VkInstance instance)
{
	if (isDebugReportSupported(vkp))
		return MovePtr<vk::DebugReportRecorder>(new vk::DebugReportRecorder(vki, instance));
	else
		TCU_THROW(NotSupportedError, "VK_EXT_debug_report is not supported");
}

} // anonymous

// TestCaseExecutor

class TestCaseExecutor : public tcu::TestCaseExecutor
{
public:
												TestCaseExecutor	(tcu::TestContext& testCtx);
												~TestCaseExecutor	(void);

	virtual void								init				(tcu::TestCase* testCase, const std::string& path);
	virtual void								deinit				(tcu::TestCase* testCase);

	virtual tcu::TestNode::IterateResult		iterate				(tcu::TestCase* testCase);

private:
	vk::BinaryCollection						m_progCollection;
	vk::BinaryRegistryReader					m_prebuiltBinRegistry;

	const UniquePtr<vk::Library>				m_library;
	Context										m_context;

	const UniquePtr<vk::DebugReportRecorder>	m_debugReportRecorder;

	TestInstance*								m_instance;			//!< Current test case instance
};

static MovePtr<vk::Library> createLibrary (tcu::TestContext& testCtx)
{
	return MovePtr<vk::Library>(testCtx.getPlatform().getVulkanPlatform().createLibrary());
}

TestCaseExecutor::TestCaseExecutor (tcu::TestContext& testCtx)
	: m_prebuiltBinRegistry	(testCtx.getArchive(), "vulkan/prebuilt")
	, m_library				(createLibrary(testCtx))
	, m_context				(testCtx, m_library->getPlatformInterface(), m_progCollection)
	, m_debugReportRecorder	(testCtx.getCommandLine().isValidationEnabled()
							 ? createDebugReportRecorder(m_library->getPlatformInterface(),
														 m_context.getInstanceInterface(),
														 m_context.getInstance())
							 : MovePtr<vk::DebugReportRecorder>(DE_NULL))
	, m_instance			(DE_NULL)
{
}

TestCaseExecutor::~TestCaseExecutor (void)
{
	delete m_instance;
}

void TestCaseExecutor::init (tcu::TestCase* testCase, const std::string& casePath)
{
	const TestCase*			vktCase		= dynamic_cast<TestCase*>(testCase);
	tcu::TestLog&			log			= m_context.getTestContext().getLog();
	vk::SourceCollections	sourceProgs;

	DE_UNREF(casePath); // \todo [2015-03-13 pyry] Use this to identify ProgramCollection storage path

	if (!vktCase)
		TCU_THROW(InternalError, "Test node not an instance of vkt::TestCase");

	m_progCollection.clear();
	vktCase->initPrograms(sourceProgs);

	for (vk::GlslSourceCollection::Iterator progIter = sourceProgs.glslSources.begin(); progIter != sourceProgs.glslSources.end(); ++progIter)
	{
		const vk::ProgramBinary* const binProg = buildProgram<glu::ShaderProgramInfo, vk::GlslSourceCollection::Iterator>(casePath, progIter, m_prebuiltBinRegistry, log, &m_progCollection);

		try
		{
			std::ostringstream disasm;

			vk::disassembleProgram(*binProg, &disasm);

			log << vk::SpirVAsmSource(disasm.str());
		}
		catch (const tcu::NotSupportedError& err)
		{
			log << err;
		}
	}

	for (vk::HlslSourceCollection::Iterator progIter = sourceProgs.hlslSources.begin(); progIter != sourceProgs.hlslSources.end(); ++progIter)
	{
		const vk::ProgramBinary* const binProg = buildProgram<glu::ShaderProgramInfo, vk::HlslSourceCollection::Iterator>(casePath, progIter, m_prebuiltBinRegistry, log, &m_progCollection);

		try
		{
			std::ostringstream disasm;

			vk::disassembleProgram(*binProg, &disasm);

			log << vk::SpirVAsmSource(disasm.str());
		}
		catch (const tcu::NotSupportedError& err)
		{
			log << err;
		}
	}

	for (vk::SpirVAsmCollection::Iterator asmIterator = sourceProgs.spirvAsmSources.begin(); asmIterator != sourceProgs.spirvAsmSources.end(); ++asmIterator)
	{
		buildProgram<vk::SpirVProgramInfo, vk::SpirVAsmCollection::Iterator>(casePath, asmIterator, m_prebuiltBinRegistry, log, &m_progCollection);
	}

	DE_ASSERT(!m_instance);
	m_instance = vktCase->createInstance(m_context);
}

void TestCaseExecutor::deinit (tcu::TestCase*)
{
	delete m_instance;
	m_instance = DE_NULL;

	// Collect and report any debug messages
	if (m_debugReportRecorder)
	{
		// \note We are not logging INFORMATION and DEBUG messages
		static const vk::VkDebugReportFlagsEXT			errorFlags		= vk::VK_DEBUG_REPORT_ERROR_BIT_EXT;
		static const vk::VkDebugReportFlagsEXT			logFlags		= errorFlags
																		| vk::VK_DEBUG_REPORT_WARNING_BIT_EXT
																		| vk::VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

		typedef vk::DebugReportRecorder::MessageList	DebugMessages;

		const DebugMessages&	messages	= m_debugReportRecorder->getMessages();
		tcu::TestLog&			log			= m_context.getTestContext().getLog();

		if (messages.begin() != messages.end())
		{
			const tcu::ScopedLogSection	section		(log, "DebugMessages", "Debug Messages");
			int							numErrors	= 0;

			for (DebugMessages::const_iterator curMsg = messages.begin(); curMsg != messages.end(); ++curMsg)
			{
				if ((curMsg->flags & logFlags) != 0)
					log << tcu::TestLog::Message << *curMsg << tcu::TestLog::EndMessage;

				if ((curMsg->flags & errorFlags) != 0)
					numErrors += 1;
			}

			m_debugReportRecorder->clearMessages();

			if (numErrors > 0)
				m_context.getTestContext().setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, (de::toString(numErrors) + " API usage errors found").c_str());
		}
	}
}

tcu::TestNode::IterateResult TestCaseExecutor::iterate (tcu::TestCase*)
{
	DE_ASSERT(m_instance);

	const tcu::TestStatus	result	= m_instance->iterate();

	if (result.isComplete())
	{
		// Vulkan tests shouldn't set result directly
		DE_ASSERT(m_context.getTestContext().getTestResult() == QP_TEST_RESULT_LAST);
		m_context.getTestContext().setTestResult(result.getCode(), result.getDescription().c_str());
		return tcu::TestNode::STOP;
	}
	else
		return tcu::TestNode::CONTINUE;
}

// GLSL shader tests

void createGlslTests (tcu::TestCaseGroup* glslTests)
{
	tcu::TestContext&	testCtx		= glslTests->getTestContext();

	// ShaderLibrary-based tests
	static const struct
	{
		const char*		name;
		const char*		description;
	} s_es310Tests[] =
	{
		{ "arrays",						"Arrays"					},
		{ "conditionals",				"Conditional statements"	},
		{ "constant_expressions",		"Constant expressions"		},
		{ "constants",					"Constants"					},
		{ "conversions",				"Type conversions"			},
		{ "functions",					"Functions"					},
		{ "linkage",					"Linking"					},
		{ "scoping",					"Scoping"					},
		{ "swizzles",					"Swizzles"					},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_es310Tests); ndx++)
		glslTests->addChild(createShaderLibraryGroup(testCtx,
													 s_es310Tests[ndx].name,
													 s_es310Tests[ndx].description,
													 std::string("vulkan/glsl/es310/") + s_es310Tests[ndx].name + ".test").release());

	static const struct
	{
		const char*		name;
		const char*		description;
	} s_440Tests[] =
	{
		{ "linkage",					"Linking"					},
	};

	de::MovePtr<tcu::TestCaseGroup> glsl440Tests = de::MovePtr<tcu::TestCaseGroup>(new tcu::TestCaseGroup(testCtx, "440", ""));

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_440Tests); ndx++)
		glsl440Tests->addChild(createShaderLibraryGroup(testCtx,
													 s_440Tests[ndx].name,
													 s_440Tests[ndx].description,
													 std::string("vulkan/glsl/440/") + s_440Tests[ndx].name + ".test").release());

	glslTests->addChild(glsl440Tests.release());

	// ShaderRenderCase-based tests
	glslTests->addChild(sr::createDerivateTests			(testCtx));
	glslTests->addChild(sr::createDiscardTests			(testCtx));
	glslTests->addChild(sr::createIndexingTests			(testCtx));
	glslTests->addChild(sr::createLoopTests				(testCtx));
	glslTests->addChild(sr::createMatrixTests			(testCtx));
	glslTests->addChild(sr::createOperatorTests			(testCtx));
	glslTests->addChild(sr::createReturnTests			(testCtx));
	glslTests->addChild(sr::createStructTests			(testCtx));
	glslTests->addChild(sr::createSwitchTests			(testCtx));
	glslTests->addChild(sr::createTextureFunctionTests	(testCtx));
	glslTests->addChild(sr::createTextureGatherTests	(testCtx));
	glslTests->addChild(sr::createBuiltinVarTests		(testCtx));

	// ShaderExecutor-based tests
	glslTests->addChild(shaderexecutor::createBuiltinTests				(testCtx));
	glslTests->addChild(shaderexecutor::createOpaqueTypeIndexingTests	(testCtx));
	glslTests->addChild(shaderexecutor::createAtomicOperationTests		(testCtx));
}

// TestPackage

TestPackage::TestPackage (tcu::TestContext& testCtx)
	: tcu::TestPackage(testCtx, "dEQP-VK", "dEQP Vulkan Tests")
{
}

TestPackage::~TestPackage (void)
{
}

tcu::TestCaseExecutor* TestPackage::createExecutor (void) const
{
	return new TestCaseExecutor(m_testCtx);
}

void TestPackage::init (void)
{
	addChild(createTestGroup				(m_testCtx, "info", "Build and Device Info Tests", createInfoTests));
	addChild(api::createTests				(m_testCtx));
	addChild(memory::createTests			(m_testCtx));
	addChild(pipeline::createTests			(m_testCtx));
	addChild(BindingModel::createTests		(m_testCtx));
	addChild(SpirVAssembly::createTests		(m_testCtx));
	addChild(createTestGroup				(m_testCtx, "glsl", "GLSL shader execution tests", createGlslTests));
	addChild(createRenderPassTests			(m_testCtx));
	addChild(ubo::createTests				(m_testCtx));
	addChild(DynamicState::createTests		(m_testCtx));
	addChild(ssbo::createTests				(m_testCtx));
	addChild(QueryPool::createTests			(m_testCtx));
	addChild(Draw::createTests				(m_testCtx));
	addChild(compute::createTests			(m_testCtx));
	addChild(image::createTests				(m_testCtx));
	addChild(wsi::createTests				(m_testCtx));
	addChild(synchronization::createTests	(m_testCtx));
	addChild(sparse::createTests			(m_testCtx));
	addChild(tessellation::createTests		(m_testCtx));
	addChild(rasterization::createTests		(m_testCtx));
	addChild(clipping::createTests			(m_testCtx));
	addChild(FragmentOperations::createTests(m_testCtx));
	addChild(texture::createTests			(m_testCtx));
	addChild(geometry::createTests			(m_testCtx));
	addChild(robustness::createTests		(m_testCtx));
	addChild(ycbcr::createTests				(m_testCtx));
}

} // vkt
