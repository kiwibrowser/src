/******************************************************************************

 @File         PVRTBoneBatch.cpp

 @Title        PVRTBoneBatch

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Utility functions which process vertices.

******************************************************************************/

/****************************************************************************
** Includes
****************************************************************************/
#include "PVRTGlobal.h"
#include "PVRTContext.h"

#include <vector>
#include <list>

#include "PVRTMatrix.h"
#include "PVRTVertex.h"
#include "PVRTBoneBatch.h"

/****************************************************************************
** Defines
****************************************************************************/

/****************************************************************************
** Macros
****************************************************************************/

/****************************************************************************
** Structures
****************************************************************************/
/*!***************************************************************************
@Class CBatch
@Brief Class to contain and manage batch information.
*****************************************************************************/
class CBatch
{
protected:
	int	m_nCapacity, 		// Maximum size of the batch
	m_nCnt, 				// Number of elements currently contained in the batch
	*m_pnPalette;			// Array of palette indices

public:
/*!***************************************************************************
 @Function		CBatch
 @Description	The default constructor
*****************************************************************************/
	CBatch() : 	m_nCapacity(0),
				m_nCnt(0),
				m_pnPalette(0)
	{
	}

/*!***************************************************************************
 @Function		CBatch
 @Input			src				CBatch to copy
 @Description	Copy constructor
*****************************************************************************/
	CBatch(const CBatch &src) : m_pnPalette(0)
	{
		SetSize(src.m_nCapacity);
		*this = src;
	}
	
/*!***************************************************************************
 @Function		~CBatch
 @Description	Destructor
*****************************************************************************/
	~CBatch()
	{
		FREE(m_pnPalette);
	}
	
/*!***************************************************************************
 @Function		operator=
 @Description	Operator overload for the '=' operand
*****************************************************************************/
	CBatch& operator= (const CBatch &src)
	{
		_ASSERT(m_nCapacity == src.m_nCapacity);
		m_nCnt = src.m_nCnt;
		memcpy(m_pnPalette, src.m_pnPalette, m_nCnt * sizeof(*m_pnPalette));
		return *this;
	}
	
/*!***************************************************************************
 @Function		SetSize
 @Input			nSize			The new size of the batch
 @Description	Delete all current information and resizes the batch 
				to the value that has been passed in.
*****************************************************************************/
	void SetSize(const int nSize)
	{
		FREE(m_pnPalette);

		m_nCapacity	= nSize;
		m_nCnt		= 0;
		m_pnPalette		= (int*)malloc(m_nCapacity * sizeof(*m_pnPalette));
	}
	
/*!***************************************************************************
 @Function		Clear
 @Description	Resets the count
*****************************************************************************/
	void Clear()
	{
		m_nCnt = 0;
	}

/*!***************************************************************************
 @Function		Clear
 @Input			n			The index of the new item
 Return			bool		Returns true if the item already exists or has been added.
 @Description	Adds a new item to the batch, providing it has not already
				been added to the batch and the count doesn't exceed the
				maximum number of bones the batch can hold.
*****************************************************************************/
	bool Add(const int n)
	{
		int i;

		if(n < 0)
			return false;

		// If we already have this item, do nothing
		for(i = 0; i < m_nCnt; ++i)
		{
			if(m_pnPalette[i] == n)
				return true;
		}

		// Add the new item
		if(m_nCnt < m_nCapacity)
		{
			m_pnPalette[m_nCnt] = n;
			++m_nCnt;
			return true;
		}
		else
		{
			return false;
		}
	}

/*!***************************************************************************
 @Function		Merge
 @Input			src				The batch to merge with
 @Description	Merges the input batch with the current batch.
*****************************************************************************/
	void Merge(const CBatch &src)
	{
		int i;

		for(i = 0; i < src.m_nCnt; ++i)
			Add(src.m_pnPalette[i]);
	}

/*!***************************************************************************
 @Function		TestMerge
 @Input			src				The batch to merge with
 @Return		int				The number of items that are not already
								present in the batch. -1 if the merge will
								exceed the capacity of the batch
 @Description	Tests how many of the items of the input batch are not
				already contained in the batch. This returns the number of
				items that would need to be added, or -1 if the number
				of additional items would exceed the capacity of the batch.
*****************************************************************************/
	int TestMerge(const CBatch &src)
	{
		int i, nCnt;

		nCnt = 0;
		for(i = 0; i < src.m_nCnt; ++i)
			if(!Contains(src.m_pnPalette[i]))
				++nCnt;

		return m_nCnt+nCnt > m_nCapacity ? -1 : nCnt;
	}

/*!***************************************************************************
 @Function		Contains
 @Input			src				The batch to compare
 @Return		bool			Returns true if the batch and the input batch
								have at least one item in common
 @Description	Returns true if the batch's have at least one item in common
*****************************************************************************/
	bool Contains(const CBatch &batch) const
	{
		int i;

		for(i = 0; i < batch.m_nCnt; ++i)
			if(!Contains(batch.m_pnPalette[i]))
				return false;

		return true;
	}
	
/*!***************************************************************************
 @Function		Contains
 @Input			n				The index of the new item
 @Return		bool			Returns true if the batch contains the item
 @Description	Returns true if the batch contains the item.
*****************************************************************************/
	bool Contains(const int n) const
	{
		int i;

		for(i = 0; i < m_nCnt; ++i)
			if(m_pnPalette[i] == n)
				return true;

		return false;
	}
	
/*!***************************************************************************
 @Function		Write
 @Output		pn				The array of items to overwrite
 @Output		pnCnt			The number of items in the array
 @Description	Writes the array of items and the number of items to the output
				parameters.
*****************************************************************************/
	void Write(
		int * const pn,
		int * const pnCnt) const
	{
		memcpy(pn, m_pnPalette, m_nCnt * sizeof(*pn));
		*pnCnt = m_nCnt;
	}

/*!***************************************************************************
 @Function		GetVertexBoneIndices
 @Modified		pfI				Returned index
 @Input			pfW				Weight?
 @Input			n				Length of index array
 @Description	For each element of the input array, the index value is compared
				with the palette's index value. If the values are equal, the
				value of the current input array element is replaced with the
				palette index, otherwise the value is set to zero.
*****************************************************************************/
	void GetVertexBoneIndices(
		float		* const pfI,
		const float	* const pfW,
		const int	n)
	{
		int i, j;

		for(i = 0; i < n; ++i)
		{
			if(pfW[i] != 0)
			{
				for(j = 0; j < m_nCnt; ++j)
				{
					if(pfI[i] != m_pnPalette[j])
						continue;

					pfI[i] = (float)j;
					break;
				}

				// This batch *must* contain this vertex
				_ASSERT(j != m_nCnt);
			}
			else
			{
				pfI[i] = 0;
			}
		}
	}
};

