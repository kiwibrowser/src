// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.Constant');


/**
 * The set of Latin valid characters .
 */
i18n.input.chrome.Constant.LATIN_VALID_CHAR =
    "[a-z\\-\\\'\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u017F]";


/**
 * Languages need to support NACL module for XKB.
 *
 * @type {!Array.<string>}
 */
i18n.input.chrome.Constant.NACL_LANGUAGES = [
  'da',
  'de',
  'en',
  'es',
  'fi',
  'fr',
  'it',
  'nl',
  'no',
  'pl',
  'pt',
  'pt-BR',
  'pt-PT',
  'sv',
  'tr'
];
