// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_CHANNEL_CAST_CHANNEL_SERVICE_H_
#define COMPONENTS_CAST_CHANNEL_CAST_CHANNEL_SERVICE_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "components/cast_channel/cast_socket.h"

namespace cast_channel {

// Manages, opens, and closes CastSockets.
// This class may be created on any thread. All methods, unless otherwise noted,
// must be invoked on the SequencedTaskRunner given by |task_runner_|.
class CastSocketService {
 public:
  static CastSocketService* GetInstance();

  virtual ~CastSocketService();

  // Returns a pointer to the Logger member variable.
  scoped_refptr<cast_channel::Logger> GetLogger();

  // Removes the CastSocket corresponding to |channel_id| from the
  // CastSocketRegistry. Returns nullptr if no such CastSocket exists.
  std::unique_ptr<CastSocket> RemoveSocket(int channel_id);

  // Returns the socket corresponding to |channel_id| if one exists, or nullptr
  // otherwise.
  virtual CastSocket* GetSocket(int channel_id) const;

  CastSocket* GetSocket(const net::IPEndPoint& ip_endpoint) const;

  // Opens cast socket with |open_params| and invokes |open_cb| when opening
  // operation finishes. If cast socket with |ip_endpoint| already exists,
  // invoke |open_cb| directly with the existing socket.
  // It is the caller's responsibility to ensure |open_params.ip_address| is
  // a valid private IP address as determined by |IsValidCastIPAddress()|.
  // |open_params|: Parameters necessary to open a Cast channel.
  // |open_cb|: OnOpenCallback invoked when cast socket is opened.
  virtual void OpenSocket(const CastSocketOpenParams& open_params,
                          CastSocket::OnOpenCallback open_cb);

  // Adds |observer| to socket service. When socket service opens cast socket,
  // it passes |observer| to opened socket.
  // Does not take ownership of |observer|.
  void AddObserver(CastSocket::Observer* observer);

  // Remove |observer| from each socket in |sockets_|
  void RemoveObserver(CastSocket::Observer* observer);

  // Gets the TaskRunner for accessing this instance. Can be called from any
  // thread.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner() {
    return task_runner_;
  }

  void SetTaskRunnerForTest(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
    task_runner_ = task_runner;
  }

  // Allow test to inject a mock cast socket.
  void SetSocketForTest(std::unique_ptr<CastSocket> socket_for_test);

 private:
  friend class CastSocketServiceTest;
  friend class MockCastSocketService;

  CastSocketService();

  // Adds |socket| to |sockets_| and returns raw pointer of |socket|. Takes
  // ownership of |socket|.
  CastSocket* AddSocket(std::unique_ptr<CastSocket> socket);

  // Used to generate CastSocket id.
  static int last_channel_id_;

  // The collection of CastSocket keyed by channel_id.
  std::map<int, std::unique_ptr<CastSocket>> sockets_;

  // List of socket observers.
  base::ObserverList<CastSocket::Observer> observers_;

  scoped_refptr<Logger> logger_;

  std::unique_ptr<CastSocket> socket_for_test_;

  // The task runner on which |this| runs.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(CastSocketService);
};

}  // namespace cast_channel

#endif  // COMPONENTS_CAST_CHANNEL_CAST_CHANNEL_SERVICE_H_
