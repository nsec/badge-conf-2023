/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#include "board.hpp"
#include "config.hpp"
#include "globals.hpp"
#include "network/network_handler.hpp"

#define NETWORK_VERBOSE_LOG 0

namespace ns = nsec::scheduling;
namespace nc = nsec::communication;
namespace ng = nsec::g;

namespace {
enum class wire_msg_type : uint8_t {
	// Wire protocol reserved messages
	MONITOR = 3,
	RESET = 4,
	ANNOUNCE = 5,
	ANNOUNCE_REPLY = 6,

	// Application messages (forwarded to the application layer)
	// ...
};
}

namespace logging {
#if NETWORK_VERBOSE_LOG == 1
const char left_str[] PROGMEM = "left";
const char right_str[] PROGMEM = "right";

const char unknown_str[] PROGMEM = "UNKNOWN";
const char unconnected_str[] PROGMEM = "UNCONNECTED";

// Wire protocol states
const char wait_to_send_announce_str[] PROGMEM = "WAIT_TO_SEND_ANNOUNCE";
const char discovery_str[] PROGMEM = "DISCOVERY";
const char running_str[] PROGMEM = "RUNNING";

// Message reception states
const char magic_byte_1_str[] PROGMEM = "RECEIVE_MAGIC_BYTE_1";
const char magic_byte_2_str[] PROGMEM = "RECEIVE_MAGIC_BYTE_2";
const char receive_header_str[] PROGMEM = "RECEIVE_HEADER";
const char receive_payload_str[] PROGMEM = "RECEIVE_PAYLOAD";

// Wire message types
const char monitor_str[] PROGMEM = "MONITOR";
const char reset_str[] PROGMEM = "RESET";
const char announce_str[] PROGMEM = "ANNOUNCE";
const char announce_reply_str[] PROGMEM = "ANNOUNCE_REPLY";
const char app_message_str[] PROGMEM = "APP_MESSAGE";

const __FlashStringHelper *as_flash_string(const char *str)
{
	return static_cast<const __FlashStringHelper *>(static_cast<const void *>(str));
}

void log_wire_msg_type(uint8_t type, const __FlashStringHelper *operation) noexcept
{
	const __FlashStringHelper *str;

	Serial.print(operation);
	Serial.print(F(" wire msg type: "));
	switch (wire_msg_type(type)) {
	case wire_msg_type::MONITOR:
		str = as_flash_string(monitor_str);
		break;
	case wire_msg_type::RESET:
		str = as_flash_string(reset_str);
		break;
	case wire_msg_type::ANNOUNCE:
		str = as_flash_string(announce_str);
		break;
	case wire_msg_type::ANNOUNCE_REPLY:
		str = as_flash_string(announce_reply_str);
		break;
	default:
		Serial.print(as_flash_string(app_message_str));
		Serial.print(F(" "));
		Serial.println(int(type));
		return;
	}

	Serial.println(str);
}

void log_connector_states(bool left_is_connected, bool right_is_connected) noexcept
{
	Serial.print(F("Network topology change: left ["));
	if (!left_is_connected) {
		Serial.print(F("un"));
	}

	Serial.print(F("connected"));
	Serial.print(F("], right ["));
	if (!right_is_connected) {
		Serial.print(F("un"));
	}

	Serial.print(F("connected"));
	Serial.println(F("]"));
}

void log_listening_side(nc::peer_relative_position side) noexcept
{
	Serial.print(F("Listening to peer: "));
	Serial.println(side == nc::peer_relative_position::LEFT ? as_flash_string(left_str) :
								  as_flash_string(right_str));
}

void log_network_reset() noexcept
{
	Serial.println(F("Network reset"));
}

void log_serial_overflow() noexcept
{
	Serial.println(F("Serial overflow"));
}

void log_invalid_magic_byte(uint8_t expected, uint8_t actual) noexcept
{
	Serial.print(F("Invalid magic byte: "));
	Serial.print(int(actual));
	Serial.print(F(" expected "));
	Serial.println(int(expected));
}

void log_network_timeout() noexcept
{
	Serial.println(F("Network timeout!"));
}

#else

void log_wire_msg_type(uint8_t type, const __FlashStringHelper *operation) noexcept
{
}

void log_connector_states(bool left_is_connected, bool right_is_connected) noexcept
{
}

void log_listening_side(nc::peer_relative_position side) noexcept
{
}

void log_network_reset() noexcept
{
}

void log_serial_overflow() noexcept
{
}

void log_invalid_magic_byte(uint8_t expected, uint8_t actual) noexcept
{
}

void log_network_timeout() noexcept
{
}

#endif
} /* namespace logging */

