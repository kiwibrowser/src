
#ifndef __NVC0_PGRAPH_MACROS_H__
#define __NVC0_PGRAPH_MACROS_H__

/* extrinsrt r1, r2, src, size, dst: replace bits [dst:dst+size) in r1
 *  with bits [src:src+size) in r2
 *
 * bra(n)z annul: no delay slot
 */

/* Bitfield version of NVC0_3D_VERTEX_ARRAY_PER_INSTANCE[].
 * Args: size, bitfield
 */
static const uint32_t nvc0_9097_per_instance_bf[] =
{
   0x00000301, /* parm $r3 (the bitfield) */
   0x00000211, /* mov $r2 0 */
   0x05880021, /* maddr [NVC0_3D_VERTEX_ARRAY_PER_INSTANCE(0), increment = 4] */
   0xffffc911, /* mov $r1 (add $r1 -0x1) */
   0x0040d043, /* send (extrshl $r3 $r2 0x1 0) */
   0xffff8897, /* exit branz $r1 0x3 */
   0x00005211  /* mov $r2 (add $r2 0x1) */
};

/* The comments above the macros describe what they *should* be doing,
 * but we use less functionality for now.
 */

/*
 * for (i = 0; i < 8; ++i)
 *    [NVC0_3D_BLEND_ENABLE(i)] = BIT(i of arg);
 *
 * [3428] = arg;
 *
 * if (arg == 0 || [NVC0_3D_MULTISAMPLE_ENABLE] == 0)
 *    [0d9c] = 0;
 * else
 *    [0d9c] = [342c];
 */
static const uint32_t nvc0_9097_blend_enables[] =
{
   0x05360021, /* 0x00: maddr [NVC0_3D_BLEND_ENABLE(0), increment = 4] */
   0x00404042, /* 0x01: send extrinsrt 0 $r1 0 0x1 0 */
   0x00424042, /* 0x02: send extrinsrt 0 $r1 0x1 0x1 0 */
   0x00444042, /* 0x03: send extrinsrt 0 $r1 0x2 0x1 0 */
   0x00464042, /* 0x04: send extrinsrt 0 $r1 0x3 0x1 0 */
   0x00484042, /* 0x05: send extrinsrt 0 $r1 0x4 0x1 0 */
   0x004a4042, /* 0x06: send extrinsrt 0 $r1 0x5 0x1 0 */
   0x004c40c2, /* 0x07: exit send extrinsrt 0 $r1 0x6 0x1 0 */
   0x004e4042, /* 0x08: send extrinsrt 0 $r1 0x7 0x1 0 */
};

/*
 * uint64 limit = (parm(0) << 32) | parm(1);
 * uint64 start = (parm(2) << 32);
 *
 * if (limit) {
 *    start |= parm(3);
 *    --limit;
 * } else {
 *    start |= 1;
 * }
 *
 * [0x1c04 + (arg & 0xf) * 16 + 0] = (start >> 32) & 0xff;
 * [0x1c04 + (arg & 0xf) * 16 + 4] = start & 0xffffffff;
 * [0x1f00 + (arg & 0xf) * 8 + 0] = (limit >> 32) & 0xff;
 * [0x1f00 + (arg & 0xf) * 8 + 4] = limit & 0xffffffff;
 */
static const uint32_t nvc0_9097_vertex_array_select[] =
{
   0x00000201, /* 0x00: parm $r2 */
   0x00000301, /* 0x01: parm $r3 */
   0x00000401, /* 0x02: parm $r4 */
   0x00000501, /* 0x03: parm $r5 */
   0x11004612, /* 0x04: mov $r6 extrinsrt 0 $r1 0 4 2 */
   0x09004712, /* 0x05: mov $r7 extrinsrt 0 $r1 0 4 1 */
   0x05c07621, /* 0x06: maddr $r6 add $6 0x1701 */
   0x00002041, /* 0x07: send $r4 */
   0x00002841, /* 0x08: send $r5 */
   0x05f03f21, /* 0x09: maddr $r7 add $7 0x17c0 */
   0x000010c1, /* 0x0a: exit send $r2 */
   0x00001841, /* 0x0b: send $r3 */
};

