// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - avoid NetworkIdleTimeout
// - make a client object that can send and receive more than one stream
// - make a server object that can send and receive more than one stream

import (
	"context"
	"crypto/rand"
	"crypto/rsa"
	"crypto/tls"
	"crypto/x509"
	"encoding/pem"
	"fmt"
	"io"
	"log"
	"math/big"

	quic "github.com/lucas-clemente/quic-go"
)

func GenerateTlsCert() (*tls.Certificate, error) {
	key, err := rsa.GenerateKey(rand.Reader, 1024)
	if err != nil {
		return nil, err
	}
	template := x509.Certificate{SerialNumber: big.NewInt(1)}
	certDER, err := x509.CreateCertificate(rand.Reader, &template, &template, &key.PublicKey, key)
	if err != nil {
		return nil, err
	}
	keyPEM := pem.EncodeToMemory(&pem.Block{Type: "RSA PRIVATE KEY", Bytes: x509.MarshalPKCS1PrivateKey(key)})
	certPEM := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: certDER})

	tlsCert, err := tls.X509KeyPair(certPEM, keyPEM)
	if err != nil {
		return nil, err
	}
	return &tlsCert, nil
}

func readAllStreams(ctx context.Context, session quic.Session, streams chan<- io.ReadWriteCloser) {
	for !done(ctx) {
		stream, err := session.AcceptStream()
		if err != nil && !done(ctx) {
			log.Println("Failed to accept stream with QUIC", err.Error())
		}
		streams <- stream
	}
}

// Returns a quic.Session object with a .OpenStreamSync method to send streams
func DialAsQuicClient(ctx context.Context, hostname string, port int) (quic.Session, error) {
	// TODO: Change InsecureSkipVerify
	tlsConfig := &tls.Config{InsecureSkipVerify: true}
	addr := fmt.Sprintf("%s:%d", hostname, port)
	session, err := quic.DialAddrContext(ctx, addr, tlsConfig, nil)
	return session, err
}

// Reads in streams
func RunQuicServer(ctx context.Context, port int, cert tls.Certificate, streams chan<- io.ReadWriteCloser) error {
	addr := fmt.Sprintf(":%d", port)
	tlsConfig := &tls.Config{Certificates: []tls.Certificate{cert}}
	listener, err := quic.ListenAddr(addr, tlsConfig, nil)
	if err != nil {
		return err
	}
	go func() {
		waitUntilDone(ctx)
		listener.Close()
	}()
	for {
		session, err := listener.Accept()
		if err != nil && !done(ctx) {
			log.Println("Failed to accept session with QUIC:", err.Error())
		}
		go readAllStreams(ctx, session, streams)
	}
}
