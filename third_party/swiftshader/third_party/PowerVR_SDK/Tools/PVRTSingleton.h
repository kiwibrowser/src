/*!****************************************************************************

 @file         PVRTSingleton.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Singleton template. 
 @details      Pattern Usage: Inherit from CPVRTSingleton
               class like this: class Foo : public CPVRTSingleton<Foo> { ... };

******************************************************************************/
#ifndef __PVRTSINGLETON__
#define __PVRTSINGLETON__

/*!****************************************************************************
 @class        CPVRTSingleton
 @brief        Singleton template.
 @details      Pattern Usage: Inherit from CPVRTSingleton class like this: 
               class Foo : public CPVRTSingleton<Foo> { ... };
******************************************************************************/
template<typename T> class CPVRTSingleton
{
private:
    /*! @brief  Constructor. */
	CPVRTSingleton(const CPVRTSingleton&);
	
    /*! @brief  Deconstructor. */
    CPVRTSingleton & operator=(const CPVRTSingleton&);

public:
	static T& inst()
	{
		static T object;
		return object;
	}

	static T* ptr()
	{
		return &inst();
	}

protected:
	CPVRTSingleton() {};
	virtual ~CPVRTSingleton() {};
};


#endif // __PVRTSINGLETON__

/*****************************************************************************
End of file (PVRTSingleton.h)
*****************************************************************************/

