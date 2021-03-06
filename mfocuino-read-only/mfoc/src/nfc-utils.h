/*-
 * Public platform independent Near Field Communication (NFC) library examples
 *
 * Copyright (C) 2009, Roel Verdult
 * Copyright (C) 2010, Romuald Conty, Romain Tartière
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file nfc-utils.h
 * @brief Provide some examples shared functions like print, parity calculation, options parsing.
 */

#ifndef _EXAMPLES_NFC_UTILS_H_
#define _EXAMPLES_NFC_UTILS_H_


#ifdef __cplusplus
extern  "C" {
#endif

#include <stdlib.h>
#include <string.h>

/**
 * @macro DBG
 * @brief Print a message of standard output only in DEBUG mode
 */
#define DBG(...)

/**
 * @macro WARN
 * @brief Print a warn message
 */
#define WARN(...)

/**
 * @macro ERR
 * @brief Print a error message
 */
#define ERR(...)

void printHex(const uint8_t * data, const uint32_t numBytes);
void printHexChar(const uint8_t * data, const uint32_t numBytes);

void    print_nfc_iso14443a_info (const nfc_iso14443a_info nai, bool verbose);
void    print_nfc_iso14443b_info (const nfc_iso14443b_info nbi, bool verbose);
void    print_nfc_iso14443bi_info (const nfc_iso14443bi_info nii, bool verbose);
void    print_nfc_iso14443b2sr_info (const nfc_iso14443b2sr_info nsi, bool verbose);
void    print_nfc_iso14443b2ct_info (const nfc_iso14443b2ct_info nci, bool verbose);
void    print_nfc_felica_info (const nfc_felica_info nfi, bool verbose);
void    print_nfc_jewel_info (const nfc_jewel_info nji, bool verbose);
void    print_nfc_dep_info (const nfc_dep_info ndi, bool verbose);
const char * str_nfc_baud_rate (const nfc_baud_rate nbr);

void    print_nfc_target (const nfc_target nt, bool verbose);

#ifdef __cplusplus
}
#endif

#endif
