/*!****************************************************************************

 @file         PVRTTriStrip.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Strips a triangle list.

******************************************************************************/
#ifndef _PVRTTRISTRIP_H_
#define _PVRTTRISTRIP_H_


/****************************************************************************
** Declarations
****************************************************************************/

/*!***************************************************************************
 @brief      		Reads a triangle list and generates an optimised triangle strip.
 @param[out]		ppui32Strips
 @param[out]		ppnStripLen
 @param[out]		pnStripCnt
 @param[in]			pui32TriList
 @param[in]			nTriCnt
*****************************************************************************/
void PVRTTriStrip(
	unsigned int			**ppui32Strips,
	unsigned int			**ppnStripLen,
	unsigned int			*pnStripCnt,
	const unsigned int	* const pui32TriList,
	const unsigned int		nTriCnt);


/*!***************************************************************************
 @brief      		Reads a triangle list and generates an optimised triangle strip. Result is
 					converted back to a triangle list.
 @param[in,out]		pui32TriList
 @param[in]			nTriCnt
*****************************************************************************/
void PVRTTriStripList(unsigned int * const pui32TriList, const unsigned int nTriCnt);


#endif /* _PVRTTRISTRIP_H_ */

/*****************************************************************************
 End of file (PVRTTriStrip.h)
*****************************************************************************/

