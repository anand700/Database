#include"record_mgr.h"
#include"buffer_mgr.h"
#include"storage_mgr.h"
#include "btree_mgr.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Assumptions
DataType defaultDataType = DT_INT;
void * nodeStart = NULL;

/**
*This function is used to initiate manager.
*
*
* @author  Anand N
* @param   mgmtData
* @return   RC
* @since   2015-12-03
*/
RC initIndexManager(void* mgmtData) {
	return RC_OK;
}

/**
*This function is used to create a B+ tree.
*
*
* @author  Anand N
* @param   Index Id
* @param   Key Type
* @param   number of 
* @return   RC
* @since   2015-12-03
*/
RC createBtree(char* idxId, DataType keyType, int n) {
	BM_BufferPool *bbp = MAKE_POOL();
	BM_PageHandle *bp = MAKE_PAGE_HANDLE();
	int offset = sizeof(int);

	// create page file
	createPageFile(idxId);
	initBufferPool(bbp, idxId, 3, RS_FIFO, NULL);
	if (pinPage(bbp, bp, 0) != RC_OK)
	{
		free(bp);
		return RC_PIN_PAGE_FAILED;
	}


	memmove(bp->data + offset, &keyType, sizeof(int));
	offset = offset + sizeof(int);
	memmove(bp->data + offset, &n, sizeof(int));


	if (markDirty(bbp, bp) != RC_OK)
	{
		free(bp);
		return RC_MAKE_DIRTY_FAILED;
	}
	unpinPage(bbp, bp);
	//Force page to be written on disk
	forcePage(bbp, bp);
	shutdownBufferPool(bbp);


	free(bbp);
	return RC_OK;
}


/**
*This function is used to create a node.
*
*
* @author  Anand N
* @param   Tree
* @return   Tree
* @since   2015-12-03
*/
Tree* createANode(BTreeHandle* tree) {
	//Make page handle
	BM_PageHandle *bp = MAKE_PAGE_HANDLE();
	//Tree Management Data consists of the tree info structure
	TreeInfo *treeInfo = tree->mgmtData;
	if (pinPage(treeInfo->bufferMgrPtr, bp, 0) != RC_OK)
	{
		free(bp);
		return RC_PIN_PAGE_FAILED;
	}
	Tree *newNode = MAKE_TREE();
	newNode->parent = NULL;
	//Allocate memory for the pointers
	newNode->ptrs = malloc((treeInfo->numberOfValuesPerNode + 1) * sizeof(void *));
	//Set the initial pointer to NULL
	newNode->ptrs[0] = NULL;
	newNode->next = NULL;
	newNode->prev = NULL;
	newNode->records = malloc(treeInfo->numberOfValuesPerNode * sizeof(void *));
	newNode->keys = malloc(treeInfo->numberOfValuesPerNode * sizeof(int));
	// Initially set to true
	newNode->isLeafNode = true;
	// Initially set key size to zero
	newNode->Keysize = 0;
	if (markDirty(treeInfo->bufferMgrPtr, bp) != RC_OK)
	{
		free(bp);
		return RC_MAKE_DIRTY_FAILED;
	}
	unpinPage(treeInfo->bufferMgrPtr, bp);
	//Force page to be written on disk
	forcePage(treeInfo->bufferMgrPtr, bp);
	return newNode;
}

