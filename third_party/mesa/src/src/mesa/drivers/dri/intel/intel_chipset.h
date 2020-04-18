 /*
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#define PCI_CHIP_I810			0x7121
#define PCI_CHIP_I810_DC100		0x7123
#define PCI_CHIP_I810_E			0x7125
#define PCI_CHIP_I815			0x1132

#define PCI_CHIP_I830_M			0x3577
#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I855_GM		0x3582
#define PCI_CHIP_I865_G			0x2572

#define PCI_CHIP_I915_G			0x2582
#define PCI_CHIP_E7221_G		0x258A
#define PCI_CHIP_I915_GM		0x2592
#define PCI_CHIP_I945_G			0x2772
#define PCI_CHIP_I945_GM		0x27A2
#define PCI_CHIP_I945_GME		0x27AE

#define PCI_CHIP_Q35_G			0x29B2
#define PCI_CHIP_G33_G			0x29C2
#define PCI_CHIP_Q33_G			0x29D2

#define PCI_CHIP_IGD_GM			0xA011
#define PCI_CHIP_IGD_G			0xA001

#define IS_IGDGM(devid)	(devid == PCI_CHIP_IGD_GM)
#define IS_IGDG(devid)	(devid == PCI_CHIP_IGD_G)
#define IS_IGD(devid) (IS_IGDG(devid) || IS_IGDGM(devid))

#define PCI_CHIP_I965_G			0x29A2
#define PCI_CHIP_I965_Q			0x2992
#define PCI_CHIP_I965_G_1		0x2982
#define PCI_CHIP_I946_GZ		0x2972
#define PCI_CHIP_I965_GM                0x2A02
#define PCI_CHIP_I965_GME               0x2A12

#define PCI_CHIP_GM45_GM                0x2A42

#define PCI_CHIP_IGD_E_G                0x2E02
#define PCI_CHIP_Q45_G                  0x2E12
#define PCI_CHIP_G45_G                  0x2E22
#define PCI_CHIP_G41_G                  0x2E32
#define PCI_CHIP_B43_G                  0x2E42
#define PCI_CHIP_B43_G1                 0x2E92

#define PCI_CHIP_ILD_G                  0x0042
#define PCI_CHIP_ILM_G                  0x0046

#define PCI_CHIP_SANDYBRIDGE_GT1	0x0102	/* Desktop */
#define PCI_CHIP_SANDYBRIDGE_GT2	0x0112
#define PCI_CHIP_SANDYBRIDGE_GT2_PLUS	0x0122
#define PCI_CHIP_SANDYBRIDGE_M_GT1	0x0106	/* Mobile */
#define PCI_CHIP_SANDYBRIDGE_M_GT2	0x0116
#define PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS	0x0126
#define PCI_CHIP_SANDYBRIDGE_S		0x010A	/* Server */

#define PCI_CHIP_IVYBRIDGE_GT1          0x0152  /* Desktop */
#define PCI_CHIP_IVYBRIDGE_GT2          0x0162
#define PCI_CHIP_IVYBRIDGE_M_GT1        0x0156  /* Mobile */
#define PCI_CHIP_IVYBRIDGE_M_GT2        0x0166
#define PCI_CHIP_IVYBRIDGE_S_GT1        0x015a  /* Server */
#define PCI_CHIP_IVYBRIDGE_S_GT2        0x016a

