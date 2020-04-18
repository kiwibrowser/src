// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Interfaces for the system resources used by the Ticl. System resources are an
// abstraction layer over the host operating system that provides the Ticl with
// the ability to schedule events, send network messages, store data, and
// perform logging.
//
// NOTE: All the resource types and SystemResources are required to be
// thread-safe.

#ifndef GOOGLE_CACHEINVALIDATION_INCLUDE_SYSTEM_RESOURCES_H_
#define GOOGLE_CACHEINVALIDATION_INCLUDE_SYSTEM_RESOURCES_H_

#include <string>
#include <utility>

#include "google/cacheinvalidation/deps/callback.h"
#include "google/cacheinvalidation/deps/stl-namespace.h"
#include "google/cacheinvalidation/deps/time.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::pair;
using INVALIDATION_STL_NAMESPACE::string;

class Status;
class SystemResources;  // Declared below.

typedef pair<Status, string> StatusStringPair;
typedef INVALIDATION_CALLBACK1_TYPE(string) MessageCallback;
typedef INVALIDATION_CALLBACK1_TYPE(bool) NetworkStatusCallback;
typedef INVALIDATION_CALLBACK1_TYPE(StatusStringPair) ReadKeyCallback;
typedef INVALIDATION_CALLBACK1_TYPE(Status) WriteKeyCallback;
typedef INVALIDATION_CALLBACK1_TYPE(bool) DeleteKeyCallback;
typedef INVALIDATION_CALLBACK1_TYPE(StatusStringPair) ReadAllKeysCallback;

/* Interface for a component of a SystemResources implementation constructed by
 * calls to set* methods of SystemResourcesBuilder.
 *
 * The SystemResourcesBuilder allows applications to create a single
 * SystemResources implementation by composing individual building blocks, each
 * of which implements one of the four required interfaces (Logger, Storage,
 * NetworkChannel, Scheduler).
 *
 * However, each interface implementation may require functionality from
 * another. For example, the network implementation may need to do logging. In
 * order to allow this, we require that the interface implementations also
 * implement ResourceComponent, which specifies the single method
 * SetSystemResources. It is guaranteed that this method will be invoked exactly
 * once on each interface implementation and before any other calls are
 * made. Implementations can then save a reference to the provided resources for
 * later use.
 *
 * Note: for the obvious reasons of infinite recursion, implementations should
 * not attempt to access themselves through the provided SystemResources.
 */
class ResourceComponent {
 public:
  virtual ~ResourceComponent() {}

  /* Supplies a |SystemResources| instance to the component. */
  virtual void SetSystemResources(SystemResources* resources) = 0;
};

/* Interface specifying the logging functionality provided by
 * SystemResources.
 */
class Logger : public ResourceComponent {
 public:
  enum LogLevel {
    FINE_LEVEL,
    INFO_LEVEL,
    WARNING_LEVEL,
    SEVERE_LEVEL
  };

  virtual ~Logger() {}

  /* Logs a message.
   *
   * Arguments:
   *     level - the level at which the message should be logged (e.g., INFO)
   *     file - the file from which the message is being logged
   *     line - the line number from which the message is being logged
   *     template - the string to log, optionally containing %s sequences
   *     ... - values to substitute for %s sequences in template
   */
  virtual void Log(LogLevel level, const char* file, int line,
                   const char* format, ...) = 0;
};

/* Interface specifying the scheduling functionality provided by
 * SystemResources.
 */
class Scheduler : public ResourceComponent {
 public:
  virtual ~Scheduler() {}

  /* Function returning a zero time delta, for readability. */
  static TimeDelta NoDelay() {
    return TimeDelta::FromMilliseconds(0);
  }

  /* Schedules runnable to be run on scheduler's thread after at least
   * delay.
   * Callee owns the runnable and must delete it after the task has run
   * (or if the scheduler is shut down before the task has run).
   */
  virtual void Schedule(TimeDelta delay, Closure* runnable) = 0;

  /* Returns whether the current code is executing on the scheduler's thread.
   */
  virtual bool IsRunningOnThread() const = 0;

  /* Returns the current time in milliseconds since *some* epoch (NOT
   * necessarily the UNIX epoch).  The only requirement is that this time
   * advance at the rate of real time.
   */
  virtual Time GetCurrentTime() const = 0;
};

/* Interface specifying the network functionality provided by
 * SystemResources.
 */
class NetworkChannel : public ResourceComponent {
 public:
  virtual ~NetworkChannel() {}

