// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/net_error_rust_reference.h"

#include <string>

#include "base/check_op.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
namespace net_error_rust_frozen_quic {
enum QuicErrorCode : uint32_t {
  QUIC_NO_ERROR = 0,

  // Connection has reached an invalid state.
  QUIC_INTERNAL_ERROR = 1,
  // There were data frames after the a fin or reset.
  QUIC_STREAM_DATA_AFTER_TERMINATION = 2,
  // Control frame is malformed.
  QUIC_INVALID_PACKET_HEADER = 3,
  // Frame data is malformed.
  QUIC_INVALID_FRAME_DATA = 4,
  // The packet contained no payload.
  QUIC_MISSING_PAYLOAD = 48,
  // FEC data is malformed.
  QUIC_INVALID_FEC_DATA = 5,
  // STREAM frame data is malformed.
  QUIC_INVALID_STREAM_DATA = 46,
  // STREAM frame data overlaps with buffered data.
  QUIC_OVERLAPPING_STREAM_DATA = 87,
  // Received STREAM frame data is not encrypted.
  QUIC_UNENCRYPTED_STREAM_DATA = 61,
  // Attempt to send unencrypted STREAM frame.
  QUIC_ATTEMPT_TO_SEND_UNENCRYPTED_STREAM_DATA = 88,
  // Received a frame which is likely the result of memory corruption.
  QUIC_MAYBE_CORRUPTED_MEMORY = 89,
  // FEC frame data is not encrypted.
  QUIC_UNENCRYPTED_FEC_DATA = 77,
  // RST_STREAM frame data is malformed.
  QUIC_INVALID_RST_STREAM_DATA = 6,
  // CONNECTION_CLOSE frame data is malformed.
  QUIC_INVALID_CONNECTION_CLOSE_DATA = 7,
  // GOAWAY frame data is malformed.
  QUIC_INVALID_GOAWAY_DATA = 8,
  // WINDOW_UPDATE frame data is malformed.
  QUIC_INVALID_WINDOW_UPDATE_DATA = 57,
  // BLOCKED frame data is malformed.
  QUIC_INVALID_BLOCKED_DATA = 58,
  // STOP_WAITING frame data is malformed.
  QUIC_INVALID_STOP_WAITING_DATA = 60,
  // PATH_CLOSE frame data is malformed.
  QUIC_INVALID_PATH_CLOSE_DATA = 78,
  // ACK frame data is malformed.
  QUIC_INVALID_ACK_DATA = 9,
  // Message frame data is malformed.
  QUIC_INVALID_DATAGRAM_DATA = 112,

  // Version negotiation packet is malformed.
  QUIC_INVALID_VERSION_NEGOTIATION_PACKET = 10,
  // Public RST packet is malformed.
  QUIC_INVALID_PUBLIC_RST_PACKET = 11,
  // There was an error decrypting.
  QUIC_DECRYPTION_FAILURE = 12,
  // There was an error encrypting.
  QUIC_ENCRYPTION_FAILURE = 13,
  // The packet exceeded kMaxOutgoingPacketSize.
  QUIC_PACKET_TOO_LARGE = 14,
  // The peer is going away.  May be a client or server.
  QUIC_PEER_GOING_AWAY = 16,
  // A stream ID was invalid.
  QUIC_INVALID_STREAM_ID = 17,
  // A priority was invalid.
  QUIC_INVALID_PRIORITY = 49,
  // Too many streams already open.
  QUIC_TOO_MANY_OPEN_STREAMS = 18,
  // The peer created too many available streams.
  QUIC_TOO_MANY_AVAILABLE_STREAMS = 76,
  // Received public reset for this connection.
  QUIC_PUBLIC_RESET = 19,
  // Version selected by client is not acceptable to the server.
  QUIC_INVALID_VERSION = 20,
  // Received packet indicates version that does not match connection version.
  QUIC_PACKET_WRONG_VERSION = 212,

  // The Header ID for a stream was too far from the previous.
  QUIC_INVALID_HEADER_ID = 22,
  // Negotiable parameter received during handshake had invalid value.
  QUIC_INVALID_NEGOTIATED_VALUE = 23,
  // There was an error decompressing data.
  QUIC_DECOMPRESSION_FAILURE = 24,
  // The connection timed out due to no network activity.
  QUIC_NETWORK_IDLE_TIMEOUT = 25,
  // The connection timed out waiting for the handshake to complete.
  QUIC_HANDSHAKE_TIMEOUT = 67,
  // There was an error encountered migrating addresses.
  QUIC_ERROR_MIGRATING_ADDRESS = 26,
  // There was an error encountered migrating port only.
  QUIC_ERROR_MIGRATING_PORT = 86,
  // There was an error while writing to the socket.
  QUIC_PACKET_WRITE_ERROR = 27,
  // There was an error while reading from the socket.
  QUIC_PACKET_READ_ERROR = 51,
  // We received a STREAM_FRAME with no data and no fin flag set.
  QUIC_EMPTY_STREAM_FRAME_NO_FIN = 50,
  // We received invalid data on the headers stream.
  QUIC_INVALID_HEADERS_STREAM_DATA = 56,
  // Invalid data on the headers stream received because of decompression
  // failure.
  QUIC_HEADERS_STREAM_DATA_DECOMPRESS_FAILURE = 97,
  // The peer received too much data, violating flow control.
  QUIC_FLOW_CONTROL_RECEIVED_TOO_MUCH_DATA = 59,
  // The peer sent too much data, violating flow control.
  QUIC_FLOW_CONTROL_SENT_TOO_MUCH_DATA = 63,
  // The peer received an invalid flow control window.
  QUIC_FLOW_CONTROL_INVALID_WINDOW = 64,
  // The peer sent a RESET_STREAM or STREAM frame indicating a change in the
  // final size for the stream.
  QUIC_FLOW_CONTROL_FINAL_SIZE_CHANGED = 221,
  // The connection has been IP pooled into an existing connection.
  QUIC_CONNECTION_IP_POOLED = 62,
  // The connection has too many outstanding sent packets.
  QUIC_TOO_MANY_OUTSTANDING_SENT_PACKETS = 68,
  // The connection has too many outstanding received packets.
  QUIC_TOO_MANY_OUTSTANDING_RECEIVED_PACKETS = 69,
  // The quic connection has been cancelled.
  QUIC_CONNECTION_CANCELLED = 70,
  // Disabled QUIC because of high packet loss rate.
  QUIC_BAD_PACKET_LOSS_RATE = 71,
  // Disabled QUIC because of too many PUBLIC_RESETs post handshake.
  QUIC_PUBLIC_RESETS_POST_HANDSHAKE = 73,
  // Closed because we failed to serialize a packet.
  QUIC_FAILED_TO_SERIALIZE_PACKET = 75,
  // QUIC timed out after too many RTOs.
  QUIC_TOO_MANY_RTOS = 85,

  // Crypto errors.

