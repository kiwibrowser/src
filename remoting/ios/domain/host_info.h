// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_DOMAIN_HOST_INFO_H_
#define REMOTING_IOS_DOMAIN_HOST_INFO_H_

#import <Foundation/Foundation.h>

namespace remoting {
struct HostInfo;
}  // namespace remoting

// A detail record for a Remoting Host.
@interface HostInfo : NSObject

// Various properties of the Remoting Host.
@property(nonatomic, copy) NSString* createdTime;
@property(nonatomic, copy) NSString* hostId;
@property(nonatomic, copy) NSString* hostName;
@property(nonatomic, copy) NSString* hostOs;
@property(nonatomic, copy) NSString* hostOsVersion;
@property(nonatomic, copy) NSString* hostVersion;
@property(nonatomic, copy) NSString* jabberId;
@property(nonatomic, copy) NSString* kind;
@property(nonatomic, copy) NSString* publicKey;
@property(nonatomic, copy) NSString* status;
@property(nonatomic, copy) NSString* updatedTime;
@property(nonatomic, copy) NSString* offlineReason;
// True when |status| is @"ONLINE", anything else is False.
@property(nonatomic, readonly) bool isOnline;

- (instancetype)initWithRemotingHostInfo:(const remoting::HostInfo&)hostInfo;

// First consider if |isOnline| is greater than anything else, then consider by
// case insensitive locale of |hostName|.
- (NSComparisonResult)compare:(HostInfo*)host;

@end

#endif  //  REMOTING_IOS_DOMAIN_HOST_INFO_H_
