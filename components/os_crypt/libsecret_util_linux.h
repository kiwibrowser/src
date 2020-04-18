// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_LIBSECRET_UTIL_LINUX_H_
#define COMPONENTS_OS_CRYPT_LIBSECRET_UTIL_LINUX_H_

#include <libsecret/secret.h>

#include <list>
#include <string>

#include "base/macros.h"

// Utility for dynamically loading libsecret.
class LibsecretLoader {
 public:
  static decltype(&::secret_item_get_attributes) secret_item_get_attributes;
  static decltype(&::secret_item_get_secret) secret_item_get_secret;
  static decltype(&::secret_item_load_secret_sync) secret_item_load_secret_sync;
  static decltype(&::secret_password_clear_sync) secret_password_clear_sync;
  static decltype(&::secret_password_store_sync) secret_password_store_sync;
  static decltype(&::secret_service_search_sync) secret_service_search_sync;
  static decltype(&::secret_value_get_text) secret_value_get_text;
  static decltype(&::secret_value_unref) secret_value_unref;

  // Wrapper for secret_service_search_sync that prevents common leaks. See
  // https://crbug.com/393395.
  class SearchHelper {
   public:
    SearchHelper();
    ~SearchHelper();

    // Search must be called exactly once for success() and results() to be
    // populated.
    void Search(const SecretSchema* schema, GHashTable* attrs, int flags);

    bool success() { return !error_; }

    GList* results() { return results_; }
    GError* error() { return error_; }

   private:
    // |results_| and |error_| are C-style objects owned by this instance.
    GList* results_ = nullptr;
    GError* error_ = nullptr;
    DISALLOW_COPY_AND_ASSIGN(SearchHelper);
  };

  // Loads the libsecret library and checks that it responds to queries.
  // Returns false if either step fails.
  // Repeated calls check the responsiveness every time, but do not load the
  // the library again if already successful.
  static bool EnsureLibsecretLoaded();

  // Ensure that the default keyring is accessible. This won't prevent the user
  // from locking their keyring while Chrome is running.
  static void EnsureKeyringUnlocked();

 protected:
  static bool libsecret_loaded_;

 private:
  struct FunctionInfo {
    const char* name;
    void** pointer;
  };

  static const FunctionInfo kFunctions[];

  // Load the libsecret binaries. Returns true on success.
  // If successful, the result is cached and the function can be safely called
  // multiple times.
  // Checking |LibsecretIsAvailable| is necessary after this to verify that the
  // service responds to queries.
  static bool LoadLibsecret();

  // True if the libsecret binaries have been loaded and the library responds
  // to queries.
  static bool LibsecretIsAvailable();

  DISALLOW_IMPLICIT_CONSTRUCTORS(LibsecretLoader);
};

class LibsecretAttributesBuilder {
 public:
  LibsecretAttributesBuilder();
  ~LibsecretAttributesBuilder();

  void Append(const std::string& name, const std::string& value);

  void Append(const std::string& name, int64_t value);

  // GHashTable, its keys and values returned from Get() are destroyed in
  // |LibsecretAttributesBuilder| destructor.
  GHashTable* Get() { return attrs_; }

 private:
  // |name_values_| is a storage for strings referenced in |attrs_|.
  // TODO(crbug.com/607950): Make implementation more robust by not depending on
  // the implementation details of containers. External objects keep references
  // to the objects stored in this container. Using a vector here will fail the
  // ASan tests, because it may move the objects and break the references.
  std::list<std::string> name_values_;
  GHashTable* attrs_;

  DISALLOW_COPY_AND_ASSIGN(LibsecretAttributesBuilder);
};

#endif  // COMPONENTS_OS_CRYPT_LIBSECRET_UTIL_LINUX_H_
