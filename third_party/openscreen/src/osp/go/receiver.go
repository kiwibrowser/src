// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - Send a response message
// - Make a nice object API with methods
// - Make it possible to have a presentation receiver that is a client
// - Close connection on unknown message types

import (
	"context"
	"crypto/tls"
)

func RunPresentationReceiver(ctx context.Context, mdnsInstanceName string, port int, cert tls.Certificate, presentUrl func(string)) {
	messages := make(chan interface{})
	go ReadMessagesAsServer(ctx, mdnsInstanceName, port, cert, messages)
	for msg := range messages {
		switch m := msg.(type) {
		case PresentationStartRequest:
			presentUrl(m.URL)
		}
	}
}