/*!***************************************************************************
@Class CGrowableArray
@Brief Class that provides an array structure that can change its size dynamically.
*****************************************************************************/
class CGrowableArray
{
protected:
	char	*m_p;
	int		m_nSize;
	int		m_nCnt;

public:
/*!***************************************************************************
 @Function		CGrowableArray
 @Input			nSize			The size of the data (in bytes) that the array will contain
 @Description	Initialises the size of the data the array will contain to the
				value that has been passed in and initialises the remaining
				data members with default values.
*****************************************************************************/
	CGrowableArray(const int nSize)
	{
		m_p		= NULL;
		m_nSize	= nSize;
		m_nCnt	= 0;
	}
	
/*!***************************************************************************
 @Function		~CGrowableArray
 @Description	The destructor
*****************************************************************************/
	~CGrowableArray()
	{
		FREE(m_p);
	}
	
/*!***************************************************************************
 @Function		Append
 @Input			pData			The data to append
 @Input			nCnt			The amount of data elements to append
 @Description	Resizes the array and appends the new data that has been passed in.
*****************************************************************************/
	void Append(const void * const pData, const int nCnt)
	{
		m_p = (char*)realloc(m_p, (m_nCnt + nCnt) * m_nSize);
		_ASSERT(m_p);

		memcpy(&m_p[m_nCnt * m_nSize], pData, nCnt * m_nSize);
		m_nCnt += nCnt;
	}

/*!***************************************************************************
 @Function		last
 @Return		char*			The last element of the array
 @Description	Returns a pointer to the last element of the array.
*****************************************************************************/
	char *last()
	{
		return at(m_nCnt-1);
	}

/*!***************************************************************************
 @Function		at
 @Input			nIdx			The index of the requested element
 @Return		char*			The element at the specified index of the array
 @Description	Returns a pointer to the data at the specified index of the array.
*****************************************************************************/	
	char *at(const int nIdx)
	{
		return &m_p[nIdx * m_nSize];
	}

/*!***************************************************************************
 @Function		size
 @Return		int				The number of elements contained in the array
 @Description	Returns the number of elements contained in the array.
*****************************************************************************/
	int size() const
	{
		return m_nCnt;
	}

/*!***************************************************************************
 @Function		Surrender
 @Output		pData			The pointer to surrender the data to
 @Description	Assigns the memory address of the data to the pointer that has
				been passed in. Sets the class's number of elements and 
				data pointer back to their default values.
*****************************************************************************/
	int Surrender(
		char ** const pData)
	{
		int nCnt;

		*pData = m_p;
		nCnt = m_nCnt;

		m_p		= NULL;
		m_nCnt	= 0;

		return nCnt;
	}
};

