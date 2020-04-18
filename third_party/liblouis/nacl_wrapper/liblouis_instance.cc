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

#include "liblouis_instance.h"

#include <stddef.h>
#include <sys/mount.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "native_client_sdk/src/libraries/nacl_io/nacl_io.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module.h"

#include "translation_result.h"

namespace {

static const char kHexadecimalChars[] = "0123456789abcdef";

// Converts a vector of bytes into a (lowercase) hexadecimal string.
static void BytesToHexString(const std::vector<unsigned char>& bytes,
    std::string* out) {
  std::string hex;
  hex.reserve(bytes.size() * 2);
  for (size_t i = 0; i < bytes.size(); ++i) {
    unsigned char byte = bytes[i];
    hex.push_back(kHexadecimalChars[byte >> 4]);
    hex.push_back(kHexadecimalChars[byte & 0x0f]);
  }
  out->swap(hex);
}

// Converts a hexadecimal string to a vector of bytes.
// Returns false on failure.
static bool HexStringToBytes(const std::string& hex,
    std::vector<unsigned char>* out) {
  if (hex.size() % 2 != 0) {
    return false;
  }

  std::vector<unsigned char> bytes;
  bytes.reserve(hex.size() / 2);
  for (size_t i = 0; i < hex.size(); i += 2) {
    unsigned char byte;
    char ch = hex[i];
    if ('0' <= ch && ch <= '9') {
      byte = (ch - '0') << 4;
    } else if ('a' <= ch && ch <= 'f') {
      byte = (ch - 'a' + 10) << 4;
    } else if ('A' <= ch && ch <= 'F') {
      byte = (ch - 'A' + 10) << 4;
    } else {
      return false;
    }
    ch = hex[i+1];
    if ('0' <= ch && ch <= '9') {
      byte |= ch - '0';
    } else if ('a' <= ch && ch <= 'f') {
      byte |= ch - 'a' + 10;
    } else if ('A' <= ch && ch <= 'F') {
      byte |= ch - 'A' + 10;
    } else {
      return false;
    }
    bytes.push_back(byte);
  }
  out->swap(bytes);
  return true;
}

template <typename T>
static void CopyVectorToJson(const std::vector<T>& vec, Json::Value* out) {
  Json::Value result(Json::arrayValue);
  result.resize(vec.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    result[i] = vec[i];
  }
  out->swap(result);
}

}  // namespace


