/*
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _INTEL_CHIPSET_H
#define _INTEL_CHIPSET_H

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

#define IS_IGDGM(devid)		((devid) == PCI_CHIP_IGD_GM)
#define IS_IGDG(devid)		((devid) == PCI_CHIP_IGD_G)
#define IS_IGD(devid)		(IS_IGDG(devid) || IS_IGDGM(devid))

#define PCI_CHIP_I965_G			0x29A2
#define PCI_CHIP_I965_Q			0x2992
#define PCI_CHIP_I965_G_1		0x2982
#define PCI_CHIP_I946_GZ		0x2972
#define PCI_CHIP_I965_GM		0x2A02
#define PCI_CHIP_I965_GME		0x2A12

#define PCI_CHIP_GM45_GM		0x2A42

#define PCI_CHIP_IGD_E_G		0x2E02
#define PCI_CHIP_Q45_G			0x2E12
#define PCI_CHIP_G45_G			0x2E22
#define PCI_CHIP_G41_G			0x2E32

#define PCI_CHIP_ILD_G			0x0042
#define PCI_CHIP_ILM_G			0x0046

#define PCI_CHIP_SANDYBRIDGE_GT1	0x0102 /* desktop */
#define PCI_CHIP_SANDYBRIDGE_GT2	0x0112
#define PCI_CHIP_SANDYBRIDGE_GT2_PLUS	0x0122
#define PCI_CHIP_SANDYBRIDGE_M_GT1	0x0106 /* mobile */
#define PCI_CHIP_SANDYBRIDGE_M_GT2	0x0116
#define PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS	0x0126
#define PCI_CHIP_SANDYBRIDGE_S		0x010A /* server */

#define PCI_CHIP_IVYBRIDGE_GT1		0x0152 /* desktop */
#define PCI_CHIP_IVYBRIDGE_GT2		0x0162
#define PCI_CHIP_IVYBRIDGE_M_GT1	0x0156 /* mobile */
#define PCI_CHIP_IVYBRIDGE_M_GT2	0x0166
#define PCI_CHIP_IVYBRIDGE_S		0x015a /* server */
#define PCI_CHIP_IVYBRIDGE_S_GT2	0x016a /* server */

