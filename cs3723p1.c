#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cs3723p1.h"
#define RC_NOT_AVAIL -1

void deleteFreeNodes(StorageManager *pMgr, FreeNode *pNode);
void errorFreeNodeCheck(StorageManager *pMgr, FreeNode *pNode);


/******************** smInit **************************************
 void smInit(StorageManager *pMgr)
 Purpose:
To initialize the heap and the StorageManager structure. 
It also clears out the heap to make sure there is the amount
of memory needed. Sets up the entire heap to one free node.

Parameters:
    I   StorageManager *pMgr    Pointer to the SorageManager structure. It
				is used to initialize the heap and zero out 
				the heap; 
            
Returns:
    n/a 
Notes:
    - This function makes sure to make the entire heap as one free node
**************************************************************************/
void smInit(StorageManager *pMgr){

	if(pMgr->pBeginStorage == NULL){
		printf("pBeginStorage is pointing to nothing\n");
		//exit(1);
	}
	
	
	pMgr->pFreeList = (FreeNode *) pMgr->pBeginStorage;
	memset(pMgr->pFreeList, 0, pMgr->shHeapSize);// zero out the heap 
	pMgr->pFreeList->shItemSize = pMgr->shHeapSize;
	pMgr->pFreeList->cAF = 'F';
	pMgr->pFreeList->pNext = NULL;
	pMgr->pFreeList->pPrev = NULL;

	// Get address of end size
	char *pEnd = ((char *) pMgr->pFreeList + pMgr->pFreeList->shItemSize);
	// Set the size at that location
	*((short *) (pEnd - sizeof(short))) = pMgr->shHeapSize;
}


/******************** smAlloc **************************************
 void * smAlloc(StorageManager *pMgr, short shdataSize, char sbData[], SMResult *psmResult)
 Purpose:
 The function allocs memory for the node that wants to be alloced from the available
 memory in the free list. Removes the alloced node from the free list and makes sure to
 reconnect the nodes from the free list correctly.

Parameters:
    I   StorageManager *pMgr    A pointer to a storage manager structure. 
				it is used to point to the beginning of 
				the free list.
    
    II  short shDataSize	The size of the nodes that is going to be
				alloced 

    III char sbData[]		The data is going to be alloced

    IV  SMResult *psmResult	Used for error check and storing error message

Returns:
    n/a 
Notes:
  -Function makes sure to isolate the free node that is going to be allocated. 
**************************************************************************/
void * smAlloc(StorageManager *pMgr, short shdataSize, char sbData[], SMResult *psmResult){

	FreeNode *pNode;
	short allocSize = shdataSize + OVERHEAD_SIZE; // the amount we have to alloc
	if (allocSize < pMgr->shMinimumSize ){
		allocSize = pMgr->shMinimumSize;
	}

	for(pNode = pMgr->pFreeList; pNode!=NULL; pNode = pNode->pNext){
		if(pNode->shItemSize >= allocSize){
			// isolate the pNode that we are going to alloc memory from
			deleteFreeNodes(pMgr, pNode);
			// the remaining of the free node will be made into another free node and put in the front		
			if((pNode->shItemSize-allocSize) >= pMgr->shMinimumSize){
				//deleteFreeNodes(pMgr, pNode); 
				FreeNode *pNew = (FreeNode *) ((char *)pNode + allocSize);
				pNew->shItemSize = pNode->shItemSize-allocSize;
				pNew->cAF = 'F';
				pNew->pNext = pMgr->pFreeList; // points to the initila free node
				pNew->pPrev = NULL;
				
				errorFreeNodeCheck(pMgr, pNew);
				//if (pMgr->pFreeList != NULL)
				//	pMgr->pFreeList->pPrev = pNew;
				//if (pNew->pNext != NULL && pNew->cAF == 'F')
				//	pNew->pNext->pPrev = pNew;

				pMgr->pFreeList = pNew;// now the free list points to the actual first free node;

				char *pEnd = ((char *) pNew + pNew->shItemSize);
        			// Set the size at that location
        			*((short *) (pEnd - sizeof(short))) = pNew->shItemSize;
			}else{
				allocSize = pNode->shItemSize;
			}
				

			AllocNode *pAlloc = (AllocNode *)pNode;// allocs memory for the pAlloc node
			memset(pAlloc, 0, allocSize);
			pAlloc->cAF = 'A';
			pAlloc->shItemSize = allocSize;
			
			char *pAllocEnd = ((char *) pAlloc + pAlloc->shItemSize);
		
        // Set the size at that location
        		*((short *) (pAllocEnd - sizeof(short))) = pAlloc->shItemSize;
			psmResult->rc = 0;			
			//pAlloc->sbAllocDumb = *sbData;			
			memcpy(pAlloc->sbAllocDumb, sbData, shdataSize);
			//return some value 
			return (AllocNode *) ((char*)pAlloc + OVERHEAD_SIZE); 
		}
	}

	psmResult->rc = RC_NOT_AVAIL;
        strcpy(psmResult->szErrorMessage, "There was no alloc");
        return NULL;

}



