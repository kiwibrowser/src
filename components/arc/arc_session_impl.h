// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_SESSION_IMPL_H_
#define COMPONENTS_ARC_ARC_SESSION_IMPL_H_

#include <memory>
#include <ostream>
#include <string>

#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/arc/arc_session.h"

namespace arc {

namespace mojom {
class ArcBridgeHost;
}  // namespace mojom

class ArcSessionImpl : public ArcSession,
                       public chromeos::SessionManagerClient::Observer {
 public:
  // The possible states of the session. Expected state changes are as follows.
  //
  // NOT_STARTED
  // -> StartMiniInstance() ->
  // STARTING_MINI_INSTANCE
  //   -> OnMiniInstanceStarted() ->
  // RUNNING_MINI_INSTANCE.
  //   -> RequestUpgrade() ->
  // STARTING_FULL_INSTANCE
  //   -> OnUpgraded() ->
  // CONNECTING_MOJO
  //   -> OnMojoConnected() ->
  // RUNNING_FULL_INSTANCE
  //
  // Note that, if RequestUpgrade() is called during STARTING_MINI_INSTANCE
  // state, the state change to STARTING_FULL_INSTANCE is suspended until
  // the state becomes RUNNING_MINI_INSTANCE.
  //
  // At any state, Stop() can be called. It may not immediately stop the
  // instance, but will eventually stop it. The actual stop will be notified
  // via ArcSession::Observer::OnSessionStopped().
  //
  // When Stop() is called, it makes various behavior based on the current
  // phase.
  //
  // NOT_STARTED:
  //   Do nothing. Immediately transition to the STOPPED state.
  // STARTING_{MINI,FULL}_INSTANCE:
  //   The ARC instance is starting via SessionManager. Stop() just sets the
  //   flag and return. On the main task completion, a callback will run on the
  //   thread, and the flag is checked at the beginning of them. This should
  //   work under the assumption that the main tasks do not block indefinitely.
  //   In its callback, it checks if ARC instance is successfully started or
  //   not. In case of success, a request to stop the ARC instance is sent to
  //   SessionManager. Its completion will be notified via ArcInstanceStopped.
  //   Otherwise, it just turns into STOPPED state.
  // CONNECTING_MOJO:
  //   The main task runs on TaskScheduler's thread, but it is a blocking call.
  //   So, Stop() sends a request to cancel the blocking by closing the pipe
  //   whose read side is also polled. Then, in its callback, similar to
  //   STARTING_{MINI,FULL}_INSTANCE, a request to stop the ARC instance is
  //   sent to SessionManager, and ArcInstanceStopped handles remaining
  //   procedure.
  // RUNNING_{MINI,FULL}_INSTANCE:
  //   There is no more callback which runs on normal flow, so Stop() requests
  //   to stop the ARC instance via SessionManager.
  //
  // Another trigger to change the state coming from outside of this class
  // is an event ArcInstanceStopped() sent from SessionManager, when ARC
  // instace unexpectedly terminates. ArcInstanceStopped() turns the state into
  // STOPPED immediately.
  //
  // In NOT_STARTED or STOPPED state, the instance can be safely destructed.
  // Specifically, in STOPPED state, there may be inflight operations or
  // pending callbacks. Though, what they do is just do-nothing conceptually
  // and they can be safely ignored.
  //
  // Note: Order of constants below matters. Please make sure to sort them
  // in chronological order.
  enum class State {
    // ARC is not yet started.
    NOT_STARTED,

    // The request to start a mini instance has been sent.
    STARTING_MINI_INSTANCE,

    // The instance is set up, but only a handful of processes NOT including
    // arcbridgeservice (i.e. mojo endpoint) are running.
    RUNNING_MINI_INSTANCE,

    // The request to upgrade to a full instance has been sent.
    STARTING_FULL_INSTANCE,

    // The instance has started. Waiting for it to connect to the IPC bridge.
    CONNECTING_MOJO,

    // The instance is fully set up.
    RUNNING_FULL_INSTANCE,

    // ARC is terminated.
    STOPPED,
  };

  static const char kPackagesCacheModeCopy[];
  static const char kPackagesCacheModeSkipCopy[];

  // Delegate interface to emulate ArcBridgeHost mojo connection establishment.
  class Delegate {
   public:
    // Used for ConnectMojo completion callback.
    using ConnectMojoCallback =
        base::OnceCallback<void(std::unique_ptr<mojom::ArcBridgeHost>)>;

    virtual ~Delegate() = default;

    // Connects ArcBridgeHost via |socket_fd|, and invokes |callback| with
    // connected ArcBridgeHost instance if succeeded (or nullptr if failed).
    // Returns a FD which cancels the current connection on close(2).
    virtual base::ScopedFD ConnectMojo(base::ScopedFD socket_fd,
                                       ConnectMojoCallback callback) = 0;
  };

  explicit ArcSessionImpl(std::unique_ptr<Delegate> delegate);
  ~ArcSessionImpl() override;

  // Returns default delegate implementation used for the production.
  static std::unique_ptr<Delegate> CreateDelegate(
      ArcBridgeService* arc_bridge_service);

  State GetStateForTesting() { return state_; }

  // ArcSession overrides:
  void StartMiniInstance() override;
  void RequestUpgrade() override;
  void Stop() override;
  bool IsStopRequested() override;
  void OnShutdown() override;

 private:
  // D-Bus callback for StartArcMiniContainer().
  void OnMiniInstanceStarted(base::Optional<std::string> container_instance_id);

  // Sends a D-Bus message to upgrade to a full instance.
  void DoUpgrade();

  // D-Bus callback for UpgradeArcContainer(). |socket_fd| should be a socket
  // which should be accept(2)ed to connect ArcBridgeService Mojo channel.
  void OnUpgraded(base::ScopedFD socket_fd);

  // D-Bus callback for UpgradeArcContainer when the upgrade fails.
  // |low_free_disk_space| signals whether the failure was due to low free disk
  // space.
  void OnUpgradeError(bool low_free_disk_space);

  // Called when Mojo connection is established (or canceled during the
  // connect.)
  void OnMojoConnected(std::unique_ptr<mojom::ArcBridgeHost> arc_bridge_host);

  // Request to stop ARC instance via DBus.
  void StopArcInstance();

  // chromeos::SessionManagerClient::Observer:
  void ArcInstanceStopped(login_manager::ArcContainerStopReason stop_reason,
                          const std::string& container_instance_id) override;

  // Completes the termination procedure. Note that calling this may end up with
  // deleting |this| because the function calls observers' OnSessionStopped().
  void OnStopped(ArcStopReason reason);

  // Checks whether a function runs on the thread where the instance is
  // created.
  THREAD_CHECKER(thread_checker_);

  // Delegate implementation.
  std::unique_ptr<Delegate> delegate_;

  // The state of the session.
  State state_ = State::NOT_STARTED;

  // When Stop() is called, this flag is set.
  bool stop_requested_ = false;

  // Whether the full container has been requested
  bool upgrade_requested_ = false;

  // Container instance id passed from session_manager.
  // Should be available only after On{Mini,Full}InstanceStarted().
  std::string container_instance_id_;

  // In CONNECTING_MOJO state, this is set to the write side of the pipe
  // to notify cancelling of the procedure.
  base::ScopedFD accept_cancel_pipe_;

  // Mojo endpoint.
  std::unique_ptr<mojom::ArcBridgeHost> arc_bridge_host_;

  // WeakPtrFactory to use callbacks.
  base::WeakPtrFactory<ArcSessionImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcSessionImpl);
};

// Stringified output for logging purpose.
std::ostream& operator<<(std::ostream& os, ArcSessionImpl::State state);

}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_SESSION_IMPL_H_
