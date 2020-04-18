/*!***********************************************************************

 @file           PVRScopeComms.h
 @copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved.
 @brief          PVRScopeComms header file. @copybrief ScopeComms
 
**************************************************************************/

#ifndef _PVRSCOPE_H_
#define _PVRSCOPE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
	SGSPerfServer and PVRTune communications
*/

/*!
 @addtogroup ScopeComms PVRScopeComms
 @brief      The PVRScopeComms functionality of PVRScope allows an application to send user defined information to
             PVRTune via PVRPerfServer, both as counters and marks, or as editable data that can be
             passed back to the application.
 @details    PVRScopeComms has the following limitations:
             \li PVRPerfServer must be running on the host device if a @ref ScopeComms enabled
                 application wishes to send custom counters or marks to PVRTune. If the application in
                 question also wishes to communicate with PVRScopeServices without experiencing any
                 undesired behaviour PVRPerfServer should be run with the '--disable-hwperf' flag.
             \li The following types may be sent: Boolean, Enumerator, Float, Integer, String.
 @{
*/

/****************************************************************************
** Enums
****************************************************************************/
    
/*!**************************************************************************
 @enum          ESPSCommsLibType
 @brief         Each editable library item has a data type associated with it
****************************************************************************/
/// 
enum ESPSCommsLibType
{
	eSPSCommsLibTypeString,		///< data is string (NOT NULL-terminated, use length parameter)
	eSPSCommsLibTypeFloat,		///< data is SSPSCommsLibraryTypeFloat
	eSPSCommsLibTypeInt,		///< data is SSPSCommsLibraryTypeInt
	eSPSCommsLibTypeEnum,		///< data is string (NOT NULL-terminated, use length parameter). First line is selection number, subsequent lines are available options.
	eSPSCommsLibTypeBool		///< data is SSPSCommsLibraryTypeBool
};

/****************************************************************************
** Structures
****************************************************************************/

// Internal implementation data
struct SSPSCommsData;
    
/*!**************************************************************************
 @struct        SSPSCommsLibraryItem
 @brief         Definition of one editable library item
****************************************************************************/
struct SSPSCommsLibraryItem
{
	
	const char			*pszName;       ///< Item name. If dots are used, PVRTune could show these as a foldable tree view.
	unsigned int		nNameLength;	///< Item name length

	ESPSCommsLibType	eType;			///< Item type

	const char			*pData;         ///< Item data
	unsigned int		nDataLength;	///< Item data length
};
    
/*!**************************************************************************
 @struct        SSPSCommsLibraryTypeFloat
 @brief         Current, minimum and maximum values for an editable library item of type float
****************************************************************************/
struct SSPSCommsLibraryTypeFloat
{
	float fCurrent;						///< Current value
	float fMin;							///< Minimal value
	float fMax;							///< Maximum value
};
    
/*!**************************************************************************
 @struct        SSPSCommsLibraryTypeInt
 @brief         Current, minimum and maximum values for an editable library item of type int
****************************************************************************/
struct SSPSCommsLibraryTypeInt
{
	int nCurrent;						///< Current value
	int nMin;    						///< Minimal value
	int nMax;    						///< Maximum value
};
    
/*!**************************************************************************
 @struct        SSPSCommsLibraryTypeBool 
 @brief         Current value for an editable library item of type bool
****************************************************************************/
struct SSPSCommsLibraryTypeBool
{
	bool bValue;                            ///< Boolean value
};
    
/*!**************************************************************************
 @struct        SSPSCommsCounterDef
 @brief         Definition of one custom counter
****************************************************************************/
struct SSPSCommsCounterDef
{
	const char			*pszName;		    ///< Custom counter name
	unsigned int		nNameLength;	    ///< Custom counter name length
};

/****************************************************************************
** Declarations
****************************************************************************/

    
/*!**************************************************************************
 @brief         Initialise @ref ScopeComms
 @return        @ref ScopeComms data.
****************************************************************************/
SSPSCommsData *pplInitialise(
	const char			* const pszName,	///< String to describe the application
	const unsigned int	nNameLen			///< String length
);
    