#define PCI_CHIP_HASWELL_GT1		0x0402 /* Desktop */
#define PCI_CHIP_HASWELL_GT2		0x0412
#define PCI_CHIP_HASWELL_GT3		0x0422
#define PCI_CHIP_HASWELL_M_GT1		0x0406 /* Mobile */
#define PCI_CHIP_HASWELL_M_GT2		0x0416
#define PCI_CHIP_HASWELL_M_GT3		0x0426
#define PCI_CHIP_HASWELL_S_GT1		0x040A /* Server */
#define PCI_CHIP_HASWELL_S_GT2		0x041A
#define PCI_CHIP_HASWELL_S_GT3		0x042A
#define PCI_CHIP_HASWELL_B_GT1		0x040B /* Reserved */
#define PCI_CHIP_HASWELL_B_GT2		0x041B
#define PCI_CHIP_HASWELL_B_GT3		0x042B
#define PCI_CHIP_HASWELL_E_GT1		0x040E /* Reserved */
#define PCI_CHIP_HASWELL_E_GT2		0x041E
#define PCI_CHIP_HASWELL_E_GT3		0x042E
#define PCI_CHIP_HASWELL_SDV_GT1	0x0C02 /* Desktop */
#define PCI_CHIP_HASWELL_SDV_GT2	0x0C12
#define PCI_CHIP_HASWELL_SDV_GT3	0x0C22
#define PCI_CHIP_HASWELL_SDV_M_GT1	0x0C06 /* Mobile */
#define PCI_CHIP_HASWELL_SDV_M_GT2	0x0C16
#define PCI_CHIP_HASWELL_SDV_M_GT3	0x0C26
#define PCI_CHIP_HASWELL_SDV_S_GT1	0x0C0A /* Server */
#define PCI_CHIP_HASWELL_SDV_S_GT2	0x0C1A
#define PCI_CHIP_HASWELL_SDV_S_GT3	0x0C2A
#define PCI_CHIP_HASWELL_SDV_B_GT1	0x0C0B /* Reserved */
#define PCI_CHIP_HASWELL_SDV_B_GT2	0x0C1B
#define PCI_CHIP_HASWELL_SDV_B_GT3	0x0C2B
#define PCI_CHIP_HASWELL_SDV_E_GT1	0x0C0E /* Reserved */
#define PCI_CHIP_HASWELL_SDV_E_GT2	0x0C1E
#define PCI_CHIP_HASWELL_SDV_E_GT3	0x0C2E
#define PCI_CHIP_HASWELL_ULT_GT1	0x0A02 /* Desktop */
#define PCI_CHIP_HASWELL_ULT_GT2	0x0A12
#define PCI_CHIP_HASWELL_ULT_GT3	0x0A22
#define PCI_CHIP_HASWELL_ULT_M_GT1	0x0A06 /* Mobile */
#define PCI_CHIP_HASWELL_ULT_M_GT2	0x0A16
#define PCI_CHIP_HASWELL_ULT_M_GT3	0x0A26
#define PCI_CHIP_HASWELL_ULT_S_GT1	0x0A0A /* Server */
#define PCI_CHIP_HASWELL_ULT_S_GT2	0x0A1A
#define PCI_CHIP_HASWELL_ULT_S_GT3	0x0A2A
#define PCI_CHIP_HASWELL_ULT_B_GT1	0x0A0B /* Reserved */
#define PCI_CHIP_HASWELL_ULT_B_GT2	0x0A1B
#define PCI_CHIP_HASWELL_ULT_B_GT3	0x0A2B
#define PCI_CHIP_HASWELL_ULT_E_GT1	0x0A0E /* Reserved */
#define PCI_CHIP_HASWELL_ULT_E_GT2	0x0A1E
#define PCI_CHIP_HASWELL_ULT_E_GT3	0x0A2E
#define PCI_CHIP_HASWELL_CRW_GT1	0x0D02 /* Desktop */
#define PCI_CHIP_HASWELL_CRW_GT2	0x0D12
#define PCI_CHIP_HASWELL_CRW_GT3	0x0D22
#define PCI_CHIP_HASWELL_CRW_M_GT1	0x0D06 /* Mobile */
#define PCI_CHIP_HASWELL_CRW_M_GT2	0x0D16
#define PCI_CHIP_HASWELL_CRW_M_GT3	0x0D26
#define PCI_CHIP_HASWELL_CRW_S_GT1	0x0D0A /* Server */
#define PCI_CHIP_HASWELL_CRW_S_GT2	0x0D1A
#define PCI_CHIP_HASWELL_CRW_S_GT3	0x0D2A
#define PCI_CHIP_HASWELL_CRW_B_GT1	0x0D0B /* Reserved */
#define PCI_CHIP_HASWELL_CRW_B_GT2	0x0D1B
#define PCI_CHIP_HASWELL_CRW_B_GT3	0x0D2B
#define PCI_CHIP_HASWELL_CRW_E_GT1	0x0D0E /* Reserved */
#define PCI_CHIP_HASWELL_CRW_E_GT2	0x0D1E
#define PCI_CHIP_HASWELL_CRW_E_GT3	0x0D2E
#define BDW_SPARE			0x2
#define BDW_ULT				0x6
#define BDW_SERVER			0xa
#define BDW_IRIS			0xb
#define BDW_WORKSTATION			0xd
#define BDW_ULX				0xe

#define PCI_CHIP_VALLEYVIEW_PO		0x0f30 /* VLV PO board */
#define PCI_CHIP_VALLEYVIEW_1		0x0f31
#define PCI_CHIP_VALLEYVIEW_2		0x0f32
#define PCI_CHIP_VALLEYVIEW_3		0x0f33

