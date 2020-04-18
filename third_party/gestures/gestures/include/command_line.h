// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class works with command lines: building and parsing.
// Arguments with prefixes ('--', '-', and on Windows, '/') are switches.
// Switches will precede all other arguments without switch prefixes.
// Switches can optionally have values, delimited by '=', e.g., "-switch=value".
// An argument of "--" will terminate switch parsing during initialization,
// interpreting subsequent tokens as non-switch arguments, regardless of prefix.

// There is a singleton read-only CommandLine that represents the command line
// that the current process was started with.  It must be initialized in main().

#ifndef GESTURES_COMMAND_LINE_H_
#define GESTURES_COMMAND_LINE_H_

#include <stddef.h>
#include <map>
#include <string>
#include <vector>

namespace gestures {

class CommandLine {
 public:
  typedef std::string::value_type CharType;
  typedef std::vector<std::string> StringVector;
  typedef std::map<std::string, std::string> SwitchMap;

  CommandLine();
  ~CommandLine();

  // Initialize the current process CommandLine singleton. On Windows, ignores
  // its arguments (we instead parse GetCommandLineW() directly) because we
  // don't trust the CRT's parsing of the command line, but it still must be
  // called to set up the command line. Returns false if initialization has
  // already occurred, and true otherwise. Only the caller receiving a 'true'
  // return value should take responsibility for calling Reset.
  static bool Init(int argc, const char* const* argv);

  // Destroys the current process CommandLine singleton. This is necessary if
  // you want to reset the base library to its initial state (for example, in an
  // outer library that needs to be able to terminate, and be re-initialized).
  // If Init is called only once, as in main(), Reset() is not necessary.
  static void Reset();

  // Get the singleton CommandLine representing the current process's
  // command line. Note: returned value is mutable, but not thread safe;
  // only mutate if you know what you're doing!
  static CommandLine* ForCurrentProcess();

  // Initialize from an argv vector.
  void InitFromArgv(int argc, const CharType* const* argv);
  void InitFromArgv(const StringVector& argv);

  // Returns the original command line string as a vector of strings.
  const StringVector& argv() const { return argv_; }

  // Get and Set the program part of the command line string (the first item).
  std::string GetProgram() const;
  void SetProgram(const std::string& program);

  // Returns true if this command line contains the given switch.
  // (Switch names are case-insensitive).
  bool HasSwitch(const std::string& switch_string) const;

  // Returns the value associated with the given switch. If the switch has no
  // value or isn't present, this method returns the empty string.
  std::string GetSwitchValueASCII(const std::string& switch_string) const;

  // Get a copy of all switches, along with their values.
  const SwitchMap& GetSwitches() const { return switches_; }

  // Append a switch [with optional value] to the command line.
  // Note: Switches will precede arguments regardless of appending order.
  void AppendSwitch(const std::string& switch_string);
  void AppendSwitchASCII(const std::string& switch_string,
                          const std::string& value);

  // Get the remaining arguments to the command.
  StringVector GetArgs() const;

  // Append an argument to the command line. Note that the argument is quoted
  // properly such that it is interpreted as one argument to the target command.
  // AppendArg is primarily for ASCII; non-ASCII input is interpreted as UTF-8.
  // Note: Switches will precede arguments regardless of appending order.
  void AppendArgASCII(const std::string& value);

 private:
  // Allow the copy constructor. A common pattern is to copy of the current
  // process's command line and then add some flags to it. For example:
  //   CommandLine cl(*CommandLine::ForCurrentProcess());
  //   cl.AppendSwitch(...);

  // The singleton CommandLine representing the current process's command line.
  static CommandLine* current_process_commandline_;

  // The argv array: { program, [(--|-|/)switch[=value]]*, [--], [argument]* }
  StringVector argv_;

  // Parsed-out switch keys and values.
  SwitchMap switches_;

  // The index after the program and switches, any arguments start here.
  size_t begin_args_;
};

}  // namespace gestures

#endif  // GESTURES_COMMAND_LINE_H_
