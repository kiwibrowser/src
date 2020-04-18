// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_SERIAL_SERIAL_API_H_
#define EXTENSIONS_BROWSER_API_SERIAL_SERIAL_API_H_

#include <string>

#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/browser/api/async_api_function.h"
#include "extensions/common/api/serial.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace extensions {

class SerialConnection;

namespace api {

class SerialEventDispatcher;

class SerialAsyncApiFunction : public AsyncApiFunction {
 public:
  SerialAsyncApiFunction();

 protected:
  ~SerialAsyncApiFunction() override;

  // AsyncApiFunction:
  bool PrePrepare() override;
  bool Respond() override;

  SerialConnection* GetSerialConnection(int api_resource_id);
  void RemoveSerialConnection(int api_resource_id);

  ApiResourceManager<SerialConnection>* manager_;
};

class SerialGetDevicesFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.getDevices", SERIAL_GETDEVICES)

  SerialGetDevicesFunction();

 protected:
  ~SerialGetDevicesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void OnGotDevices(std::vector<device::mojom::SerialDeviceInfoPtr> devices);

  device::mojom::SerialDeviceEnumeratorPtr enumerator_;

  DISALLOW_COPY_AND_ASSIGN(SerialGetDevicesFunction);
};

class SerialConnectFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.connect", SERIAL_CONNECT)

  SerialConnectFunction();

 protected:
  ~SerialConnectFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnConnected(bool success);
  void FinishConnect(bool connected,
                     bool got_complete_info,
                     std::unique_ptr<serial::ConnectionInfo> info);

  std::unique_ptr<serial::Connect::Params> params_;

  // SerialEventDispatcher is owned by a BrowserContext.
  SerialEventDispatcher* serial_event_dispatcher_;

  // This connection is created within SerialConnectFunction.
  // From there its ownership is transferred to the
  // ApiResourceManager<SerialConnection> upon success.
  std::unique_ptr<SerialConnection> connection_;

  device::mojom::SerialIoHandlerPtrInfo io_handler_info_;
};

class SerialUpdateFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.update", SERIAL_UPDATE);

  SerialUpdateFunction();

 protected:
  ~SerialUpdateFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnUpdated(bool success);

  std::unique_ptr<serial::Update::Params> params_;
};

class SerialDisconnectFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.disconnect", SERIAL_DISCONNECT)

  SerialDisconnectFunction();

 protected:
  ~SerialDisconnectFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void Work() override;

 private:
  std::unique_ptr<serial::Disconnect::Params> params_;
};

class SerialSetPausedFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.setPaused", SERIAL_SETPAUSED)

  SerialSetPausedFunction();

 protected:
  ~SerialSetPausedFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void Work() override;

 private:
  std::unique_ptr<serial::SetPaused::Params> params_;
  SerialEventDispatcher* serial_event_dispatcher_;
};

class SerialGetInfoFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.getInfo", SERIAL_GETINFO)

  SerialGetInfoFunction();

 protected:
  ~SerialGetInfoFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnGotInfo(bool got_complete_info,
                 std::unique_ptr<serial::ConnectionInfo> info);

  std::unique_ptr<serial::GetInfo::Params> params_;
};

class SerialGetConnectionsFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.getConnections", SERIAL_GETCONNECTIONS);

  SerialGetConnectionsFunction();

 protected:
  ~SerialGetConnectionsFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnGotOne(int connection_id,
                bool got_complete_info,
                std::unique_ptr<serial::ConnectionInfo> info);
  void OnGotAll();

  size_t count_ = 0;
  std::vector<serial::ConnectionInfo> infos_;
};

class SerialSendFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.send", SERIAL_SEND)

  SerialSendFunction();

 protected:
  ~SerialSendFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnSendComplete(uint32_t bytes_sent, serial::SendError error);

  std::unique_ptr<serial::Send::Params> params_;
};

class SerialFlushFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.flush", SERIAL_FLUSH)

  SerialFlushFunction();

 protected:
  ~SerialFlushFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnFlushed(bool success);

  std::unique_ptr<serial::Flush::Params> params_;
};

class SerialGetControlSignalsFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.getControlSignals",
                             SERIAL_GETCONTROLSIGNALS)

  SerialGetControlSignalsFunction();

 protected:
  ~SerialGetControlSignalsFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnGotControlSignals(
      std::unique_ptr<serial::DeviceControlSignals> signals);

  std::unique_ptr<serial::GetControlSignals::Params> params_;
};

class SerialSetControlSignalsFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.setControlSignals",
                             SERIAL_SETCONTROLSIGNALS)

  SerialSetControlSignalsFunction();

 protected:
  ~SerialSetControlSignalsFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnSetControlSignals(bool success);

  std::unique_ptr<serial::SetControlSignals::Params> params_;
};

class SerialSetBreakFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.setBreak", SERIAL_SETBREAK)
  SerialSetBreakFunction();

 protected:
  ~SerialSetBreakFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnSetBreak(bool success);

  std::unique_ptr<serial::SetBreak::Params> params_;
};

class SerialClearBreakFunction : public SerialAsyncApiFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("serial.clearBreak", SERIAL_CLEARBREAK)
  SerialClearBreakFunction();

 protected:
  ~SerialClearBreakFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  void AsyncWorkStart() override;

 private:
  void OnClearBreak(bool success);

  std::unique_ptr<serial::ClearBreak::Params> params_;
};

}  // namespace api

}  // namespace extensions

namespace mojo {

template <>
struct TypeConverter<extensions::api::serial::DeviceInfo,
                     device::mojom::SerialDeviceInfoPtr> {
  static extensions::api::serial::DeviceInfo Convert(
      const device::mojom::SerialDeviceInfoPtr& input);
};

}  // namespace mojo

#endif  // EXTENSIONS_BROWSER_API_SERIAL_SERIAL_API_H_
