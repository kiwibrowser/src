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

#include <stdlib.h>
#include <string.h>
#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32) && !defined(__WIN32__)
#include <strings.h>
#endif
#ifndef APGBFM
#include "fips181.h"
#include "randpass.h"
#endif

#include "base/rand_util.h"
#include "convert.h"

/*
** GLOBALS
*/

/* small letters */
char let[26] =
 {
 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
 'u', 'v', 'w', 'x', 'w', 'z'
 };
/* capital letters */
char clet[26] =
 {
 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
 'U', 'V', 'W', 'X', 'W', 'Z'
  };

/*
** FUNCTIONS
*/

/*
** decapitalize() - This routine replaces all capital letters
**                  to small letters in the word:
** INPUT:
**   char * - word.
** OUTPUT:
**   none.
** NOTES:
**   none.
*/
void
decapitalize (char *word)
{
 int i = 0; /* counter */
 int j = 0; /* counter */
 int str_len = (int) strlen(word);
 for(j = 0; j < str_len; j++)
  for(i=0; i < 26; i++)
   if(word[j] == clet[i])
       word[j] = let[i];
}

#ifndef APGBFM
/*
** capitalize() - This routine designed to modify sullable like this:
** adel ----> Adel
** dot  ----> Dot
** etc.
** INPUT:
**   char * - syllable.
** OUTPUT:
**   none.
** NOTES:
**   none.
*/
void
capitalize (char *syllable)
{
 char tmp = 0x00;
 int i = 0;
 if (base::RandInt(0, 1) == 1)
  {
   (void)memcpy((void *)&tmp, (void *)syllable, sizeof(tmp));
   for(i=0; i < 26; i++)
     if ( let[i] == tmp )
       if (!is_restricted_symbol(clet[i]))
         (void)memcpy ((void *)syllable, (void *)&clet[i], 1);
  }
}

/*
** numerize() - This routine designed to modify single-letter
** syllable like this:
** a ----> 1 or 2 or 3 etc.
** u ----> 1 or 2 or 3 etc.
** etc.
** INPUT:
**   char * - single-letter syllable
** OUTPUT:
**   none.
** NOTES:
**   none.
*/
void
numerize (char *syllable)
{
 char *tmp = (char *)calloc(1, 4);
 if ( strlen (syllable) == 1 )
      {
       (void) gen_rand_symbol(tmp, S_NB);
       (void)memcpy ((void *)syllable, (void *)tmp, 1);
      }
 free ((void *)tmp);
}
/*
** specialize() - This routine designed to modify single-letter syllable
** like this:
** a ----> # or $ or % etc.
** u ----> # or $ or % etc.
** etc.
** INPUT:
**   char * - single-letter syllable.
** OUTPUT:
**   none.
** NOTES:
**   none.
*/
void
specialize (char *syllable)
{
 char *tmp = (char *)calloc(1, 4);
 if ( strlen (syllable) == 1 )
      {
       (void) gen_rand_symbol(tmp, S_SS);
       (void)memcpy ((void *)syllable, (void *)tmp, 1);
      }
 free ((void *)tmp);
}

/*
** symb2name - convert symbol to it's name
** INPUT:
**   char * - one symbol syllable
** OUTPUT:
**   none.
** NOTES:
**   none.
*/
void
symb2name(char * syllable, char * h_syllable)
{
 struct ssymb_names
  {
   char symbol;
   const char * name;
  };
 static const struct ssymb_names ssn[42] =
  {
   {'1',"ONE"},
   {'2',"TWO"},
   {'3',"THREE"},
   {'4',"FOUR"},
   {'5',"FIVE"},
   {'6',"SIX"},
   {'7',"SEVEN"},
   {'8',"EIGHT"},
   {'9',"NINE"},
   {'0',"ZERO"},
   {33, "EXCLAMATION_POINT"},
   {34, "QUOTATION_MARK"},
   {35, "CROSSHATCH"},
   {36, "DOLLAR_SIGN"},
   {37, "PERCENT_SIGN"},
   {38, "AMPERSAND"},
   {39, "APOSTROPHE"},
   {40, "LEFT_PARENTHESIS"},
   {41, "RIGHT_PARENTHESIS"},
   {42, "ASTERISK"},
   {43, "PLUS_SIGN"},
   {44, "COMMA"},
   {45, "HYPHEN"},
   {46, "PERIOD"},
   {47, "SLASH"},
   {58, "COLON"},
   {59, "SEMICOLON"},
   {60, "LESS_THAN"},
   {61, "EQUAL_SIGN"},
   {62, "GREATER_THAN"},
   {63, "QUESTION_MARK"},
   {64, "AT_SIGN"},
   {91, "LEFT_BRACKET"},
   {92, "BACKSLASH"},
   {93, "RIGHT_BRACKET"},
   {94, "CIRCUMFLEX"},
   {95, "UNDERSCORE"},
   {96, "GRAVE"},
   {123, "LEFT_BRACE"},
   {124, "VERTICAL_BAR"},
   {125, "RIGHT_BRACE"},
   {126, "TILDE"}
  };
 int i = 0;
 bool flag = false;
 
 if (strlen(syllable) == 1)
    {
     for (i = 0; i < 42; i++)
      {
       if(*syllable == ssn[i].symbol)
        {
         (void)memcpy((void*)h_syllable, (void*)ssn[i].name, strlen(ssn[i].name));
	 flag = true;
        }
      }
     if (flag != true)
       (void)memcpy((void*)h_syllable, (void*)syllable, strlen(syllable));
    }
}

