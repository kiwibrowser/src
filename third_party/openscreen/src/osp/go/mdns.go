// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package osp

// TODO(pthatcher):
// - Make our own abstraction that has
//   .InstanceName, .HostName, .MetadataVersion, .FingerPrint 
//   rather than using mdns.ServiceEntry
// - Advertise TXT (text below) with "fp" and "mv"

import (
	"context"

	mdns "github.com/grandcat/zeroconf"
)

const (
	MdnsServiceType = "_openscreen._udp"
	MdnsDomain      = "local"
)

// Returns a channel of mDNS entries
// The critical parts are entry.Target (name) entry.HostName (address)
func BrowseMdns(ctx context.Context) (<-chan *mdns.ServiceEntry, error) {
	entries := make(chan *mdns.ServiceEntry)

	resolver, err := mdns.NewResolver(nil)
	if err != nil {
		return entries, err
	}

	err = resolver.Browse(ctx, MdnsServiceType, MdnsDomain, entries)
	return entries, err
}

func RunMdnsServer(ctx context.Context, instance string, port int) error {
	var text []string
	server, err := mdns.Register(instance, MdnsServiceType, MdnsDomain, port, text, nil /* ifaces */)
	if err != nil {
		return err
	}
	waitUntilDone(ctx)
	server.Shutdown()
	return nil
}
