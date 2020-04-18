// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/net/http_server_properties_manager_factory.h"

#include <memory>

#include "base/values.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/json_pref_store.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/web/public/web_thread.h"
#include "net/http/http_server_properties_manager.h"

namespace {

class PrefServiceAdapter
    : public net::HttpServerPropertiesManager::PrefDelegate,
      public PrefStore::Observer {
 public:
  explicit PrefServiceAdapter(scoped_refptr<JsonPrefStore> pref_store)
      : pref_store_(std::move(pref_store)),
        path_(prefs::kHttpServerProperties) {
    pref_store_->AddObserver(this);
  }

  ~PrefServiceAdapter() override { pref_store_->RemoveObserver(this); }

  // PrefDelegate implementation.
  const base::DictionaryValue* GetServerProperties() const override {
    const base::Value* value;
    if (pref_store_->GetValue(path_, &value)) {
      const base::DictionaryValue* dict;
      if (value->GetAsDictionary(&dict))
        return dict;
    }

    return nullptr;
  }
  void SetServerProperties(const base::DictionaryValue& value,
                           base::OnceClosure callback) override {
    pref_store_->SetValue(path_, value.CreateDeepCopy(),
                          WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
    if (callback)
      pref_store_->CommitPendingWrite(std::move(callback));
  }
  void StartListeningForUpdates(
      const base::RepeatingClosure& callback) override {
    on_changed_callback_ = callback;
  }

  // PrefStore::Observer implementation.
  void OnPrefValueChanged(const std::string& key) override {
    if (key == path_ && on_changed_callback_)
      on_changed_callback_.Run();
  }
  void OnInitializationCompleted(bool succeeded) override {
    if (succeeded && on_changed_callback_)
      on_changed_callback_.Run();
  }

 private:
  scoped_refptr<JsonPrefStore> pref_store_;
  const std::string path_;

  base::Closure on_changed_callback_;

  DISALLOW_COPY_AND_ASSIGN(PrefServiceAdapter);
};

}  // namespace

// static
std::unique_ptr<net::HttpServerPropertiesManager>
HttpServerPropertiesManagerFactory::CreateManager(
    scoped_refptr<JsonPrefStore> pref_store,
    net::NetLog* net_log) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  return std::make_unique<net::HttpServerPropertiesManager>(
      std::make_unique<PrefServiceAdapter>(std::move(pref_store)), net_log);
}
