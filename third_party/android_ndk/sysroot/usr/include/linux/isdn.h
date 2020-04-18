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
#ifndef _UAPI__ISDN_H__
#define _UAPI__ISDN_H__
#include <linux/ioctl.h>
#include <linux/tty.h>
#define ISDN_MAX_DRIVERS 32
#define ISDN_MAX_CHANNELS 64
#define IIOCNETAIF _IO('I', 1)
#define IIOCNETDIF _IO('I', 2)
#define IIOCNETSCF _IO('I', 3)
#define IIOCNETGCF _IO('I', 4)
#define IIOCNETANM _IO('I', 5)
#define IIOCNETDNM _IO('I', 6)
#define IIOCNETGNM _IO('I', 7)
#define IIOCGETSET _IO('I', 8)
#define IIOCSETSET _IO('I', 9)
#define IIOCSETVER _IO('I', 10)
#define IIOCNETHUP _IO('I', 11)
#define IIOCSETGST _IO('I', 12)
#define IIOCSETBRJ _IO('I', 13)
#define IIOCSIGPRF _IO('I', 14)
#define IIOCGETPRF _IO('I', 15)
#define IIOCSETPRF _IO('I', 16)
#define IIOCGETMAP _IO('I', 17)
#define IIOCSETMAP _IO('I', 18)
#define IIOCNETASL _IO('I', 19)
#define IIOCNETDIL _IO('I', 20)
#define IIOCGETCPS _IO('I', 21)
#define IIOCGETDVR _IO('I', 22)
#define IIOCNETLCR _IO('I', 23)
#define IIOCNETDWRSET _IO('I', 24)
#define IIOCNETALN _IO('I', 32)
#define IIOCNETDLN _IO('I', 33)
#define IIOCNETGPN _IO('I', 34)
#define IIOCDBGVAR _IO('I', 127)
#define IIOCDRVCTL _IO('I', 128)
#define SIOCGKEEPPERIOD (SIOCDEVPRIVATE + 0)
#define SIOCSKEEPPERIOD (SIOCDEVPRIVATE + 1)
#define SIOCGDEBSERINT (SIOCDEVPRIVATE + 2)
#define SIOCSDEBSERINT (SIOCDEVPRIVATE + 3)
#define ISDN_NET_ENCAP_ETHER 0
#define ISDN_NET_ENCAP_RAWIP 1
#define ISDN_NET_ENCAP_IPTYP 2
#define ISDN_NET_ENCAP_CISCOHDLC 3
#define ISDN_NET_ENCAP_SYNCPPP 4
#define ISDN_NET_ENCAP_UIHDLC 5
#define ISDN_NET_ENCAP_CISCOHDLCK 6
#define ISDN_NET_ENCAP_X25IFACE 7
#define ISDN_NET_ENCAP_MAX_ENCAP ISDN_NET_ENCAP_X25IFACE
#define ISDN_USAGE_NONE 0
#define ISDN_USAGE_RAW 1
#define ISDN_USAGE_MODEM 2
#define ISDN_USAGE_NET 3
#define ISDN_USAGE_VOICE 4
#define ISDN_USAGE_FAX 5
#define ISDN_USAGE_MASK 7
#define ISDN_USAGE_DISABLED 32
#define ISDN_USAGE_EXCLUSIVE 64
#define ISDN_USAGE_OUTGOING 128
#define ISDN_MODEM_NUMREG 24
#define ISDN_LMSNLEN 255
#define ISDN_CMSGLEN 50
#define ISDN_MSNLEN 32
#define NET_DV 0x06
#define TTY_DV 0x06
#define INF_DV 0x01
typedef struct {
  char drvid[25];
  unsigned long arg;
} isdn_ioctl_struct;
typedef struct {
  char name[10];
  char phone[ISDN_MSNLEN];
  int outgoing;
} isdn_net_ioctl_phone;
typedef struct {
  char name[10];
  char master[10];
  char slave[10];
  char eaz[256];
  char drvid[25];
  int onhtime;
  int charge;
  int l2_proto;
  int l3_proto;
  int p_encap;
  int exclusive;
  int dialmax;
  int slavedelay;
  int cbdelay;
  int chargehup;
  int ihup;
  int secure;
  int callback;
  int cbhup;
  int pppbind;
  int chargeint;
  int triggercps;
  int dialtimeout;
  int dialwait;
  int dialmode;
} isdn_net_ioctl_cfg;
#define ISDN_NET_DIALMODE_MASK 0xC0
#define ISDN_NET_DM_OFF 0x00
#define ISDN_NET_DM_MANUAL 0x40
#define ISDN_NET_DM_AUTO 0x80
#define ISDN_NET_DIALMODE(x) ((& (x))->flags & ISDN_NET_DIALMODE_MASK)
#endif