#define PCI_CHIP_HASWELL_GT1            0x0402 /* Desktop */
#define PCI_CHIP_HASWELL_GT2            0x0412
#define PCI_CHIP_HASWELL_GT2_PLUS       0x0422
#define PCI_CHIP_HASWELL_M_GT1          0x0406 /* Mobile */
#define PCI_CHIP_HASWELL_M_GT2          0x0416
#define PCI_CHIP_HASWELL_M_GT2_PLUS     0x0426
#define PCI_CHIP_HASWELL_S_GT1          0x040A /* Server */
#define PCI_CHIP_HASWELL_S_GT2          0x041A
#define PCI_CHIP_HASWELL_S_GT2_PLUS     0x042A
#define PCI_CHIP_HASWELL_SDV_GT1        0x0C02 /* Desktop */
#define PCI_CHIP_HASWELL_SDV_GT2        0x0C12
#define PCI_CHIP_HASWELL_SDV_GT2_PLUS   0x0C22
#define PCI_CHIP_HASWELL_SDV_M_GT1      0x0C06 /* Mobile */
#define PCI_CHIP_HASWELL_SDV_M_GT2      0x0C16
#define PCI_CHIP_HASWELL_SDV_M_GT2_PLUS 0x0C26
#define PCI_CHIP_HASWELL_SDV_S_GT1      0x0C0A /* Server */
#define PCI_CHIP_HASWELL_SDV_S_GT2      0x0C1A
#define PCI_CHIP_HASWELL_SDV_S_GT2_PLUS 0x0C2A
#define PCI_CHIP_HASWELL_ULT_GT1        0x0A02 /* Desktop */
#define PCI_CHIP_HASWELL_ULT_GT2        0x0A12
#define PCI_CHIP_HASWELL_ULT_GT2_PLUS   0x0A22
#define PCI_CHIP_HASWELL_ULT_M_GT1      0x0A06 /* Mobile */
#define PCI_CHIP_HASWELL_ULT_M_GT2      0x0A16
#define PCI_CHIP_HASWELL_ULT_M_GT2_PLUS 0x0A26
#define PCI_CHIP_HASWELL_ULT_S_GT1      0x0A0A /* Server */
#define PCI_CHIP_HASWELL_ULT_S_GT2      0x0A1A
#define PCI_CHIP_HASWELL_ULT_S_GT2_PLUS 0x0A2A
#define PCI_CHIP_HASWELL_CRW_GT1        0x0D12 /* Desktop */
#define PCI_CHIP_HASWELL_CRW_GT2        0x0D22
#define PCI_CHIP_HASWELL_CRW_GT2_PLUS   0x0D32
#define PCI_CHIP_HASWELL_CRW_M_GT1      0x0D16 /* Mobile */
#define PCI_CHIP_HASWELL_CRW_M_GT2      0x0D26
#define PCI_CHIP_HASWELL_CRW_M_GT2_PLUS 0x0D36
#define PCI_CHIP_HASWELL_CRW_S_GT1      0x0D1A /* Server */
#define PCI_CHIP_HASWELL_CRW_S_GT2      0x0D2A
#define PCI_CHIP_HASWELL_CRW_S_GT2_PLUS 0x0D3A

#define IS_MOBILE(devid)	(devid == PCI_CHIP_I855_GM || \
				 devid == PCI_CHIP_I915_GM || \
				 devid == PCI_CHIP_I945_GM || \
				 devid == PCI_CHIP_I945_GME || \
				 devid == PCI_CHIP_I965_GM || \
				 devid == PCI_CHIP_I965_GME || \
				 devid == PCI_CHIP_GM45_GM || \
				 IS_IGD(devid) || \
				 devid == PCI_CHIP_ILM_G)

#define IS_G45(devid)           (devid == PCI_CHIP_IGD_E_G || \
                                 devid == PCI_CHIP_Q45_G || \
                                 devid == PCI_CHIP_G45_G || \
                                 devid == PCI_CHIP_G41_G || \
                                 devid == PCI_CHIP_B43_G || \
                                 devid == PCI_CHIP_B43_G1)
#define IS_GM45(devid)          (devid == PCI_CHIP_GM45_GM)
#define IS_G4X(devid)		(IS_G45(devid) || IS_GM45(devid))

#define IS_ILD(devid)           (devid == PCI_CHIP_ILD_G)
#define IS_ILM(devid)           (devid == PCI_CHIP_ILM_G)
#define IS_GEN5(devid)          (IS_ILD(devid) || IS_ILM(devid))