/*
 * [GL_POLYGON_MODE_FRONT] = arg;
 *
 * if (BIT(31 of [0x3410]))
 *    [1a24] = 0x7353;
 *
 * if ([NVC0_3D_SP_SELECT(3)] == 0x31 || [NVC0_3D_SP_SELECT(4)] == 0x41)
 *    [02ec] = 0;
 * else
 * if ([GL_POLYGON_MODE_BACK] == GL_LINE || arg == GL_LINE)
 *    [02ec] = BYTE(1 of [0x3410]) << 4;
 * else
 *    [02ec] = BYTE(0 of [0x3410]) << 4;
 */
static const uint32_t nvc0_9097_poly_mode_front[] =
{
   0x00db0215, /* 0x00: read $r2 [NVC0_3D_POLYGON_MODE_BACK] */
   0x020c0315, /* 0x01: read $r3 [NVC0_3D_SP_SELECT(3)] */
   0x00128f10, /* 0x02: mov $r7 or $r1 $r2 */
   0x02100415, /* 0x03: read $r4 [NVC0_3D_SP_SELECT(4)] */
   0x00004211, /* 0x04: mov $r2 0x1 */
   0x00180611, /* 0x05: mov $r6 0x60 */
   0x0014bf10, /* 0x06: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x07: braz $r7 0xa */
   0x00dac021, /* 0x08: maddr 0x36b */
   0x00800611, /* 0x09: mov $r6 0x200 */
   0x00131f10, /* 0x0a: mov $r7 or $r3 $r4 */
   0x0014bf10, /* 0x0b: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x0c: braz $r7 0xf */
   0x00000841, /* 0x0d: send $r1 */
   0x00000611, /* 0x0e: mov $r6 0 */
   0x002ec0a1, /* 0x0f: exit maddr [02ec] */
   0x00003041  /* 0x10: send $r6 */
};

/*
 * [GL_POLYGON_MODE_BACK] = arg;
 *
 * if (BIT(31 of [0x3410]))
 *    [1a24] = 0x7353;
 *
 * if ([NVC0_3D_SP_SELECT(3)] == 0x31 || [NVC0_3D_SP_SELECT(4)] == 0x41)
 *    [02ec] = 0;
 * else
 * if ([GL_POLYGON_MODE_FRONT] == GL_LINE || arg == GL_LINE)
 *    [02ec] = BYTE(1 of [0x3410]) << 4;
 * else
 *    [02ec] = BYTE(0 of [0x3410]) << 4;
 */
/* NOTE: 0x3410 = 0x80002006 by default,
 *  POLYGON_MODE == GL_LINE check replaced by (MODE & 1)
 *  SP_SELECT(i) == (i << 4) | 1 check replaced by SP_SELECT(i) & 1
 */
static const uint32_t nvc0_9097_poly_mode_back[] =
{
   0x00dac215, /* 0x00: read $r2 [NVC0_3D_POLYGON_MODE_FRONT] */
   0x020c0315, /* 0x01: read $r3 [NVC0_3D_SP_SELECT(3)] */
   0x00128f10, /* 0x02: mov $r7 or $r1 $r2 */
   0x02100415, /* 0x03: read $r4 [NVC0_3D_SP_SELECT(4)] */
   0x00004211, /* 0x04: mov $r2 0x1 */
   0x00180611, /* 0x05: mov $r6 0x60 */
   0x0014bf10, /* 0x06: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x07: braz $r7 0xa */
   0x00db0021, /* 0x08: maddr 0x36c */
   0x00800611, /* 0x09: mov $r6 0x200 */
   0x00131f10, /* 0x0a: mov $r7 or $r3 $r4 */
   0x0014bf10, /* 0x0b: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x0c: braz $r7 0xf */
   0x00000841, /* 0x0d: send $r1 */
   0x00000611, /* 0x0e: mov $r6 0 */
   0x002ec0a1, /* 0x0f: exit maddr [02ec] */
   0x00003041  /* 0x10: send $r6 */
};

