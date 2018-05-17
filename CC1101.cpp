//
// CC1101 transceiver minimalistic small-packet-oriented driver
//

#include "CC1101.h"
#include "CC1101Int.h"
#include <SPI.h>

// The following file is generated using TI SmartRF Studio
#include "CC1101Cfg.h"

static const uint8_t PROGMEM s_def_cfg1[] = {
	SMARTRF_SETTING_PKTCTRL1,// PKTCTRL1 0x07         // Packet automation control
	SMARTRF_SETTING_PKTCTRL0,// PKTCTRL0 0x08         // Packet automation control
	SMARTRF_SETTING_ADDR,    // ADDR     0x09         // Device address
	SMARTRF_SETTING_CHANNR , // CHANNR   0x0A         // Channel number
	SMARTRF_SETTING_FSCTRL1, // FSCTRL1  0x0B         // Frequency synthesizer control
	SMARTRF_SETTING_FSCTRL0, // FSCTRL0  0x0C         // Frequency synthesizer control
	SMARTRF_SETTING_FREQ2,   // FREQ2    0x0D         // Frequency control word, high byte
	SMARTRF_SETTING_FREQ1,   // FREQ1    0x0E         // Frequency control word, middle byte
	SMARTRF_SETTING_FREQ0,   // FREQ0    0x0F         // Frequency control word, low byte
	SMARTRF_SETTING_MDMCFG4, // MDMCFG4  0x10         // Modem configuration
	SMARTRF_SETTING_MDMCFG3, // MDMCFG3  0x11         // Modem configuration
	SMARTRF_SETTING_MDMCFG2, // MDMCFG2  0x12         // Modem configuration
	SMARTRF_SETTING_MDMCFG1, // MDMCFG1  0x13         // Modem configuration
	SMARTRF_SETTING_MDMCFG0, // MDMCFG0  0x14         // Modem configuration
	SMARTRF_SETTING_DEVIATN, // DEVIATN  0x15         // Modem deviation setting
};

static const uint8_t PROGMEM s_def_cfg2[] = {
	SMARTRF_SETTING_MCSM0,   // MCSM0    0x18         // Main Radio Cntrl State Machine config
	SMARTRF_SETTING_FOCCFG,  // FOCCFG   0x19         // Frequency Offset Compensation config
	SMARTRF_SETTING_BSCFG,   // BSCFG    0x1A         // Bit Synchronization configuration
	SMARTRF_SETTING_AGCCTRL2,// AGCCTRL2 0x1B         // AGC control
	SMARTRF_SETTING_AGCCTRL1,// AGCCTRL1 0x1C         // AGC control
	SMARTRF_SETTING_AGCCTRL0,// AGCCTRL0 0x1D         // AGC control
};

static const uint8_t PROGMEM s_def_cfg3[] = {
	SMARTRF_SETTING_FREND1, // FREND1   0x21         // Front end RX configuration
	SMARTRF_SETTING_FREND0, // FREND0   0x22         // Front end TX configuration
	SMARTRF_SETTING_FSCAL3, // FSCAL3   0x23         // Frequency synthesizer calibration
	SMARTRF_SETTING_FSCAL2, // FSCAL2   0x24         // Frequency synthesizer calibration
	SMARTRF_SETTING_FSCAL1, // FSCAL1   0x25         // Frequency synthesizer calibration
	SMARTRF_SETTING_FSCAL0, // FSCAL0   0x26         // Frequency synthesizer calibration
};

static inline void select()
{ 
	digitalWrite(SS, LOW);
}

static inline void deselect()
{
	digitalWrite(SS, HIGH);
}

static inline void wait_ready()
{
	while (digitalRead(MISO)) {}
}

static inline uint8_t strobe(uint8_t cmd)
{
	select();
	wait_ready();
	uint8_t st = SPI.transfer(cmd);
	if (cmd == SRES) {
		wait_ready();
	}
	deselect();
	return st;
}

