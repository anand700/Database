#include"buffer_mgr.h"
#include"dberror.h"
#include"storage_mgr.h"
#include<stdlib.h>
#include"dt.h"
#include<string.h>
#include<math.h>



int writeIOCount;
int readIOCount;

SM_FileHandle *fh;
SM_PageHandle ph;
pageFrameDetails *startNode;
int FIFOCounter;


#define FOR_EACH(iterator, pagesize) \
    for (iterator = 0; iterator < pagesize; iterator++)


/**
*	Initializes the bufferPool with the provided data and resets the global read and write IO counts
* @param bm(BM_BufferPool) - its a *const where only the value of the variable it is pointing to can be changed
* @param pageFileName - gives the name of the file that the buffer pool belongs to
* @param strategy - gives the name of the place replacement strategy that this buffer pool follows
* @param stratData - Void pointer which can contain any data that is required by the strategy
* @returns RC code
* @author Pavan Rao
* @date 9/27/2015
**/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
	const int numPages, ReplacementStrategy strategy,
	void *stratData)
{

	// setting the members of the structure
	bm->pageFile = pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = NULL;

	fh = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
	initFileHandle(fh);
	readIOCount = 0;
	writeIOCount = 0;
	ph = (SM_PageHandle *)malloc(PAGE_SIZE);
	startNode = NULL;
	FIFOCounter = 0;

	return openPageFile(bm->pageFile, fh);
}

/**
* This forces all the dirty pages with its fix_count set to zero to write back the pages to 0
* @param bm(BM_BufferPool) - its a *const where only the value of the variable it is pointing to can be changed
* @results RC code
* @author Pavan Rao
* @date 9/27/2015
**/
RC forceFlushPool(BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}
	pageFrameDetails *curr = startNode;

	while (((pageFrameDetails *)curr) != NULL)
	{
		BM_PageHandle *temp = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
		temp->pageNum = ((pageFrameDetails *)curr)->pageNumberOnDisk;
		temp->data = ((pageFrameDetails *)curr)->data;
		forcePage(bm, temp);
		free(temp);
		curr = ((pageFrameDetails *)curr)->next_node;
	}

	return RC_OK;
}

/**
* This is the function that checks if the page is dirty and then writes the page back to the disk if the fix count is 0
* @param bm(BM_BufferPool) - its a *const where only the value of the variable it is pointing to can be changed
* @param page(BM_PageHandle) - this gives the data that is to be checked for disk page number and the data to be writted back to the disk
* @returns RC_code
* @author Pavan Rao
* @date 9/27/2015
*/
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty(bm);
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}
	pageFrameDetails *curr = startNode;

	while (((pageFrameDetails *)curr) != NULL)
	{
		if (((pageFrameDetails *)curr)->pageNumberOnDisk == page->pageNum)
		{
			if (((pageFrameDetails *)curr)->fixedCount == 0)
			{
				if (((pageFrameDetails *)curr)->isDirtyPage == TRUE)
				{
					if (writeBlock(page->pageNum, fh, ((pageFrameDetails *)curr)->data) != RC_OK)
					{
						return RC_WRITE_FAILED;
					}

					writeIOCount++;
					curr->isDirtyPage = FALSE;
				}
				return RC_OK;
			}
			else
			{
				return RC_BUFFER_PAGE_IN_USE;
			}
		}

		curr = ((pageFrameDetails *)curr)->next_node;
	}

	return RC_BUFFER_PAGE_NOT_FOUND;
}

/**
* Fetches an array of bool values whose each element denotes the dirtyFlagStatus of its respective PageFrame
* @param bm(BM_BufferPool) - its a *const where only the value of the variable it is pointing to can be changed
* @returns bool - An array of bool values
* @author Pavan Rao
* @date - 9/27/2015
*/
bool *getDirtyFlags(BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}
	pageFrameDetails *curr = startNode;
	bool *dirtyFlagStatusArr = (bool*)malloc(sizeof(bool) * bm->numPages);
	int i = 0;

	while (i < bm->numPages)
	{
		if (((pageFrameDetails *)curr) != NULL)
		{
			dirtyFlagStatusArr[i] = ((pageFrameDetails *)curr)->isDirtyPage;
			curr = ((pageFrameDetails *)curr)->next_node;
		}
		else
			dirtyFlagStatusArr[i] = FALSE;
		i++;
	}

	return dirtyFlagStatusArr;
}