#define IS_915(devid)		(devid == PCI_CHIP_I915_G || \
				 devid == PCI_CHIP_E7221_G || \
				 devid == PCI_CHIP_I915_GM)

#define IS_945(devid)		(devid == PCI_CHIP_I945_G || \
				 devid == PCI_CHIP_I945_GM || \
				 devid == PCI_CHIP_I945_GME || \
				 devid == PCI_CHIP_G33_G || \
				 devid == PCI_CHIP_Q33_G || \
				 devid == PCI_CHIP_Q35_G || IS_IGD(devid))

#define IS_GEN4(devid)		(devid == PCI_CHIP_I965_G || \
				 devid == PCI_CHIP_I965_Q || \
				 devid == PCI_CHIP_I965_G_1 || \
				 devid == PCI_CHIP_I965_GM || \
				 devid == PCI_CHIP_I965_GME || \
				 devid == PCI_CHIP_I946_GZ || \
				 IS_G4X(devid))

/* Compat macro for intel_decode.c */
#define IS_IRONLAKE(devid)	IS_GEN5(devid)

#define IS_SNB_GT1(devid)	(devid == PCI_CHIP_SANDYBRIDGE_GT1 || \
				 devid == PCI_CHIP_SANDYBRIDGE_M_GT1 || \
				 devid == PCI_CHIP_SANDYBRIDGE_S)

#define IS_SNB_GT2(devid)	(devid == PCI_CHIP_SANDYBRIDGE_GT2 || \
				 devid == PCI_CHIP_SANDYBRIDGE_GT2_PLUS	|| \
				 devid == PCI_CHIP_SANDYBRIDGE_M_GT2 || \
				 devid == PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS)

#define IS_GEN6(devid)		(IS_SNB_GT1(devid) || IS_SNB_GT2(devid))

#define IS_IVB_GT1(devid)       (devid == PCI_CHIP_IVYBRIDGE_GT1 || \
				 devid == PCI_CHIP_IVYBRIDGE_M_GT1 || \
				 devid == PCI_CHIP_IVYBRIDGE_S_GT1)

#define IS_IVB_GT2(devid)       (devid == PCI_CHIP_IVYBRIDGE_GT2 || \
				 devid == PCI_CHIP_IVYBRIDGE_M_GT2 || \
				 devid == PCI_CHIP_IVYBRIDGE_S_GT2)

#define IS_IVYBRIDGE(devid)     (IS_IVB_GT1(devid) || IS_IVB_GT2(devid))

#define IS_GEN7(devid)	        (IS_IVYBRIDGE(devid) || \
				 IS_HASWELL(devid))

#define IS_HSW_GT1(devid)	(devid == PCI_CHIP_HASWELL_GT1 || \
				 devid == PCI_CHIP_HASWELL_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_S_GT1)
#define IS_HSW_GT2(devid)	(devid == PCI_CHIP_HASWELL_GT2 || \
				 devid == PCI_CHIP_HASWELL_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_M_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_S_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_SDV_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_SDV_M_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_SDV_S_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_ULT_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_ULT_M_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_ULT_S_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_CRW_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_CRW_M_GT2_PLUS || \
				 devid == PCI_CHIP_HASWELL_CRW_S_GT2_PLUS)

#define IS_HASWELL(devid)       (IS_HSW_GT1(devid) || \
				 IS_HSW_GT2(devid))

#define IS_965(devid)		(IS_GEN4(devid) || \
				 IS_G4X(devid) || \
				 IS_GEN5(devid) || \
				 IS_GEN6(devid) || \
				 IS_GEN7(devid))

#define IS_9XX(devid)		(IS_915(devid) || \
				 IS_945(devid) || \
				 IS_965(devid))

#define IS_GEN3(devid)		(IS_915(devid) ||	\
				 IS_945(devid))

#define IS_GEN2(devid)		(devid == PCI_CHIP_I830_M || \
				 devid == PCI_CHIP_845_G ||  \
				 devid == PCI_CHIP_I855_GM ||	\
				 devid == PCI_CHIP_I865_G)