/****************************************************************************
** Constants
****************************************************************************/

/****************************************************************************
** Local function definitions
****************************************************************************/
static bool FillBatch(
	CBatch					&batch,
	const unsigned int	* const pui32Idx,	// input AND output; index array for triangle list
	const char				* const pVtx,	// Input vertices
	const int				nStride,		// Size of a vertex (in bytes)
	const int				nOffsetWeight,	// Offset in bytes to the vertex bone-weights
	EPVRTDataType			eTypeWeight,	// Data type of the vertex bone-weights
	const int				nOffsetIdx,		// Offset in bytes to the vertex bone-indices
	EPVRTDataType			eTypeIdx,		// Data type of the vertex bone-indices
	const int				nVertexBones);	// Number of bones affecting each vertex

static bool BonesMatch(
	const float * const pfIdx0,
	const float * const pfIdx1);

/*****************************************************************************
** Functions
*****************************************************************************/

/*!***************************************************************************
 @Function		Create
 @Output		pnVtxNumOut		vertex count
 @Output		pVtxOut			Output vertices (program must free() this)
 @Modified		pui32Idx			index array for triangle list
 @Input			nVtxNum			vertex count
 @Input			pVtx			vertices
 @Input			nStride			Size of a vertex (in bytes)
 @Input			nOffsetWeight	Offset in bytes to the vertex bone-weights
 @Input			eTypeWeight		Data type of the vertex bone-weights
 @Input			nOffsetIdx		Offset in bytes to the vertex bone-indices
 @Input			eTypeIdx		Data type of the vertex bone-indices
 @Input			nTriNum			Number of triangles
 @Input			nBatchBoneMax	Number of bones a batch can reference
 @Input			nVertexBones	Number of bones affecting each vertex
 @Returns		PVR_SUCCESS if successful
 @Description	Fills the bone batch structure
*****************************************************************************/
EPVRTError CPVRTBoneBatches::Create(
	int					* const pnVtxNumOut,
	char				** const pVtxOut,
	unsigned int		* const pui32Idx,
	const int			nVtxNum,
	const char			* const pVtx,
	const int			nStride,
	const int			nOffsetWeight,
	const EPVRTDataType	eTypeWeight,
	const int			nOffsetIdx,
	const EPVRTDataType	eTypeIdx,
	const int			nTriNum,
	const int			nBatchBoneMax,
	const int			nVertexBones)
{
	int							i, j, k, nTriCnt;
	CBatch						batch;
	std::list<CBatch>			lBatch;
	std::list<CBatch>::iterator	iBatch, iBatch2;
	CBatch						**ppBatch;
	unsigned int				*pui32IdxNew;
	const char					*pV, *pV2;
	PVRTVECTOR4					vWeight, vIdx;
	PVRTVECTOR4					vWeight2, vIdx2;
	std::vector<int>			*pvDup;
	CGrowableArray				*pVtxBuf;
	unsigned int				ui32SrcIdx;

	memset(this, 0, sizeof(*this));

	if(nVertexBones <= 0 || nVertexBones > 4)
	{
		_RPT0(_CRT_WARN, "CPVRTBoneBatching() will only handle 1..4 bones per vertex.\n");
		return PVR_FAIL;
	}

	memset(&vWeight, 0, sizeof(vWeight));
	memset(&vWeight2, 0, sizeof(vWeight2));
	memset(&vIdx, 0, sizeof(vIdx));
	memset(&vIdx2, 0, sizeof(vIdx2));

	batch.SetSize(nBatchBoneMax);

	// Allocate some working space
	ppBatch		= (CBatch**)malloc(nTriNum * sizeof(*ppBatch));
	pui32IdxNew	= (unsigned int*)malloc(nTriNum * 3 * sizeof(*pui32IdxNew));
	pvDup		= new std::vector<int>[nVtxNum];
	pVtxBuf		= new CGrowableArray(nStride);

	// Check what batches are necessary
	for(i = 0; i < nTriNum; ++i)
	{
		// Build the batch
		if(!FillBatch(batch, &pui32Idx[i * 3], pVtx, nStride, nOffsetWeight, eTypeWeight, nOffsetIdx, eTypeIdx, nVertexBones))
		{
			free(pui32IdxNew);
			return PVR_FAIL;
		}

		// Update the batch list
		for(iBatch = lBatch.begin(); iBatch != lBatch.end(); ++iBatch)
		{
			// Do nothing if an existing batch is a superset of this new batch
			if(iBatch->Contains(batch))
			{
				break;
			}

			// If this new batch is a superset of an existing batch, replace the old with the new
			if(batch.Contains(*iBatch))
			{
				*iBatch = batch;
				break;
			}
		}

		// If no suitable batch exists, create a new one
		if(iBatch == lBatch.end())
		{
			lBatch.push_back(batch);
		}
	}

	//	Group batches into fewer batches. This simple greedy algorithm could be improved.
		int							nCurrent, nShortest;
		std::list<CBatch>::iterator	iShortest;

		for(iBatch = lBatch.begin(); iBatch != lBatch.end(); ++iBatch)
		{
			for(;;)
			{
				nShortest	= nBatchBoneMax;
				iBatch2		= iBatch;
				++iBatch2;
				for(; iBatch2 != lBatch.end(); ++iBatch2)
				{
					nCurrent = iBatch->TestMerge(*iBatch2);

					if(nCurrent >= 0 && nCurrent < nShortest)
					{
						nShortest	= nCurrent;
						iShortest	= iBatch2;
					}
				}

				if(nShortest < nBatchBoneMax)
				{
					iBatch->Merge(*iShortest);
					lBatch.erase(iShortest);
				}
				else
				{
					break;
				}
			}
		}

	// Place each triangle in a batch.
	for(i = 0; i < nTriNum; ++i)
	{
		if(!FillBatch(batch, &pui32Idx[i * 3], pVtx, nStride, nOffsetWeight, eTypeWeight, nOffsetIdx, eTypeIdx, nVertexBones))
		{
			free(pui32IdxNew);
			return PVR_FAIL;
		}

		for(iBatch = lBatch.begin(); iBatch != lBatch.end(); ++iBatch)
		{
			if(iBatch->Contains(batch))
			{
				ppBatch[i] = &*iBatch;
				break;
			}
		}

		_ASSERT(iBatch != lBatch.end());
	}

	// Now that we know how many batches there are, we can allocate the output arrays
	CPVRTBoneBatches::nBatchBoneMax = nBatchBoneMax;
	pnBatches		= (int*) calloc(lBatch.size() * nBatchBoneMax, sizeof(*pnBatches));
	pnBatchBoneCnt	= (int*) calloc(lBatch.size(), sizeof(*pnBatchBoneCnt));
	pnBatchOffset	= (int*) calloc(lBatch.size(), sizeof(*pnBatchOffset));

	// Create the new triangle index list, the new vertex list, and the batch information.
	nTriCnt = 0;
	nBatchCnt = 0;

	for(iBatch = lBatch.begin(); iBatch != lBatch.end(); ++iBatch)
	{
		// Write pnBatches, pnBatchBoneCnt and pnBatchOffset for this batch.
		iBatch->Write(&pnBatches[nBatchCnt * nBatchBoneMax], &pnBatchBoneCnt[nBatchCnt]);
		pnBatchOffset[nBatchCnt] = nTriCnt;
		++nBatchCnt;

		// Copy any triangle indices for this batch
		for(i = 0; i < nTriNum; ++i)
		{
			if(ppBatch[i] != &*iBatch)
				continue;

			for(j = 0; j < 3; ++j)
			{
				ui32SrcIdx = pui32Idx[3 * i + j];

				// Get desired bone indices for this vertex/tri
				pV = &pVtx[ui32SrcIdx * nStride];

				PVRTVertexRead(&vWeight, &pV[nOffsetWeight], eTypeWeight, nVertexBones);
				PVRTVertexRead(&vIdx, &pV[nOffsetIdx], eTypeIdx, nVertexBones);

				iBatch->GetVertexBoneIndices(&vIdx.x, &vWeight.x, nVertexBones);
				_ASSERT(vIdx.x == 0 || vIdx.x != vIdx.y);

				// Check the list of copies of this vertex for one with suitable bone indices
				for(k = 0; k < (int)pvDup[ui32SrcIdx].size(); ++k)
				{
					pV2 = pVtxBuf->at(pvDup[ui32SrcIdx][k]);

					PVRTVertexRead(&vWeight2, &pV2[nOffsetWeight], eTypeWeight, nVertexBones);
					PVRTVertexRead(&vIdx2, &pV2[nOffsetIdx], eTypeIdx, nVertexBones);

					if(BonesMatch(&vIdx2.x, &vIdx.x))
					{
						pui32IdxNew[3 * nTriCnt + j] = pvDup[ui32SrcIdx][k];
						break;
					}
				}

				if(k != (int)pvDup[ui32SrcIdx].size())
					continue;

				//	Did not find a suitable duplicate of the vertex, so create one
				pVtxBuf->Append(pV, 1);
				pvDup[ui32SrcIdx].push_back(pVtxBuf->size() - 1);

				PVRTVertexWrite(&pVtxBuf->last()[nOffsetIdx], eTypeIdx, nVertexBones, &vIdx);

				pui32IdxNew[3 * nTriCnt + j] = pVtxBuf->size() - 1;
			}
			++nTriCnt;
		}
	}
	_ASSERTE(nTriCnt == nTriNum);
	_ASSERTE(nBatchCnt == (int)lBatch.size());

	//	Copy indices to output
	memcpy(pui32Idx, pui32IdxNew, nTriNum * 3 * sizeof(*pui32IdxNew));

	//	Move vertices to output
	*pnVtxNumOut = pVtxBuf->Surrender(pVtxOut);

	//	Free working memory
	delete [] pvDup;
	delete pVtxBuf;
	FREE(ppBatch);
	FREE(pui32IdxNew);

	return PVR_SUCCESS;
}