/*
 * [NVC0_3D_SP_SELECT(4)] = arg
 *
 * if BIT(31 of [0x3410]) == 0
 *    [1a24] = 0x7353;
 *
 * if ([NVC0_3D_SP_SELECT(3)] == 0x31 || arg == 0x41)
 *    [02ec] = 0
 * else
 * if (any POLYGON MODE == LINE)
 *    [02ec] = BYTE(1 of [3410]) << 4;
 * else
 *    [02ec] = BYTE(0 of [3410]) << 4; // 02ec valid bits are 0xff1
 */
static const uint32_t nvc0_9097_gp_select[] = /* 0x0f */
{
   0x00dac215, /* 0x00: read $r2 0x36b */
   0x00db0315, /* 0x01: read $r3 0x36c */
   0x0012d710, /* 0x02: mov $r7 or $r2 $r3 */
   0x020c0415, /* 0x03: read $r4 0x830 */
   0x00004211, /* 0x04: mov $r2 0x1 */
   0x00180611, /* 0x05: mov $r6 0x60 */
   0x0014bf10, /* 0x06: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x07: braz $r7 0xa */
   0x02100021, /* 0x08: maddr 0x840 */
   0x00800611, /* 0x09: mov $r6 0x200 */
   0x00130f10, /* 0x0a: mov $r7 or $r1 $r4 */
   0x0014bf10, /* 0x0b: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x0c: braz $r7 0xf */
   0x00000841, /* 0x0d: send $r1 */
   0x00000611, /* 0x0e: mov $r6 0 */
   0x002ec0a1, /* 0x0f: exit maddr 0xbb */
   0x00003041, /* 0x10: send $r6 */
};

/*
 * [NVC0_3D_SP_SELECT(3)] = arg
 *
 * if BIT(31 of [0x3410]) == 0
 *    [1a24] = 0x7353;
 *
 * if (arg == 0x31) {
 *    if (BIT(2 of [0x3430])) {
 *       int i = 15; do { --i; } while(i);
 *       [0x1a2c] = 0;
 *    }
 * }
 *
 * if ([NVC0_3D_SP_SELECT(4)] == 0x41 || arg == 0x31)
 *    [02ec] = 0
 * else
 * if ([any POLYGON_MODE] == GL_LINE)
 *    [02ec] = BYTE(1 of [3410]) << 4;
 * else
 *    [02ec] = BYTE(0 of [3410]) << 4;
 */
static const uint32_t nvc0_9097_tep_select[] = /* 0x10 */
{
   0x00dac215, /* 0x00: read $r2 0x36b */
   0x00db0315, /* 0x01: read $r3 0x36c */
   0x0012d710, /* 0x02: mov $r7 or $r2 $r3 */
   0x02100415, /* 0x03: read $r4 0x840 */
   0x00004211, /* 0x04: mov $r2 0x1 */
   0x00180611, /* 0x05: mov $r6 0x60 */
   0x0014bf10, /* 0x06: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x07: braz $r7 0xa */
   0x020c0021, /* 0x08: maddr 0x830 */
   0x00800611, /* 0x09: mov $r6 0x200 */
   0x00130f10, /* 0x0a: mov $r7 or $r1 $r4 */
   0x0014bf10, /* 0x0b: mov $r7 and $r7 $r2 */
   0x0000f807, /* 0x0c: braz $r7 0xf */
   0x00000841, /* 0x0d: send $r1 */
   0x00000611, /* 0x0e: mov $r6 0 */
   0x002ec0a1, /* 0x0f: exit maddr 0xbb */
   0x00003041, /* 0x10: send $r6 */
};

#endif
