// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_ACTIVATION_HANDLER_H_
#define CHROMEOS_NETWORK_NETWORK_ACTIVATION_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/network/network_handler_callbacks.h"

namespace chromeos {

// The NetworkActivationHandler class allows making service specific
// calls required for activation on mobile networks.
class CHROMEOS_EXPORT NetworkActivationHandler
    : public base::SupportsWeakPtr<NetworkActivationHandler> {
 public:
  // Constants for |error_name| from |error_callback|.
  // TODO(gauravsh): Merge various error constants from Network*Handlers into
  // a single place. crbug.com/272554
  static const char kErrorNotFound[];
  static const char kErrorShillError[];

  virtual ~NetworkActivationHandler();

  // ActivateNetwork() will start an asynchronous activation attempt.
  // |carrier| may be empty or may specify a carrier to activate.
  // On success, |success_callback| will be called.
  // On failure, |error_callback| will be called with |error_name| one of:
  //  kErrorNotFound if no network matching |service_path| is found.
  //  kErrorShillError if a DBus or Shill error occurred.
  void Activate(const std::string& service_path,
                const std::string& carrier,
                const base::Closure& success_callback,
                const network_handler::ErrorCallback& error_callback);

  // CompleteActivation() will start an asynchronous activation completion
  // attempt.
  // On success, |success_callback| will be called.
  // On failure, |error_callback| will be called with |error_name| one of:
  //  kErrorNotFound if no network matching |service_path| is found.
  //  kErrorShillError if a DBus or Shill error occurred.
  void CompleteActivation(const std::string& service_path,
                          const base::Closure& success_callback,
                          const network_handler::ErrorCallback& error_callback);

 private:
  friend class NetworkHandler;

  NetworkActivationHandler();

  // Calls Shill.Service.ActivateCellularModem asynchronously.
  void CallShillActivate(const std::string& service_path,
                         const std::string& carrier,
                         const base::Closure& success_callback,
                         const network_handler::ErrorCallback& error_callback);

  // Calls Shill.Service.CompleteCellularActivation asynchronously.
  void CallShillCompleteActivation(
      const std::string& service_path,
      const base::Closure& success_callback,
      const network_handler::ErrorCallback& error_callback);

  // Handle success from Shill.Service.ActivateCellularModem or
  // Shill.Service.CompleteCellularActivation.
  void HandleShillSuccess(const std::string& service_path,
                          const base::Closure& success_callback);

  DISALLOW_COPY_AND_ASSIGN(NetworkActivationHandler);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_ACTIVATION_HANDLER_H_
