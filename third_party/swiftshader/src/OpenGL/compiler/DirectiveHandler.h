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

#ifndef COMPILER_DIRECTIVE_HANDLER_H_
#define COMPILER_DIRECTIVE_HANDLER_H_

#include "ExtensionBehavior.h"
#include "Pragma.h"
#include "preprocessor/DirectiveHandlerBase.h"

class TDiagnostics;

class TDirectiveHandler : public pp::DirectiveHandler
{
public:
	TDirectiveHandler(TExtensionBehavior& extBehavior,
	                  TDiagnostics& diagnostics,
	                  int& shaderVersion);
	virtual ~TDirectiveHandler();

	const TPragma& pragma() const { return mPragma; }
	const TExtensionBehavior& extensionBehavior() const { return mExtensionBehavior; }

	virtual void handleError(const pp::SourceLocation& loc,
	                         const std::string& msg);

	virtual void handlePragma(const pp::SourceLocation& loc,
	                          const std::string& name,
	                          const std::string& value,
	                          bool stdgl);

	virtual void handleExtension(const pp::SourceLocation& loc,
	                             const std::string& name,
	                             const std::string& behavior);

	virtual void handleVersion(const pp::SourceLocation& loc,
	                           int version);

private:
	TPragma mPragma;
	TExtensionBehavior& mExtensionBehavior;
	TDiagnostics& mDiagnostics;
	int& mShaderVersion;
};

#endif  // COMPILER_DIRECTIVE_HANDLER_H_
