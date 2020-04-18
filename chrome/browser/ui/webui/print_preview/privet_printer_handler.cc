// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/privet_printer_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/timer/timer.h"
#include "chrome/browser/printing/cloud_print/privet_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_utils.h"
#include "chrome/common/chrome_switches.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ui/gfx/geometry/size.h"

namespace {
// Timeout for searching for privet printers, in seconds.
const int kSearchTimeoutSec = 5;

void FillPrinterDescription(const std::string& name,
                            const cloud_print::DeviceDescription& description,
                            bool has_local_printing,
                            base::DictionaryValue* printer_value) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  printer_value->SetString("serviceName", name);
  printer_value->SetString("name", description.name);
  printer_value->SetBoolean("hasLocalPrinting", has_local_printing);
  printer_value->SetBoolean(
      "isUnregistered",
      description.id.empty() &&
          command_line->HasSwitch(switches::kEnablePrintPreviewRegisterPromos));
  printer_value->SetString("cloudID", description.id);
}
}  //  namespace

PrivetPrinterHandler::PrivetPrinterHandler(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {}

PrivetPrinterHandler::~PrivetPrinterHandler() {}

void PrivetPrinterHandler::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  added_printers_callback_.Reset();
  if (done_callback_)
    std::move(done_callback_).Run();
}

void PrivetPrinterHandler::StartGetPrinters(
    const AddedPrintersCallback& added_printers_callback,
    GetPrintersDoneCallback done_callback) {
  using local_discovery::ServiceDiscoverySharedClient;
  scoped_refptr<ServiceDiscoverySharedClient> service_discovery =
      ServiceDiscoverySharedClient::GetInstance();
  DCHECK(!added_printers_callback_);
  DCHECK(!done_callback_);
  added_printers_callback_ = added_printers_callback;
  done_callback_ = std::move(done_callback);
  StartLister(service_discovery);
}

void PrivetPrinterHandler::StartGetCapability(const std::string& destination_id,
                                              GetCapabilityCallback callback) {
  DCHECK(!capabilities_callback_);
  capabilities_callback_ = std::move(callback);
  CreateHTTP(destination_id,
             base::Bind(&PrivetPrinterHandler::CapabilitiesUpdateClient,
                        weak_ptr_factory_.GetWeakPtr()));
}

void PrivetPrinterHandler::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedMemory>& print_data,
    PrintCallback callback) {
  DCHECK(!print_callback_);
  print_callback_ = std::move(callback);
  CreateHTTP(destination_id,
             base::Bind(&PrivetPrinterHandler::PrintUpdateClient,
                        weak_ptr_factory_.GetWeakPtr(), job_title, print_data,
                        ticket_json, capability, page_size));
}

void PrivetPrinterHandler::LocalPrinterChanged(
    const std::string& name,
    bool has_local_printing,
    const cloud_print::DeviceDescription& description) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!added_printers_callback_ ||
      (!has_local_printing &&
       !command_line->HasSwitch(switches::kEnablePrintPreviewRegisterPromos))) {
    // If Print Preview is not expecting this printer (callback reset or no
    // registration promos and not a local printer), return early.
    return;
  }
  auto printer_info = std::make_unique<base::DictionaryValue>();
  FillPrinterDescription(name, description, has_local_printing,
                         printer_info.get());
  base::ListValue printers;
  printers.Set(0, std::move(printer_info));
  added_printers_callback_.Run(printers);
}

void PrivetPrinterHandler::LocalPrinterRemoved(const std::string& name) {}

void PrivetPrinterHandler::LocalPrinterCacheFlushed() {}

void PrivetPrinterHandler::OnPrivetPrintingDone(
    const cloud_print::PrivetLocalPrintOperation* print_operation) {
  DCHECK(print_callback_);
  std::move(print_callback_).Run(base::Value());
}

void PrivetPrinterHandler::OnPrivetPrintingError(
    const cloud_print::PrivetLocalPrintOperation* print_operation,
    int http_code) {
  DCHECK(print_callback_);
  std::move(print_callback_).Run(base::Value(http_code));
}

void PrivetPrinterHandler::StartLister(
    const scoped_refptr<local_discovery::ServiceDiscoverySharedClient>&
        client) {
  DCHECK(!service_discovery_client_.get() ||
         service_discovery_client_.get() == client.get());
  service_discovery_client_ = client;
  printer_lister_ = std::make_unique<cloud_print::PrivetLocalPrinterLister>(
      service_discovery_client_.get(), profile_->GetRequestContext(), this);
  privet_lister_timer_ = std::make_unique<base::OneShotTimer>();
  privet_lister_timer_->Start(FROM_HERE,
                              base::TimeDelta::FromSeconds(kSearchTimeoutSec),
                              this, &PrivetPrinterHandler::StopLister);
  printer_lister_->Start();
}

