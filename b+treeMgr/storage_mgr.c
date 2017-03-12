#include "storage_mgr.h"
#include "math.h"
#include "stdlib.h"
#include "dberror.h"
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#pragma warning (disable : 4996)

#define FOR_EACH(iterator, pagesize) \
    for (iterator = 0; iterator < pagesize; iterator++)

/**
*This function initializes all the values
*required for the storage manager
*
*
* @author  Anand N
* @return  void
* @since   2014-09-04
*
*/
void initStorageManager() {
}

/**
*This function creates a page file
*having one page as default.
*
*
* @author  Anand N
* @param   fileName
* @return  RC value(defined in dberror.h)
* @since   2014-09-04
*/
RC createPageFile(char *fileName)
{
	RC returnVal = checkIfFileNameIsValid(fileName);

	if (returnVal != RC_OK)
	{
		return returnVal;
	}

	printf(">> createPageFile() : fileName:%s", fileName);
	FILE *newFilePointer;
	int iterator;
	newFilePointer = fopen(fileName, "w+");

	if (newFilePointer == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	FOR_EACH(iterator, PAGE_SIZE) {
		fprintf(newFilePointer, "%c", '\0');
	}

	fclose(newFilePointer);
	printf("<< createPageFile() : fileName:%s", fileName);

	return RC_OK;
}

/**
*	Opens the page file and updates the page file handler
*	@param fileName(char) - pointer that points to the name of the file which has to be opened
*	@param fHandle(SM_fileHandler) - Pointer that holds the information of the file that is being pointed
*	@returns RC
*	@author Pavan Rao
*	@date 9/6/2015
*/
RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
	RC returnVal = checkIfFileNameIsValid(fileName);

	if (returnVal != RC_OK)
	{
		return returnVal;
	}

	printf(">> openPageFile() : fileName:%s", fileName);
	//check if there already exists a file that is open previously
	if (NULL != fHandle && NULL != fHandle->mgmtInfo)
	{
		//this means that there already exists a file that is opened previously
		return RC_FILE_ALREADY_IN_USE;
	}

	FILE *newFile = fopen(fileName, "r+");
	if (!newFile)
	{
		//this is the scenario when the file is not able to open
		return RC_FILE_NOT_FOUND;
	}
	//to find the total number of pages in the file
	int isSeekSuccessfull = fseek(newFile, 0L, SEEK_END);  /* fseek returns non-zero on error. */

	if (isSeekSuccessfull != 0)
	{
		fclose(newFile);
		return RC_RM_NO_MORE_TUPLES;
	}

	long fileSize = ftell(newFile);   // fileSize is of type long and it represents the total bytes in the file
	rewind(newFile); // this resets the file pointer to begining of the file

	if (fileSize == 0)	// this is check if the file is empty
	{
		fHandle->totalNumPages = 0;
		fclose(newFile);
		return RC_IM_NO_MORE_ENTRIES;
	}

	int totalPageSize = (int *)(fileSize / PAGE_SIZE);

	fHandle->fileName = fileName;
	fHandle->curPagePos = 0;

	if ((fileSize % PAGE_SIZE) == 0)
	{
		fHandle->totalNumPages = totalPageSize;
	}
	else
	{
		fHandle->totalNumPages = ++totalPageSize;
	}

	fHandle->mgmtInfo = newFile;
	printf("<< openPageFile() : fileName:%s", fileName);

	return RC_OK;
}

/**
*	This closes the file or distroys the file
*	@param SM_FileHandle : pointer that points to the file page
*	@returns RC_OK : if the page handler is closed properly
*	@returns RC_FILE_NOT_FOUND : if the file is not found
*	@author Pavan
*	@date 9/6/2015
*/
RC closePageFile(SM_FileHandle *fHandle)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> closePageFile() : fileName:%s", fHandle->fileName);

	fclose(fHandle->mgmtInfo);

	fHandle->curPagePos = NULL;	// this closes the file
	fHandle->fileName = NULL;
	fHandle->mgmtInfo = NULL;
	fHandle->totalNumPages = NULL;

	printf("<< closePageFile() : fileName:%s", fHandle->fileName);
	return RC_OK;
}