/*!**************************************************************************
 @brief         Shutdown or de-initialise the remote control section of PVRScope.
****************************************************************************/
void pplShutdown(
    SSPSCommsData *psData			        ///< Context data
);

/*!**************************************************************************
 @brief         Query for the time. Units are microseconds, resolution is undefined.
****************************************************************************/
unsigned int pplGetTimeUS(
	SSPSCommsData		&sData	        ///< Context data
);

/*!**************************************************************************
 @brief         Send a time-stamped string marker to be displayed in PVRTune.
 @details       Examples might be:
                \li switching to outdoor renderer
                \li starting benchmark test N
****************************************************************************/
bool pplSendMark(
	SSPSCommsData		&sData,				///< Context data
	const char			* const pszString,	///< Time-stamped string
	const unsigned int	nLen				///< String length
);

/*!**************************************************************************
 @brief         Send a time-stamped begin marker to PVRTune. 
 @details       Every begin must be followed by an end; they cannot be interleaved or 
                nested. PVRTune will show these as an activity timeline.
****************************************************************************/
bool pplSendProcessingBegin(
	SSPSCommsData		&sData,				///< Context data
	const char			* const	pszString,	///< Name of the processing block
	const unsigned int	nLen,				///< String length
	const unsigned int	nFrame=0			///< Iteration (or frame) number, by which processes can be grouped.
);

/*!**************************************************************************
 @brief         Send a time-stamped end marker to PVRTune. 
 @details       Every end must be preceeded by a begin; they cannot be interleaved or 
                nested. PVRTune will show these as an activity timeline.
****************************************************************************/
bool pplSendProcessingEnd(
	SSPSCommsData		&sData      ///< Context data
);

/*!**************************************************************************
 @brief         Create a library of remotely editable items
****************************************************************************/
bool pplLibraryCreate(
	SSPSCommsData				&sData,			///< Context data
	const SSPSCommsLibraryItem	* const pItems,	///< Editable items
	const unsigned int			nItemCount		///< Number of items
);

/*!**************************************************************************
 @brief	        Query to see whether a library item has been edited, and also retrieve the new data.
****************************************************************************/
bool pplLibraryDirtyGetFirst(
	SSPSCommsData		&sData,	                ///< Context data
	unsigned int		&nItem,	                ///< Item number
	unsigned int		&nNewDataLen,           ///< New data length
	const char			**ppData        	    ///< New data
);

/*!**************************************************************************
 @brief         Specify the number of custom counters and their definitions
****************************************************************************/
bool pplCountersCreate(
	SSPSCommsData				&sData,	                ///< Context data
	const SSPSCommsCounterDef	* const psCounterDefs,  ///< Counter definitions
	const unsigned int			nCount    	            ///< Number of counters
);

/*!**************************************************************************
 @brief 	    Send an update for all the custom counters. The psCounterReadings array must be
                nCount long.
****************************************************************************/
bool pplCountersUpdate(
	SSPSCommsData				&sData,	                    ///< Context data
	const unsigned int			* const psCounterReadings	///< Counter readings array
);
        
/*!**************************************************************************
 @brief	        Force a cache flush.
 @details       Some implementations store data sends in the cache. If the data rate is low, the real
                send of data can be significantly delayed. 
                If it is necessary to flush the cache, the best results are likely to be
                achieved by calling this function with a frequency between once per second up to once per
                frame. If data is sent extremely infrequently, this function could be
                called once at the end of each bout of data send.
****************************************************************************/
bool pplSendFlush(
	SSPSCommsData				&sData	    ///< Context data
);

/*! @} */

#ifdef __cplusplus
}
#endif

#endif /* _PVRSCOPE_H_ */

/*****************************************************************************
 End of file (PVRScopeComms.h)
*****************************************************************************/
