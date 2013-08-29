// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

/** modified by Christophe Duvernois <christophe.duvernois@gmail.com>
 * for the libnfc arduino driver */

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "PN532.h"

byte pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
byte pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

#define PN532_PACKBUFFSIZ 64
byte pn532_packetbuffer[PN532_PACKBUFFSIZ];

/**************************************************************************/
/*!
    @brief  Prints a hexadecimal value in plain characters

    @param  data      Pointer to the byte data
    @param  numBytes  Data length in bytes
*/
/**************************************************************************/
void PN532::PrintHex(const byte * data, const uint32_t numBytes) {
	uint32_t szPos;
	for (szPos=0; szPos < numBytes; szPos++) {
		Serial.print("0x");
		// Append leading 0 for small values
		if (data[szPos] <= 0xF)
			Serial.print("0");
		Serial.print(data[szPos], HEX);
		if ((numBytes > 1) && (szPos != numBytes - 1)) {
			Serial.print(" ");
		}
	}
	Serial.println("");
}

/**************************************************************************/
/*!
    @brief  Prints a hexadecimal value in plain characters, along with
            the char equivalents in the following format

            00 00 00 00 00 00  ......

    @param  data      Pointer to the byte data
    @param  numBytes  Data length in bytes
*/
/**************************************************************************/
void PN532::PrintHexChar(const byte * data, const uint32_t numBytes) {
	uint32_t szPos;
	for (szPos=0; szPos < numBytes; szPos++) {
		// Append leading 0 for small values
		if (data[szPos] <= 0xF)
			Serial.print("0");
		Serial.print(data[szPos], HEX);
		if ((numBytes > 1) && (szPos != numBytes - 1)) {
			Serial.print(" ");
		}
	}
	Serial.print("  ");
	for (szPos=0; szPos < numBytes; szPos++) {
		if (data[szPos] <= 0x1F)
			Serial.print(".");
		else
			Serial.print(data[szPos]);
	}
	Serial.println("");
}

PN532::PN532(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss) {
	_clk = clk;
	_miso = miso;
	_mosi = mosi;
	_ss = ss;

	pinMode(_ss, OUTPUT);
	pinMode(_clk, OUTPUT);
	pinMode(_mosi, OUTPUT);
	pinMode(_miso, INPUT);
}

void PN532::begin() {
	digitalWrite(_ss, LOW);

	//delay(1000);

	// not exactly sure why but we have to send a dummy command to get synced up
	pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;
	sendCommandCheckAck(pn532_packetbuffer, 1);

	// ignore response!
}

uint32_t PN532::getFirmwareVersion(void) {
	uint32_t response;

	pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;

	if (! sendCommandCheckAck(pn532_packetbuffer, 1))
		return 0;

	// read data packet
	readspidata(pn532_packetbuffer, 12);
	// check some basic stuff
	if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6)) {
		return 0;
	}

	response = pn532_packetbuffer[6];
	response <<= 8;
	response |= pn532_packetbuffer[7];
	response <<= 8;
	response |= pn532_packetbuffer[8];
	response <<= 8;
	response |= pn532_packetbuffer[9];

	return response;
}


// default timeout of one second
boolean PN532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout) {
	uint16_t timer = 0;

	// write the command
	spiwritecommand(cmd, cmdlen);

	// Wait for chip to say its ready!
	while (readspistatus() != PN532_SPI_READY) {
		if (timeout != 0) {
			timer+=10;
			if (timer > timeout)
				return false;
		}
		delay(10);
	}

	// read acknowledgement
	if (!spi_readack()) {
		return false;
	}

	timer = 0;
	// Wait for chip to say its ready!
	while (readspistatus() != PN532_SPI_READY) {
		if (timeout != 0) {
			timer+=10;
			if (timer > timeout)
				return false;
		}
		delay(10);
	}

	return true; // ack'd command
}

// default timeout of one second
uint8_t PN532::sendRawCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout) {
	uint16_t timer = 0;

	// write the command
	spiwriteraw(cmd, cmdlen);

	// Wait for chip to say its ready!
	while (readspistatus() != PN532_SPI_READY) {
		if (timeout != 0) {
			timer+=10;
			if (timer > timeout)
				return 0xF1;
		}
		delay(10);
	}

	// read acknowledgement
	uint8_t ack[6] = { 0 };
	spi_readack(ack);
	//send ack to uart
	Serial.write(ack, 6);

	// Wait for chip to say its ready!
	timer = 0;
	while (readspistatus() != PN532_SPI_READY) {
		if (timeout != 0) {
			timer+=10;
			if (timer > timeout)
				return 0xF2;
		}
		delay(10);
	}

	return 0; // ack'd command
}