  // Handshake failed.
  QUIC_HANDSHAKE_FAILED = 28,
  // Split from QUIC_HANDSHAKE_FAILED and specially indicates handshake failure
  // due to packets buffered for too long.
  QUIC_HANDSHAKE_FAILED_PACKETS_BUFFERED_TOO_LONG = 214,
  // Handshake failed due to invalid hostname in ClientHello. Only sent from
  // server.
  QUIC_HANDSHAKE_FAILED_INVALID_HOSTNAME = 216,
  // Handshake failed because server is rejecting all connections. Only sent
  // from server.
  QUIC_HANDSHAKE_FAILED_REJECTING_ALL_CONNECTIONS = 217,
  // Handshake failed because the connection failed validity checks in
  // QuicDispatcher. Only sent from server.
  QUIC_HANDSHAKE_FAILED_INVALID_CONNECTION = 218,
  // Handshake failed because
  // 1) There used to be a QuicConnection created for the connection ID. And
  // 2) When the QuicConnection was destroyed, it did not have a termination
  //    packet so the QuicDispatcher synthesized one using this error code.
  // Only sent from server.
  QUIC_HANDSHAKE_FAILED_SYNTHETIC_CONNECTION_CLOSE = 219,
  // Handshake failed because there is a CID collision. Only sent from server.
  QUIC_HANDSHAKE_FAILED_CID_COLLISION = 220,
  // Handshake message contained out of order tags.
  QUIC_CRYPTO_TAGS_OUT_OF_ORDER = 29,
  // Handshake message contained too many entries.
  QUIC_CRYPTO_TOO_MANY_ENTRIES = 30,
  // Handshake message contained an invalid value length.
  QUIC_CRYPTO_INVALID_VALUE_LENGTH = 31,
  // A crypto message was received after the handshake was complete.
  QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE = 32,
  // A crypto message was received with an illegal message tag.
  QUIC_INVALID_CRYPTO_MESSAGE_TYPE = 33,
  // A crypto message was received with an illegal parameter.
  QUIC_INVALID_CRYPTO_MESSAGE_PARAMETER = 34,
  // An invalid channel id signature was supplied.
  QUIC_INVALID_CHANNEL_ID_SIGNATURE = 52,
  // A crypto message was received with a mandatory parameter missing.
  QUIC_CRYPTO_MESSAGE_PARAMETER_NOT_FOUND = 35,
  // A crypto message was received with a parameter that has no overlap
  // with the local parameter.
  QUIC_CRYPTO_MESSAGE_PARAMETER_NO_OVERLAP = 36,
  // A crypto message was received that contained a parameter with too few
  // values.
  QUIC_CRYPTO_MESSAGE_INDEX_NOT_FOUND = 37,
  // A demand for an unsupport proof type was received.
  QUIC_UNSUPPORTED_PROOF_DEMAND = 94,
  // An internal error occurred in crypto processing.
  QUIC_CRYPTO_INTERNAL_ERROR = 38,
  // A crypto handshake message specified an unsupported version.
  QUIC_CRYPTO_VERSION_NOT_SUPPORTED = 39,
  // (Deprecated) A crypto handshake message resulted in a stateless reject.
  // QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT = 72,
  // There was no intersection between the crypto primitives supported by the
  // peer and ourselves.
  QUIC_CRYPTO_NO_SUPPORT = 40,
  // The server rejected our client hello messages too many times.
  QUIC_CRYPTO_TOO_MANY_REJECTS = 41,
  // The client rejected the server's certificate chain or signature.
  QUIC_PROOF_INVALID = 42,
  // A crypto message was received with a duplicate tag.
  QUIC_CRYPTO_DUPLICATE_TAG = 43,
  // A crypto message was received with the wrong encryption level (i.e. it
  // should have been encrypted but was not.)
  QUIC_CRYPTO_ENCRYPTION_LEVEL_INCORRECT = 44,
  // The server config for a server has expired.
  QUIC_CRYPTO_SERVER_CONFIG_EXPIRED = 45,
  // We failed to setup the symmetric keys for a connection.
  QUIC_CRYPTO_SYMMETRIC_KEY_SETUP_FAILED = 53,
  // A handshake message arrived, but we are still validating the
  // previous handshake message.
  QUIC_CRYPTO_MESSAGE_WHILE_VALIDATING_CLIENT_HELLO = 54,
  // A server config update arrived before the handshake is complete.
  QUIC_CRYPTO_UPDATE_BEFORE_HANDSHAKE_COMPLETE = 65,
  // CHLO cannot fit in one packet.
  QUIC_CRYPTO_CHLO_TOO_LARGE = 90,
  // This connection involved a version negotiation which appears to have been
  // tampered with.
  QUIC_VERSION_NEGOTIATION_MISMATCH = 55,

  // Multipath errors.
  // Multipath is not enabled, but a packet with multipath flag on is received.
  QUIC_BAD_MULTIPATH_FLAG = 79,
  // A path is supposed to exist but does not.
  QUIC_MULTIPATH_PATH_DOES_NOT_EXIST = 91,
  // A path is supposed to be active but is not.
  QUIC_MULTIPATH_PATH_NOT_ACTIVE = 92,

  // IP address changed causing connection close.
  QUIC_IP_ADDRESS_CHANGED = 80,

  // Connection migration errors.
  // Network changed, but connection had no migratable streams.
  QUIC_CONNECTION_MIGRATION_NO_MIGRATABLE_STREAMS = 81,
  // Connection changed networks too many times.
  QUIC_CONNECTION_MIGRATION_TOO_MANY_CHANGES = 82,
  // Connection migration was attempted, but there was no new network to
  // migrate to.
  QUIC_CONNECTION_MIGRATION_NO_NEW_NETWORK = 83,
  // Network changed, but connection had one or more non-migratable streams.
  QUIC_CONNECTION_MIGRATION_NON_MIGRATABLE_STREAM = 84,
  // Network changed, but connection migration was disabled by config.
  QUIC_CONNECTION_MIGRATION_DISABLED_BY_CONFIG = 99,
  // Network changed, but error was encountered on the alternative network.
  QUIC_CONNECTION_MIGRATION_INTERNAL_ERROR = 100,
  // Network changed, but handshake is not confirmed yet.
  QUIC_CONNECTION_MIGRATION_HANDSHAKE_UNCONFIRMED = 111,
  QUIC_PEER_PORT_CHANGE_HANDSHAKE_UNCONFIRMED = 194,

  // Stream frames arrived too discontiguously so that stream sequencer buffer
  // maintains too many intervals.
  QUIC_TOO_MANY_STREAM_DATA_INTERVALS = 93,

  // Sequencer buffer get into weird state where continuing read/write will lead
  // to crash.
  QUIC_STREAM_SEQUENCER_INVALID_STATE = 95,

  // Connection closed because of server hits max number of sessions allowed.
  QUIC_TOO_MANY_SESSIONS_ON_SERVER = 96,

