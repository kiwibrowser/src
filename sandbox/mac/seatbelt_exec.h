// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_MAC_SEATBELT_EXEC_H_
#define SANDBOX_MAC_SEATBELT_EXEC_H_

#include <string>

#include "base/compiler_specific.h"
#include "sandbox/mac/seatbelt.pb.h"
#include "sandbox/mac/seatbelt_export.h"

namespace sandbox {

// SeatbeltExecClient is used by the process that is launching another sandboxed
// process. The API allows the launcher process to supply a sandbox profile and
// parameters, which will be communicated to the sandboxed process over IPC.
class SEATBELT_EXPORT SeatbeltExecClient {
 public:
  SeatbeltExecClient();
  ~SeatbeltExecClient();

  // The Set*Parameter functions return true if the parameter was successfully
  // inserted. Check the return value, which indicates if the parameter was
  // added successfully.

  // Set a boolean parameter in the sandbox profile.
  bool SetBooleanParameter(const std::string& key,
                           bool value) WARN_UNUSED_RESULT;

  // Set a string parameter in the sandbox profile.
  bool SetParameter(const std::string& key,
                    const std::string& value) WARN_UNUSED_RESULT;

  // Set the actual sandbox profile, using the scheme-like SBPL.
  void SetProfile(const std::string& policy);

  // This returns the FD used for reading the sandbox profile in the child
  // process. The FD should be mapped into the sandboxed child process.
  // This must be called before SendProfile() or the returned FD will be -1.
  // Callers should check that the returned FD is valid.
  int GetReadFD();

  // Sends the policy to the SeatbeltExecServer and returns success or failure.
  bool SendProfile();

  // Returns the underlying protobuf for testing purposes.
  const mac::SandboxPolicy& GetPolicyForTesting() { return policy_; }

 private:
  // This writes a string (the serialized protobuf) to the |pipe_|.
  bool WriteString(const std::string& str);

  // This is the protobuf which contains the sandbox profile and parameters,
  // and is serialized and sent to the other process.
  mac::SandboxPolicy policy_;

  // A file descriptor pair used for interprocess communication.
  int pipe_[2];
};

// SeatbeltExecServer is used by the process that will be sandboxed to receive
// the profile and parameters from the launcher process. It can then initialize
// the profile, sandboxing the process.
class SEATBELT_EXPORT SeatbeltExecServer {
 public:
  // |sandbox_fd| should be the result of SendProfileAndGetFD().
  explicit SeatbeltExecServer(int sandbox_fd);
  ~SeatbeltExecServer();

  // Reads the policy from the client, applies the profile, and returns whether
  // or not the operation succeeds.
  bool InitializeSandbox();

  // Applies the given sandbox policy, and returns whether or not the operation
  // succeeds.
  bool ApplySandboxProfile(const mac::SandboxPolicy& sandbox_policy);

  // Set a string parameter in the sandbox profile. This is present in the
  // server because the process about to initialize a sandbox may need to add
  // some extra parameters, such as the path to the executable or the current
  // PID. This must be called before InitializeSandbox().
  bool SetParameter(const std::string& key,
                    const std::string& value) WARN_UNUSED_RESULT;

 private:
  // Reads from the |fd_| and stores the data into a string. This does
  // not append a NUL terminator as protobuf does not expect one.
  bool ReadString(std::string* string);

  // The file descriptor used to communicate with the launcher process.
  int fd_;

  // Extra parameters added by the server process.
  std::map<std::string, std::string> extra_params_;
};

}  // namespace sandbox

#endif  // SANDBOX_MAC_SEATBELT_EXEC_H_
