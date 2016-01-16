#include <stdlib.h>
#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"
#include "dt.h"
#include<stdio.h>
#include<string.h>

#define ASSERT_EQUALS_RECORDS(_l,_r, schema, message)			\
  do {									\
    Record *_lR = _l;                                                   \
    Record *_rR = _r;                                                   \
    ASSERT_TRUE(memcmp(_lR->data,_rR->data,getRecordSize(schema)) == 0, message); \
    int i;								\
    for(i = 0; i < schema->numAttr; i++)				\
      {									\
        Value *lVal, *rVal;                                             \
		char *lSer, *rSer; \
        getAttr(_lR, schema, i, &lVal);                                  \
        getAttr(_rR, schema, i, &rVal);                                  \
		lSer = serializeValue(lVal); \
		rSer = serializeValue(rVal); \
        ASSERT_EQUALS_STRING(lSer, rSer, "attr same");	\
		free(lVal); \
		free(rVal); \
		free(lSer); \
		free(rSer); \
      }									\
  } while(0)

#define ASSERT_EQUALS_RECORD_IN(_l,_r, rSize, schema, message)		\
  do {									\
    int i;								\
    boolean found = false;						\
    for(i = 0; i < rSize; i++)						\
      if (memcmp(_l->data,_r[i]->data,getRecordSize(schema)) == 0)	\
	found = true;							\
    ASSERT_TRUE(0, message);						\
  } while(0)

#define OP_TRUE(left, right, op, message)		\
  do {							\
    Value *result = (Value *) malloc(sizeof(Value));	\
    op(left, right, result);				\
    bool b = result->v.boolV;				\
    free(result);					\
    ASSERT_TRUE(b,message);				\
   } while (0)


// struct for test records
typedef struct TestRecord {
	int a;
	char *b;
	int c;
} TestRecord;

// helper methods
Record *testRecord(Schema *schema, int a, char *b, int c);
Record *fromTestRecord(Schema *schema, TestRecord in);

// test name
char *testName;




Schema *testSchema(void)
{
	Schema *result;
	char *names[] = { "a", "b", "c" };
	DataType dt[] = { DT_INT, DT_STRING, DT_INT };
	int sizes[] = { 0, 4, 0 };
	int keys[] = { 0 };
	int i;
	char **cpNames = (char **)malloc(sizeof(char*) * 3);
	DataType *cpDt = (DataType *)malloc(sizeof(DataType) * 3);
	int *cpSizes = (int *)malloc(sizeof(int) * 3);
	int *cpKeys = (int *)malloc(sizeof(int));

	for (i = 0; i < 3; i++)
	{
		cpNames[i] = (char *)malloc(2);
		strcpy(cpNames[i], names[i]);
	}
	memcpy(cpDt, dt, sizeof(DataType) * 3);
	memcpy(cpSizes, sizes, sizeof(int) * 3);
	memcpy(cpKeys, keys, sizeof(int));

	result = createSchema(3, cpNames, cpDt, cpSizes, 1, cpKeys);

	return result;
}

bool checkEqual(RM_TABLE_INFO *a, RM_TABLE_INFO *b)
{
	bool valid = TRUE;
	int i;
	valid = a->s->numAttr == b->s->numAttr ? TRUE : FALSE;
	valid = strcmp(a->s->attrNames, b->s->attrNames) == 0 ? TRUE : FALSE;
	for (i = 0; i < a->s->numAttr; i++)
	{
		if (strcmp(a->s->attrNames[i], b->s->attrNames[i]) != 0)
			valid = FALSE;
	}
	for (i = 0; i < a->s->numAttr; i++)
	{
		if (a->s->dataTypes[i] != b->s->dataTypes[i])
			valid = FALSE;
	}
	for (i = 0; i < a->s->numAttr; i++)
	{
		if (a->s->typeLength[i] != b->s->typeLength[i])
			valid = FALSE;
	}
	valid = a->s->keySize == b->s->keySize ? TRUE : FALSE;
	for (i = 0; i < a->s->keySize; i++)
	{
		if (a->s->keyAttrs[i] != b->s->keyAttrs[i])
			valid = FALSE;
	}

	valid = a->firstFreeRec->page == a->firstFreeRec->page ? TRUE : FALSE;
	valid = a->firstFreeRec->slot == b->firstFreeRec->slot ? TRUE : FALSE;
	valid = a->totalNumOfRec == b->totalNumOfRec ? TRUE : FALSE;

	return valid;
}