/*
** spell_word - spell the word
** INPUT:
**   char * - pointer to the word
**   char * - pointer to the spelled word
** OUTPUT:
**   char * - pointer to the spelled word
**    NULL  - something is wrong
** NOTES:
**   You should free() memory pointed by spelled_word after each use of spell_word
*/
char *
spell_word(char * word, char * spelled_word)
{
 struct char_spell
  {
   char symbol;
   const char *name;
  };
 static struct char_spell cs[94] =
  {
   {'1',"ONE"              },
   {'2',"TWO"              },
   {'3',"THREE"            },
   {'4',"FOUR"             },
   {'5',"FIVE"             },
   {'6',"SIX"              },
   {'7',"SEVEN"            },
   {'8',"EIGHT"            },
   {'9',"NINE"             },
   {'0',"ZERO"             },
   {'A', "Alfa"            },
   {'B', "Bravo"           },
   {'C', "Charlie"         },
   {'D', "Delta"           },
   {'E', "Echo"            },
   {'F', "Foxtrot"         },
   {'G', "Golf"            },
   {'H', "Hotel"           },
   {'I', "India"           },
   {'J', "Juliett"         },
   {'K', "Kilo"            },
   {'L', "Lima"            },
   {'M', "Mike"            },
   {'N', "November"        },
   {'O', "Oscar"           },
   {'P', "Papa"            },
   {'Q', "Quebec"          },
   {'R', "Romeo"           },
   {'S', "Sierra"          },
   {'T', "Tango"           },
   {'U', "Uniform"         },
   {'V', "Victor"          },
   {'W', "Whiskey"         },
   {'X', "X_ray"           },
   {'Y', "Yankee"          },
   {'Z', "Zulu"            },
   {'a', "alfa"            },
   {'b', "bravo"           },
   {'c', "charlie"         },
   {'d', "delta"           },
   {'e', "echo"            },
   {'f', "foxtrot"         },
   {'g', "golf"            },
   {'h', "hotel"           },
   {'i', "india"           },
   {'j', "juliett"         },
   {'k', "kilo"            },
   {'l', "lima"            },
   {'m', "mike"            },
   {'n', "november"        },
   {'o', "oscar"           },
   {'p', "papa"            },
   {'q', "quebec"          },
   {'r', "romeo"           },
   {'s', "sierra"          },
   {'t', "tango"           },
   {'u', "uniform"         },
   {'v', "victor"          },
   {'w', "whiskey"         },
   {'x', "x_ray"           },
   {'y', "yankee"          },
   {'z', "zulu"            },
   {33, "EXCLAMATION_POINT"},
   {34, "QUOTATION_MARK"   },
   {35, "CROSSHATCH"       },
   {36, "DOLLAR_SIGN"      },
   {37, "PERCENT_SIGN"     },
   {38, "AMPERSAND"        },
   {39, "APOSTROPHE"       },
   {40, "LEFT_PARENTHESIS" },
   {41, "RIGHT_PARENTHESIS"},
   {42, "ASTERISK"         },
   {43, "PLUS_SIGN"        },
   {44, "COMMA"            },
   {45, "HYPHEN"           },
   {46, "PERIOD"           },
   {47, "SLASH"            },
   {58, "COLON"            },
   {59, "SEMICOLON"        },
   {60, "LESS_THAN"        },
   {61, "EQUAL_SIGN"       },
   {62, "GREATER_THAN"     },
   {63, "QUESTION_MARK"    },
   {64, "AT_SIGN"          },
   {91, "LEFT_BRACKET"     },
   {92, "BACKSLASH"        },
   {93, "RIGHT_BRACKET"    },
   {94, "CIRCUMFLEX"       },
   {95, "UNDERSCORE"       },
   {96, "GRAVE"            },
   {123, "LEFT_BRACE"      },
   {124, "VERTICAL_BAR"    },
   {125, "RIGHT_BRACE"     },
   {126, "TILDE"           }
  };
  int s_length = 0;
  int i = 0;
  int j = 0;
  int word_len = (int) strlen(word);
  char * tmp_ptr;
  char hyphen = '-';
  char zero   = 0x00;
  
  /* Count the length of the spelled word */
  for (i=0; i <= word_len; i++)
   for (j=0; j < 94; j++)
    if (word[i] == cs[j].symbol)
     {
      s_length = s_length + (int) strlen(cs[j].name) + 1;
      continue;
     }

  /* Allocate memory for spelled word */
  if ( (spelled_word = (char *)calloc(1, (size_t)s_length)) == NULL)
    return(NULL);

  /* Construct spelled word */
  tmp_ptr = spelled_word;

  for (i=0; i < word_len; i++)
   for (j=0; j < 94; j++)
    if (word[i] == cs[j].symbol)
     {
      (void) memcpy((void *)tmp_ptr, (void *)cs[j].name, strlen(cs[j].name));
      tmp_ptr = tmp_ptr + strlen(cs[j].name);
      /* Place the hyphen after each symbol */
      (void) memcpy((void *)(tmp_ptr), (void *)&hyphen, 1);
      tmp_ptr = tmp_ptr + 1;
      continue;
     }

  /* Remove hyphen at the end of the word */
  tmp_ptr = tmp_ptr - 1;
  (void) memcpy((void *)(tmp_ptr), (void *)&zero, 1);

  return (spelled_word);
}

#endif /* APGBFM */
