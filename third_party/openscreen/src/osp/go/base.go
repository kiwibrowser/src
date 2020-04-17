// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

import (
	"context"
)

func waitUntilDone(ctx context.Context) {
	<-ctx.Done()
}

func done(ctx context.Context) bool {
	return ctx.Err() != nil
}
