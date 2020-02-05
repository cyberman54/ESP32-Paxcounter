/*

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

#include "SoftwareSerial.h"
#include <Arduino.h>

#ifdef ESP32
#define xt_rsil(a) (a)
#define xt_wsr_ps(a)
#endif

constexpr uint8_t BYTE_ALL_BITS_SET = ~static_cast<uint8_t>(0);

SoftwareSerial::SoftwareSerial() {
    m_isrOverflow = false;
}

SoftwareSerial::SoftwareSerial(int8_t rxPin, int8_t txPin, bool invert)
{
    m_isrOverflow = false;
    m_rxPin = rxPin;
    m_txPin = txPin;
    m_invert = invert;
}

SoftwareSerial::~SoftwareSerial() {
    end();
}

bool SoftwareSerial::isValidGPIOpin(int8_t pin) {
#if defined(ESP8266)
    return (pin >= 0 && pin <= 5) || (pin >= 12 && pin <= 15);
#elif defined(ESP32)
    return pin == 0 || pin == 2 || (pin >= 4 && pin <= 5) || (pin >= 12 && pin <= 19) ||
        (pin >= 21 && pin <= 23) || (pin >= 25 && pin <= 27) || (pin >= 32 && pin <= 35);
#else
    return true;
#endif
}

void SoftwareSerial::begin(uint32_t baud, SoftwareSerialConfig config,
    int8_t rxPin, int8_t txPin,
    bool invert, int bufCapacity, int isrBufCapacity) {
    if (-1 != rxPin) m_rxPin = rxPin;
    if (-1 != txPin) m_txPin = txPin;
    m_oneWire = (m_rxPin == m_txPin);
    m_invert = invert;
    m_dataBits = 5 + (config & 07);
    m_parityMode = static_cast<SoftwareSerialParity>(config & 070);
    m_stopBits = 1 + ((config & 0300) ? 1 : 0);
    m_pduBits = m_dataBits + static_cast<bool>(m_parityMode) + m_stopBits;
    m_bitCycles = (ESP.getCpuFreqMHz() * 1000000UL + baud / 2) / baud;
    m_intTxEnabled = true;
    if (isValidGPIOpin(m_rxPin)) {
        std::unique_ptr<circular_queue<uint8_t> > buffer(new circular_queue<uint8_t>((bufCapacity > 0) ? bufCapacity : 64));
        m_buffer = move(buffer);
        if (m_parityMode)
        {
            std::unique_ptr<circular_queue<uint8_t> > parityBuffer(new circular_queue<uint8_t>((bufCapacity > 0) ? (bufCapacity + 7) / 8 : 8));
            m_parityBuffer = move(parityBuffer);
            m_parityInPos = m_parityOutPos = 1;
        }
        std::unique_ptr<circular_queue<uint32_t> > isrBuffer(new circular_queue<uint32_t>((isrBufCapacity > 0) ? isrBufCapacity : (sizeof(uint8_t) * 8 + 2) * bufCapacity));
        m_isrBuffer = move(isrBuffer);
        if (m_buffer && (!m_parityMode || m_parityBuffer) && m_isrBuffer) {
            m_rxValid = true;
            pinMode(m_rxPin, INPUT_PULLUP);
        }
    }
    if (isValidGPIOpin(m_txPin)
#ifdef ESP8266
        || ((m_txPin == 16) && !m_oneWire)) {
#else
        ) {
#endif
        m_txValid = true;
        if (!m_oneWire) {
            pinMode(m_txPin, OUTPUT);
            digitalWrite(m_txPin, !m_invert);
        }
    }
    if (!m_rxEnabled) { enableRx(true); }
}

void SoftwareSerial::end()
{
    enableRx(false);
    m_txValid = false;
    if (m_buffer) {
        m_buffer.reset();
    }
    m_parityBuffer.reset();
    if (m_isrBuffer) {
        m_isrBuffer.reset();
    }
}

uint32_t SoftwareSerial::baudRate() {
    return ESP.getCpuFreqMHz() * 1000000UL / m_bitCycles;
}

void SoftwareSerial::setTransmitEnablePin(int8_t txEnablePin) {
    if (isValidGPIOpin(txEnablePin)) {
        m_txEnableValid = true;
        m_txEnablePin = txEnablePin;
        pinMode(m_txEnablePin, OUTPUT);
        digitalWrite(m_txEnablePin, LOW);
    }
    else {
        m_txEnableValid = false;
    }
}

void SoftwareSerial::enableIntTx(bool on) {
    m_intTxEnabled = on;
}

void SoftwareSerial::enableTx(bool on) {
    if (m_txValid && m_oneWire) {
        if (on) {
            enableRx(false);
            pinMode(m_txPin, OUTPUT);
            digitalWrite(m_txPin, !m_invert);
        }
        else {
            pinMode(m_rxPin, INPUT_PULLUP);
            enableRx(true);
        }
    }
}

void SoftwareSerial::enableRx(bool on) {
    if (m_rxValid) {
        if (on) {
            m_rxCurBit = m_pduBits - 1;
            // Init to stop bit level and current cycle
            m_isrLastCycle = (ESP.getCycleCount() | 1) ^ m_invert;
            if (m_bitCycles >= (ESP.getCpuFreqMHz() * 1000000UL) / 74880UL)
                attachInterruptArg(digitalPinToInterrupt(m_rxPin), reinterpret_cast<void (*)(void*)>(rxBitISR), this, CHANGE);
            else
                attachInterruptArg(digitalPinToInterrupt(m_rxPin), reinterpret_cast<void (*)(void*)>(rxBitSyncISR), this, m_invert ? RISING : FALLING);
        }
        else {
            detachInterrupt(digitalPinToInterrupt(m_rxPin));
        }
        m_rxEnabled = on;
    }
}

int SoftwareSerial::read() {
    if (!m_rxValid) { return -1; }
    if (!m_buffer->available()) {
        rxBits();
        if (!m_buffer->available()) { return -1; }
    }
    auto val = m_buffer->pop();
    if (m_parityBuffer)
    {
        m_lastReadParity = m_parityBuffer->peek() & m_parityOutPos;
        m_parityOutPos <<= 1;
        if (!m_parityOutPos)
        {
            m_parityOutPos = 1;
            m_parityBuffer->pop();
        }
    }
    return val;
}

size_t SoftwareSerial::read(uint8_t * buffer, size_t size) {
    if (!m_rxValid) { return 0; }
    size_t avail;
    if (0 == (avail = m_buffer->pop_n(buffer, size))) {
        rxBits();
        avail = m_buffer->pop_n(buffer, size);
    }
    if (!avail) return 0;
    if (m_parityBuffer) {
        uint32_t parityBits = avail;
        while (m_parityOutPos >>= 1) ++parityBits;
        m_parityOutPos = (1 << (parityBits % 8));
        m_parityBuffer->pop_n(nullptr, parityBits / 8);
    }
    return avail;
}

size_t SoftwareSerial::readBytes(uint8_t * buffer, size_t size) {
    if (!m_rxValid || !size) { return 0; }
    size_t count = 0;
    const auto start = millis();
    do {
        count += read(&buffer[count], size - count);
        if (count >= size) break;
        yield();
    } while (millis() - start < _timeout);
    return count;
}

int SoftwareSerial::available() {
    if (!m_rxValid) { return 0; }
    rxBits();
    int avail = m_buffer->available();
    if (!avail) {
        optimistic_yield(10000UL);
    }
    return avail;
}

void ICACHE_RAM_ATTR SoftwareSerial::preciseDelay(bool sync) {
    if (!sync)
    {
        // Reenable interrupts while delaying to avoid other tasks piling up
        if (!m_intTxEnabled) { xt_wsr_ps(m_savedPS); }
        auto expired = ESP.getCycleCount() - m_periodStart;
        if (expired < m_periodDuration)
        {
            auto ms = (m_periodDuration - expired) / ESP.getCpuFreqMHz() / 1000UL;
            if (ms) delay(ms);
        }
        while ((ESP.getCycleCount() - m_periodStart) < m_periodDuration) { optimistic_yield(10000); }
        // Disable interrupts again
        if (!m_intTxEnabled) { m_savedPS = xt_rsil(15); }
    }
    else
    {
        while ((ESP.getCycleCount() - m_periodStart) < m_periodDuration) {}
    }
    m_periodDuration = 0;
    m_periodStart = ESP.getCycleCount();
}

void ICACHE_RAM_ATTR SoftwareSerial::writePeriod(
    uint32_t dutyCycle, uint32_t offCycle, bool withStopBit) {
    preciseDelay(true);
    if (dutyCycle)
    {
        digitalWrite(m_txPin, HIGH);
        m_periodDuration += dutyCycle;
        if (offCycle || (withStopBit && !m_invert)) preciseDelay(!withStopBit || m_invert);
    }
    if (offCycle)
    {
        digitalWrite(m_txPin, LOW);
        m_periodDuration += offCycle;
        if (withStopBit && m_invert) preciseDelay(false);
    }
}

size_t SoftwareSerial::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t SoftwareSerial::write(uint8_t byte, SoftwareSerialParity parity) {
    return write(&byte, 1, parity);
}

size_t SoftwareSerial::write(const uint8_t * buffer, size_t size) {
    return write(buffer, size, m_parityMode);
}

size_t ICACHE_RAM_ATTR SoftwareSerial::write(const uint8_t * buffer, size_t size, SoftwareSerialParity parity) {
    if (m_rxValid) { rxBits(); }
    if (!m_txValid) { return -1; }

    if (m_txEnableValid) {
        digitalWrite(m_txEnablePin, HIGH);
    }
    // Stop bit: if inverted, LOW, otherwise HIGH
    bool b = !m_invert;
    uint32_t dutyCycle = 0;
    uint32_t offCycle = 0;
    if (!m_intTxEnabled) {
        // Disable interrupts in order to get a clean transmit timing
        m_savedPS = xt_rsil(15);
    }
    const uint32_t dataMask = ((1UL << m_dataBits) - 1);
    bool withStopBit = true;
    m_periodDuration = 0;
    m_periodStart = ESP.getCycleCount();
    for (size_t cnt = 0; cnt < size; ++cnt) {
        uint8_t byte = ~buffer[cnt] & dataMask;
        // push LSB start-data-parity-stop bit pattern into uint32_t
        // Stop bits: HIGH
        uint32_t word = ~0UL;
        // parity bit, if any
        if (parity && m_parityMode)
        {
            uint32_t parityBit;
            switch (parity)
            {
            case SWSERIAL_PARITY_EVEN:
                // from inverted, so use odd parity
                parityBit = byte;
                parityBit ^= parityBit >> 4;
                parityBit &= 0xf;
                parityBit = (0x9669 >> parityBit) & 1;
                break;
            case SWSERIAL_PARITY_ODD:
                // from inverted, so use even parity
                parityBit = byte;
                parityBit ^= parityBit >> 4;
                parityBit &= 0xf;
                parityBit = (0x6996 >> parityBit) & 1;
                break;
            case SWSERIAL_PARITY_MARK:
                parityBit = false;
                break;
            case SWSERIAL_PARITY_SPACE:
                // suppresses warning parityBit uninitialized
            default:
                parityBit = true;
                break;
            }
            word ^= parityBit << m_dataBits;
        }
        word ^= byte;
        // Stop bit: LOW
        word <<= 1;
        if (m_invert) word = ~word;
        for (int i = 0; i <= m_pduBits; ++i) {
            bool pb = b;
            b = word & (1UL << i);
            if (!pb && b) {
                writePeriod(dutyCycle, offCycle, withStopBit);
                withStopBit = false;
                dutyCycle = offCycle = 0;
            }
            if (b) {
                dutyCycle += m_bitCycles;
            }
            else {
                offCycle += m_bitCycles;
            }
        }
        withStopBit = true;
    }
    writePeriod(dutyCycle, offCycle, true);
    if (!m_intTxEnabled) {
        // restore the interrupt state
        xt_wsr_ps(m_savedPS);
    }
    if (m_txEnableValid) {
        digitalWrite(m_txEnablePin, LOW);
    }
    return size;
}

void SoftwareSerial::flush() {
    if (!m_rxValid) { return; }
    m_buffer->flush();
    if (m_parityBuffer)
    {
        m_parityInPos = m_parityOutPos = 1;
        m_parityBuffer->flush();
    }
}

bool SoftwareSerial::overflow() {
    bool res = m_overflow;
    m_overflow = false;
    return res;
}

int SoftwareSerial::peek() {
    if (!m_rxValid) { return -1; }
    if (!m_buffer->available()) {
        rxBits();
        if (!m_buffer->available()) return -1;
    }
    auto val = m_buffer->peek();
    if (m_parityBuffer) m_lastReadParity = m_parityBuffer->peek() & m_parityOutPos;
    return val;
}

void SoftwareSerial::rxBits() {
    int isrAvail = m_isrBuffer->available();
#ifdef ESP8266
    if (m_isrOverflow.load()) {
        m_overflow = true;
        m_isrOverflow.store(false);
    }
#else
    if (m_isrOverflow.exchange(false)) {
        m_overflow = true;
    }
#endif

    // stop bit can go undetected if leading data bits are at same level
    // and there was also no next start bit yet, so one byte may be pending.
    // low-cost check first
    if (!isrAvail && m_rxCurBit >= -1 && m_rxCurBit < m_pduBits - m_stopBits) {
        uint32_t detectionCycles = (m_pduBits - m_stopBits - m_rxCurBit) * m_bitCycles;
        if (ESP.getCycleCount() - m_isrLastCycle > detectionCycles) {
            // Produce faux stop bit level, prevents start bit maldetection
            // cycle's LSB is repurposed for the level bit
            rxBits(((m_isrLastCycle + detectionCycles) | 1) ^ m_invert);
        }
    }

    m_isrBuffer->for_each([this](const uint32_t& isrCycle) { rxBits(isrCycle); });
}

void SoftwareSerial::rxBits(const uint32_t & isrCycle) {
    bool level = (m_isrLastCycle & 1) ^ m_invert;

    // error introduced by edge value in LSB of isrCycle is negligible
    int32_t cycles = isrCycle - m_isrLastCycle;
    m_isrLastCycle = isrCycle;

    uint8_t bits = cycles / m_bitCycles;
    if (cycles % m_bitCycles > (m_bitCycles >> 1)) ++bits;
    while (bits > 0) {
        // start bit detection
        if (m_rxCurBit >= (m_pduBits - 1)) {
            // leading edge of start bit
            if (level) break;
            m_rxCurBit = -1;
            --bits;
            continue;
        }
        // data bits
        if (m_rxCurBit >= -1 && m_rxCurBit < (m_dataBits - 1)) {
            int8_t dataBits = min(bits, static_cast<uint8_t>(m_dataBits - 1 - m_rxCurBit));
            m_rxCurBit += dataBits;
            bits -= dataBits;
            m_rxCurByte >>= dataBits;
            if (level) { m_rxCurByte |= (BYTE_ALL_BITS_SET << (8 - dataBits)); }
            continue;
        }
        // parity bit
        if (m_parityMode && m_rxCurBit == (m_dataBits - 1)) {
            ++m_rxCurBit;
            --bits;
            m_rxCurParity = level;
            continue;
        }
        // stop bits
        if (m_rxCurBit < (m_pduBits - m_stopBits - 1)) {
            ++m_rxCurBit;
            --bits;
            continue;
        }
        if (m_rxCurBit == (m_pduBits - m_stopBits - 1)) {
            // Store the received value in the buffer unless we have an overflow
            // if not high stop bit level, discard word
            if (level)
            {
                m_rxCurByte >>= (sizeof(uint8_t) * 8 - m_dataBits);
                if (!m_buffer->push(m_rxCurByte)) {
                    m_overflow = true;
                }
                else {
                    if (m_parityBuffer)
                    {
                        if (m_rxCurParity) {
                            m_parityBuffer->pushpeek() |= m_parityInPos;
                        }
                        else {
                            m_parityBuffer->pushpeek() &= ~m_parityInPos;
                        }
                        m_parityInPos <<= 1;
                        if (!m_parityInPos)
                        {
                            m_parityBuffer->push();
                            m_parityInPos = 1;
                        }
                    }
                }
            }
            m_rxCurBit = m_pduBits;
            // reset to 0 is important for masked bit logic
            m_rxCurByte = 0;
            m_rxCurParity = false;
            break;
        }
        break;
    }
}

void ICACHE_RAM_ATTR SoftwareSerial::rxBitISR(SoftwareSerial * self) {
    uint32_t curCycle = ESP.getCycleCount();
    bool level = digitalRead(self->m_rxPin);

    // Store level and cycle in the buffer unless we have an overflow
    // cycle's LSB is repurposed for the level bit
    if (!self->m_isrBuffer->push((curCycle | 1U) ^ !level)) self->m_isrOverflow.store(true);
}

void ICACHE_RAM_ATTR SoftwareSerial::rxBitSyncISR(SoftwareSerial * self) {
    uint32_t start = ESP.getCycleCount();
    uint32_t wait = self->m_bitCycles - 172U;

    bool level = self->m_invert;
    // Store level and cycle in the buffer unless we have an overflow
    // cycle's LSB is repurposed for the level bit
    if (!self->m_isrBuffer->push(((start + wait) | 1U) ^ !level)) self->m_isrOverflow.store(true);

    for (uint32_t i = 0; i < self->m_pduBits; ++i) {
        while (ESP.getCycleCount() - start < wait) {};
        wait += self->m_bitCycles;

        // Store level and cycle in the buffer unless we have an overflow
        // cycle's LSB is repurposed for the level bit
        if (digitalRead(self->m_rxPin) != level)
        {
            if (!self->m_isrBuffer->push(((start + wait) | 1U) ^ level)) self->m_isrOverflow.store(true);
            level = !level;
        }
    }
}

void SoftwareSerial::onReceive(Delegate<void(int available), void*> handler) {
    receiveHandler = handler;
}

void SoftwareSerial::perform_work() {
    if (!m_rxValid) { return; }
    rxBits();
    if (receiveHandler) {
        int avail = m_buffer->available();
        if (avail) { receiveHandler(avail); }
    }
}
