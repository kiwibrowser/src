// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_NETWORK_CONNECTION_TRACKER_H_
#define CONTENT_PUBLIC_COMMON_NETWORK_CONNECTION_TRACKER_H_

#include <list>
#include <memory>

#include "base/atomicops.h"
#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/observer_list_threadsafe.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/mojom/network_change_manager.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace content {

// This class subscribes to network change events from
// network::mojom::NetworkChangeManager and propogates these notifications to
// its NetworkConnectionObservers registered through
// AddObserver()/RemoveObserver().
class CONTENT_EXPORT NetworkConnectionTracker
    : public network::mojom::NetworkChangeManagerClient {
 public:
  typedef base::OnceCallback<void(network::mojom::ConnectionType)>
      ConnectionTypeCallback;

  class CONTENT_EXPORT NetworkConnectionObserver {
   public:
    // Please refer to NetworkChangeManagerClient::OnNetworkChanged for when
    // this method is invoked.
    virtual void OnConnectionChanged(network::mojom::ConnectionType type) = 0;

   protected:
    NetworkConnectionObserver() {}
    virtual ~NetworkConnectionObserver() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(NetworkConnectionObserver);
  };

  NetworkConnectionTracker();

  ~NetworkConnectionTracker() override;

  // Starts listening for connection notifications from |network_service|.
  // Observers may be added and GetConnectionType called, but no network
  // information will be provided until this method is called. For unit tests,
  // this class can be subclassed, and OnInitialConnectionType /
  // OnNetworkChanged may be called directly, instead of providing a
  // NetworkService.
  void Initialize(network::mojom::NetworkService* network_service);

  // If connection type can be retrieved synchronously, returns true and |type|
  // will contain the current connection type; Otherwise, returns false, in
  // which case, |callback| will be called on the calling thread when connection
  // type is ready. This method is thread safe. Please also refer to
  // net::NetworkChangeNotifier::GetConnectionType() for documentation.
  virtual bool GetConnectionType(network::mojom::ConnectionType* type,
                                 ConnectionTypeCallback callback);

  // Returns true if |type| is a cellular connection.
  // Returns false if |type| is CONNECTION_UNKNOWN, and thus, depending on the
  // implementation of GetConnectionType(), it is possible that
  // IsConnectionCellular(GetConnectionType()) returns false even if the
  // current connection is cellular.
  static bool IsConnectionCellular(network::mojom::ConnectionType type);

  // Registers |observer| to receive notifications of network changes. The
  // thread on which this is called is the thread on which |observer| will be
  // called back with notifications.
  void AddNetworkConnectionObserver(NetworkConnectionObserver* observer);

  // Unregisters |observer| from receiving notifications.  This must be called
  // on the same thread on which AddObserver() was called.
  // All observers must be unregistred before |this| is destroyed.
  void RemoveNetworkConnectionObserver(NetworkConnectionObserver* observer);

 protected:
  // NetworkChangeManagerClient implementation. Protected for testing.
  void OnInitialConnectionType(network::mojom::ConnectionType type) override;
  void OnNetworkChanged(network::mojom::ConnectionType type) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(NetworkGetConnectionTest,
                           GetConnectionTypeOnDifferentThread);
  // The task runner that |this| lives on.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // Protect access to |connection_type_callbacks_|.
  base::Lock lock_;

  // Saves user callback if GetConnectionType() cannot complete synchronously.
  std::list<ConnectionTypeCallback> connection_type_callbacks_;

  // |connection_type_| is set on one thread but read on many threads.
  // The default value is -1 before OnInitialConnectionType().
  base::subtle::Atomic32 connection_type_;

  const scoped_refptr<base::ObserverListThreadSafe<NetworkConnectionObserver>>
      network_change_observer_list_;

  mojo::Binding<network::mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConnectionTracker);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_NETWORK_CONNECTION_TRACKER_H_
