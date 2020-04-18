/*!****************************************************************************

 @file         PVRTArray.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Expanding array template class. Allows appending and direct
               access. Mixing access methods should be approached with caution.

******************************************************************************/
#ifndef __PVRTARRAY_H__
#define __PVRTARRAY_H__

#include "PVRTGlobal.h"
#include "PVRTError.h"

/******************************************************************************
**  Classes
******************************************************************************/

/*!***************************************************************************
 @class       CPVRTArray
 @brief       Expanding array template class.
*****************************************************************************/
template<typename T>
class CPVRTArray
{
public:
	/*!***************************************************************************
	@brief     Blank constructor. Makes a default sized array.
	*****************************************************************************/
	CPVRTArray() : m_uiSize(0), m_uiCapacity(GetDefaultSize())
	{
		m_pArray = new T[m_uiCapacity];
	}

	/*!***************************************************************************
	@brief  	Constructor taking initial size of array in elements.
	@param[in]	uiSize	intial size of array
	*****************************************************************************/
	CPVRTArray(const unsigned int uiSize) : m_uiSize(0), m_uiCapacity(uiSize)
	{
		_ASSERT(uiSize != 0);
		m_pArray = new T[uiSize];
	}

	/*!***************************************************************************
	@brief      Copy constructor.
	@param[in]	original	the other dynamic array
	*****************************************************************************/
	CPVRTArray(const CPVRTArray& original) : m_uiSize(original.m_uiSize),
											  m_uiCapacity(original.m_uiCapacity)
	{
		m_pArray = new T[m_uiCapacity];
		for(unsigned int i=0;i<m_uiSize;i++)
		{
			m_pArray[i]=original.m_pArray[i];
		}
	}

	/*!***************************************************************************
	@brief      constructor from ordinary array.
	@param[in]	pArray		an ordinary array
	@param[in]	uiSize		number of elements passed
	*****************************************************************************/
	CPVRTArray(const T* const pArray, const unsigned int uiSize) : m_uiSize(uiSize),
														  m_uiCapacity(uiSize)
	{
		_ASSERT(uiSize != 0);
		m_pArray = new T[uiSize];
		for(unsigned int i=0;i<m_uiSize;i++)
		{
			m_pArray[i]=pArray[i];
		}
	}

	/*!***************************************************************************
	@brief      constructor from a capacity and initial value.
	@param[in]	uiSize		initial capacity
	@param[in]	val			value to populate with
	*****************************************************************************/
	CPVRTArray(const unsigned int uiSize, const T& val)	: m_uiSize(uiSize),
														m_uiCapacity(uiSize)
	{
		_ASSERT(uiSize != 0);
		m_pArray = new T[uiSize];
		for(unsigned int uiIndex = 0; uiIndex < m_uiSize; ++uiIndex)
		{
			m_pArray[uiIndex] = val;
		}
	}

	/*!***************************************************************************
	@brief      Destructor.
	*****************************************************************************/
	virtual ~CPVRTArray()
	{
		if(m_pArray)
			delete [] m_pArray;
	}

	/*!***************************************************************************
	@brief      Inserts an element into the array, expanding it
				if necessary.
	@param[in]	pos		The position to insert the new element at
	@param[in]	addT	The element to insert
	@return 	The index of the new item or -1 on failure.
	*****************************************************************************/
	int Insert(const unsigned int pos, const T& addT)
	{
		unsigned int uiIndex = pos;

		if(pos >= m_uiSize) // Are we adding to the end
			uiIndex = Append(addT);
		else
		{
			unsigned int uiNewCapacity = 0;
			T* pArray = m_pArray;

			if(m_uiSize > m_uiCapacity)
			{
				uiNewCapacity = m_uiCapacity + 10;	// Expand the array by 10.

				pArray = new T[uiNewCapacity];		// New Array

				if(!pArray)
					return -1;						// Failed to allocate memory!

				// Copy the first half to the new array
				for(unsigned int i = 0; i < pos; ++i)
				{
					pArray[i] = m_pArray[i];
				}
			}

			// Copy last half to the new array
			for(unsigned int i = m_uiSize; i > pos; --i)
			{
				pArray[i] = m_pArray[i - 1];
			}

			// Insert our new element
			pArray[pos] = addT;
			uiIndex = pos;

			// Increase our size
			++m_uiSize;

			// Switch pointers and free memory if needed
			if(pArray != m_pArray)
			{
				m_uiCapacity = uiNewCapacity;
				delete[] m_pArray;
				m_pArray = pArray;
			}
		}

		return uiIndex;
	}

	/*!***************************************************************************
	@brief      Appends an element to the end of the array, expanding it
				if necessary.
	@param[in]	addT	The element to append
	@return 	The index of the new item.
	*****************************************************************************/
	unsigned int Append(const T& addT)
	{
		unsigned int uiIndex = Append();
		m_pArray[uiIndex] = addT;
		return uiIndex;
	}

