/*
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _FIMG2D_REG_H_
#define _FIMG2D_REG_H_

#define SOFT_RESET_REG			(0x0000)
#define INTEN_REG			(0x0004)
#define INTC_PEND_REG			(0x000C)
#define FIFO_STAT_REG			(0x0010)
#define AXI_MODE_REG			(0x001C)
#define DMA_SFR_BASE_ADDR_REG		(0x0080)
#define DMA_COMMAND_REG			(0x0084)
#define DMA_EXE_LIST_NUM_REG		(0x0088)
#define DMA_STATUS_REG			(0x008C)
#define DMA_HOLD_CMD_REG		(0x0090)

/* COMMAND REGISTER */
#define BITBLT_START_REG		(0x0100)
#define BITBLT_COMMAND_REG		(0x0104)
#define BLEND_FUNCTION_REG		(0x0108)	/* VER4.1 */
#define ROUND_MODE_REG			(0x010C)	/* VER4.1 */

/* PARAMETER SETTING REGISTER */
#define ROTATE_REG			(0x0200)
#define SRC_MASK_DIRECT_REG		(0x0204)
#define DST_PAT_DIRECT_REG		(0x0208)

/* SOURCE */
#define SRC_SELECT_REG			(0x0300)
#define SRC_BASE_ADDR_REG		(0x0304)
#define SRC_STRIDE_REG			(0x0308)
#define SRC_COLOR_MODE_REG		(0x030c)
#define SRC_LEFT_TOP_REG		(0x0310)
#define SRC_RIGHT_BOTTOM_REG		(0x0314)
#define SRC_PLANE2_BASE_ADDR_REG	(0x0318)	/* VER4.1 */
#define SRC_REPEAT_MODE_REG		(0x031C)
#define SRC_PAD_VALUE_REG		(0x0320)
#define SRC_A8_RGB_EXT_REG		(0x0324)
#define SRC_SCALE_CTRL_REG		(0x0328)
#define SRC_XSCALE_REG			(0x032C)
#define SRC_YSCALE_REG			(0x0330)

/* DESTINATION */
#define DST_SELECT_REG			(0x0400)
#define DST_BASE_ADDR_REG		(0x0404)
#define DST_STRIDE_REG			(0x0408)
#define DST_COLOR_MODE_REG		(0x040C)
#define DST_LEFT_TOP_REG		(0x0410)
#define DST_RIGHT_BOTTOM_REG		(0x0414)
#define DST_PLANE2_BASE_ADDR_REG	(0x0418)	/* VER4.1 */
#define DST_A8_RGB_EXT_REG		(0x041C)

/* PATTERN */
#define PAT_BASE_ADDR_REG		(0x0500)
#define PAT_SIZE_REG			(0x0504)
#define PAT_COLOR_MODE_REG		(0x0508)
#define PAT_OFFSET_REG			(0x050C)
#define PAT_STRIDE_REG			(0x0510)

/* MASK	*/
#define MASK_BASE_ADDR_REG		(0x0520)
#define MASK_STRIDE_REG			(0x0524)
#define MASK_LEFT_TOP_REG		(0x0528)	/* VER4.1 */
#define MASK_RIGHT_BOTTOM_REG		(0x052C)	/* VER4.1 */
#define MASK_MODE_REG			(0x0530)	/* VER4.1 */
#define MASK_REPEAT_MODE_REG		(0x0534)
#define MASK_PAD_VALUE_REG		(0x0538)
#define MASK_SCALE_CTRL_REG		(0x053C)
#define MASK_XSCALE_REG			(0x0540)
#define MASK_YSCALE_REG			(0x0544)

/* CLIPPING WINDOW */
#define CW_LT_REG			(0x0600)
#define CW_RB_REG			(0x0604)

/* ROP & ALPHA SETTING */
#define THIRD_OPERAND_REG		(0x0610)
#define ROP4_REG			(0x0614)
#define ALPHA_REG			(0x0618)

/* COLOR SETTING */
#define FG_COLOR_REG			(0x0700)
#define BG_COLOR_REG			(0x0704)
#define BS_COLOR_REG			(0x0708)
#define SF_COLOR_REG			(0x070C)	/* VER4.1 */

/* COLOR KEY */
#define SRC_COLORKEY_CTRL_REG		(0x0710)
#define SRC_COLORKEY_DR_MIN_REG		(0x0714)
#define SRC_COLORKEY_DR_MAX_REG		(0x0718)
#define DST_COLORKEY_CTRL_REG		(0x071C)
#define DST_COLORKEY_DR_MIN_REG		(0x0720)
#define DST_COLORKEY_DR_MAX_REG		(0x0724)
/* YCbCr src Color Key */
#define YCbCr_SRC_COLORKEY_CTRL_REG	(0x0728)	/* VER4.1 */
#define YCbCr_SRC_COLORKEY_DR_MIN_REG	(0x072C)	/* VER4.1 */
#define YCbCr_SRC_COLORKEY_DR_MAX_REG	(0x0730)	/* VER4.1 */
/*Y CbCr dst Color Key */
#define YCbCr_DST_COLORKEY_CTRL_REG	(0x0734)	/* VER4.1 */
#define YCbCr_DST_COLORKEY_DR_MIN_REG	(0x0738)	/* VER4.1 */
#define YCbCr_DST_COLORKEY_DR_MAX_REG	(0x073C)	/* VER4.1 */

#endif

