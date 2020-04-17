// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - Read and check the response message
// - Make a nice object API with methods that can do more than one thing per connection
// - Make it possible to have a presentation controller that is a server

import (
	"context"
)

func StartPresentation(ctx context.Context, hostname string, port int, url string) error {
	msg := PresentationStartRequest{
		RequestID:      1,
		PresentationID: "This is a Presentation ID.  It must be very long.",
		URL:            url,
	}

	return SendMessageAsClient(ctx, hostname, port, msg)
}