#define PCI_CHIP_CHERRYVIEW_0		0x22b0
#define PCI_CHIP_CHERRYVIEW_1		0x22b1
#define PCI_CHIP_CHERRYVIEW_2		0x22b2
#define PCI_CHIP_CHERRYVIEW_3		0x22b3

#define PCI_CHIP_SKYLAKE_DT_GT1		0x1902
#define PCI_CHIP_SKYLAKE_ULT_GT1	0x1906
#define PCI_CHIP_SKYLAKE_SRV_GT1	0x190A /* Reserved */
#define PCI_CHIP_SKYLAKE_H_GT1		0x190B
#define PCI_CHIP_SKYLAKE_ULX_GT1	0x190E /* Reserved */
#define PCI_CHIP_SKYLAKE_DT_GT2		0x1912
#define PCI_CHIP_SKYLAKE_FUSED0_GT2	0x1913 /* Reserved */
#define PCI_CHIP_SKYLAKE_FUSED1_GT2	0x1915 /* Reserved */
#define PCI_CHIP_SKYLAKE_ULT_GT2	0x1916
#define PCI_CHIP_SKYLAKE_FUSED2_GT2	0x1917 /* Reserved */
#define PCI_CHIP_SKYLAKE_SRV_GT2	0x191A /* Reserved */
#define PCI_CHIP_SKYLAKE_HALO_GT2	0x191B
#define PCI_CHIP_SKYLAKE_WKS_GT2 	0x191D
#define PCI_CHIP_SKYLAKE_ULX_GT2	0x191E
#define PCI_CHIP_SKYLAKE_MOBILE_GT2	0x1921 /* Reserved */
#define PCI_CHIP_SKYLAKE_ULT_GT3_0	0x1923
#define PCI_CHIP_SKYLAKE_ULT_GT3_1	0x1926
#define PCI_CHIP_SKYLAKE_ULT_GT3_2	0x1927
#define PCI_CHIP_SKYLAKE_SRV_GT4	0x192A
#define PCI_CHIP_SKYLAKE_HALO_GT3	0x192B /* Reserved */
#define PCI_CHIP_SKYLAKE_SRV_GT3	0x192D
#define PCI_CHIP_SKYLAKE_DT_GT4		0x1932
#define PCI_CHIP_SKYLAKE_SRV_GT4X	0x193A
#define PCI_CHIP_SKYLAKE_H_GT4		0x193B
#define PCI_CHIP_SKYLAKE_WKS_GT4	0x193D

#define PCI_CHIP_KABYLAKE_ULT_GT2	0x5916
#define PCI_CHIP_KABYLAKE_ULT_GT1_5	0x5913
#define PCI_CHIP_KABYLAKE_ULT_GT1	0x5906
#define PCI_CHIP_KABYLAKE_ULT_GT3_0	0x5923
#define PCI_CHIP_KABYLAKE_ULT_GT3_1	0x5926
#define PCI_CHIP_KABYLAKE_ULT_GT3_2	0x5927
#define PCI_CHIP_KABYLAKE_ULT_GT2F	0x5921
#define PCI_CHIP_KABYLAKE_ULX_GT1_5	0x5915
#define PCI_CHIP_KABYLAKE_ULX_GT1	0x590E
#define PCI_CHIP_KABYLAKE_ULX_GT2	0x591E
#define PCI_CHIP_KABYLAKE_DT_GT2	0x5912
#define PCI_CHIP_KABYLAKE_M_GT2		0x5917
#define PCI_CHIP_KABYLAKE_DT_GT1	0x5902
#define PCI_CHIP_KABYLAKE_HALO_GT2	0x591B
#define PCI_CHIP_KABYLAKE_HALO_GT4	0x593B
#define PCI_CHIP_KABYLAKE_HALO_GT1_0	0x5908
#define PCI_CHIP_KABYLAKE_HALO_GT1_1	0x590B
#define PCI_CHIP_KABYLAKE_SRV_GT2	0x591A
#define PCI_CHIP_KABYLAKE_SRV_GT1	0x590A
#define PCI_CHIP_KABYLAKE_WKS_GT2	0x591D

