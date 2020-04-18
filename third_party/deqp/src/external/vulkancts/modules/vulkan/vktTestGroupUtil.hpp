#ifndef _VKTTESTGROUPUTIL_HPP
#define _VKTTESTGROUPUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief TestCaseGroup utilities
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuTestCase.hpp"

namespace vkt
{

class TestGroupHelper0 : public tcu::TestCaseGroup
{
public:
	typedef void (*CreateChildrenFunc) (tcu::TestCaseGroup* testGroup);

								TestGroupHelper0	(tcu::TestContext&		testCtx,
													 const std::string&		name,
													 const std::string&		description,
													 CreateChildrenFunc		createChildren);
								~TestGroupHelper0	(void);

	void						init				(void);

private:
	const CreateChildrenFunc	m_createChildren;
};

template<typename Arg0>
class TestGroupHelper1 : public tcu::TestCaseGroup
{
public:
	typedef void (*CreateChildrenFunc) (tcu::TestCaseGroup* testGroup, Arg0 arg0);

								TestGroupHelper1	(tcu::TestContext&		testCtx,
													 const std::string&		name,
													 const std::string&		description,
													 CreateChildrenFunc		createChildren,
													 const Arg0&			arg0)
									: tcu::TestCaseGroup	(testCtx, name.c_str(), description.c_str())
									, m_createChildren		(createChildren)
									, m_arg0				(arg0)
								{}

	void						init				(void) { m_createChildren(this, m_arg0); }

private:
	const CreateChildrenFunc	m_createChildren;
	const Arg0					m_arg0;
};

inline tcu::TestCaseGroup* createTestGroup (tcu::TestContext&						testCtx,
											const std::string&						name,
											const std::string&						description,
											TestGroupHelper0::CreateChildrenFunc	createChildren)
{
	return new TestGroupHelper0(testCtx, name, description, createChildren);
}

template<typename Arg0>
tcu::TestCaseGroup* createTestGroup (tcu::TestContext&										testCtx,
									 const std::string&										name,
									 const std::string&										description,
									 typename TestGroupHelper1<Arg0>::CreateChildrenFunc	createChildren,
									 Arg0													arg0)
{
	return new TestGroupHelper1<Arg0>(testCtx, name, description, createChildren, arg0);
}

inline void addTestGroup (tcu::TestCaseGroup*					parent,
						  const std::string&					name,
						  const std::string&					description,
						  TestGroupHelper0::CreateChildrenFunc	createChildren)
{
	parent->addChild(createTestGroup(parent->getTestContext(), name, description, createChildren));
}

template<typename Arg0>
void addTestGroup (tcu::TestCaseGroup*									parent,
				   const std::string&									name,
				   const std::string&									description,
				   typename TestGroupHelper1<Arg0>::CreateChildrenFunc	createChildren,
				   Arg0													arg0)
{
	parent->addChild(createTestGroup<Arg0>(parent->getTestContext(), name, description, createChildren, arg0));
}

} // vkt

#endif // _VKTTESTGROUPUTIL_HPP