/**
*This function is used to open a B+ tree.
*
*
* @author  Anand N
* @param   Index Id
* @param   B+ Tree Handle
* @return   RC
* @since   2015-12-03
*/
RC openBtree(BTreeHandle** tree, char* idxId) {
	int offSet = sizeof(int);
	int numOfNodes = 0;
	int numOfInserts = 0;
	int numberOfValuesPerNode = 0;
	DataType keyType;
	BM_BufferPool *bbp = MAKE_POOL();
	BM_PageHandle *bp = MAKE_PAGE_HANDLE();
	TreeInfo *treeInfo = MAKE_TREE_INFO();
	(*tree) = (BTreeHandle *)malloc(sizeof(BTreeHandle));


	(*tree)->idxId = idxId;
	initBufferPool(bbp, idxId, 5, RS_FIFO, NULL);
	if (pinPage(bbp, bp, 0) != RC_OK)
	{
		free(bp);
		return RC_PIN_PAGE_FAILED;
	}

	memcpy(&keyType, bp->data + offSet, sizeof(int));
	offSet = offSet + sizeof(int);
	memcpy(&numberOfValuesPerNode, bp->data + offSet, sizeof(int));
	unpinPage(bbp, bp);

	//Assigning values to the inbuilt structure defined
	treeInfo->numberOfValuesPerNode = numberOfValuesPerNode;
	(*tree)->keyType = keyType;

	// Number of inserts in the entire tree
	treeInfo->numOfInserts = numOfInserts;
	// Number of nodes in the entire tree
	treeInfo->numOfNodes = numOfNodes;
	(*tree)->mgmtData = treeInfo;
	treeInfo->bufferMgrPtr = bbp;

	// Create a node
	treeInfo->mgmtData = createANode(*tree);
	nodeStart = treeInfo->mgmtData;
	free(bp);
	return RC_OK;
}

/**
*This function is used to split position.
*
*
* @author  Anand N
* @param   numberOfValuesPerNode
* @return  int
* @since   2015-12-03
*/
int splitLoc(int numberOfValuesPerNode) {
 
	if (numberOfValuesPerNode % 2 != 0) {

		//If it is a odd number
		return ((numberOfValuesPerNode+1)/2)+1;
	}
	else {

		//If it is a even number 
		return (numberOfValuesPerNode/2)+1;
	}
}

/**
*This function is used to insert into parent Node.
*
*
* @author  Pavan
* @param   B Tree Handle
* @param   Tree Info
* @param   old node
* @param   key
* @param   rid
* @return  RC
* @since   2015-12-03
*/
RC InsertAfterSplit(BTreeHandle* tree, TreeInfo *treeInfo, Tree *oldNode, Value* key, RID rid) {


	int idx = 0;
	int i = 0;
	int j = 0; 
	TreeInfo *tempTreeInfo;
	Tree *newNode;

	// tree mgmt date is the tree info
	tempTreeInfo = tree->mgmtData;
	if (oldNode->next != NULL) {
		Tree *tempTree;
		tempTree = oldNode->next;
		if (key->v.intV > oldNode->keys[oldNode->Keysize - 1] && tempTreeInfo->numberOfValuesPerNode > tempTree->Keysize) {
			newNode = tempTree;


			//Increment the pos until parent keysize is lesser than the pos
			while (idx < newNode->Keysize && newNode->keys[idx] < key->v.intV) {
				idx++;
			}

			// Assign i-1 positions keys and records for the new node.

			for (i = newNode->Keysize; i > idx; i--) {
				newNode->records[i] = newNode->records[i - 1];
				newNode->keys[i] = newNode->keys[i - 1];
			}
			newNode->records[idx] = rid;
			newNode->keys[idx] = key->v.intV;
			newNode->Keysize++;
			return RC_OK;
		}
		else {

			// If the oldnode-> is set as null then thhere are no more
			// tree leaves
			newNode = createANode(tree);
			tempTreeInfo->numOfNodes++;
			oldNode->next = newNode;

			// updating the new nodes
			newNode->next = tempTree;
			newNode->prev = oldNode;
			tempTree->prev = newNode;
		}
	}else {

		// create a node
		newNode = createANode(tree);
		tempTreeInfo->numOfNodes++;

		//Update the new and old nodes
		oldNode->next = newNode;
		newNode->prev = oldNode;
	}

	//Set sizes for the key pointer and rid pointers
	int *keyPtrs = malloc((tempTreeInfo->numOfInserts + 1) * sizeof(int));
	RID *ridPtrs = malloc((tempTreeInfo->numOfInserts + 1) * sizeof(void *));
	int loc;

	// set the value of the index which is used to calculate the  rid ptrs and key ptrs later
	while (oldNode->keys[idx] < key->v.intV && idx < oldNode->Keysize) {
		idx++;
	}
	do {

		//Do while loop to calculate the  rid ptrs and key ptrs
		if (j == idx) {
			j++;
		}
		ridPtrs[j] = oldNode->records[i];
		keyPtrs[j] = oldNode->keys[i];
		i++; j++;
	} while (oldNode->Keysize > i);
	ridPtrs[idx] = rid;
	keyPtrs[idx] = key->v.intV;


	//Split locations are claculated eaCH TIME
	loc = splitLoc(tempTreeInfo->numberOfValuesPerNode);
	oldNode->Keysize = 0;
	j = 0;
	for (i = 0; i < loc; i++) {
		oldNode->keys[i] = keyPtrs[i];
		oldNode->records[i] = ridPtrs[i];
		oldNode->Keysize++;
		j++;
	}

	//Update the new node recrds, keys and key sizes.
	for (i = 0; i < (tempTreeInfo->numberOfValuesPerNode + 1 - loc); i++) {
		newNode->keys[i] = keyPtrs[j];
		newNode->records[i] = ridPtrs[j];
		newNode->Keysize++;
		j++;
	}
	newNode->parent = oldNode->parent;

	// Insert to the parent
	insertToTheParentNode(tree, treeInfo, newNode, oldNode, newNode->keys[0]);
	return RC_OK;
}