void PN532::readRawCommandAnswer(uint8_t *cmd, uint8_t cmdlen){
	uint8_t size = 0;
	readspidataAnswer(pn532_packetbuffer, size);
	if((pn532_packetbuffer[0] == 0) && (pn532_packetbuffer[1] == 0)){
		//packet length is the first byte after 0x00 0x00 0xff
		Serial.write(pn532_packetbuffer, size);
	}else{
		//hack this should never happen but sometime the first 
		//null byte is missing :(
		//don't know why i don't receive the first 0x00
		//packet length is the first byte after 0x00 0x00 0xff
		Serial.write((uint8_t)0x00);
		Serial.write(pn532_packetbuffer, size - 1);
	}
	delay(5);
}

boolean PN532::SAMConfig(void) {
	pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
	pn532_packetbuffer[1] = 0x01; // normal mode;
	pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
	pn532_packetbuffer[3] = 0x01; // use IRQ pin!

	if (! sendCommandCheckAck(pn532_packetbuffer, 4))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 8);

	return  (pn532_packetbuffer[5] == 0x15);
}

uint32_t PN532::configurePeerAsInitiator(uint8_t baudrate /* Any number from 0-2. 0 for 106kbps or 1 for 201kbps or 2 for 424kbps */) {

	pn532_packetbuffer[0] = PN532_INJUMPFORDEP;
	pn532_packetbuffer[1] = 0x01; //Active Mode
	pn532_packetbuffer[2] = baudrate; // Use 1 or 2. //0 i.e 106kps is not supported yet
	pn532_packetbuffer[3] = 0x01; //Indicates Optional Payload is present

	//Polling request payload
	pn532_packetbuffer[4] = 0x00;
	pn532_packetbuffer[5] = 0xFF;
	pn532_packetbuffer[6] = 0xFF;
	pn532_packetbuffer[7] = 0x00;
	pn532_packetbuffer[8] = 0x00;

	if (! sendCommandCheckAck(pn532_packetbuffer, 9))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 19+6);

#ifdef PN532DEBUG
	Serial.println();
	// check the response
	Serial.println("PEER_INITIATOR");
	for(uint8_t i=0; i<19+6; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.print(" ");
	}
#endif

	return (pn532_packetbuffer[7] == 0x00); //No error

}

uint32_t PN532::initiatorTxRx(char *DataOut,char *DataIn) {
	pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 0x01; //Target 01

	for(uint8_t iter=(2+0); iter<(2+16); iter++) {
		pn532_packetbuffer[iter] = DataOut[iter-2]; //pack the data to send to target
	}

	if (! sendCommandCheckAck(pn532_packetbuffer, 18))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 18+6);

#ifdef PN532DEBUG
	Serial.println();
	// check the response
	Serial.println("INITIATOR_TXRX");
	for(uint8_t i=0; i<18+6; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.print(" ");
	}
#endif

	for(uint8_t iter=8; iter<(8+16); iter++) {
		DataIn[iter-8] = pn532_packetbuffer[iter]; //data received from target
	}

	return (pn532_packetbuffer[7] == 0x00); //No error
}

uint32_t PN532::configurePeerAsTarget() {

	byte pbuffer[38] =      { PN532_TGINITASTARGET,
							  0x00,
							  0x08, 0x00, //SENS_RES
							  0x12, 0x34, 0x56, //NFCID1
							  0x40, //SEL_RES

							  0x01, 0xFE, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, // POL_RES
							  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,

							  0xFF, 0xFF,

							  0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, //NFCID3t: Change this to desired value

							  0x00, 0x00 //Length of general and historical bytes
							};

	for(uint8_t iter=0; iter<38; iter++) {
		pn532_packetbuffer[iter] = pbuffer[iter];
	}

	if (! sendCommandCheckAck(pn532_packetbuffer, 38))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 18+6);