namespace liblouis_nacl {

// Well-known strings used for configuration.
static const char kTablesDirKey[] = "tablesdir";
static const char kTablesDirDefault[] = "tables";

// Well-known strings used in JSON messages.
static const char kCommandKey[] = "command";
static const char kMessageIdKey[] = "message_id";
static const char kInReplyToKey[] = "in_reply_to";
static const char kErrorKey[] = "error";
static const char kTableNamesKey[] = "table_names";
static const char kSuccessKey[] = "success";
static const char kTextKey[] = "text";
static const char kFormTypeMapKey[] = "form_type_map";
static const char kCellsKey[] = "cells";
static const char kCursorPositionKey[] = "cursor_position";
static const char kTextToBrailleKey[] = "text_to_braille";
static const char kBrailleToTextKey[] = "braille_to_text";
static const char kCheckTableCommand[] = "CheckTable";
static const char kTranslateCommand[] = "Translate";
static const char kBackTranslateCommand[] = "BackTranslate";

LibLouisInstance::LibLouisInstance(PP_Instance instance)
    : pp::Instance(instance), liblouis_thread_(this), cc_factory_(this) {}

LibLouisInstance::~LibLouisInstance() {}

bool LibLouisInstance::Init(uint32_t argc, const char* argn[],
    const char* argv[]) {
  const char* tables_dir = kTablesDirDefault;
  for (size_t i = 0; i < argc; ++i) {
    if (strcmp(argn[i], kTablesDirKey) == 0) {
      tables_dir = argv[i];
    }
  }

  nacl_io_init_ppapi(pp_instance(),
      pp::Module::Get()->get_browser_interface());
  if (mount(tables_dir, liblouis_.tables_dir(), "httpfs", 0, "") != 0) {
    // TODO(jbroman): report this error.
    return false;
  }

  return liblouis_thread_.Start();
}

void LibLouisInstance::HandleMessage(const pp::Var& var_message) {
  if (!var_message.is_string()) {
    PostError("expected message to be a JSON string");
    return;
  }

  Json::Value message;
  Json::Reader reader;
  bool parsed = reader.parse(var_message.AsString(),
      message, false /* collectComments */);
  if (!parsed) {
    PostError("expected message to be a JSON string");
    return;
  }

  Json::Value message_id = message[kMessageIdKey];
  if (!message_id.isString()) {
    PostError("expected message_id string");
    return;
  }
  std::string message_id_str = message_id.asString();

  Json::Value command = message[kCommandKey];
  if (!command.isString()) {
    PostError("expected command string", message_id_str);
    return;
  }

  std::string command_str = command.asString();
  if (command_str == kCheckTableCommand) {
    HandleCheckTable(message, message_id_str);
  } else if (command_str == kTranslateCommand) {
    HandleTranslate(message, message_id_str);
  } else if (command_str == kBackTranslateCommand) {
    HandleBackTranslate(message, message_id_str);
  } else {
    PostError("unknown command", message_id_str);
  }
}

void LibLouisInstance::PostReply(Json::Value reply,
    const std::string& in_reply_to) {
  Json::FastWriter writer;
  reply[kInReplyToKey] = in_reply_to;
  pp::Var var_reply(writer.write(reply));
  PostMessage(var_reply);
}

void LibLouisInstance::PostError(const std::string& error_message) {
  Json::FastWriter writer;
  Json::Value reply(Json::objectValue);
  reply[kErrorKey] = error_message;
  pp::Var var_reply(writer.write(reply));
  PostMessage(var_reply);
}

void LibLouisInstance::PostError(const std::string& error_message,
    const std::string& in_reply_to) {
  Json::FastWriter writer;
  Json::Value reply(Json::objectValue);
  reply[kErrorKey] = error_message;
  reply[kInReplyToKey] = in_reply_to;
  reply[kSuccessKey] = false;
  pp::Var var_reply(writer.write(reply));
  PostMessage(var_reply);
}

void LibLouisInstance::HandleCheckTable(const Json::Value& message,
    const std::string& message_id) {
  Json::Value table_names = message[kTableNamesKey];
  if (!table_names.isString()) {
    PostError("expected table_names to be a string", message_id);
    return;
  }
  PostWorkToBackground(cc_factory_.NewCallback(
      &LibLouisInstance::CheckTableInBackground,
      table_names.asString(), message_id));
}

void LibLouisInstance::CheckTableInBackground(int32_t result,
    const std::string& table_names, const std::string& message_id) {
  if (result != PP_OK) {
    PostError("failed to transfer call to background thread", message_id);
    return;
  }
  bool success = liblouis_.CheckTable(table_names);
  Json::Value reply(Json::objectValue);
  reply[kSuccessKey] = success;
  PostReply(reply, message_id);
}

void LibLouisInstance::HandleTranslate(const Json::Value& message,
    const std::string& message_id) {
  Json::Value table_names = message[kTableNamesKey];
  Json::Value text = message[kTextKey];
  Json::Value cursor_position = message[kCursorPositionKey];
  Json::Value form_type_map = message[kFormTypeMapKey];
  if (!table_names.isString()) {
    PostError("expected table_names to be a string", message_id);
    return;
  } else if (!text.isString()) {
    PostError("expected text to be a string", message_id);
    return;
  } else if (!cursor_position.isNull() && !cursor_position.isIntegral()) {
    PostError("expected cursor_position to be null or integral", message_id);
    return;
  } else if (!form_type_map.isArray()) {
    PostError("expected form_type_map to be an array", message_id);
    return;
  }
  TranslationParams params;
  params.table_names = table_names.asString();
  params.text = text.asString();
  params.cursor_position = cursor_position.isIntegral() ?
      cursor_position.asInt() : -1;
  for (size_t i = 0; i < form_type_map.size(); ++i) {
    Json::Value val = form_type_map[i];
    if (!val.isIntegral()) {
      PostError("expected form_type_map to be an integral array", message_id);
      return;
    }
    params.form_type_map.push_back(val.asInt());
  }
  PostWorkToBackground(cc_factory_.NewCallback(
      &LibLouisInstance::TranslateInBackground,
      params, message_id));
}

void LibLouisInstance::TranslateInBackground(int32_t result,
    const TranslationParams& params, const std::string& message_id) {
  if (result != PP_OK) {
    PostError("failed to transfer call to background thread", message_id);
    return;
  }
  TranslationResult translation_result;
  bool success = liblouis_.Translate(params, &translation_result);
  Json::Value reply(Json::objectValue);
  reply[kSuccessKey] = success;
  if (success) {
    std::string hex_cells;
    Json::Value text_to_braille;
    Json::Value braille_to_text;
    BytesToHexString(translation_result.cells, &hex_cells);
    CopyVectorToJson(translation_result.text_to_braille, &text_to_braille);
    CopyVectorToJson(translation_result.braille_to_text, &braille_to_text);
    reply[kCellsKey] = hex_cells;
    reply[kTextToBrailleKey] = text_to_braille;
    reply[kBrailleToTextKey] = braille_to_text;
    if (translation_result.cursor_position >= 0) {
      reply[kCursorPositionKey] = translation_result.cursor_position;
    }
  }
  PostReply(reply, message_id);
}

void LibLouisInstance::HandleBackTranslate(const Json::Value& message,
    const std::string& message_id) {
  Json::Value table_names = message[kTableNamesKey];
  Json::Value cells = message[kCellsKey];
  if (!table_names.isString()) {
    PostError("expected table_names to be a string", message_id);
    return;
  } else if (!cells.isString()) {
    PostError("expected cells to be a string", message_id);
    return;
  }
  std::vector<unsigned char> cells_vector;
  if (!HexStringToBytes(cells.asString(), &cells_vector)) {
    PostError("expected cells to be a valid hexadecimal string", message_id);
    return;
  }
  PostWorkToBackground(cc_factory_.NewCallback(
      &LibLouisInstance::BackTranslateInBackground,
      table_names.asString(), cells_vector, message_id));
}

void LibLouisInstance::BackTranslateInBackground(int32_t result,
    const std::string& table_names, const std::vector<unsigned char>& cells,
    const std::string& message_id) {
  if (result != PP_OK) {
    PostError("failed to transfer call to background thread", message_id);
    return;
  }
  std::string text;
  bool success = liblouis_.BackTranslate(table_names, cells, &text);
  Json::Value reply(Json::objectValue);
  reply[kSuccessKey] = success;
  if (success) {
    reply[kTextKey] = text;
  }
  PostReply(reply, message_id);
}

}  // namespace liblouis_nacl
