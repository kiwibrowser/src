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

#ifndef COMPILER_DIAGNOSTICS_H_
#define COMPILER_DIAGNOSTICS_H_

#include "preprocessor/DiagnosticsBase.h"

class TInfoSink;

class TDiagnostics : public pp::Diagnostics
{
public:
	TDiagnostics(TInfoSink& infoSink);
	virtual ~TDiagnostics();

	int shaderVersion() const { return mShaderVersion; }
	TInfoSink& infoSink() { return mInfoSink; }

	int numErrors() const { return mNumErrors; }
	int numWarnings() const { return mNumWarnings; }

	void setShaderVersion(int version);

	void writeInfo(Severity severity,
				   const pp::SourceLocation& loc,
				   const std::string& reason,
				   const std::string& token,
				   const std::string& extra);

	void writeDebug(const std::string& str);

protected:
	virtual void print(ID id,
					   const pp::SourceLocation& loc,
					   const std::string& text);

private:
	int mShaderVersion;

	TInfoSink& mInfoSink;
	int mNumErrors;
	int mNumWarnings;
};

#endif  // COMPILER_DIAGNOSTICS_H_
