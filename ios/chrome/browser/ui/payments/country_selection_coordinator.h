// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_COUNTRY_SELECTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_COUNTRY_SELECTION_COORDINATOR_H_

#import <UIKit/UIKit.h>
#include <vector>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/payments/payment_request_picker_view_controller.h"

@class CountrySelectionCoordinator;

// Delegate protocol for CountrySelectionCoordinator.
@protocol CountrySelectionCoordinatorDelegate<NSObject>

// Notifies the delegate that the user has selected a country.
- (void)countrySelectionCoordinator:(CountrySelectionCoordinator*)coordinator
           didSelectCountryWithCode:(NSString*)countryCode;

// Notifies the delegate that the user has chosen to return to the previous
// screen without making a selection.
- (void)countrySelectionCoordinatorDidReturn:
    (CountrySelectionCoordinator*)coordinator;

@end

// Coordinator responsible for creating and presenting the country selection
// view controller. This view controller will be presented by the view
// controller provided in the initializer.
@interface CountrySelectionCoordinator
    : ChromeCoordinator<PaymentRequestPickerViewControllerDelegate>

// A map of country codes to country names.
@property(nonatomic, strong) NSDictionary<NSString*, NSString*>* countries;

// The country code for the currently selected country, if any.
@property(nonatomic, strong) NSString* selectedCountryCode;

// The delegate to be notified when the user selects a country or returns
// without selecting one.
@property(nonatomic, weak) id<CountrySelectionCoordinatorDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_COUNTRY_SELECTION_COORDINATOR_H_