#define PCI_CHIP_BROXTON_0		0x0A84
#define PCI_CHIP_BROXTON_1		0x1A84
#define PCI_CHIP_BROXTON_2		0x5A84
#define PCI_CHIP_BROXTON_3		0x1A85
#define PCI_CHIP_BROXTON_4		0x5A85

#define PCI_CHIP_GLK			0x3184
#define PCI_CHIP_GLK_2X6		0x3185

#define PCI_CHIP_COFFEELAKE_S_GT1_1     0x3E90
#define PCI_CHIP_COFFEELAKE_S_GT1_2     0x3E93
#define PCI_CHIP_COFFEELAKE_S_GT2_1     0x3E91
#define PCI_CHIP_COFFEELAKE_S_GT2_2     0x3E92
#define PCI_CHIP_COFFEELAKE_S_GT2_3     0x3E96
#define PCI_CHIP_COFFEELAKE_H_GT2_1     0x3E9B
#define PCI_CHIP_COFFEELAKE_H_GT2_2     0x3E94
#define PCI_CHIP_COFFEELAKE_U_GT3_1     0x3EA5
#define PCI_CHIP_COFFEELAKE_U_GT3_2     0x3EA6
#define PCI_CHIP_COFFEELAKE_U_GT3_3     0x3EA7
#define PCI_CHIP_COFFEELAKE_U_GT3_4     0x3EA8

#define PCI_CHIP_CANNONLAKE_U_GT2_0	0x5A52
#define PCI_CHIP_CANNONLAKE_U_GT2_1	0x5A5A
#define PCI_CHIP_CANNONLAKE_U_GT2_2	0x5A42
#define PCI_CHIP_CANNONLAKE_U_GT2_3	0x5A4A
#define PCI_CHIP_CANNONLAKE_Y_GT2_0	0x5A51
#define PCI_CHIP_CANNONLAKE_Y_GT2_1	0x5A59
#define PCI_CHIP_CANNONLAKE_Y_GT2_2	0x5A41
#define PCI_CHIP_CANNONLAKE_Y_GT2_3	0x5A49
#define PCI_CHIP_CANNONLAKE_Y_GT2_4	0x5A71
#define PCI_CHIP_CANNONLAKE_Y_GT2_5	0x5A79

#define IS_MOBILE(devid)	((devid) == PCI_CHIP_I855_GM || \
				 (devid) == PCI_CHIP_I915_GM || \
				 (devid) == PCI_CHIP_I945_GM || \
				 (devid) == PCI_CHIP_I945_GME || \
				 (devid) == PCI_CHIP_I965_GM || \
				 (devid) == PCI_CHIP_I965_GME || \
				 (devid) == PCI_CHIP_GM45_GM || IS_IGD(devid) || \
				 (devid) == PCI_CHIP_IVYBRIDGE_M_GT1 || \
				 (devid) == PCI_CHIP_IVYBRIDGE_M_GT2)

#define IS_G45(devid)		((devid) == PCI_CHIP_IGD_E_G || \
				 (devid) == PCI_CHIP_Q45_G || \
				 (devid) == PCI_CHIP_G45_G || \
				 (devid) == PCI_CHIP_G41_G)
#define IS_GM45(devid)		((devid) == PCI_CHIP_GM45_GM)
#define IS_G4X(devid)		(IS_G45(devid) || IS_GM45(devid))

#define IS_ILD(devid)		((devid) == PCI_CHIP_ILD_G)
#define IS_ILM(devid)		((devid) == PCI_CHIP_ILM_G)

#define IS_915(devid)		((devid) == PCI_CHIP_I915_G || \
				 (devid) == PCI_CHIP_E7221_G || \
				 (devid) == PCI_CHIP_I915_GM)

#define IS_945GM(devid)		((devid) == PCI_CHIP_I945_GM || \
				 (devid) == PCI_CHIP_I945_GME)

