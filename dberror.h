#ifndef DBERROR_H
#define DBERROR_H

#include "stdio.h"

/* module wide constants */
#define PAGE_SIZE 4096

/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_FILE_HANDLE_NOT_INIT 2
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4
#define RC_EMPTY_FILE 5
#define RC_FILE_SEEK_ERROR 6
#define RC_READ_FAILED 7
#define RC_INVALID_FILE_NAME 8
#define RC_INVALID_PAGE_NUM 9
#define RC_CANNOT_WRITE_MORE_THAN_ONE_BLOCK 10

#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN 201
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_RM_NO_MORE_TUPLES 203
#define RC_RM_NO_PRINT_FOR_DATATYPE 204
#define RC_RM_UNKOWN_DATATYPE 205

#define RC_IM_KEY_NOT_FOUND 300
#define RC_IM_KEY_ALREADY_EXISTS 301
#define RC_IM_N_TO_LAGE 302
#define RC_IM_NO_MORE_ENTRIES 303

// custom Return Codes
#define RC_FILE_ALREADY_IN_USE 401
#define RC_PAGE_DIRTY 402
#define RC_BM_BUFFERPOOL_NOT_INIT 403
#define RC_BUFFER_PAGE_NOT_FOUND 404
#define RC_ENSURE_CAPACITY_FAILED 405
#define RC_BUFFER_PAGE_IN_USE 406
#define RC_BUFFER_POOL_EMPTY 407
#define RC_LRU_FAIL 408
#define RC_CANNOT_CREATE_FILE 409
#define RC_PIN_PAGE_FAILED 410
#define RC_MAKE_DIRTY_FAILED 411
#define RC_NAME_NOT_VALID 412
#define RC_INVALID_NUM_ATTR 413
#define RC_INVALID_ATTR_NAMES 414
#define RC_INVALID_DATATYPE 415
#define	RC_INVALID_TYPE_LENGTH 416
#define RC_INVALID_KEY_SIZE 417
#define RC_INFO_PAGE_TOO_BIG 418
#define RC_INVALID_ATTRIBUTE_NUM 419
#define RC_INSERT_FAILED 420
#define RC_PAGE_HEADER_SIZE_OVERFLOW 421
#define RC_UPDATE_HEADER_FAILED 422
#define RC_FIFO_INSERT_FAILED 423
#define RC_NOT_A_TOMBSTONE 424
#define RC_INVALID_PAGE_OR_SLOT 425
#define RC_NOMEM 426
#define RC_DELETE_BTREE_FAILED 427

/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError(RC error);
extern char *errorMessage(RC error);

#define THROW(rc,message) \
  do {			  \
    RC_message=message;	  \
    return rc;		  \
  } while (0)		  \

// check the return code and exit if it is an error
#define CHECK(code)							\
  do {									\
    int rc_internal = (code);						\
    if (rc_internal != RC_OK)						\
      {									\
	char *message = errorMessage(rc_internal);			\
	printf("[%s-L%i-%s] ERROR: Operation returned error: %s\n",__FILE__, __LINE__, __TIME__, message); \
	free(message);							\
	exit(1);							\
      }									\
  } while(0);


#endif
