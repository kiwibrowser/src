// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * Interface representing actions to perform when entering and exiting a state, independent of
 * the activation data.
 */
public interface VoidScopeFactory { public Scope create(); }