/*
* Function to split the parent node and insert into B+-Tree
* Arguments: BTreeHandle *tree, TreeInfo *headNode, Tree *prevNode, int key
* Output : RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/

RC InsertToParentAfterSplit(BTreeHandle *tree, TreeInfo *headNode, Tree *prevNode, int keyelement) {
	Tree *curNode;
	curNode = createANode(tree);
	headNode->numOfNodes++;
	curNode->isLeafNode = false;
	prevNode->next = curNode;
	curNode->prev = prevNode;
	int nodeIndex = 0, i = 0, j = 0;
	int *arraykeysptr;
	int splitpt, curNodeNum = 0, curKey = 0;
	arraykeysptr = malloc((headNode->numberOfValuesPerNode + 1) * sizeof(int));

	// set the value of the index which is used to calculate the  rid ptrs and key ptrs later
	while (nodeIndex < prevNode->Keysize && prevNode->keys[nodeIndex] < keyelement) {
		nodeIndex++;
	}
	do {

		//Do while loop to calculate the  rid ptrs and key ptrs
		if (j == nodeIndex) {
			j++;
		}
		arraykeysptr[j] = prevNode->keys[i];
		i++;
		j++;
	} while (i < prevNode->Keysize);
	arraykeysptr[nodeIndex] = keyelement;

	// Split the location
	splitpt = splitLoc(headNode->numberOfValuesPerNode);
	prevNode->Keysize = 0;
	j = 0;
	i = 0;
	while (i < (splitpt - 1)) {
		prevNode->keys[i] = arraykeysptr[i];
		prevNode->Keysize++;
		j++;
		i++;
	}
	j++;
	curKey = arraykeysptr[splitpt - 1];
	curNodeNum = headNode->numberOfValuesPerNode + 1 - splitpt;
	i = 0;

	//Update the new node recrds, keys and key sizes.
	while (i < curNodeNum) {
		curNode->keys[i] = arraykeysptr[j];
		curNode->Keysize++;
		j++;
		i++;
	}
	headNode->mgmtData = curNode;

	// Insert to the parent
	insertToTheParentNode(tree, headNode, curNode, prevNode, curKey);
	return RC_OK;
}
/*
* Function to search the node to insert.
* Arguments: Value* key
* Output : prev Tree pointer
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/
Tree* searchInsertNode(Value* key) {

	// Search the node to be inserted 
	Tree *treePtr, *prevTreePtr;
	treePtr = nodeStart;
	prevTreePtr = treePtr;

	//When tree ptr is not equal  to null valuse
	while (treePtr != NULL) {
		if (treePtr->isLeafNode = false) {

			//If the tree leaf is flase
			return 	prevTreePtr;
		}
		if (treePtr->isLeafNode = true) {

			//If the tree leaf is true
			if (treePtr->keys[0] > key->v.intV) {
				return prevTreePtr;
			}
		}

		prevTreePtr = treePtr;
		treePtr = prevTreePtr->next;
	}
	return prevTreePtr;
}

/*
* Function to insert the key into B+-Tree
* Arguments: BTreeHandle* tree, Value* key, RID rid
* Output : Insert key and RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/

RC insertKey(BTreeHandle* tree, Value* key, RID rid) {
	Tree *nodeptr, *tempptr;
	TreeInfo *headNode;
	int i;
	headNode = tree->mgmtData;
	nodeptr = headNode->mgmtData;
	if (headNode->numOfNodes == 0) {
		headNode->numOfNodes++;
		nodeptr->isLeafNode = true;
		nodeptr->keys[0] = key->v.intV;
		nodeptr->records[0] = rid;
		nodeptr->parent = NULL;
		nodeptr->Keysize++;
		headNode->numOfInserts++;
		trackUpdateInsert(tree, headNode);
		return RC_OK;
	}
	nodeptr = searchInsertNode(key);
	tempptr = nodeStart;
	while (tempptr != NULL) {
		for (i = 0; i < tempptr->Keysize; i++) {
			if (tempptr->keys[i] == key->v.intV) {
				trackUpdateInsert(tree, headNode);
				return RC_OK;
			}
		}
		tempptr = tempptr->next;
	}
	if (nodeptr->Keysize < headNode->numberOfValuesPerNode) {
		int index = 0, i = 0;

		while (index < nodeptr->Keysize && nodeptr->keys[index] < key->v.intV) {
			index++;
		}
		for (i = nodeptr->Keysize; i > index; i--) {
			nodeptr->keys[i] = nodeptr->keys[i - 1];
			nodeptr->records[i] = nodeptr->records[i - 1];
		}
		nodeptr->keys[index] = key->v.intV;
		nodeptr->records[index] = rid;
		nodeptr->Keysize++;
		headNode->numOfInserts++;
		trackUpdateInsert(tree, headNode);
		return RC_OK;
	}
	if (nodeptr->Keysize == headNode->numberOfValuesPerNode) {
		InsertAfterSplit(tree, headNode, nodeptr, key, rid);
		headNode->numOfInserts++;
		trackUpdateInsert(tree, headNode);
		return RC_OK;
	}
}

/*
* Function to track and update the insert's into B+-Tree
* Arguments: BTreeHandle *bptr, TreeInfo* nodeInfo
* Output : RC_OK on sucess, Error_Code on failure
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/

RC trackUpdateInsert(BTreeHandle *bptr, TreeInfo* nodeInfo)
{
	BM_PageHandle *phandle = MAKE_PAGE_HANDLE();
	int offSet = sizeof(int);

	//pin page zero
	if (pinPage(nodeInfo->bufferMgrPtr, phandle, 0) != RC_OK)
	{
		free(phandle);
		return RC_PIN_PAGE_FAILED;
	}
	memmove(phandle->data + offSet, &bptr->keyType, sizeof(int));
	offSet += sizeof(int);
	memmove(phandle->data + offSet, &nodeInfo->numberOfValuesPerNode, sizeof(int));

	//make dirty
	if (markDirty(nodeInfo->bufferMgrPtr, phandle) != RC_OK)
	{
		free(phandle);
		return RC_MAKE_DIRTY_FAILED;
	}
	unpinPage(nodeInfo->bufferMgrPtr, phandle);
	forcePage(nodeInfo->bufferMgrPtr, phandle);
	return RC_OK;
}

/*
* Function to find a given key
* Arguments: BTreeHandle *tree, Value *key, RID *result
* Output : RC_OK on sucess, Error_Code on failure
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/

RC findKey(BTreeHandle *tree, Value *key, RID *result) {
	Tree *headNode, *tempptr;
	TreeInfo *btrackptr;

	//table info is accessed through the mgmtdata
	btrackptr = tree->mgmtData;
	headNode = btrackptr->mgmtData;
	int i = 0;
	headNode = btrackptr->mgmtData;
	tempptr = headNode;

	// if the is leaf node  is equal to false

	while (tempptr->isLeafNode == false) {
		while (i < headNode->Keysize) {
			if (key->v.intV < headNode->keys[i]) {
				tempptr = headNode->ptrs[i];
				break;
			}
			else {
				tempptr = headNode->ptrs[i + 1];
			}
			i++;
		}
	}
	i = 0;
	while (i < tempptr->Keysize) {


		//records are put into the result pointer
		if (tempptr->keys[i] == key->v.intV) {
			*result = tempptr->records[i];
			return RC_OK;
		}
		i++;
	}
	return RC_IM_KEY_NOT_FOUND;
}

/*
* Function to Initialize thr tree scan
* Arguments: BTreeHandle *tree, BT_ScanHandle **handle
* Output : RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/

RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {

	//Track key data structure is used to track the keys 
	TrackKey *keyelement = NULL;
	TreeInfo *bttrackptr;
	Tree *nodeptr;

	//initiate the scan handle


	(*handle) = (BT_ScanHandle *)malloc(sizeof(BT_ScanHandle));
	bttrackptr = tree->mgmtData;
	nodeptr = bttrackptr->mgmtData;
	while (nodeptr->ptrs[0] != NULL) {
		nodeptr = nodeptr->ptrs[0];
		if (nodeptr == NULL || NULL == nodeptr->ptrs[0])
			break;
	}
	keyelement = (TrackKey *)malloc(sizeof(TrackKey));

	//set all the values associated to the key element
	
	keyelement->node = nodeptr;
	keyelement->pos = 0;
	(*handle)->tree = tree;
	(*handle)->mgmtData = (void *)keyelement;
	return RC_OK;

}


/**
*This function is used to insert into parent Node.
*
*
* @author  Anand N
* @param   B Tree Handle
* @param   Tree Info
* @param   new node
* @param   old node
* @param   key
* @return  int
* @since   2015-12-03
*/
RC insertToTheParentNode(BTreeHandle* tree, TreeInfo *treeInfo, Tree *newNode, Tree *oldNode, int key) {

	//Make a parent node
	Tree *parentNode = MAKE_TREE();
	if (oldNode->parent != NULL) {

		// Have a previous and a current node
		Tree *tempNode, *tempPrevNode;
		tempNode = oldNode->parent;
		tempPrevNode = tempNode;

		//Keep incrementing temp node to next position until it reaches null
		while (tempNode != NULL) {
			parentNode = tempPrevNode;
			tempPrevNode = tempNode;
			tempNode = tempNode->next;
		}

		//           Assign the previous node to the parent.
		parentNode = tempPrevNode;

		//New node to the parents pointers
		parentNode->ptrs[parentNode->Keysize + 1] = newNode;
	}
	else {
		//Create a node
		parentNode = createANode(tree);
		treeInfo->numOfNodes++;

		//Assigning oldnode to the pointers
		parentNode->ptrs[0] = oldNode;

		//Assigning newnode to the pointers
		parentNode->ptrs[1] = newNode;
		treeInfo->mgmtData = parentNode;
	}

	// Initially set the isleafNode parameter to false for the 
	// parent node and also assign the parent node to both new 
	// and old node.
	parentNode->isLeafNode = false;
	newNode->parent = parentNode;
	oldNode->parent = parentNode;

	//Codition to check if the key size is lesser than the 
	//numb of values per node
	if (treeInfo->numberOfValuesPerNode > parentNode->Keysize) {
		parentNode->ptrs[parentNode->Keysize + 1] = newNode;
		int pos = 0;
		int i = 0;

		//Increment the pos until parent keysize is lesser than the pos
		while (parentNode->Keysize > pos && key > parentNode->keys[pos]) {
			pos++;
		}

		// Set the parent records and keys
		for (i = pos; i <parentNode->Keysize; i++) {
			parentNode->records[i + 1] = parentNode->records[i];
			parentNode->keys[i + 1] = parentNode->keys[i];
		}
		parentNode->keys[pos] = key;

		// By default set the value as zero.
		parentNode->records[pos].page = 0;
		parentNode->records[pos].slot = 0;
		parentNode->Keysize++;
		return RC_OK;
	}
	if (treeInfo->numberOfValuesPerNode == parentNode->Keysize) {

		//Split and insert to the parent 
		InsertToParentAfterSplit(tree, treeInfo, parentNode, key);
		return RC_OK;
	}
}


