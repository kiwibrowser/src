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

/*
** randpass.c - Random password generation module of PWGEN program
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "base/rand_util.h"
#include "randpass.h"
#include "smbl.h"

/*
** gen_rand_pass - generates random password of specified type
** INPUT:
**   char * - password string.
**   int    - minimum password length.
**   int    - maximum password length.
**   unsigned int - password generation mode.
** OUTPUT:
**   int - password length or -1 on error.
** NOTES:
**   none.
*/
int
gen_rand_pass (char *password_string, int minl, int maxl, unsigned int pass_mode)
{
  int i = 0;
  int j = 0;
  int length = 0;
  char *str_pointer;
  int random_weight[94];
  int max_weight = 0;
  int max_weight_element_number = 0;

  if (minl > APG_MAX_PASSWORD_LENGTH || maxl > APG_MAX_PASSWORD_LENGTH ||
      minl < 1 || maxl < 1 || minl > maxl)
      return (-1);
  for (i = 0; i <= 93; i++) random_weight[i] = 0;
  length = base::RandInt(minl, maxl);
  str_pointer = password_string;

  for (i = 0; i < length; i++)
    {
/* Asign random weight in weight array if mode is present*/
      for (j = 0; j <= 93 ; j++)
         if ( ( (pass_mode & smbl[j].type) > 0) &&
	     !( (S_RS & smbl[j].type) > 0))
           random_weight[j] = base::RandInt(1, 20000);
      j = 0;
/* Find an element with maximum weight */
      for (j = 0; j <= 93; j++)
	if (random_weight[j] > max_weight)
	  {
	    max_weight = random_weight[j];
	    max_weight_element_number = j;
	  }
/* Get password symbol */
      *str_pointer = smbl[max_weight_element_number].ch;
      str_pointer++;
      max_weight = 0;
      max_weight_element_number = 0;
      for (j = 0; j <= 93; j++) random_weight[j] = 0;
    }
  *str_pointer = 0;
  return (length);
}

/*
** gen_rand_symbol - generates random password of specified type
** INPUT:
**   char * - symbol.
**   unsigned int - symbol type.
** OUTPUT:
**   int - password length or -1 on error.
** NOTES:
**   none.
*/
int
gen_rand_symbol (char *symbol, unsigned int mode)
{
  int j = 0;
  char *str_pointer;
  int random_weight[94];
  int max_weight = 0;
  int max_weight_element_number = 0;

  for (j = 0; j <= 93; j++) random_weight[j] = 0; 
  str_pointer = symbol;
  j = 0;
/* Asign random weight in weight array if mode is present*/
  for (j = 0; j <= 93 ; j++)
     if ( ( (mode & smbl[j].type) > 0) &&
         !( (S_RS & smbl[j].type) > 0))
          random_weight[j] = base::RandInt(1, 20000);
  j = 0;
/* Find an element with maximum weight */
  for (j = 0; j <= 93; j++)
     if (random_weight[j] > max_weight)
       {
        max_weight = random_weight[j];
        max_weight_element_number = j;
       }
/* Get password symbol */
  *str_pointer = smbl[max_weight_element_number].ch;
  max_weight = 0;
  max_weight_element_number = 0;
  return (0);
}

/*
** is_restricted_symbol - detcts if symbol is restricted rigt now
** INPUT:
**   char - symbol.
** OUTPUT:
**   bool - false - not restricted
**          true - restricted
** NOTES:
**   none.
*/
bool
is_restricted_symbol (char symbol)
{
  int j = 0;
  for (j = 0; j <= 93 ; j++)
    if (symbol == smbl[j].ch)
      if ((S_RS & smbl[j].type) > 0)
        return(true);
  return(false);
}