#ifdef PN532DEBUG
	Serial.println();
	// check some basic stuff

	Serial.println("PEER_TARGET");
	for(uint8_t i=0; i<18+6; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
	}
#endif

	return (pn532_packetbuffer[23] == 0x00); //No error as it received all response
}

uint32_t PN532::targetTxRx(char *DataOut,char *DataIn) {
///////////////////////////////////// Receiving from Initiator ///////////////////////////
	pn532_packetbuffer[0] = PN532_TGGETDATA;
	if (! sendCommandCheckAck(pn532_packetbuffer, 1))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 18+6);

#ifdef PN532DEBUG
	Serial.println();
	// check the response
	Serial.println("TARGET_RX");
	for(uint8_t i=0; i<18+6; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.print(" ");
	}
#endif

	for(uint8_t iter=8; iter<(8+16); iter++) {
		DataIn[iter-8] = pn532_packetbuffer[iter]; //data received from initiator
	}

///////////////////////////////////// Sending to Initiator ///////////////////////////
	if(pn532_packetbuffer[7] == 0x00) { //If no errors in receiving, send data.
		pn532_packetbuffer[0] = PN532_TGSETDATA;
		for(uint8_t iter=(1+0); iter<(1+16); iter++) {
			pn532_packetbuffer[iter] = DataOut[iter-1]; //pack the data to send to target
		}

		if (! sendCommandCheckAck(pn532_packetbuffer, 17))
			return false;

		// read data packet
		readspidata(pn532_packetbuffer, 2+6);

#ifdef PN532DEBUG
		Serial.println();
		// check the response
		Serial.println("TARGET_TX");
		for(uint8_t i=0; i<2+6; i++) {
			Serial.print(pn532_packetbuffer[i], HEX); Serial.print(" ");
		}
#endif

		return (pn532_packetbuffer[7] == 0x00); //No error
	}

}

uint32_t PN532::authenticateBlock(uint8_t cardnumber /*1 or 2*/,uint32_t cid /*Card NUID*/, uint8_t blockaddress /*0 to 63*/,uint8_t authtype/*Either KEY_A or KEY_B */, uint8_t * keys) {
	pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
	pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
	if(authtype == KEY_A) {
		pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYA;
	} else {
		pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYB;
	}
	pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

	pn532_packetbuffer[4] = keys[0];
	pn532_packetbuffer[5] = keys[1];
	pn532_packetbuffer[6] = keys[2];
	pn532_packetbuffer[7] = keys[3];
	pn532_packetbuffer[8] = keys[4];
	pn532_packetbuffer[9] = keys[5];

	pn532_packetbuffer[10] = ((cid >> 24) & 0xFF);
	pn532_packetbuffer[11] = ((cid >> 16) & 0xFF);
	pn532_packetbuffer[12] = ((cid >> 8) & 0xFF);
	pn532_packetbuffer[13] = ((cid >> 0) & 0xFF);

	if (! sendCommandCheckAck(pn532_packetbuffer, 14))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 2+6);

#ifdef PN532DEBUG
	for(int iter=0; iter<14; iter++) {
		Serial.print(pn532_packetbuffer[iter], HEX);
		Serial.print(" ");
	}
	Serial.println();
	// check some basic stuff

	Serial.println("AUTH");
	for(uint8_t i=0; i<2+6; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
	}
#endif
	PN532::PrintHexChar(pn532_packetbuffer, 12);

	if((pn532_packetbuffer[6] == 0x41) && (pn532_packetbuffer[7] == 0x00)) {
		return true;
	} else {
		return false;
	}

}

uint32_t PN532::readMemoryBlock(uint8_t cardnumber /*1 or 2*/,uint8_t blockaddress /*0 to 63*/, uint8_t * block) {
	pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
	pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
	pn532_packetbuffer[2] = PN532_MIFARE_READ;
	pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

	if (! sendCommandCheckAck(pn532_packetbuffer, 4))
		return false;

	// read data packet
	readspidata(pn532_packetbuffer, 18+6);
	// check some basic stuff
#ifdef PN532DEBUG
	Serial.println("READ");
#endif
	for(uint8_t i=8; i<18+6; i++) {
		block[i-8] = pn532_packetbuffer[i];
#ifdef PN532DEBUG
		Serial.print(pn532_packetbuffer[i], HEX); Serial.print(" ");
#endif
	}
	if((pn532_packetbuffer[6] == 0x41) && (pn532_packetbuffer[7] == 0x00)) {
		return true; //read successful
	} else

	{
		return false;
	}

}

