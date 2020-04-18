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

#ifndef COMPILER_PREPROCESSOR_MACRO_H_
#define COMPILER_PREPROCESSOR_MACRO_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pp
{

struct Token;

struct Macro
{
    enum Type
    {
        kTypeObj,
        kTypeFunc
    };
    typedef std::vector<std::string> Parameters;
    typedef std::vector<Token> Replacements;

    Macro();
    ~Macro();
    bool equals(const Macro &other) const;

    bool predefined;
    mutable bool disabled;
    mutable int expansionCount;

    Type type;
    std::string name;
    Parameters parameters;
    Replacements replacements;
};

typedef std::map<std::string, std::shared_ptr<Macro>> MacroSet;

void PredefineMacro(MacroSet *macroSet, const char *name, int value);

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_MACRO_H_
