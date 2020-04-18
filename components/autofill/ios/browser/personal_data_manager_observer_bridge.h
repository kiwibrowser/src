// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_PERSONAL_DATA_MANAGER_OBSERVER_BRIDGE_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_PERSONAL_DATA_MANAGER_OBSERVER_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"
#include "components/autofill/core/browser/personal_data_manager_observer.h"

// PersonalDataManagerObserverBridgeDelegate is used by PersonalDataManager to
// informs its client implemented in Objective-C when it has finished loading
// personal data from the web database.
@protocol PersonalDataManagerObserverBridgeDelegate<NSObject>

// Called when the PersonalDataManager changed in some way.
- (void)onPersonalDataChanged;

@optional

// Called when there is insufficient data to fill a form.
- (void)onInsufficientFormData;

@end

namespace autofill {

// PersonalDataManagerObserverBridge forwards PersonalDataManager notification
// to an Objective-C delegate.
class PersonalDataManagerObserverBridge : public PersonalDataManagerObserver {
 public:
  explicit PersonalDataManagerObserverBridge(
      id<PersonalDataManagerObserverBridgeDelegate> delegate);
  ~PersonalDataManagerObserverBridge() override;

  // PersonalDataManagerObserver implementation.
  void OnPersonalDataChanged() override;
  void OnInsufficientFormData() override;

 private:
  __unsafe_unretained id<PersonalDataManagerObserverBridgeDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(PersonalDataManagerObserverBridge);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_PERSONAL_DATA_MANAGER_OBSERVER_BRIDGE_H_