/*
* Function to find the next entry in the B+-Tree
* Arguments: BT_ScanHandle *handle, RID *result
* Output : RC_OK on sucess, RC_IM_NO_MORE_ENTRIES on failure
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
RC nextEntry(BT_ScanHandle *handle, RID *result) {
	int recLength;
	Tree *nodeptr;
	TrackKey *keydata = NULL;


	keydata = handle->mgmtData;
	nodeptr = keydata->node;
	recLength = keydata->pos;

	//If node pointer is NULL return no more entries
	if (nodeptr == NULL) {
		return RC_IM_NO_MORE_ENTRIES;
	}

	//check if keysize is less than reclength and set the node pointer record length to result
	if (recLength < nodeptr->Keysize) {
		*result = nodeptr->records[recLength];
		recLength++;
	}
	if (nodeptr->Keysize == recLength) {
		nodeptr = nodeptr->next;
		recLength = 0;
	}

	//update the node keydata with the node pointer and assign record lenth to keydata position
	keydata->node = nodeptr;
	keydata->pos = recLength;
	return RC_OK;
}



/*
* Function to get the total number of keys in B+-Tree
* Arguments: BTreeHandle *tree, int *result
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
RC getNumEntries(BTreeHandle *tree, int *result) {
	TreeInfo *headNode;

	//Intialize the tree handle pointer to root node
	headNode = tree->mgmtData;

	//assign the track of number of inserts to result
	*result = headNode->numOfInserts;
	return RC_OK;
}

/*
* Function to check underflow.
* Arguments: treeInfo, tree
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
int checkUnderflow(TreeInfo* treeInfo, Tree * tree) {

	//Check for the number of values per node and decide upon whether a node has underflow situation or not
	if (treeInfo->numberOfValuesPerNode % 2 != 0) {
		if (((treeInfo->numberOfValuesPerNode + 1) / 2) > tree->Keysize) {
			return 1;
		}
	}
	else {
		//Return one for the underflow
		if (((treeInfo->numberOfValuesPerNode) / 2) > tree->Keysize) {
			return 1;
		}
	}

	//Else return zero
	return 0;
}



/*
* Function to redistribute the keys.
* Arguments: Tree Information, left node, right node
* Output : RC_OK
* AUTHOR: Pavan
* DATE: 29, Nov 2015
*/
RC redistribute(TreeInfo *stat, Tree *left_node, Tree *right_node) {

	RID *rid = NULL;

	//Intialize the page and slot number to -1
	rid->page = -1;
	rid->slot = -1;
	int i = 0;
	int idx = 0;
	int key;
	//Check and access the left sibling for redistribution
	key = left_node->keys[idx];
	idx = left_node->Keysize - 1;
	*rid = left_node->records[idx];
	left_node->keys[idx] = NULL;
	left_node->records[idx].page = 0;
	left_node->records[idx].slot = 0;
	left_node->Keysize--;

	//Bring the pointer towards the right sibling
	while (right_node->keys[idx] < key && idx < right_node->Keysize) {
		idx++;
	}
	i = right_node->Keysize;

	//If there are are no leftnode elements then access the right siblings for redistribution
	while (i > idx) {
		right_node->records[i] = right_node->records[i - 1];
		right_node->keys[i] = right_node->keys[i - 1];
		i--;
	}
	right_node->records[idx] = *rid;
	right_node->keys[idx] = key;
	right_node->Keysize++;
	right_node = right_node->parent;
	while (right_node != NULL) {
		if (right_node->isLeafNode == false) {
			right_node->keys[0] = key;
		}

		//After redistribution the the parent node using the right sibling split element
		right_node = right_node->parent;
	}
	return RC_OK;
}