/**
* Inserts the element in FIFO fashion
* @param page which contains the details of the page that is to be inserted
* @param bm(BM_BufferPool) - its a *const where only the value of the variable it is pointing to can be changed
* @returns RC code for success and errors
* @author Pavan Rao
* @date 9/27/2015
*/
RC FIFOInsert(pageFrameDetails *page, BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);
	int i;
	if (returnVal != RC_OK) {
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}
	pageFrameDetails *curr = startNode;
	pageFrameDetails *prev = NULL;

	for (i = 0; i < FIFOCounter; i++)
	{
		prev = curr;
		curr = curr->next_node;
	}

	int originalFifoCounter = FIFOCounter;

	do
	{
		//check if the page that is to be removed is dirty and Fix count is > 0
		if (((pageFrameDetails *)curr)->fixedCount == 0)
		{
			if (((pageFrameDetails *)curr)->isDirtyPage == TRUE)
			{
				BM_PageHandle *temp = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));

				temp->data = curr->data;
				temp->pageNum = ((pageFrameDetails *)curr)->pageNumberOnDisk;

				//this is to write the page content back to the file when it is about to be removed
				RC status = forcePage(bm, temp);
				free(temp);

				if (RC_OK != status)
				{
					return status;
				}
			}
			FIFOCounter = ++FIFOCounter % bm->numPages;
			break;
		}
		else
		{
			FIFOCounter = ++FIFOCounter % bm->numPages;
			if (FIFOCounter == 0)
			{
				prev = NULL;
				curr = startNode;
			}
			else
			{
				prev = curr;
				curr = curr->next_node;
			}
		}
	} while (FIFOCounter != originalFifoCounter);

	if (FIFOCounter == originalFifoCounter)
	{
		return RC_BUFFER_PAGE_NOT_FOUND;
	}

	if (prev == NULL)
	{
		page->next_node = curr->next_node;
		startNode = page;
	}
	else
	{
		prev->next_node = page;
		page->next_node = curr->next_node;
	}
	free(curr);

	return RC_OK;
}



/**
* Inserts the page in LRU fashion
* @param page which contains the details of the page that is to be inserted
* @param bm(BM_BufferPool) - its a *const where only the value of the variable it is pointing to can be changed
* @returns RC code
* @author Pavan Rao
* @date 9/27/2015
*/
RC LRUInsert(pageFrameDetails *page, BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}

	bool isHit = FALSE;
	pageFrameDetails *curr = startNode;
	pageFrameDetails *prev = NULL;
	int numOfElements = 0;

	if (curr == NULL)
	{
		page->order = 1;
		page->next_node = NULL;
		startNode = page;
		return RC_OK;
	}

	while (curr != NULL)
	{
		prev = curr;
		if (curr->pageNumberOnDisk == page->pageNumberOnDisk)
		{
			isHit = TRUE;
		}

		numOfElements++;
		curr = curr->next_node;
	}


	if (isHit == TRUE)
	{
		if (RC_OK != LRUReorder(page))
		{
			return RC_LRU_FAIL;
		}
	}
	else
	{
		if (numOfElements < bm->numPages)
		{
			pageFrameDetails *temp1 = startNode;
			while (temp1 != NULL)
			{
				temp1->order++;
				temp1 = temp1->next_node;
			}
			page->order = 1;
			prev->next_node = page;
		}
		else
		{
			curr = startNode;
			prev = NULL;
			int i = bm->numPages;

			while (curr != NULL && i>0)
			{

				if (curr->order == i)
				{
					if (curr->fixedCount != 0)
					{
						i--;
						curr = startNode;
						continue;
					}
					else
					{
						int x = curr->order;
						pageFrameDetails *temp = startNode;

						while (temp != NULL)
						{
							if (temp->order < x)
							{
								temp->order++;
							}
							temp = temp->next_node;
						}

						if (prev == NULL)
						{
							page->order = 1;
							page->next_node = curr->next_node;
							free(curr);
							startNode = page;
						}
						else
						{
							prev->next_node = page;
							page->order = 1;
							page->next_node = curr->next_node;
							free(curr);
						}
						return RC_OK;
					}
				}
				prev = curr;
				curr = curr->next_node;
			}
		}
	}
	curr = NULL;
	prev = NULL;
	free(curr);
	free(prev);
	return RC_OK;
}


RC LRUReorder(pageFrameDetails *page)
{
	pageFrameDetails *curr = startNode;
	int x = page->order;

	while (curr != NULL)
	{
		if (curr->order < x)
		{
			curr->order++;
		}
		else if (curr->order == x)
		{
			curr->order = 1;
		}
		curr = curr->next_node;
	}

	return RC_OK;
}

