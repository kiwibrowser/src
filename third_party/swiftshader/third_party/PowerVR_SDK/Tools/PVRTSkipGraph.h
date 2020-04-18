/*!****************************************************************************

 @file         PVRTSkipGraph.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        A "tree-like" structure for storing data which, unlike a tree, can
               reference any other node.

******************************************************************************/
#ifndef __PVRTSKIPGRAPH_H__
#define __PVRTSKIPGRAPH_H__


#include "PVRTArray.h"
#include "PVRTHash.h"

/*!***************************************************************************
 @class				CPVRTSkipGraphNode
 @brief      		Stores a pointer to the node's data and also uses a dynamic
					array to store pointer to nodes this node depends on and
					another to store pointers to nodes that are dependant on this node
*****************************************************************************/
template<class T>
class CPVRTSkipGraphNode
{
private:
	T								m_pData;
	CPVRTArray<CPVRTSkipGraphNode*> m_apDependencies;	// What I depend on
	CPVRTArray<CPVRTSkipGraphNode*> m_apDependents;		// What depends on me
	
public:
	/*!***************************************************************************
	@brief      	Constructor
	*****************************************************************************/
	CPVRTSkipGraphNode()
	{}

	/*!***************************************************************************
	@brief      	Overloaded constructor
	@param[in]		data    Pointer to a node
	*****************************************************************************/
	CPVRTSkipGraphNode(const T& data) : m_pData(data)
	{}

	/*!***************************************************************************
	@brief      	Destructor
	*****************************************************************************/
	~CPVRTSkipGraphNode()
	{} 

	/*!***************************************************************************
	@fn       		GetNumDependencies
	@return			unsigned int	
	@brief      	Returns the number of dependencies referenced by this node.
	*****************************************************************************/
	unsigned int GetNumDependencies() const					
	{ 
		return (unsigned int)m_apDependencies.GetSize(); 
	}

	/*!***************************************************************************
	@fn       		GetDependency
	@param[in]			ui32Id
	@return			CPVRTSkipGraphNode &	
	@brief      	Returns given dependency.
	*****************************************************************************/
	CPVRTSkipGraphNode& GetDependency(const unsigned int ui32Id) const
	{ 
		_ASSERT(ui32Id >= 0 && ui32Id < (unsigned int)m_apDependencies.GetSize());
		return *m_apDependencies[ui32Id];
	}


	/*!***************************************************************************
	@fn       		AddDependency
	@param[out]			pDependentNode
	@return			bool	
	@brief      	Adds a dependency to this node.
	*****************************************************************************/
	bool AddDependency(CPVRTSkipGraphNode* pDependentNode)
	{
		unsigned int ui(0);

		if(pDependentNode == this)
			return false;

		if(!pDependentNode)
			return false;

		/*
			Check the dependency doesn't already exist
		*/
		for(ui = 0; ui < (unsigned int)m_apDependencies.GetSize(); ++ui)
		{
			if(m_apDependencies[ui] == pDependentNode)
			{
				return true;
			}
		}

		/*
			Add the dependency and also set this node as a dependent of
			the referenced node
		*/
		m_apDependencies.Append(pDependentNode);
		pDependentNode->AddDependent(this);
		
		return true;
	}

	/*!***************************************************************************
	@fn       		GetData
	@return			T &	
	@brief      	Returns the data associated with this node.
	*****************************************************************************/
	T& GetData()				
	{
		return m_pData;
	}

private:
	/*!***************************************************************************
	@fn       		AddDependent
	@param[out]			pDependancyNode
	@return			bool	
	@brief      	Adds a dependent to this node.
	*****************************************************************************/
	bool AddDependent(CPVRTSkipGraphNode* pDependencyNode)
	{
		unsigned int ui(0);

		if(!pDependencyNode)
			return false;

		/*
			Check the dependency doesn't already exist
		*/
		for(ui = 0; ui < (unsigned int)m_apDependents.GetSize(); ++ui)
		{
			if(m_apDependencies[ui] == pDependencyNode)
			{
				return true;
			}
		}

		/*
			Add the dependancy
		*/
		m_apDependents.Append(pDependencyNode);
		return true;
	}
};

/*!***************************************************************************
 @class				CPVRTSkipGraphRoot
 @brief      		This class is the entry point for creating and accessing
					the elements of a skip graph. It uses a hash table to store
					the nodes of the structure and a hash value that allows
					fast searching of the skip graph
*****************************************************************************/
template<class T>
class CPVRTSkipGraphRoot
{
//-------------------------------------------------------------------------//
private:

