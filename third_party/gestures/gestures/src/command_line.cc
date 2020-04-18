// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/command_line.h"

#include <algorithm>
#include <ostream>
#include <string>

#include "gestures/include/macros.h"
#include "gestures/include/string_util.h"

namespace gestures {

CommandLine* CommandLine::current_process_commandline_ = NULL;

namespace {

const CommandLine::CharType kSwitchTerminator[] = "--";
const CommandLine::CharType kSwitchValueSeparator[] = "=";

// Since we use a lazy match, make sure that longer versions (like "--") are
// listed before shorter versions (like "-") of similar prefixes.
// Unixes don't use slash as a switch.
const CommandLine::CharType* const kSwitchPrefixes[] = {"--", "-"};
size_t switch_prefix_count = arraysize(kSwitchPrefixes);

size_t GetSwitchPrefixLength(const std::string& string) {
  for (size_t i = 0; i < switch_prefix_count; ++i) {
    std::string prefix(kSwitchPrefixes[i]);
    if (string.compare(0, prefix.length(), prefix) == 0)
      return prefix.length();
  }
  return 0;
}

// Fills in |switch_string| and |switch_value| if |string| is a switch.
// This will preserve the input switch prefix in the output |switch_string|.
bool IsSwitch(const std::string& string,
              std::string* switch_string,
              std::string* switch_value) {
  switch_string->clear();
  switch_value->clear();
  size_t prefix_length = GetSwitchPrefixLength(string);
  if (prefix_length == 0 || prefix_length == string.length())
    return false;

  const size_t equals_position = string.find(kSwitchValueSeparator);
  *switch_string = string.substr(0, equals_position);
  if (equals_position != std::string::npos)
    *switch_value = string.substr(equals_position + 1);
  return true;
}

// Append switches and arguments, keeping switches before arguments.
void AppendSwitchesAndArguments(CommandLine& command_line,
                                const CommandLine::StringVector& argv) {
  bool parse_switches = true;
  for (size_t i = 1; i < argv.size(); ++i) {
    std::string arg = argv[i];
    TrimWhitespaceASCII(arg, TRIM_ALL, &arg);

    std::string switch_string;
    std::string switch_value;
    parse_switches &= (arg != kSwitchTerminator);
    if (parse_switches && IsSwitch(arg, &switch_string, &switch_value)) {
      command_line.AppendSwitchASCII(switch_string, switch_value);
    } else {
      command_line.AppendArgASCII(arg);
    }
  }
}

}  // namespace

CommandLine::CommandLine()
    : argv_(1),
      begin_args_(1) {}

CommandLine::~CommandLine() {}

// static
bool CommandLine::Init(int argc, const char* const* argv) {
  if (current_process_commandline_) {
    // If this is intentional, Reset() must be called first. If we are using
    // the shared build mode, we have to share a single object across multiple
    // shared libraries.
    return false;
  }

  current_process_commandline_ = new CommandLine();
  current_process_commandline_->InitFromArgv(argc, argv);
  return true;
}

// static
void CommandLine::Reset() {
  delete current_process_commandline_;
  current_process_commandline_ = NULL;
}

// static
CommandLine* CommandLine::ForCurrentProcess() {
  if (!current_process_commandline_) {
    fprintf(stderr, "FATAL: Command line not initialized!\n");
    abort();
  }
  return current_process_commandline_;
}


void CommandLine::InitFromArgv(int argc,
                               const CommandLine::CharType* const* argv) {
  StringVector new_argv;
  for (int i = 0; i < argc; ++i)
    new_argv.push_back(argv[i]);
  InitFromArgv(new_argv);
}

void CommandLine::InitFromArgv(const StringVector& argv) {
  argv_ = StringVector(1);
  switches_.clear();
  begin_args_ = 1;
  SetProgram(argv.empty() ? "" : argv[0]);
  AppendSwitchesAndArguments(*this, argv);
}

std::string CommandLine::GetProgram() const {
  return argv_[0];
}

void CommandLine::SetProgram(const std::string& program) {
  TrimWhitespaceASCII(program, TRIM_ALL, &argv_[0]);
}

bool CommandLine::HasSwitch(const std::string& switch_string) const {
  return switches_.find(switch_string) != switches_.end();
}

std::string CommandLine::GetSwitchValueASCII(
    const std::string& switch_string) const {
  SwitchMap::const_iterator result =
    switches_.find(switch_string);
  return result == switches_.end() ? std::string() : result->second;
}

void CommandLine::AppendSwitch(const std::string& switch_string) {
  AppendSwitchASCII(switch_string, std::string());
}

void CommandLine::AppendSwitchASCII(const std::string& switch_string,
                                     const std::string& value) {
  std::string switch_key(switch_string);
  std::string combined_switch_string(switch_string);
  size_t prefix_length = GetSwitchPrefixLength(combined_switch_string);
  switches_[switch_key.substr(prefix_length)] = value;
  // Preserve existing switch prefixes in |argv_|; only append one if necessary.
  if (prefix_length == 0)
    combined_switch_string = kSwitchPrefixes[0] + combined_switch_string;
  if (!value.empty())
    combined_switch_string += kSwitchValueSeparator + value;
  // Append the switch and update the switches/arguments divider |begin_args_|.
  argv_.insert(argv_.begin() + begin_args_++, combined_switch_string);
}

CommandLine::StringVector CommandLine::GetArgs() const {
  // Gather all arguments after the last switch (may include kSwitchTerminator).
  StringVector args(argv_.begin() + begin_args_, argv_.end());
  // Erase only the first kSwitchTerminator (maybe "--" is a legitimate page?)
  StringVector::iterator switch_terminator =
      std::find(args.begin(), args.end(), kSwitchTerminator);
  if (switch_terminator != args.end())
    args.erase(switch_terminator);
  return args;
}

void CommandLine::AppendArgASCII(const std::string& value) {
  argv_.push_back(value);
}

}  // namespace gestures