/******************** smFree **************************************
short smFree(StorageManager *pMgr, char *pUserData, SMResult *psmResult) 
Purpose:
Function frees the allocated node and once the node is freed
it makes sure to connect the free node back to the free list;

Parameters:
    I   StorageManager *pMgr    A pointer to a storage manager structure. 
                                it is used to point to the beginning of 
                                the free list.

    II char *pUserData          its the node that is going to be freed

    III  SMResult *psmResult     Used for error check and storing error message

Returns:
    returns the size of the node that has been freed
Notes:
    - Function makes sure to make the free node the first free node of the list
**************************************************************************/
short smFree(StorageManager *pMgr, char *pUserData, SMResult *psmResult){

	FreeNode *pCurrent = (FreeNode *) ((char *)pUserData - OVERHEAD_SIZE);
	if(pCurrent->cAF == 'A')
	{
		FreeNode *pAdj = (FreeNode *) ((char *)pCurrent + pCurrent->shItemSize); // size of adjacent node
		short *pPrevSize = (short *) (((char *)pCurrent) - sizeof(pCurrent->shItemSize));// size of previous node
		FreeNode *pPrevAdj = (FreeNode *) ((char *) pCurrent - *pPrevSize); //address of preceding node
		if(pAdj->cAF == 'F'){// merges adjacent node
			pCurrent->shItemSize += pAdj->shItemSize;
			deleteFreeNodes(pMgr, pAdj);
			
		}
		if(pPrevAdj->cAF == 'F'){// merges preceding node
			deleteFreeNodes(pMgr, pPrevAdj);		
			pPrevAdj->shItemSize += pCurrent->shItemSize;
			pCurrent = pPrevAdj;
		}
		short totalSize = pCurrent->shItemSize;// saves the total size
		memset(pCurrent, 0, pCurrent->shItemSize);// zeros out the freed node
		pCurrent->shItemSize = totalSize;
		
		char *pEnd = ((char *) pCurrent + pCurrent->shItemSize);
        // Set the size at that location
        	*((short *) (pEnd - sizeof(short))) = pCurrent->shItemSize;
		pCurrent->cAF = 'F';
		pCurrent->pPrev = NULL;
		pCurrent->pNext = pMgr->pFreeList; 
		
		
		errorFreeNodeCheck(pMgr, pCurrent);

		pMgr->pFreeList = pCurrent;
		return pCurrent->shItemSize;
		
	}	
}


/******************** DeleteFreeNodes **************************************
void deleteFreeNodes( StorageManager *pMgr, FreeNode *pNode)
Purpose:
Function makes sure isolate the free node from the free list
Parameters:
    I   StorageManager *pMgr    A pointer to a storage manager structure. 
                                it is used to point to the beginning of 
                                the free list.

    II FreeNode *pNode		Node that is going to be disconnected  from the free list.
Returns:
 n/a
Notes:
 n/a
**************************************************************************/
void deleteFreeNodes( StorageManager *pMgr, FreeNode *pNode){

	  if(pNode == pMgr->pFreeList){
                 pMgr->pFreeList = pNode->pNext;
          }
          if(pNode->pNext != NULL){
                  pNode->pNext->pPrev = pNode->pPrev;
          }
          if(pNode->pPrev != NULL){
                   pNode->pPrev->pNext = pNode->pNext;
           }

}


/******************** errorFreeNodecheck **************************************
void deleteFreeNodes( StorageManager *pMgr, FreeNode *pNode)
Purpose:
Function makes sure to check if the first node prev is pointing to iself and not pointing to 
anything else. Also makes sure the adjacent free node is pointing to the new free node
Parameters:
    I   StorageManager *pMgr    A pointer to a storage manager structure. 
                                it is used to point to the beginning of 
                                the free list.

    II FreeNode *pNode          Free node to be checked if it is connected correctly
Returns:
 n/a
Notes:
 n/a
**************************************************************************/
void errorFreeNodeCheck(StorageManager *pMgr, FreeNode *pNode){
	if (pMgr->pFreeList != NULL)
        	pMgr->pFreeList->pPrev = pNode;
        if (pNode->pNext != NULL && pNode->cAF == 'F')
           	pNode->pNext->pPrev = pNode;
}





