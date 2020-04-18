// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMPILER_PREPROCESSOR_PREPROCESSOR_H_
#define COMPILER_PREPROCESSOR_PREPROCESSOR_H_

#include "pp_utils.h"

#include <cstddef>

namespace pp
{

class Diagnostics;
class DirectiveHandler;
struct PreprocessorImpl;
struct Token;

struct PreprocessorSettings
{
	PreprocessorSettings() : maxMacroExpansionDepth(1000) {}
	int maxMacroExpansionDepth;
};

class Preprocessor
{
public:
	Preprocessor(Diagnostics *diagnostics,
	             DirectiveHandler *directiveHandler,
	             const PreprocessorSettings &settings);
	~Preprocessor();

	// count: specifies the number of elements in the string and length arrays.
	// string: specifies an array of pointers to strings.
	// length: specifies an array of string lengths.
	// If length is NULL, each string is assumed to be null terminated.
	// If length is a value other than NULL, it points to an array containing
	// a string length for each of the corresponding elements of string.
	// Each element in the length array may contain the length of the
	// corresponding string or a value less than 0 to indicate that the string
	// is null terminated.
	bool init(size_t count, const char *const string[], const int length[]);
	// Adds a pre-defined macro.
	void predefineMacro(const char *name, int value);

	void lex(Token *token);

	// Set maximum preprocessor token size
	void setMaxTokenSize(size_t maxTokenSize);

private:
	PP_DISALLOW_COPY_AND_ASSIGN(Preprocessor);

	PreprocessorImpl *mImpl;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_PREPROCESSOR_H_

