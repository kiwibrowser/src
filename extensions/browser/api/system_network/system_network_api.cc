// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/system_network/system_network_api.h"

#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"

namespace {
const char kNetworkListError[] = "Network lookup failed or unsupported";

std::unique_ptr<net::NetworkInterfaceList> GetListOnBlockingTaskRunner() {
  auto interface_list = std::make_unique<net::NetworkInterfaceList>();
  if (net::GetNetworkList(interface_list.get(),
                          net::INCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES)) {
    return interface_list;
  }

  return nullptr;
}

}  // namespace

namespace extensions {
namespace api {

SystemNetworkGetNetworkInterfacesFunction::
    SystemNetworkGetNetworkInterfacesFunction() {
}

SystemNetworkGetNetworkInterfacesFunction::
    ~SystemNetworkGetNetworkInterfacesFunction() {
}

ExtensionFunction::ResponseAction
SystemNetworkGetNetworkInterfacesFunction::Run() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  constexpr base::TaskTraits kTraits = {
      base::MayBlock(), base::TaskPriority::USER_VISIBLE,
      base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN};
  using Self = SystemNetworkGetNetworkInterfacesFunction;
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, kTraits, base::BindOnce(&GetListOnBlockingTaskRunner),
      base::BindOnce(&Self::SendResponseOnUIThread, this));
  return RespondLater();
}

void SystemNetworkGetNetworkInterfacesFunction::SendResponseOnUIThread(
    std::unique_ptr<net::NetworkInterfaceList> interface_list) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!interface_list) {
    Respond(Error(kNetworkListError));
    return;
  }

  std::vector<api::system_network::NetworkInterface> create_arg;
  create_arg.reserve(interface_list->size());
  for (const net::NetworkInterface& interface : *interface_list) {
    api::system_network::NetworkInterface info;
    info.name = interface.name;
    info.address = interface.address.ToString();
    info.prefix_length = interface.prefix_length;
    create_arg.push_back(std::move(info));
  }

  Respond(ArgumentList(
      api::system_network::GetNetworkInterfaces::Results::Create(create_arg)));
}

}  // namespace api
}  // namespace extensions