  // Receive a RST_STREAM with offset larger than kMaxStreamLength.
  QUIC_STREAM_LENGTH_OVERFLOW = 98,
  // Received a MAX DATA frame with errors.
  QUIC_INVALID_MAX_DATA_FRAME_DATA = 102,
  // Received a MAX STREAM DATA frame with errors.
  QUIC_INVALID_MAX_STREAM_DATA_FRAME_DATA = 103,
  // Received a MAX_STREAMS frame with bad data
  QUIC_MAX_STREAMS_DATA = 104,
  // Received a STREAMS_BLOCKED frame with bad data
  QUIC_STREAMS_BLOCKED_DATA = 105,
  // Error deframing a STREAM BLOCKED frame.
  QUIC_INVALID_STREAM_BLOCKED_DATA = 106,
  // NEW CONNECTION ID frame data is malformed.
  QUIC_INVALID_NEW_CONNECTION_ID_DATA = 107,
  // More connection IDs than allowed are issued.
  QUIC_CONNECTION_ID_LIMIT_ERROR = 203,
  // The peer retires connection IDs too quickly.
  QUIC_TOO_MANY_CONNECTION_ID_WAITING_TO_RETIRE = 204,
  // Received a MAX STREAM DATA frame with errors.
  QUIC_INVALID_STOP_SENDING_FRAME_DATA = 108,
  // Error deframing PATH CHALLENGE or PATH RESPONSE frames.
  QUIC_INVALID_PATH_CHALLENGE_DATA = 109,
  QUIC_INVALID_PATH_RESPONSE_DATA = 110,
  // This is used to indicate an IETF QUIC PROTOCOL VIOLATION
  // transport error within Google (pre-v99) QUIC.
  IETF_QUIC_PROTOCOL_VIOLATION = 113,
  QUIC_INVALID_NEW_TOKEN = 114,

  // Received stream data on a WRITE_UNIDIRECTIONAL stream.
  QUIC_DATA_RECEIVED_ON_WRITE_UNIDIRECTIONAL_STREAM = 115,
  // Try to send stream data on a READ_UNIDIRECTIONAL stream.
  QUIC_TRY_TO_WRITE_DATA_ON_READ_UNIDIRECTIONAL_STREAM = 116,

  // RETIRE CONNECTION ID frame data is malformed.
  QUIC_INVALID_RETIRE_CONNECTION_ID_DATA = 117,

  // Error in a received STREAMS BLOCKED frame.
  QUIC_STREAMS_BLOCKED_ERROR = 118,
  // Error in a received MAX STREAMS frame
  QUIC_MAX_STREAMS_ERROR = 119,
  // Error in Http decoder
  QUIC_HTTP_DECODER_ERROR = 120,
  // Connection from stale host needs to be cancelled.
  QUIC_STALE_CONNECTION_CANCELLED = 121,

  // A pseudo error, used as an extended error reason code in the error_details
  // of IETF-QUIC CONNECTION_CLOSE frames. It is used in
  // OnConnectionClosed upcalls to indicate that extended error information was
  // not available in a received CONNECTION_CLOSE frame.
  QUIC_IETF_GQUIC_ERROR_MISSING = 122,

  // Received WindowUpdate on a READ_UNIDIRECTIONAL stream.
  QUIC_WINDOW_UPDATE_RECEIVED_ON_READ_UNIDIRECTIONAL_STREAM = 123,

  // There are too many buffered control frames in control frame manager.
  QUIC_TOO_MANY_BUFFERED_CONTROL_FRAMES = 124,

  // QuicTransport received invalid client indication.
  QUIC_TRANSPORT_INVALID_CLIENT_INDICATION = 125,

  // Internal error codes for QPACK errors.
  QUIC_QPACK_DECOMPRESSION_FAILED = 126,

  // Obsolete generic QPACK encoder and decoder stream error codes.
  QUIC_QPACK_ENCODER_STREAM_ERROR = 127,
  QUIC_QPACK_DECODER_STREAM_ERROR = 128,

  // QPACK encoder stream errors.

  // Variable integer exceeding 2^64-1 received.
  QUIC_QPACK_ENCODER_STREAM_INTEGER_TOO_LARGE = 174,
  // String literal exceeding kStringLiteralLengthLimit in length received.
  QUIC_QPACK_ENCODER_STREAM_STRING_LITERAL_TOO_LONG = 175,
  // String literal with invalid Huffman encoding received.
  QUIC_QPACK_ENCODER_STREAM_HUFFMAN_ENCODING_ERROR = 176,
  // Invalid static table index in Insert With Name Reference instruction.
  QUIC_QPACK_ENCODER_STREAM_INVALID_STATIC_ENTRY = 177,
  // Error inserting entry with static name reference in Insert With Name
  // Reference instruction due to entry size exceeding dynamic table capacity.
  QUIC_QPACK_ENCODER_STREAM_ERROR_INSERTING_STATIC = 178,
  // Invalid relative index in Insert With Name Reference instruction.
  QUIC_QPACK_ENCODER_STREAM_INSERTION_INVALID_RELATIVE_INDEX = 179,
  // Dynamic entry not found in Insert With Name Reference instruction.
  QUIC_QPACK_ENCODER_STREAM_INSERTION_DYNAMIC_ENTRY_NOT_FOUND = 180,
  // Error inserting entry with dynamic name reference in Insert With Name
  // Reference instruction due to entry size exceeding dynamic table capacity.
  QUIC_QPACK_ENCODER_STREAM_ERROR_INSERTING_DYNAMIC = 181,
  // Error inserting entry in Insert With Literal Name instruction due to entry
  // size exceeding dynamic table capacity.
  QUIC_QPACK_ENCODER_STREAM_ERROR_INSERTING_LITERAL = 182,
  // Invalid relative index in Duplicate instruction.
  QUIC_QPACK_ENCODER_STREAM_DUPLICATE_INVALID_RELATIVE_INDEX = 183,
  // Dynamic entry not found in Duplicate instruction.
  QUIC_QPACK_ENCODER_STREAM_DUPLICATE_DYNAMIC_ENTRY_NOT_FOUND = 184,
  // Error in Set Dynamic Table Capacity instruction due to new capacity
  // exceeding maximum dynamic table capacity.
  QUIC_QPACK_ENCODER_STREAM_SET_DYNAMIC_TABLE_CAPACITY = 185,

  // QPACK decoder stream errors.

  // Variable integer exceeding 2^64-1 received.
  QUIC_QPACK_DECODER_STREAM_INTEGER_TOO_LARGE = 186,
  // Insert Count Increment instruction received with invalid 0 increment.
  QUIC_QPACK_DECODER_STREAM_INVALID_ZERO_INCREMENT = 187,
  // Insert Count Increment instruction causes uint64_t overflow.
  QUIC_QPACK_DECODER_STREAM_INCREMENT_OVERFLOW = 188,
  // Insert Count Increment instruction increases Known Received Count beyond
  // inserted entry cound.
  QUIC_QPACK_DECODER_STREAM_IMPOSSIBLE_INSERT_COUNT = 189,
  // Header Acknowledgement received for stream that has no outstanding header
  // blocks.
  QUIC_QPACK_DECODER_STREAM_INCORRECT_ACKNOWLEDGEMENT = 190,

