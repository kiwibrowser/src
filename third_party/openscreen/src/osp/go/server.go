// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - Write messages as well

import (
	"context"
	"crypto/tls"
	"io"
)

func ReadMessagesAsServer(ctx context.Context, instanceName string, port int, cert tls.Certificate, messages chan<- interface{}) error {
	// TODO: log error if it fails
	go RunMdnsServer(ctx, instanceName, port)
	streams := make(chan io.ReadWriteCloser)
	go RunQuicServer(ctx, port, cert, streams)

	for stream := range streams {
		msg, err := ReadMessage(stream)
		if err != nil {
			return err
		}
		messages <- msg
	}
	return nil
}
