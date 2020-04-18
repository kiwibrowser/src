#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main(int argc, char **argv)
{

/*   input      aaabcd ef g
                012345 67 8
     pass0      a  bcddef g
                0  345567 8
     pass1      x   cddw  g
                0   4556  8
     pass2      zy    dw  g
                00    56  8
     pass3      zy    deeeg
                00    56668
*/

  int result = 0;

  const char* txt = "aaabcdefg";

  const char* table1 = "inpos_pass0.ctb";
  const char* table2 = "inpos_pass0.ctb,inpos_pass1.ctb";
  const char* table3 = "inpos_pass0.ctb,inpos_pass1.ctb,inpos_pass2.ctb";
  const char* table4 = "inpos_pass0.ctb,inpos_pass1.ctb,inpos_pass2.ctb,inpos_pass3.ctb";

  const char* brl1 = "abcddefg";
  const char* brl2 = "xcddwg";
  const char* brl3 = "zydwg";
  const char* brl4 = "zydeeeg";

  const int inpos1[] = {0,3,4,5,5,6,7,8};
  const int inpos2[] = {0,4,5,5,6,8};
  const int inpos3[] = {0,0,5,6,8};
  const int inpos4[] = {0,0,5,6,6,6,8};

  result |= check_inpos(table1, txt, inpos1);
  result |= check_inpos(table2, txt, inpos2);
  result |= check_inpos(table3, txt, inpos3);
  result |= check_inpos(table4, txt, inpos4);

  result |= check_translation(table1, txt, NULL, brl1);
  result |= check_translation(table2, txt, NULL, brl2);
  result |= check_translation(table3, txt, NULL, brl3);
  result |= check_translation(table4, txt, NULL, brl4);

  return result;

}
