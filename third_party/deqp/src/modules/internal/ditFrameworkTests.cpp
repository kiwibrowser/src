/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
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
 * \brief Miscellaneous framework tests.
 *//*--------------------------------------------------------------------*/

#include "ditFrameworkTests.hpp"
#include "ditTextureFormatTests.hpp"
#include "ditAstcTests.hpp"
#include "ditVulkanTests.hpp"

#include "tcuFloatFormat.hpp"
#include "tcuEither.hpp"
#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"

#include "rrRenderer.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuFloat.hpp"

#include "deRandom.hpp"
#include "deArrayUtil.hpp"

#include <stdexcept>

namespace dit
{

namespace
{

using std::string;
using std::vector;
using tcu::TestLog;

struct MatchCase
{
	enum Expected { NO_MATCH, MATCH_GROUP, MATCH_CASE, EXPECTED_LAST };

	const char*	path;
	Expected	expected;
};

const char* getMatchCaseExpectedDesc (MatchCase::Expected expected)
{
	static const char* descs[] =
	{
		"no match",
		"group to match",
		"case to match"
	};
	return de::getSizedArrayElement<MatchCase::EXPECTED_LAST>(descs, expected);
}

class CaseListParserCase : public tcu::TestCase
{
public:
	CaseListParserCase (tcu::TestContext& testCtx, const char* name, const char* caseList, const MatchCase* subCases, int numSubCases)
		: tcu::TestCase	(testCtx, name, "")
		, m_caseList	(caseList)
		, m_subCases	(subCases)
		, m_numSubCases	(numSubCases)
	{
	}

	IterateResult iterate (void)
	{
		TestLog&							log		= m_testCtx.getLog();
		tcu::CommandLine					cmdLine;
		de::MovePtr<tcu::CaseListFilter>	caseListFilter;
		int									numPass	= 0;

		log << TestLog::Message << "Input:\n\"" << m_caseList << "\"" << TestLog::EndMessage;

		{
			const char* argv[] =
			{
				"deqp",
				"--deqp-caselist",
				m_caseList
			};

			if (!cmdLine.parse(DE_LENGTH_OF_ARRAY(argv), argv))
				TCU_FAIL("Failed to parse command line");
		}

		caseListFilter = cmdLine.createCaseListFilter(m_testCtx.getArchive());

		for (int subCaseNdx = 0; subCaseNdx < m_numSubCases; subCaseNdx++)
		{
			const MatchCase&	curCase		= m_subCases[subCaseNdx];
			bool				matchGroup;
			bool				matchCase;

			log << TestLog::Message << "Checking \"" << curCase.path << "\""
									<< ", expecting " << getMatchCaseExpectedDesc(curCase.expected)
				<< TestLog::EndMessage;

			matchGroup	= caseListFilter->checkTestGroupName(curCase.path);
			matchCase	= caseListFilter->checkTestCaseName(curCase.path);

			if ((matchGroup	== (curCase.expected == MatchCase::MATCH_GROUP)) &&
				(matchCase	== (curCase.expected == MatchCase::MATCH_CASE)))
			{
				log << TestLog::Message << "   pass" << TestLog::EndMessage;
				numPass += 1;
			}
			else
				log << TestLog::Message << "   FAIL!" << TestLog::EndMessage;
		}

		m_testCtx.setTestResult((numPass == m_numSubCases) ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								(numPass == m_numSubCases) ? "All passed"			: "Unexpected match result");

		return STOP;
	}

private:
	const char* const			m_caseList;
	const MatchCase* const		m_subCases;
	const int					m_numSubCases;
};

class NegativeCaseListCase : public tcu::TestCase
{
public:
	NegativeCaseListCase (tcu::TestContext& testCtx, const char* name, const char* caseList)
		: tcu::TestCase	(testCtx, name, "")
		, m_caseList	(caseList)
	{
	}

