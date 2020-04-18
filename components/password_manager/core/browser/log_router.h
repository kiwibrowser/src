// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOG_ROUTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOG_ROUTER_H_

#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace password_manager {

class LogManager;
class LogReceiver;

// The router stands between LogManager and LogReceiver instances. Both managers
// and receivers need to register (and unregister) with the router. After that,
// the following communication is enabled:
//   * LogManagers are notified when logging starts or stops being possible
//   * LogReceivers are sent logs routed through LogRouter
class LogRouter {
 public:
  LogRouter();
  ~LogRouter();

  // Passes logs to the router. Only call when there are receivers registered.
  void ProcessLog(const std::string& text);

  // All four (Unr|R)egister* methods below are safe to call from the
  // constructor of the registered object, because they do not call that object,
  // and the router only runs on a single thread.

  // The managers must register to be notified about whether there are some
  // receivers or not. RegisterManager adds |manager| to the right observer list
  // and returns true iff there are some receivers registered.
  bool RegisterManager(LogManager* manager);
  // Remove |manager| from the observers list.
  void UnregisterManager(LogManager* manager);

  // The receivers must register to get updates with new logs in the future.
  // RegisterReceiver adds |receiver| to the right observer list, and returns
  // the logs accumulated so far. (It returns by value, not const ref, to
  // provide a snapshot as opposed to a link to |accumulated_logs_|.)
  std::string RegisterReceiver(LogReceiver* receiver) WARN_UNUSED_RESULT;
  // Remove |receiver| from the observers list.
  void UnregisterReceiver(LogReceiver* receiver);

 private:
  // Observer lists for managers and receivers. The |true| in the template
  // specialisation means that they will check that all observers were removed
  // on destruction.
  base::ObserverList<LogManager, true> managers_;
  base::ObserverList<LogReceiver, true> receivers_;

  // Logs accumulated since the first receiver was registered.
  std::string accumulated_logs_;

  DISALLOW_COPY_AND_ASSIGN(LogRouter);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOG_ROUTER_H_
