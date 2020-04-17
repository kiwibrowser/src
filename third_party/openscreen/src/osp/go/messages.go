// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - Read and write size prefixes

import (
	"fmt"
	"github.com/ugorji/go/codec"
	"io"
)

const PresentationStartRequestTypeKey = 104

// AKA presentation-start-request
type PresentationStartRequest struct {
	// Must be public to allow (de)serialization
	RequestID      uint64 `codec:"0"`
	PresentationID string `codec:"1"`
	URL            string `codec:"2"`
}

// Read with type/size prefix
func ReadMessage(r io.Reader) (interface{}, error) {
	typeKey, err := ReadVaruint(r)
	if err != nil {
		return nil, err
	}

	switch typeKey {
	case PresentationStartRequestTypeKey:
		var msg PresentationStartRequest
		h := &codec.CborHandle{}
		dec := codec.NewDecoder(r, h)
		err = dec.Decode(&msg)
		if err != nil {
			return nil, err
		}
		return msg, nil
	default:
		return nil, fmt.Errorf("Unknown type key: %d", typeKey)
	}
}

// Write with type/size prefix
func WriteMessage(msg interface{}, w io.Writer) error {
	var typeKey uint64
	switch msg.(type) {
	case PresentationStartRequest:
		typeKey = uint64(PresentationStartRequestTypeKey)
	default:
		return fmt.Errorf("Unknown message type: %v", msg)
	}

	err := WriteVaruint(typeKey, w)
	if err != nil {
		return err
	}

	h := &codec.CborHandle{}
	enc := codec.NewEncoder(w, h)
	return enc.Encode(msg)
}
