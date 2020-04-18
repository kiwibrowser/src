/*!***********************************************************************

 @file           PVRScopeStats.h
 @copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved.
 @brief          PVRScopeStats header file. @copybrief ScopeStats
 
**************************************************************************/

/*! @mainpage PVRScope
 
 @section overview Library Overview
*****************************
PVRScope is a utility library which has two functionalities:
 \li @ref ScopeStats is used to access the hardware performance counters in
     PowerVR hardware via a driver library called PVRScopeServices.
 \li @ref ScopeComms allows an application to send user defined information to
     PVRTune via PVRPerfServer, both as counters and marks, or as editable data that can be
     passed back to the application.

PVRScope is supplied in the PVRScope.h header file. Your application also needs to link to the PVRScope 
library file, either a <tt>.lib</tt>, <tt>.so</tt>, or <tt>.dy</tt> file, depending on your platform.
 
For more information on PVRScope, see the <em>PVRScope User Manual</em>.

 @subsection limitStats PVRScopeStats Limitations
*****************************
@copydetails ScopeStats

 @subsection limitComms PVRScopeComms Limitations
*****************************
@copydetails ScopeComms
*/

#ifndef _PVRSCOPE_H_
#define _PVRSCOPE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 @addtogroup ScopeStats PVRScopeStats
 @brief    The PVRScopeStats functionality of PVRScope is used to access the hardware performance counters in
           PowerVR hardware via a driver library called PVRScopeServices.
 @details  PVRScopeStats has the following limitations:
          \li Only one instance of @ref ScopeStats may communicate with PVRScopeServices at any
              given time. If a PVRScope enabled application attempts to communicate with
              PVRScopeServices at the same time as another such application, or at the same time as
              PVRPerfServer, conflicts can occur that may make performance data unreliable.
          \li Performance counters can only be read on devices whose drivers have been built with
              hardware profiling enabled. This configuration is the default in most production drivers due to negligible overhead.
          \li Performance counters contain the average value of that counter since the last time the counter was interrogated.
 @{
*/


/****************************************************************************
** Enums
****************************************************************************/
/*!**************************************************************************
 @enum          EPVRScopeInitCode
 @brief         PVRScope initialisation return codes.
****************************************************************************/
enum EPVRScopeInitCode
{
    ePVRScopeInitCodeOk,                             ///< Initialisation OK
    ePVRScopeInitCodeOutOfMem,                       ///< Out of memory
    ePVRScopeInitCodeDriverSupportNotFound,	         ///< Driver support not found
    ePVRScopeInitCodeDriverSupportInsufficient,      ///< Driver support insufficient
    ePVRScopeInitCodeDriverSupportInitFailed,        ///< Driver support initialisation failed
    ePVRScopeInitCodeDriverSupportQueryInfoFailed,   ///< Driver support information query failed
    ePVRScopeInitCodeUnrecognisedHW                  ///< Unrecognised hardware
};

/****************************************************************************
** Structures
****************************************************************************/

// Internal implementation data
struct SPVRScopeImplData;

/*!**************************************************************************
 @struct        SPVRScopeCounterDef
 @brief         Definition of a counter that PVRScope calculates.
****************************************************************************/
struct SPVRScopeCounterDef
{
    const char      *pszName;                   ///< Counter name, null terminated
    bool			bPercentage;                ///< true if the counter is a percentage
    unsigned int    nGroup;                     ///< The counter group that the counter is in.
};

/*!**************************************************************************
 @struct        SPVRScopeCounterReading
 @brief         A set of return values resulting from querying the counter values.
****************************************************************************/
struct SPVRScopeCounterReading
{
    float           *pfValueBuf;                ///< Array of returned values
    unsigned int    nValueCnt;                  ///< Number of values set in the above array
    unsigned int    nReadingActiveGroup;        ///< Group that was active when counters were sampled
};

/*!**************************************************************************
 @struct        SPVRScopeGetInfo
 @brief         A set of return values holding miscellaneous PVRScope information.
****************************************************************************/
struct SPVRScopeGetInfo
{
    unsigned int    nGroupMax;                  ///< Highest group number of any counter
};

/****************************************************************************
** Declarations
****************************************************************************/

const char *PVRScopeGetDescription();           ///< Query the PVRScope library description


/*!**************************************************************************
 @brief         Initialise @ref ScopeStats, to access the HW performance counters in PowerVR.
 @return        EPVRScopeInitCodeOk on success.
****************************************************************************/
EPVRScopeInitCode PVRScopeInitialise(
    SPVRScopeImplData **ppsData                 ///< Context data
);

/*!**************************************************************************	
 @brief         Shutdown or de-initalise @ref ScopeStats and free the allocated memory.
***************************************************************************/
void PVRScopeDeInitialise(
    SPVRScopeImplData       **ppsData,          ///< Context data
    SPVRScopeCounterDef     **ppsCounters,      ///< Array of counters
    SPVRScopeCounterReading	* const psReading   ///< Results memory area
);
    
/*!**************************************************************************
 @brief         Query for @ref ScopeStats information. This function should only be called during initialisation.
****************************************************************************/
void PVRScopeGetInfo(
    SPVRScopeImplData   * const psData,         ///< Context data
    SPVRScopeGetInfo    * const psInfo          ///< Returned information
);
        
/*!**************************************************************************
 @brief         Query for the list of @ref ScopeStats HW performance counters, and
                allocate memory in which the counter values will be received. This function
                should only be called during initialisation.
****************************************************************************/
bool PVRScopeGetCounters(
	SPVRScopeImplData		* const psData,		///< Context data
	unsigned int			* const pnCount,	///< Returned number of counters
	SPVRScopeCounterDef		**ppsCounters,		///< Returned counter array
	SPVRScopeCounterReading	* const psReading	///< Pass a pointer to the structure to be initialised
);

/*!**************************************************************************
 @brief	        This function should be called regularly, such as once per frame. psReading
                may be NULL until a new reading is required, in order to smooth out values
                across longer time periods, perhaps a number of frames.
 @details       As and when desired, call this function to fill the counter-value array with
                the current counter values then change the active performance counter
                group. In a 3D application, you might call this once per frame or every N
                frames. Typically the group ID should be 0xffffffff in order to leave the
                active group unchanged; if it is desired to change it then pass the new
                group ID.
****************************************************************************/
bool PVRScopeReadCountersThenSetGroup(
	SPVRScopeImplData		* const psData,		///< Context data
	SPVRScopeCounterReading	* const psReading,	///< Returned data will be filled into the pointed-to structure
	const unsigned int		nTimeUS,			///< Current time, in microseconds. Ignored if psReading is NULL.
	const unsigned int		nGroup	    		///< New group; 0xffffffff to leave it unchanged
);

/*! @} */

#ifdef __cplusplus
}
#endif

#endif /* _PVRSCOPE_H_ */

/*****************************************************************************
 End of file (PVRScopeStats.h)
*****************************************************************************/
