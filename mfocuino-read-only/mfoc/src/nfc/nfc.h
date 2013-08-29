/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010-2012, Romuald Conty
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file nfc.h
 * @brief libnfc interface
 *
 * Provide all usefull functions (API) to handle NFC devices.
 * Simplified Hearder for mfoc
 */

#ifndef _LIBNFC_H_
#define _LIBNFC_H_

#include <nfc/nfc-types.h>

#ifdef __cplusplus
extern  "C" {
#endif

  /* Library initialization/deinitialization */
void nfc_init(nfc_context *context);
void nfc_exit(nfc_context *context);

  /* NFC Device/Hardware manipulation */
nfc_device *nfc_open(nfc_context *context, const nfc_connstring connstring);
void nfc_close(nfc_device *pnd);

  /* NFC initiator: act as "reader" */
int nfc_initiator_init(nfc_device *pnd);
int nfc_initiator_select_passive_target(nfc_device *pnd, const nfc_modulation nm, const uint8_t *pbtInitData, const size_t szInitData, nfc_target *pnt);
int nfc_initiator_transceive_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx, const size_t szRx, int timeout);
int nfc_initiator_transceive_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtRx, uint8_t *pbtRxPar);

  /* NFC target: act as tag (i.e. MIFARE Classic) or NFC target device. */

  /* Error reporting */
void nfc_perror(nfc_device *pnd, int error);

  /* Special data accessors */

  /* Properties accessors */
int nfc_device_set_property_bool(nfc_device *pnd, const nfc_property property, const bool bEnable);

  /* Misc. functions */
void iso14443a_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc);
void iso14443a_crc_append(uint8_t *pbtData, size_t szLen);

  /* String converter functions */
const char *str_nfc_baud_rate(const nfc_baud_rate nbr);


  /* Error codes */
  /** @ingroup error
   * @hideinitializer
   * Success (no error)
   */
#define NFC_SUCCESS			 0
  /** @ingroup error
   * @hideinitializer
   * Input / output error, device may not be usable anymore without re-open it
   */
#define NFC_EIO				-1
  /** @ingroup error
   * @hideinitializer
   * Invalid argument(s)
   */
#define NFC_EINVARG			-2
  /** @ingroup error
   * @hideinitializer
   *  Operation not supported by device
   */
#define NFC_EDEVNOTSUPP			-3
  /** @ingroup error
   * @hideinitializer
   * No such device
   */
#define NFC_ENOTSUCHDEV			-4
  /** @ingroup error
   * @hideinitializer
   * Buffer overflow
   */
#define NFC_EOVFLOW			-5
  /** @ingroup error
   * @hideinitializer
   * Operation timed out
   */
#define NFC_ETIMEOUT			-6
  /** @ingroup error
   * @hideinitializer
   * Operation aborted (by user)
   */
#define NFC_EOPABORTED			-7
  /** @ingroup error
   * @hideinitializer
   * Not (yet) implemented
   */
#define NFC_ENOTIMPL			-8
  /** @ingroup error
   * @hideinitializer
   * Target released
   */
#define NFC_ETGRELEASED			-10
  /** @ingroup error
   * @hideinitializer
   * Error while RF transmission
   */
#define NFC_ERFTRANS			-20
  /** @ingroup error
   * @hideinitializer
   * Software error (allocation, file/pipe creation, etc.)
   */
#define NFC_ESOFT			-80
  /** @ingroup error
   * @hideinitializer
   * Device's internal chip error
   */
#define NFC_ECHIP			-90


#ifdef __cplusplus
}
#endif                        // __cplusplus
#endif                          // _LIBNFC_H_