  /* Sends outgoing_message to the data center. */
  // Implementation note: this is currently a serialized ClientToServerMessage
  // protocol buffer.  Implementors MAY NOT rely on this fact.
  virtual void SendMessage(const string& outgoing_message) = 0;

  /* Sets the receiver to which messages from the data center will be delivered.
   * Ownership of |incoming_receiver| is transferred to the network channel.
   */
  // Implementation note: this is currently a serialized ServerToClientMessage
  // protocol buffer.  Implementors MAY NOT rely on this fact.
  virtual void SetMessageReceiver(MessageCallback* incoming_receiver) = 0;

  /* Informs the network channel that network_status_receiver be informed about
   * changes to network status changes. If the network is connected, the channel
   * should call network_Status_Receiver->Run(true) and when the network is
   * disconnected, it should call network_status_receiver->Run(false). Note that
   * multiple receivers can be registered with the channel to receive such
   * status updates.
   *
   * The informing of the status to the network_status_receiver can be
   * implemented in a best-effort manner with the caveat that indicating
   * incorrectly that the network is connected can result in unnecessary calls
   * for SendMessage. Incorrect information that the network is disconnected can
   * result in messages not being sent by the client library.
   *
   * Ownership of network_status_receiver is transferred to the network channel.
   */
  virtual void AddNetworkStatusReceiver(
      NetworkStatusCallback* network_status_receiver) = 0;
};

/* Interface specifying the storage functionality provided by
 * SystemResources. Basically, the required functionality is a small subset of
 * the method of a regular hash map.
 */
class Storage : public ResourceComponent {
 public:
  virtual ~Storage() {}

  /* Attempts to persist value for the given key. Invokes done when finished,
   * passing a value that indicates whether it was successful.
   *
   * Note: If a wrie W1 finishes unsuccessfully and then W2 is issued for the
   * same key and W2 finishes successfully, W1 must NOT later overwrite W2.
   * Callee owns |done| after this call. After it calls |done->Run()|, it must
   * delete |done|.
   *
   * REQUIRES: Neither key nor value is null.
   */
  virtual void WriteKey(const string& key, const string& value,
                        WriteKeyCallback* done) = 0;

  /* Reads the value corresponding to key and calls done with the result.  If it
   * finds the key, passes a success status and the value. Else passes a failure
   * status and a null value.
   * Callee owns |done| after this call. After it calls |done->Run()|, it must
   * delete |done|.
   */
  virtual void ReadKey(const string& key, ReadKeyCallback* done) = 0;

  /* Deletes the key, value pair corresponding to key. If the deletion succeeds,
   * calls done with true; else calls it with false.
   * Callee owns |done| after this call. After it calls |done->Run()|, it must
   * delete |done|.
   */
  virtual void DeleteKey(const string& key, DeleteKeyCallback* done) = 0;

  /* Reads all the keys from the underlying store and then calls key_callback
   * with each key that was written earlier and not deleted. When all the keys
   * are done, calls key_callback with null. With each key, the code can
   * indicate a failed status, in which case the iteration stops.
   * Caller continues to own |key_callback|.
   */
  virtual void ReadAllKeys(ReadAllKeysCallback* key_callback) = 0;
};

class SystemResources {
 public:
  virtual ~SystemResources() {}

  /* Starts the resources.
   *
   * REQUIRES: This method is called before the resources are used.
   */
  virtual void Start() = 0;

  /* Stops the resources. After this point, all the resources will eventually
   * stop doing any work (e.g., scheduling, sending/receiving messages from the
   * network etc). They will eventually convert any further operations to
   * no-ops.
   *
   * REQUIRES: Start has been called.
   */
  virtual void Stop() = 0;

  /* Returns whether the resources are started. */
  virtual bool IsStarted() const = 0;

  /* Returns information about the client operating system/platform, e.g.,
   * Windows, ChromeOS (for debugging/monitoring purposes).
   */
  virtual string platform() const = 0;

  /* Returns an object that can be used to do logging. */
  virtual Logger* logger() = 0;

  /* Returns an object that can be used to persist data locally. */
  virtual Storage* storage() = 0;

  /* Returns an object that can be used to send and receive messages. */
  virtual NetworkChannel* network() = 0;

  /* Returns an object that can be used by the client library to schedule its
   * internal events.
   */
  virtual Scheduler* internal_scheduler() = 0;

  /* Returns an object that can be used to schedule events for the
   * application.
   */
  virtual Scheduler* listener_scheduler() = 0;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_INCLUDE_SYSTEM_RESOURCES_H_