/*

* METHOD: Destroy the file
* INPUT: fileName
* OUTPUT: RC_OK­SUCCESS or RC_File_Not_Found­FAIL
* AUTHOR: Chethan
* DATE: 2,SEP ­2015
*/
RC destroyPageFile(char *fileName)
{
	RC returnVal = checkIfFileNameIsValid(fileName);
	if (returnVal != RC_OK)
	{
		return returnVal;
	}

	printf(">> destroyPageFile() : fileName:%s", fileName);
	if (remove(fileName) == 0) {
		return RC_OK;
	}
	printf("<< destroyPageFile() : fileName: %s", fileName);
	return RC_FILE_NOT_FOUND;
}


/*Reading Blocks from Disc */
/*
* METHOD: Read the current Block from the File
* INPUT: File Structure, Content of the data to be read
* OUTPUT: RC_OK­SUCESSS;RC_OTHERS­ON FAIL;
* AUTHOR: Chethan
* DATE: 4­,SEP­ 2015
*/
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC isPageNumberValid = checkIfPageNumberIsValid(pageNum);
	if (isPageNumberValid != RC_OK)
	{
		return isPageNumberValid;
	}

	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> readBlock() : To pageNum:%d", pageNum);
	if (pageNum > fHandle->totalNumPages - 1) {		//its comparing with totalNumPages-1 because the pageNum starts from 0
		return RC_READ_NON_EXISTING_PAGE;
	}
	fseek(fHandle->mgmtInfo, PAGE_SIZE*(pageNum), SEEK_SET);//Move the file pointer to the desried postion
	fread(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo);//read the content to mempage  passed.
	fHandle->curPagePos = pageNum;//Set the current page position as soon as page is read.
	printf("<< readBlock() ");
	return RC_OK;
}

/**
*This function gets the block position
*to be read or written.
*
*
* @author  Anand N
* @param   fileHandle(consists of current page position,
*		   file name, total number of pages and
*          management info)
* @return  the current page position in a file
* @since   2014-09-04
*/
int getBlockPos(SM_FileHandle *fHandle) {
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> getBlockPos() : fileName:%s", fHandle->fileName);
	if (fHandle->totalNumPages == 0) {
		return RC_EMPTY_FILE;
	}
	printf("<< getBlockPos() : fHandle->curPagePos:%d", fHandle->curPagePos);
	return fHandle->curPagePos;
}

/**
*This function reads the first block.
*
*
* @author  Anand N
* @param   fileHandle(consists of current page position,
*		   file name, total number of pages and
*          management info),
* @param   pageHandle(is a pointer to an area in memory
*          storing the data of a page)
* @return  RC value(defined in dberror.h)
* @since   2014-09-04
*/
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> readFirstBlock() : fileName:%s", fHandle->fileName);
	if (fHandle->totalNumPages == 0) {
		return RC_EMPTY_FILE;
	}
	int isSeekSuccessfull = fseek(fHandle->mgmtInfo, 0, SEEK_SET);  /* fseek returns non-zero on error. */
	if (isSeekSuccessfull != 0) {
		return RC_RM_NO_MORE_TUPLES;
	}
	fread(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo);
	fHandle->curPagePos = 1;
	printf("<< readFirstBlock() ");
	return RC_OK;

}

/**
*	Reads the previous block of the file if it exists
*	@param SM_FileHandle : Contains the details of the file that has been opened
*	@param SM_PageHandle : points to the memory space which contains the data that is to be read
*	results RC
*	@author Pavan Rao
*	@date 9/6/2015
*/
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> readPreviousBlock() : fileName:%s", fHandle->fileName);
	FILE *newFile = fHandle->mgmtInfo;

	if (newFile == NULL)	// Check for whether the read function was successfull
	{
		return RC_FILE_NOT_FOUND;
	}

	int bitPos = fHandle->curPagePos;	//Assumption curPagePos contains the reference to the initial address of the page
	bitPos = (--bitPos)*PAGE_SIZE;

	if (bitPos < 0) //to check if there exists a previous page
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	fseek(newFile, bitPos, SEEK_SET);

	int readSize = fread(memPage, PAGE_SIZE, 1, newFile); // readSize is the number of elements that is read

	if (readSize == NULL)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	fHandle->mgmtInfo = newFile;	//re-initializing the file pointer to mgminfo for further info
	printf("<< readPreviousBlock() ");

	return RC_OK;
}

