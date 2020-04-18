/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.system;

import libcore.util.Objects;
import java.net.SocketAddress;

/**
 * Packet socket address.
 *
 * Corresponds to Linux's {@code struct sockaddr_ll}.
 *
 * @hide
 */
public final class PacketSocketAddress extends SocketAddress {
    /** Protocol. An Ethernet protocol type, e.g., {@code ETH_P_IPV6}. */
    public short sll_protocol;

    /** Interface index. */
    public int sll_ifindex;

    /** ARP hardware type. One of the {@code ARPHRD_*} constants. */
    public short sll_hatype;

    /** Packet type. One of the {@code PACKET_*} constants, such as {@code PACKET_OTHERHOST}. */
    public byte sll_pkttype;

    /** Hardware address. */
    public byte[] sll_addr;

    /** Constructs a new PacketSocketAddress. */
    public PacketSocketAddress(short sll_protocol, int sll_ifindex,
            short sll_hatype, byte sll_pkttype, byte[] sll_addr) {
        this.sll_protocol = sll_protocol;
        this.sll_ifindex = sll_ifindex;
        this.sll_hatype = sll_hatype;
        this.sll_pkttype = sll_pkttype;
        this.sll_addr = sll_addr;
    }

    /** Constructs a new PacketSocketAddress suitable for binding to. */
    public PacketSocketAddress(short sll_protocol, int sll_ifindex) {
        this(sll_protocol, sll_ifindex, (short) 0, (byte) 0, null);
    }

    /** Constructs a new PacketSocketAddress suitable for sending to. */
    public PacketSocketAddress(int sll_ifindex, byte[] sll_addr) {
        this((short) 0, sll_ifindex, (short) 0, (byte) 0, sll_addr);
    }
}
