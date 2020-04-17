// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

import (
	"encoding/binary"
	"errors"
	"io"
)

const (
	maxInt6  = 1<<5 - 1
	maxInt14 = 1<<13 - 1
	maxInt30 = 1<<29 - 1
	maxInt62 = 1<<63 - 1
)

// https://tools.ietf.org/html/draft-ietf-quic-transport-16#section-16
func ReadVaruint(r io.Reader) (uint64, error) {
	b0, err := readByte(r)
	if err != nil {
		return 0, err
	}
	var a [8]byte
	b := a[:]
	e := binary.BigEndian
	b[0] = last6Bits(b0)
	switch first2Bits(b0) {
	case 0:
		return uint64(b[0]), nil
	case 1:
		err = readBytes(r, b[1:2])
		return uint64(e.Uint16(b)), err
	case 2:
		err = readBytes(r, b[1:4])
		return uint64(e.Uint32(b)), err
	case 3:
		err = readBytes(r, b[1:8])
		return uint64(e.Uint64(b)), err
	}
	return 0, nil
}

// https://tools.ietf.org/html/draft-ietf-quic-transport-16#section-16
func WriteVaruint(v uint64, w io.Writer) error {
	var a [8]byte
	b := a[:]
	e := binary.BigEndian
	if v <= maxInt6 {
		b[0] = byte(v)
		setFirst2Bits(0, b)
		return writeBytes(b[:1], w)
	} else if v <= maxInt14 {
		e.PutUint16(b, uint16(v))
		setFirst2Bits(1, b)
		return writeBytes(b[:2], w)
	} else if v <= maxInt30 {
		e.PutUint32(b, uint32(v))
		setFirst2Bits(2, b)
		return writeBytes(b[:4], w)
	} else if v <= maxInt62 {
		e.PutUint64(b, v)
		setFirst2Bits(3, b)
		return writeBytes(b[:8], w)
	}
	return errors.New("Too big")
}

func first2Bits(b byte) byte {
	return b >> 6
}

func last6Bits(b byte) byte {
	return b & 0x3f // 0b00111111
}

func setFirst2Bits(first byte, b []byte) {
	b[0] = (first << 6) | b[0]
}

func readByte(r io.Reader) (byte, error) {
	var b [1]byte
	err := readBytes(r, b[:])
	return b[0], err
}

// Read len(b) bytes from r into b
// If you hit an error (of the end), propogate the error from r.Read
func readBytes(r io.Reader, b []byte) error {
	start := 0
	end := len(b)
	for start < end {
		n, err := r.Read(b[start:end])
		if err != nil {
			return err
		}
		start += n
	}
	return nil
}

// Write len(b) bytes from b to w
// If you hit an error, propogate the error from w.Write
func writeBytes(b []byte, w io.Writer) error {
	start := 0
	end := len(b)
	for start < end {
		n, err := w.Write(b[start:end])
		if err != nil {
			return err
		}
		start += n
	}
	return nil
}
