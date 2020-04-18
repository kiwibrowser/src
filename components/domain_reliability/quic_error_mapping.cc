// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/domain_reliability/quic_error_mapping.h"

namespace domain_reliability {

namespace {

const struct QuicErrorMapping {
  net::QuicErrorCode quic_error;
  const char* beacon_quic_error;
} kQuicErrorMap[] = {
    // Connection has reached an invalid state.
    {net::QUIC_INTERNAL_ERROR, "quic.internal_error"},
    // There were data frames after the a fin or reset.
    {net::QUIC_STREAM_DATA_AFTER_TERMINATION,
     "quic.stream_data.after_termination"},
    // Control frame is malformed.
    {net::QUIC_INVALID_PACKET_HEADER, "quic.invalid.packet_header"},
    // Frame data is malformed.
    {net::QUIC_INVALID_FRAME_DATA, "quic.invalid_frame_data"},
    // The packet contained no payload.
    {net::QUIC_MISSING_PAYLOAD, "quic.missing.payload"},
    // FEC data is malformed.
    {net::QUIC_INVALID_FEC_DATA, "quic.invalid.fec_data"},
    // STREAM frame data is malformed.
    {net::QUIC_INVALID_STREAM_DATA, "quic.invalid.stream_data"},
    // STREAM frame data is not encrypted.
    {net::QUIC_UNENCRYPTED_STREAM_DATA, "quic.unencrypted.stream_data"},
    // Attempt to send unencrypted STREAM frame.
    {net::QUIC_ATTEMPT_TO_SEND_UNENCRYPTED_STREAM_DATA,
     "quic.attempt.to.unencrypted.stream.data"},
    // Received a frame which is likely the result of memory corruption.
    {net::QUIC_MAYBE_CORRUPTED_MEMORY, "quic.maybe.corrupted.momery"},
    // FEC frame data is not encrypted.
    {net::QUIC_UNENCRYPTED_FEC_DATA, "quic.unencrypted.fec.data"},
    // RST_STREAM frame data is malformed.
    {net::QUIC_INVALID_RST_STREAM_DATA, "quic.invalid.rst_stream_data"},
    // CONNECTION_CLOSE frame data is malformed.
    {net::QUIC_INVALID_CONNECTION_CLOSE_DATA,
     "quic.invalid.connection_close_data"},
    // GOAWAY frame data is malformed.
    {net::QUIC_INVALID_GOAWAY_DATA, "quic.invalid.goaway_data"},
    // WINDOW_UPDATE frame data is malformed.
    {net::QUIC_INVALID_WINDOW_UPDATE_DATA, "quic.invalid.window_update_data"},
    // BLOCKED frame data is malformed.
    {net::QUIC_INVALID_BLOCKED_DATA, "quic.invalid.blocked_data"},
    // STOP_WAITING frame data is malformed.
    {net::QUIC_INVALID_STOP_WAITING_DATA, "quic.invalid.stop_waiting_data"},
    // PATH_CLOSE frame data is malformed.
    {net::QUIC_INVALID_PATH_CLOSE_DATA, "quic.invalid_path_close_data"},
    // ACK frame data is malformed.
    {net::QUIC_INVALID_ACK_DATA, "quic.invalid.ack_data"},

    // Version negotiation packet is malformed.
    {net::QUIC_INVALID_VERSION_NEGOTIATION_PACKET,
     "quic_invalid_version_negotiation_packet"},
    // Public RST packet is malformed.
    {net::QUIC_INVALID_PUBLIC_RST_PACKET, "quic.invalid.public_rst_packet"},

    // There was an error decrypting.
    {net::QUIC_DECRYPTION_FAILURE, "quic.decryption.failure"},
    // There was an error encrypting.
    {net::QUIC_ENCRYPTION_FAILURE, "quic.encryption.failure"},
    // The packet exceeded kMaxPacketSize.
    {net::QUIC_PACKET_TOO_LARGE, "quic.packet.too_large"},
    // The peer is going away.  May be a client or server.
    {net::QUIC_PEER_GOING_AWAY, "quic.peer_going_away"},
    // A stream ID was invalid.
    {net::QUIC_INVALID_STREAM_ID, "quic.invalid_stream_id"},
    // A priority was invalid.
    {net::QUIC_INVALID_PRIORITY, "quic.invalid_priority"},
    // Too many streams already open.
    {net::QUIC_TOO_MANY_OPEN_STREAMS, "quic.too_many_open_streams"},
    // The peer created too many available streams.
    {net::QUIC_TOO_MANY_AVAILABLE_STREAMS, "quic.too_many_available_streams"},
    // Received public reset for this connection.
    {net::QUIC_PUBLIC_RESET, "quic.public_reset"},
    // Invalid protocol version.
    {net::QUIC_INVALID_VERSION, "quic.invalid_version"},

    // The Header ID for a stream was too far from the previous.
    {net::QUIC_INVALID_HEADER_ID, "quic.invalid_header_id"},
    // Negotiable parameter received during handshake had invalid value.
    {net::QUIC_INVALID_NEGOTIATED_VALUE, "quic.invalid_negotiated_value"},
    // There was an error decompressing data.
    {net::QUIC_DECOMPRESSION_FAILURE, "quic.decompression_failure"},
    // We hit our prenegotiated (or default) timeout
    {net::QUIC_NETWORK_IDLE_TIMEOUT, "quic.connection.idle_time_out"},
    // We hit our overall connection timeout
    {net::QUIC_HANDSHAKE_TIMEOUT, "quic.connection.handshake_timed_out"},
    // There was an error encountered migrating addresses.
    {net::QUIC_ERROR_MIGRATING_ADDRESS, "quic.error_migrating_address"},
    // There was an error encountered migrating port only.
    {net::QUIC_ERROR_MIGRATING_PORT, "quic.error_migrating_port"},
    // There was an error while writing to the socket.
    {net::QUIC_PACKET_WRITE_ERROR, "quic.packet.write_error"},
    // There was an error while reading from the socket.
    {net::QUIC_PACKET_READ_ERROR, "quic.packet.read_error"},
    // We received a STREAM_FRAME with no data and no fin flag set.
    {net::QUIC_EMPTY_STREAM_FRAME_NO_FIN, "quic.empty_stream_frame_no_fin"},
    // We received invalid data on the headers stream.
    {net::QUIC_INVALID_HEADERS_STREAM_DATA, "quic.invalid_headers_stream_data"},
    // The peer received too much data, violating flow control.
    {net::QUIC_FLOW_CONTROL_RECEIVED_TOO_MUCH_DATA,
     "quic.flow_control.received_too_much_data"},
    // The peer sent too much data, violating flow control.
    {net::QUIC_FLOW_CONTROL_SENT_TOO_MUCH_DATA,
     "quic.flow_control.sent_too_much_data"},
    // The peer received an invalid flow control window.
    {net::QUIC_FLOW_CONTROL_INVALID_WINDOW, "quic.flow_control.invalid_window"},
    // The connection has been IP pooled into an existing connection.
    {net::QUIC_CONNECTION_IP_POOLED, "quic.connection.ip_pooled"},
    // The connection has too many outstanding sent packets.
    {net::QUIC_TOO_MANY_OUTSTANDING_SENT_PACKETS,
     "quic.too_many_outstanding_sent_packets"},
    // The connection has too many outstanding received packets.
    {net::QUIC_TOO_MANY_OUTSTANDING_RECEIVED_PACKETS,
     "quic.too_many_outstanding_received_packets"},
    // The quic connection job to load server config is cancelled.
    {net::QUIC_CONNECTION_CANCELLED, "quic.connection.cancelled"},
    // Disabled QUIC because of high packet loss rate.
    {net::QUIC_BAD_PACKET_LOSS_RATE, "quic.bad_packet_loss_rate"},
    // Disabled QUIC because of too many PUBLIC_RESETs post handshake.
    {net::QUIC_PUBLIC_RESETS_POST_HANDSHAKE,
     "quic.public_resets_post_handshake"},
    // Closed because we failed to serialize a packet.
    {net::QUIC_FAILED_TO_SERIALIZE_PACKET, "quic.failed_to_serialize_packet"},
    // QUIC timed out after too many RTOs.
    {net::QUIC_TOO_MANY_RTOS, "quic.too_many_rtos"},
    // Crypto errors.

    // Hanshake failed.
    {net::QUIC_HANDSHAKE_FAILED, "quic.handshake_failed"},
    // Handshake message contained out of order tags.
    {net::QUIC_CRYPTO_TAGS_OUT_OF_ORDER, "quic.crypto.tags_out_of_order"},
    // Handshake message contained too many entries.
    {net::QUIC_CRYPTO_TOO_MANY_ENTRIES, "quic.crypto.too_many_entries"},
    // Handshake message contained an invalid value length.
    {net::QUIC_CRYPTO_INVALID_VALUE_LENGTH, "quic.crypto.invalid_value_length"},
    // A crypto message was received after the handshake was complete.
    {net::QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE,
     "quic.crypto_message_after_handshake_complete"},
    // A crypto message was received with an illegal message tag.
    {net::QUIC_INVALID_CRYPTO_MESSAGE_TYPE, "quic.invalid_crypto_message_type"},
    // A crypto message was received with an illegal parameter.
    {net::QUIC_INVALID_CRYPTO_MESSAGE_PARAMETER,
     "quic.invalid_crypto_message_parameter"},
    // An invalid channel id signature was supplied.
    {net::QUIC_INVALID_CHANNEL_ID_SIGNATURE,
     "quic.invalid_channel_id_signature"},
    // A crypto message was received with a mandatory parameter missing.
    {net::QUIC_CRYPTO_MESSAGE_PARAMETER_NOT_FOUND,
     "quic.crypto_message.parameter_not_found"},
    // A crypto message was received with a parameter that has no overlap
    // with the local parameter.
    {net::QUIC_CRYPTO_MESSAGE_PARAMETER_NO_OVERLAP,
     "quic.crypto_message.parameter_no_overlap"},
    // A crypto message was received that contained a parameter with too few
    // values.
    {net::QUIC_CRYPTO_MESSAGE_INDEX_NOT_FOUND,
     "quic_crypto_message_index_not_found"},
    // A demand for an unsupport proof type was received.
    {net::QUIC_UNSUPPORTED_PROOF_DEMAND, "quic.unsupported_proof_demand"},
    // An internal error occured in crypto processing.
    {net::QUIC_CRYPTO_INTERNAL_ERROR, "quic.crypto.internal_error"},
    // A crypto handshake message specified an unsupported version.
    {net::QUIC_CRYPTO_VERSION_NOT_SUPPORTED,
     "quic.crypto.version_not_supported"},
    // A crypto handshake message resulted in a stateless reject.
    {net::QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT,
     "quic.crypto.handshake_stateless_reject"},
    // There was no intersection between the crypto primitives supported by the
    // peer and ourselves.
    {net::QUIC_CRYPTO_NO_SUPPORT, "quic.crypto.no_support"},
    // The server rejected our client hello messages too many times.
    {net::QUIC_CRYPTO_TOO_MANY_REJECTS, "quic.crypto.too_many_rejects"},
    // The client rejected the server's certificate chain or signature.
    {net::QUIC_PROOF_INVALID, "quic.proof_invalid"},
    // A crypto message was received with a duplicate tag.
    {net::QUIC_CRYPTO_DUPLICATE_TAG, "quic.crypto.duplicate_tag"},
    // A crypto message was received with the wrong encryption level (i.e. it
    // should have been encrypted but was not.)
    {net::QUIC_CRYPTO_ENCRYPTION_LEVEL_INCORRECT,
     "quic.crypto.encryption_level_incorrect"},
    // The server config for a server has expired.
    {net::QUIC_CRYPTO_SERVER_CONFIG_EXPIRED,
     "quic.crypto.server_config_expired"},
    // We failed to setup the symmetric keys for a connection.
    {net::QUIC_CRYPTO_SYMMETRIC_KEY_SETUP_FAILED,
     "quic.crypto.symmetric_key_setup_failed"},
    // A handshake message arrived, but we are still validating the
    // previous handshake message.
    {net::QUIC_CRYPTO_MESSAGE_WHILE_VALIDATING_CLIENT_HELLO,
     "quic.crypto_message_while_validating_client_hello"},
    // A server config update arrived before the handshake is complete.
    {net::QUIC_CRYPTO_UPDATE_BEFORE_HANDSHAKE_COMPLETE,
     "quic.crypto.update_before_handshake_complete"},
    // CHLO cannot fit in one packet.
    {net::QUIC_CRYPTO_CHLO_TOO_LARGE, "quic.crypto.chlo_too_large"},
    // This connection involved a version negotiation which appears to have been
    // tampered with.
    {net::QUIC_VERSION_NEGOTIATION_MISMATCH,
     "quic.version_negotiation_mismatch"},

    // Multipath is not enabled, but a packet with multipath flag on is
    // received.
    {net::QUIC_BAD_MULTIPATH_FLAG, "quic.bad_multipath_flag"},
    // A path is supposed to exist but does not.
    {net::QUIC_MULTIPATH_PATH_DOES_NOT_EXIST,
     "quic.quic_multipath_path_does_not_exist"},
    // A path is supposed to be active but is not.
    {net::QUIC_MULTIPATH_PATH_NOT_ACTIVE,
     "quic.quic_multipath_path_not_active"},

    // Network change and connection migration errors.

    // IP address changed causing connection close.
    {net::QUIC_IP_ADDRESS_CHANGED, "quic.ip_address_changed"},
    // Network changed, but connection had no migratable streams.
    {net::QUIC_CONNECTION_MIGRATION_NO_MIGRATABLE_STREAMS,
     "quic.connection_migration_no_migratable_streams"},
    // Connection changed networks too many times.
    {net::QUIC_CONNECTION_MIGRATION_TOO_MANY_CHANGES,
     "quic.connection_migration_too_many_changes"},
    // Connection migration was attempted, but there was no new network to
    // migrate to.
    {net::QUIC_CONNECTION_MIGRATION_NO_NEW_NETWORK,
     "quic.connection_migration_no_new_network"},
    // Network changed, but connection had one or more non-migratable streams.
    {net::QUIC_CONNECTION_MIGRATION_NON_MIGRATABLE_STREAM,
     "quic.connection_migration_non_migratable_stream"},
    // Network changed, but connection migration was disabled by config.
    {net::QUIC_CONNECTION_MIGRATION_DISABLED_BY_CONFIG,
     "quic.connection_migration_disabled_by_config"},
    // Network changed, but error was encountered on the alternative network.
    {net::QUIC_CONNECTION_MIGRATION_INTERNAL_ERROR,
     "quic.connection_migration_internal_error"},

    // Stream frame overlaps with buffered data.
    {net::QUIC_OVERLAPPING_STREAM_DATA, "quic.overlapping_stream_data"},
    // Stream frames arrived too discontiguously so that stream sequencer buffer
    // maintains too many intervals.
    {net::QUIC_TOO_MANY_STREAM_DATA_INTERVALS,
     "quic.too_many_stream_data_intervals"},
    // Sequencer buffer get into weird state where continuing read/write
    // will lead to crash.
    {net::QUIC_STREAM_SEQUENCER_INVALID_STATE,
     "quic.stream_sequencer_invalid_state"},
    // Connection closed because of server hits max number of sessions allowed.
    {net::QUIC_TOO_MANY_SESSIONS_ON_SERVER, "quic.too_many_sessions_on_server"},
    // There was an error decompressing data.
    {net::QUIC_DECOMPRESSION_FAILURE, "quic.decompression_failure"},
    // Receive a RST_STREAM with offset larger than kMaxStreamLength.
    {net::QUIC_STREAM_LENGTH_OVERFLOW, "quic.stream_length_overflow"},

    // No error. Used as bound while iterating.
    {net::QUIC_LAST_ERROR, "quic.last_error"}};

// Must be updated any time a net::QuicErrorCode is deprecated in
// net/quic/core/quic_packets.h.
const int kDeprecatedQuicErrorCount = 5;
const int kActiveQuicErrorCount =
    net::QUIC_LAST_ERROR - kDeprecatedQuicErrorCount;

static_assert(arraysize(kQuicErrorMap) == kActiveQuicErrorCount,
              "quic_error_map is not in sync with quic protocol!");

}  // namespace

// static
bool GetDomainReliabilityBeaconQuicError(net::QuicErrorCode quic_error,
                                         std::string* beacon_quic_error_out) {
  if (quic_error != net::QUIC_NO_ERROR) {
    // Convert a QUIC error.
    // TODO(juliatuttle): Consider sorting and using binary search?
    for (size_t i = 0; i < arraysize(kQuicErrorMap); i++) {
      if (kQuicErrorMap[i].quic_error == quic_error) {
        *beacon_quic_error_out = kQuicErrorMap[i].beacon_quic_error;
        return true;
      }
    }
  }
  beacon_quic_error_out->clear();
  return false;
}

}  // namespace domain_reliability
