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

#include "InfoSink.h"

void TInfoSinkBase::prefix(TPrefixType message) {
	switch(message) {
		case EPrefixNone:
			break;
		case EPrefixWarning:
			sink.append("WARNING: ");
			break;
		case EPrefixError:
			sink.append("ERROR: ");
			break;
		case EPrefixInternalError:
			sink.append("INTERNAL ERROR: ");
			break;
		case EPrefixUnimplemented:
			sink.append("UNIMPLEMENTED: ");
			break;
		case EPrefixNote:
			sink.append("NOTE: ");
			break;
		default:
			sink.append("UNKOWN ERROR: ");
			break;
	}
}

void TInfoSinkBase::location(const TSourceLoc& loc) {
	int string = loc.first_file, line = loc.first_line;

	TPersistStringStream stream;
	if (line)
		stream << string << ":" << line;
	else
		stream << string << ":? ";
	stream << ": ";

	sink.append(stream.str());
}

void TInfoSinkBase::message(TPrefixType message, const char* s) {
	prefix(message);
	sink.append(s);
	sink.append("\n");
}

void TInfoSinkBase::message(TPrefixType message, const char* s, TSourceLoc loc) {
	prefix(message);
	location(loc);
	sink.append(s);
	sink.append("\n");
}
