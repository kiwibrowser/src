#include "brl_checks.h"

int main(int argc, char **argv)
{
  return check_hyphenation("en-us-g1.ctb,hyph_en_US.dic", "straightforward", "010000001001000");
}
