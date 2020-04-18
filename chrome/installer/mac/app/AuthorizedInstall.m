// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "AuthorizedInstall.h"

@interface AuthorizedInstall () {
  NSFileHandle* communicationFile_;
  NSString* destinationAppBundlePath_;
}
@end

@implementation AuthorizedInstall
// Does the setup needed to authorize a subprocess to run as root.
- (OSStatus)setUpAuthorization:(AuthorizationRef*)authRef {
  OSStatus status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment,
                                        kAuthorizationFlagDefaults, authRef);

  AuthorizationItem items = {kAuthorizationRightExecute, 0, NULL, 0};
  AuthorizationRights rights = {1, &items};
  AuthorizationFlags flags =
      kAuthorizationFlagDefaults | kAuthorizationFlagInteractionAllowed |
      kAuthorizationFlagPreAuthorize | kAuthorizationFlagExtendRights;

  status = AuthorizationCopyRights(*authRef, &rights, NULL, flags, NULL);
  return status;
}

// Starts up the proccess with privileged permissions.
- (void)startPrivilegedTool:(const char*)toolPath
              withArguments:(const char**)args
              authorization:(AuthorizationRef)authRef
                     status:(OSStatus)status {
  if (status != errAuthorizationSuccess)
    return;

  FILE* file;
// AuthorizationExecuteWithPrivileges is deprecated in macOS 10.7, but no good
// replacement exists. https://crbug.com/593133.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  status = AuthorizationExecuteWithPrivileges(
      authRef, toolPath, kAuthorizationFlagDefaults, (char* const*)args, &file);
#pragma clang diagnostic pop
  communicationFile_ = [[NSFileHandle alloc] initWithFileDescriptor:fileno(file)
                                                     closeOnDealloc:YES];
}

// Starts up same proccess as above without privileged permissions.
- (void)startUnprivilegedTool:(NSString*)toolPath withArguments:(NSArray*)args {
  NSPipe* pipe = [NSPipe pipe];
  NSTask* task = [[NSTask alloc] init];
  [task setArguments:args];
  [task setLaunchPath:toolPath];
  [task setStandardInput:pipe];
  [task launch];
  communicationFile_ = [pipe fileHandleForWriting];
}

// Determines which "Applications" folder to use based on authorization.
// There are three possible scenarios and two possible return values.
//   1) /Applications is returned if:
//     a) The user authenticates the app.
//     b) The user doesn't authenticate but is an admin.
//   2) $HOME/Applications is returned if:
//     c) The user doesn't authenticate and is not an admin.
- (NSString*)getApplicationsFolder:(BOOL)isAuthorized {
  NSFileManager* manager = [NSFileManager defaultManager];
  NSArray* applicationDirectories = NSSearchPathForDirectoriesInDomains(
      NSApplicationDirectory, NSLocalDomainMask, YES);
  if (isAuthorized ||
      [manager isWritableFileAtPath:applicationDirectories.firstObject]) {
    return applicationDirectories.firstObject;
  } else {
    NSString* usersApplicationsDirectory =
        [NSString pathWithComponents:@[ NSHomeDirectory(), @"Applications" ]];
    if (![manager fileExistsAtPath:usersApplicationsDirectory]) {
      [manager createDirectoryAtPath:usersApplicationsDirectory
          withIntermediateDirectories:NO
                           attributes:nil
                                error:nil];
    }
    return usersApplicationsDirectory;
  }
}

- (BOOL)loadInstallationTool {
  AuthorizationRef authRef = NULL;
  OSStatus status = [self setUpAuthorization:&authRef];
  BOOL isAuthorized = (status == errAuthorizationSuccess);

  NSString* toolPath =
      [[NSBundle mainBundle] pathForResource:@"copy_to_disk" ofType:@"sh"];
  NSFileManager* manager = [NSFileManager defaultManager];
  if (![manager fileExistsAtPath:toolPath]) {
    return false;
  }

  NSString* applicationsDirectory = [self getApplicationsFolder:isAuthorized];
  destinationAppBundlePath_ = [NSString pathWithComponents: @[
      applicationsDirectory, @"Google Chrome.app"]];

  if (isAuthorized) {
    const char* args[] = {[applicationsDirectory UTF8String], NULL};
    [self startPrivilegedTool:[toolPath UTF8String]
                withArguments:args
                authorization:authRef
                       status:status];
  } else {
    NSArray* args = @[ applicationsDirectory ];
    [self startUnprivilegedTool:toolPath withArguments:args];
  }

  AuthorizationFree(authRef, kAuthorizationFlagDestroyRights);
  return true;
}

// Sends a message to the tool's stdin. The tool is using 'read' to wait for
// input. 'read' adds to its buffer until it receives a newline to continue so
// append '\n' to the message to end the read.
- (void)sendMessageToTool:(NSString*)message {
  [communicationFile_  writeData:[[message stringByAppendingString:@"\n"]
               dataUsingEncoding:NSUTF8StringEncoding]];
}

- (NSString*)startInstall:(NSString*)appBundlePath {
  [self sendMessageToTool:appBundlePath];
  return destinationAppBundlePath_;
}

@end