	/*!***************************************************************************
	 @struct		SPVRTHashElement
	 @brief      	A struct to store data and a hash value generated from the
					data. The hash value allows faster searching of the skip graph.
	*****************************************************************************/
	struct SPVRTHashElement
	{		
	public:
		/*!***************************************************************************
		@fn       		SPVRTHashElement
		@param[in]			hash
		@param[in]			data
		@brief      	Overloaded constructor.
		*****************************************************************************/
		SPVRTHashElement(const CPVRTHash& hash, const T& data)
		:
		m_hash(hash),
		m_skipGraphNode(data)
		{}

		/*!***************************************************************************
		@fn       		SPVRTHashElement
		@brief      	Constructor
		*****************************************************************************/
		SPVRTHashElement()
		{}

		/*!***************************************************************************
		@fn       		~SPVRTHashElement
		@brief      	Destructor
		*****************************************************************************/
		~SPVRTHashElement()
		{}
		
		/*!***************************************************************************
		@fn       		GetHash
		@return			unsigned int	
		@brief      	Returns the element's hash value.
		*****************************************************************************/
		const CPVRTHash& GetHash() const				
		{ 
			return m_hash;
		}

		/*!***************************************************************************
		@fn       		GetNode
		@return			CPVRTSkipGraphNode<T>&	
		@brief      	Return the node associated with this element.
		*****************************************************************************/
		CPVRTSkipGraphNode<T>& GetNode()			
		{
			return m_skipGraphNode; 
		}

		/*!***************************************************************************
		@fn       		GetNode
		@return			CPVRTSkipGraphNode<T>&	
		@brief      	Return the node associated with this element.
		*****************************************************************************/
		const CPVRTSkipGraphNode<T>& GetNode() const			
		{
			return m_skipGraphNode; 
		}

	private:
		CPVRTHash				m_hash;
		CPVRTSkipGraphNode<T>	m_skipGraphNode;
	};

	
	CPVRTArray<SPVRTHashElement>		m_aHashTable;

//-------------------------------------------------------------------------//
public:

	/*!***************************************************************************
	 @fn       			AddNode
	 @param[in]			data							The data of the node to be added
	 @return			CPVRTSkipGraphNode<T>* const	A handle to the added node
	 @brief      		Searches through the hash table to see if the added node already
						exists. If it doesn't, it creates a node.
						The function returns true if the node was found or was created
						successfully.
	*****************************************************************************/
	bool AddNode(const T& data)
	{
		CPVRTHash NewNodeHash((void*)&data, sizeof(T), 1);
		int iArrayElement(-1);
		/*
			First, search the hash table to see
			if the node already exists
		*/
		CPVRTSkipGraphNode<T>* skipGraphNode(FindNode(NewNodeHash));

		if(skipGraphNode == NULL)
		{
			/*
				The node wasn't found, so a new node needs to be
				created
			*/
			iArrayElement = m_aHashTable.Append(SPVRTHashElement(NewNodeHash, data));

			/*
				Now point to the new instance
			*/
			skipGraphNode = &m_aHashTable[iArrayElement].GetNode();
		}

		return skipGraphNode ? true : false;
	}


	/*!***************************************************************************
	@brief      	Adds a node dependency.
	@param[in]		nodeData
	@param[in]		dependantData
	@return			bool	
	*****************************************************************************/
	bool AddNodeDependency(const T& nodeData, const T& dependantData)
	{
		CPVRTSkipGraphNode<T>* pNode(NULL);
		CPVRTSkipGraphNode<T>* pDependantNode(NULL);

		pNode = FindNode(nodeData);
		if(!pNode)
		{
			return false;
		}

		pDependantNode = FindNode(dependantData);
		if(!pDependantNode)
		{
			return false;
		}

		/*
			Nodes are not allowed to self reference
		*/
		if(pNode == pDependantNode)
		{
			return false;
		}
		pNode->AddDependency(pDependantNode);

		return true;
	}

	/*!***************************************************************************
	 @fn       			GetNumNodes
	 @return			unsigned int	The total number of nodes
	 @brief      		Returns the total number of nodes in the skip graph.
	*****************************************************************************/
	unsigned int GetNumNodes() const
	{
		return (unsigned int)m_aHashTable.GetSize();
	}