namespace {

constexpr uint8_t wire_protocol_magic_1 = 0b10101111;
constexpr uint8_t wire_protocol_magic_2 = 0b11111010;

struct wire_msg_header {
	// Message start with a 16-bit magic number to re-sync on message frames.
	uint8_t type;

	/*
	 * 16-bit fletcher checksum, see
	 * https://en.wikipedia.org/wiki/Fletcher%27s_checksum#Fletcher-16
	 *
	 * Checksum includes: type and payload bytes
	 */
	uint16_t checksum;
} __attribute__((packed));

struct wire_msg_announce {
	uint8_t peer_id;
} __attribute__((packed));

struct wire_msg_announce_reply {
	uint8_t peer_count;
} __attribute__((packed));

uint8_t wire_msg_payload_size(uint8_t type)
{
	switch (wire_msg_type(type)) {
	case wire_msg_type::ANNOUNCE:
		return sizeof(wire_msg_announce);
	case wire_msg_type::ANNOUNCE_REPLY:
		return sizeof(wire_msg_announce_reply);
	case wire_msg_type::MONITOR:
	case wire_msg_type::RESET:
		return 0;
	default:
		// TODO query app msg size from id
		return 0;
	}
}

class fletcher16_checksumer {
public:
	void push(uint8_t value)
	{
		_sum_low += value;
		_sum_high += _sum_low;
	}

	uint16_t checksum() const noexcept
	{
		return (uint16_t(_sum_high) << 8) | uint16_t(_sum_low);
	}

private:
	uint8_t _sum_low = 0;
	uint8_t _sum_high = 0;
};

void send_wire_magic(SoftwareSerial& serial) noexcept
{
	serial.write(wire_protocol_magic_1);
	serial.write(wire_protocol_magic_2);
}

void send_wire_header(SoftwareSerial& serial, uint8_t msg_type, uint16_t checksum) noexcept
{
	const wire_msg_header header = { .type = msg_type, .checksum = checksum };

	logging::log_wire_msg_type(msg_type, F("Sending"));
	send_wire_magic(serial);
	serial.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header));
}

void send_wire_msg(SoftwareSerial& serial,
		   uint8_t msg_type,
		   const uint8_t *msg = nullptr,
		   uint8_t msg_payload_size = 0) noexcept
{
	fletcher16_checksumer checksummer;

	checksummer.push(msg_type);
	for (uint8_t i = 0; i < msg_payload_size; i++) {
		checksummer.push(msg[i]);
	}

	send_wire_header(serial, msg_type, checksummer.checksum());
	serial.write(msg, msg_payload_size);
}

void send_wire_reset_msg(SoftwareSerial& serial) noexcept
{
	send_wire_msg(serial, uint8_t(wire_msg_type::RESET));
}

void send_wire_monitor_msg(SoftwareSerial& serial) noexcept
{
	send_wire_msg(serial, uint8_t(wire_msg_type::MONITOR));
}

void send_wire_announce_msg(SoftwareSerial& serial, nc::peer_id_t peer_id) noexcept
{
	const wire_msg_announce announce_msg = { .peer_id = uint8_t(peer_id) };

	send_wire_msg(serial,
		      uint8_t(wire_msg_type::ANNOUNCE),
		      reinterpret_cast<const uint8_t *>(&announce_msg),
		      sizeof(announce_msg));
}
} /* namespace */

nc::network_handler::network_handler(disconnection_notifier unconnected,
				     pairing_begin_notifier pairing_begin,
				     pairing_end_notifier pairing_end,
				     message_received_notifier message_received) noexcept :
	ns::periodic_task(nsec::config::communication::network_handler_base_period_ms),
	_notify_unconnected{ unconnected },
	_notify_pairing_begin{ pairing_begin },
	_notify_pairing_end{ pairing_end },
	_notify_message_received{ message_received },
	_left_serial(nsec::config::communication::serial_rx_pin_left,
		     nsec::config::communication::serial_tx_pin_left,
		     true),
	_right_serial(nsec::config::communication::serial_rx_pin_right,
		      nsec::config::communication::serial_tx_pin_right,
		      true),
	_is_left_connected{ false },
	_is_right_connected{ false },
	_current_wire_protocol_state{ uint8_t(wire_protocol_state::UNCONNECTED) }
{
	_reset();
	ng::the_scheduler.schedule_task(*this);
}