#define IS_945(devid)		((devid) == PCI_CHIP_I945_G || \
				 (devid) == PCI_CHIP_I945_GM || \
				 (devid) == PCI_CHIP_I945_GME || \
				 IS_G33(devid))

#define IS_G33(devid)		((devid) == PCI_CHIP_G33_G || \
				 (devid) == PCI_CHIP_Q33_G || \
				 (devid) == PCI_CHIP_Q35_G || IS_IGD(devid))

#define IS_GEN2(devid)		((devid) == PCI_CHIP_I830_M || \
				 (devid) == PCI_CHIP_845_G || \
				 (devid) == PCI_CHIP_I855_GM || \
				 (devid) == PCI_CHIP_I865_G)

#define IS_GEN3(devid)		(IS_945(devid) || IS_915(devid))

#define IS_GEN4(devid)		((devid) == PCI_CHIP_I965_G || \
				 (devid) == PCI_CHIP_I965_Q || \
				 (devid) == PCI_CHIP_I965_G_1 || \
				 (devid) == PCI_CHIP_I965_GM || \
				 (devid) == PCI_CHIP_I965_GME || \
				 (devid) == PCI_CHIP_I946_GZ || \
				 IS_G4X(devid))

#define IS_GEN5(devid)		(IS_ILD(devid) || IS_ILM(devid))

#define IS_GEN6(devid)		((devid) == PCI_CHIP_SANDYBRIDGE_GT1 || \
				 (devid) == PCI_CHIP_SANDYBRIDGE_GT2 || \
				 (devid) == PCI_CHIP_SANDYBRIDGE_GT2_PLUS || \
				 (devid) == PCI_CHIP_SANDYBRIDGE_M_GT1 || \
				 (devid) == PCI_CHIP_SANDYBRIDGE_M_GT2 || \
				 (devid) == PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS || \
				 (devid) == PCI_CHIP_SANDYBRIDGE_S)

#define IS_GEN7(devid)		(IS_IVYBRIDGE(devid) || \
				 IS_HASWELL(devid) || \
				 IS_VALLEYVIEW(devid))

#define IS_IVYBRIDGE(devid)	((devid) == PCI_CHIP_IVYBRIDGE_GT1 || \
				 (devid) == PCI_CHIP_IVYBRIDGE_GT2 || \
				 (devid) == PCI_CHIP_IVYBRIDGE_M_GT1 || \
				 (devid) == PCI_CHIP_IVYBRIDGE_M_GT2 || \
				 (devid) == PCI_CHIP_IVYBRIDGE_S || \
				 (devid) == PCI_CHIP_IVYBRIDGE_S_GT2)

#define IS_VALLEYVIEW(devid)	((devid) == PCI_CHIP_VALLEYVIEW_PO || \
				 (devid) == PCI_CHIP_VALLEYVIEW_1 || \
				 (devid) == PCI_CHIP_VALLEYVIEW_2 || \
				 (devid) == PCI_CHIP_VALLEYVIEW_3)

#define IS_HSW_GT1(devid)	((devid) == PCI_CHIP_HASWELL_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_M_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_S_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_B_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_E_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_M_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_S_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_B_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_E_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_M_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_S_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_B_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_E_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_M_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_S_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_B_GT1 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_E_GT1)
#define IS_HSW_GT2(devid)	((devid) == PCI_CHIP_HASWELL_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_M_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_S_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_B_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_E_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_M_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_S_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_B_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_E_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_M_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_S_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_B_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_E_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_M_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_S_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_B_GT2 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_E_GT2)
#define IS_HSW_GT3(devid)	((devid) == PCI_CHIP_HASWELL_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_M_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_S_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_B_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_E_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_M_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_S_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_B_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_SDV_E_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_M_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_S_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_B_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_ULT_E_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_M_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_S_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_B_GT3 || \
				 (devid) == PCI_CHIP_HASWELL_CRW_E_GT3)

#define IS_HASWELL(devid)	(IS_HSW_GT1(devid) || \
				 IS_HSW_GT2(devid) || \
				 IS_HSW_GT3(devid))