  // Received stream data beyond close offset.
  QUIC_STREAM_DATA_BEYOND_CLOSE_OFFSET = 129,

  // Received multiple close offset.
  QUIC_STREAM_MULTIPLE_OFFSET = 130,

  // HTTP/3 errors.

  // Frame payload larger than what HttpDecoder is willing to buffer.
  QUIC_HTTP_FRAME_TOO_LARGE = 131,
  // Malformed HTTP/3 frame, or PUSH_PROMISE or CANCEL_PUSH received (which is
  // an error because MAX_PUSH_ID is never sent).
  QUIC_HTTP_FRAME_ERROR = 132,
  // A frame that is never allowed on a request stream is received.
  QUIC_HTTP_FRAME_UNEXPECTED_ON_SPDY_STREAM = 133,
  // A frame that is never allowed on the control stream is received.
  QUIC_HTTP_FRAME_UNEXPECTED_ON_CONTROL_STREAM = 134,
  // An invalid sequence of frames normally allowed on a request stream is
  // received.
  QUIC_HTTP_INVALID_FRAME_SEQUENCE_ON_SPDY_STREAM = 151,
  // A second SETTINGS frame is received on the control stream.
  QUIC_HTTP_INVALID_FRAME_SEQUENCE_ON_CONTROL_STREAM = 152,
  // A second instance of a unidirectional stream of a certain type is created.
  QUIC_HTTP_DUPLICATE_UNIDIRECTIONAL_STREAM = 153,
  // Client receives a server-initiated bidirectional stream.
  QUIC_HTTP_SERVER_INITIATED_BIDIRECTIONAL_STREAM = 154,
  // Server opens stream with stream ID corresponding to client-initiated
  // stream or vice versa.
  QUIC_HTTP_STREAM_WRONG_DIRECTION = 155,
  // Peer closes one of the six critical unidirectional streams (control, QPACK
  // encoder or decoder, in either direction).
  QUIC_HTTP_CLOSED_CRITICAL_STREAM = 156,
  // The first frame received on the control stream is not a SETTINGS frame.
  QUIC_HTTP_MISSING_SETTINGS_FRAME = 157,
  // The received SETTINGS frame contains duplicate setting identifiers.
  QUIC_HTTP_DUPLICATE_SETTING_IDENTIFIER = 158,
  // MAX_PUSH_ID frame received with push ID value smaller than a previously
  // received value.
  QUIC_HTTP_INVALID_MAX_PUSH_ID = 159,
  // Received unidirectional stream limit is lower than required by HTTP/3.
  QUIC_HTTP_STREAM_LIMIT_TOO_LOW = 160,
  // Received mismatched SETTINGS frame from HTTP/3 connection where early data
  // is accepted. Server violated the HTTP/3 spec.
  QUIC_HTTP_ZERO_RTT_RESUMPTION_SETTINGS_MISMATCH = 164,
  // Received mismatched SETTINGS frame from HTTP/3 connection where early data
  // is rejected. Our implementation currently doesn't support it.
  QUIC_HTTP_ZERO_RTT_REJECTION_SETTINGS_MISMATCH = 165,
  // Client received GOAWAY frame with stream ID that is not for a
  // client-initiated bidirectional stream.
  QUIC_HTTP_GOAWAY_INVALID_STREAM_ID = 166,
  // Received GOAWAY frame with ID that is greater than previously received ID.
  QUIC_HTTP_GOAWAY_ID_LARGER_THAN_PREVIOUS = 167,
  // HTTP/3 session received SETTINGS frame which contains HTTP/2 specific
  // settings.
  QUIC_HTTP_RECEIVE_SPDY_SETTING = 169,
  // HTTP/3 session received an HTTP/2 only frame.
  QUIC_HTTP_RECEIVE_SPDY_FRAME = 171,
  // HTTP/3 session received SERVER_PUSH stream, which is an error because
  // PUSH_PROMISE is not accepted.
  QUIC_HTTP_RECEIVE_SERVER_PUSH = 205,
  // HTTP/3 session received invalid SETTING value.
  QUIC_HTTP_INVALID_SETTING_VALUE = 207,

  // HPACK header block decoding errors.
  // Index varint beyond implementation limit.
  QUIC_HPACK_INDEX_VARINT_ERROR = 135,
  // Name length varint beyond implementation limit.
  QUIC_HPACK_NAME_LENGTH_VARINT_ERROR = 136,
  // Value length varint beyond implementation limit.
  QUIC_HPACK_VALUE_LENGTH_VARINT_ERROR = 137,
  // Name length exceeds buffer limit.
  QUIC_HPACK_NAME_TOO_LONG = 138,
  // Value length exceeds buffer limit.
  QUIC_HPACK_VALUE_TOO_LONG = 139,
  // Name Huffman encoding error.
  QUIC_HPACK_NAME_HUFFMAN_ERROR = 140,
  // Value Huffman encoding error.
  QUIC_HPACK_VALUE_HUFFMAN_ERROR = 141,
  // Next instruction should have been a dynamic table size update.
  QUIC_HPACK_MISSING_DYNAMIC_TABLE_SIZE_UPDATE = 142,
  // Invalid index in indexed header field representation.
  QUIC_HPACK_INVALID_INDEX = 143,
  // Invalid index in literal header field with indexed name representation.
  QUIC_HPACK_INVALID_NAME_INDEX = 144,
  // Dynamic table size update not allowed.
  QUIC_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_NOT_ALLOWED = 145,
  // Initial dynamic table size update is above low water mark.
  QUIC_HPACK_INITIAL_TABLE_SIZE_UPDATE_IS_ABOVE_LOW_WATER_MARK = 146,
  // Dynamic table size update is above acknowledged setting.
  QUIC_HPACK_TABLE_SIZE_UPDATE_IS_ABOVE_ACKNOWLEDGED_SETTING = 147,
  // HPACK block ends in the middle of an instruction.
  QUIC_HPACK_TRUNCATED_BLOCK = 148,
  // Incoming data fragment exceeds buffer limit.
  QUIC_HPACK_FRAGMENT_TOO_LONG = 149,
  // Total compressed HPACK data size exceeds limit.
  QUIC_HPACK_COMPRESSED_HEADER_SIZE_EXCEEDS_LIMIT = 150,