/*
* Function to delete an entry from B+-Tree
* Arguments: BTreeHandle *tree, Tree *node, Value *key
* Output : RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/
RC deleteElement(Tree *nodeptr, Value *keyelement) {

	int i = 0, k;

	// Remove the Element in the node and shift all other elements.

	while (keyelement->v.intV != nodeptr->keys[i]) {
		i++;
	}

	//Check if the node keysize is less than or equal to 1 and move the pointer accordingly
	if (nodeptr->Keysize <= 1) {
		nodeptr->keys[i] = NULL;

		//Mark the node pointer slot and page to zero
		nodeptr->records[i].slot = 0;
		nodeptr->records[i].page = 0;
		nodeptr->ptrs[i] = NULL;
		nodeptr->Keysize--;
		nodeptr = NULL;
	}
	else {

		// if the node keysize is greater than 1 update the node pointer accordingly
		k = i;
		while (k < (nodeptr->Keysize - 1)) {
			nodeptr->keys[k] = nodeptr->keys[k + 1];
			nodeptr->ptrs[k] = nodeptr->ptrs[k + 1];
			nodeptr->records[k] = nodeptr->records[k + 1];
			k++;
		}

		//Assign the node pointer position to current node and decrement the key size
		nodeptr->ptrs[k] = nodeptr->ptrs[k + 1];
		nodeptr->Keysize--;
	}
	return RC_OK;
}


/*
* Function to get the total number of nodes in B+-Tree
* Arguments: BTreeHandle *tree, int *result
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/

RC getNumNodes(BTreeHandle *tree, int *result) {

	TreeInfo *headNode;

	//Intialize the tree handle pointer to root node
	headNode = tree->mgmtData;

	//assign the track of number of inserts to result
	*result = headNode->numOfNodes;
	return RC_OK;

}


/*
* Function to find the leaf node in the B+-Tree
* Arguments: BTreeHandle *tree, Value *key
* Output : RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/
Tree* findLeaf(BTreeHandle *btree, Value *keyelement) {
	Tree *headNode, *tempptr;
	TreeInfo *bttrackptr;

	//Initialize the Btrre scan pointer
	bttrackptr = btree->mgmtData;
	headNode = bttrackptr->mgmtData;

	int i = 0;

	headNode = bttrackptr->mgmtData;

	// Begin the Search and continue until the leaf node is found 
	tempptr = headNode;
	while (tempptr->isLeafNode == false) {
		while (i<headNode->Keysize) {
			if (headNode->keys[i] > keyelement->v.intV) {

				tempptr = headNode->ptrs[i];
				break;

			}
			else {

				tempptr = headNode->ptrs[i + 1];
			}
			i++;
		}
	}

	// Return the leaf node pointer 
	return tempptr;
}


/*
* Function to free the memory space occupied by B + Tree after scan.
* Arguments: BTreeHandle *tree
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
RC closeTreeScan(BT_ScanHandle* handle) {
	//free up the Scan Handle
	free(handle->tree);
	free(handle->mgmtData);

	//return OK after sucessfully closing Tree scan
	return RC_OK;
}


/*
* Function to update initial parent node from B+-Tree
* Arguments: Tree *child, Value *key
* Output : RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/
RC updateParentNode(Tree *childNode, Value *keyelement) {

	Tree *parNode;
	int i = 0;

	//Initialize the parent node
	parNode = childNode->parent;
	if (parNode != NULL) {

		//check for parent node keysize and update the parent node with child node
		while (i<parNode->Keysize) {
			if (keyelement->v.intV == parNode->keys[i]) {

				parNode->keys[i] = childNode->keys[0];

			}
			i++;
		}

		//recursively call the function until parent is NULL
		updateParentNode(parNode, keyelement);
	}
	return RC_OK;

}


/**
*This function is used to insert into parent Node.
*
*
* @author  Pavan
* @param   B Tree Handle
* @param   key
* @return  RC
* @since   2015-12-03
*/
RC deleteKey(BTreeHandle *tree, Value *key) {
	Tree *root;
	TreeInfo *treeInfo;


	//Intializing the pointer variables
	treeInfo = tree->mgmtData;
	root = treeInfo->mgmtData;
	Tree * leafKey, *leftNode;
	leafKey = findLeaf(tree, key);
	leftNode = leafKey->prev;


	//Checking leafKey value for NULL
	if (NULL != leafKey) {
		if (NULL == leafKey->prev) {
			deleteElement(leafKey, key);
			return RC_OK;
		}

		// if leafkey value matches the given key delete leaf element
		if (key->v.intV == leafKey->keys[0]) {
			deleteElement(leafKey, key);
			if (0 == leafKey->Keysize) {
				leftNode->next = leafKey->next;
				deleteParentNode(treeInfo, leafKey, key);
				return RC_OK;
			}

			// Update the parent node after deleting
			updateParentNode(leafKey, key);
			return RC_OK;
		}
		else {

			//Delete the Leaf keyelement check for underflow
			deleteElement(leafKey, key);
			if (leafKey != NULL) {
				if (1 == checkUnderflow(treeInfo, leafKey)) {
					if (NULL != leftNode) {

						//Split the node in case of underflow
						if ((splitLoc(treeInfo->numberOfValuesPerNode) - 1 < leftNode->Keysize) &&
							treeInfo->numberOfValuesPerNode != leftNode->Keysize) {

							//redistribute the node after splitting
							redistribute(treeInfo, leftNode, leafKey);
						}
					}
				}
			}
		}
	}
	return RC_OK;
}

