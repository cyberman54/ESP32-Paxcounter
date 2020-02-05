/*
SoftwareSerial.h

SoftwareSerial.cpp - Implementation of the Arduino software serial for ESP8266/ESP32.
Copyright (c) 2015-2016 Peter Lerup. All rights reserved.
Copyright (c) 2018-2019 Dirk O. Kaar. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef __SoftwareSerial_h
#define __SoftwareSerial_h

#include "circular_queue/circular_queue.h"
#include <Stream.h>

enum SoftwareSerialParity : uint8_t {
    SWSERIAL_PARITY_NONE = 000,
    SWSERIAL_PARITY_EVEN = 020,
    SWSERIAL_PARITY_ODD = 030,
    SWSERIAL_PARITY_MARK = 040,
    SWSERIAL_PARITY_SPACE = 070,
};

enum SoftwareSerialConfig {
    SWSERIAL_5N1 = SWSERIAL_PARITY_NONE,
    SWSERIAL_6N1,
    SWSERIAL_7N1,
    SWSERIAL_8N1,
    SWSERIAL_5E1 = SWSERIAL_PARITY_EVEN,
    SWSERIAL_6E1,
    SWSERIAL_7E1,
    SWSERIAL_8E1,
    SWSERIAL_5O1 = SWSERIAL_PARITY_ODD,
    SWSERIAL_6O1,
    SWSERIAL_7O1,
    SWSERIAL_8O1,
    SWSERIAL_5M1 = SWSERIAL_PARITY_MARK,
    SWSERIAL_6M1,
    SWSERIAL_7M1,
    SWSERIAL_8M1,
    SWSERIAL_5S1 = SWSERIAL_PARITY_SPACE,
    SWSERIAL_6S1,
    SWSERIAL_7S1,
    SWSERIAL_8S1,
    SWSERIAL_5N2 = 0200 | SWSERIAL_PARITY_NONE,
    SWSERIAL_6N2,
    SWSERIAL_7N2,
    SWSERIAL_8N2,
    SWSERIAL_5E2 = 0200 | SWSERIAL_PARITY_EVEN,
    SWSERIAL_6E2,
    SWSERIAL_7E2,
    SWSERIAL_8E2,
    SWSERIAL_5O2 = 0200 | SWSERIAL_PARITY_ODD,
    SWSERIAL_6O2,
    SWSERIAL_7O2,
    SWSERIAL_8O2,
    SWSERIAL_5M2 = 0200 | SWSERIAL_PARITY_MARK,
    SWSERIAL_6M2,
    SWSERIAL_7M2,
    SWSERIAL_8M2,
    SWSERIAL_5S2 = 0200 | SWSERIAL_PARITY_SPACE,
    SWSERIAL_6S2,
    SWSERIAL_7S2,
    SWSERIAL_8S2,
};

/// This class is compatible with the corresponding AVR one, however,
/// the constructor takes no arguments, for compatibility with the
/// HardwareSerial class.
/// Instead, the begin() function handles pin assignments and logic inversion.
/// It also has optional input buffer capacity arguments for byte buffer and ISR bit buffer.
/// Bitrates up to at least 115200 can be used.
class SoftwareSerial : public Stream {
public:
    SoftwareSerial();
    /// Ctor to set defaults for pins.
    /// @param rxPin the GPIO pin used for RX
    /// @param txPin -1 for onewire protocol, GPIO pin used for twowire TX
    SoftwareSerial(int8_t rxPin, int8_t txPin = -1, bool invert = false);
    SoftwareSerial(const SoftwareSerial&) = delete;
    SoftwareSerial& operator= (const SoftwareSerial&) = delete;
    virtual ~SoftwareSerial();
    /// Configure the SoftwareSerial object for use.
    /// @param baud the TX/RX bitrate
    /// @param config sets databits, parity, and stop bit count
    /// @param rxPin -1 or default: either no RX pin, or keeps the rxPin set in the ctor
    /// @param txPin -1 or default: either no TX pin (onewire), or keeps the txPin set in the ctor
    /// @param invert true: uses invert line level logic
    /// @param bufCapacity the capacity for the received bytes buffer
    /// @param isrBufCapacity 0: derived from bufCapacity. The capacity of the internal asynchronous
    ///	       bit receive buffer, a suggested size is bufCapacity times the sum of
    ///	       start, data, parity and stop bit count.
    void begin(uint32_t baud, SoftwareSerialConfig config,
        int8_t rxPin, int8_t txPin, bool invert,
        int bufCapacity = 64, int isrBufCapacity = 0);
    void begin(uint32_t baud, SoftwareSerialConfig config,
        int8_t rxPin, int8_t txPin) {
        begin(baud, config, rxPin, txPin, m_invert);
    }
    void begin(uint32_t baud, SoftwareSerialConfig config,
        int8_t rxPin) {
        begin(baud, config, rxPin, m_txPin, m_invert);
    }
    void begin(uint32_t baud, SoftwareSerialConfig config = SWSERIAL_8N1) {
        begin(baud, config, m_rxPin, m_txPin, m_invert);
    }

    uint32_t baudRate();
    /// Transmit control pin.
    void setTransmitEnablePin(int8_t txEnablePin);
    /// Enable or disable interrupts during tx.
    void enableIntTx(bool on);

    bool overflow();

    int available() override;
    int availableForWrite() {
        if (!m_txValid) return 0;
        return 1;
    }
    int peek() override;
    int read() override;
    /// @returns The verbatim parity bit associated with the last read() or peek() call
    bool readParity()
    {
        return m_lastReadParity;
    }
    /// @returns The calculated bit for even parity of the parameter byte
    static bool parityEven(uint8_t byte) {
        byte ^= byte >> 4;
        byte &= 0xf;
        return (0x6996 >> byte) & 1;
    }
    /// @returns The calculated bit for odd parity of the parameter byte
    static bool parityOdd(uint8_t byte) {
        byte ^= byte >> 4;
        byte &= 0xf;
        return (0x9669 >> byte) & 1;
    }
    /// The read(buffer, size) functions are non-blocking, the same as readBytes but without timeout
    size_t read(uint8_t* buffer, size_t size);
    /// The read(buffer, size) functions are non-blocking, the same as readBytes but without timeout
    size_t read(char* buffer, size_t size) {
        return read(reinterpret_cast<uint8_t*>(buffer), size);
    }
    /// @returns The number of bytes read into buffer, up to size. Times out if the limit set through
    ///          Stream::setTimeout() is reached.
    size_t readBytes(uint8_t* buffer, size_t size) override;
    /// @returns The number of bytes read into buffer, up to size. Times out if the limit set through
    ///          Stream::setTimeout() is reached.
    size_t readBytes(char* buffer, size_t size) override {
        return readBytes(reinterpret_cast<uint8_t*>(buffer), size);
    }
    void flush() override;
    size_t write(uint8_t byte) override;
    size_t write(uint8_t byte, SoftwareSerialParity parity);
    size_t write(const uint8_t* buffer, size_t size) override;
    size_t write(const char* buffer, size_t size) {
        return write(reinterpret_cast<const uint8_t*>(buffer), size);
    }
    size_t write(const uint8_t* buffer, size_t size, SoftwareSerialParity parity);
    size_t write(const char* buffer, size_t size, SoftwareSerialParity parity) {
        return write(reinterpret_cast<const uint8_t*>(buffer), size, parity);
    }
    operator bool() const { return m_rxValid || m_txValid; }

    /// Disable or enable interrupts on the rx pin.
    void enableRx(bool on);
    /// One wire control.
    void enableTx(bool on);

    // AVR compatibility methods.
    bool listen() { enableRx(true); return true; }
    void end();
    bool isListening() { return m_rxEnabled; }
    bool stopListening() { enableRx(false); return true; }

    /// Set an event handler for received data.
    void onReceive(Delegate<void(int available), void*> handler);

    /// Run the internal processing and event engine. Can be iteratively called
    /// from loop, or otherwise scheduled.
    void perform_work();

    using Print::write;

private:
    // If sync is false, it's legal to exceed the deadline, for instance,
    // by enabling interrupts.
    void preciseDelay(bool sync);
    // If withStopBit is set, either cycle contains a stop bit.
    // If dutyCycle == 0, the level is not forced to HIGH.
    // If offCycle == 0, the level remains unchanged from dutyCycle.
    void writePeriod(
        uint32_t dutyCycle, uint32_t offCycle, bool withStopBit);
    bool isValidGPIOpin(int8_t pin);
    /* check m_rxValid that calling is safe */
    void rxBits();
    void rxBits(const uint32_t& isrCycle);

    static void rxBitISR(SoftwareSerial* self);
    static void rxBitSyncISR(SoftwareSerial* self);

    // Member variables
    int8_t m_rxPin = -1;
    int8_t m_txPin = -1;
    int8_t m_txEnablePin = -1;
    uint8_t m_dataBits;
    bool m_oneWire;
    bool m_rxValid = false;
    bool m_rxEnabled = false;
    bool m_txValid = false;
    bool m_txEnableValid = false;
    bool m_invert;
    /// PDU bits include data, parity and stop bits; the start bit is not counted.
    uint8_t m_pduBits;
    bool m_intTxEnabled;
    SoftwareSerialParity m_parityMode;
    uint8_t m_stopBits;
    bool m_lastReadParity;
    bool m_overflow = false;
    uint32_t m_bitCycles;
    uint8_t m_parityInPos;
    uint8_t m_parityOutPos;
    int8_t m_rxCurBit; // 0 thru (m_pduBits - m_stopBits - 1): data/parity bits. -1: start bit. (m_pduBits - 1): stop bit.
    uint8_t m_rxCurByte = 0;
    std::unique_ptr<circular_queue<uint8_t> > m_buffer;
    std::unique_ptr<circular_queue<uint8_t> > m_parityBuffer;
    uint32_t m_periodStart;
    uint32_t m_periodDuration;
    uint32_t m_savedPS = 0;
    // the ISR stores the relative bit times in the buffer. The inversion corrected level is used as sign bit (2's complement):
    // 1 = positive including 0, 0 = negative.
    std::unique_ptr<circular_queue<uint32_t> > m_isrBuffer;
    std::atomic<bool> m_isrOverflow;
    uint32_t m_isrLastCycle;
    bool m_rxCurParity = false;
    Delegate<void(int available), void*> receiveHandler;
};

#endif // __SoftwareSerial_h
