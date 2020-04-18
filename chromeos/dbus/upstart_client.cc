// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/upstart_client.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace chromeos {

namespace {

const char kUpstartServiceName[] = "com.ubuntu.Upstart";
const char kUpstartJobInterface[] = "com.ubuntu.Upstart0_6.Job";
const char kUpstartStartMethod[] = "Start";
const char kUpstartRestartMethod[] = "Restart";
const char kUpstartStopMethod[] = "Stop";

const char kUpstartAuthPolicyPath[] = "/com/ubuntu/Upstart/jobs/authpolicyd";
const char kUpstartMediaAnalyticsPath[] =
    "/com/ubuntu/Upstart/jobs/rtanalytics";

class UpstartClientImpl : public UpstartClient {
 public:
  UpstartClientImpl() : weak_ptr_factory_(this) {}

  ~UpstartClientImpl() override = default;

  // UpstartClient override.
  void StartAuthPolicyService() override {
    dbus::MethodCall method_call(kUpstartJobInterface, kUpstartStartMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendArrayOfStrings(std::vector<std::string>());
    writer.AppendBool(true /* wait for response */);
    auth_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&UpstartClientImpl::HandleAuthResponse,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void RestartAuthPolicyService() override {
    dbus::MethodCall method_call(kUpstartJobInterface, kUpstartRestartMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendArrayOfStrings(std::vector<std::string>());
    writer.AppendBool(true /* wait for response */);
    auth_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&UpstartClientImpl::HandleAuthResponse,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void StartMediaAnalytics(const std::vector<std::string>& upstart_env,
                           VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(kUpstartJobInterface, kUpstartStartMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendArrayOfStrings(upstart_env);
    writer.AppendBool(true /* wait for response */);
    ma_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&UpstartClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void RestartMediaAnalytics(VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(kUpstartJobInterface, kUpstartRestartMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendArrayOfStrings(std::vector<std::string>());
    writer.AppendBool(true /* wait for response */);
    ma_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&UpstartClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void StopMediaAnalytics() override {
    dbus::MethodCall method_call(kUpstartJobInterface, kUpstartStopMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendArrayOfStrings(std::vector<std::string>());
    writer.AppendBool(true /* wait for response */);
    ma_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&UpstartClientImpl::HandleStopMediaAnalyticsResponse,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void StopMediaAnalytics(VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(kUpstartJobInterface, kUpstartStopMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendArrayOfStrings(std::vector<std::string>());
    writer.AppendBool(true /* wait for response */);
    ma_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&UpstartClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

 protected:
  void Init(dbus::Bus* bus) override {
    bus_ = bus;
    auth_proxy_ = bus_->GetObjectProxy(
        kUpstartServiceName, dbus::ObjectPath(kUpstartAuthPolicyPath));
    ma_proxy_ = bus_->GetObjectProxy(
        kUpstartServiceName, dbus::ObjectPath(kUpstartMediaAnalyticsPath));
  }

 private:
  void HandleAuthResponse(dbus::Response* response) {
    LOG_IF(ERROR, !response) << "Failed to signal Upstart, response is null";
  }

  void OnVoidMethod(VoidDBusMethodCallback callback, dbus::Response* response) {
    std::move(callback).Run(response);
  }

  void HandleStopMediaAnalyticsResponse(dbus::Response* response) {
    LOG_IF(ERROR, !response) << "Failed to signal Upstart, response is null";
  }

  dbus::Bus* bus_ = nullptr;
  dbus::ObjectProxy* auth_proxy_ = nullptr;
  dbus::ObjectProxy* ma_proxy_ = nullptr;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<UpstartClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UpstartClientImpl);
};

}  // namespace

UpstartClient::UpstartClient() = default;

UpstartClient::~UpstartClient() = default;

// static
UpstartClient* UpstartClient::Create() {
  return new UpstartClientImpl();
}

}  // namespace chromeos
