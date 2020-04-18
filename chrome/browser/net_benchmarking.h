// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_BENCHMARKING_H_
#define CHROME_BROWSER_NET_BENCHMARKING_H_

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/net_benchmarking.mojom.h"

namespace net {
class URLRequestContextGetter;
}

class Profile;

// This class handles Chrome-specific benchmarking IPC messages
// for the renderer process.
class NetBenchmarking : public chrome::mojom::NetBenchmarking {
 public:
  NetBenchmarking(Profile* profile,
                  net::URLRequestContextGetter* request_context);
  ~NetBenchmarking() override;

  static void Create(Profile* profile,
                     net::URLRequestContextGetter* request_context,
                     chrome::mojom::NetBenchmarkingRequest request);
  static bool CheckBenchmarkingEnabled();

 private:
  // chrome:mojom:NetBenchmarking.
  void CloseCurrentConnections(
      const CloseCurrentConnectionsCallback& callback) override;
  void ClearCache(const ClearCacheCallback& callback) override;
  void ClearHostResolverCache(
      const ClearHostResolverCacheCallback& callback) override;
  void ClearPredictorCache(
      const ClearPredictorCacheCallback& callback) override;

  // The Profile associated with our renderer process.  This should only be
  // accessed on the UI thread!
  // TODO(623967): Store the Predictor* here instead of the Profile.
  Profile* profile_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  DISALLOW_COPY_AND_ASSIGN(NetBenchmarking);
};

#endif  // CHROME_BROWSER_NET_BENCHMARKING_H_