void nc::network_handler::setup() noexcept
{
	// Init software serial for both sides of the badge.
	pinMode(nsec::config::communication::connection_sense_pin_right, INPUT_PULLUP);
	pinMode(nsec::config::communication::connection_sense_pin_left, OUTPUT);
	digitalWrite(nsec::config::communication::connection_sense_pin_left, LOW);

	pinMode(nsec::config::communication::serial_rx_pin_left, INPUT_PULLUP);

	_left_serial.begin(nsec::config::communication::software_serial_speed);
	_right_serial.begin(nsec::config::communication::software_serial_speed);
}

bool nc::network_handler::_sense_is_left_connected() const noexcept
{
	// We hope/assume that we'll see the other side's TX pin low when idle.
	return digitalRead(nsec::config::communication::serial_rx_pin_left) == LOW;
}

bool nc::network_handler::_sense_is_right_connected() const noexcept
{
	return digitalRead(nsec::config::communication::connection_sense_pin_right) == LOW;
}

nc::network_handler::link_position nc::network_handler::_position() const noexcept
{
	return link_position(_current_position);
}

#if NETWORK_VERBOSE_LOG == 1
void nc::network_handler::_log_wire_protocol_state(wire_protocol_state state) noexcept
{
	const __FlashStringHelper *str;

	Serial.print(F("Protocol state: "));
	switch (state) {
	case wire_protocol_state::UNCONNECTED:
		str = logging::as_flash_string(logging::unconnected_str);
		break;
	case wire_protocol_state::WAIT_TO_SEND_ANNOUNCE:
		str = logging::as_flash_string(logging::wait_to_send_announce_str);
		break;
	case wire_protocol_state::DISCOVERY:
		str = logging::as_flash_string(logging::discovery_str);
		break;
	case wire_protocol_state::RUNNING:
		str = logging::as_flash_string(logging::running_str);
		break;
	}

	Serial.println(str);
}

void nc::network_handler::_log_message_reception_state(message_reception_state state) noexcept
{
	const __FlashStringHelper *str;

	Serial.print(F("Msg reception state: "));
	switch (state) {
	case message_reception_state::RECEIVE_MAGIC_BYTE_1:
		str = logging::as_flash_string(logging::magic_byte_1_str);
		break;
	case message_reception_state::RECEIVE_MAGIC_BYTE_2:
		str = logging::as_flash_string(logging::magic_byte_2_str);
		break;
	case message_reception_state::RECEIVE_HEADER:
		str = logging::as_flash_string(logging::receive_header_str);
		break;
	case message_reception_state::RECEIVE_PAYLOAD:
		str = logging::as_flash_string(logging::receive_payload_str);
		break;
	}

	Serial.println(str);
}
#else /* NETWORK_VERBOSE_LOG */
void nc::network_handler::_log_wire_protocol_state(wire_protocol_state state) noexcept
{
}
void nc::network_handler::_log_message_reception_state(message_reception_state state) noexcept
{
}
#endif

void nc::network_handler::_reset() noexcept
{
	logging::log_network_reset();

	if (_wire_protocol_state() != wire_protocol_state::UNCONNECTED) {
		send_wire_reset_msg(_listening_side_serial());
	}

	_position(link_position::UNKNOWN);
	_wire_protocol_state(wire_protocol_state::UNCONNECTED);
	_is_left_connected = false;
	_is_right_connected = false;
}

void nc::network_handler::_detect_and_set_position() noexcept
{
	const auto connection_mask = (_is_left_connected << 1) | _is_right_connected;
	const link_position new_position[] = { [0b00] = link_position::UNKNOWN,
					       [0b01] = link_position::LEFT_MOST,
					       [0b10] = link_position::RIGHT_MOST,
					       [0b11] = link_position::MIDDLE };

	_position(new_position[connection_mask]);
}

