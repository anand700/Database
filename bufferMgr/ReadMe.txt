*******************************************************************
Introduction
*******************************************************************
Buffer Manager is an application that manages a buffer of blocks in memory including reading/flushing to disk and block replacement.
(flushing blocks to disk to make space for reading new blocks from disk)

*******************************************************************
Data Structure
*******************************************************************
BM_bufferPool:
	pageFile - a pointer that contains the name of the PageFile.
	numPages - It denotes the number of page frames i.e., size of the Buffer pool.
	stratergy - It denotes the page replacement stratergy.
	mgmtData - It is a pointer to the book keeping data.This could include a pointer to the area in memory that stores the page frames
			or data structures needed by the page replacement strategy to make replacement decisions.

BM_Page_Handle:
	pageNum - It denotes the Disk page number(position of the page in the page file).
	data - It denotes the page frame data. A pointer that points to the area in memory, storing the content of the page.

pageFrameDetails:
	pageNumberOnDisk - It denotes the page number of the disk.
	bufferPageNumber - It denotes the page number on the Buffer.
	isDirtyPage - bollean value to indicate whether the page is dirty or not.
	fixedCount - It denotes the number of clients.
	data - It is a pointer that points to the page content.
	next_node - It is pointer that points to the structure pageFrameDetails.

*******************************************************************
Method description
*******************************************************************
Our implimentation approach towards above functionalities are as follows

initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
-Initializes Buffer Pool with the given size and the data with Page Replacement Strategy.
-Initialize the page frame with the given number of pages.

shutdownBufferPool(BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- checks for the dirty pages and if not dirty it shut downs the buffer pool by freeing all the associated resources(Buffer Pool related reference values, Buffer Pool pages and memory).
- Returns an error if the Buffer pool has a pinned page.

forceFlushPool(BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- Traverse to the current page frame and Forces Buffer Manager and writes all Dirty Pages to disk with a fix-count of 0. 

markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
- Checks if the buffer pool is valid.
- Traverse to the current page frame and if the current page is equal to given page, mark that page as Dirty in Buffer pool. 

unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
- Checks if the buffer pool is valid.
- Traverse to the current page frame and if the current page is equal to given page, unpin that page from the Buffer pool.

forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
- Checks if the buffer pool is valid.
- Traverse to the current page frame and if the current page is equal to given page and if it is dirty, Write the current content of given Page back to the page file on Disk.

pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
- Checks if the buffer pool is valid.
- Traverse through the bufferpool to check if the page is already present.
- If the page is not present in bufferpool then get it from memory.
- Check if the bufferpool is full. If the bufferpool is not full then add a new node and pin the page to that node.
- If the Bufferpool is full. Then use Page replacement statergies(FIFO/LRU) to pin the page.
- Using FIFO page replacement stratergy
	-We remove the page which is pinned first into the buffer pool and pin the current page.
- Using LRU page replacement stratergy
	-We remove the least recently used page from bufferpool and reorder and pin the page.

getFrameContents (BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- Traverse to the page frame and return an array of disk page numbers or No_Page error if the page frame is empty.

getDirtyFlags (BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- Traverse to the page frame and return an array of boolean values indicating whether the page is dirty or not.
- If the page frame is empty then it indicates the page frame is clean.

getFixCounts (BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- Traverse to the page frame and return an array of integers which indicates the fix count of the page stored in page frame.
- Returns 0 If the page frame is empty.

getNumReadIO (BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- Returns the count of number of pages that have been read from the disk.

getNumWriteIO (BM_BufferPool *const bm)
- Checks if the buffer pool is valid.
- Returns the count of number of pages that have been written to the Page file.



*********************************************************************
PROCEDURE TO RUN THE PROJECT
*********************************************************************
There are two makefiles in the project (one for test_assign2_1.c and one for test_assign2_2.c).

Steps to run the makefiles are as follows:

FIRST MAKEFILE (Makefile1):
1) Open the Linux terminal, navigate to the directory with assignment contents using the command 'cd'
2) Execute the make file as below: 
		make -f Makefile1

//3) Then run the below command:
		./assignment2
			
SECOND MAKEFILE (Makefile2):
1) Open the Linux terminal, navigate to the directory with assignment contents using the command 'cd'
2) Execute the make file as below: 
		make -f Makefile2










