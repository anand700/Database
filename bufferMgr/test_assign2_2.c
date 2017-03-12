#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "test_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// var to store the current test's name
char *testName;

// check whether two the content of a buffer pool is the same as an expected content 
// (given in the format produced by sprintPoolContent)
#define ASSERT_EQUALS_POOL(expected,bm,message)			        \
  do {									\
    char *real;								\
    char *_exp = (char *) (expected);                                   \
    real = sprintPoolContent(bm);					\
    if (strcmp((_exp),real) != 0)					\
		      {									\
	printf("[%s-%s-L%i-%s] FAILED: expected <%s> but was <%s>: %s\n",TEST_INFO, _exp, real, message); \
	free(real);							\
	exit(1);							\
		      }									\
    printf("[%s-%s-L%i-%s] OK: expected <%s> and was <%s>: %s\n",TEST_INFO, _exp, real, message); \
    free(real);								\
      } while(0)

// test and helper methods
static void testBufferMgrMethodsForNullValues(void);
static void testIfUnipinPageNotInBufferPool(void);
static void testFunctionsWhenPoolIsEmpty(void);
static void testShutDownBufferPoolWhenPageIsDirty(void);


// main method
int
main(void)
{
	initStorageManager();

	testBufferMgrMethodsForNullValues();
	testIfUnipinPageNotInBufferPool();
	testFunctionsWhenPoolIsEmpty();
	testShutDownBufferPoolWhenPageIsDirty();
}

/* Test methods for null values
*	void
*/
void testBufferMgrMethodsForNullValues(void)
{
	printf(">> testBufferMgrMethodsForNullValues() ");

	ASSERT_TRUE((forceFlushPool(NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((forcePage(NULL, NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((getDirtyFlags(NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((FIFOInsert(NULL, NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((LRUInsert(NULL, NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((shutdownBufferPool(NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((unpinPage(NULL, NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((getFrameContents(NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((pinPage(NULL, NULL, 1) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");
	ASSERT_TRUE((markDirty(NULL, NULL) == RC_BM_BUFFERPOOL_NOT_INIT), "Buffer Pool not initiated.");

	printf("<< testBufferMgrMethodsForNullValues() ");
	TEST_DONE();
}

/* Test methods If we are Unipining Page which is not in buffer pool
*	void
*/
void testIfUnipinPageNotInBufferPool(void)
{
	printf(">> testIfUnipinPageNotInBufferPool() ");

	BM_BufferPool *const bm = MAKE_POOL();
	int i;
	BM_PageHandle *h = MAKE_PAGE_HANDLE();
	BM_PageHandle *invalidPage = MAKE_PAGE_HANDLE();
	invalidPage->pageNum = 999999999999999;//Assuming this pagenum is invalid
	CHECK(createPageFile("testbuffer9999.bin"));
	CHECK(initBufferPool(bm, "testbuffer9999.bin", 3, RS_FIFO, NULL));
	for (i = 0; i < 5; i++)
	{
		CHECK(pinPage(bm, h, i));
	}
	ASSERT_TRUE((unpinPage(bm, invalidPage) == RC_INVALID_PAGE_NUM), "Cannot unpin page for invalid page number.");

	CHECK(shutdownBufferPool(bm));
	CHECK(destroyPageFile("testbuffer9999.bin"));
	free(bm);
	free(h);
	free(invalidPage);
	printf("<< testIfUnipinPageNotInBufferPool() ");
	TEST_DONE();
}

/* Test specific functions When Pool Is Empty
*	void
*/
void testFunctionsWhenPoolIsEmpty(void)
{
	printf(">> testFunctionsWhenPoolIsEmpty() ");

	BM_BufferPool *const bm = MAKE_POOL();
	int i;
	BM_PageHandle *h = MAKE_PAGE_HANDLE();
	CHECK(createPageFile("testbuffer99999.bin"));
	CHECK(initBufferPool(bm, "testbuffer99999.bin", 3, RS_FIFO, NULL));

	ASSERT_TRUE((forceFlushPool(bm) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");
	ASSERT_TRUE((forcePage(bm, h) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");
	ASSERT_TRUE((getDirtyFlags(bm) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");
	ASSERT_TRUE((FIFOInsert(NULL, bm) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");
	ASSERT_TRUE((unpinPage(bm, h) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");
	ASSERT_TRUE((getFrameContents(bm) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");
	ASSERT_TRUE((markDirty(bm, NULL) == RC_BUFFER_POOL_EMPTY), "Buffer Pool is empty.");

	CHECK(shutdownBufferPool(bm));
	CHECK(destroyPageFile("testbuffer99999.bin"));
	free(bm);
	free(h);
	printf("<< testFunctionsWhenPoolIsEmpty() ");
	TEST_DONE();
}

/* test ShutDownBufferPool When Page Is Dirty
*	void
*/
void testShutDownBufferPoolWhenPageIsDirty(void)
{
	printf(">> testShutDownBufferPoolWhenPageIsDirty() ");

	BM_BufferPool *bm = MAKE_POOL();
	BM_PageHandle *h = MAKE_PAGE_HANDLE();
	testName = "Reading a page";

	CHECK(createPageFile("testbuffer.bin"));
	CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL));
	CHECK(pinPage(bm, h, 0));
	CHECK(markDirty(bm, h));
	ASSERT_TRUE((shutdownBufferPool(bm) == RC_OK), "Successfully shutdown buffer pool after force page.");
	CHECK(destroyPageFile("testbuffer.bin"));
	free(bm);
	free(h);

	printf("<< testShutDownBufferPoolWhenPageIsDirty() ");
	TEST_DONE();
}