	/*!***************************************************************************
	@brief      Creates space for a new item, but doesn't add. Instead
				returns the index of the new item.
	@return 	The index of the new item.
	*****************************************************************************/
	unsigned int Append()
	{
		unsigned int uiIndex = m_uiSize;
		SetCapacity(m_uiSize+1);
		m_uiSize++;

		return uiIndex;
	}

	/*!***************************************************************************
	@brief      Clears the array.
	*****************************************************************************/
	void Clear()
	{
		m_uiSize = 0U;
	}

	/*!***************************************************************************
	@brief      Changes the array to the new size
	@param[in]	uiSize		New size of array
	*****************************************************************************/
	EPVRTError Resize(const unsigned int uiSize)
	{
		EPVRTError err = SetCapacity(uiSize);

		if(err != PVR_SUCCESS)
			return err;

		m_uiSize = uiSize;
		return PVR_SUCCESS;
	}

	/*!***************************************************************************
	@brief      Expands array to new capacity
	@param[in]	uiSize		New capacity of array
	*****************************************************************************/
	EPVRTError SetCapacity(const unsigned int uiSize)
	{
		if(uiSize <= m_uiCapacity)
			return PVR_SUCCESS;	// nothing to be done

		unsigned int uiNewCapacity;
		if(uiSize < m_uiCapacity*2)
		{
			uiNewCapacity = m_uiCapacity*2;			// Ignore the new size. Expand to twice the previous size.
		}
		else
		{
			uiNewCapacity = uiSize;
		}

		T* pNewArray = new T[uiNewCapacity];		// New Array
		if(!pNewArray)
			return PVR_FAIL;						// Failed to allocate memory!

		// Copy source data to new array
		for(unsigned int i = 0; i < m_uiSize; ++i)
		{
			pNewArray[i] = m_pArray[i];
		}

		// Switch pointers and free memory
		m_uiCapacity	= uiNewCapacity;
		T* pOldArray	= m_pArray;
		m_pArray		= pNewArray;
		delete [] pOldArray;
		return PVR_SUCCESS;
	}

	/*!***************************************************************************
	@fn     	Copy
	@brief      A copy function. Will attempt to copy from other CPVRTArrays
				if this is possible.
	@param[in]	other	The CPVRTArray needing copied
	*****************************************************************************/
	template<typename T2>
	void Copy(const CPVRTArray<T2>& other)
	{
		T* pNewArray = new T[other.GetCapacity()];
		if(pNewArray)
		{
			// Copy data
			for(unsigned int i = 0; i < other.GetSize(); i++)
			{
				pNewArray[i] = other[i];
			}

			// Free current array
			if(m_pArray)
				delete [] m_pArray;

			// Swap pointers
			m_pArray		= pNewArray;

			m_uiCapacity	= other.GetCapacity();
			m_uiSize		= other.GetSize();
		}
	}

	/*!***************************************************************************
	@brief      assignment operator.
	@param[in]	other	The CPVRTArray needing copied
	*****************************************************************************/
	CPVRTArray& operator=(const CPVRTArray<T>& other)
	{
		if(&other != this)
			Copy(other);

		return *this;
	}

	/*!***************************************************************************
	@brief      appends an existing CPVRTArray on to this one.
	@param[in]	other		the array to append.
	*****************************************************************************/
	CPVRTArray& operator+=(const CPVRTArray<T>& other)
	{
		if(&other != this)
		{
			for(unsigned int uiIndex = 0; uiIndex < other.GetSize(); ++uiIndex)
			{
				Append(other[uiIndex]);
			}
		}

		return *this;
	}

	/*!***************************************************************************
	@brief      Indexed access into array. Note that this has no error
				checking whatsoever
	@param[in]	uiIndex	index of element in array
	@return 	the element indexed
	*****************************************************************************/
	T& operator[](const unsigned int uiIndex)
	{
		_ASSERT(uiIndex < m_uiCapacity);
		return m_pArray[uiIndex];
	}

	/*!***************************************************************************
	@brief      Indexed access into array. Note that this has no error checking whatsoever
	@param[in]	uiIndex	    index of element in array
	@return 	The element indexed
	*****************************************************************************/
	const T& operator[](const unsigned int uiIndex) const
	{
		_ASSERT(uiIndex < m_uiCapacity);
		return m_pArray[uiIndex];
	}

	/*!***************************************************************************
	@return 	Size of array
	@brief      Gives current size of array/number of elements
	*****************************************************************************/
	unsigned int GetSize() const
	{
		return m_uiSize;
	}

	/*!***************************************************************************
	@brief      Gives the default size of array/number of elements
	@return 	Default size of array
	*****************************************************************************/
	static unsigned int GetDefaultSize()
	{
		return 16U;
	}

	/*!***************************************************************************
	@brief      Gives current allocated size of array/number of elements
	@return 	Capacity of array
	*****************************************************************************/
	unsigned int GetCapacity() const
	{
		return m_uiCapacity;
	}

