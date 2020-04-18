// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#import "testing/gtest/ios_enable_coverage.h"

#if !defined(NDEBUG) && BUILDFLAG(IOS_ENABLE_COVERAGE)
extern "C" void __llvm_profile_set_filename(const char* name);
#endif

namespace coverage_util {

void ConfigureCoverageReportPath() {
#if !defined(NDEBUG) && BUILDFLAG(IOS_ENABLE_COVERAGE)
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    // Writes the profraw file to the Documents directory, where the app has
    // write rights.
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                         NSUserDomainMask, YES);
    NSString* documents_directory = [paths firstObject];
    NSString* file_name = [documents_directory
        stringByAppendingPathComponent:@"coverage.profraw"];

    // For documentation, see:
    // http://clang.llvm.org/docs/SourceBasedCodeCoverage.html
    __llvm_profile_set_filename(
        [file_name cStringUsingEncoding:NSUTF8StringEncoding]);

    // Print the path for easier retrieval.
    NSLog(@"Coverage data at %@.", file_name);
  });
#endif  // !defined(NDEBUG) && BUILDFLAG(IOS_ENABLE_COVERAGE)
}

}  // namespace coverage_util
