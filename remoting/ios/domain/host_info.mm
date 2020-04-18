// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/domain/host_info.h"

#include "base/i18n/time_formatting.h"
#include "base/strings/sys_string_conversions.h"
#include "remoting/ios/facade/host_info.h"

@implementation HostInfo

@synthesize createdTime = _createdTime;
@synthesize hostId = _hostId;
@synthesize hostName = _hostName;
@synthesize hostOs = _hostOs;
@synthesize hostOsVersion = _hostOsVersion;
@synthesize hostVersion = _hostVersion;
@synthesize jabberId = _jabberId;
@synthesize kind = _kind;
@synthesize publicKey = _publicKey;
@synthesize status = _status;
@synthesize updatedTime = _updatedTime;
@synthesize offlineReason = _offlineReason;

- (instancetype)initWithRemotingHostInfo:(const remoting::HostInfo&)hostInfo {
  if (self = [super init]) {
    NSString* status;
    switch (hostInfo.status) {
      case remoting::kHostStatusOnline:
        status = @"ONLINE";
        break;
      case remoting::kHostStatusOffline:
        status = @"OFFLINE";
        break;
      default:
        NOTREACHED();
    }
    _hostId = base::SysUTF8ToNSString(hostInfo.host_id);
    _hostName = base::SysUTF8ToNSString(hostInfo.host_name);
    _hostOs = base::SysUTF8ToNSString(hostInfo.host_os);
    _hostOsVersion = base::SysUTF8ToNSString(hostInfo.host_os_version);
    _hostVersion = base::SysUTF8ToNSString(hostInfo.host_version);
    _jabberId = base::SysUTF8ToNSString(hostInfo.host_jid);
    _publicKey = base::SysUTF8ToNSString(hostInfo.public_key);
    _status = status;
    _updatedTime = base::SysUTF16ToNSString(
        base::TimeFormatShortDateAndTime(hostInfo.updated_time));
    _offlineReason = base::SysUTF8ToNSString(hostInfo.offline_reason);
  }
  return self;
}

- (bool)isOnline {
  return (self.status && [self.status isEqualToString:@"ONLINE"]);
}

- (NSComparisonResult)compare:(HostInfo*)host {
  if (self.isOnline != host.isOnline) {
    return self.isOnline ? NSOrderedAscending : NSOrderedDescending;
  } else {
    return [self.hostName localizedCaseInsensitiveCompare:_hostName];
  }
}

- (NSString*)description {
  return
      [NSString stringWithFormat:@"HostInfo: name=%@ status=%@ updatedTime= %@",
                                 _hostName, _status, _updatedTime];
}

@end