	/*!***************************************************************************
	@brief      Indicates whether the given object resides inside the array. 
	@param[in]	object		The object to check in the array
	@return 	true if object is contained in this array.
	*****************************************************************************/
	bool Contains(const T& object) const
	{
		for(unsigned int uiIndex = 0; uiIndex < m_uiSize; ++uiIndex)
		{
			if(m_pArray[uiIndex] == object)
				return true;
		}
		return false;
	}

	/*!***************************************************************************
	@brief     	Attempts to find the object in the array and returns a
				pointer if it is found, or NULL if not found. The time
				taken is O(N).
	@param[in]	object		The object to check in the array
	@return 	Pointer to the found object or NULL.
	*****************************************************************************/
	T* Find(const T& object) const
	{
		for(unsigned int uiIndex = 0; uiIndex < m_uiSize; ++uiIndex)
		{
			if(m_pArray[uiIndex] == object)
				return &m_pArray[uiIndex];
		}
		return NULL;
	}

	/*!***************************************************************************
	@brief      Simple bubble-sort of the array. Pred should be an object that
				defines a bool operator().
	@param[in]	predicate		The object which defines "bool operator()"
	*****************************************************************************/
	template<class Pred>
	void Sort(Pred predicate)
	{
		bool bSwap;
		for(unsigned int i=0; i < m_uiSize; ++i)
		{
			bSwap = false;
			for(unsigned int j=0; j < m_uiSize-1; ++j)
			{
				if(predicate(m_pArray[j], m_pArray[j+1]))
				{
					PVRTswap(m_pArray[j], m_pArray[j+1]);
					bSwap = true;
				}
			}

			if(!bSwap)
				return;
		}
	}

	/*!***************************************************************************
	@brief      Removes an element from the array.
	@param[in]	uiIndex		The index to remove
	@return 	success or failure
	*****************************************************************************/
	virtual EPVRTError Remove(unsigned int uiIndex)
	{
		_ASSERT(uiIndex < m_uiSize);
		if(m_uiSize == 0)
			return PVR_FAIL;

		if(uiIndex == m_uiSize-1)
		{
			return RemoveLast();
		}
        
        m_uiSize--;
        // Copy the data. memmove will only work for built-in types.
        for(unsigned int uiNewIdx = uiIndex; uiNewIdx < m_uiSize; ++uiNewIdx)
        {
            m_pArray[uiNewIdx] = m_pArray[uiNewIdx+1];
        }
		
		return PVR_SUCCESS;
	}

	/*!***************************************************************************
	@brief    	Removes the last element. Simply decrements the size value
	@return 	success or failure
	*****************************************************************************/
	virtual EPVRTError RemoveLast()
	{
		if(m_uiSize > 0)
		{
			m_uiSize--;
			return PVR_SUCCESS;
		}	
		else
		{
			return PVR_FAIL;
		}
	}

protected:
	unsigned int 	m_uiSize;		/*!< Current size of contents of array */
	unsigned int	m_uiCapacity;	/*!< Currently allocated size of array */
	T				*m_pArray;		/*!< The actual array itself */
};

// note "this" is required for ISO standard, C++ and gcc complains otherwise
// http://lists.apple.com/archives/Xcode-users//2005/Dec/msg00644.html

/*!***************************************************************************
 @class       CPVRTArrayManagedPointers
 @brief       Maintains an array of managed pointers.
*****************************************************************************/
template<typename T>
class CPVRTArrayManagedPointers : public CPVRTArray<T*>
{
public:
	/*!***************************************************************************
	@brief     Destructor.
	*****************************************************************************/
    virtual ~CPVRTArrayManagedPointers()
	{
		if(this->m_pArray)
		{
			for(unsigned int i=0;i<this->m_uiSize;i++)
			{
				delete(this->m_pArray[i]);
			}
		}
	}

	/*!***************************************************************************
	@brief      Removes an element from the array.
	@param[in]	uiIndex		The index to remove.
	@return 	success or failure
	*****************************************************************************/
	virtual EPVRTError Remove(unsigned int uiIndex)
	{
		_ASSERT(uiIndex < this->m_uiSize);
		if(this->m_uiSize == 0)
			return PVR_FAIL;

		if(uiIndex == this->m_uiSize-1)
		{
			return this->RemoveLast();
		}

		unsigned int uiSize = (this->m_uiSize - (uiIndex+1)) * sizeof(T*);
	
		delete this->m_pArray[uiIndex];
		memmove(this->m_pArray + uiIndex, this->m_pArray + (uiIndex+1), uiSize);

		this->m_uiSize--;
		return PVR_SUCCESS;
	}

	/*!***************************************************************************
	@brief      Removes the last element. Simply decrements the size value
	@return 	success or failure
	*****************************************************************************/
	virtual EPVRTError RemoveLast()
	{
		if(this->m_uiSize > 0 && this->m_pArray)
		{
			delete this->m_pArray[this->m_uiSize-1];
			this->m_uiSize--;
			return PVR_SUCCESS;
		}
		else
		{
			return PVR_FAIL;
		}
	}
};

#endif // __PVRTARRAY_H__

/*****************************************************************************
End of file (PVRTArray.h)
*****************************************************************************/