void nc::network_handler::_check_connections() noexcept
{
	const bool left_is_connected = _sense_is_left_connected();
	const bool right_is_connected = _sense_is_right_connected();
	const bool left_was_connected = _is_left_connected;
	const bool right_was_connected = _is_right_connected;
	const bool left_state_changed = left_is_connected != left_was_connected;
	const bool right_state_changed = right_is_connected != right_was_connected;
	const bool topology_changed = left_state_changed || right_state_changed;

	if (!topology_changed) {
		return;
	}

	logging::log_connector_states(left_is_connected, right_is_connected);
	_reset();

	_is_left_connected = left_is_connected;
	_is_right_connected = right_is_connected;
	_detect_and_set_position();
}

void nc::network_handler::_position(link_position new_position) noexcept
{
	_current_position = uint8_t(new_position);
	switch (_position()) {
	case link_position::UNKNOWN:
		break;
	case link_position::LEFT_MOST:
		_listening_side(peer_relative_position::RIGHT);
		// We are peer 0 (i.e. the left-most node) and will
		// initiate the discovery.
		_wire_protocol_state(wire_protocol_state::WAIT_TO_SEND_ANNOUNCE);
		_peer_id = 0;
		break;
	case link_position::RIGHT_MOST:
	case link_position::MIDDLE:
		// A peer on our left will initiate the discovery: wait for the message.
		_listening_side(peer_relative_position::LEFT);
		_wire_protocol_state(wire_protocol_state::DISCOVERY);
		break;
	}
}

nc::peer_relative_position nc::network_handler::_listening_side() const noexcept
{
	return peer_relative_position(_current_listening_side);
}

void nc::network_handler::_listening_side(peer_relative_position side) noexcept
{
	_current_listening_side = uint8_t(side);
	logging::log_listening_side(_listening_side());
	switch (_listening_side()) {
	case peer_relative_position::LEFT:
		_left_serial.listen();
		break;
	case peer_relative_position::RIGHT:
		_right_serial.listen();
		break;
	}
}

void nc::network_handler::_reverse_listening_side() noexcept
{
	_listening_side(_listening_side() == peer_relative_position::LEFT ?
				peer_relative_position::RIGHT :
				peer_relative_position::LEFT);
}

void nc::network_handler::_reverse_wave_front_direction() noexcept
{
	_wave_front_direction(_wave_front_direction() == peer_relative_position::LEFT ?
				      peer_relative_position::RIGHT :
				      peer_relative_position::LEFT);
}

nc::network_handler::wire_protocol_state nc::network_handler::_wire_protocol_state() const noexcept
{
	return wire_protocol_state(_current_wire_protocol_state);
}

void nc::network_handler::_wire_protocol_state(wire_protocol_state state) noexcept
{
	const auto previous_protocol_state = wire_protocol_state(_current_wire_protocol_state);

	_current_wire_protocol_state = uint8_t(state);
	_ticks_in_wire_state = 0;
	// Reset timeout timestamp.
	_last_monitor_message_received_time_ms = millis();

	if (previous_protocol_state != _wire_protocol_state()) {
		// Log the state change.
		_log_wire_protocol_state(_wire_protocol_state());
	}

	if (previous_protocol_state == wire_protocol_state::RUNNING &&
	    state == wire_protocol_state::UNCONNECTED) {
		_notify_unconnected();
	}

	if (state == wire_protocol_state::UNCONNECTED) {
		_current_pending_outgoing_message_size = 0;
		// We are a sad and lonely node hacking together a network protocol.
		_peer_count = 1;
		// Unknown peer id.
		_peer_id = 0;
		_wave_front_direction(peer_relative_position::RIGHT);
		_message_reception_state(message_reception_state::RECEIVE_MAGIC_BYTE_1);
		_clear_pending_outgoing_message();

		/* Empty the serial buffers. */
		while (_left_serial.available()) {
			_left_serial.read();
		}
		while (_right_serial.available()) {
			_right_serial.read();
		}
	} else if (state == wire_protocol_state::RUNNING) {
		_notify_pairing_end(_peer_id, _peer_count);
	}
}

nc::peer_relative_position nc::network_handler::_wave_front_direction() const noexcept
{
	return peer_relative_position(_current_wave_front_direction);
}

void nc::network_handler::_wave_front_direction(peer_relative_position new_direction) noexcept
{
	_current_wave_front_direction = uint8_t(new_direction);
}

nc::network_handler::message_reception_state
nc::network_handler::_message_reception_state() const noexcept
{
	return message_reception_state(_current_message_reception_state);
}

