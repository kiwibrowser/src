// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Collection of constants defined for multicast DNS (RFC 6762) and DNS-Based
// Service Discovery (RFC 6763), along with a few constants used specifically
// for cast receiver's implementation of the mDNS spec (not 100% conformant).
//
// Some cast-specific implementation details include the maximum multicast
// message size (synced with sender SDK) and usage of site-local MDNS group
// in addition to the default link-local MDNS multicast group.

#ifndef CAST_COMMON_MDNS_MDNS_CONSTANTS_H_
#define CAST_COMMON_MDNS_MDNS_CONSTANTS_H_

#include <stddef.h>
#include <stdint.h>

namespace cast {
namespace mdns {

// ============================================================================
// Networking
// ============================================================================

// RFC 6762: https://www.ietf.org/rfc/rfc6762.txt
// RFC 2365: https://www.ietf.org/rfc/rfc2365.txt
// RFC 5771: https://www.ietf.org/rfc/rfc5771.txt
// RFC 7346: https://www.ietf.org/rfc/rfc7346.txt

// IPv4 group address for joining mDNS multicast group, given as byte array in
// network-order. This is a link-local multicast address, so messages will not
// be forwarded outside local network. See RFC 6762, section 3.
constexpr uint8_t kDefaultMulticastGroupIPv4[4] = {224, 0, 0, 251};

// IPv6 group address for joining mDNS multicast group, given as byte array in
// network-order. This is a link-local multicast address, so messages will not
// be forwarded outside local network. See RFC 6762, section 3.
constexpr uint8_t kDefaultMulticastGroupIpv6[16] = {
    0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB,
};

// IPv4 group address for joining cast-specific site-local mDNS multicast group,
// given as byte array in network-order. This is a site-local multicast address,
// so messages can extend past the local network to the administrative area
// boundary. Local sockets will filter out messages that are not on local-
// subnet though, so it will behave the same as link-local. The difference is
// that router sometimes treat link-local and site-local differently, which
// may cause link-local to have worse reliability than site-local.
// See https://tools.ietf.org/html/draft-cai-ssdp-v1-03

// 239.X.X.X is "administratively scoped IPv4 multicast space". See RFC 2365
// and RFC 5771, Section 3. Combined with relative address of 5 for SSDP this
// gives 239.255.255.250. See
// https://www.iana.org/assignments/multicast-addresses

// NOTE: For now the group address is the same group address used for SSDP
// discovery, albeit using the MDNS port rather than SSDP port.
constexpr uint8_t kDefaultSiteLocalGroupIPv4[4] = {239, 255, 255, 250};

// IPv6 group address for joining cast-specific site-local mDNS multicast group,
// give as byte array in network-order. See comments for IPv4 group address for
// more details on site-local vs. link-local.
// 0xFF05 is site-local. See RFC 7346.
// FF0X:0:0:0:0:0:0:C is variable scope multicast addresses for SSDP. See
// https://www.iana.org/assignments/ipv6-multicast-addresses
constexpr uint8_t kDefaultSiteLocalGroupIPv6[16] = {
    0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
};

// Default multicast port used by mDNS protocol. On some systems there may be
// multiple processes binding to same port, so prefer to allow address re-use.
// See RFC 6762, Section 2
constexpr uint16_t kDefaultMulticastPort = 5353;

// Maximum MTU size (1500) minus the UDP header size (8) and IP header size
// (20). If any packets are larger than this size, the responder or sender
// should break up the message into multiple packets and set the TC
// (Truncated) bit to 1. See RFC 6762, section 7.2.
// TODO(https://crbug.com/openscreen/47): Figure out the exact size that the
// senders are using and sync with them. We want to verify that we are using the
// same maximum packet size. The spec also suggests keeping all UDP messsages
// below 512 bytes, since that is where some fragmentation may occur. If
// possible we should measure the rate of fragmented messages and see if
// lowering the max size alleviates it.
constexpr size_t kMaxMulticastMessageSize = 1500 - 20 - 8;

// Specifies whether the site-local group described above should be enabled
// by default. When enabled, the responder will be able to receive messages from
// that group; when disabled, only the default MDNS multicast group will be
// enabled.
constexpr bool kDefaultSupportSiteLocalGroup = true;

// ============================================================================
// Message Header
// ============================================================================

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
// RFC 2535: https://www.ietf.org/rfc/rfc2535.txt

// DNS packet consists of a fixed-size header (12 bytes) followed by
// zero or more questions and/or records.
// For the meaning of specific fields, please see RFC 1035 and 2535.
//
// Header format:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                      ID                       |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |QR|   Opcode  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    QDCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    ANCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    NSCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    ARCOUNT                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// On-the-wire header. All uint16_t are in network order.
struct Header {
  uint16_t id;
  uint16_t flags;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
};

// TODO(mayaki): Here and below consider converting constants to members of
// enum classes.

// DNS Header flags. All flags are formatted to mask directly onto FLAG header
// field in network-byte order.
constexpr uint16_t kFlagResponse = 0x8000;
constexpr uint16_t kFlagAA = 0x0400;
constexpr uint16_t kFlagTC = 0x0200;
constexpr uint16_t kFlagRD = 0x0100;
constexpr uint16_t kFlagRA = 0x0080;
constexpr uint16_t kFlagZ = 0x0040;  // Unused field
constexpr uint16_t kFlagAD = 0x0020;
constexpr uint16_t kFlagCD = 0x0010;

// DNS Header OPCODE mask and values. The mask is formatted to mask directly
// onto FLAG header field in network-byte order. The values are formatted after
// shifting into correct position.
constexpr uint16_t kOpcodeMask = 0x7800;
constexpr uint8_t kOpcodeQUERY = 0;
constexpr uint8_t kOpcodeIQUERY = 1;
constexpr uint8_t kOpcodeSTATUS = 2;
constexpr uint8_t kOpcodeUNASSIGNED = 3;  // Unused for now
constexpr uint8_t kOpcodeNOTIFY = 4;
constexpr uint8_t kOpcodeUPDATE = 5;

// DNS Header RCODE mask and values. The mask is formatted to mask directly onto
// FLAG header field in network-byte order. The values are formatted after
// shifting into correct position.
constexpr uint16_t kRcodeMask = 0x000F;
constexpr uint8_t kRcodeNOERROR = 0;
constexpr uint8_t kRcodeFORMERR = 1;
constexpr uint8_t kRcodeSERVFAIL = 2;
constexpr uint8_t kRcodeNXDOMAIN = 3;
constexpr uint8_t kRcodeNOTIMP = 4;
constexpr uint8_t kRcodeREFUSED = 5;

// ============================================================================
// Domain Name
// ============================================================================

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt

// Maximum number of octets allowed in a single domain name.
constexpr size_t kMaxDomainNameLength = 255;
// Maximum number of octets allowed in a single domain name label.
constexpr size_t kMaxLabelLength = 63;

// To allow for message compression, domain names can fall under the following
// categories:
// - A sequence of labels ending in a zero octet (label of length zero)
// - A pointer to prior occurance of name in message
// - A sequence of labels ending with a pointer.
//
// Domain Name Label - DIRECT:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  | 0  0|  LABEL LENGTH   |        CHAR 0         |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /         CHAR 1        |        CHAR 2         /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
// Domain Name Label - POINTER:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  | 1  1|               OFFSET                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

constexpr uint8_t kLabelMask = 0xC0;
constexpr uint8_t kLabelPointer = 0xC0;
constexpr uint8_t kLabelDirect = 0x00;
constexpr uint8_t kLabelTermination = 0x00;
constexpr uint16_t kLabelOffsetMask = 0x3FFF;

// ============================================================================
// Record Fields
// ============================================================================

// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
// RFC 2535: https://www.ietf.org/rfc/rfc2535.txt

// Question format:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                                               |
//  /                     QNAME                     /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                     QTYPE                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                     QCLASS                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// Answer format:
//                                  1  1  1  1  1  1
//    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                                               |
//  /                                               /
//  /                      NAME                     /
//  |                                               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                      TYPE                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                     CLASS                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                      TTL                      |
//  |                                               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   RDLENGTH                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
//  /                     RDATA                     /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// DNS TYPE values. See http://www.iana.org/assignments/dns-parameters for full
// list. Only a sub-set is used and listed here.
constexpr uint16_t kTypeA = 1;
constexpr uint16_t kTypeCNAME = 5;
constexpr uint16_t kTypePTR = 12;
constexpr uint16_t kTypeTXT = 16;
constexpr uint16_t kTypeAAAA = 28;
constexpr uint16_t kTypeSRV = 33;
constexpr uint16_t kTypeNSEC = 47;
constexpr uint16_t kTypeANY = 255;  // Only allowed for QTYPE

// DNS CLASS masks and values.
constexpr uint16_t kClassMask = 0x7FFF;
constexpr uint16_t kClassIN = 1;
constexpr uint16_t kClassANY = 255;  // Only allowed for QCLASS
// In mDNS the most significant bit of the RRCLASS for response records is
// designated as the "cache-flush bit", as described in
// https://tools.ietf.org/html/rfc6762#section-10.2
constexpr uint16_t kCacheFlushBit = 0x8000;
// In mDNS the most significant bit of the RRCLASS for query records is
// designated as the "unicast-response bit", as described in
// https://tools.ietf.org/html/rfc6762#section-5.4
constexpr uint16_t kUnicastResponseBit = 0x8000;

// See RFC 6762, section 11: https://tools.ietf.org/html/rfc6762#section-11
//
// The IP TTL value for the UDP packets sent to the multicast group is advised
// to be 255 in order to be compatible with older DNS queriers. This also keeps
// consistent with other mDNS solutions (jMDNS, Avahi, etc.), which use 255
// as the IP TTL as well.
//
// The default mDNS group address is in a range of link-local addresses, so
// messages should not be forwarded by routers even when TTL is greater than 1.
constexpr uint32_t kDefaultRecordTTL = 255;

// ============================================================================
// RDATA Constants
// ============================================================================

// The maximum allowed size for a single entry in a TXT resource record. The
// mDNS spec specifies that entries in TXT record should be key/value pair
// separated by '=', so the size of the key + value + '=' should not exceed
// the maximum allowed size.
// See: https://tools.ietf.org/html/rfc6763#section-6.1
constexpr size_t kTXTMaxEntrySize = 255;
// Placeholder RDATA for "empty" TXT records that contains only a single
// zero length byte. This is required since TXT records CANNOT have empty
// RDATA sections.
// See RFC: https://tools.ietf.org/html/rfc6763#section-6.1
constexpr uint8_t kTXTEmptyRdata = 0;

}  // namespace mdns
}  // namespace cast

#endif  // CAST_COMMON_MDNS_MDNS_CONSTANTS_H_