#define IS_BROADWELL(devid)     (((devid & 0xff00) != 0x1600) ? 0 : \
				(((devid & 0x00f0) >> 4) > 3) ? 0 : \
				((devid & 0x000f) == BDW_SPARE) ? 1 : \
				((devid & 0x000f) == BDW_ULT) ? 1 : \
				((devid & 0x000f) == BDW_IRIS) ? 1 : \
				((devid & 0x000f) == BDW_SERVER) ? 1 : \
				((devid & 0x000f) == BDW_WORKSTATION) ? 1 : \
				((devid & 0x000f) == BDW_ULX) ? 1 : 0)

#define IS_CHERRYVIEW(devid)	((devid) == PCI_CHIP_CHERRYVIEW_0 || \
				 (devid) == PCI_CHIP_CHERRYVIEW_1 || \
				 (devid) == PCI_CHIP_CHERRYVIEW_2 || \
				 (devid) == PCI_CHIP_CHERRYVIEW_3)

#define IS_GEN8(devid)		(IS_BROADWELL(devid) || \
				 IS_CHERRYVIEW(devid))

#define IS_SKL_GT1(devid)	((devid) == PCI_CHIP_SKYLAKE_DT_GT1	|| \
				 (devid) == PCI_CHIP_SKYLAKE_ULT_GT1	|| \
				 (devid) == PCI_CHIP_SKYLAKE_SRV_GT1	|| \
				 (devid) == PCI_CHIP_SKYLAKE_H_GT1	|| \
				 (devid) == PCI_CHIP_SKYLAKE_ULX_GT1)

#define IS_SKL_GT2(devid)	((devid) == PCI_CHIP_SKYLAKE_DT_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_FUSED0_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_FUSED1_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_ULT_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_FUSED2_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_SRV_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_HALO_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_WKS_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_ULX_GT2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_MOBILE_GT2)

#define IS_SKL_GT3(devid)	((devid) == PCI_CHIP_SKYLAKE_ULT_GT3_0	|| \
				 (devid) == PCI_CHIP_SKYLAKE_ULT_GT3_1	|| \
				 (devid) == PCI_CHIP_SKYLAKE_ULT_GT3_2	|| \
				 (devid) == PCI_CHIP_SKYLAKE_HALO_GT3	|| \
				 (devid) == PCI_CHIP_SKYLAKE_SRV_GT3)

#define IS_SKL_GT4(devid)	((devid) == PCI_CHIP_SKYLAKE_SRV_GT4	|| \
				 (devid) == PCI_CHIP_SKYLAKE_DT_GT4	|| \
				 (devid) == PCI_CHIP_SKYLAKE_SRV_GT4X	|| \
				 (devid) == PCI_CHIP_SKYLAKE_H_GT4	|| \
				 (devid) == PCI_CHIP_SKYLAKE_WKS_GT4)

#define IS_KBL_GT1(devid)	((devid) == PCI_CHIP_KABYLAKE_ULT_GT1_5	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULX_GT1_5	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULT_GT1	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULX_GT1	|| \
				 (devid) == PCI_CHIP_KABYLAKE_DT_GT1	|| \
				 (devid) == PCI_CHIP_KABYLAKE_HALO_GT1_0 || \
				 (devid) == PCI_CHIP_KABYLAKE_HALO_GT1_1 || \
				 (devid) == PCI_CHIP_KABYLAKE_SRV_GT1)

#define IS_KBL_GT2(devid)	((devid) == PCI_CHIP_KABYLAKE_ULT_GT2	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULT_GT2F	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULX_GT2	|| \
				 (devid) == PCI_CHIP_KABYLAKE_DT_GT2	|| \
				 (devid) == PCI_CHIP_KABYLAKE_M_GT2	|| \
				 (devid) == PCI_CHIP_KABYLAKE_HALO_GT2	|| \
				 (devid) == PCI_CHIP_KABYLAKE_SRV_GT2	|| \
				 (devid) == PCI_CHIP_KABYLAKE_WKS_GT2)