  // Stream/flow control limit from 1-RTT handshake is too low to retransmit
  // 0-RTT data. This is our implentation error. We could in theory keep the
  // connection alive but chose not to for simplicity.
  QUIC_ZERO_RTT_UNRETRANSMITTABLE = 161,
  // Stream/flow control limit from 0-RTT rejection reduces cached limit.
  // This is our implentation error. We could in theory keep the connection
  // alive but chose not to for simplicity.
  QUIC_ZERO_RTT_REJECTION_LIMIT_REDUCED = 162,
  // Stream/flow control limit from 0-RTT resumption reduces cached limit.
  // This is the peer violating QUIC spec.
  QUIC_ZERO_RTT_RESUMPTION_LIMIT_REDUCED = 163,

  // The connection silently timed out due to no network activity.
  QUIC_SILENT_IDLE_TIMEOUT = 168,
  // Connection idle timeout detected when about to send a new request.
  QUIC_CLIENT_REQUEST_IDLE_TIMEOUT = 222,

  // Try to write data without the right write keys.
  QUIC_MISSING_WRITE_KEYS = 170,

  // An endpoint detected errors in performing key updates.
  QUIC_KEY_UPDATE_ERROR = 172,

  // An endpoint has reached the confidentiality or integrity limit for the
  // AEAD algorithm used by the given connection.
  QUIC_AEAD_LIMIT_REACHED = 173,

  // Connection reached maximum age (regardless of activity), no new requests
  // are accepted.  This error code is sent in transport layer GOAWAY frame when
  // using gQUIC, and only used internally when using HTTP/3.  Active requests
  // are still served, after which connection will be closed due to idle
  // timeout.
  QUIC_MAX_AGE_TIMEOUT = 191,

  // Decrypted a 0-RTT packet with a higher packet number than a 1-RTT packet.
  QUIC_INVALID_0RTT_PACKET_NUMBER_OUT_OF_ORDER = 192,

  // Received PRIORITY_UPDATE frame with invalid payload.
  QUIC_INVALID_PRIORITY_UPDATE = 193,

  // Maps to specific errors from the CRYPTO_ERROR range from
  // https://quicwg.org/base-drafts/draft-ietf-quic-transport.html#name-transport-error-codes
  // This attempts to choose a subset of the most interesting errors rather
  // than mapping every possible CRYPTO_ERROR code.
  QUIC_TLS_BAD_CERTIFICATE = 195,
  QUIC_TLS_UNSUPPORTED_CERTIFICATE = 196,
  QUIC_TLS_CERTIFICATE_REVOKED = 197,
  QUIC_TLS_CERTIFICATE_EXPIRED = 198,
  QUIC_TLS_CERTIFICATE_UNKNOWN = 199,
  QUIC_TLS_INTERNAL_ERROR = 200,
  QUIC_TLS_UNRECOGNIZED_NAME = 201,
  QUIC_TLS_CERTIFICATE_REQUIRED = 202,

  // An HTTP field value containing an invalid character has been received.
  QUIC_INVALID_CHARACTER_IN_FIELD_VALUE = 206,

  // Error code related to the usage of TLS keying material export.
  QUIC_TLS_UNEXPECTED_KEYING_MATERIAL_EXPORT_LABEL = 208,
  QUIC_TLS_KEYING_MATERIAL_EXPORTS_MISMATCH = 209,
  QUIC_TLS_KEYING_MATERIAL_EXPORT_NOT_AVAILABLE = 210,
  QUIC_UNEXPECTED_DATA_BEFORE_ENCRYPTION_ESTABLISHED = 211,

  // Error code related to backend health-check.
  QUIC_SERVER_UNHEALTHY = 213,

  // Client application lost network access.
  QUIC_CLIENT_LOST_NETWORK_ACCESS = 215,

