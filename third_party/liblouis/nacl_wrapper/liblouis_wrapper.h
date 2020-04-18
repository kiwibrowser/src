// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef LIBLOUIS_NACL_LIBLOUIS_WRAPPER_H_
#define LIBLOUIS_NACL_LIBLOUIS_WRAPPER_H_

#include <string>

#include "base/macros.h"
#include "translation_params.h"
#include "translation_result.h"

namespace liblouis_nacl {

// Encapsulates logic for interacting (synchronously) with liblouis.
//
// This class is *not* thread-safe; it should be used only from one thread.
// Since the underlying library is not reentrant, only one instance should be
// in use at a time.
//
// All input strings should be represented in UTF-8.
class LibLouisWrapper {
 public:
  LibLouisWrapper();
  ~LibLouisWrapper();

  // Returns one of the paths where tables may be searched for.
  const char* tables_dir() const;

  // Loads, checks, and compiles the requested table.
  // Returns true on success.
  bool CheckTable(const std::string& table_names);

  // Translates the given text and cursor position into braille.
  bool Translate(const TranslationParams& params, TranslationResult* out);

  // Translates the given braille cells into text.
  bool BackTranslate(const std::string& table_names,
      const std::vector<unsigned char>& cells, std::string* out);

 private:
  DISALLOW_COPY_AND_ASSIGN(LibLouisWrapper);
};

}  // namespace liblouis_nacl

#endif  // LIBLOUIS_NACL_LIBLOUIS_WRAPPER_H_