void PrivetPrinterHandler::StopLister() {
  privet_lister_timer_.reset();
  if (printer_lister_)
    printer_lister_->Stop();
  std::move(done_callback_).Run();
  added_printers_callback_.Reset();
}

void PrivetPrinterHandler::CapabilitiesUpdateClient(
    std::unique_ptr<cloud_print::PrivetHTTPClient> http_client) {
  if (!UpdateClient(std::move(http_client))) {
    DCHECK(capabilities_callback_);
    std::move(capabilities_callback_).Run(nullptr);
    return;
  }

  privet_capabilities_operation_ =
      privet_http_client_->CreateCapabilitiesOperation(
          base::Bind(&PrivetPrinterHandler::OnGotCapabilities,
                     weak_ptr_factory_.GetWeakPtr()));
  privet_capabilities_operation_->Start();
}

void PrivetPrinterHandler::OnGotCapabilities(
    const base::DictionaryValue* capabilities) {
  DCHECK(capabilities_callback_);
  if (!capabilities || capabilities->HasKey(cloud_print::kPrivetKeyError) ||
      !printer_lister_) {
    std::move(capabilities_callback_).Run(nullptr);
    return;
  }

  std::string name = privet_capabilities_operation_->GetHTTPClient()->GetName();
  const cloud_print::DeviceDescription* description =
      printer_lister_->GetDeviceDescription(name);

  if (!description) {
    std::move(capabilities_callback_).Run(nullptr);
    return;
  }

  std::unique_ptr<base::DictionaryValue> printer_info =
      std::make_unique<base::DictionaryValue>();
  FillPrinterDescription(name, *description, true, printer_info.get());
  base::DictionaryValue printer_info_and_caps;
  printer_info_and_caps.SetDictionary(cloud_print::kPrivetTypePrinter,
                                      std::move(printer_info));
  std::unique_ptr<base::DictionaryValue> capabilities_copy =
      capabilities->CreateDeepCopy();
  printer_info_and_caps.SetDictionary(printing::kSettingCapabilities,
                                      std::move(capabilities_copy));
  std::move(capabilities_callback_)
      .Run(printing::ValidateCddForPrintPreview(printer_info_and_caps));
  privet_capabilities_operation_.reset();
}

void PrivetPrinterHandler::PrintUpdateClient(
    const base::string16& job_title,
    const scoped_refptr<base::RefCountedMemory>& print_data,
    const std::string& print_ticket,
    const std::string& capabilities,
    const gfx::Size& page_size,
    std::unique_ptr<cloud_print::PrivetHTTPClient> http_client) {
  if (!UpdateClient(std::move(http_client))) {
    DCHECK(print_callback_);
    std::move(print_callback_).Run(base::Value(-1));
    return;
  }
  StartPrint(job_title, print_data, print_ticket, capabilities, page_size);
}

bool PrivetPrinterHandler::UpdateClient(
    std::unique_ptr<cloud_print::PrivetHTTPClient> http_client) {
  if (!http_client) {
    privet_http_resolution_.reset();
    return false;
  }

  privet_local_print_operation_.reset();
  privet_capabilities_operation_.reset();
  privet_http_client_ =
      cloud_print::PrivetV1HTTPClient::CreateDefault(std::move(http_client));

  privet_http_resolution_.reset();

  return true;
}

void PrivetPrinterHandler::StartPrint(
    const base::string16& job_title,
    const scoped_refptr<base::RefCountedMemory>& print_data,
    const std::string& print_ticket,
    const std::string& capabilities,
    const gfx::Size& page_size) {
  privet_local_print_operation_ =
      privet_http_client_->CreateLocalPrintOperation(this);

  privet_local_print_operation_->SetTicket(print_ticket);
  privet_local_print_operation_->SetCapabilities(capabilities);
  privet_local_print_operation_->SetJobname(base::UTF16ToUTF8(job_title));
  privet_local_print_operation_->SetPageSize(page_size);
  privet_local_print_operation_->SetData(print_data);

  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfileIfExists(profile_);
  if (signin_manager) {
    privet_local_print_operation_->SetUsername(
        signin_manager->GetAuthenticatedAccountInfo().email);
  }

  privet_local_print_operation_->Start();
}

void PrivetPrinterHandler::CreateHTTP(
    const std::string& name,
    cloud_print::PrivetHTTPAsynchronousFactory::ResultCallback callback) {
  const cloud_print::DeviceDescription* device_description =
      printer_lister_ ? printer_lister_->GetDeviceDescription(name) : nullptr;

  if (!device_description) {
    callback.Run(nullptr);
    return;
  }

  privet_http_factory_ =
      cloud_print::PrivetHTTPAsynchronousFactory::CreateInstance(
          profile_->GetRequestContext());
  privet_http_resolution_ = privet_http_factory_->CreatePrivetHTTP(name);
  privet_http_resolution_->Start(device_description->address, callback);
}