	/*!***************************************************************************
	 @brief      		Returns a sorted list of dependencies for the specified
						node. The list is ordered with the leaf nodes at the front,
						followed by nodes that depend on them and so forth until
						the root node is reached and added at the end of the list
	 @param[in]			aOutputArray	The dynamic array to store
										the sorted results in
	 @param[in]			ui32NodeID		The ID of the root node for
										the dependency search
	*****************************************************************************/
	void RetreiveSortedDependencyList(CPVRTArray<T> &aOutputArray, 
										const unsigned int ui32NodeID)
	{
		_ASSERT(ui32NodeID >= 0 && ui32NodeID < (unsigned int)m_aHashTable.GetSize());
		RecursiveSortedListAdd(aOutputArray, m_aHashTable[ui32NodeID].GetNode());
	}

	/*!***************************************************************************
	 @brief      		Overloads operator[] to returns a handle to the node data
						for the specified ID
	 @return			T&		Handle to the node data
	*****************************************************************************/
	T& operator[](const unsigned int ui32NodeID)
	{
		return *(GetNodeData(ui32NodeID));
	}
	
	/*!***************************************************************************
	 @brief      		Overloads operator[] to returns a const handle to the node 
						data for the specified ID
	 @return			T&		Handle to the node data
	*****************************************************************************/
	const T& operator[](const unsigned int ui32NodeID) const
	{
		return *(GetNodeData(ui32NodeID));
	}

//-------------------------------------------------------------------------//
private:
	/*!***************************************************************************
	 @brief      		Recursively adds node dependancies to aOutputArray.
						By doing so, aOutputArray will be ordered from leaf nodes to
						the root node that started the recursive chain
	 @param[in]			aOutputArray	The dynamic array to store
										the sorted results in
	 @param[in]			currentNode		The current node to process
	*****************************************************************************/
	void RecursiveSortedListAdd(CPVRTArray<T> &aOutputArray,
								CPVRTSkipGraphNode<T> &currentNode)
	{
		unsigned int ui(0);

		/*
			Recursively add dependancies first
		*/
		for(ui = 0; ui < currentNode.GetNumDependencies(); ++ui)
		{
			RecursiveSortedListAdd(aOutputArray, currentNode.GetDependency(ui));
		}

		/*
			Then add this node to the array
		*/
		aOutputArray.Append(currentNode.GetData());
	}

	/*!***************************************************************************
	 @fn       			GetNodeData
	 @param[in,out] 	ui32NodeID		The node's ID
	 @return			T*				A handle to node's data
	 @brief      		Retrieve a handle to the specified node's data
	*****************************************************************************/
	T* GetNodeData(unsigned int ui32NodeID)
	{
		_ASSERT(ui32NodeID >= 0 && ui32NodeID < (unsigned int)m_aHashTable.GetSize());
		return &m_aHashTable[ui32NodeID].GetNode().GetData();
	}
	
	/*!***************************************************************************
	 @brief      		Use the input hash to search the hash table and see if the
						node already exists. If it does, the function returns a pointer
						to the node. If it doesn't, it returns NULL
	 @param[in,out] 	ui32Hash						The hash value to search with
	 @return			CPVRTSkipGraphNode<T>* const	A handle to the found node
	*****************************************************************************/
	CPVRTSkipGraphNode<T>* FindNode(const CPVRTHash& Hash)
	{
		int i(0);
		int i32HashTableSize(m_aHashTable.GetSize());

		/*
			A NULL hash means the node has not initialised
			correctly
		*/
		if(Hash == 0)
			return NULL;

		/*
			NOTE:
			In the future, the hash table could be sorted from
			lowest hash value to highest so that binary search could
			be used to find a given node. It should be possible to
			use a bool (or some other mechanism) to toggle this form of search
			(if the skip graph is small, it might be faster to just use a brute
			force for loop to search through)
		*/
		for(i = 0; i < i32HashTableSize; i++)
		{
			if(m_aHashTable[i].GetHash() == Hash)
			{
				return &m_aHashTable[i].GetNode();
			}
		}

		/*
			The element wasn't found, so return null
		*/
		return NULL;
	}

	/*!***************************************************************************
	 @brief      		Use the input data to generate a hash and then
						search the hash table and see if the node already exists.
						If it does, the function returns a pointer
						to the node. If it doesn't, it returns NULL
	 @param[in,out] 	data							Data to use as a source for the hash
	 @return			CPVRTSkipGraphNode<T>* const	A handle to the found node
	*****************************************************************************/
	CPVRTSkipGraphNode<T>* FindNode(const T& data)
	{
		CPVRTHash inputHash((void*)&data, sizeof(T), 1);	// Generate hash for searching
		return FindNode(inputHash);
	}
};

#endif //__PVRTSKIPGRAPH_H__

/*****************************************************************************
 End of file (PVRTSkipGraph.h)
*****************************************************************************/

