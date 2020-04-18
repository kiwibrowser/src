// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync.notifier;

/** Interface for classes that create an Invalidation client's name. */
public interface InvalidationClientNameGenerator { public byte[] generateInvalidatorClientName(); }
