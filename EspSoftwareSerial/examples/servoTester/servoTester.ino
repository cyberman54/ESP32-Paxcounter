#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

SoftwareSerial swSer;

byte buf[10] = { 0xFA, 0xAF,0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0xED };
byte cmd[10] = { 0xFA, 0xAF,0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0xED };
byte ver[10] = { 0xFC, 0xCF,0x00,0xAA,0x41, 0x16, 0x51, 0x01, 0x00, 0xED };


void setup() {
	delay(2000);
	Serial.begin(115200);
	Serial.println("\nAlpha 1S Servo Tester");
	swSer.begin(115200, SWSERIAL_8N1, 12, 12, false, 256);
}

void loop() {
	for (int i = 1; i <= 32; i++) {
		GetVersion(i);
		delay(100);
	}
	SetLED(1, 0);
	GoPos(1, 0, 50);
	delay(1000);
	GoPos(1, 90, 50);
	delay(1000);
	GoPos(1, 100, 50);
	delay(1000);
	SetLED(1, 1);
	delay(2000);
}




void GetVersion(byte id) {
	memcpy(buf, cmd, 10);
	buf[0] = 0xFC;
	buf[1] = 0xCF;
	buf[2] = id;
	buf[3] = 0x01;
	SendCommand();
}


void GoPos(byte id, byte Pos, byte Time) {
	memcpy(buf, cmd, 10);
	buf[2] = id;
	buf[3] = 0x01;
	buf[4] = Pos;
	buf[5] = Time;
	buf[6] = 0x00;
	buf[7] = Time;
	SendCommand();
}

void GetPos(byte id) {
	memcpy(buf, cmd, 10);
	buf[2] = id;
	buf[3] = 0x02;
	SendCommand();
}


void SetLED(byte id, byte mode) {
	memcpy(buf, cmd, 10);
	buf[2] = id;
	buf[3] = 0x04;
	buf[4] = mode;
	SendCommand();
}

void SendCommand() {
	SendCommand(true);
}

void SendCommand(bool checkResult) {
	byte sum = 0;
	for (int i = 2; i < 8; i++) {
		sum += buf[i];
	}
	buf[8] = sum;
	ShowCommand();
	swSer.flush();
	swSer.enableTx(true);
	swSer.write(buf, 10);
	swSer.enableTx(false);
	if (checkResult) checkReturn();
}

void ShowCommand() {
	Serial.print(millis());
	Serial.print(" OUT>>");
	for (int i = 0; i < 10; i++) {
		Serial.print((buf[i] < 0x10 ? " 0" : " "));
		Serial.print(buf[i], HEX);
	}
	Serial.println();
}

void checkReturn() {
	unsigned long startMs = millis();
	while (((millis() - startMs) < 500) && (!swSer.available()));
	if (swSer.available()) {
		Serial.print(millis());
		Serial.print(" IN>>>");
		while (swSer.available()) {
			byte ch = (byte)swSer.read();
			Serial.print((ch < 0x10 ? " 0" : " "));
			Serial.print(ch, HEX);
		}
		Serial.println();
	}
}
