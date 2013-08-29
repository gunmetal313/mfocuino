/**
 *
 * Copyright (C) 2011 Christophe Duvernois <christophe.duvernois@gmail.com>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
 
/**
 * Arduino driver code to allow libnfc to drive the nfc shield with the uart driver
 * ./configure --with-drivers=pn532_uart --enable-serial-autoprobe (--enable-debug)
 */

#include <PN532.h>

#define SCK 13
#define MOSI 11
#define SS 10
#define MISO 12

PN532 nfc(SCK, MISO, MOSI, SS);

uint8_t buffer[32];

void setup(void) {
    Serial.begin(921600); //460800, 115200
	Serial.flush();
    nfc.begin();
}

void loop(void) {
	
	int b = Serial.available();
	if (b >= 5){
		Serial.readBytes((char*)buffer, 5);
		if(buffer[0] == 0x55){
			//handle wake up case
			while(Serial.available() < 5); //wait the command
			b = Serial.readBytes((char*)buffer+5, 5);
			//send raw command to pn532
			//get length of package : 
			// (PREAMBLE + START + LEN + LCS) + (TFI + DATA[0...n]) + (DCS + POSTAMBLE)
			uint8_t l = buffer[8] + 2;
			while(Serial.available() < l); //wait the command
			//read command from uart
			Serial.readBytes((char*)buffer+10, l);
			//send raw command to pn532
			nfc.sendRawCommandCheckAck(buffer, l+10);
			//read pn532 answer
			nfc.readRawCommandAnswer(buffer, l+10);
		}else{
			//normal case
			//get length of package : 
			// (PREAMBLE + START + LEN + LCS) + (TFI + DATA[0...n]) + (DCS + POSTAMBLE)
			uint8_t l = buffer[3] + 2;
			//read command from uart
			
			//while(Serial.available() < l); //wait the command
			//Serial.readBytes((char*)buffer+5, l);
			
			uint8_t br = l;
			uint8_t* bufptr = buffer + 5;
			while(br){
				if(Serial.available()){
					*bufptr++ = Serial.read();
					--br;
				}
			}
			
			//send raw command to pn532
			nfc.sendRawCommandCheckAck(buffer, l+5);
			//read pn532 answer
			nfc.readRawCommandAnswer(buffer, l+5);
		}
	}
	
}


