// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SESSIONS_SESSION_DATA_DELETER_H_
#define CHROME_BROWSER_SESSIONS_SESSION_DATA_DELETER_H_

class Profile;

// Clears cookies and local storage for origins that are session-only and clears
// session cookies unless the startup preference is to continue the previous
// session.
void DeleteSessionOnlyData(Profile* profile);

#endif  // CHROME_BROWSER_SESSIONS_SESSION_DATA_DELETER_H_
