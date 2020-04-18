#ifndef _GLUPROGRAMINTERFACEQUERY_HPP
#define _GLUPROGRAMINTERFACEQUERY_HPP
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

#include "gluDefs.hpp"

#include <vector>
#include <string>

namespace glw
{
class Functions;
}

namespace glu
{

//! Interface block info.
struct InterfaceBlockInfo
{
	std::string			name;
	deUint32			index;
	deUint32			bufferBinding;		//!< GL_BUFFER_BINDING
	deUint32			dataSize;			//!< GL_BUFFER_DATA_SIZE
	std::vector<int>	activeVariables;	//!< GL_ACTIVE_VARIABLES

	InterfaceBlockInfo (void)
		: index				(~0u /* GL_INVALID_INDEX */)
		, bufferBinding		(0)
		, dataSize			(0)
	{
	}
};

//! Interface variable (uniform in uniform block, buffer variable) info.
struct InterfaceVariableInfo
{
	std::string			name;
	deUint32			index;
	deUint32			blockIndex;					//!< GL_BLOCK_INDEX
	deUint32			atomicCounterBufferIndex;	//!< GL_ATOMIC_COUNTER_BUFFER_INDEX
	deUint32			type;						//!< GL_TYPE
	deUint32			arraySize;					//!< GL_ARRAY_SIZE
	deUint32			offset;						//!< GL_OFFSET
	deInt32				arrayStride;				//!< GL_ARRAY_STRIDE
	deInt32				matrixStride;				//!< GL_MATRIX_STRIDE
	deUint32			topLevelArraySize;			//!< GL_TOP_LEVEL_ARRAY_SIZE	- set only for GL_BUFFER_VARIABLEs
	deInt32				topLevelArrayStride;		//!< GL_TOP_LEVEL_ARRAY_STRIDE	- set only for GL_BUFFER_VARIABLEs
	bool				isRowMajor;					//!< GL_IS_ROW_MAJOR

	InterfaceVariableInfo (void)
		: index						(~0u /* GL_INVALID_INDEX */)
		, blockIndex				(~0u /* GL_INVALID_INDEX */)
		, atomicCounterBufferIndex	(~0u /* GL_INVALID_INDEX */)
		, type						(0)
		, arraySize					(0)
		, offset					(0)
		, arrayStride				(0)
		, matrixStride				(0)
		, topLevelArraySize			(0)
		, topLevelArrayStride		(0)
		, isRowMajor				(0)
	{
	}
};


int						getProgramResourceInt			(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, deUint32 queryParam);
deUint32				getProgramResourceUint			(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, deUint32 queryParam);

void					getProgramResourceName			(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, std::string& dst);
std::string				getProgramResourceName			(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index);

void					getProgramInterfaceBlockInfo	(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, InterfaceBlockInfo& info);
InterfaceBlockInfo		getProgramInterfaceBlockInfo	(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index);

void					getProgramInterfaceVariableInfo	(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, InterfaceVariableInfo& info);
InterfaceVariableInfo	getProgramInterfaceVariableInfo	(const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index);

// Inline implementations for optimization (RVO in most cases).

inline int getProgramResourceInt (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index, deUint32 queryParam)
{
	return (int)getProgramResourceUint(gl, program, programInterface, index, queryParam);
}

inline std::string getProgramResourceName (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index)
{
	std::string name;
	getProgramResourceName(gl, program, programInterface, index, name);
	return name;
}

inline InterfaceBlockInfo getProgramInterfaceBlockInfo (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index)
{
	InterfaceBlockInfo info;
	getProgramInterfaceBlockInfo(gl, program, programInterface, index, info);
	return info;
}

inline InterfaceVariableInfo getProgramInterfaceVariableInfo (const glw::Functions& gl, deUint32 program, deUint32 programInterface, deUint32 index)
{
	InterfaceVariableInfo info;
	getProgramInterfaceVariableInfo(gl, program, programInterface, index, info);
	return info;
}

} // glu

#endif // _GLUPROGRAMINTERFACEQUERY_HPP