void nc::network_handler::_message_reception_state(message_reception_state new_state) noexcept
{
	_current_message_reception_state = uint8_t(new_state);
	_log_message_reception_state(new_state);
}

nc::peer_relative_position nc::network_handler::_pending_outgoing_message_direction() const noexcept
{
	return peer_relative_position(_current_pending_outgoing_message_direction);
}

uint8_t nc::network_handler::_pending_outgoing_message_size() const noexcept
{
	return _current_pending_outgoing_message_size;
}

bool nc::network_handler::_has_pending_outgoing_message() const noexcept
{
	return _pending_outgoing_message_size();
}

void nc::network_handler::_clear_pending_outgoing_message() noexcept
{
	_current_pending_outgoing_message_size = 0;
}

SoftwareSerial& nc::network_handler::_listening_side_serial() noexcept
{
	return _listening_side() == peer_relative_position::LEFT ? _left_serial : _right_serial;
}

nc::network_handler::handle_incoming_data_result
nc::network_handler::_handle_incoming_data(SoftwareSerial& serial) noexcept
{
	if (serial.overflow()) {
		logging::log_serial_overflow();
	}

	while (serial.available()) {
		switch (_message_reception_state()) {
		case message_reception_state::RECEIVE_MAGIC_BYTE_1:
		{
			const auto front_byte = uint8_t(serial.read());
			if (front_byte != wire_protocol_magic_1) {
				logging::log_invalid_magic_byte(wire_protocol_magic_1, front_byte);
				break;
			}

			_message_reception_state(message_reception_state::RECEIVE_MAGIC_BYTE_2);
			break;
		}
		case message_reception_state::RECEIVE_MAGIC_BYTE_2:
		{
			const auto front_byte = uint8_t(serial.read());

			if (front_byte != wire_protocol_magic_2) {
				logging::log_invalid_magic_byte(wire_protocol_magic_2, front_byte);
				break;
			}

			_message_reception_state(message_reception_state::RECEIVE_HEADER);
			break;
		}
		case message_reception_state::RECEIVE_HEADER:
		{
			if (serial.available() < sizeof(wire_msg_header)) {
				return handle_incoming_data_result::INCOMPLETE;
			}

			const wire_msg_header header = { .type = uint8_t(serial.peek()) };

			const auto msg_type = header.type;
			logging::log_wire_msg_type(msg_type, F("Received"));

			const auto msg_payload_size = wire_msg_payload_size(msg_type);
			_message_reception_state(message_reception_state::RECEIVE_PAYLOAD);
			/*
			 * Keep the payload and the header's type byte which will allow
			 * us to dispatch the message, and the checksym, which will allow us
			 * to validate the message.
			 */
			_payload_bytes_to_receive = msg_payload_size + sizeof(header);

			break;
		}
		case message_reception_state::RECEIVE_PAYLOAD:
		{
			if (serial.available() < _payload_bytes_to_receive) {
				return handle_incoming_data_result::INCOMPLETE;
			}

			_message_reception_state(message_reception_state::RECEIVE_MAGIC_BYTE_1);
			return handle_incoming_data_result::COMPLETE;
		}
		}
	}

	return handle_incoming_data_result::INCOMPLETE;
}