/**
*This function shuts down the buffer pool
*and free's up memory
*
*
* @author  Anand N
* @param   BM_BufferPool
* @return  RC value(defined in dberror.h)
* @since   2015-10-02
*/
RC shutdownBufferPool(BM_BufferPool *const bm) {

	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	printf(">> shutdownBufferPool() : bm->pageFile:%s", bm->pageFile);
	pageFrameDetails *tempNode = startNode;
	pageFrameDetails *prevToTempNode;
	forceFlushPool(bm);
	closePageFile(fh);
	printf("<< shutdownBufferPool() : bm->pageFile:%s", bm->pageFile);

	return RC_OK;
}

/**
*This function un pins a page
*
*
* @author  Anand N
* @param   BM_BufferPool
* @param   BM_PageHandle
* @return  RC value(defined in dberror.h)
* @since   2015-10-02
*/
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {

	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}
	printf(">> unpinPage() : bm->pageFile:%s", bm->pageFile);
	pageFrameDetails *tempNode = startNode;
	while (tempNode->pageNumberOnDisk != page->pageNum) {
		if (NULL == tempNode->next_node) {
			return RC_INVALID_PAGE_NUM;
		}
		tempNode = tempNode->next_node;
	}
	tempNode->fixedCount--;
	printf("<< unpinPage() : bm->pageFile:%s", bm->pageFile);

	return RC_OK;
}


/**
*This function gets the Frame Contents
*
*
* @author  Anand N
* @param   BM_BufferPool
* @return  PageNumber
* @since   2015-10-02
*/
PageNumber *getFrameContents(BM_BufferPool *const bm) {

	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}
	printf(">> getFrameContents() : bm->pageFile:%s", bm->pageFile);
	int iterator = 0;
	int *arrayOfPageNumbers = (int *)malloc(sizeof(int) * bm->numPages);
	pageFrameDetails* tempNode = startNode;
	while (iterator < bm->numPages) {
		if (tempNode == NULL)
		{
			arrayOfPageNumbers[iterator] = NO_PAGE;
		}
		else
		{
			arrayOfPageNumbers[iterator] = tempNode->pageNumberOnDisk;
			tempNode = tempNode->next_node;
		}
		iterator++;
	}
	printf("<< getFrameContents() : bm->pageFile:%s", bm->pageFile);

	return arrayOfPageNumbers;
}

/**
*This function pins a page onto the buffer pool
*
*
* @author  Anand N
* @param   BM_BufferPool
* @param   BM_PageHandle
* @param   pageNum
* @return  RC value(defined in dberror.h)
* @since   2015-10-02
*/
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
	const PageNumber pageNum) {

	RC returnVal = checkIfBufferPoolIsValid(bm);
	if (returnVal != RC_OK) {
		return returnVal;
	}
	printf(">> pinPage() : bm->pageFile:%s", bm->pageFile);
	int countPagesInBufferPool = 0;
	pageFrameDetails *traverseNode = (pageFrameDetails *)malloc(sizeof(pageFrameDetails));
	traverseNode = startNode;

	// this is when there are pageframes in the buffer
	if (traverseNode != NULL)
		countPagesInBufferPool = 1;

	char temp[PAGE_SIZE];
	while (traverseNode == NULL || traverseNode->pageNumberOnDisk != pageNum) {
		// If page is not present in the buffer pool get it from the memory
		if (traverseNode == NULL
			|| NULL == traverseNode->next_node) {
			if (RC_OK != ensureCapacity(pageNum + 1, fh)) {
				return RC_ENSURE_CAPACITY_FAILED;
			}
			if (RC_OK != readBlock(pageNum, fh, ph))
			{
				return RC_READ_FAILED;
			}
			memmove(temp, ph, PAGE_SIZE);
			page->data = temp;
			page->pageNum = pageNum;
			readIOCount = readIOCount + 1;
			pageFrameDetails *newNode = (pageFrameDetails *)malloc(sizeof(pageFrameDetails));
			newNode->pageNumberOnDisk = pageNum;
			memmove(newNode->data, (page->data), PAGE_SIZE);
			newNode->fixedCount = 1;
			newNode->isDirtyPage = FALSE;
			newNode->next_node = NULL;

			// If size of the buffer pool is not full
			if (countPagesInBufferPool < bm->numPages
				&& bm->strategy != RS_LRU) {
				if (traverseNode == NULL)
				{
					startNode = newNode;
				}
				else
				{
					traverseNode->next_node = newNode;
				}
				return RC_OK;
			}
			// If size of the buffer pool is full
			else {
				if (bm->strategy == RS_FIFO) {
					if (FIFOInsert(newNode, bm) != RC_OK)
					{
						return RC_FIFO_INSERT_FAILED;
					}
					return RC_OK;
				}
				else if (bm->strategy == RS_LRU) {
					LRUInsert(newNode, bm);
					return RC_OK;
				}
			}

		}
		traverseNode = traverseNode->next_node;
		countPagesInBufferPool++;
	}
	// If page is present in the buffer pool reorder
	// If size of the buffer pool is not full
	traverseNode->fixedCount += 1;
	page->pageNum = traverseNode->pageNumberOnDisk;
	page->data = traverseNode->data;
	if (bm->strategy == RS_LRU) {
		LRUReorder(traverseNode);
	}
	printf("<< pinPage() : bm->pageFile:%s", bm->pageFile);

	return RC_OK;
}