//Do not write to Sector Trailer Block unless you know what you are doing.
uint32_t PN532::writeMemoryBlock(uint8_t cardnumber /*1 or 2*/,uint8_t blockaddress /*0 to 63*/, uint8_t * block) {
	pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
	pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
	pn532_packetbuffer[2] = PN532_MIFARE_WRITE;
	pn532_packetbuffer[3] = blockaddress;

	for(uint8_t byte=0; byte <16; byte++) {
		pn532_packetbuffer[4+byte] = block[byte];
	}

	if (! sendCommandCheckAck(pn532_packetbuffer, 20))
		return false;
	// read data packet
	readspidata(pn532_packetbuffer, 2+6);

#ifdef PN532DEBUG
	// check some basic stuff
	Serial.println("WRITE");
	for(uint8_t i=0; i<2+6; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
	}
#endif

	if((pn532_packetbuffer[6] == 0x41) && (pn532_packetbuffer[7] == 0x00)) {
		return true; //write successful
	} else {
		return false;
	}
}

uint32_t PN532::readPassiveTargetID(uint8_t cardbaudrate) {
	uint32_t cid;

	pn532_packetbuffer[0] = PN532_INLISTPASSIVETARGET;
	pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
	pn532_packetbuffer[2] = cardbaudrate;

	if (! sendCommandCheckAck(pn532_packetbuffer, 3))
		return 0x0;  // no cards read

	// read data packet
	readspidata(pn532_packetbuffer, 20);
	// check some basic stuff

	Serial.print("Found "); Serial.print(pn532_packetbuffer[7], DEC); Serial.println(" tags");
	if (pn532_packetbuffer[7] != 1)
		return 0;

	uint16_t sens_res = pn532_packetbuffer[9];
	sens_res <<= 8;
	sens_res |= pn532_packetbuffer[10];
	Serial.print("Sens Response: 0x");  Serial.println(sens_res, HEX);
	Serial.print("Sel Response: 0x");  Serial.println(pn532_packetbuffer[11], HEX);
	cid = 0;
	for (uint8_t i=0; i< pn532_packetbuffer[12]; i++) {
		cid <<= 8;
		cid |= pn532_packetbuffer[13+i];
		Serial.print(" 0x"); Serial.print(pn532_packetbuffer[13+i], HEX);
	}

#ifdef PN532DEBUG
	Serial.println("TargetID");
	for(uint8_t i=0; i<20; i++) {
		Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
	}
#endif
	return cid;
}

int32_t PN532::initiatorSelectPassiveTarget() {
	return 0;
}

uint32_t PN532::readRegister(uint16_t address, uint8_t* value) {
	pn532_packetbuffer[0] = PN532_READREGISTER;
	pn532_packetbuffer[1] = address >> 8;
	pn532_packetbuffer[2] = address & 0xff;

	if (!sendCommandCheckAck(pn532_packetbuffer, 3))
		return 0;

	// read data packet
	readspidata(pn532_packetbuffer, 2);
	*value = pn532_packetbuffer[0];
	return 1;
}

uint32_t PN532::writeRegister(uint16_t address, uint8_t value) {
	pn532_packetbuffer[0] = PN532_WRITEREGISTER;
	pn532_packetbuffer[1] = address >> 8;
	pn532_packetbuffer[2] = address & 0xff;
	pn532_packetbuffer[3] = value;

	if (!sendCommandCheckAck(pn532_packetbuffer, 4))
		return 0;

	return 1;
}

/************** high level SPI */