/**
*	Reads the current block to memPage
*	@param SM_FileHandle : Pointer that holds the references to the position of the file
*	@param SM_PageHandle : Char Pointer that holds the initial reference to the page that has to be read
*	@return RC
*	@author	Pavan Rao
*	@date 9/6/2015
*/
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> readCurrentBlock() : fileName:%s", fHandle->fileName);
	if (fHandle->curPagePos >= fHandle->totalNumPages)
	{
		// this is to check if the previously the last page of the file was read and the cursor is pointing to an invalid location
		return RC_READ_NON_EXISTING_PAGE;
	}

	FILE *newFile = fHandle->mgmtInfo;

	if (newFile == NULL)	// Check whether there exists a valid file open
	{
		return RC_FILE_NOT_FOUND;
	}

	int bitPos = fHandle->curPagePos;
	bitPos = (bitPos)*PAGE_SIZE;

	if (bitPos < 0) //to check if this is a valid page
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	fseek(newFile, bitPos, SEEK_SET);
	int readSize = fread(memPage, PAGE_SIZE, 1, newFile); // readSize is the number of elements that is read

	if (readSize == NULL)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	fHandle->curPagePos++;
	fHandle->mgmtInfo = newFile;
	printf("<< readCurrentBlock() ");

	return RC_OK;
}


/*
* METHOD: Read the next Block from the current block in the File
* INPUT: File Structure, Content of the data to be read
* OUTPUT: RC_OK­SUCESSS;RC_OTHERS­ON FAIL;
* AUTHOR: Chethan
* DATE: 4­,SEP ­2015
*/
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> readNextBlock() : fileName:%s", fHandle->fileName);
	if ((fHandle->curPagePos) + 1 >= fHandle->totalNumPages)//Check for existence
	{
		return RC_READ_NON_EXISTING_PAGE;//return if non existing page is tried to read.
	}
	fseek(fHandle->mgmtInfo, PAGE_SIZE, SEEK_CUR);//Move the file pointer to the desried postion
	fread(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo);//read the content to mempage as passed.
	fHandle->curPagePos = (fHandle->curPagePos) + 2;//Set the next page position as soon as page is read.
	printf("<< readNextBlock() ");
	return RC_OK;//return final status.
}

/*
* METHOD: Read Last Block from File
* INPUT: Page Number,File Structure, Content of the data to be read
* OUTPUT: RC_OK­SUCESSS;RC_OTHERS­ON FAIL;
* AUTHOR: Chethan
* DATE: 5,­SEP ­2015
*/
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> readLastBlock() : fileName:%s", fHandle->fileName);
	int curPos = -1;

	//Seeking to the last block of the file
	if ((fseek(fHandle->mgmtInfo, PAGE_SIZE * (fHandle->totalNumPages - 1), SEEK_SET)) != 0) {
		return RC_FILE_SEEK_ERROR;
	}
	if ((fread(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo)) == -1) {
		return RC_READ_FAILED;
	}
	curPos = (int *)(ftell(fHandle->mgmtInfo) / PAGE_SIZE);
	fHandle->curPagePos = curPos - 1;
	printf("<< readLastBlock() ");
	return RC_OK;
}

/**
*This function writes a block.
*
*
* @author  Anand N
* @param   fileHandle(consists of current page position,
*		   file name, total number of pages and
*          management info),
*          pageHandle(is a pointer to an area in memory
*          storing the data of a page)
* @return  RC value(defined in dberror.h)
* @since   2014-09-04
*/
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC isPageNumberValid = checkIfPageNumberIsValid(pageNum);
	if (isPageNumberValid != RC_OK)
	{
		return isPageNumberValid;
	}

	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> writeBlock() : To pageNum:%d", pageNum);

	if (fHandle->totalNumPages == 0) {
		return RC_EMPTY_FILE;
	}
	if (sizeof(memPage) / PAGE_SIZE > 1) {
		return RC_CANNOT_WRITE_MORE_THAN_ONE_BLOCK;
	}

	//if the requested page to be written is less than the number of pages present in the pageFile
	if (fHandle->totalNumPages < pageNum)
	{
		ensureCapacity(pageNum + 1, fHandle);
	}

	int isSeekSuccessfull = fseek(fHandle->mgmtInfo, PAGE_SIZE*(pageNum), SEEK_SET);  /* fseek returns non-zero on error. */
	if (isSeekSuccessfull != 0) {
		return RC_RM_NO_MORE_TUPLES;
	}
	fwrite(memPage, 1, PAGE_SIZE, fHandle->mgmtInfo);	//this is changed to page_size - 1 because after writing the 4097 values the filepointer used to get advanced by one value and ftell would return 4097
	fHandle->curPagePos = pageNum;
	printf("<< writeBlock() ");
	return RC_OK;
}