  // No error. Used as bound while iterating.
  QUIC_LAST_ERROR = 223,
};
#define RETURN_STRING_LITERAL(x) \
  case x:                        \
    return #x;
const char* QuicErrorCodeToString(QuicErrorCode error) {
  switch (error) {
    RETURN_STRING_LITERAL(QUIC_NO_ERROR);
    RETURN_STRING_LITERAL(QUIC_INTERNAL_ERROR);
    RETURN_STRING_LITERAL(QUIC_STREAM_DATA_AFTER_TERMINATION);
    RETURN_STRING_LITERAL(QUIC_INVALID_PACKET_HEADER);
    RETURN_STRING_LITERAL(QUIC_INVALID_FRAME_DATA);
    RETURN_STRING_LITERAL(QUIC_MISSING_PAYLOAD);
    RETURN_STRING_LITERAL(QUIC_INVALID_FEC_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_OVERLAPPING_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_UNENCRYPTED_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_RST_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_CONNECTION_CLOSE_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_GOAWAY_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_WINDOW_UPDATE_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_BLOCKED_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_STOP_WAITING_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_PATH_CLOSE_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_ACK_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_VERSION_NEGOTIATION_PACKET);
    RETURN_STRING_LITERAL(QUIC_INVALID_PUBLIC_RST_PACKET);
    RETURN_STRING_LITERAL(QUIC_DECRYPTION_FAILURE);
    RETURN_STRING_LITERAL(QUIC_ENCRYPTION_FAILURE);
    RETURN_STRING_LITERAL(QUIC_PACKET_TOO_LARGE);
    RETURN_STRING_LITERAL(QUIC_PEER_GOING_AWAY);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED_PACKETS_BUFFERED_TOO_LONG);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED_INVALID_HOSTNAME);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED_REJECTING_ALL_CONNECTIONS);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED_INVALID_CONNECTION);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED_SYNTHETIC_CONNECTION_CLOSE);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_FAILED_CID_COLLISION);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_TAGS_OUT_OF_ORDER);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_TOO_MANY_ENTRIES);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_TOO_MANY_REJECTS);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_INVALID_VALUE_LENGTH)
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_INTERNAL_ERROR);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_VERSION_NOT_SUPPORTED);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_NO_SUPPORT);
    RETURN_STRING_LITERAL(QUIC_INVALID_CRYPTO_MESSAGE_TYPE);
    RETURN_STRING_LITERAL(QUIC_INVALID_CRYPTO_MESSAGE_PARAMETER);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_PARAMETER_NOT_FOUND);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_PARAMETER_NO_OVERLAP);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_INDEX_NOT_FOUND);
    RETURN_STRING_LITERAL(QUIC_UNSUPPORTED_PROOF_DEMAND);
    RETURN_STRING_LITERAL(QUIC_INVALID_STREAM_ID);
    RETURN_STRING_LITERAL(QUIC_INVALID_PRIORITY);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_OPEN_STREAMS);
    RETURN_STRING_LITERAL(QUIC_PUBLIC_RESET);
    RETURN_STRING_LITERAL(QUIC_INVALID_VERSION);
    RETURN_STRING_LITERAL(QUIC_PACKET_WRONG_VERSION);
    RETURN_STRING_LITERAL(QUIC_INVALID_0RTT_PACKET_NUMBER_OUT_OF_ORDER);
    RETURN_STRING_LITERAL(QUIC_INVALID_HEADER_ID);
    RETURN_STRING_LITERAL(QUIC_INVALID_NEGOTIATED_VALUE);
    RETURN_STRING_LITERAL(QUIC_DECOMPRESSION_FAILURE);
    RETURN_STRING_LITERAL(QUIC_NETWORK_IDLE_TIMEOUT);
    RETURN_STRING_LITERAL(QUIC_HANDSHAKE_TIMEOUT);
    RETURN_STRING_LITERAL(QUIC_ERROR_MIGRATING_ADDRESS);
    RETURN_STRING_LITERAL(QUIC_ERROR_MIGRATING_PORT);
    RETURN_STRING_LITERAL(QUIC_PACKET_WRITE_ERROR);
    RETURN_STRING_LITERAL(QUIC_PACKET_READ_ERROR);
    RETURN_STRING_LITERAL(QUIC_EMPTY_STREAM_FRAME_NO_FIN);
    RETURN_STRING_LITERAL(QUIC_INVALID_HEADERS_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_HEADERS_STREAM_DATA_DECOMPRESS_FAILURE);
    RETURN_STRING_LITERAL(QUIC_FLOW_CONTROL_RECEIVED_TOO_MUCH_DATA);
    RETURN_STRING_LITERAL(QUIC_FLOW_CONTROL_SENT_TOO_MUCH_DATA);
    RETURN_STRING_LITERAL(QUIC_FLOW_CONTROL_INVALID_WINDOW);
    RETURN_STRING_LITERAL(QUIC_FLOW_CONTROL_FINAL_SIZE_CHANGED);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_IP_POOLED);
    RETURN_STRING_LITERAL(QUIC_PROOF_INVALID);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_DUPLICATE_TAG);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_ENCRYPTION_LEVEL_INCORRECT);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_SERVER_CONFIG_EXPIRED);
    RETURN_STRING_LITERAL(QUIC_INVALID_CHANNEL_ID_SIGNATURE);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_SYMMETRIC_KEY_SETUP_FAILED);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_WHILE_VALIDATING_CLIENT_HELLO);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_UPDATE_BEFORE_HANDSHAKE_COMPLETE);
    RETURN_STRING_LITERAL(QUIC_VERSION_NEGOTIATION_MISMATCH);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_OUTSTANDING_SENT_PACKETS);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_OUTSTANDING_RECEIVED_PACKETS);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_CANCELLED);
    RETURN_STRING_LITERAL(QUIC_BAD_PACKET_LOSS_RATE);
    RETURN_STRING_LITERAL(QUIC_PUBLIC_RESETS_POST_HANDSHAKE);
    RETURN_STRING_LITERAL(QUIC_FAILED_TO_SERIALIZE_PACKET);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_AVAILABLE_STREAMS);
    RETURN_STRING_LITERAL(QUIC_UNENCRYPTED_FEC_DATA);
    RETURN_STRING_LITERAL(QUIC_BAD_MULTIPATH_FLAG);
    RETURN_STRING_LITERAL(QUIC_IP_ADDRESS_CHANGED);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_NO_MIGRATABLE_STREAMS);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_TOO_MANY_CHANGES);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_NO_NEW_NETWORK);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_NON_MIGRATABLE_STREAM);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_RTOS);
    RETURN_STRING_LITERAL(QUIC_ATTEMPT_TO_SEND_UNENCRYPTED_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_MAYBE_CORRUPTED_MEMORY);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_CHLO_TOO_LARGE);
    RETURN_STRING_LITERAL(QUIC_MULTIPATH_PATH_DOES_NOT_EXIST);
    RETURN_STRING_LITERAL(QUIC_MULTIPATH_PATH_NOT_ACTIVE);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_STREAM_DATA_INTERVALS);
    RETURN_STRING_LITERAL(QUIC_STREAM_SEQUENCER_INVALID_STATE);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_SESSIONS_ON_SERVER);
    RETURN_STRING_LITERAL(QUIC_STREAM_LENGTH_OVERFLOW);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_DISABLED_BY_CONFIG);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_INTERNAL_ERROR);
    RETURN_STRING_LITERAL(QUIC_INVALID_MAX_DATA_FRAME_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_MAX_STREAM_DATA_FRAME_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_STREAM_BLOCKED_DATA);
    RETURN_STRING_LITERAL(QUIC_MAX_STREAMS_DATA);
    RETURN_STRING_LITERAL(QUIC_STREAMS_BLOCKED_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_NEW_CONNECTION_ID_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_RETIRE_CONNECTION_ID_DATA);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_ID_LIMIT_ERROR);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_CONNECTION_ID_WAITING_TO_RETIRE);
    RETURN_STRING_LITERAL(QUIC_INVALID_STOP_SENDING_FRAME_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_PATH_CHALLENGE_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_PATH_RESPONSE_DATA);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_MIGRATION_HANDSHAKE_UNCONFIRMED);
    RETURN_STRING_LITERAL(QUIC_PEER_PORT_CHANGE_HANDSHAKE_UNCONFIRMED);
    RETURN_STRING_LITERAL(QUIC_INVALID_DATAGRAM_DATA);
    RETURN_STRING_LITERAL(IETF_QUIC_PROTOCOL_VIOLATION);
    RETURN_STRING_LITERAL(QUIC_INVALID_NEW_TOKEN);
    RETURN_STRING_LITERAL(QUIC_DATA_RECEIVED_ON_WRITE_UNIDIRECTIONAL_STREAM);
    RETURN_STRING_LITERAL(QUIC_TRY_TO_WRITE_DATA_ON_READ_UNIDIRECTIONAL_STREAM);
    RETURN_STRING_LITERAL(QUIC_STREAMS_BLOCKED_ERROR);
    RETURN_STRING_LITERAL(QUIC_MAX_STREAMS_ERROR);
    RETURN_STRING_LITERAL(QUIC_HTTP_DECODER_ERROR);
    RETURN_STRING_LITERAL(QUIC_STALE_CONNECTION_CANCELLED);
    RETURN_STRING_LITERAL(QUIC_IETF_GQUIC_ERROR_MISSING);
    RETURN_STRING_LITERAL(
        QUIC_WINDOW_UPDATE_RECEIVED_ON_READ_UNIDIRECTIONAL_STREAM);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_BUFFERED_CONTROL_FRAMES);
    RETURN_STRING_LITERAL(QUIC_TRANSPORT_INVALID_CLIENT_INDICATION);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECOMPRESSION_FAILED);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_ERROR);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECODER_STREAM_ERROR);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_INTEGER_TOO_LARGE);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_STRING_LITERAL_TOO_LONG);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_HUFFMAN_ENCODING_ERROR);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_INVALID_STATIC_ENTRY);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_ERROR_INSERTING_STATIC);
    RETURN_STRING_LITERAL(
        QUIC_QPACK_ENCODER_STREAM_INSERTION_INVALID_RELATIVE_INDEX);
    RETURN_STRING_LITERAL(
        QUIC_QPACK_ENCODER_STREAM_INSERTION_DYNAMIC_ENTRY_NOT_FOUND);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_ERROR_INSERTING_DYNAMIC);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_ERROR_INSERTING_LITERAL);
    RETURN_STRING_LITERAL(
        QUIC_QPACK_ENCODER_STREAM_DUPLICATE_INVALID_RELATIVE_INDEX);
    RETURN_STRING_LITERAL(
        QUIC_QPACK_ENCODER_STREAM_DUPLICATE_DYNAMIC_ENTRY_NOT_FOUND);
    RETURN_STRING_LITERAL(QUIC_QPACK_ENCODER_STREAM_SET_DYNAMIC_TABLE_CAPACITY);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECODER_STREAM_INTEGER_TOO_LARGE);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECODER_STREAM_INVALID_ZERO_INCREMENT);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECODER_STREAM_INCREMENT_OVERFLOW);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECODER_STREAM_IMPOSSIBLE_INSERT_COUNT);
    RETURN_STRING_LITERAL(QUIC_QPACK_DECODER_STREAM_INCORRECT_ACKNOWLEDGEMENT);
    RETURN_STRING_LITERAL(QUIC_STREAM_DATA_BEYOND_CLOSE_OFFSET);
    RETURN_STRING_LITERAL(QUIC_STREAM_MULTIPLE_OFFSET);
    RETURN_STRING_LITERAL(QUIC_HTTP_FRAME_TOO_LARGE);
    RETURN_STRING_LITERAL(QUIC_HTTP_FRAME_ERROR);
    RETURN_STRING_LITERAL(QUIC_HTTP_FRAME_UNEXPECTED_ON_SPDY_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_FRAME_UNEXPECTED_ON_CONTROL_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_INVALID_FRAME_SEQUENCE_ON_SPDY_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_INVALID_FRAME_SEQUENCE_ON_CONTROL_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_DUPLICATE_UNIDIRECTIONAL_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_SERVER_INITIATED_BIDIRECTIONAL_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_STREAM_WRONG_DIRECTION);
    RETURN_STRING_LITERAL(QUIC_HTTP_CLOSED_CRITICAL_STREAM);
    RETURN_STRING_LITERAL(QUIC_HTTP_MISSING_SETTINGS_FRAME);
    RETURN_STRING_LITERAL(QUIC_HTTP_DUPLICATE_SETTING_IDENTIFIER);
    RETURN_STRING_LITERAL(QUIC_HTTP_INVALID_MAX_PUSH_ID);
    RETURN_STRING_LITERAL(QUIC_HTTP_STREAM_LIMIT_TOO_LOW);
    RETURN_STRING_LITERAL(QUIC_HTTP_ZERO_RTT_RESUMPTION_SETTINGS_MISMATCH);
    RETURN_STRING_LITERAL(QUIC_HTTP_ZERO_RTT_REJECTION_SETTINGS_MISMATCH);
    RETURN_STRING_LITERAL(QUIC_HTTP_GOAWAY_INVALID_STREAM_ID);
    RETURN_STRING_LITERAL(QUIC_HTTP_GOAWAY_ID_LARGER_THAN_PREVIOUS);
    RETURN_STRING_LITERAL(QUIC_HTTP_RECEIVE_SPDY_SETTING);
    RETURN_STRING_LITERAL(QUIC_HTTP_RECEIVE_SPDY_FRAME);
    RETURN_STRING_LITERAL(QUIC_HTTP_RECEIVE_SERVER_PUSH);
    RETURN_STRING_LITERAL(QUIC_HTTP_INVALID_SETTING_VALUE);
    RETURN_STRING_LITERAL(QUIC_HPACK_INDEX_VARINT_ERROR);
    RETURN_STRING_LITERAL(QUIC_HPACK_NAME_LENGTH_VARINT_ERROR);
    RETURN_STRING_LITERAL(QUIC_HPACK_VALUE_LENGTH_VARINT_ERROR);
    RETURN_STRING_LITERAL(QUIC_HPACK_NAME_TOO_LONG);
    RETURN_STRING_LITERAL(QUIC_HPACK_VALUE_TOO_LONG);
    RETURN_STRING_LITERAL(QUIC_HPACK_NAME_HUFFMAN_ERROR);
    RETURN_STRING_LITERAL(QUIC_HPACK_VALUE_HUFFMAN_ERROR);
    RETURN_STRING_LITERAL(QUIC_HPACK_MISSING_DYNAMIC_TABLE_SIZE_UPDATE);
    RETURN_STRING_LITERAL(QUIC_HPACK_INVALID_INDEX);
    RETURN_STRING_LITERAL(QUIC_HPACK_INVALID_NAME_INDEX);
    RETURN_STRING_LITERAL(QUIC_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_NOT_ALLOWED);
    RETURN_STRING_LITERAL(
        QUIC_HPACK_INITIAL_TABLE_SIZE_UPDATE_IS_ABOVE_LOW_WATER_MARK);
    RETURN_STRING_LITERAL(
        QUIC_HPACK_TABLE_SIZE_UPDATE_IS_ABOVE_ACKNOWLEDGED_SETTING);
    RETURN_STRING_LITERAL(QUIC_HPACK_TRUNCATED_BLOCK);
    RETURN_STRING_LITERAL(QUIC_HPACK_FRAGMENT_TOO_LONG);
    RETURN_STRING_LITERAL(QUIC_HPACK_COMPRESSED_HEADER_SIZE_EXCEEDS_LIMIT);
    RETURN_STRING_LITERAL(QUIC_ZERO_RTT_UNRETRANSMITTABLE);
    RETURN_STRING_LITERAL(QUIC_ZERO_RTT_REJECTION_LIMIT_REDUCED);
    RETURN_STRING_LITERAL(QUIC_ZERO_RTT_RESUMPTION_LIMIT_REDUCED);
    RETURN_STRING_LITERAL(QUIC_SILENT_IDLE_TIMEOUT);
    RETURN_STRING_LITERAL(QUIC_CLIENT_REQUEST_IDLE_TIMEOUT);
    RETURN_STRING_LITERAL(QUIC_MISSING_WRITE_KEYS);
    RETURN_STRING_LITERAL(QUIC_KEY_UPDATE_ERROR);
    RETURN_STRING_LITERAL(QUIC_AEAD_LIMIT_REACHED);
    RETURN_STRING_LITERAL(QUIC_MAX_AGE_TIMEOUT);
    RETURN_STRING_LITERAL(QUIC_INVALID_PRIORITY_UPDATE);
    RETURN_STRING_LITERAL(QUIC_TLS_BAD_CERTIFICATE);
    RETURN_STRING_LITERAL(QUIC_TLS_UNSUPPORTED_CERTIFICATE);
    RETURN_STRING_LITERAL(QUIC_TLS_CERTIFICATE_REVOKED);
    RETURN_STRING_LITERAL(QUIC_TLS_CERTIFICATE_EXPIRED);
    RETURN_STRING_LITERAL(QUIC_TLS_CERTIFICATE_UNKNOWN);
    RETURN_STRING_LITERAL(QUIC_TLS_INTERNAL_ERROR);
    RETURN_STRING_LITERAL(QUIC_TLS_UNRECOGNIZED_NAME);
    RETURN_STRING_LITERAL(QUIC_TLS_CERTIFICATE_REQUIRED);
    RETURN_STRING_LITERAL(QUIC_INVALID_CHARACTER_IN_FIELD_VALUE);
    RETURN_STRING_LITERAL(QUIC_TLS_UNEXPECTED_KEYING_MATERIAL_EXPORT_LABEL);
    RETURN_STRING_LITERAL(QUIC_TLS_KEYING_MATERIAL_EXPORTS_MISMATCH);
    RETURN_STRING_LITERAL(QUIC_TLS_KEYING_MATERIAL_EXPORT_NOT_AVAILABLE);
    RETURN_STRING_LITERAL(QUIC_UNEXPECTED_DATA_BEFORE_ENCRYPTION_ESTABLISHED);
    RETURN_STRING_LITERAL(QUIC_SERVER_UNHEALTHY);
    RETURN_STRING_LITERAL(QUIC_CLIENT_LOST_NETWORK_ACCESS);

    RETURN_STRING_LITERAL(QUIC_LAST_ERROR);
    // Intentionally have no default case, so we'll break the build
    // if we add errors and don't put them here.
  }
  // Return a default value so that we return this when |error| doesn't match
  // any of the QuicErrorCodes. This can happen when the ConnectionClose
  // frame sent by the peer (attacker) has invalid error code.
  return "INVALID_ERROR_CODE";
}
#undef RETURN_STRING_LITERAL
}  // namespace net_error_rust_frozen_quic

