#ifndef _ES31FPROGRAMINTERFACEQUERYTESTCASE_HPP
#define _ES31FPROGRAMINTERFACEQUERYTESTCASE_HPP
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
 * \brief Program interface query test case
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"
#include "es31fProgramInterfaceDefinition.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

struct ProgramResourceQueryTestTarget
{
						ProgramResourceQueryTestTarget (ProgramInterface interface_, deUint32 propFlags_);

	ProgramInterface	interface;
	deUint32			propFlags;
};

enum ProgramResourcePropFlags
{
	PROGRAMRESOURCEPROP_ARRAY_SIZE						= (1 <<  1),
	PROGRAMRESOURCEPROP_ARRAY_STRIDE					= (1 <<  2),
	PROGRAMRESOURCEPROP_ATOMIC_COUNTER_BUFFER_INDEX		= (1 <<  3),
	PROGRAMRESOURCEPROP_BLOCK_INDEX						= (1 <<  4),
	PROGRAMRESOURCEPROP_LOCATION						= (1 <<  5),
	PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR				= (1 <<  6),
	PROGRAMRESOURCEPROP_MATRIX_STRIDE					= (1 <<  7),
	PROGRAMRESOURCEPROP_NAME_LENGTH						= (1 <<  8),
	PROGRAMRESOURCEPROP_OFFSET							= (1 <<  9),
	PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			= (1 << 10),
	PROGRAMRESOURCEPROP_TYPE							= (1 << 11),
	PROGRAMRESOURCEPROP_BUFFER_BINDING					= (1 << 12),
	PROGRAMRESOURCEPROP_BUFFER_DATA_SIZE				= (1 << 13),
	PROGRAMRESOURCEPROP_ACTIVE_VARIABLES				= (1 << 14),
	PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_SIZE			= (1 << 15),
	PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_STRIDE			= (1 << 16),
	PROGRAMRESOURCEPROP_IS_PER_PATCH					= (1 << 17),

	PROGRAMRESOURCEPROP_UNIFORM_INTERFACE_MASK			= PROGRAMRESOURCEPROP_ARRAY_SIZE					|
														  PROGRAMRESOURCEPROP_ARRAY_STRIDE					|
														  PROGRAMRESOURCEPROP_ATOMIC_COUNTER_BUFFER_INDEX	|
														  PROGRAMRESOURCEPROP_BLOCK_INDEX					|
														  PROGRAMRESOURCEPROP_LOCATION						|
														  PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR				|
														  PROGRAMRESOURCEPROP_MATRIX_STRIDE					|
														  PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_OFFSET						|
														  PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			|
														  PROGRAMRESOURCEPROP_TYPE,

	PROGRAMRESOURCEPROP_UNIFORM_BLOCK_INTERFACE_MASK	= PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			|
														  PROGRAMRESOURCEPROP_BUFFER_BINDING				|
														  PROGRAMRESOURCEPROP_BUFFER_DATA_SIZE				|
														  PROGRAMRESOURCEPROP_ACTIVE_VARIABLES,

	PROGRAMRESOURCEPROP_SHADER_STORAGE_BLOCK_MASK		= PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			|
														  PROGRAMRESOURCEPROP_BUFFER_BINDING				|
														  PROGRAMRESOURCEPROP_BUFFER_DATA_SIZE				|
														  PROGRAMRESOURCEPROP_ACTIVE_VARIABLES,

	PROGRAMRESOURCEPROP_PROGRAM_INPUT_MASK				= PROGRAMRESOURCEPROP_ARRAY_SIZE					|
														  PROGRAMRESOURCEPROP_LOCATION						|
														  PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			|
														  PROGRAMRESOURCEPROP_TYPE							|
														  PROGRAMRESOURCEPROP_IS_PER_PATCH,

	PROGRAMRESOURCEPROP_PROGRAM_OUTPUT_MASK				= PROGRAMRESOURCEPROP_ARRAY_SIZE					|
														  PROGRAMRESOURCEPROP_LOCATION						|
														  PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			|
														  PROGRAMRESOURCEPROP_TYPE							|
														  PROGRAMRESOURCEPROP_IS_PER_PATCH,

	PROGRAMRESOURCEPROP_BUFFER_VARIABLE_MASK			= PROGRAMRESOURCEPROP_ARRAY_SIZE					|
														  PROGRAMRESOURCEPROP_ARRAY_STRIDE					|
														  PROGRAMRESOURCEPROP_BLOCK_INDEX					|
														  PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR				|
														  PROGRAMRESOURCEPROP_MATRIX_STRIDE					|
														  PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_OFFSET						|
														  PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER			|
														  PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_SIZE			|
														  PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_STRIDE		|
														  PROGRAMRESOURCEPROP_TYPE,

	PROGRAMRESOURCEPROP_TRANSFORM_FEEDBACK_VARYING_MASK	= PROGRAMRESOURCEPROP_ARRAY_SIZE					|
														  PROGRAMRESOURCEPROP_NAME_LENGTH					|
														  PROGRAMRESOURCEPROP_TYPE,
};

class ProgramInterfaceQueryTestCase : public TestCase
{
public:
														ProgramInterfaceQueryTestCase	(Context& context, const char* name, const char* description, ProgramResourceQueryTestTarget queryTarget);
														~ProgramInterfaceQueryTestCase	(void);

protected:
	ProgramInterface									getTargetInterface				(void) const;

private:
	const ProgramInterfaceDefinition::Program*			getAndCheckProgramDefinition	(void);
	int													getMaxPatchVertices				(void);
	IterateResult										iterate							(void);

	virtual const ProgramInterfaceDefinition::Program*	getProgramDefinition			(void) const = 0;
	virtual std::vector<std::string>					getQueryTargetResources			(void) const = 0;

	const ProgramResourceQueryTestTarget				m_queryTarget;
};

void checkProgramResourceUsage (const ProgramInterfaceDefinition::Program* program, const glw::Functions& gl, tcu::TestLog& log);

} // Functional
} // gles31
} // deqp

#endif // _ES31FPROGRAMINTERFACEQUERYTESTCASE_HPP
