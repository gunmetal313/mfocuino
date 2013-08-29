#include <PN532/PN532.h>
#include "nfc.h"

extern PN532 nfc;

int nfc_initiator_init(nfc_device *pnd) {
	return 0;
}

int nfc_device_set_property_bool(nfc_device *pnd, const nfc_property property, const bool bEnable) {
	return 0;
}

void nfc_perror(nfc_device *pnd, int error) {
	Serial.print("Error :");
	Serial.println(error, DEC);
    nfc_close(pnd);
    nfc_exit(NULL);
}

int nfc_initiator_transceive_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, int timeout) {
	/*byte_t  abtRx[PN53x_EXTENDED_FRAME_MAX_LEN];
	size_t  szRx = PN53x_EXTENDED_FRAME_MAX_LEN;
	size_t  szExtraTxLen;
	byte_t  abtCmd[sizeof (pncmd_initiator_exchange_raw_data)];

	// We can not just send bytes without parity if while the PN53X expects we handled them
	if (!pnd->bPar)
		return false;

	// Copy the data into the command frame
	if (pnd->bEasyFraming) {
		memcpy (abtCmd, pncmd_initiator_exchange_data, sizeof (pncmd_initiator_exchange_data));
		abtCmd[2] = 1;              // target number 
		memcpy (abtCmd + 3, pbtTx, szTx);
		szExtraTxLen = 3;
	} else {
		memcpy (abtCmd, pncmd_initiator_exchange_raw_data, sizeof (pncmd_initiator_exchange_raw_data));
		memcpy (abtCmd + 2, pbtTx, szTx);
		szExtraTxLen = 2;
	}

	// To transfer command frames bytes we can not have any leading bits, reset this to zero
	//if (!pn53x_set_tx_bits (pnd, 0))
	//	return false;
	#  define REG_CIU_BIT_FRAMING       0x633D
	#  define SYMBOL_TX_LAST_BITS       0x07
	uint8_t reg = 0;
	nfc.readRegister(0x633D, &reg);
	reg &= ~0x07;
	nfc.writeRegister(0x633D, reg);

	// Send the frame to the PN53X chip and get the answer
	// We have to give the amount of bytes + (the two command bytes 0xD4, 0x42)
	if (!pn53x_transceive (pnd, abtCmd, szTx + szExtraTxLen, abtRx, &szRx))
		return false;

	// Save the received byte count
	*pszRx = szRx - 1;

	// Copy the received bytes
	memcpy (pbtRx, abtRx + 1, *pszRx);

	// Everything went successful
	return true;
	*/
	return 0;
}

int nfc_initiator_transceive_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtRx, uint8_t *pbtRxPar) {
	return 0;
}

void iso14443a_crc_append(uint8_t *pbtData, size_t szLen) {

}

int nfc_initiator_select_passive_target(nfc_device *pnd, const nfc_modulation nm, const uint8_t *pbtInitData, const size_t szInitData, nfc_target *pnt) {
	return 0;
}

void nfc_init(nfc_context *context) {

}

void nfc_exit(nfc_context *context) {

}

nfc_device *nfc_open(nfc_context *context, const nfc_connstring connstring) {
	return 0;
}

void nfc_close(nfc_device *pnd) {

}
