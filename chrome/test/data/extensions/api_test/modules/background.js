// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(devlin): Sadly, there's no good way of checking that the import
// succeeds, so in the failure case the test just times out, rather than
// giving a useful error.
import {pass} from '/module.js';

pass();