/****************************************************************************
** Local functions
****************************************************************************/

/*!***********************************************************************
 @Function		FillBatch
 @Modified		batch 			The batch to fill
 @Input			pui32Idx		Input index array for triangle list
 @Input			pVtx			Input vertices
 @Input			nStride			Size of a vertex (in bytes)
 @Input			nOffsetWeight	Offset in bytes to the vertex bone-weights
 @Input			eTypeWeight		Data type of the vertex bone-weights
 @Input			nOffsetIdx		Offset in bytes to the vertex bone-indices
 @Input			eTypeIdx		Data type of the vertex bone-indices
 @Input			nVertexBones	Number of bones affecting each vertex
 @Returns		True if successful
 @Description	Creates a bone batch from a triangle.
*************************************************************************/
static bool FillBatch(
	CBatch					&batch,
	const unsigned int	* const pui32Idx,
	const char				* const pVtx,
	const int				nStride,
	const int				nOffsetWeight,
	EPVRTDataType			eTypeWeight,
	const int				nOffsetIdx,
	EPVRTDataType			eTypeIdx,
	const int				nVertexBones)
{
	PVRTVECTOR4	vWeight, vIdx;
	const char	*pV;
	int			i;
	bool		bOk;

	bOk = true;
	batch.Clear();
	for(i = 0; i < 3; ++i)
	{
		pV = &pVtx[pui32Idx[i] * nStride];

		memset(&vWeight, 0, sizeof(vWeight));
		PVRTVertexRead(&vWeight, &pV[nOffsetWeight], eTypeWeight, nVertexBones);
		PVRTVertexRead(&vIdx, &pV[nOffsetIdx], eTypeIdx, nVertexBones);

		if(nVertexBones >= 1 && vWeight.x != 0)	bOk &= batch.Add((int)vIdx.x);
		if(nVertexBones >= 2 && vWeight.y != 0)	bOk &= batch.Add((int)vIdx.y);
		if(nVertexBones >= 3 && vWeight.z != 0)	bOk &= batch.Add((int)vIdx.z);
		if(nVertexBones >= 4 && vWeight.w != 0)	bOk &= batch.Add((int)vIdx.w);
	}
	return bOk;
}

/*!***********************************************************************
 @Function		BonesMatch
 @Input			pfIdx0 A float 4 array
 @Input			pfIdx1 A float 4 array
 @Returns		True if the two float4 arraus are identical
 @Description	Checks if the two float4 arrays are identical.
*************************************************************************/
static bool BonesMatch(
	const float * const pfIdx0,
	const float * const pfIdx1)
{
	int i;

	for(i = 0; i < 4; ++i)
	{
		if(pfIdx0[i] != pfIdx1[i])
			return false;
	}

	return true;
}

/*****************************************************************************
 End of file (PVRTBoneBatch.cpp)
*****************************************************************************/