// Message handler in the "discovery" protocol state.
void nc::network_handler::_wire_protocol_discovery_handle_message(
	uint8_t msg_type, const uint8_t *msg_payload, ns::absolute_time_ms current_time_ms) noexcept
{
	switch (wire_msg_type(msg_type)) {
	case wire_msg_type::ANNOUNCE:
	{
		if (_wave_front_direction() != peer_relative_position::RIGHT) {
			// Invalid message, must come from the left.
			_reset();
			return;
		}

		const auto *announce_msg = reinterpret_cast<const wire_msg_announce *>(msg_payload);
		/*
		 * Tentative for the moment; confirmed on announce reply of if we are the right-most
		 * node.
		 */
		_peer_id = announce_msg->peer_id + 1;
		_peer_count = _peer_id + 1;

		/*
		 * In case this was the last announce message before the monitor message, enqueue
		 * our own announcement.
		 */
		_clear_pending_outgoing_message();
		enqueue_message_result enqueue_result;

		if (_position() == link_position::MIDDLE) {
			const wire_msg_announce our_annouce_msg = { .peer_id = _peer_id };

			enqueue_result =
				enqueue_message(peer_relative_position::RIGHT,
						uint8_t(wire_msg_type::ANNOUNCE),
						reinterpret_cast<const uint8_t *>(&our_annouce_msg),
						sizeof(our_annouce_msg));
		} else {
			// We are the right-most node, enqueue the announce reply.
			const wire_msg_announce_reply announce_reply_msg = { .peer_count =
										     _peer_count };

			enqueue_result = enqueue_message(
				peer_relative_position::LEFT,
				uint8_t(wire_msg_type::ANNOUNCE_REPLY),
				reinterpret_cast<const uint8_t *>(&announce_reply_msg),
				sizeof(announce_reply_msg));
			_wire_protocol_state(wire_protocol_state::RUNNING);
		}

		if (enqueue_result != enqueue_message_result::QUEUED) {
			_reset();
		}

		break;
	}
	case wire_msg_type::ANNOUNCE_REPLY:
	{
		if (_wave_front_direction() != peer_relative_position::LEFT) {
			// Invalid message, must come from the right.
			_reset();
			return;
		}

		const auto *announce_reply_msg =
			reinterpret_cast<const wire_msg_announce_reply *>(msg_payload);
		if (_peer_id >= announce_reply_msg->peer_count) {
			// Invalid value.
			_reset();
			return;
		}

		_peer_count = announce_reply_msg->peer_count;
		_wire_protocol_state(wire_protocol_state::RUNNING);

		if (_position() != link_position::LEFT_MOST) {
			// Forward the announce reply message and send it when it's our turn to
			// broadcast.
			const auto result = enqueue_message(peer_relative_position::LEFT,
							    uint8_t(wire_msg_type::ANNOUNCE_REPLY),
							    msg_payload,
							    sizeof(*announce_reply_msg));

			if (result != enqueue_message_result::QUEUED) {
				_reset();
			}
		}

		break;
	}
	case wire_msg_type::MONITOR:
		_handle_monitor_message(current_time_ms);
		return;
	case wire_msg_type::RESET:
	default:
		// Invalid message during the discovery state.
		_reset();
		return;
	}
}

nc::network_handler::enqueue_message_result
nc::network_handler::enqueue_message(peer_relative_position direction,
				     uint8_t msg_type,
				     const uint8_t *msg_payload,
				     uint8_t payload_size)
{
	if (_has_pending_outgoing_message() ||
	    payload_size > sizeof(_current_pending_outgoing_message_payload)) {
		return enqueue_message_result::FULL;
	}

	if (_position() == link_position::LEFT_MOST && direction == peer_relative_position::LEFT) {
		_reset();
		return enqueue_message_result::UNCONNECTED;
	} else if (_position() == link_position::RIGHT_MOST &&
		   direction == peer_relative_position::RIGHT) {
		_reset();
		return enqueue_message_result::UNCONNECTED;
	}

	_current_pending_outgoing_message_direction = uint8_t(direction);
	_current_pending_outgoing_message_size = payload_size;
	_current_pending_outgoing_message_type = msg_type;
	memcpy(_current_pending_outgoing_message_payload, msg_payload, payload_size);
	return enqueue_message_result::QUEUED;
}

void nc::network_handler::_handle_monitor_message(ns::absolute_time_ms current_time_ms) noexcept
{
	_last_monitor_message_received_time_ms = current_time_ms;
	SoftwareSerial *destination_serial;

	// Temporary, for debugging purposes.
	digitalWrite(LED_DBG, HIGH);

	switch (_position()) {
	case link_position::LEFT_MOST:
		destination_serial = &_right_serial;
		_reverse_wave_front_direction();
		break;
	case link_position::RIGHT_MOST:
		destination_serial = &_left_serial;
		_reverse_wave_front_direction();
		break;
	case link_position::MIDDLE:
		destination_serial = _wave_front_direction() == peer_relative_position::LEFT ?
			&_left_serial :
			&_right_serial;
		_reverse_listening_side();
	default:
		// Unreachable.
		break;
	}

	if (_has_pending_outgoing_message() &&
	    _pending_outgoing_message_direction() == _wave_front_direction()) {
		send_wire_msg(*destination_serial,
			      _current_pending_outgoing_message_type,
			      _current_pending_outgoing_message_payload,
			      _current_pending_outgoing_message_size);
		_clear_pending_outgoing_message();
	}

	// We are done broadcasting.
	send_wire_monitor_msg(*destination_serial);

	if (_position() == link_position::MIDDLE) {
		// Next message will come from the other side.
		_reverse_wave_front_direction();
	}
}

