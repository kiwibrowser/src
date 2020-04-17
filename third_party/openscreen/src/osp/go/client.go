// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - Read messages as well, and more than one

import (
	"context"
)

func SendMessageAsClient(ctx context.Context, hostname string, port int, msg interface{}) error {
	session, err := DialAsQuicClient(ctx, hostname, port)
	if err != nil {
		return err
	}
	stream, err := session.OpenStreamSync()
	if err != nil {
		return err
	}
	return WriteMessage(msg, stream)
}
