#ifndef _VKPROGRAMS_HPP
#define _VKPROGRAMS_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
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
 * \brief Program utilities.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkSpirVProgram.hpp"
#include "vkShaderProgram.hpp"

#include "deUniquePtr.hpp"
#include "deSTLUtil.hpp"

#include <vector>
#include <map>

namespace vk
{

enum ProgramFormat
{
	PROGRAM_FORMAT_SPIRV = 0,

	PROGRAM_FORMAT_LAST
};

class ProgramBinary
{
public:
								ProgramBinary	(ProgramFormat format, size_t binarySize, const deUint8* binary);

	ProgramFormat				getFormat		(void) const { return m_format;										}
	size_t						getSize			(void) const { return m_binary.size();								}
	const deUint8*				getBinary		(void) const { return m_binary.empty() ? DE_NULL : &m_binary[0];	}

private:
	const ProgramFormat			m_format;
	const std::vector<deUint8>	m_binary;
};

template<typename Program>
class ProgramCollection
{
public:
								ProgramCollection	(void);
								~ProgramCollection	(void);

	void						clear				(void);

	Program&					add					(const std::string& name);
	void						add					(const std::string& name, de::MovePtr<Program>& program);

	bool						contains			(const std::string& name) const;
	const Program&				get					(const std::string& name) const;

	class Iterator
	{
	private:
		typedef typename std::map<std::string, Program*>::const_iterator	IteratorImpl;

	public:
		explicit			Iterator	(const IteratorImpl& i) : m_impl(i) {}

		Iterator&			operator++	(void)			{ ++m_impl; return *this;	}
		const Program&		operator*	(void) const	{ return getProgram();		}

		const std::string&	getName		(void) const	{ return m_impl->first;		}
		const Program&		getProgram	(void) const	{ return *m_impl->second;	}

		bool				operator==	(const Iterator& other) const	{ return m_impl == other.m_impl;	}
		bool				operator!=	(const Iterator& other) const	{ return m_impl != other.m_impl;	}

	private:

		IteratorImpl	m_impl;
	};

	Iterator					begin				(void) const { return Iterator(m_programs.begin());	}
	Iterator					end					(void) const { return Iterator(m_programs.end());	}

private:
	typedef std::map<std::string, Program*>	ProgramMap;

	ProgramMap					m_programs;
};

template<typename Program>
ProgramCollection<Program>::ProgramCollection (void)
{
}

template<typename Program>
ProgramCollection<Program>::~ProgramCollection (void)
{
	clear();
}

template<typename Program>
void ProgramCollection<Program>::clear (void)
{
	for (typename ProgramMap::const_iterator i = m_programs.begin(); i != m_programs.end(); ++i)
		delete i->second;
	m_programs.clear();
}

template<typename Program>
Program& ProgramCollection<Program>::add (const std::string& name)
{
	DE_ASSERT(!contains(name));
	de::MovePtr<Program> prog = de::newMovePtr<Program>();
	m_programs[name] = prog.get();
	prog.release();
	return *m_programs[name];
}

template<typename Program>
void ProgramCollection<Program>::add (const std::string& name, de::MovePtr<Program>& program)
{
	DE_ASSERT(!contains(name));
	m_programs[name] = program.get();
	program.release();
}

template<typename Program>
bool ProgramCollection<Program>::contains (const std::string& name) const
{
	return de::contains(m_programs, name);
}

template<typename Program>
const Program& ProgramCollection<Program>::get (const std::string& name) const
{
	DE_ASSERT(contains(name));
	return *m_programs.find(name)->second;
}

typedef ProgramCollection<GlslSource>		GlslSourceCollection;
typedef ProgramCollection<HlslSource>		HlslSourceCollection;
typedef ProgramCollection<SpirVAsmSource>	SpirVAsmCollection;

struct SourceCollections
{
	GlslSourceCollection	glslSources;
	HlslSourceCollection	hlslSources;
	SpirVAsmCollection		spirvAsmSources;
};

typedef ProgramCollection<ProgramBinary>		BinaryCollection;

ProgramBinary*			buildProgram		(const GlslSource& program, glu::ShaderProgramInfo* buildInfo);
ProgramBinary*			buildProgram		(const HlslSource& program, glu::ShaderProgramInfo* buildInfo);
ProgramBinary*			assembleProgram		(const vk::SpirVAsmSource& program, SpirVProgramInfo* buildInfo);
void					disassembleProgram	(const ProgramBinary& program, std::ostream* dst);
bool					validateProgram		(const ProgramBinary& program, std::ostream* dst);

Move<VkShaderModule>	createShaderModule	(const DeviceInterface& deviceInterface, VkDevice device, const ProgramBinary& binary, VkShaderModuleCreateFlags flags);

glu::ShaderType			getGluShaderType	(VkShaderStageFlagBits shaderStage);
VkShaderStageFlagBits	getVkShaderStage	(glu::ShaderType shaderType);

} // vk

#endif // _VKPROGRAMS_HPP