/**
*This function checks if Buffer Pool is initiated.
*
*
* @author  Anand N
* @param   BM_BufferPool
* @return  RC
* @since   2015-10-02
*/
RC checkIfBufferPoolIsValid(BM_BufferPool *const bm) {
	if (NULL == bm ||
		NULL == bm->pageFile ||
		0 > bm->strategy ||
		0 > bm->numPages) {
		// checks if the Buffer Pool is initiated
		return RC_BM_BUFFERPOOL_NOT_INIT;
	}

	return RC_OK;
}

/*
* Function to mark given page as Dirty Page in the Buffer Pool
* INPUT: const buffermanger pointer, const PageHandle pointer
* OUTPUT: Mark the bit Dirty and return RC_OK
* AUTHOR: Chethan
* DATE: 26,SEP 2015
*/
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);

	if (returnVal != RC_OK)
	{
		return returnVal;
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return returnVal1;
	}

	pageFrameDetails *cur_node = startNode;

	while (cur_node != NULL && cur_node->pageNumberOnDisk != page->pageNum)
	{
		cur_node = cur_node->next_node;
	}

	if (cur_node == NULL)
	{
		return RC_BUFFER_PAGE_NOT_FOUND;
	}
	else
	{
		cur_node->isDirtyPage = true;
		memmove(cur_node->data, page->data, PAGE_SIZE);
		return RC_OK;
	}
}

/*
* function which returns fix count of pages stored in Buffer Pool
* INPUT: const buffermanger pointer
* OUTPUT: Return fixCount
* AUTHOR: Chethan
* DATE: 26,SEP 2015
*/
int *getFixCounts(BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);


	if (returnVal != RC_OK)
	{
		return 0; //if the bufferpool that is received as a parameter is not a valid bufferpool then this returns 0
	}
	RC returnVal1 = checkIfBufferPoolIsEmpty();
	if (returnVal1 != RC_OK) {
		return 0;
	}
	int i;
	int *fixCount = (int *)malloc(sizeof(int) * bm->numPages);
	pageFrameDetails *cur_node = startNode;

	for (i = 0; i < bm->numPages; i++)
	{
		if (cur_node != NULL)
		{
			fixCount[i] = cur_node->fixedCount;
			cur_node = cur_node->next_node;
		}
		else
		{
			fixCount[i] = 0;
		}
	}

	return fixCount;
}

/*
* Function which returns the count of pages read from Disk
* INPUT: const buffermanger pointer
* OUTPUT: return readIOCount
* AUTHOR: Chethan
* DATE: 27,SEP 2015
*/
int getNumReadIO(BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);

	if (returnVal != RC_OK)
	{
		return 0;
	}

	return readIOCount;
}
/*
* Returns the count of  pages written to the page file from the Buffer Pool
* INPUT: const buffermanger pointer
* OUTPUT: return writeIOCount
* AUTHOR: Chethan
* DATE: 27,SEP 2015
*/
int getNumWriteIO(BM_BufferPool *const bm)
{
	RC returnVal = checkIfBufferPoolIsValid(bm);

	if (returnVal != RC_OK)
	{
		return 0;
	}

	return writeIOCount;
}

/**
*This function checks if Buffer pool is empty.
*
*
* @author  Anand N
* @param   BM_PageHandle
* @return  RC
* @since   2015-10-02
*/
RC checkIfBufferPoolIsEmpty() {
	if (NULL == startNode) {
		// checks if the Buffer Page is initiated
		return RC_BUFFER_POOL_EMPTY;
	}

	return RC_OK;
}