namespace net_error_rust_reference {

// Validate all error values in net_error_list.h are negative.
#define NET_ERROR(label, value) \
  static_assert(value < 0, "ERR_" #label " should be negative");
#include "net/base/net_error_rust_reference_list.h"
#undef NET_ERROR

std::string ErrorToString(int error) {
  return "net::" + ErrorToShortString(error);
}

std::string ExtendedErrorToString(int error, int extended_error_code) {
  if (error == ERR_QUIC_PROTOCOL_ERROR && extended_error_code != 0) {
    return base::StrCat(
        {"net::ERR_QUIC_PROTOCOL_ERROR.",
         net_error_rust_frozen_quic::QuicErrorCodeToString(
             static_cast<net_error_rust_frozen_quic::QuicErrorCode>(
                 extended_error_code))});
  }
  return ErrorToString(error);
}

std::string ErrorToShortString(int error) {
  if (error == OK) {
    return "OK";
  }

  const char* error_string;
  switch (error) {
#define NET_ERROR(label, value) \
  case ERR_##label:             \
    error_string = #label;      \
    break;
#include "net/base/net_error_rust_reference_list.h"
#undef NET_ERROR
    default:
      // TODO(crbug.com/40909121): Figure out why this is firing, fix and
      // upgrade this to be fatal.
      DUMP_WILL_BE_NOTREACHED() << error;
      error_string = "<unknown>";
  }
  return base::StrCat({"ERR_", error_string});
}

bool IsCertificateError(int error) {
  // Certificate errors are negative integers from net::ERR_CERT_BEGIN
  // (inclusive) to net::ERR_CERT_END (exclusive) in *decreasing* order.
  // ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN is currently an exception to this
  // rule.
  return (error <= ERR_CERT_BEGIN && error > ERR_CERT_END) ||
         (error == ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN);
}

bool IsClientCertificateError(int error) {
  switch (error) {
    case ERR_BAD_SSL_CLIENT_AUTH_CERT:
    case ERR_SSL_CLIENT_AUTH_PRIVATE_KEY_ACCESS_DENIED:
    case ERR_SSL_CLIENT_AUTH_CERT_NO_PRIVATE_KEY:
    case ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED:
    case ERR_SSL_CLIENT_AUTH_NO_COMMON_ALGORITHMS:
      return true;
    default:
      return false;
  }
}

bool IsHostnameResolutionError(int error) {
  DCHECK_NE(ERR_NAME_RESOLUTION_FAILED, error);
  return error == ERR_NAME_NOT_RESOLVED;
}

bool IsRequestBlockedError(int error) {
  switch (error) {
    case ERR_BLOCKED_BY_CLIENT:
    case ERR_BLOCKED_BY_ADMINISTRATOR:
    case ERR_BLOCKED_BY_CSP:
    case ERR_BLOCKED_IN_INCOGNITO_BY_ADMINISTRATOR:
      return true;
    default:
      return false;
  }
}

bool IsOkOrDefinedError(int error) {
  switch (error) {
    case OK:
#define NET_ERROR(label, value) case value:
#include "net/base/net_error_rust_reference_list.h"
#undef NET_ERROR
      return true;
  }
  return false;
}

Error FileErrorToNetError(base::File::Error file_error) {
  switch (file_error) {
    case base::File::FILE_OK:
      return OK;
    case base::File::FILE_ERROR_EXISTS:
      return ERR_FILE_EXISTS;
    case base::File::FILE_ERROR_NOT_FOUND:
      return ERR_FILE_NOT_FOUND;
    case base::File::FILE_ERROR_ACCESS_DENIED:
      return ERR_ACCESS_DENIED;
    case base::File::FILE_ERROR_NO_MEMORY:
      return ERR_OUT_OF_MEMORY;
    case base::File::FILE_ERROR_NO_SPACE:
      return ERR_FILE_NO_SPACE;
    case base::File::FILE_ERROR_INVALID_OPERATION:
      return ERR_INVALID_ARGUMENT;
    case base::File::FILE_ERROR_ABORT:
      return ERR_ABORTED;
    case base::File::FILE_ERROR_INVALID_URL:
      return ERR_INVALID_URL;
    case base::File::FILE_ERROR_TOO_MANY_OPENED:
      return ERR_INSUFFICIENT_RESOURCES;
    case base::File::FILE_ERROR_SECURITY:
      return ERR_ACCESS_DENIED;
    case base::File::FILE_ERROR_MAX:
      NOTREACHED();
    case base::File::FILE_ERROR_NOT_A_DIRECTORY:
    case base::File::FILE_ERROR_NOT_A_FILE:
    case base::File::FILE_ERROR_NOT_EMPTY:
    case base::File::FILE_ERROR_IO:
    case base::File::FILE_ERROR_IN_USE:
    // No good mappings for these, so just fallthrough to generic fail.
    case base::File::FILE_ERROR_FAILED:
      return ERR_FAILED;
  }
  NOTREACHED();
}

}  // namespace net_error_rust_reference
