// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/recent_tabs/views/session_section_header_view.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/views/disclosure_view.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/l10n/time_format.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kDesiredHeight = 72;

// The UI displays relative time for up to this number of hours and then
// switches to absolute values.
const int kRelativeTimeMaxHours = 4;

}  // namespace

@interface SessionSectionHeaderView () {
  UIImageView* _deviceIcon;
  UILabel* _nameLabel;
  UILabel* _timeLabel;
  DisclosureView* _disclosureView;
}

// Returns a relative string (e.g. 15 mins ago) if the time passed in is within
// the last 4 hours. Returns the full formatted time in short style otherwise.
- (NSString*)relativeTimeStringForTime:(base::Time)time;

@end

@implementation SessionSectionHeaderView

- (instancetype)initWithFrame:(CGRect)aRect sectionIsCollapsed:(BOOL)collapsed {
  self = [super initWithFrame:aRect];
  if (self) {
    _deviceIcon = [[UIImageView alloc] initWithImage:nil];
    [_deviceIcon setTranslatesAutoresizingMaskIntoConstraints:NO];

    _nameLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    [_nameLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_nameLabel setHighlightedTextColor:[_nameLabel textColor]];
    [_nameLabel setTextAlignment:NSTextAlignmentNatural];
    [_nameLabel setFont:[[MDCTypography fontLoader] regularFontOfSize:16]];
    [_nameLabel setBackgroundColor:[UIColor whiteColor]];
    [_nameLabel
        setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];

    _timeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    [_timeLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_timeLabel setTextAlignment:NSTextAlignmentNatural];
    [_timeLabel setFont:[MDCTypography captionFont]];
    [_timeLabel setHighlightedTextColor:[_timeLabel textColor]];
    [_timeLabel setBackgroundColor:[UIColor whiteColor]];

    _disclosureView = [[DisclosureView alloc] init];
    [_disclosureView setTranslatesAutoresizingMaskIntoConstraints:NO];

    [self addSubview:_deviceIcon];
    [self addSubview:_nameLabel];
    [self addSubview:_timeLabel];
    [self addSubview:_disclosureView];

    NSDictionary* viewsDictionary = @{
      @"deviceIcon" : _deviceIcon,
      @"nameLabel" : _nameLabel,
      @"timeLabel" : _timeLabel,
      @"disclosureView" : _disclosureView,
    };

    NSArray* constraints = @[
      @"H:|-16-[deviceIcon]-16-[nameLabel]-(>=16)-[disclosureView]-16-|",
      @"V:|-16-[nameLabel]-5-[timeLabel]-16-|",
      @"H:[deviceIcon]-16-[timeLabel]",
    ];
    ApplyVisualConstraints(constraints, viewsDictionary);
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:_disclosureView
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:_deviceIcon
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:_nameLabel
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self setSectionIsCollapsed:collapsed animated:NO];
  }
  // TODO(jif): Add timer that refreshes the time label.
  return self;
}

- (void)updateWithSession:
    (synced_sessions::DistantSession const*)distantSession {
  NSString* imageName = nil;
  switch (distantSession->device_type) {
    case sync_pb::SyncEnums::TYPE_PHONE:
      imageName = @"ntp_opentabs_phone";
      break;
    case sync_pb::SyncEnums::TYPE_TABLET:
      imageName = @"ntp_opentabs_tablet";
      break;
    default:
      imageName = @"ntp_opentabs_laptop";
      break;
  }
  [_deviceIcon
      setImage:[[UIImage imageNamed:imageName]
                   imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]];
  [_nameLabel setText:base::SysUTF8ToNSString(distantSession->name)];

  NSDate* lastUsedDate = [NSDate
      dateWithTimeIntervalSince1970:distantSession->modified_time.ToTimeT()];
  NSString* timeString =
      [self relativeTimeStringForTime:distantSession->modified_time];
  NSString* dateString =
      [NSDateFormatter localizedStringFromDate:lastUsedDate
                                     dateStyle:NSDateFormatterShortStyle
                                     timeStyle:NSDateFormatterNoStyle];
  NSString* timeDateString =
      [NSString stringWithFormat:@"%@ %@", timeString, dateString];

  [_timeLabel setText:l10n_util::GetNSStringF(
                          IDS_IOS_OPEN_TABS_LAST_USED,
                          base::SysNSStringToUTF16(timeDateString))];
}

- (NSString*)relativeTimeStringForTime:(base::Time)time {
  base::TimeDelta last_used_delta;
  if (base::Time::Now() > time)
    last_used_delta = base::Time::Now() - time;
  if (last_used_delta.ToInternalValue() < base::Time::kMicrosecondsPerMinute)
    return l10n_util::GetNSString(IDS_IOS_OPEN_TABS_RECENTLY_SYNCED);
  if (last_used_delta.InHours() < kRelativeTimeMaxHours) {
    return base::SysUTF16ToNSString(
        ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_ELAPSED,
                               ui::TimeFormat::LENGTH_SHORT, last_used_delta));
  }
  NSDate* date = [NSDate dateWithTimeIntervalSince1970:time.ToTimeT()];
  return [NSDateFormatter localizedStringFromDate:date
                                        dateStyle:NSDateFormatterNoStyle
                                        timeStyle:NSDateFormatterShortStyle];
}

+ (CGFloat)desiredHeightInUITableViewCell {
  return kDesiredHeight;
}

#pragma mark - HeaderOfCollapsableSectionProtocol

- (void)setSectionIsCollapsed:(BOOL)collapsed animated:(BOOL)animated {
  [_disclosureView setTransformWhenCollapsed:collapsed animated:animated];
  UIColor* tintColor = (collapsed ? recent_tabs::GetIconColorGray()
                                  : recent_tabs::GetIconColorBlue());
  [self setTintColor:tintColor];
  UIColor* textColor = (collapsed ? recent_tabs::GetTextColorGray()
                                  : recent_tabs::GetTextColorBlue());
  [_nameLabel setTextColor:textColor];
  UIColor* subtitleColor = (collapsed ? recent_tabs::GetSubtitleColorGray()
                                      : recent_tabs::GetSubtitleColorBlue());
  [_timeLabel setTextColor:subtitleColor];
}

@end