// Message handler in the "running" protocol state.
void nc::network_handler::_wire_protocol_running_handle_message(
	uint8_t msg_type, const uint8_t *msg_payload, ns::absolute_time_ms current_time_ms) noexcept
{
	if (msg_type >= nsec::config::communication::application_message_type_range_begin) {
		// Message targets the application layer.
		auto message_action = _notify_message_received(
			nsec::communication::message::type(msg_type), msg_payload);
		switch (message_action) {
		case application_message_action::FORWARD:
		{
			if (_position() == link_position::MIDDLE) {
				send_wire_msg(_listening_side() == peer_relative_position::LEFT ?
						      _right_serial :
						      _left_serial,
					      msg_type,
					      msg_payload,
					      wire_msg_payload_size(msg_type));
			}

			break;
		}
		case application_message_action::SWALLOW:
			break;
		case application_message_action::RESET:
			send_wire_reset_msg(_listening_side_serial());
			break;
		}

	} else {
		switch (wire_msg_type(msg_type)) {
		case wire_msg_type::MONITOR:
			_handle_monitor_message(current_time_ms);
			return;
		default:
			// Invalid message outside of discovery state.
			_reset();
		}
	}
}

void nc::network_handler::_run_wire_protocol(ns::absolute_time_ms current_time_ms) noexcept
{
	if (_last_monitor_message_received_time_ms < current_time_ms &&
	    (current_time_ms - _last_monitor_message_received_time_ms) >
		    nsec::config::communication::network_handler_timeout_ms &&
	    _wire_protocol_state() != wire_protocol_state ::UNCONNECTED) {
		// No activity for a while... reset.
		logging::log_network_timeout();
		_reset();
		return;
	}

	switch (_wire_protocol_state()) {
	case wire_protocol_state::UNCONNECTED:
		// Nothing to do.
		break;
	case wire_protocol_state::WAIT_TO_SEND_ANNOUNCE:
		/*
		 * State only reached by the left-most node.
		 * Wait for the other boards to setup and expect our messages.
		 */
		if (_ticks_in_wire_state++ == 3) {
			send_wire_announce_msg(_right_serial, 0);
			send_wire_monitor_msg(_right_serial);
			_wire_protocol_state(wire_protocol_state::DISCOVERY);
			// Start waiting for a reply from our right neighboor.
			_listening_side(peer_relative_position::RIGHT);
			_reverse_wave_front_direction();
		}

		break;
	case wire_protocol_state::DISCOVERY:
	case wire_protocol_state::RUNNING:
	{
		auto& listening_serial = _listening_side_serial();
		const auto status = _handle_incoming_data(listening_serial);

		if (status == handle_incoming_data_result::INCOMPLETE) {
			// Receive the remaining data during the next tick.
			break;
		}

		// A complete message is available.
		const uint8_t message_type = uint8_t(listening_serial.read());
		const uint16_t checksum = uint16_t(listening_serial.read()) |
			uint16_t(listening_serial.read()) << 8;
		const auto payload_size = wire_msg_payload_size(message_type);

		uint8_t message_buffer[nsec::config::communication::protocol_max_message_size -
				       sizeof(wire_msg_header)];
		if (payload_size != 0) {
			listening_serial.readBytes(message_buffer, payload_size);
		}

		// Validate checksum
		fletcher16_checksumer checksummer;
		checksummer.push(message_type);
		for (uint8_t i = 0; i < payload_size; i++) {
			checksummer.push(message_buffer[i]);
		}

		if (checksum != checksummer.checksum()) {
			Serial.println(F("Checksum failed"));
		}

		if (_wire_protocol_state() == wire_protocol_state::DISCOVERY) {
			_wire_protocol_discovery_handle_message(message_type,
								payload_size ? message_buffer :
									       nullptr,
								current_time_ms);
		} else {
			_wire_protocol_running_handle_message(message_type,
							      payload_size ? message_buffer :
									     nullptr,
							      current_time_ms);
		}
	}
	}
}

void nc::network_handler::run(ns::absolute_time_ms current_time_ms) noexcept
{
	// Monitor message activity LED
	digitalWrite(LED_DBG, LOW);
	_check_connections();
	_run_wire_protocol(current_time_ms);
}