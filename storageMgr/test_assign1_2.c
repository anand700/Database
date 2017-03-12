#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

//
//extern RC createPageFile(char *fileName);

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"
#define TESTPFONE "test_pagefile1.bin"
#define TESTPFTWO "test_pagefile2.bin"

/* prototypes for test functions */
static void testTwoFilesOpenFunctionWithSameFH(void);
static void testTwoFilesOpenFunctionWithDifferentFH(void);
static void testMethodsForNullValues(void);
static void testOpenFileIfFileAlreadyInUse(void);
static void testReadBlockForPageNumberNotExisting(void);
static void testReadNextBlockWhenNextBlockIsAbsent(void);
static void testWriteFromBlock(void);
static void testWriteFromCurrentBlock(void);

/* main function running all tests */
int
main(void)
{
	testName = "";
	testTwoFilesOpenFunctionWithSameFH();
	testTwoFilesOpenFunctionWithDifferentFH();
	testMethodsForNullValues();
	testOpenFileIfFileAlreadyInUse();
	testReadBlockForPageNumberNotExisting();
	testReadNextBlockWhenNextBlockIsAbsent();
	testWriteFromBlock();
	testWriteFromCurrentBlock();
	return 0;
}


/* Try to open two files with file handle
*	information already populated
*/
void testTwoFilesOpenFunctionWithSameFH(void)
{
	printf(">> testTwoFilesOpenFunctionWithSameFH() ");
	SM_FileHandle sfh1 = { NULL, 0, 0, NULL };

	// create a new page file
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh1));
	printf("created file :", TESTPFONE);

	// create a another page file
	TEST_CHECK(createPageFile(TESTPFTWO));
	// after creating TESTPFONE trying to open another file(TESTPFTWO) with filehandle
	// having information of TESTPFONE file should cause an error
	ASSERT_TRUE((openPageFile(TESTPFTWO, &sfh1) == RC_FILE_ALREADY_IN_USE), "File already in use. Create another file handle.");

	TEST_CHECK(closePageFile(&sfh1));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	//need not close file since file handle is not populated.
	TEST_CHECK(destroyPageFile(TESTPFTWO));

	printf("<< testTwoFilesOpenFunctionWithSameFH() ");
	TEST_DONE();
}

/* Try to open two files with different
*	file handle
*/
void testTwoFilesOpenFunctionWithDifferentFH(void)
{
	printf(">> testTwoFilesOpenFunctionWithDifferentFH() ");
	SM_FileHandle sfh1 = { NULL, 0, 0, NULL };
	SM_FileHandle sfh2 = { NULL, 0, 0, NULL };

	// create a new page file.
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh1));
	printf("created file :", TESTPFONE);

	// create a another page file.
	TEST_CHECK(createPageFile(TESTPFTWO));
	// after creating TESTPFONE trying to open another file(TESTPFTWO) with filehandle
	// having information of TESTPFONE file should cause an error.
	ASSERT_TRUE((openPageFile(TESTPFTWO, &sfh2) == RC_OK), "File two successfully opened.");

	TEST_CHECK(closePageFile(&sfh1));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	TEST_CHECK(closePageFile(&sfh2));
	TEST_CHECK(destroyPageFile(TESTPFTWO));

	printf("<< testTwoFilesOpenFunctionWithDifferentFH() ");
	TEST_DONE();
}