#define IS_KBL_GT3(devid)	((devid) == PCI_CHIP_KABYLAKE_ULT_GT3_0	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULT_GT3_1	|| \
				 (devid) == PCI_CHIP_KABYLAKE_ULT_GT3_2)

#define IS_KBL_GT4(devid)	((devid) == PCI_CHIP_KABYLAKE_HALO_GT4)

#define IS_KABYLAKE(devid)	(IS_KBL_GT1(devid) || \
				 IS_KBL_GT2(devid) || \
				 IS_KBL_GT3(devid) || \
				 IS_KBL_GT4(devid))

#define IS_SKYLAKE(devid)	(IS_SKL_GT1(devid) || \
				 IS_SKL_GT2(devid) || \
				 IS_SKL_GT3(devid) || \
				 IS_SKL_GT4(devid))

#define IS_BROXTON(devid)	((devid) == PCI_CHIP_BROXTON_0	|| \
				 (devid) == PCI_CHIP_BROXTON_1	|| \
				 (devid) == PCI_CHIP_BROXTON_2	|| \
				 (devid) == PCI_CHIP_BROXTON_3	|| \
				 (devid) == PCI_CHIP_BROXTON_4)

#define IS_GEMINILAKE(devid)	((devid) == PCI_CHIP_GLK || \
				 (devid) == PCI_CHIP_GLK_2X6)

#define IS_CFL_S(devid)         ((devid) == PCI_CHIP_COFFEELAKE_S_GT1_1 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_S_GT1_2 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_S_GT2_1 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_S_GT2_2 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_S_GT2_3)

#define IS_CFL_H(devid)         ((devid) == PCI_CHIP_COFFEELAKE_H_GT2_1 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_H_GT2_2)

#define IS_CFL_U(devid)         ((devid) == PCI_CHIP_COFFEELAKE_U_GT3_1 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_U_GT3_2 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_U_GT3_3 || \
                                 (devid) == PCI_CHIP_COFFEELAKE_U_GT3_4)

#define IS_COFFEELAKE(devid)   (IS_CFL_S(devid) || \
				IS_CFL_H(devid) || \
				IS_CFL_U(devid))

#define IS_GEN9(devid)		(IS_SKYLAKE(devid)  || \
				 IS_BROXTON(devid)  || \
				 IS_KABYLAKE(devid) || \
				 IS_GEMINILAKE(devid) || \
				 IS_COFFEELAKE(devid))

#define IS_CNL_Y(devid)		((devid) == PCI_CHIP_CANNONLAKE_Y_GT2_0 || \
				 (devid) == PCI_CHIP_CANNONLAKE_Y_GT2_1 || \
				 (devid) == PCI_CHIP_CANNONLAKE_Y_GT2_2 || \
				 (devid) == PCI_CHIP_CANNONLAKE_Y_GT2_3 || \
				 (devid) == PCI_CHIP_CANNONLAKE_Y_GT2_4 || \
				 (devid) == PCI_CHIP_CANNONLAKE_Y_GT2_5)

#define IS_CNL_U(devid)		((devid) == PCI_CHIP_CANNONLAKE_U_GT2_0 || \
				 (devid) == PCI_CHIP_CANNONLAKE_U_GT2_1 || \
				 (devid) == PCI_CHIP_CANNONLAKE_U_GT2_2 || \
				 (devid) == PCI_CHIP_CANNONLAKE_U_GT2_3)

#define IS_CANNONLAKE(devid)	(IS_CNL_U(devid) || \
				 IS_CNL_Y(devid))

#define IS_GEN10(devid)		(IS_CANNONLAKE(devid))

#define IS_9XX(dev)		(IS_GEN3(dev) || \
				 IS_GEN4(dev) || \
				 IS_GEN5(dev) || \
				 IS_GEN6(dev) || \
				 IS_GEN7(dev) || \
				 IS_GEN8(dev) || \
				 IS_GEN9(dev) || \
				 IS_GEN10(dev))

#endif /* _INTEL_CHIPSET_H */