/*Writing Blocks onto Disc */
/*
* AUTHOR: Chethan
* METHOD: To write the given data to the current page block in the file
* INPUT: File Structure, Content of the data to be written
* OUTPUT: RC_OK­SUCESSS;RC_OTHERS­ON FAIL;
* AUTHOR: Chethan
* DATE: 5,­SEP ­2015
*/
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> writeCurrentBlock() : fileName:%s", fHandle->fileName);

	int curPos = -1;
	int bitPos = fHandle->curPagePos;	//Assumption curPagePos contains the reference to the initial address of the page

	bitPos = (bitPos)*PAGE_SIZE;

	if (bitPos < 0) //to check if this is a valid page
	{
		return RC_WRITE_FAILED;
	}
	if ((fseek(fHandle->mgmtInfo, bitPos, SEEK_SET)) < 0) {
		return RC_FILE_SEEK_ERROR;
	}

	if ((fwrite(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo)) == -1) {
		return RC_WRITE_FAILED;
	}

	fHandle->curPagePos = curPos + 1;
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	printf("<< writeCurrentBlock() ");

	return RC_OK;
}

/*
* METHOD: To add an empty block to the file.
* INPUT: File Structure
* OUTPUT: RC_OK­SUCESSS;RC_OTHERS­ON FAIL;
* AUTHOR: Chethan
* DATE: 6­,SEP ­2015
*/
RC appendEmptyBlock(SM_FileHandle *fHandle)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> appendEmptyBlock() : fileName:%s", fHandle->fileName);
	int i;
	FILE *fpointer;
	fpointer = fHandle->mgmtInfo;
	for (i = 0; i<PAGE_SIZE; i++) {
		fprintf(fpointer, "%c", '\0');
	}
	fHandle->curPagePos += 1;
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	printf("<< appendEmptyBlock() ");
	return RC_OK;
}


/**
*	This function ensures the capasity of SM_FileHandle
*	@param numberOfPages : Gives the required number of pages
*	@param SM_FileHandle : Pointer that holds the references to the position of the file
*	@returns : RC
*	@author Pavan Rao
*	@date 9/6/2015
*/
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
	RC isPageNumberValid = checkIfPageNumberIsValid(numberOfPages);
	if (isPageNumberValid != RC_OK)
	{
		return isPageNumberValid;
	}

	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);
	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> ensureCapacity() : numberOfPages:%d", numberOfPages);

	if (fHandle->totalNumPages < numberOfPages)
	{
		while (fHandle->totalNumPages != numberOfPages)
		{
			appendEmptyBlock(fHandle);
		}
	}
	printf("<< ensureCapacity() ");
	return RC_OK;
}


/**
*This function checks if the file name is valid.
*
*
* @param   file Name
* @return  RC
* @since   2014-09-10
*/
RC checkIfFileNameIsValid(char *fileName) {
	if (NULL == fileName) {
		// checks if the file name is valid
		return RC_INVALID_FILE_NAME;
	}

	return RC_OK;
}

/**
*This function checks if the file handle is initiated.
*
*
* @param   fileHandle(consists of current page position,
*		   file name, total number of pages and
*          management info)
* @return  RC
* @since   2014-09-10
*/
RC checkIfFileHandleIsInitiated(SM_FileHandle *fHandle) {
	if (NULL == fHandle ||
		NULL == fHandle->fileName ||
		NULL == fHandle->mgmtInfo ||
		0 > fHandle->totalNumPages ||
		0 > fHandle->curPagePos) {
		// checks if the File Handle is initiated
		return RC_FILE_HANDLE_NOT_INIT;
	}

	return RC_OK;
}

