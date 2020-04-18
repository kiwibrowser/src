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
#ifndef __LINUX_IXJUSER_H
#define __LINUX_IXJUSER_H
#include <linux/telephony.h>
#define IXJCTL_DSP_RESET _IO('q', 0xC0)
#define IXJCTL_RING PHONE_RING
#define IXJCTL_HOOKSTATE PHONE_HOOKSTATE
#define IXJCTL_MAXRINGS PHONE_MAXRINGS
#define IXJCTL_RING_CADENCE PHONE_RING_CADENCE
#define IXJCTL_RING_START PHONE_RING_START
#define IXJCTL_RING_STOP PHONE_RING_STOP
#define IXJCTL_CARDTYPE _IOR('q', 0xC1, int)
#define IXJCTL_SERIAL _IOR('q', 0xC2, int)
#define IXJCTL_DSP_TYPE _IOR('q', 0xC3, int)
#define IXJCTL_DSP_VERSION _IOR('q', 0xC4, int)
#define IXJCTL_VERSION _IOR('q', 0xDA, char *)
#define IXJCTL_DSP_IDLE _IO('q', 0xC5)
#define IXJCTL_TESTRAM _IO('q', 0xC6)
#define IXJCTL_REC_CODEC PHONE_REC_CODEC
#define IXJCTL_REC_START PHONE_REC_START
#define IXJCTL_REC_STOP PHONE_REC_STOP
#define IXJCTL_REC_DEPTH PHONE_REC_DEPTH
#define IXJCTL_FRAME PHONE_FRAME
#define IXJCTL_REC_VOLUME PHONE_REC_VOLUME
#define IXJCTL_REC_LEVEL PHONE_REC_LEVEL
typedef enum {
  f300_640 = 4,
  f300_500,
  f1100,
  f350,
  f400,
  f480,
  f440,
  f620,
  f20_50,
  f133_200,
  f300,
  f300_420,
  f330,
  f300_425,
  f330_440,
  f340,
  f350_400,
  f350_440,
  f350_450,
  f360,
  f380_420,
  f392,
  f400_425,
  f400_440,
  f400_450,
  f420,
  f425,
  f425_450,
  f425_475,
  f435,
  f440_450,
  f440_480,
  f445,
  f450,
  f452,
  f475,
  f480_620,
  f494,
  f500,
  f520,
  f523,
  f525,
  f540_660,
  f587,
  f590,
  f600,
  f660,
  f700,
  f740,
  f750,
  f750_1450,
  f770,
  f800,
  f816,
  f850,
  f857_1645,
  f900,
  f900_1300,
  f935_1215,
  f941_1477,
  f942,
  f950,
  f950_1400,
  f975,
  f1000,
  f1020,
  f1050,
  f1100_1750,
  f1140,
  f1200,
  f1209,
  f1330,
  f1336,
  lf1366,
  f1380,
  f1400,
  f1477,
  f1600,
  f1633_1638,
  f1800,
  f1860
} IXJ_FILTER_FREQ;
typedef struct {
  unsigned int filter;
  IXJ_FILTER_FREQ freq;
  char enable;
} IXJ_FILTER;
typedef struct {
  char enable;
  char en_filter;
  unsigned int filter;
  unsigned int on1;
  unsigned int off1;
  unsigned int on2;
  unsigned int off2;
  unsigned int on3;
  unsigned int off3;
} IXJ_FILTER_CADENCE;
#define IXJCTL_SET_FILTER _IOW('q', 0xC7, IXJ_FILTER *)
#define IXJCTL_SET_FILTER_RAW _IOW('q', 0xDD, IXJ_FILTER_RAW *)
#define IXJCTL_GET_FILTER_HIST _IOW('q', 0xC8, int)
#define IXJCTL_FILTER_CADENCE _IOW('q', 0xD6, IXJ_FILTER_CADENCE *)
#define IXJCTL_PLAY_CID _IO('q', 0xD7)
typedef enum {
  hz20 = 0x7ffa,
  hz50 = 0x7fe5,
  hz133 = 0x7f4c,
  hz200 = 0x7e6b,
  hz261 = 0x7d50,
  hz277 = 0x7cfa,
  hz293 = 0x7c9f,
  hz300 = 0x7c75,
  hz311 = 0x7c32,
  hz329 = 0x7bbf,
  hz330 = 0x7bb8,
  hz340 = 0x7b75,
  hz349 = 0x7b37,
  hz350 = 0x7b30,
  hz360 = 0x7ae9,
  hz369 = 0x7aa8,
  hz380 = 0x7a56,
  hz392 = 0x79fa,
  hz400 = 0x79bb,
  hz415 = 0x7941,
  hz420 = 0x7918,
  hz425 = 0x78ee,
  hz435 = 0x7899,
  hz440 = 0x786d,
  hz445 = 0x7842,
  hz450 = 0x7815,
  hz452 = 0x7803,
  hz466 = 0x7784,
  hz475 = 0x7731,
  hz480 = 0x7701,
  hz493 = 0x7685,
  hz494 = 0x767b,
  hz500 = 0x7640,
  hz520 = 0x7578,
  hz523 = 0x7559,
  hz525 = 0x7544,
  hz540 = 0x74a7,
  hz554 = 0x7411,
  hz587 = 0x72a1,
  hz590 = 0x727f,
  hz600 = 0x720b,
  hz620 = 0x711e,
  hz622 = 0x7106,
  hz659 = 0x6f3b,
  hz660 = 0x6f2e,
  hz698 = 0x6d3d,
  hz700 = 0x6d22,
  hz739 = 0x6b09,
  hz740 = 0x6afa,
  hz750 = 0x6a6c,
  hz770 = 0x694b,
  hz783 = 0x688b,
  hz800 = 0x678d,
  hz816 = 0x6698,
  hz830 = 0x65bf,
  hz850 = 0x6484,
  hz857 = 0x6414,
  hz880 = 0x629f,
  hz900 = 0x6154,
  hz932 = 0x5f35,
  hz935 = 0x5f01,
  hz941 = 0x5e9a,
  hz942 = 0x5e88,
  hz950 = 0x5dfd,
  hz975 = 0x5c44,
  hz1000 = 0x5a81,
  hz1020 = 0x5912,
  hz1050 = 0x56e2,
  hz1100 = 0x5320,
  hz1140 = 0x5007,
  hz1200 = 0x4b3b,
  hz1209 = 0x4a80,
  hz1215 = 0x4a02,
  hz1250 = 0x471c,
  hz1300 = 0x42e0,
  hz1330 = 0x4049,
  hz1336 = 0x3fc4,
  hz1366 = 0x3d22,
  hz1380 = 0x3be4,
  hz1400 = 0x3a1b,
  hz1450 = 0x3596,
  hz1477 = 0x331c,
  hz1500 = 0x30fb,
  hz1600 = 0x278d,
  hz1633 = 0x2462,
  hz1638 = 0x23e7,
  hz1645 = 0x233a,
  hz1750 = 0x18f8,
  hz1800 = 0x1405,
  hz1860 = 0xe0b,
  hz2100 = 0xf5f6,
  hz2130 = 0xf2f5,
  hz2450 = 0xd3b3,
  hz2750 = 0xb8e4
} IXJ_FREQ;
typedef enum {
  C1 = hz261,
  CS1 = hz277,
  D1 = hz293,
  DS1 = hz311,
  E1 = hz329,
  F1 = hz349,
  FS1 = hz369,
  G1 = hz392,
  GS1 = hz415,
  A1 = hz440,
  AS1 = hz466,
  B1 = hz493,
  C2 = hz523,
  CS2 = hz554,
  D2 = hz587,
  DS2 = hz622,
  E2 = hz659,
  F2 = hz698,
  FS2 = hz739,
  G2 = hz783,
  GS2 = hz830,
  A2 = hz880,
  AS2 = hz932,
} IXJ_NOTE;
typedef struct {
  int tone_index;
  int freq0;
  int gain0;
  int freq1;
  int gain1;
} IXJ_TONE;
#define IXJCTL_INIT_TONE _IOW('q', 0xC9, IXJ_TONE *)
typedef struct {
  int index;
  int tone_on_time;
  int tone_off_time;
  int freq0;
  int gain0;
  int freq1;
  int gain1;
} IXJ_CADENCE_ELEMENT;
typedef enum {
  PLAY_ONCE,
  REPEAT_LAST_ELEMENT,
  REPEAT_ALL
} IXJ_CADENCE_TERM;
typedef struct {
  int elements_used;
  IXJ_CADENCE_TERM termination;
  IXJ_CADENCE_ELEMENT __user * ce;
} IXJ_CADENCE;
#define IXJCTL_TONE_CADENCE _IOW('q', 0xCA, IXJ_CADENCE *)
#define IXJCTL_PLAY_CODEC PHONE_PLAY_CODEC
#define IXJCTL_PLAY_START PHONE_PLAY_START
#define IXJCTL_PLAY_STOP PHONE_PLAY_STOP
#define IXJCTL_PLAY_DEPTH PHONE_PLAY_DEPTH
#define IXJCTL_PLAY_VOLUME PHONE_PLAY_VOLUME
#define IXJCTL_PLAY_LEVEL PHONE_PLAY_LEVEL
#define IXJCTL_AEC_START _IOW('q', 0xCB, int)
#define IXJCTL_AEC_STOP _IO('q', 0xCC)
#define IXJCTL_AEC_GET_LEVEL _IO('q', 0xCD)
#define AEC_OFF 0
#define AEC_LOW 1
#define AEC_MED 2
#define AEC_HIGH 3
#define AEC_AUTO 4
#define AEC_AGC 5
#define IXJCTL_DTMF_READY PHONE_DTMF_READY
#define IXJCTL_GET_DTMF PHONE_GET_DTMF
#define IXJCTL_GET_DTMF_ASCII PHONE_GET_DTMF_ASCII
#define IXJCTL_DTMF_OOB PHONE_DTMF_OOB
#define IXJCTL_EXCEPTION PHONE_EXCEPTION
#define IXJCTL_PLAY_TONE PHONE_PLAY_TONE
#define IXJCTL_SET_TONE_ON_TIME PHONE_SET_TONE_ON_TIME
#define IXJCTL_SET_TONE_OFF_TIME PHONE_SET_TONE_OFF_TIME
#define IXJCTL_GET_TONE_ON_TIME PHONE_GET_TONE_ON_TIME
#define IXJCTL_GET_TONE_OFF_TIME PHONE_GET_TONE_OFF_TIME
#define IXJCTL_GET_TONE_STATE PHONE_GET_TONE_STATE
#define IXJCTL_BUSY PHONE_BUSY
#define IXJCTL_RINGBACK PHONE_RINGBACK
#define IXJCTL_DIALTONE PHONE_DIALTONE
#define IXJCTL_CPT_STOP PHONE_CPT_STOP
#define IXJCTL_SET_LED _IOW('q', 0xCE, int)
#define IXJCTL_MIXER _IOW('q', 0xCF, int)
#define MIXER_MASTER_L 0x0000
#define MIXER_MASTER_R 0x0100
#define ATT00DB 0x00
#define ATT02DB 0x01
#define ATT04DB 0x02
#define ATT06DB 0x03
#define ATT08DB 0x04
#define ATT10DB 0x05
#define ATT12DB 0x06
#define ATT14DB 0x07
#define ATT16DB 0x08
#define ATT18DB 0x09
#define ATT20DB 0x0A
#define ATT22DB 0x0B
#define ATT24DB 0x0C
#define ATT26DB 0x0D
#define ATT28DB 0x0E
#define ATT30DB 0x0F
#define ATT32DB 0x10
#define ATT34DB 0x11
#define ATT36DB 0x12
#define ATT38DB 0x13
#define ATT40DB 0x14
#define ATT42DB 0x15
#define ATT44DB 0x16
#define ATT46DB 0x17
#define ATT48DB 0x18
#define ATT50DB 0x19
#define ATT52DB 0x1A
#define ATT54DB 0x1B
#define ATT56DB 0x1C
#define ATT58DB 0x1D
#define ATT60DB 0x1E
#define ATT62DB 0x1F
#define MASTER_MUTE 0x80
#define MIXER_PORT_CD_L 0x0600
#define MIXER_PORT_CD_R 0x0700
#define MIXER_PORT_LINE_IN_L 0x0800
#define MIXER_PORT_LINE_IN_R 0x0900
#define MIXER_PORT_POTS_REC 0x0C00
#define MIXER_PORT_MIC 0x0E00
#define GAIN12DB 0x00
#define GAIN10DB 0x01
#define GAIN08DB 0x02
#define GAIN06DB 0x03
#define GAIN04DB 0x04
#define GAIN02DB 0x05
#define GAIN00DB 0x06
#define GAIN_02DB 0x07
#define GAIN_04DB 0x08
#define GAIN_06DB 0x09
#define GAIN_08DB 0x0A
#define GAIN_10DB 0x0B
#define GAIN_12DB 0x0C
#define GAIN_14DB 0x0D
#define GAIN_16DB 0x0E
#define GAIN_18DB 0x0F
#define GAIN_20DB 0x10
#define GAIN_22DB 0x11
#define GAIN_24DB 0x12
#define GAIN_26DB 0x13
#define GAIN_28DB 0x14
#define GAIN_30DB 0x15
#define GAIN_32DB 0x16
#define GAIN_34DB 0x17
#define GAIN_36DB 0x18
#define GAIN_38DB 0x19
#define GAIN_40DB 0x1A
#define GAIN_42DB 0x1B
#define GAIN_44DB 0x1C
#define GAIN_46DB 0x1D
#define GAIN_48DB 0x1E
#define GAIN_50DB 0x1F
#define INPUT_MUTE 0x80
#define MIXER_PORT_POTS_PLAY 0x0F00
#define POTS_ATT_00DB 0x00
#define POTS_ATT_04DB 0x01
#define POTS_ATT_08DB 0x02
#define POTS_ATT_12DB 0x03
#define POTS_ATT_16DB 0x04
#define POTS_ATT_20DB 0x05
#define POTS_ATT_24DB 0x06
#define POTS_ATT_28DB 0x07
#define POTS_MUTE 0x80
#define IXJCTL_DAA_COEFF_SET _IOW('q', 0xD0, int)
#define DAA_US 1
#define DAA_UK 2
#define DAA_FRANCE 3
#define DAA_GERMANY 4
#define DAA_AUSTRALIA 5
#define DAA_JAPAN 6
#define IXJCTL_PORT _IOW('q', 0xD1, int)
#define PORT_QUERY 0
#define PORT_POTS 1
#define PORT_PSTN 2
#define PORT_SPEAKER 3
#define PORT_HANDSET 4
#define IXJCTL_PSTN_SET_STATE PHONE_PSTN_SET_STATE
#define IXJCTL_PSTN_GET_STATE PHONE_PSTN_GET_STATE
#define PSTN_ON_HOOK 0
#define PSTN_RINGING 1
#define PSTN_OFF_HOOK 2
#define PSTN_PULSE_DIAL 3
#define IXJCTL_DAA_AGAIN _IOW('q', 0xD2, int)
#define AGRR00DB 0x00
#define AGRR3_5DB 0x10
#define AGRR06DB 0x30
#define AGX00DB 0x00
#define AGX_6DB 0x04
#define AGX3_5DB 0x08
#define AGX_2_5B 0x0C
#define IXJCTL_PSTN_LINETEST _IO('q', 0xD3)
#define IXJCTL_CID _IOR('q', 0xD4, PHONE_CID *)
#define IXJCTL_VMWI _IOR('q', 0xD8, int)
#define IXJCTL_CIDCW _IOW('q', 0xD9, PHONE_CID *)
#define IXJCTL_WINK_DURATION PHONE_WINK_DURATION
#define IXJCTL_POTS_PSTN _IOW('q', 0xD5, int)
#define IXJCTL_HZ _IOW('q', 0xE0, int)
#define IXJCTL_RATE _IOW('q', 0xE1, int)
#define IXJCTL_FRAMES_READ _IOR('q', 0xE2, unsigned long)
#define IXJCTL_FRAMES_WRITTEN _IOR('q', 0xE3, unsigned long)
#define IXJCTL_READ_WAIT _IOR('q', 0xE4, unsigned long)
#define IXJCTL_WRITE_WAIT _IOR('q', 0xE5, unsigned long)
#define IXJCTL_DRYBUFFER_READ _IOR('q', 0xE6, unsigned long)
#define IXJCTL_DRYBUFFER_CLEAR _IO('q', 0xE7)
#define IXJCTL_DTMF_PRESCALE _IOW('q', 0xE8, int)
typedef enum {
  SIG_DTMF_READY,
  SIG_HOOKSTATE,
  SIG_FLASH,
  SIG_PSTN_RING,
  SIG_CALLER_ID,
  SIG_PSTN_WINK,
  SIG_F0,
  SIG_F1,
  SIG_F2,
  SIG_F3,
  SIG_FC0,
  SIG_FC1,
  SIG_FC2,
  SIG_FC3,
  SIG_READ_READY = 33,
  SIG_WRITE_READY = 34
} IXJ_SIGEVENT;
typedef struct {
  unsigned int event;
  int signal;
} IXJ_SIGDEF;
#define IXJCTL_SIGCTL _IOW('q', 0xE9, IXJ_SIGDEF *)
#define IXJCTL_SC_RXG _IOW('q', 0xEA, int)
#define IXJCTL_SC_TXG _IOW('q', 0xEB, int)
#define IXJCTL_INTERCOM_START _IOW('q', 0xFD, int)
#define IXJCTL_INTERCOM_STOP _IOW('q', 0xFE, int)
typedef struct {
  unsigned int filter;
  char enable;
  unsigned int coeff[19];
} IXJ_FILTER_RAW;
#endif
