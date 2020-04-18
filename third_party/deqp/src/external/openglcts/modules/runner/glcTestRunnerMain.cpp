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
 * \brief CTS runner main().
 */ /*-------------------------------------------------------------------*/

#include "deString.h"
#include "deUniquePtr.hpp"
#include "glcTestRunner.hpp"
#include "tcuPlatform.hpp"
#include "tcuResource.hpp"

#include <cstdio>

// See tcuMain.cpp
tcu::Platform* createPlatform(void);

struct CommandLine
{
	CommandLine(void) : runType(glu::ApiType::es(2, 0)), flags(0)
	{
	}

	glu::ApiType runType;
	std::string  dstLogDir;
	deUint32	 flags;
};

static bool parseCommandLine(CommandLine& cmdLine, int argc, const char* const* argv)
{
	for (int argNdx = 1; argNdx < argc; argNdx++)
	{
		const char* arg = argv[argNdx];

		if (deStringBeginsWith(arg, "--type="))
		{
			static const struct
			{
				const char*  name;
				glu::ApiType runType;
			} runTypes[] = { { "es2", glu::ApiType::es(2, 0) },	{ "es3", glu::ApiType::es(3, 0) },
							 { "es31", glu::ApiType::es(3, 1) },   { "es32", glu::ApiType::es(3, 2) },
							 { "gl30", glu::ApiType::core(3, 0) }, { "gl31", glu::ApiType::core(3, 1) },
							 { "gl32", glu::ApiType::core(3, 2) }, { "gl33", glu::ApiType::core(3, 3) },
							 { "gl40", glu::ApiType::core(4, 0) }, { "gl41", glu::ApiType::core(4, 1) },
							 { "gl42", glu::ApiType::core(4, 2) }, { "gl43", glu::ApiType::core(4, 3) },
							 { "gl44", glu::ApiType::core(4, 4) }, { "gl45", glu::ApiType::core(4, 5) },
							 { "gl46", glu::ApiType::core(4, 6) } };

			const char* value = arg + 7;
			int			ndx   = 0;

			for (; ndx < DE_LENGTH_OF_ARRAY(runTypes); ndx++)
			{
				if (deStringEqual(runTypes[ndx].name, value))
				{
					cmdLine.runType = runTypes[ndx].runType;
					break;
				}
			}

			if (ndx >= DE_LENGTH_OF_ARRAY(runTypes))
				return false;
		}
		else if (deStringBeginsWith(arg, "--logdir="))
		{
			const char* value = arg + 9;
			cmdLine.dstLogDir = value;
		}
		else if (deStringBeginsWith(arg, "--summary"))
		{
			cmdLine.flags = glcts::TestRunner::PRINT_SUMMARY;
		}
		else if (deStringEqual(arg, "--verbose"))
			cmdLine.flags = glcts::TestRunner::VERBOSE_ALL;
		else
			return false;
	}

	return true;
}

static void printHelp(const char* binName)
{
	printf("%s:\n", binName);
	printf("  --type=[esN[M]|glNM] Conformance test run type. Choose from\n");
	printf("                       ES: es2, es3, es31, es32\n");
	printf("                       GL: gl30, gl31, gl32, gl33, gl40, gl41, gl42, gl43, gl44, gl45, gl46\n");
	printf("  --logdir=[path]      Destination directory for log files\n");
	printf("  --summary            Print summary without running the tests\n");
	printf("  --verbose            Print out and log more information\n");
}

int main(int argc, char** argv)
{
	CommandLine cmdLine;

	if (!parseCommandLine(cmdLine, argc, argv))
	{
		printHelp(argv[0]);
		return -1;
	}

	try
	{
		de::UniquePtr<tcu::Platform> platform(createPlatform());
		tcu::DirArchive				 archive(".");
		glcts::TestRunner runner(static_cast<tcu::Platform&>(*platform.get()), archive, cmdLine.dstLogDir.c_str(),
								 cmdLine.runType, cmdLine.flags);

		for (;;)
		{
			if (!runner.iterate())
				break;
		}
	}
	catch (const std::exception& e)
	{
		printf("ERROR: %s\n", e.what());
		return -1;
	}

	return 0;
}
