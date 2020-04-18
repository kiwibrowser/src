// mach_override.h semver:1.2.0
//   Copyright (c) 2003-2012 Jonathan 'Wolf' Rentzsch: http://rentzsch.com
//   Some rights reserved: http://opensource.org/licenses/mit
//   https://github.com/rentzsch/mach_override

#ifndef		_mach_override_
#define		_mach_override_

#include <sys/types.h>
#include <mach/error.h>

#define	err_cannot_override	(err_local|1)

__BEGIN_DECLS

/****************************************************************************************
	Dynamically overrides the function implementation referenced by
	originalFunctionAddress with the implentation pointed to by overrideFunctionAddress.
	Optionally returns a pointer to a "reentry island" which, if jumped to, will resume
	the original implementation.
	
	@param	originalFunctionAddress			->	Required address of the function to
												override (with overrideFunctionAddress).
	@param	overrideFunctionAddress			->	Required address to the overriding
												function.
	@param	originalFunctionReentryIsland	<-	Optional pointer to pointer to the
												reentry island. Can be NULL.
	@result									<-	err_cannot_override if the original
												function's implementation begins with
												the 'mfctr' instruction.

	************************************************************************************/

    mach_error_t
mach_override_ptr(
	void *originalFunctionAddress,
    const void *overrideFunctionAddress,
    void **originalFunctionReentryIsland );

/****************************************************************************************
	mach_override_ptr makes multiple allocation attempts with vm_allocate or malloc,
  until a suitable address is found for the branch islands. This method returns the
  global number of such attempts made by all mach_override_ptr calls so far. This
  statistic is provided for testing purposes and it can be off, if mach_override_ptr
  is called by multiple threads.

	@result									<-	Total number of vm_allocate calls so far.

	************************************************************************************/
u_int64_t mach_override_ptr_allocation_attempts();

__END_DECLS

/****************************************************************************************
	If you're using C++ this macro will ease the tedium of typedef'ing, naming, keeping
	track of reentry islands and defining your override code. See test_mach_override.cp
	for example usage.

	************************************************************************************/
 
#ifdef	__cplusplus
#define MACH_OVERRIDE( ORIGINAL_FUNCTION_RETURN_TYPE, ORIGINAL_FUNCTION_NAME, ORIGINAL_FUNCTION_ARGS, ERR )		\
{																												\
	static ORIGINAL_FUNCTION_RETURN_TYPE (*ORIGINAL_FUNCTION_NAME##_reenter)ORIGINAL_FUNCTION_ARGS;				\
	static bool ORIGINAL_FUNCTION_NAME##_overriden = false;														\
	class mach_override_class__##ORIGINAL_FUNCTION_NAME {														\
	public:																										\
		static kern_return_t override(void *originalFunctionPtr) {												\
			kern_return_t result = err_none;																	\
			if (!ORIGINAL_FUNCTION_NAME##_overriden) {															\
				ORIGINAL_FUNCTION_NAME##_overriden = true;														\
				result = mach_override_ptr( (void*)originalFunctionPtr,											\
											(void*)mach_override_class__##ORIGINAL_FUNCTION_NAME::replacement,	\
											(void**)&ORIGINAL_FUNCTION_NAME##_reenter );						\
			}																									\
			return result;																						\
		}																										\
		static ORIGINAL_FUNCTION_RETURN_TYPE replacement ORIGINAL_FUNCTION_ARGS {

#define END_MACH_OVERRIDE( ORIGINAL_FUNCTION_NAME )																\
		}																										\
	};																											\
																												\
	err = mach_override_class__##ORIGINAL_FUNCTION_NAME::override((void*)ORIGINAL_FUNCTION_NAME);				\
}
#endif

#endif	//	_mach_override_