static void write_reg(uint8_t addr, uint8_t value)
{
	select();
	wait_ready();
	SPI.transfer(addr);
	SPI.transfer(value);
	deselect();
}

static void write_burst(uint8_t addr, uint8_t const * data, uint8_t len)
{
	select();
	wait_ready();
	SPI.transfer(addr | BURST);
	for (; len; --len, ++data) {
		SPI.transfer(*data);
	}
	deselect();
}

static void write_burst_pgm(uint8_t addr, uint8_t const * PROGMEM data, uint8_t len)
{
	select();
	wait_ready();
	SPI.transfer(addr | BURST);
	for (; len; --len, ++data) {
		SPI.transfer(pgm_read_byte(data));
	}
	deselect();
}

static uint8_t read_reg(uint8_t addr)
{
	select();
	wait_ready();
	SPI.transfer(addr | READ);
	uint8_t val = SPI.transfer(0xff);
	deselect();
	return val;
}

static void read_burst(uint8_t addr, uint8_t * buff, uint8_t len)
{
	select();
	wait_ready();
	SPI.transfer(addr | BURST | READ);
	for (; len; --len, ++buff) {
		*buff = SPI.transfer(0xff);
	}
	deselect();
}

// Initialize SPI port, reset device and configure it
bool CC1101Transceiver::begin()
{
	SPI.begin();
	return reset();
}

// Reset device and configure it
bool CC1101Transceiver::reset()
{
	deselect();
	delayMicroseconds(5);
	wakeup();
	delayMicroseconds(50);
	strobe(SRES);
	return configure();
}

// Go to sleep state
void CC1101Transceiver::sleep()
{
	strobe(SPWD);
}

// Wake up device from the sleep
void CC1101Transceiver::wakeup()
{
	select();
	delayMicroseconds(10);
	deselect();
}

// Read device status byte
uint8_t CC1101Transceiver::get_status()
{
	return strobe(SNOP);
}

// Change listening address
void CC1101Transceiver::set_address(uint8_t addr)
{
	write_reg(ADDR, addr);
}

// Change channel
void CC1101Transceiver::set_channel(uint8_t chan)
{
	write_reg(CHANNR, chan);
}

// Change output power level
void CC1101Transceiver::set_tx_power(uint8_t power)
{
	write_reg(PATABLE, power);
}

// Start receiving
void CC1101Transceiver::receive()
{
	strobe(SRX);
}

// Read the packet received
bool CC1101Transceiver::get_packet(struct CC1101RxPacket * p)
{
	p->rx_len = read_reg(RXBYTES);
	if (p->rx_len < 4 || CC1101_FIFO_LEN < p->rx_len)
		return false;
	read_burst(FIFO, p->buff, p->rx_len);
	return true;
}

// Write packet and start transmission
void CC1101Transceiver::send_packet(struct CC1101Packet const* p)
{
	write_burst(FIFO, p->buff, 1 + p->len);
	strobe(STX);
}

// Read receiver signal strength indicator (RSSI)
int8_t CC1101Transceiver::get_rssi()
{
	return read_reg(RSSI);
}

// Configure device
bool CC1101Transceiver::configure()
{
	uint8_t sync1 = m_sync >> 8, sync0 = m_sync & 0xff;

	write_burst_pgm(PKTCTRL1, s_def_cfg1, sizeof(s_def_cfg1));
	write_burst_pgm(MCSM0,    s_def_cfg2, sizeof(s_def_cfg2));
	write_burst_pgm(FREND1,   s_def_cfg3, sizeof(s_def_cfg3));

	write_reg(SYNC1,   sync1);
	write_reg(SYNC0,   sync0);
	write_reg(ADDR,    m_addr);
	write_reg(CHANNR,  m_chan);
	write_reg(PATABLE, m_power);

	return read_reg(SYNC1) == sync1 && read_reg(SYNC0) == sync0;
}
