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
#ifndef KERNEL_IRDA_H
#define KERNEL_IRDA_H
#include <linux/types.h>
#include <linux/socket.h>
#define HINT_PNP 0x01
#define HINT_PDA 0x02
#define HINT_COMPUTER 0x04
#define HINT_PRINTER 0x08
#define HINT_MODEM 0x10
#define HINT_FAX 0x20
#define HINT_LAN 0x40
#define HINT_EXTENSION 0x80
#define HINT_TELEPHONY 0x01
#define HINT_FILE_SERVER 0x02
#define HINT_COMM 0x04
#define HINT_MESSAGE 0x08
#define HINT_HTTP 0x10
#define HINT_OBEX 0x20
#define CS_ASCII 0x00
#define CS_ISO_8859_1 0x01
#define CS_ISO_8859_2 0x02
#define CS_ISO_8859_3 0x03
#define CS_ISO_8859_4 0x04
#define CS_ISO_8859_5 0x05
#define CS_ISO_8859_6 0x06
#define CS_ISO_8859_7 0x07
#define CS_ISO_8859_8 0x08
#define CS_ISO_8859_9 0x09
#define CS_UNICODE 0xff
typedef enum {
  IRDA_TEKRAM_DONGLE = 0,
  IRDA_ESI_DONGLE = 1,
  IRDA_ACTISYS_DONGLE = 2,
  IRDA_ACTISYS_PLUS_DONGLE = 3,
  IRDA_GIRBIL_DONGLE = 4,
  IRDA_LITELINK_DONGLE = 5,
  IRDA_AIRPORT_DONGLE = 6,
  IRDA_OLD_BELKIN_DONGLE = 7,
  IRDA_EP7211_IR = 8,
  IRDA_MCP2120_DONGLE = 9,
  IRDA_ACT200L_DONGLE = 10,
  IRDA_MA600_DONGLE = 11,
  IRDA_TOIM3232_DONGLE = 12,
  IRDA_EP7211_DONGLE = 13,
} IRDA_DONGLE;
enum {
  IRDAPROTO_UNITDATA = 0,
  IRDAPROTO_ULTRA = 1,
  IRDAPROTO_MAX
};
#define SOL_IRLMP 266
#define SOL_IRTTP 266
#define IRLMP_ENUMDEVICES 1
#define IRLMP_IAS_SET 2
#define IRLMP_IAS_QUERY 3
#define IRLMP_HINTS_SET 4
#define IRLMP_QOS_SET 5
#define IRLMP_QOS_GET 6
#define IRLMP_MAX_SDU_SIZE 7
#define IRLMP_IAS_GET 8
#define IRLMP_IAS_DEL 9
#define IRLMP_HINT_MASK_SET 10
#define IRLMP_WAITDEVICE 11
#define IRTTP_MAX_SDU_SIZE IRLMP_MAX_SDU_SIZE
#define IAS_MAX_STRING 256
#define IAS_MAX_OCTET_STRING 1024
#define IAS_MAX_CLASSNAME 60
#define IAS_MAX_ATTRIBNAME 60
#define IAS_MAX_ATTRIBNUMBER 256
#define IAS_EXPORT_CLASSNAME 64
#define IAS_EXPORT_ATTRIBNAME 256
#define IAS_MISSING 0
#define IAS_INTEGER 1
#define IAS_OCT_SEQ 2
#define IAS_STRING 3
#define LSAP_ANY 0xff
struct sockaddr_irda {
  __kernel_sa_family_t sir_family;
  __u8 sir_lsap_sel;
  __u32 sir_addr;
  char sir_name[25];
};
struct irda_device_info {
  __u32 saddr;
  __u32 daddr;
  char info[22];
  __u8 charset;
  __u8 hints[2];
};
struct irda_device_list {
  __u32 len;
  struct irda_device_info dev[1];
};
struct irda_ias_set {
  char irda_class_name[IAS_EXPORT_CLASSNAME];
  char irda_attrib_name[IAS_EXPORT_ATTRIBNAME];
  unsigned int irda_attrib_type;
  union {
    unsigned int irda_attrib_int;
    struct {
      unsigned short len;
      __u8 octet_seq[IAS_MAX_OCTET_STRING];
    } irda_attrib_octet_seq;
    struct {
      __u8 len;
      __u8 charset;
      __u8 string[IAS_MAX_STRING];
    } irda_attrib_string;
  } attribute;
  __u32 daddr;
};
#define SIOCSDONGLE (SIOCDEVPRIVATE + 0)
#define SIOCGDONGLE (SIOCDEVPRIVATE + 1)
#define SIOCSBANDWIDTH (SIOCDEVPRIVATE + 2)
#define SIOCSMEDIABUSY (SIOCDEVPRIVATE + 3)
#define SIOCGMEDIABUSY (SIOCDEVPRIVATE + 4)
#define SIOCGRECEIVING (SIOCDEVPRIVATE + 5)
#define SIOCSMODE (SIOCDEVPRIVATE + 6)
#define SIOCGMODE (SIOCDEVPRIVATE + 7)
#define SIOCSDTRRTS (SIOCDEVPRIVATE + 8)
#define SIOCGQOS (SIOCDEVPRIVATE + 9)
#define IRNAMSIZ 16
struct if_irda_qos {
  unsigned long baudrate;
  unsigned short data_size;
  unsigned short window_size;
  unsigned short min_turn_time;
  unsigned short max_turn_time;
  unsigned char add_bofs;
  unsigned char link_disc;
};
struct if_irda_line {
  __u8 dtr;
  __u8 rts;
};
struct if_irda_req {
  union {
    char ifrn_name[IRNAMSIZ];
  } ifr_ifrn;
  union {
    struct if_irda_line ifru_line;
    struct if_irda_qos ifru_qos;
    unsigned short ifru_flags;
    unsigned int ifru_receiving;
    unsigned int ifru_mode;
    unsigned int ifru_dongle;
  } ifr_ifru;
};
#define ifr_baudrate ifr_ifru.ifru_qos.baudrate
#define ifr_receiving ifr_ifru.ifru_receiving
#define ifr_dongle ifr_ifru.ifru_dongle
#define ifr_mode ifr_ifru.ifru_mode
#define ifr_dtr ifr_ifru.ifru_line.dtr
#define ifr_rts ifr_ifru.ifru_line.rts
#define IRDA_NL_NAME "irda"
#define IRDA_NL_VERSION 1
enum irda_nl_commands {
  IRDA_NL_CMD_UNSPEC,
  IRDA_NL_CMD_SET_MODE,
  IRDA_NL_CMD_GET_MODE,
  __IRDA_NL_CMD_AFTER_LAST
};
#define IRDA_NL_CMD_MAX (__IRDA_NL_CMD_AFTER_LAST - 1)
enum nl80211_attrs {
  IRDA_NL_ATTR_UNSPEC,
  IRDA_NL_ATTR_IFNAME,
  IRDA_NL_ATTR_MODE,
  __IRDA_NL_ATTR_AFTER_LAST
};
#define IRDA_NL_ATTR_MAX (__IRDA_NL_ATTR_AFTER_LAST - 1)
#define IRDA_MODE_PRIMARY 0x1
#define IRDA_MODE_SECONDARY 0x2
#define IRDA_MODE_MONITOR 0x4
#endif