/* Try to open two files with different
*	file handle
*/
void testMethodsForNullValues(void)
{
	printf(">> testMethodsForNullValues() ");
	SM_FileHandle sfh = { NULL, 0, 0, NULL };
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(PAGE_SIZE);

	ASSERT_TRUE((createPageFile(NULL) == RC_INVALID_FILE_NAME), "Invalid File Name.");
	ASSERT_TRUE((openPageFile(NULL, &sfh) == RC_INVALID_FILE_NAME), "Invalid File Name.");
	ASSERT_TRUE((closePageFile(NULL) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((destroyPageFile(NULL) == RC_INVALID_FILE_NAME), "Invalid File Name.");
	ASSERT_TRUE((readBlock(0, NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((getBlockPos(NULL) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((readFirstBlock(NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((readPreviousBlock(NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((readCurrentBlock(NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((readNextBlock(NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((readLastBlock(NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((writeBlock(0, NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((writeCurrentBlock(NULL, ph) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((appendEmptyBlock(NULL) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");
	ASSERT_TRUE((ensureCapacity(0, &sfh) == RC_FILE_HANDLE_NOT_INIT), "File handle not initiated.");

	printf("<< testMethodsForNullValues() ");
	TEST_DONE();
}

/* Try to open file which is already in use
*	
*/
void testOpenFileIfFileAlreadyInUse(void)
{
	printf(">> testOpenFileIfFileAlreadyInUse() ");
	
	SM_FileHandle sfh = { NULL, 0, 0, NULL };

	// create a new page file.
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh));

	ASSERT_TRUE((openPageFile(TESTPFONE, &sfh) == RC_FILE_ALREADY_IN_USE), "File already in use.");

	TEST_CHECK(closePageFile(&sfh));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	printf("<< testOpenFileIfFileAlreadyInUse() ");
	TEST_DONE();
}

/* Try to read next block when next block is not present
*
*/
void testReadNextBlockWhenNextBlockIsAbsent(void)
{
	printf(">> testReadNextBlockWhenNextBlockIsAbsent() ");

	SM_FileHandle sfh = { NULL, 0, 0, NULL };
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(PAGE_SIZE);
	int i;

	// create a new page file.
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh));

	// change ph to be a string and write that one to disk
	for (i = 0; i < PAGE_SIZE; i++) {
		ph[i] = (i % 10) + '0';
	}
	TEST_CHECK(writeBlock(0, &sfh, ph));
	ASSERT_TRUE((readNextBlock(&sfh, ph) == RC_READ_NON_EXISTING_PAGE), "Read Non existing page.");

	TEST_CHECK(closePageFile(&sfh));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	printf("<< testReadNextBlockWhenNextBlockIsAbsent() ");
	TEST_DONE();
}

/* Try to read next block when next block is not present
*
*/
void testReadBlockForPageNumberNotExisting(void)
{
	printf(">> testReadBlockForPageNumberNotExisting() ");

	SM_FileHandle sfh = { NULL, 0, 0, NULL };
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(PAGE_SIZE);
	int i;

	// create a new page file.
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh));

	// change ph to be a string and write that one to disk
	for (i = 0; i < PAGE_SIZE ; i++) {
		ph[i] = (i % 10) + '0';
	}
	TEST_CHECK(writeBlock(0, &sfh, ph));
	ASSERT_TRUE((readBlock(20,&sfh, ph) == RC_READ_NON_EXISTING_PAGE), "Read Non existing page.");

	TEST_CHECK(closePageFile(&sfh));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	printf("<< testReadBlockForPageNumberNotExisting() ");
	TEST_DONE();
}

/**
*	tests the functionality of the write next block
*/
void testWriteFromBlock(void)
{
	printf(">> testWriteFromBlock() ");

	SM_FileHandle sfh = { NULL, 0, 0, NULL };
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(PAGE_SIZE*3);
	int i;

	// create a new page file.
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh));

	// change ph to be a string and write that one to disk
	for (i = 0; i < PAGE_SIZE*3; i++)
	{
		ph[i] = (i % 10) + '0';
	}
	TEST_CHECK(writeFromBlock(0, &sfh, ph, i));

	TEST_CHECK(closePageFile(&sfh));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	printf("<< testWriteFromBlock() ");
	TEST_DONE();
}

/**
*	tests the functionality of the write next block
*/
void testWriteFromCurrentBlock(void)
{
	printf(">> testWriteFromCurrentBlock() ");

	SM_FileHandle sfh = { NULL, 0, 0, NULL };
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(PAGE_SIZE*3);
	int i;

	// create a new page file.
	TEST_CHECK(createPageFile(TESTPFONE));
	TEST_CHECK(openPageFile(TESTPFONE, &sfh));

	// change ph to be a string and write that one to disk
	for (i = 0; i < PAGE_SIZE * 3; i++)
	{
		ph[i] = (i % 10) + '0';
	}
	TEST_CHECK(writeFromCurrentBlock(&sfh, ph, i));

	TEST_CHECK(closePageFile(&sfh));
	TEST_CHECK(destroyPageFile(TESTPFONE));

	printf("<< testWriteFromCurrentBlock() ");
	TEST_DONE();
}
