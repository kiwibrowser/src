// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/syncable/read_node.h"
#include "components/sync/syncable/read_transaction.h"

using base::FeatureList;
using syncer::ModelType;
using syncer::ModelTypeSet;
using syncer::ModelTypeToString;
using syncer::ProxyTypes;
using syncer::SyncPrefs;
using syncer::UserSelectableTypes;
using syncer::UserShare;

namespace {

bool DoesTopLevelNodeExist(UserShare* user_share, ModelType type) {
  syncer::ReadTransaction trans(FROM_HERE, user_share);
  syncer::ReadNode node(&trans);
  return node.InitTypeRoot(type) == syncer::BaseNode::INIT_OK;
}

void VerifyExistence(UserShare* user_share,
                     bool should_root_node_exist,
                     ModelType type) {
  EXPECT_EQ(should_root_node_exist, DoesTopLevelNodeExist(user_share, type))
      << ModelTypeToString(type);
}

void VerifyExistence(UserShare* user_share,
                     bool should_root_node_exist,
                     ModelType grouped,
                     ModelType selectable) {
  EXPECT_EQ(should_root_node_exist, DoesTopLevelNodeExist(user_share, grouped))
      << ModelTypeToString(selectable) << "->" << ModelTypeToString(grouped);
}

bool IsUnready(const syncer::DataTypeStatusTable& data_type_status_table,
               ModelType type) {
  return data_type_status_table.GetUnreadyErrorTypes().Has(type);
}

// The current approach this test class takes is to examine the Directory and
// check for root nodes to see if a type is currently enabled. While this works
// for things in the directory, it does not work for USS types. USS does not
// have any general data access mechanism, at least yet. Until that exists,
// simply omit types that may be USS from these cases.
ModelTypeSet UnifiedSyncServiceTypes() {
  ModelTypeSet set;
  set.Put(syncer::AUTOFILL);
  set.Put(syncer::DEVICE_INFO);
  set.Put(syncer::TYPED_URLS);
  // PRINTERS was the first USS type, and should precede all other USS types.
  // All new types should be USS. This logic is fragile to reordering ModelType.
  for (int typeInt = syncer::PRINTERS; typeInt < syncer::FIRST_PROXY_TYPE;
       typeInt++) {
    set.Put(static_cast<ModelType>(typeInt));
  }
  return set;
}

// Some types show up in multiple groups. This means that there are at least two
// user selectable groups that will cause these types to become enabled. This
// affects our tests because we cannot assume that before enabling a multi type
// it will be disabled, because the other selectable type(s) could already be
// enabling it. And vice versa for disabling.
ModelTypeSet MultiGroupTypes(const SyncPrefs& sync_prefs,
                             const ModelTypeSet& registered_types) {
  const ModelTypeSet selectable_types = UserSelectableTypes();
  ModelTypeSet seen;
  ModelTypeSet multi;
  for (ModelTypeSet::Iterator si = selectable_types.First(); si.Good();
       si.Inc()) {
    const ModelTypeSet grouped_types =
        sync_prefs.ResolvePrefGroups(registered_types, ModelTypeSet(si.Get()));
    for (ModelTypeSet::Iterator gi = grouped_types.First(); gi.Good();
         gi.Inc()) {
      if (seen.Has(gi.Get())) {
        multi.Put(gi.Get());
      } else {
        seen.Put(gi.Get());
      }
    }
  }
  return multi;
}

// This test enables and disables types and verifies the type is sufficiently
// affected by checking for existence of a root node.
class EnableDisableSingleClientTest : public SyncTest {
 public:
  EnableDisableSingleClientTest() : SyncTest(SINGLE_CLIENT) {}
  ~EnableDisableSingleClientTest() override {}

  // Don't use self-notifications as they can trigger additional sync cycles.
  bool TestUsesSelfNotifications() override { return false; }

