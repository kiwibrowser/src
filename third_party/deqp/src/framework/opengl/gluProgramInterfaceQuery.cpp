/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL Utilities
 * ---------------------------------------------
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
 * \brief Program interface query utilities
 *//*--------------------------------------------------------------------*/

#include "gluProgramInterfaceQuery.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include <sstream>

namespace glu
{

deUint32 getProgramResourceUint (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, deUint32 queryParam)
{
	deUint32 value = 0;
	gl.getProgramResourceiv(program, programInterface, index, 1, &queryParam, 1, DE_NULL, (int*)&value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv()");
	return value;
}

void getProgramResourceName (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, std::string& dst)
{
	const int length = getProgramResourceInt(gl, program, programInterface, index, GL_NAME_LENGTH);

	if (length > 0)
	{
		std::vector<char> buf(length+1);
		gl.getProgramResourceName(program, programInterface, index, (int)buf.size(), DE_NULL, &buf[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceName()");

		dst = (const char*)&buf[0];
	}
	else
	{
		std::ostringstream msg;
		msg << "Empty name returned for " << programInterface << " at index " << index;
		throw tcu::TestError(msg.str());
	}
}

static void getProgramInterfaceActiveVariables (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, std::vector<int>& activeVariables)
{
	const int numActiveVariables = getProgramResourceInt(gl, program, programInterface, index, GL_NUM_ACTIVE_VARIABLES);

	activeVariables.resize(numActiveVariables);
	if (numActiveVariables > 0)
	{
		const deUint32 queryParam = GL_ACTIVE_VARIABLES;
		gl.getProgramResourceiv(program, programInterface, index, 1, &queryParam, (int)activeVariables.size(), DE_NULL, &activeVariables[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv(GL_PROGRAM_ACTIVE_VARIABLES)");
	}
}

void getProgramInterfaceBlockInfo (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, InterfaceBlockInfo& info)
{
	info.index			= index;
	info.bufferBinding	= getProgramResourceUint(gl, program, programInterface, index, GL_BUFFER_BINDING);
	info.dataSize		= getProgramResourceUint(gl, program, programInterface, index, GL_BUFFER_DATA_SIZE);

	getProgramInterfaceActiveVariables(gl, program, programInterface, index, info.activeVariables);

	if (programInterface != GL_ATOMIC_COUNTER_BUFFER)
		getProgramResourceName(gl, program, programInterface, index, info.name);
}

void getProgramInterfaceVariableInfo (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, InterfaceVariableInfo& info)
{
	// \todo [2013-08-27 pyry] Batch queries!
	info.index			= index;
	info.blockIndex		= getProgramResourceUint(gl, program, programInterface, index, GL_BLOCK_INDEX);
	info.type			= getProgramResourceUint(gl, program, programInterface, index, GL_TYPE);
	info.arraySize		= getProgramResourceUint(gl, program, programInterface, index, GL_ARRAY_SIZE);
	info.offset			= getProgramResourceUint(gl, program, programInterface, index, GL_OFFSET);
	info.arrayStride	= getProgramResourceUint(gl, program, programInterface, index, GL_ARRAY_STRIDE);
	info.matrixStride	= getProgramResourceUint(gl, program, programInterface, index, GL_MATRIX_STRIDE);
	info.isRowMajor		= getProgramResourceUint(gl, program, programInterface, index, GL_IS_ROW_MAJOR) != GL_FALSE;

	if (programInterface == GL_UNIFORM)
		info.atomicCounterBufferIndex = getProgramResourceUint(gl, program, programInterface, index, GL_ATOMIC_COUNTER_BUFFER_INDEX);

	if (programInterface == GL_BUFFER_VARIABLE)
	{
		info.topLevelArraySize		= getProgramResourceUint(gl, program, programInterface, index, GL_TOP_LEVEL_ARRAY_SIZE);
		info.topLevelArrayStride	= getProgramResourceUint(gl, program, programInterface, index, GL_TOP_LEVEL_ARRAY_STRIDE);
	}

	getProgramResourceName(gl, program, programInterface, index, info.name);
}

} // glu
