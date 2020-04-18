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

#include "Diagnostics.h"

#include "debug.h"
#include "InfoSink.h"
#include "preprocessor/SourceLocation.h"

TDiagnostics::TDiagnostics(TInfoSink& infoSink) :
	mShaderVersion(100),
	mInfoSink(infoSink),
	mNumErrors(0),
	mNumWarnings(0)
{
}

TDiagnostics::~TDiagnostics()
{
}

void TDiagnostics::setShaderVersion(int version)
{
	mShaderVersion = version;
}

void TDiagnostics::writeInfo(Severity severity,
                             const pp::SourceLocation& loc,
                             const std::string& reason,
                             const std::string& token,
                             const std::string& extra)
{
	TPrefixType prefix = EPrefixNone;
	switch(severity)
	{
	case PP_ERROR:
		++mNumErrors;
		prefix = EPrefixError;
		break;
	case PP_WARNING:
		++mNumWarnings;
		prefix = EPrefixWarning;
		break;
	default:
		UNREACHABLE(severity);
		break;
	}

	TInfoSinkBase& sink = mInfoSink.info;
	/* VC++ format: file(linenum) : error #: 'token' : extrainfo */
	sink.prefix(prefix);
	TSourceLoc sourceLoc;
	sourceLoc.first_file = sourceLoc.last_file = loc.file;
	sourceLoc.first_line = sourceLoc.last_line = loc.line;
	sink.location(sourceLoc);
	sink << "'" << token <<  "' : " << reason << " " << extra << "\n";
}

void TDiagnostics::writeDebug(const std::string& str)
{
	mInfoSink.debug << str;
}

void TDiagnostics::print(ID id,
                         const pp::SourceLocation& loc,
                         const std::string& text)
{
	writeInfo(severity(id), loc, message(id), text, "");
}