/**
*This function checks if page number is valid.
*
*
* @param   Page Number
* @return  RC
* @since   2014-09-10
*/
RC checkIfPageNumberIsValid(int pageNum) {
	// checks if page number is valid
	if (0 > pageNum) {
		return RC_INVALID_PAGE_NUM;
	}

	return RC_OK;
}

/**
*This function writes more than one block onto disk.
*
*
* @author  Anand N
* @param   fileHandle(consists of current page position,
*		   file name, total number of pages and
*          management info),
*          pageHandle(is a pointer to an area in memory
*          storing the data of a page)
*		   countOfMemPage (is a count of how many pages this memPage variable has)
* @return  RC value(defined in dberror.h)
* @since   2014-09-04
*/
RC writeFromBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage, int countOfMemPage)
{
	RC isPageNumberValid = checkIfPageNumberIsValid(pageNum);
	int totalNumberOfPages = -1;

	if (isPageNumberValid != RC_OK)
	{
		return isPageNumberValid;
	}

	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> writeFromBlock() : To pageNum:%d", pageNum);

	if (fHandle->totalNumPages == 0)
	{
		return RC_EMPTY_FILE;
	}

	int numberOfPages = 0;

	if ((countOfMemPage % PAGE_SIZE) == 0)
	{
		numberOfPages = (int*)(countOfMemPage / PAGE_SIZE);
	}
	else
	{
		numberOfPages = (int*)(countOfMemPage / PAGE_SIZE) + 1;
	}

	//computing the value for number of pages required a successfull right
	totalNumberOfPages = numberOfPages - (fHandle->totalNumPages - fHandle->curPagePos + 1) > 0 ? numberOfPages : fHandle->totalNumPages;

	ensureCapacity(totalNumberOfPages, fHandle);

	int isSeekSuccessfull = fseek(fHandle->mgmtInfo, PAGE_SIZE*(pageNum), SEEK_SET);  // fseek returns non-zero on error

	if (isSeekSuccessfull != 0)
	{
		return RC_RM_NO_MORE_TUPLES;
	}

	fwrite(memPage, PAGE_SIZE * numberOfPages, 1, fHandle->mgmtInfo);
	printf("<< writeFromBlock()");

	return RC_OK;
}

/*Writing more than one Block onto Disc */
/*
*	@param SM_FileHandle : Pointer that holds the references to the position of the file
*	@param SM_PageHandle : Char Pointer that holds the initial reference to the page that has to be read
*	@param countOfMemPage : is a count of how many pages this memPage variable has
*	@return RC
*	@date 9/6/2015
*/
RC writeFromCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage, int countOfMemPage)
{
	RC isFHandleInit = checkIfFileHandleIsInitiated(fHandle);

	if (isFHandleInit != RC_OK)
	{
		return isFHandleInit;
	}

	printf(">> writeCurrentBlock() : fileName:%s", fHandle->fileName);

	int curPos = -1;
	int numberOfPages;
	int bitPos = fHandle->curPagePos;	//Assumption curPagePos contains the reference to the initial address of the page

	bitPos = (bitPos)*PAGE_SIZE;

	if (bitPos < 0) //to check if this is a valid page
	{
		return RC_WRITE_FAILED;
	}

	if ((countOfMemPage % PAGE_SIZE) == 0)
	{
		numberOfPages = (int*)(countOfMemPage / PAGE_SIZE);
	}
	else
	{
		numberOfPages = (int*)(countOfMemPage / PAGE_SIZE) + 1;
	}

	if ((fHandle->totalNumPages - fHandle->curPagePos + 1) < numberOfPages)
	{
		ensureCapacity(numberOfPages, fHandle);
	}

	if ((fseek(fHandle->mgmtInfo, bitPos, SEEK_SET)) < 0)
	{
		return RC_FILE_SEEK_ERROR;
	}

	if ((fwrite(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo)) == -1)
	{
		return RC_WRITE_FAILED;
	}

	printf("<< writeCurrentBlock() ");

	return RC_OK;
}

/**
* This initializes the File Handle to its default values
* @param SM_FileHandle
*
* @date 10/6/2015
* @returns void
* @author Pavan Rao
*/
void initFileHandle(SM_FileHandle *newFile)
{
	newFile->fileName = NULL;
	newFile->curPagePos = 0;
	newFile->totalNumPages = 0;
	newFile->mgmtInfo = NULL;
}