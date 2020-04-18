/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _B1LLI_H_
#define _B1LLI_H_
typedef struct avmb1_t4file {
  int len;
  unsigned char * data;
} avmb1_t4file;
typedef struct avmb1_loaddef {
  int contr;
  avmb1_t4file t4file;
} avmb1_loaddef;
typedef struct avmb1_loadandconfigdef {
  int contr;
  avmb1_t4file t4file;
  avmb1_t4file t4config;
} avmb1_loadandconfigdef;
typedef struct avmb1_resetdef {
  int contr;
} avmb1_resetdef;
typedef struct avmb1_getdef {
  int contr;
  int cardtype;
  int cardstate;
} avmb1_getdef;
typedef struct avmb1_carddef {
  int port;
  int irq;
} avmb1_carddef;
#define AVM_CARDTYPE_B1 0
#define AVM_CARDTYPE_T1 1
#define AVM_CARDTYPE_M1 2
#define AVM_CARDTYPE_M2 3
typedef struct avmb1_extcarddef {
  int port;
  int irq;
  int cardtype;
  int cardnr;
} avmb1_extcarddef;
#define AVMB1_LOAD 0
#define AVMB1_ADDCARD 1
#define AVMB1_RESETCARD 2
#define AVMB1_LOAD_AND_CONFIG 3
#define AVMB1_ADDCARD_WITH_TYPE 4
#define AVMB1_GET_CARDINFO 5
#define AVMB1_REMOVECARD 6
#define AVMB1_REGISTERCARD_IS_OBSOLETE
#endif
