

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../chrome/elevation_service/elevation_service_idl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.xx.xxxx 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __elevation_service_idl_h__
#define __elevation_service_idl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRegisteredCommandElevator_FWD_DEFINED__
#define __IRegisteredCommandElevator_FWD_DEFINED__
typedef interface IRegisteredCommandElevator IRegisteredCommandElevator;

#endif 	/* __IRegisteredCommandElevator_FWD_DEFINED__ */


#ifndef __IRegisteredCommandElevator_FWD_DEFINED__
#define __IRegisteredCommandElevator_FWD_DEFINED__
typedef interface IRegisteredCommandElevator IRegisteredCommandElevator;

#endif 	/* __IRegisteredCommandElevator_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IRegisteredCommandElevator_INTERFACE_DEFINED__
#define __IRegisteredCommandElevator_INTERFACE_DEFINED__

/* interface IRegisteredCommandElevator */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IRegisteredCommandElevator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A949CB4E-C4F9-44C4-B213-6BF8AA9AC69C")
    IRegisteredCommandElevator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LaunchCommand( 
            /* [string][in] */ const WCHAR *cmd_id,
            /* [in] */ DWORD caller_proc_id,
            /* [out] */ ULONG_PTR *proc_handle) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IRegisteredCommandElevatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRegisteredCommandElevator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRegisteredCommandElevator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRegisteredCommandElevator * This);
        
        HRESULT ( STDMETHODCALLTYPE *LaunchCommand )( 
            IRegisteredCommandElevator * This,
            /* [string][in] */ const WCHAR *cmd_id,
            /* [in] */ DWORD caller_proc_id,
            /* [out] */ ULONG_PTR *proc_handle);
        
        END_INTERFACE
    } IRegisteredCommandElevatorVtbl;

    interface IRegisteredCommandElevator
    {
        CONST_VTBL struct IRegisteredCommandElevatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegisteredCommandElevator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IRegisteredCommandElevator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IRegisteredCommandElevator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IRegisteredCommandElevator_LaunchCommand(This,cmd_id,caller_proc_id,proc_handle)	\
    ( (This)->lpVtbl -> LaunchCommand(This,cmd_id,caller_proc_id,proc_handle) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IRegisteredCommandElevator_INTERFACE_DEFINED__ */



#ifndef __ElevatorLib_LIBRARY_DEFINED__
#define __ElevatorLib_LIBRARY_DEFINED__

/* library ElevatorLib */
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_ElevatorLib;
#endif /* __ElevatorLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


