/*
** This module uses code from the NIST implementation of  FIPS-181,
** but the algorythm is CHANGED and I think that I CAN
** copyright it. See copiright notes below.
*/

/*
** Copyright (c) 1999, 2000, 2001, 2002, 2003
** Adel I. Mirzazhanov. All rights reserved
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 
**     1.Redistributions of source code must retain the above copyright notice,
**       this list of conditions and the following disclaimer. 
**     2.Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution. 
**     3.The name of the author may not be used to endorse or promote products
**       derived from this software without specific prior written permission. 
** 		  
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND ANY EXPRESS
** OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN  NO  EVENT  SHALL THE AUTHOR BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE
** GOODS OR SERVICES;  LOSS OF USE,  DATA,  OR  PROFITS;  OR BUSINESS
** INTERRUPTION)  HOWEVER  CAUSED  AND  ON  ANY  THEORY OF LIABILITY,
** WHETHER  IN  CONTRACT,   STRICT   LIABILITY,  OR  TORT  (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef FIPS181_H
#define FIPS181_H	1

#define RULE_SIZE             (sizeof(rules)/sizeof(struct unit))
#define ALLOWED(flag)         (digram[units_in_syllable[current_unit -1]][unit] & (flag))

#define MAX_UNACCEPTABLE      20
#define MAX_RETRIES           (4 * (int) pwlen + RULE_SIZE)

#define NOT_BEGIN_SYLLABLE    010
#define NO_FINAL_SPLIT        04
#define VOWEL                 02
#define ALTERNATE_VOWEL       01
#define NO_SPECIAL_RULE       0

#define BEGIN                 0200
#define NOT_BEGIN             0100
#define BREAK                 040
#define PREFIX                020
#define ILLEGAL_PAIR          010
#define SUFFIX                04
#define END                   02
#define NOT_END               01
#define ANY_COMBINATION       0

#define S_NB    0x01 /* Numeric */
#define S_SS    0x02 /* Special */
#define S_CL    0x04 /* Capital */
#define S_SL    0x08 /* Small   */
#define S_RS    0x10 /* Restricted Symbol*/

#define APG_MAX_PASSWORD_LENGTH 255

extern int gen_pron_pass (char *word, char* hypenated_word,
                          unsigned short minlen, unsigned short maxlen, unsigned int pass_mode);
unsigned short  random_unit (unsigned short type);
unsigned short  get_random (unsigned short minlen, unsigned short maxlen);
bool have_initial_y (unsigned short *units, unsigned short unit_size);
bool illegal_placement (unsigned short *units, unsigned short pwlen);
bool improper_word (unsigned short *units, unsigned short word_size);
bool have_final_split (unsigned short *units, unsigned short unit_size);
int gen_word (char *word, char *hyphenated_word, unsigned short pwlen, unsigned int pass_mode);
char   	*gen_syllable(char *syllable, unsigned short pwlen, unsigned short *units_in_syllable,
                      unsigned short *syllable_length);

#endif /* FIPS181_H */
