#include <SoftwareSerial.h>

// On ESP8266:
// SoftwareSerial loopback for remote source (loopback.ino), or hardware loopback.
// Connect source D5 (rx) to local D8 (tx), source D6 (tx) to local D7 (rx).
// Hint: The logger is run at 9600bps such that enableIntTx(true) can remain unchanged. Blocking
// interrupts severely impacts the ability of the SoftwareSerial devices to operate concurrently
// and/or in duplex mode.
// On ESP32:
// For software or hardware loopback, connect source rx to local D8 (tx), source tx to local D7 (rx).

#if defined(ESP8266) && !defined(D5)
#define D5 (14)
#define D6 (12)
#define D7 (13)
#define D8 (15)
#define TX (1)
#endif

#define HWLOOPBACK 1
#define HALFDUPLEX 1

#ifdef ESP32
constexpr int IUTBITRATE = 19200;
#else
constexpr int IUTBITRATE = 19200;
#endif

#if defined(ESP8266)
constexpr SoftwareSerialConfig swSerialConfig = SWSERIAL_8E1;
constexpr SerialConfig hwSerialConfig = SERIAL_8E1;
#elif defined(ESP32)
constexpr SoftwareSerialConfig swSerialConfig = SWSERIAL_8E1;
constexpr uint32_t hwSerialConfig = SERIAL_8E1;
#else
constexpr unsigned swSerialConfig = 3;
#endif
constexpr bool invert = false;

constexpr int BLOCKSIZE = 16; // use fractions of 256

unsigned long start;
String bitRateTxt("Effective data rate: ");
int rxCount;
int seqErrors;
int parityErrors;
int expected;
constexpr int ReportInterval = IUTBITRATE / 8;

#if defined(ESP8266)
#if defined(HWLOOPBACK)
HardwareSerial& repeater(Serial);
SoftwareSerial logger;
#else
SoftwareSerial repeater;
HardwareSerial& logger(Serial);
#endif
#elif defined(ESP32)
#if defined(HWLOOPBACK)
HardwareSerial& repeater(Serial2);
#else
SoftwareSerial repeater;
#endif
HardwareSerial& logger(Serial);
#else
SoftwareSerial repeater(14, 12);
HardwareSerial& logger(Serial);
#endif

void setup() {
#if defined(ESP8266)
#if defined(HWLOOPBACK)
    repeater.begin(IUTBITRATE, hwSerialConfig, SERIAL_FULL, 1, invert);
    repeater.swap();
    repeater.setRxBufferSize(2 * BLOCKSIZE);
    logger.begin(9600, SWSERIAL_8N1, -1, TX);
#else
    repeater.begin(IUTBITRATE, swSerialConfig, D7, D8, invert, 4 * BLOCKSIZE);
#ifdef HALFDUPLEX
    repeater.enableIntTx(false);
#endif
    logger.begin(9600);
#endif
#elif defined(ESP32)
#if defined(HWLOOPBACK)
    repeater.begin(IUTBITRATE, hwSerialConfig, D7, D8, invert);
    repeater.setRxBufferSize(2 * BLOCKSIZE);
#else
    repeater.begin(IUTBITRATE, swSerialConfig, D7, D8, invert, 4 * BLOCKSIZE);
#ifdef HALFDUPLEX
    repeater.enableIntTx(false);
#endif
#endif
    logger.begin(9600);
#else
    repeater.begin(IUTBITRATE);
    logger.begin(9600);
#endif

    logger.println("Repeater example for EspSoftwareSerial");
    start = micros();
    rxCount = 0;
    seqErrors = 0;
    parityErrors = 0;
    expected = -1;
}

void loop() {
#ifdef HWLOOPBACK
#if defined(ESP8266)
    if (repeater.hasOverrun()) { logger.println("repeater.overrun"); }
#endif
#else
    if (repeater.overflow()) { logger.println("repeater.overflow"); }
#endif

#ifdef HALFDUPLEX
    char block[BLOCKSIZE];
#endif
    // starting deadline for the first bytes to come in
    uint32_t deadlineStart = ESP.getCycleCount();
    int inCnt = 0;
    while ((ESP.getCycleCount() - deadlineStart) < (1000000UL * 12 * BLOCKSIZE) / IUTBITRATE * 24 * ESP.getCpuFreqMHz()) {
        int avail = repeater.available();
        for (int i = 0; i < avail; ++i)
        {
            int r = repeater.read();
            if (r == -1) { logger.println("read() == -1"); }
            if (expected == -1) { expected = r; }
            else {
                expected = (expected + 1) % (1UL << (5 + swSerialConfig % 4));
            }
            if (r != expected) {
                ++seqErrors;
                expected = -1;
            }
#ifndef HWLOOPBACK
            if (repeater.readParity() != (static_cast<bool>(swSerialConfig & 010) ? repeater.parityOdd(r) : repeater.parityEven(r)))
            {
                ++parityErrors;
            }
#endif
            ++rxCount;
#ifdef HALFDUPLEX
            block[inCnt] = r;
#else
            repeater.write(r);
#endif
            if (++inCnt >= BLOCKSIZE) { break; }
        }
        if (inCnt >= BLOCKSIZE) { break; }
        // wait for more outstanding bytes to trickle in
        if (avail) deadlineStart = ESP.getCycleCount();
    }

#ifdef HALFDUPLEX
    repeater.write(block, inCnt);
#endif

    if (rxCount >= ReportInterval) {
        auto end = micros();
        unsigned long interval = end - start;
        long cps = rxCount * (1000000.0 / interval);
        long seqErrorsps = seqErrors * (1000000.0 / interval);
        logger.print(bitRateTxt + 10 * cps + "bps, "
            + seqErrorsps + "cps seq. errors (" + 100.0 * seqErrors / rxCount + "%)");
#ifndef HWLOOPBACK
        if (0 != (swSerialConfig & 070))
        {
            logger.print(" ("); logger.print(parityErrors); logger.print(" parity errors)");
        }
        else
#endif
        {
            logger.println();
        }
        start = end;
        rxCount = 0;
        seqErrors = 0;
        parityErrors = 0;
        expected = -1;
    }
}
