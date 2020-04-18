// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/wmi.h"

#include <windows.h>
#include <objbase.h>
#include <stdint.h>
#include <wrl/client.h>

#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"

using base::win::ScopedVariant;

namespace installer {

bool WMI::CreateLocalConnection(bool set_blanket,
                                IWbemServices** wmi_services) {
  Microsoft::WRL::ComPtr<IWbemLocator> wmi_locator;
  HRESULT hr = ::CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&wmi_locator));
  if (FAILED(hr))
    return false;

  Microsoft::WRL::ComPtr<IWbemServices> wmi_services_r;
  hr = wmi_locator->ConnectServer(base::win::ScopedBstr(L"ROOT\\CIMV2"), NULL,
                                  NULL, 0, NULL, 0, 0,
                                  wmi_services_r.GetAddressOf());
  if (FAILED(hr))
    return false;

  if (set_blanket) {
    hr = ::CoSetProxyBlanket(wmi_services_r.Get(), RPC_C_AUTHN_WINNT,
                             RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
      return false;
  }

  *wmi_services = wmi_services_r.Detach();
  return true;
}

bool WMI::CreateClassMethodObject(IWbemServices* wmi_services,
                                  const std::wstring& class_name,
                                  const std::wstring& method_name,
                                  IWbemClassObject** class_instance) {
  // We attempt to instantiate a COM object that represents a WMI object plus
  // a method rolled into one entity.
  base::win::ScopedBstr b_class_name(class_name.c_str());
  base::win::ScopedBstr b_method_name(method_name.c_str());
  Microsoft::WRL::ComPtr<IWbemClassObject> class_object;
  HRESULT hr;
  hr = wmi_services->GetObject(b_class_name, 0, NULL,
                               class_object.GetAddressOf(), NULL);
  if (FAILED(hr))
    return false;

  Microsoft::WRL::ComPtr<IWbemClassObject> params_def;
  hr = class_object->GetMethod(b_method_name, 0, params_def.GetAddressOf(),
                               NULL);
  if (FAILED(hr))
    return false;

  if (NULL == params_def.Get()) {
    // You hit this special case if the WMI class is not a CIM class. MSDN
    // sometimes tells you this. Welcome to WMI hell.
    return false;
  }

  hr = params_def->SpawnInstance(0, class_instance);
  return(SUCCEEDED(hr));
}

bool SetParameter(IWbemClassObject* class_method,
                  const std::wstring& parameter_name, VARIANT* parameter) {
  HRESULT hr = class_method->Put(parameter_name.c_str(), 0, parameter, 0);
  return SUCCEEDED(hr);
}


// The code in Launch() basically calls the Create Method of the Win32_Process
// CIM class is documented here:
// http://msdn2.microsoft.com/en-us/library/aa389388(VS.85).aspx
// NOTE: The documentation for the Create method suggests that the ProcessId
// parameter and return value are of type uint32_t, but when we call the method
// the values in the returned out_params, are VT_I4, which is int32_t.

bool WMIProcess::Launch(const std::wstring& command_line, int* process_id) {
  Microsoft::WRL::ComPtr<IWbemServices> wmi_local;
  if (!WMI::CreateLocalConnection(true, wmi_local.GetAddressOf()))
    return false;

  const wchar_t class_name[] = L"Win32_Process";
  const wchar_t method_name[] = L"Create";
  Microsoft::WRL::ComPtr<IWbemClassObject> process_create;
  if (!WMI::CreateClassMethodObject(wmi_local.Get(), class_name, method_name,
                                    process_create.GetAddressOf()))
    return false;

  ScopedVariant b_command_line(command_line.c_str());

  if (!SetParameter(process_create.Get(), L"CommandLine",
                    b_command_line.AsInput()))
    return false;

  Microsoft::WRL::ComPtr<IWbemClassObject> out_params;
  HRESULT hr = wmi_local->ExecMethod(
      base::win::ScopedBstr(class_name), base::win::ScopedBstr(method_name), 0,
      NULL, process_create.Get(), out_params.GetAddressOf(), NULL);
  if (FAILED(hr))
    return false;

  // We're only expecting int32_t or uint32_t values, so no need for
  // ScopedVariant.
  VARIANT ret_value = {{{VT_EMPTY}}};
  hr = out_params->Get(L"ReturnValue", 0, &ret_value, NULL, 0);
  if (FAILED(hr) || 0 != V_I4(&ret_value))
    return false;

  VARIANT pid = {{{VT_EMPTY}}};
  hr = out_params->Get(L"ProcessId", 0, &pid, NULL, 0);
  if (FAILED(hr) || 0 == V_I4(&pid))
    return false;

  if (process_id)
    *process_id = V_I4(&pid);

  return true;
}

base::string16 WMIComputerSystem::GetModel() {
  Microsoft::WRL::ComPtr<IWbemServices> services;
  if (!WMI::CreateLocalConnection(true, services.GetAddressOf()))
    return base::string16();

  base::win::ScopedBstr query_language(L"WQL");
  base::win::ScopedBstr query(L"SELECT * FROM Win32_ComputerSystem");
  Microsoft::WRL::ComPtr<IEnumWbemClassObject> enumerator;
  HRESULT hr =
      services->ExecQuery(query_language, query,
                          WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                          NULL, enumerator.GetAddressOf());
  if (FAILED(hr) || !enumerator.Get())
    return base::string16();

  Microsoft::WRL::ComPtr<IWbemClassObject> class_object;
  ULONG items_returned = 0;
  hr = enumerator->Next(WBEM_INFINITE, 1, class_object.GetAddressOf(),
                        &items_returned);
  if (!items_returned)
    return base::string16();

  base::win::ScopedVariant manufacturer;
  class_object->Get(L"Manufacturer", 0, manufacturer.Receive(), 0, 0);
  base::win::ScopedVariant model;
  class_object->Get(L"Model", 0, model.Receive(), 0, 0);

  base::string16 model_string;
  if (manufacturer.type() == VT_BSTR) {
    model_string = V_BSTR(manufacturer.ptr());
    if (model.type() == VT_BSTR)
      model_string += L" ";
  }
  if (model.type() == VT_BSTR)
    model_string += V_BSTR(model.ptr());

  return model_string;
}

}  // namespace installer
