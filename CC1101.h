#pragma once

//
// CC1101 transceiver minimalistic small-packet-oriented driver
//

#include <Arduino.h>

#define CC1101_FIFO_LEN 64
#define CC1101_MAX_DATA_LEN (CC1101_FIFO_LEN-2)
#define CC1101_MAX_PAYLOAD  (CC1101_MAX_DATA_LEN-2)

// The transceiver as represented in status word
typedef enum {
	st_IDLE = 0,
	st_RX,
	st_TX,
	st_FSTXON,
	st_CALIBRATE,
	st_SETTLING,
	st_RXFIFO_OVERFLOW,
	st_TXFIFO_UNDERFLOW,
	st_FAILED,
} CC1100State_t;

// The received packet information
struct CC1101PacketRxInfo {
	int8_t  rssi;
	uint8_t lqi_crc;

	bool    crc_ok() const { return lqi_crc & 0x80; }
	uint8_t lqi()    const { return lqi_crc & 0x7f; }
};

// The packet structure. Use it for sending your data.
struct CC1101Packet {
	union {
		struct {
			uint8_t len;
			uint8_t addr;
			uint8_t data[CC1101_MAX_DATA_LEN];
		};
		uint8_t buff[CC1101_FIFO_LEN];
	};
	CC1101Packet(
		uint8_t address,     // target address, use 0 for broadcasting
		uint8_t payload_len  // payload data length
	)
		: len(payload_len + 1), addr(address) {}
protected:
	CC1101Packet() {}
};

// The received packet structure.
struct CC1101RxPacket : public CC1101Packet {
	uint8_t rx_len;  // The total amount of data read from Rx FIFO

	CC1101RxPacket() {}

	// Validate packet size
	bool size_valid() const {
		return 4 <= rx_len && rx_len <= CC1101_FIFO_LEN && rx_len == len + 3;
	}

	// Check if the packet is valid (not corrupted in transit)
	bool valid() const { return size_valid() && info()->crc_ok(); }

	// Get payload length (stored in data array)
	uint8_t payload_len() const { return len - 1; }
	
	// Get the packet information structure
	struct CC1101PacketRxInfo const* info() const {
		return (struct CC1101PacketRxInfo const*)(&buff[rx_len-2]);
	}
};

class CC1101Transceiver {
public:
	// Create transceiver object
	CC1101Transceiver(
			uint16_t sync,        // sync word acts like network id - all nodes must have exactly the same sync word
			uint8_t  addr = 0,    // the listening address
			uint8_t  chan = 165,  // the channel number (400 + 165 * 0.2 = 433MHz)
			uint8_t  power = 0xc6 // output power level
		)
			: m_sync(sync), m_addr(addr), m_chan(chan), m_power(power) {}

	// Initialize SPI port, reset device and configure it
	bool begin();

	// Reset device and configure it
	bool reset();

	// Go to sleep state
	void sleep();

	// Wake up device from the sleep
	void wakeup();

	// Change listening address
	void set_address(uint8_t addr);

	// Change channel
	void set_channel(uint8_t chan);

	// Change output power level
	void set_tx_power(uint8_t power);

	// Start receiving
	void receive();

	// Read the packet received
	bool get_packet(struct CC1101RxPacket * p);

	// Write packet and start transmission
	void send_packet(struct CC1101Packet const* p);

	// Read receiver signal strength indicator (RSSI)
	int8_t get_rssi();
	
	// Read device status byte
	uint8_t get_status();

	// Read current state
	CC1100State_t get_state()
	{
		uint8_t sta = get_status();
		if (sta & 0x80) {
			return st_FAILED;
		} else {
			return (CC1100State_t)((sta >> 4) & 7);
		}
	}

private:
	// Configure device
	bool configure();

	// Configuration parameters
	uint16_t m_sync; // sync word acts like network id - all nodes must have exactly the same sync word
	uint8_t  m_addr; // the listening address
	uint8_t  m_chan; // the channel number
	uint8_t  m_power;// output power level
};