/*
* Function to shutdown Index Manager.
* Arguments: index Id
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
RC shutdownIndexManager() {
	//Return OK
	return RC_OK;
}


/*
* Function to delete initial parent node from B+-Tree
* Arguments: TreeInfo *stat, Tree *nodeptr, Value *keyelement
* Output : RC_OK on sucess
* AUTHOR: Chethan
* DATE: 30, Nov 2015
*/
RC deleteParentNode(TreeInfo *trackinfo, Tree *nodeptr, Value *keyelement) {

	int i = 0, k;
	Tree *curNode, *leftNode;

	// Remove the Parentkey and shift all other keys.

	if (nodeptr != NULL) {
		while (keyelement->v.intV != nodeptr->keys[i]) {
			i++;
		}

		//Check if the parent node keysize is less than or equal to 1 and move the pointer accordingly
		if (nodeptr->Keysize <= 1) {
			curNode = nodeptr->parent;
			leftNode = curNode->ptrs[1];
			nodeptr->keys[i] = NULL;

			//Mark the parent node ptr slot and page to zero
			nodeptr->records[i].slot = 0;
			nodeptr->records[i].page = 0;
			nodeptr->ptrs[i + 1] = NULL;
		}

		else {

			// if the parent node keysize is greater than 1 update the node pointer accordingly
			k = i;
			while (k < (nodeptr->Keysize - 1)) {
				nodeptr->keys[k] = nodeptr->keys[k + 1];
				nodeptr->records[k] = nodeptr->records[k + 1];
				nodeptr->ptrs[k + 1] = nodeptr->ptrs[k + 2];
				k++;
			}

			//Assign the node pointer position to current node and decrement the key size
			curNode = nodeptr->ptrs[1];
			nodeptr->Keysize--;
		}

		//recursively call the function until parent node pointer is NULL
		deleteParentNode(trackinfo, nodeptr->parent, keyelement);
	}
	return RC_OK;

}


/*
* Function to delete the btree by destoring it.
* Arguments: index Id
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
RC deleteBtree(char* idxId) {
	//Destroy the page file using the Index ID
	destroyPageFile(idxId);

	//return OK after sucessfully deleting the Btree
	return RC_OK;
}

/*
* Function to free the memory space occupied by B + Tree
* Arguments: BTreeHandle *tree
* Output : RC_OK
* AUTHOR: Chethan
* DATE: 29, Nov 2015
*/
RC closeBtree(BTreeHandle* tree) {
	//Set the Index ID of the tree handle to NULL
	tree->idxId = NULL;

	//return OK after sucessfully closing the Btree
	return RC_OK;
}
