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

#include "InitializeParseContext.h"

#include "osinclude.h"

OS_TLSIndex GlobalParseContextIndex = OS_INVALID_TLS_INDEX;

bool InitializeParseContextIndex()
{
	assert(GlobalParseContextIndex == OS_INVALID_TLS_INDEX);

	GlobalParseContextIndex = OS_AllocTLSIndex();
	return GlobalParseContextIndex != OS_INVALID_TLS_INDEX;
}

void FreeParseContextIndex()
{
	assert(GlobalParseContextIndex != OS_INVALID_TLS_INDEX);

	OS_FreeTLSIndex(GlobalParseContextIndex);
	GlobalParseContextIndex = OS_INVALID_TLS_INDEX;
}

void SetGlobalParseContext(TParseContext* context)
{
	assert(GlobalParseContextIndex != OS_INVALID_TLS_INDEX);
	OS_SetTLSValue(GlobalParseContextIndex, context);
}

TParseContext* GetGlobalParseContext()
{
	assert(GlobalParseContextIndex != OS_INVALID_TLS_INDEX);
	return static_cast<TParseContext*>(OS_GetTLSValue(GlobalParseContextIndex));
}

