// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <unordered_map>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/os_crypt/key_storage_libsecret.h"
#include "components/os_crypt/libsecret_util_linux.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const SecretSchema kKeystoreSchemaV1 = {
    "chrome_libsecret_os_crypt_password",
    SECRET_SCHEMA_NONE,
    {
        {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
    }};

const SecretSchema kKeystoreSchemaV2 = {
    "chrome_libsecret_os_crypt_password_v2",
    SECRET_SCHEMA_DONT_MATCH_NAME,
    {
        {"application", SECRET_SCHEMA_ATTRIBUTE_STRING},
        {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
    }};

// This test mocks C-functions used by Libsecret. In order to present a
// consistent world view, we need a single backing instance that contains all
// the relevant data.
class MockPasswordStore {
  // The functions that interact with the password store expect to return
  // gobjects. These C-style objects are hard to work with. Rather than finagle
  // with the type system, we always use objects with type G_TYPE_OBJECT. We
  // then keep a local map from G_TYPE_OBJECT to std::string, and all relevant
  // translation just look up entries in this map.
 public:
  void Reset() {
    mapping_.clear();
    ClearV1Password();
    ClearV2Password();
    for (GObject* o : objects_returned_to_caller_) {
      ASSERT_EQ(o->ref_count, 1u);
      g_object_unref(o);
    }
    objects_returned_to_caller_.clear();
  }

  void ClearV1Password() {
    if (v1_password_) {
      ASSERT_EQ(v1_password_->ref_count, 1u);
      g_object_unref(v1_password_);
      v1_password_ = nullptr;
    }
  }
  void ClearV2Password() {
    if (v2_password_) {
      ASSERT_EQ(v2_password_->ref_count, 1u);
      g_object_unref(v2_password_);
      v2_password_ = nullptr;
    }
  }

  void SetV1Password(const std::string& password) {
    ASSERT_FALSE(v1_password_);
    v1_password_ = static_cast<GObject*>(g_object_new(G_TYPE_OBJECT, nullptr));
    mapping_[v1_password_] = password;
  }

  void SetV2Password(const std::string& password) {
    ASSERT_FALSE(v2_password_);
    v2_password_ = static_cast<GObject*>(g_object_new(G_TYPE_OBJECT, nullptr));
    mapping_[v2_password_] = password;
  }

  GObject* MakeTempObject(const std::string& value) {
    GObject* temp = static_cast<GObject*>(g_object_new(G_TYPE_OBJECT, nullptr));
    // The returned object has a ref count of 2. This way, after the client
    // deletes the object, it isn't destroyed, and we can check that all these
    // objects have ref count of 1 at the end of the test.
    g_object_ref(temp);
    objects_returned_to_caller_.push_back(temp);
    mapping_[temp] = value;
    return temp;
  }

  const gchar* GetString(void* opaque_id) {
    return mapping_[static_cast<GObject*>(opaque_id)].c_str();
  }

  GObject* v1_password() { return v1_password_; }
  GObject* v2_password() { return v2_password_; }

  std::unordered_map<GObject*, std::string> mapping_;
  std::vector<GObject*> objects_returned_to_caller_;
  GObject* v1_password_ = nullptr;
  GObject* v2_password_ = nullptr;
};
base::LazyInstance<MockPasswordStore>::Leaky g_password_store =
    LAZY_INSTANCE_INITIALIZER;

// Replaces some of LibsecretLoader's methods with mocked ones.
class MockLibsecretLoader : public LibsecretLoader {
 public:
  // Sets up the minimum mock implementation necessary for OSCrypt to work
  // with Libsecret. Also resets the state to mock a clean database.
  static bool ResetForOSCrypt();

  // Sets OSCrypt's password in the libsecret mock to a specific value
  static void SetOSCryptPassword(const char*);

  // Releases memory and restores LibsecretLoader to an uninitialized state.
  static void TearDown();

 private:
  // These methods are used to redirect calls through LibsecretLoader
  static const gchar* mock_secret_value_get_text(SecretValue* value);

  static gboolean mock_secret_password_store_sync(const SecretSchema* schema,
                                                  const gchar* collection,
                                                  const gchar* label,
                                                  const gchar* password,
                                                  GCancellable* cancellable,
                                                  GError** error,
                                                  ...);

  static void mock_secret_value_unref(gpointer value);

  static GList* mock_secret_service_search_sync(SecretService* service,
                                                const SecretSchema* schema,
                                                GHashTable* attributes,
                                                SecretSearchFlags flags,
                                                GCancellable* cancellable,
                                                GError** error);

  static gboolean mock_secret_password_clear_sync(const SecretSchema* schema,
                                                  GCancellable* cancellable,
                                                  GError** error,
                                                  ...);

  static SecretValue* mock_secret_item_get_secret(SecretItem* item);
};

const gchar* MockLibsecretLoader::mock_secret_value_get_text(
    SecretValue* value) {
  return g_password_store.Pointer()->GetString(value);
}

// static
gboolean MockLibsecretLoader::mock_secret_password_store_sync(
    const SecretSchema* schema,
    const gchar* collection,
    const gchar* label,
    const gchar* password,
    GCancellable* cancellable,
    GError** error,
    ...) {
  // TODO(crbug.com/660005) We don't read the dummy we store to unlock keyring.
  if (strcmp("_chrome_dummy_schema_for_unlocking", schema->name) == 0)
    return true;

  EXPECT_STREQ(kKeystoreSchemaV2.name, schema->name);
  g_password_store.Pointer()->SetV2Password(password);
  return true;
}

// static
void MockLibsecretLoader::mock_secret_value_unref(gpointer value) {
  g_object_unref(value);
}

// static
GList* MockLibsecretLoader::mock_secret_service_search_sync(
    SecretService* service,
    const SecretSchema* schema,
    GHashTable* attributes,
    SecretSearchFlags flags,
    GCancellable* cancellable,
    GError** error) {
  bool is_known_schema = strcmp(schema->name, kKeystoreSchemaV2.name) == 0 ||
                         strcmp(schema->name, kKeystoreSchemaV1.name) == 0;
  EXPECT_TRUE(is_known_schema);

  EXPECT_TRUE(flags & SECRET_SEARCH_UNLOCK);
  EXPECT_TRUE(flags & SECRET_SEARCH_LOAD_SECRETS);

  GObject* item = nullptr;
  MockPasswordStore* store = g_password_store.Pointer();
  if (strcmp(schema->name, kKeystoreSchemaV2.name) == 0) {
    GObject* password = store->v2_password();
    if (password)
      item = store->MakeTempObject(store->GetString(password));
  } else if (strcmp(schema->name, kKeystoreSchemaV1.name) == 0) {
    GObject* password = store->v1_password();
    if (password)
      item = store->MakeTempObject(store->GetString(password));
  }

  if (!item) {
    return nullptr;
  }

  GList* result = nullptr;
  result = g_list_append(result, item);
  g_clear_error(error);
  return result;
}

// static
gboolean MockLibsecretLoader::mock_secret_password_clear_sync(
    const SecretSchema* schema,
    GCancellable* cancellable,
    GError** error,
    ...) {
  // We would only delete entries in the deprecated schema.
  EXPECT_STREQ(kKeystoreSchemaV1.name, schema->name);
  g_password_store.Pointer()->ClearV1Password();
  return true;
}

// static
SecretValue* MockLibsecretLoader::mock_secret_item_get_secret(
    SecretItem* item) {
  // Add a ref to make sure that the caller unrefs with secret_value_unref.
  g_object_ref(item);
  return reinterpret_cast<SecretValue*>(item);
}

// static
bool MockLibsecretLoader::ResetForOSCrypt() {
  // Methods used by KeyStorageLibsecret
  secret_password_store_sync =
      &MockLibsecretLoader::mock_secret_password_store_sync;
  secret_value_get_text = &MockLibsecretLoader::mock_secret_value_get_text;
  secret_value_unref = &MockLibsecretLoader::mock_secret_value_unref;
  secret_service_search_sync =
      &MockLibsecretLoader::mock_secret_service_search_sync;
  secret_item_get_secret = &MockLibsecretLoader::mock_secret_item_get_secret;
  // Used by Migrate()
  secret_password_clear_sync =
      &MockLibsecretLoader::mock_secret_password_clear_sync;

  g_password_store.Pointer()->Reset();
  libsecret_loaded_ = true;

  return true;
}

// static
void MockLibsecretLoader::TearDown() {
  g_password_store.Pointer()->Reset();
  libsecret_loaded_ =
      false;  // Function pointers will be restored when loading.
}

class LibsecretTest : public testing::Test {
 public:
  LibsecretTest() = default;
  ~LibsecretTest() override = default;

  void SetUp() override { MockLibsecretLoader::ResetForOSCrypt(); }

  void TearDown() override { MockLibsecretLoader::TearDown(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(LibsecretTest);
};

TEST_F(LibsecretTest, LibsecretRepeats) {
  KeyStorageLibsecret libsecret;
  MockLibsecretLoader::ResetForOSCrypt();
  g_password_store.Pointer()->SetV2Password("initial password");
  std::string password = libsecret.GetKey();
  EXPECT_FALSE(password.empty());
  std::string password_repeat = libsecret.GetKey();
  EXPECT_EQ(password, password_repeat);
}

TEST_F(LibsecretTest, LibsecretCreatesRandomised) {
  KeyStorageLibsecret libsecret;
  MockLibsecretLoader::ResetForOSCrypt();
  std::string password = libsecret.GetKey();
  MockLibsecretLoader::ResetForOSCrypt();
  std::string password_new = libsecret.GetKey();
  EXPECT_NE(password, password_new);
}

TEST_F(LibsecretTest, LibsecretMigratesFromSchemaV1ToV2) {
  KeyStorageLibsecret libsecret;
  MockLibsecretLoader::ResetForOSCrypt();
  g_password_store.Pointer()->SetV1Password("swallow");
  g_password_store.Pointer()->ClearV2Password();
  std::string password = libsecret.GetKey();
  EXPECT_EQ("swallow", password);
}

}  // namespace