 protected:
  void SetupTest(bool all_types_enabled) {
    ASSERT_TRUE(SetupClients());
    sync_prefs_ = std::make_unique<SyncPrefs>(GetProfile(0)->GetPrefs());
    if (all_types_enabled) {
      ASSERT_TRUE(GetClient(0)->SetupSync());
    } else {
      ASSERT_TRUE(GetClient(0)->SetupSync(ModelTypeSet()));
    }
    user_share_ = GetSyncService(0)->GetUserShare();
    data_type_status_table_ = GetSyncService(0)->data_type_status_table();

    registered_types_ = GetSyncService(0)->GetRegisteredDataTypes();
    selectable_types_ = UserSelectableTypes();
    multi_grouped_types_ = MultiGroupTypes(*sync_prefs_, registered_types_);
    registered_directory_types_ = Difference(
        Difference(registered_types_, UnifiedSyncServiceTypes()), ProxyTypes());
    registered_selectable_types_ =
        Intersection(registered_types_, UserSelectableTypes());
  }

  ModelTypeSet ResolveGroup(ModelType type) {
    return Difference(Difference(sync_prefs_->ResolvePrefGroups(
                                     registered_types_, ModelTypeSet(type)),
                                 ProxyTypes()),
                      UnifiedSyncServiceTypes());
  }

  ModelTypeSet WithoutMultiTypes(const ModelTypeSet& input) {
    return Difference(input, multi_grouped_types_);
  }

  void TearDownOnMainThread() override {
    // Has to be done before user prefs are destroyed.
    sync_prefs_.reset();
    SyncTest::TearDownOnMainThread();
  }

  std::unique_ptr<SyncPrefs> sync_prefs_;
  UserShare* user_share_;
  syncer::DataTypeStatusTable data_type_status_table_;
  ModelTypeSet registered_types_;
  ModelTypeSet selectable_types_;
  ModelTypeSet multi_grouped_types_;
  ModelTypeSet registered_directory_types_;
  ModelTypeSet registered_selectable_types_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EnableDisableSingleClientTest);
};

IN_PROC_BROWSER_TEST_F(EnableDisableSingleClientTest, EnableOneAtATime) {
  // Setup sync with no enabled types.
  SetupTest(false);

  for (ModelTypeSet::Iterator si = selectable_types_.First(); si.Good();
       si.Inc()) {
    const ModelTypeSet grouped_types = ResolveGroup(si.Get());
    const ModelTypeSet single_grouped_types = WithoutMultiTypes(grouped_types);
    for (ModelTypeSet::Iterator sgi = single_grouped_types.First(); sgi.Good();
         sgi.Inc()) {
      VerifyExistence(user_share_, false, sgi.Get(), si.Get());
    }

    EXPECT_TRUE(GetClient(0)->EnableSyncForDatatype(si.Get()));

    for (ModelTypeSet::Iterator gi = grouped_types.First(); gi.Good();
         gi.Inc()) {
      VerifyExistence(user_share_, true, gi.Get(), si.Get());
    }
  }
}

IN_PROC_BROWSER_TEST_F(EnableDisableSingleClientTest, DisableOneAtATime) {
  // Setup sync with no disabled types.
  SetupTest(true);

  // Make sure all top-level nodes exist first.
  for (ModelTypeSet::Iterator rdi = registered_directory_types_.First();
       rdi.Good(); rdi.Inc()) {
    EXPECT_TRUE(DoesTopLevelNodeExist(user_share_, rdi.Get()) ||
                IsUnready(data_type_status_table_, rdi.Get()))
        << ModelTypeToString(rdi.Get());
  }

  for (ModelTypeSet::Iterator si = selectable_types_.First(); si.Good();
       si.Inc()) {
    const ModelTypeSet grouped_types = ResolveGroup(si.Get());
    for (ModelTypeSet::Iterator gi = grouped_types.First(); gi.Good();
         gi.Inc()) {
      VerifyExistence(user_share_, true, gi.Get(), si.Get());
    }

    EXPECT_TRUE(GetClient(0)->DisableSyncForDatatype(si.Get()));

    const ModelTypeSet single_grouped_types = WithoutMultiTypes(grouped_types);
    for (ModelTypeSet::Iterator sgi = single_grouped_types.First(); sgi.Good();
         sgi.Inc()) {
      VerifyExistence(user_share_, false, sgi.Get(), si.Get());
    }
  }

  // Lastly make sure that all the multi grouped times are all gone, since we
  // did not check these after disabling inside the above loop.
  for (ModelTypeSet::Iterator mgi = multi_grouped_types_.First(); mgi.Good();
       mgi.Inc()) {
    VerifyExistence(user_share_, false, mgi.Get());
  }
}

}  // namespace
