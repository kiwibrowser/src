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

#ifndef COMPILER_PREPROCESSOR_DIRECTIVE_PARSER_H_
#define COMPILER_PREPROCESSOR_DIRECTIVE_PARSER_H_

#include "Lexer.h"
#include "Macro.h"
#include "pp_utils.h"
#include "SourceLocation.h"

namespace pp
{

class Diagnostics;
class DirectiveHandler;
class Tokenizer;

class DirectiveParser : public Lexer
{
public:
	DirectiveParser(Tokenizer *tokenizer,
	                MacroSet *macroSet,
	                Diagnostics *diagnostics,
	                DirectiveHandler *directiveHandler,
	                int maxMacroExpansionDepth);
	~DirectiveParser() override;

	void lex(Token *token) override;

private:
	PP_DISALLOW_COPY_AND_ASSIGN(DirectiveParser);

	void parseDirective(Token *token);
	void parseDefine(Token *token);
	void parseUndef(Token *token);
	void parseIf(Token *token);
	void parseIfdef(Token *token);
	void parseIfndef(Token *token);
	void parseElse(Token *token);
	void parseElif(Token *token);
	void parseEndif(Token *token);
	void parseError(Token *token);
	void parsePragma(Token *token);
	void parseExtension(Token *token);
	void parseVersion(Token *token);
	void parseLine(Token *token);

	bool skipping() const;
	void parseConditionalIf(Token *token);
	int parseExpressionIf(Token *token);
	int parseExpressionIfdef(Token *token);

	struct ConditionalBlock
	{
		std::string type;
		SourceLocation location;
		bool skipBlock;
		bool skipGroup;
		bool foundValidGroup;
		bool foundElseGroup;

		ConditionalBlock() :
			skipBlock(false),
			skipGroup(false),
			foundValidGroup(false),
			foundElseGroup(false)
		{
		}
	};
	bool mPastFirstStatement;
	bool mSeenNonPreprocessorToken;  // Tracks if a non-preprocessor token has been seen yet.  Some
	                                 // macros, such as
	                                 // #extension must be declared before all shader code.
	std::vector<ConditionalBlock> mConditionalStack;
	Tokenizer *mTokenizer;
	MacroSet *mMacroSet;
	Diagnostics *mDiagnostics;
	DirectiveHandler *mDirectiveHandler;
	int mShaderVersion;
	int mMaxMacroExpansionDepth;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_DIRECTIVE_PARSER_H_

