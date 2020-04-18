// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_SERIAL_SERIAL_EVENT_DISPATCHER_H_
#define EXTENSIONS_BROWSER_API_SERIAL_SERIAL_EVENT_DISPATCHER_H_

#include <string>
#include <vector>

#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/common/api/serial.h"

namespace content {
class BrowserContext;
}

namespace extensions {

struct Event;
class SerialConnection;

namespace api {

// Per-browser-context dispatcher for events on serial connections.
class SerialEventDispatcher : public BrowserContextKeyedAPI {
 public:
  explicit SerialEventDispatcher(content::BrowserContext* context);
  ~SerialEventDispatcher() override;

  // Start receiving data and firing events for a connection.
  void PollConnection(const std::string& extension_id, int connection_id);

  static SerialEventDispatcher* Get(content::BrowserContext* context);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<SerialEventDispatcher>*
      GetFactoryInstance();

 private:
  typedef ApiResourceManager<SerialConnection>::ApiResourceData ConnectionData;
  friend class BrowserContextKeyedAPIFactory<SerialEventDispatcher>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "SerialEventDispatcher"; }
  static const bool kServiceHasOwnInstanceInIncognito = true;
  static const bool kServiceIsNULLWhileTesting = true;

  struct ReceiveParams {
    ReceiveParams();
    ReceiveParams(const ReceiveParams& other);
    ~ReceiveParams();

    content::BrowserThread::ID thread_id;
    void* browser_context_id;
    std::string extension_id;
    scoped_refptr<ConnectionData> connections;
    int connection_id;
  };

  static void StartReceive(const ReceiveParams& params);

  static void ReceiveCallback(const ReceiveParams& params,
                              std::vector<char> data,
                              serial::ReceiveError error);

  static void PostEvent(const ReceiveParams& params,
                        std::unique_ptr<extensions::Event> event);

  static void DispatchEvent(void* browser_context_id,
                            const std::string& extension_id,
                            std::unique_ptr<extensions::Event> event);

  content::BrowserThread::ID thread_id_;
  content::BrowserContext* const context_;
  scoped_refptr<ConnectionData> connections_;
};

}  // namespace api

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_SERIAL_SERIAL_EVENT_DISPATCHER_H_