RC testTombstone(void)
{
	RM_TableData *table = (RM_TableData *)malloc(sizeof(RM_TableData));
	TestRecord inserts[] = {
		{ 1, "aaaa", 3 },
		{ 2, "bbbb", 2 },
		{ 3, "cccc", 1 },
		{ 4, "dddd", 3 },
		{ 5, "eeee", 5 },
		{ 6, "ffff", 1 },
		{ 7, "gggg", 3 },
		{ 8, "hhhh", 3 },
		{ 9, "iiii", 2 },
		{ 10, "jjjj", 5 },
	};
	int numInserts = 10, numUpdates = 3, numDeletes = 5, numFinal = 5, i;
	Record *r;
	RID *rids;
	Schema *schema;
	testName = "test creating a new table and insert,update,delete tuples";
	schema = testSchema();
	rids = (RID *)malloc(sizeof(RID) * numInserts);

	TEST_CHECK(initRecordManager(NULL));
	TEST_CHECK(createTable("test_table_r", schema));
	TEST_CHECK(openTable(table, "test_table_r"));

	// insert rows into table
	for (i = 0; i < numInserts; i++)
	{
		r = fromTestRecord(schema, inserts[i]);
		TEST_CHECK(insertRecord(table, r));
		rids[i] = r->id;
	}

	TEST_CHECK(deleteRecord(table, rids[9]));
	RID id;
	id.page = rids[9].page;
	id.slot = rids[10].slot;
	int isTombstone = checkIfTombstoneEncountered(table, rids[9]);

	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_r"));
	TEST_CHECK(shutdownRecordManager());

	free(table);
	if (isTombstone == 1) {
		return RC_OK;
	}
	else {
		return RC_NOT_A_TOMBSTONE;
	}

}

Record *
fromTestRecord(Schema *schema, TestRecord in)
{
	return testRecord(schema, in.a, in.b, in.c);
}

Record *
testRecord(Schema *schema, int a, char *b, int c)
{
	Record *result;
	Value *value;

	TEST_CHECK(createRecord(&result, schema));

	MAKE_VALUE(value, DT_INT, a);
	TEST_CHECK(setAttr(result, schema, 0, value));
	freeVal(value);

	MAKE_STRING_VALUE(value, b);
	TEST_CHECK(setAttr(result, schema, 1, value));
	freeVal(value);

	MAKE_VALUE(value, DT_INT, c);
	TEST_CHECK(setAttr(result, schema, 2, value));
	freeVal(value);

	return result;
}

void main()
{
	Schema *temp;
	char *s = (char *)malloc(100), **c = (char *)malloc(100 * 10);
	temp = testSchema();
	char *res = (char *)malloc(1000);
	RM_TABLE_INFO *newTable = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO)), *outPutTable = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));
	RID *newRID = (RID *)malloc(sizeof(RID));
	newRID->page = 1;
	newRID->slot = 1;
	newTable->s = temp;
	newTable->firstFreeRec = newRID;
	serializeTableInformation("testTable", newTable, res);
	printf("%s\n", res);
	deSerializeTableInformation(res, outPutTable);
	if (checkEqual(newTable, outPutTable) == TRUE)
		printf("successful serialization and deserialization\n");
	else
		printf("unsuccessful serialization and deserialization\n");
	free(res);
	free(newRID);
	free(newTable);

	if (testTombstone() == RC_OK) {
		printf("successful implementation of tombstone\n");
	}
	TEST_DONE();
}