	IterateResult iterate (void)
	{
		TestLog&			log		= m_testCtx.getLog();
		tcu::CommandLine	cmdLine;

		log << TestLog::Message << "Input:\n\"" << m_caseList << "\"" << TestLog::EndMessage;

		{
			const char* argv[] =
			{
				"deqp",
				"--deqp-caselist",
				m_caseList
			};

			TCU_CHECK(cmdLine.parse(DE_LENGTH_OF_ARRAY(argv), argv));

			try
			{
				de::UniquePtr<tcu::CaseListFilter>	filter	(cmdLine.createCaseListFilter(m_testCtx.getArchive()));

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Parsing passed, should have failed");
			}
			catch (const std::invalid_argument& e)
			{
				log << TestLog::Message << e.what() << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Parsing failed as expected");
			}
		}

		return STOP;
	}

private:
	const char* const	m_caseList;
};

class TrieParserTests : public tcu::TestCaseGroup
{
public:
	TrieParserTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "trie", "Test case trie parser tests")
	{
	}

	void init (void)
	{
		{
			static const char* const	caseList	= "{test}";
			static const MatchCase		subCases[]	=
			{
				{ "test",		MatchCase::MATCH_CASE	},
				{ "test.cd",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "single_case", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{a{b}}";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "simple_group_1", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{a{b,c}}";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "simple_group_2", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{a{b},c{d,e}}";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.c",	MatchCase::NO_MATCH		},
				{ "a.d",	MatchCase::NO_MATCH		},
				{ "a.e",	MatchCase::NO_MATCH		},
				{ "c",		MatchCase::MATCH_GROUP	},
				{ "c.b",	MatchCase::NO_MATCH		},
				{ "c.d",	MatchCase::MATCH_CASE	},
				{ "c.e",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "two_groups", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{a,c{d,e}}";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_CASE	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::NO_MATCH		},
				{ "a.d",	MatchCase::NO_MATCH		},
				{ "a.e",	MatchCase::NO_MATCH		},
				{ "c",		MatchCase::MATCH_GROUP	},
				{ "c.b",	MatchCase::NO_MATCH		},
				{ "c.d",	MatchCase::MATCH_CASE	},
				{ "c.e",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "case_group", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{c{d,e},a}";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_CASE	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::NO_MATCH		},
				{ "a.d",	MatchCase::NO_MATCH		},
				{ "a.e",	MatchCase::NO_MATCH		},
				{ "c",		MatchCase::MATCH_GROUP	},
				{ "c.b",	MatchCase::NO_MATCH		},
				{ "c.d",	MatchCase::MATCH_CASE	},
				{ "c.e",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "group_case", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{test}\r";
			static const MatchCase		subCases[]	=
			{
				{ "test",		MatchCase::MATCH_CASE	},
				{ "test.cd",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "trailing_cr", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{test}\n";
			static const MatchCase		subCases[]	=
			{
				{ "test",		MatchCase::MATCH_CASE	},
				{ "test.cd",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "trailing_lf", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "{test}\r\n";
			static const MatchCase		subCases[]	=
			{
				{ "test",		MatchCase::MATCH_CASE	},
				{ "test.cd",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "trailing_crlf", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}

		// Negative tests
		addChild(new NegativeCaseListCase(m_testCtx, "empty_string",			""));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_line",				"\n"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_root",				"{}"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_group",				"{test{}}"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_group_name_1",		"{{}}"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_group_name_2",		"{{test}}"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_root_1",		"{"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_root_2",		"{test"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_root_3",		"{test,"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_root_4",		"{test{a}"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_root_5",		"{a,b"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_group_1",	"{test{"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_group_2",	"{test{a"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_group_3",	"{test{a,"));
		addChild(new NegativeCaseListCase(m_testCtx, "unterminated_group_4",	"{test{a,b"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_case_name_1",		"{a,,b}"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_case_name_2",		"{,b}"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_case_name_3",		"{a,}"));
		addChild(new NegativeCaseListCase(m_testCtx, "no_separator",			"{a{b}c}"));
		addChild(new NegativeCaseListCase(m_testCtx, "invalid_char_1",			"{a.b}"));
		addChild(new NegativeCaseListCase(m_testCtx, "invalid_char_2",			"{a[]}"));
		addChild(new NegativeCaseListCase(m_testCtx, "trailing_char_1",			"{a}}"));
		addChild(new NegativeCaseListCase(m_testCtx, "trailing_char_2",			"{a}x"));
		addChild(new NegativeCaseListCase(m_testCtx, "embedded_newline_1",		"{\na}"));
		addChild(new NegativeCaseListCase(m_testCtx, "embedded_newline_2",		"{a\n,b}"));
		addChild(new NegativeCaseListCase(m_testCtx, "embedded_newline_3",		"{a,\nb}"));
		addChild(new NegativeCaseListCase(m_testCtx, "embedded_newline_4",		"{a{b\n}}"));
		addChild(new NegativeCaseListCase(m_testCtx, "embedded_newline_5",		"{a{b}\n}"));
	}
};

class ListParserTests : public tcu::TestCaseGroup
{
public:
	ListParserTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "list", "Test case list parser tests")
	{
	}

	void init (void)
	{
		{
			static const char* const	caseList	= "test";
			static const MatchCase		subCases[]	=
			{
				{ "test",		MatchCase::MATCH_CASE	},
				{ "test.cd",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "single_case", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
			};
			addChild(new CaseListParserCase(m_testCtx, "simple_group_1", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\na.c";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "simple_group_2", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\na.c";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "separator_ln", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\ra.c";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "separator_cr", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\r\na.c";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "separator_crlf", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\na.c\n";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "end_ln", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\na.c\r";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "end_cr", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\na.c\r\n";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.a",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "end_crlf", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b\nc.d\nc.e";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_GROUP	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::MATCH_CASE	},
				{ "a.c",	MatchCase::NO_MATCH		},
				{ "a.d",	MatchCase::NO_MATCH		},
				{ "a.e",	MatchCase::NO_MATCH		},
				{ "c",		MatchCase::MATCH_GROUP	},
				{ "c.b",	MatchCase::NO_MATCH		},
				{ "c.d",	MatchCase::MATCH_CASE	},
				{ "c.e",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "two_groups", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a\nc.d\nc.e";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_CASE	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::NO_MATCH		},
				{ "a.d",	MatchCase::NO_MATCH		},
				{ "a.e",	MatchCase::NO_MATCH		},
				{ "c",		MatchCase::MATCH_GROUP	},
				{ "c.b",	MatchCase::NO_MATCH		},
				{ "c.d",	MatchCase::MATCH_CASE	},
				{ "c.e",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "case_group", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "c.d\nc.e\na";
			static const MatchCase		subCases[]	=
			{
				{ "a",		MatchCase::MATCH_CASE	},
				{ "b",		MatchCase::NO_MATCH		},
				{ "a.b",	MatchCase::NO_MATCH		},
				{ "a.c",	MatchCase::NO_MATCH		},
				{ "a.d",	MatchCase::NO_MATCH		},
				{ "a.e",	MatchCase::NO_MATCH		},
				{ "c",		MatchCase::MATCH_GROUP	},
				{ "c.b",	MatchCase::NO_MATCH		},
				{ "c.d",	MatchCase::MATCH_CASE	},
				{ "c.e",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "group_case", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	= "a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.x";
			static const MatchCase		subCases[]	=
			{
				{ "a",												MatchCase::MATCH_GROUP	},
				{ "b",												MatchCase::NO_MATCH		},
				{ "a.b",											MatchCase::MATCH_GROUP	},
				{ "a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.x",	MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "long_name", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	=
				"a.b.c.d.e\n"
				"a.b.c.f\n"
				"x.y.z\n"
				"a.b.c.d.g\n"
				"a.b.c.x\n";
			static const MatchCase		subCases[]	=
			{
				{ "a",				MatchCase::MATCH_GROUP	},
				{ "a.b",			MatchCase::MATCH_GROUP	},
				{ "a.b.c.d.e",		MatchCase::MATCH_CASE	},
				{ "a.b.c.d.g",		MatchCase::MATCH_CASE	},
				{ "x.y",			MatchCase::MATCH_GROUP	},
				{ "x.y.z",			MatchCase::MATCH_CASE	},
				{ "a.b.c.f",		MatchCase::MATCH_CASE	},
				{ "a.b.c.x",		MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "partial_prefix", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}
		{
			static const char* const	caseList	=
				"a.a.c.d\n"
				"a.b.c.d\n";
			static const MatchCase		subCases[]	=
			{
				{ "a",				MatchCase::MATCH_GROUP	},
				{ "a.a",			MatchCase::MATCH_GROUP	},
				{ "a.b.c.d",		MatchCase::MATCH_CASE	},
				{ "a.b.c.d",		MatchCase::MATCH_CASE	},
			};
			addChild(new CaseListParserCase(m_testCtx, "reparenting", caseList, subCases, DE_LENGTH_OF_ARRAY(subCases)));
		}

		// Negative tests
		addChild(new NegativeCaseListCase(m_testCtx, "empty_string",			""));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_line",				"\n"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_group_name",		".test"));
		addChild(new NegativeCaseListCase(m_testCtx, "empty_case_name",			"test."));
	}
};

class CaseListParserTests : public tcu::TestCaseGroup
{
public:
	CaseListParserTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "case_list_parser", "Test case list parser tests")
	{
	}

	void init (void)
	{
		addChild(new TrieParserTests(m_testCtx));
		addChild(new ListParserTests(m_testCtx));
	}
};

inline deUint32 ulpDiff (float a, float b)
{
	const deUint32 ab = tcu::Float32(a).bits();
	const deUint32 bb = tcu::Float32(b).bits();
	return de::max(ab, bb) - de::min(ab, bb);
}

template<int Size>
inline tcu::Vector<deUint32, Size> ulpDiff (const tcu::Vector<float, Size>& a, const tcu::Vector<float,  Size>& b)
{
	tcu::Vector<deUint32, Size> res;
	for (int ndx = 0; ndx < Size; ndx++)
		res[ndx] = ulpDiff(a[ndx], b[ndx]);
	return res;
}

class ConstantInterpolationTest : public tcu::TestCase
{
public:
	ConstantInterpolationTest (tcu::TestContext& testCtx)
		: tcu::TestCase(testCtx, "const_interpolation", "Constant value interpolation")
	{
		const int supportedMsaaLevels[] = {1, 2, 4, 8, 16};

		for (int msaaNdx = 0; msaaNdx < DE_LENGTH_OF_ARRAY(supportedMsaaLevels); msaaNdx++)
		{
			const int numSamples = supportedMsaaLevels[msaaNdx];
			{
				SubCase c;
				c.rtSize	= tcu::IVec3(128, 128, numSamples);
				c.vtx[0]	= tcu::Vec4(-1.0f, -1.0f, 0.5f, 1.0f);
				c.vtx[1]	= tcu::Vec4(-1.0f, +1.0f, 0.5f, 1.0f);
				c.vtx[2]	= tcu::Vec4(+1.0f, -1.0f, 0.5f, 1.0f);
				c.varying	= tcu::Vec4(0.0f, 1.0f, 8.0f, -8.0f);
				m_cases.push_back(c);
			}

			{
				SubCase c;
				c.rtSize	= tcu::IVec3(128, 128, numSamples);
				c.vtx[0]	= tcu::Vec4(-1.0f, +1.0f, 0.5f, 1.0f);
				c.vtx[1]	= tcu::Vec4(+1.0f, -1.0f, 0.5f, 1.0f);
				c.vtx[2]	= tcu::Vec4(+1.0f, +1.0f, 0.5f, 1.0f);
				c.varying	= tcu::Vec4(0.0f, 1.0f, 8.0f, -8.0f);
				m_cases.push_back(c);
			}
			{
				SubCase c;
				c.rtSize	= tcu::IVec3(129, 113, numSamples);
				c.vtx[0]	= tcu::Vec4(-1.0f, -1.0f, 0.5f, 1.0f);
				c.vtx[1]	= tcu::Vec4(-1.0f, +1.0f, 0.5f, 1.0f);
				c.vtx[2]	= tcu::Vec4(+1.0f, -1.0f, 0.5f, 1.0f);
				c.varying	= tcu::Vec4(0.0f, 1.0f, 8.0f, -8.0f);
				m_cases.push_back(c);
			}
			{
				SubCase c;
				c.rtSize	= tcu::IVec3(107, 131, numSamples);
				c.vtx[0]	= tcu::Vec4(-1.0f, +1.0f, 0.5f, 1.0f);
				c.vtx[1]	= tcu::Vec4(+1.0f, -1.0f, 0.5f, 1.0f);
				c.vtx[2]	= tcu::Vec4(+1.0f, +1.0f, 0.5f, 1.0f);
				c.varying	= tcu::Vec4(0.0f, 1.0f, 8.0f, -8.0f);
				m_cases.push_back(c);
			}
		}

		{
			de::Random rnd(0x89423f);
			for (int ndx = 0; ndx < 25; ndx++)
			{
				const float	depth	= rnd.getFloat()*2.0f - 1.0f;
				SubCase		c;

				c.rtSize.x() = rnd.getInt(16, 256);
				c.rtSize.y() = rnd.getInt(16, 256);
				c.rtSize.z() = rnd.choose<int>(DE_ARRAY_BEGIN(supportedMsaaLevels), DE_ARRAY_END(supportedMsaaLevels));

				for (int vtxNdx = 0; vtxNdx < DE_LENGTH_OF_ARRAY(c.vtx); vtxNdx++)
				{
					c.vtx[vtxNdx].x() = rnd.getFloat()*2.0f - 1.0f;
					c.vtx[vtxNdx].y() = rnd.getFloat()*2.0f - 1.0f;
					c.vtx[vtxNdx].z() = depth;
					c.vtx[vtxNdx].w() = 1.0f;
				}

				for (int compNdx = 0; compNdx < 4; compNdx++)
				{
					float v;
					do
					{
						v = tcu::Float32(rnd.getUint32()).asFloat();
					} while (deFloatIsInf(v) || deFloatIsNaN(v));
					c.varying[compNdx] = v;
				}
				m_cases.push_back(c);
			}
		}
	}

	void init (void)
	{
		m_caseIter = m_cases.begin();
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "All iterations passed");
	}

	IterateResult iterate (void)
	{
		{
			tcu::ScopedLogSection section(m_testCtx.getLog(), "SubCase", "");
			runCase(*m_caseIter);
		}
		return (++m_caseIter != m_cases.end()) ? CONTINUE : STOP;
	}

protected:
	struct SubCase
	{
		tcu::IVec3	rtSize;	// (width, height, samples)
		tcu::Vec4	vtx[3];
		tcu::Vec4	varying;
	};

	void runCase (const SubCase& subCase)
	{
		using namespace tcu;

		const deUint32	maxColorUlpDiff	= 2;
		const deUint32	maxDepthUlpDiff	= 0;

		const int		width			= subCase.rtSize.x();
		const int		height			= subCase.rtSize.y();
		const int		numSamples		= subCase.rtSize.z();
		const float		zn				= 0.0f;
		const float		zf				= 1.0f;

		TextureLevel	interpolated	(TextureFormat(TextureFormat::RGBA, TextureFormat::FLOAT), numSamples, width, height);
		TextureLevel	depthStencil	(TextureFormat(TextureFormat::DS, TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV), numSamples, width, height);

		m_testCtx.getLog() << TestLog::Message
						   << "RT size (w, h, #samples) = " << subCase.rtSize << "\n"
						   << "vtx[0] = " << subCase.vtx[0] << "\n"
						   << "vtx[1] = " << subCase.vtx[1] << "\n"
						   << "vtx[2] = " << subCase.vtx[2] << "\n"
						   << "color = " << subCase.varying
						   << TestLog::EndMessage;

		clear			(interpolated.getAccess(), subCase.varying - Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		clearDepth		(depthStencil.getAccess(), 0.0f);
		clearStencil	(depthStencil.getAccess(), 0);

		{
			class VtxShader : public rr::VertexShader
			{
			public:
				VtxShader (void)
					: rr::VertexShader(2, 1)
				{
					m_inputs[0].type	= rr::GENERICVECTYPE_FLOAT;
					m_inputs[1].type	= rr::GENERICVECTYPE_FLOAT;
					m_outputs[0].type	= rr::GENERICVECTYPE_FLOAT;
				}

				void shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
				{
					for (int packetNdx = 0; packetNdx < numPackets; packetNdx++)
					{
						rr::readVertexAttrib(packets[packetNdx]->position, inputs[0], packets[packetNdx]->instanceNdx, packets[packetNdx]->vertexNdx);
						packets[packetNdx]->outputs[0] = rr::readVertexAttribFloat(inputs[1], packets[packetNdx]->instanceNdx, packets[packetNdx]->vertexNdx);
					}
				}
			} vtxShader;

			class FragShader : public rr::FragmentShader
			{
			public:
				FragShader (void)
					: rr::FragmentShader(1, 1)
				{
					m_inputs[0].type	= rr::GENERICVECTYPE_FLOAT;
					m_outputs[0].type	= rr::GENERICVECTYPE_FLOAT;
				}

				void shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
				{
					for (int packetNdx = 0; packetNdx < numPackets; packetNdx++)
					{
						for (int fragNdx = 0; fragNdx < rr::NUM_FRAGMENTS_PER_PACKET; fragNdx++)
						{
							const tcu::Vec4 interp = rr::readTriangleVarying<float>(packets[packetNdx], context, 0, fragNdx);
							rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, interp);
						}
					}
				}
			} fragShader;

			const rr::Program						program			(&vtxShader, &fragShader);

			const rr::MultisamplePixelBufferAccess	colorAccess		= rr::MultisamplePixelBufferAccess::fromMultisampleAccess(interpolated.getAccess());
			const rr::MultisamplePixelBufferAccess	dsAccess		= rr::MultisamplePixelBufferAccess::fromMultisampleAccess(depthStencil.getAccess());
			const rr::RenderTarget					renderTarget	(colorAccess, dsAccess, dsAccess);
			const rr::VertexAttrib					vertexAttribs[]	=
			{
				rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, 0, 0, subCase.vtx),
				rr::VertexAttrib(subCase.varying)
			};
			rr::ViewportState						viewport		(colorAccess);
			rr::RenderState							state			(viewport);
			const rr::DrawCommand					drawCmd			(state, renderTarget, program, DE_LENGTH_OF_ARRAY(vertexAttribs), vertexAttribs, rr::PrimitiveList(rr::PRIMITIVETYPE_TRIANGLES, 3, 0));
			const rr::Renderer						renderer;

			viewport.zn	= zn;
			viewport.zf	= zf;

			state.fragOps.depthTestEnabled							= true;
			state.fragOps.depthFunc									= rr::TESTFUNC_ALWAYS;
			state.fragOps.stencilTestEnabled						= true;
			state.fragOps.stencilStates[rr::FACETYPE_BACK].func		= rr::TESTFUNC_ALWAYS;
			state.fragOps.stencilStates[rr::FACETYPE_BACK].dpPass	= rr::STENCILOP_INCR;
			state.fragOps.stencilStates[rr::FACETYPE_FRONT]			= state.fragOps.stencilStates[rr::FACETYPE_BACK];

			renderer.draw(drawCmd);
		}

		// Verify interpolated values
		{
			TextureLevel					resolvedColor			(interpolated.getFormat(), width, height); // For debugging
			TextureLevel					resolvedDepthStencil	(depthStencil.getFormat(), width, height); // For debugging
			TextureLevel					errorMask				(TextureFormat(TextureFormat::RGB, TextureFormat::UNORM_INT8), width, height);
			const ConstPixelBufferAccess	interpAccess			= interpolated.getAccess();
			const ConstPixelBufferAccess	dsAccess				= depthStencil.getAccess();
			const PixelBufferAccess			errorAccess				= errorMask.getAccess();
			int								numCoveredSamples		= 0;
			int								numFailedColorSamples	= 0;
			int								numFailedDepthSamples	= 0;
			const bool						verifyDepth				= (subCase.vtx[0].z() == subCase.vtx[1].z()) &&
																	  (subCase.vtx[1].z() == subCase.vtx[2].z());
			const float						refDepth				= subCase.vtx[0].z()*(zf - zn)/2.0f + (zn + zf)/2.0f;

			rr::resolveMultisampleBuffer(resolvedColor.getAccess(), rr::MultisampleConstPixelBufferAccess::fromMultisampleAccess(interpolated.getAccess()));
			rr::resolveMultisampleBuffer(resolvedDepthStencil.getAccess(), rr::MultisampleConstPixelBufferAccess::fromMultisampleAccess(depthStencil.getAccess()));
			clear(errorAccess, Vec4(0.0f, 1.0f, 0.0f, 1.0f));

			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					for (int sampleNdx = 0; sampleNdx < numSamples; sampleNdx++)
					{
						if (dsAccess.getPixStencil(sampleNdx, x, y) != 0)
						{
							const Vec4		color		= interpAccess.getPixel(sampleNdx, x, y);
							const UVec4		colorDiff	= ulpDiff(color, subCase.varying);
							const bool		colorOk		= boolAll(lessThanEqual(colorDiff, tcu::UVec4(maxColorUlpDiff)));

							const float		depth		= dsAccess.getPixDepth(sampleNdx, x, y);
							const deUint32	depthDiff	= ulpDiff(depth, refDepth);
							const bool		depthOk		= verifyDepth && (depthDiff <= maxDepthUlpDiff);

							const int		maxMsgs		= 10;

							numCoveredSamples += 1;

							if (!colorOk)
							{
								numFailedColorSamples += 1;

								if (numFailedColorSamples <= maxMsgs)
									m_testCtx.getLog() << TestLog::Message
													   << "FAIL: " << tcu::IVec3(x, y, sampleNdx)
													   << " color ulp diff = " << colorDiff
													   << TestLog::EndMessage;
							}

							if (!depthOk)
								numFailedDepthSamples += 1;

							if (!colorOk || !depthOk)
								errorAccess.setPixel(errorAccess.getPixel(x, y) + Vec4(1.0f, -1.0f, 0.0f, 0.0f) / float(numSamples-1), x, y);
						}
					}
				}
			}

			m_testCtx.getLog() << TestLog::Image("ResolvedColor", "Resolved colorbuffer", resolvedColor)
							   << TestLog::Image("ResolvedDepthStencil", "Resolved depth- & stencilbuffer", resolvedDepthStencil);

			if (numFailedColorSamples != 0 || numFailedDepthSamples != 0)
			{
				m_testCtx.getLog() << TestLog::Image("ErrorMask", "Error mask", errorMask);

				if (numFailedColorSamples != 0)
					m_testCtx.getLog() << TestLog::Message << "FAIL: Found " << numFailedColorSamples << " invalid color samples!" << TestLog::EndMessage;

				if (numFailedDepthSamples != 0)
					m_testCtx.getLog() << TestLog::Message << "FAIL: Found " << numFailedDepthSamples << " invalid depth samples!" << TestLog::EndMessage;

				if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid samples found");
			}

			m_testCtx.getLog() << TestLog::Message << (numCoveredSamples-numFailedColorSamples) << " / " << numCoveredSamples << " color samples passed" << TestLog::EndMessage;
			m_testCtx.getLog() << TestLog::Message << (numCoveredSamples-numFailedDepthSamples) << " / " << numCoveredSamples << " depth samples passed" << TestLog::EndMessage;
		}
	}

	vector<SubCase>					m_cases;
	vector<SubCase>::const_iterator	m_caseIter;
};

class CommonFrameworkTests : public tcu::TestCaseGroup
{
public:
	CommonFrameworkTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "common", "Tests for the common utility framework")
	{
	}

	void init (void)
	{
		addChild(new SelfCheckCase(m_testCtx, "float_format","tcu::FloatFormat_selfTest()",
								   tcu::FloatFormat_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "either","tcu::Either_selfTest()",
								   tcu::Either_selfTest));
	}
};

class ReferenceRendererTests : public tcu::TestCaseGroup
{
public:
	ReferenceRendererTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "reference_renderer", "Reference renderer tests")
	{
	}

	void init (void)
	{
		addChild(new ConstantInterpolationTest(m_testCtx));
	}
};

} // anonymous

FrameworkTests::FrameworkTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup(testCtx, "framework", "Miscellaneous framework tests")
{
}

FrameworkTests::~FrameworkTests (void)
{
}

void FrameworkTests::init (void)
{
	addChild(new CommonFrameworkTests	(m_testCtx));
	addChild(new CaseListParserTests	(m_testCtx));
	addChild(new ReferenceRendererTests	(m_testCtx));
	addChild(createTextureFormatTests	(m_testCtx));
	addChild(createAstcTests			(m_testCtx));
	addChild(createVulkanTests			(m_testCtx));
}

} // dit