boolean PN532::spi_readack() {
	uint8_t ackbuff[6];

	readspidata(ackbuff, 6);

	return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

void PN532::spi_readack(uint8_t* ack){
	readspidata(ack, 6);
}

/************** mid level SPI */

uint8_t PN532::readspistatus(void) {
	digitalWrite(_ss, LOW);
	delay(2);
	spiwrite(PN532_SPI_STATREAD);
	// read byte
	uint8_t x = spiread();

	digitalWrite(_ss, HIGH);
	return x;
}

void PN532::readspidata(uint8_t* buff, uint8_t n) {
	digitalWrite(_ss, LOW);
	delay(2);
	spiwrite(PN532_SPI_DATAREAD);

	for (uint8_t i=0; i<n; i++) {
		//delay(1);
		buff[i] = spiread();
	}

	digitalWrite(_ss, HIGH);
}

void PN532::readspidataAnswer(uint8_t* buff, uint8_t& n) {
	digitalWrite(_ss, LOW);
	delay(2);
	spiwrite(PN532_SPI_DATAREAD);

	for (uint8_t i=0; i<5; i++) {
		delay(1);
		buff[i] = spiread();
	}
	if(buff[2] == 0xff)
		n = 5 + buff[3] + 2;
	else
		//hack this should never happen but sometime the first 
		//null byte is missing :(
		//don't know why i don't receive the first 0x00
		n = 5 + buff[2] + 2;
		
	for (uint8_t i=5; i<n; i++) {
		delay(1);
		buff[i] = spiread();
	}
	
	digitalWrite(_ss, HIGH);
}

void PN532::spiwriteraw(uint8_t* raw, uint8_t cmdlen) {

	digitalWrite(_ss, LOW);
	delay(2);     // or whatever the delay is for waking up the board
	spiwrite(PN532_SPI_DATAWRITE);

	for (uint8_t i=0; i<cmdlen; i++) {
		spiwrite(raw[i]);
	}

	digitalWrite(_ss, HIGH);
}

void PN532::spiwritecommand(uint8_t* cmd, uint8_t cmdlen) {
	uint8_t checksum;

	cmdlen++;

#ifdef PN532DEBUG
	Serial.print("\nSending: ");
#endif

	digitalWrite(_ss, LOW);
	delay(2);     // or whatever the delay is for waking up the board
	spiwrite(PN532_SPI_DATAWRITE);

	checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
	spiwrite(PN532_PREAMBLE);
	spiwrite(PN532_PREAMBLE);
	spiwrite(PN532_STARTCODE2);

	spiwrite(cmdlen);
	uint8_t cmdlen_1=~cmdlen + 1;
	spiwrite(cmdlen_1);

	spiwrite(PN532_HOSTTOPN532);
	checksum += PN532_HOSTTOPN532;

#ifdef PN532DEBUG
	Serial.print(" 0x"); Serial.print(PN532_PREAMBLE, HEX);
	Serial.print(" 0x"); Serial.print(PN532_PREAMBLE, HEX);
	Serial.print(" 0x"); Serial.print(PN532_STARTCODE2, HEX);
	Serial.print(" 0x"); Serial.print(cmdlen, HEX);
	Serial.print(" 0x"); Serial.print(cmdlen_1, HEX);
	Serial.print(" 0x"); Serial.print(PN532_HOSTTOPN532, HEX);
#endif

	for (uint8_t i=0; i<cmdlen-1; i++) {
		spiwrite(cmd[i]);
		checksum += cmd[i];
#ifdef PN532DEBUG
		Serial.print(" 0x"); Serial.print(cmd[i], HEX);
#endif
	}
	uint8_t checksum_1=~checksum;
	spiwrite(checksum_1);
	spiwrite(PN532_POSTAMBLE);
	digitalWrite(_ss, HIGH);

#ifdef PN532DEBUG
	Serial.print(" 0x"); Serial.print(checksum_1, HEX);
	Serial.print(" 0x"); Serial.print(PN532_POSTAMBLE, HEX);
	Serial.println();
#endif
}
/************** low level SPI */

void PN532::spiwrite(uint8_t c) {
	int8_t i;
	digitalWrite(_clk, HIGH);

	for (i=0; i<8; i++) {
		digitalWrite(_clk, LOW);
		if (c & _BV(i)) {
			digitalWrite(_mosi, HIGH);
		} else {
			digitalWrite(_mosi, LOW);
		}
		digitalWrite(_clk, HIGH);
	}
}

uint8_t PN532::spiread(void) {
	int8_t i, x;
	x = 0;
	digitalWrite(_clk, HIGH);

	for (i=0; i<8; i++) {
		if (digitalRead(_miso)) {
			x |= _BV(i);
		}
		digitalWrite(_clk, LOW);
		digitalWrite(_clk, HIGH);
	}
	return x;
}
