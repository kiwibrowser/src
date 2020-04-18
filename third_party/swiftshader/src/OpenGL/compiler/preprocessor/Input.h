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

#ifndef COMPILER_PREPROCESSOR_INPUT_H_
#define COMPILER_PREPROCESSOR_INPUT_H_

#include <cstddef>
#include <vector>
#include <climits>

namespace pp
{

// Holds and reads input for Lexer.
class Input
{
public:
	Input();
	~Input();
	Input(size_t count, const char *const string[], const int length[]);

	size_t count() const { return mCount; }
	const char *string(size_t index) const { return mString[index]; }
	size_t length(size_t index) const { return mLength[index]; }

	size_t read(char *buf, size_t maxSize, int *lineNo);

	struct Location
	{
		size_t sIndex;  // String index;
		size_t cIndex;  // Char index.

		Location() : sIndex(0), cIndex(0) {}
	};
	const Location &readLoc() const { return mReadLoc; }

private:
	// Skip a character and return the next character after the one that was skipped.
	// Return nullptr if data runs out.
	const char *skipChar();

	// Input.
	size_t mCount;
	const char *const *mString;
	std::vector<size_t> mLength;

	Location mReadLoc;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_INPUT_H_

