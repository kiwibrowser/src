/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief Base class for a test case.
 *//*--------------------------------------------------------------------*/

#include "tcuTestCase.hpp"
#include "tcuPlatform.hpp"

#include "deString.h"

namespace tcu
{

using namespace std;

// TestNode.

inline bool isValidCaseName (const char* name)
{
	for (const char* p = name; *p != '\0'; p++)
	{
		if (!isValidTestCaseNameChar(*p))
			return false;
	}
	return true;
}

TestNode::TestNode (TestContext& testCtx, TestNodeType nodeType, const char* name, const char* description)
	: m_testCtx		(testCtx)
	, m_name		(name)
	, m_description	(description)
	, m_nodeType	(nodeType)
{
	DE_ASSERT(isValidCaseName(name));
}

TestNode::TestNode (TestContext& testCtx, TestNodeType nodeType, const char* name, const char* description, const vector<TestNode*>& children)
	: m_testCtx		(testCtx)
	, m_name		(name)
	, m_description	(description)
	, m_nodeType	(nodeType)
{
	DE_ASSERT(isValidCaseName(name));
	for (int i = 0; i < (int)children.size(); i++)
		addChild(children[i]);
}

TestNode::~TestNode (void)
{
	TestNode::deinit();
}

void TestNode::getChildren (vector<TestNode*>& res)
{
	res.clear();
	for (int i = 0; i < (int)m_children.size(); i++)
		res.push_back(m_children[i]);
}

void TestNode::addChild (TestNode* node)
{
	// Child names must be unique!
	// \todo [petri] O(n^2) algorithm, but shouldn't really matter..
#if defined(DE_DEBUG)
	for (int i = 0; i < (int)m_children.size(); i++)
	{
		if (deStringEqual(node->getName(), m_children[i]->getName()))
			throw tcu::InternalError(std::string("Test case with non-unique name '") + node->getName() + "' added to group '" + getName() + "'.");
	}
#endif

	// children only in group nodes
	DE_ASSERT(getTestNodeTypeClass(m_nodeType) == NODECLASS_GROUP);

	// children must have the same class
	if (!m_children.empty())
		DE_ASSERT(getTestNodeTypeClass(m_children.front()->getNodeType()) == getTestNodeTypeClass(node->getNodeType()));

	m_children.push_back(node);
}

void TestNode::init (void)
{
}

void TestNode::deinit (void)
{
	for (int i = 0; i < (int)m_children.size(); i++)
		delete m_children[i];
	m_children.clear();
}

// TestCaseGroup

TestCaseGroup::TestCaseGroup (TestContext& testCtx, const char* name, const char* description)
	: TestNode(testCtx, NODETYPE_GROUP, name, description)
{
}

TestCaseGroup::TestCaseGroup (TestContext& testCtx, const char* name, const char* description, const vector<TestNode*>& children)
	: TestNode(testCtx, NODETYPE_GROUP, name, description, children)
{
}

TestCaseGroup::~TestCaseGroup (void)
{
}

TestCase::IterateResult TestCaseGroup::iterate (void)
{
	DE_ASSERT(DE_FALSE); // should never be here!
	throw InternalError("TestCaseGroup::iterate() called!", "", __FILE__, __LINE__);
}

// TestCase

TestCase::TestCase (TestContext& testCtx, const char* name, const char* description)
	: TestNode(testCtx, NODETYPE_SELF_VALIDATE, name, description)
{
}

TestCase::TestCase (TestContext& testCtx, TestNodeType nodeType, const char* name, const char* description)
	: TestNode(testCtx, nodeType, name, description)
{
	DE_ASSERT(isTestNodeTypeExecutable(nodeType));
}

TestCase::~TestCase (void)
{
}

} // tcu
