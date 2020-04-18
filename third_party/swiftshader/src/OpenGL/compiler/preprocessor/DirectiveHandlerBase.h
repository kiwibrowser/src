// Copyright 2017 The SwiftShader Authors. All Rights Reserved.
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

#ifndef COMPILER_PREPROCESSOR_DIRECTIVEHANDLERBASE_H_
#define COMPILER_PREPROCESSOR_DIRECTIVEHANDLERBASE_H_

#include <string>

namespace pp
{

struct SourceLocation;

// Base class for handling directives.
// Preprocessor uses this class to notify the clients about certain
// preprocessor directives. Derived classes are responsible for
// handling them in an appropriate manner.
class DirectiveHandler
{
  public:
	virtual ~DirectiveHandler();

	virtual void handleError(const SourceLocation &loc, const std::string &msg) = 0;

	// Handle pragma of form: #pragma name[(value)]
	virtual void handlePragma(const SourceLocation &loc,
	                          const std::string &name,
	                          const std::string &value,
	                          bool stdgl) = 0;

	virtual void handleExtension(const SourceLocation &loc,
	                             const std::string &name,
	                             const std::string &behavior) = 0;

	virtual void handleVersion(const SourceLocation &loc, int version) = 0;
};

}  // namespace pp

#endif  // COMPILER_PREPROCESSOR_DIRECTIVEHANDLERBASE_H_
