#include <stdlib.h>
#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"

//Defining assert equals
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

// test methods
static void testRecords(void);
static void testCreateTableAndInsert(void);
static void testUpdateTable(void);
static void testScans(void);
static void testScansTwo(void);
static void testInsertManyRecords(void);
static void testMultipleScans(void);

// struct for test records
typedef struct TestRecord {
	int a;
	char *b;
	int c;
} TestRecord;

// helper methods
Record *testRecord(Schema *schema, int a, char *b, int c);
Schema *testSchema(void);
Record *fromTestRecord(Schema *schema, TestRecord in);

// test name
char *testName;

// main method
int
main (void)
{
testName = "";

testInsertManyRecords();
testRecords();
testCreateTableAndInsert();
testUpdateTable();
testScans();
testScansTwo();
testMultipleScans();

return 0;
}

// ************************************************************ 
void
testRecords(void)
{
	TestRecord expected[] = {
		{ 1, "aaaa", 3 },
	};
	Schema *schema;
	Record *r;
	Value *value;
	testName = "test creating records and manipulating attributes";

	// check attributes of created record
	schema = testSchema();
	r = fromTestRecord(schema, expected[0]);

	getAttr(r, schema, 0, &value);
	OP_TRUE(stringToValue("i1"), value, valueEquals, "first attr");
	freeVal(value);

	getAttr(r, schema, 1, &value);
	OP_TRUE(stringToValue("saaaa"), value, valueEquals, "second attr");
	freeVal(value);

	getAttr(r, schema, 2, &value);
	OP_TRUE(stringToValue("i3"), value, valueEquals, "third attr");
	freeVal(value);

	//modify attrs
	setAttr(r, schema, 2, stringToValue("i4"));
	getAttr(r, schema, 2, &value);
	OP_TRUE(stringToValue("i4"), value, valueEquals, "third attr after setting");
	freeVal(value);

	freeRecord(r);
	TEST_DONE();
}

// ************************************************************ 
void
testCreateTableAndInsert(void)
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
		{ 9, "iiii", 2 }
	};
	int numInserts = 9, i;
	Record *r;
	RID *rids;
	Schema *schema;
	testName = "test creating a new table and inserting tuples";
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

	TEST_CHECK(closeTable(table));
	TEST_CHECK(openTable(table, "test_table_r"));

	// randomly retrieve records from the table and compare to inserted ones
	for (i = 0; i < 1000; i++)
	{
		int pos = rand() % numInserts;
		RID rid = rids[pos];
		TEST_CHECK(getRecord(table, rid, r));
		ASSERT_EQUALS_RECORDS(fromTestRecord(schema, inserts[pos]), r, schema, "compare records");
	}

	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_r"));
	TEST_CHECK(shutdownRecordManager());

	free(rids);
	free(table);
	TEST_DONE();
}

void
testMultipleScans(void)
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
	int numInserts = 10, i, scanOne = 0, scanTwo = 0;
	Record *r;
	RID *rids;
	Schema *schema;
	testName = "test running muliple scans ";
	schema = testSchema();
	rids = (RID *)malloc(sizeof(RID) * numInserts);
	RM_ScanHandle *sc1 = (RM_ScanHandle *)malloc(sizeof(RM_ScanHandle));
	RM_ScanHandle *sc2 = (RM_ScanHandle *)malloc(sizeof(RM_ScanHandle));
	Expr *se1, *left, *right;
	int rc, rc2;

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

	// Mix 2 scans with c=3 as condition
	MAKE_CONS(left, stringToValue("i3"));
	MAKE_ATTRREF(right, 2);
	MAKE_BINOP_EXPR(se1, left, right, OP_COMP_EQUAL);
	createRecord(&r, schema);
	TEST_CHECK(startScan(table, sc1, se1));
	TEST_CHECK(startScan(table, sc2, se1));
	if ((rc2 = next(sc2, r)) == RC_OK)
		scanTwo++;
	i = 0;
	while ((rc = next(sc1, r)) == RC_OK)
	{
		scanOne++;
		i++;
		if (i % 3 == 0)
			if ((rc2 = next(sc2, r)) == RC_OK)
				scanTwo++;
	}
	while ((rc2 = next(sc2, r)) == RC_OK)
		scanTwo++;

	ASSERT_TRUE(scanOne == scanTwo, "scans returned same number of tuples");
	if (rc != RC_RM_NO_MORE_TUPLES)
		TEST_CHECK(rc);
	TEST_CHECK(closeScan(sc1));
	TEST_CHECK(closeScan(sc2));

	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_r"));
	TEST_CHECK(shutdownRecordManager());

	free(rids);
	free(table);
	TEST_DONE();
}

void
testUpdateTable(void)
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
	TestRecord updates[] = {
		{ 1, "iiii", 6 },
		{ 2, "iiii", 6 },
		{ 3, "iiii", 6 }
	};
	int deletes[] = {
		9,
		6,
		7,
		8,
		5
	};
	TestRecord finalR[] = {
		{ 1, "iiii", 6 },
		{ 2, "iiii", 6 },
		{ 3, "iiii", 6 },
		{ 4, "dddd", 3 },
		{ 5, "eeee", 5 },
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

	// delete rows from table
	for (i = 0; i < numDeletes; i++)
	{
		TEST_CHECK(deleteRecord(table, rids[deletes[i]]));
	}

	// update rows into table
	for (i = 0; i < numUpdates; i++)
	{
		r = fromTestRecord(schema, updates[i]);
		r->id = rids[i];
		TEST_CHECK(updateRecord(table, r));
	}

	TEST_CHECK(closeTable(table));
	TEST_CHECK(openTable(table, "test_table_r"));

	// retrieve records from the table and compare to expected final stage
	for (i = 0; i < numFinal; i++)
	{
		RID rid = rids[i];
		TEST_CHECK(getRecord(table, rid, r));
		ASSERT_EQUALS_RECORDS(fromTestRecord(schema, finalR[i]), r, schema, "compare records");
	}

	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_r"));
	TEST_CHECK(shutdownRecordManager());

	free(table);
	TEST_DONE();
}

void
testInsertManyRecords(void)
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
	TestRecord realInserts[10000];
	TestRecord updates[] = {
		{ 3333, "iiii", 6 }
	};
	int numInserts = 10000, i;
	int randomRec = 3333;
	Record *r;
	RID *rids;
	Schema *schema;
	testName = "test creating a new table and inserting 10000 records then updating record from rids[3333]";
	schema = testSchema();
	rids = (RID *)malloc(sizeof(RID) * numInserts);

	TEST_CHECK(initRecordManager(NULL));
	TEST_CHECK(createTable("test_table_t", schema));
	TEST_CHECK(openTable(table, "test_table_t"));

	// insert rows into table
	for (i = 0; i < numInserts; i++)
	{
		realInserts[i] = inserts[i % 10];
		realInserts[i].a = i;
		r = fromTestRecord(schema, realInserts[i]);
		TEST_CHECK(insertRecord(table, r));
		rids[i] = r->id;
	}
	TEST_CHECK(closeTable(table));
	TEST_CHECK(openTable(table, "test_table_t"));

	int j;
	// retrieve records from the table and compare to expected final stage
	for (j = 0; j < numInserts; j++)
	{
		RID rid = rids[j];
		TEST_CHECK(getRecord(table, rid, r));
		ASSERT_EQUALS_RECORDS(fromTestRecord(schema, realInserts[j]), r, schema, "compare records");
	}

	r = fromTestRecord(schema, updates[0]);
	r->id = rids[randomRec];
	TEST_CHECK(updateRecord(table, r));
	TEST_CHECK(getRecord(table, rids[randomRec], r));
	ASSERT_EQUALS_RECORDS(fromTestRecord(schema, updates[0]), r, schema, "compare records");

	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_t"));
	TEST_CHECK(shutdownRecordManager());

	freeRecord(r);
	free(table);
	TEST_DONE();
}

void testScans(void)
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
	TestRecord scanOneResult[] = {
		{ 3, "cccc", 1 },
		{ 6, "ffff", 1 },
	};
	bool foundScan[] = {
		FALSE,
		FALSE
	};
	int numInserts = 10, scanSizeOne = 2, i;
	Record *r;
	RID *rids;
	Schema *schema;
	RM_ScanHandle *sc = (RM_ScanHandle *)malloc(sizeof(RM_ScanHandle));
	Expr *sel, *left, *right;
	int rc;

	testName = "test creating a new table and inserting tuples";
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

	TEST_CHECK(closeTable(table));
	TEST_CHECK(openTable(table, "test_table_r"));

	// run some scans
	MAKE_CONS(left, stringToValue("i1"));
	MAKE_ATTRREF(right, 2);
	MAKE_BINOP_EXPR(sel, left, right, OP_COMP_EQUAL);

	TEST_CHECK(startScan(table, sc, sel));
	while ((rc = next(sc, r)) == RC_OK)
	{
		for (i = 0; i < scanSizeOne; i++)
		{
			if (memcmp(fromTestRecord(schema, scanOneResult[i])->data, r->data, getRecordSize(schema)) == 0)
				foundScan[i] = TRUE;
		}
	}
	if (rc != RC_RM_NO_MORE_TUPLES)
		TEST_CHECK(rc);
	TEST_CHECK(closeScan(sc));
	for (i = 0; i < scanSizeOne; i++)
		ASSERT_TRUE(foundScan[i], "check for scan result");

	// clean up
	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_r"));
	TEST_CHECK(shutdownRecordManager());

	free(table);
	free(sc);
	freeExpr(sel);
	TEST_DONE();
}


void testScansTwo(void)
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
	bool foundScan[] = {
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE
	};
	int numInserts = 10, i;
	Record *r;
	RID *rids;
	Schema *schema;
	RM_ScanHandle *sc = (RM_ScanHandle *)malloc(sizeof(RM_ScanHandle));
	Expr *sel, *left, *right, *first, *se;
	int rc;

	testName = "test creating a new table and inserting tuples";
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

	TEST_CHECK(closeTable(table));
	TEST_CHECK(openTable(table, "test_table_r"));

	// Select 1 record with INT in condition a=2.
	MAKE_CONS(left, stringToValue("i2"));
	MAKE_ATTRREF(right, 0);
	MAKE_BINOP_EXPR(sel, left, right, OP_COMP_EQUAL);
	createRecord(&r, schema);
	TEST_CHECK(startScan(table, sc, sel));
	while ((rc = next(sc, r)) == RC_OK)
	{
		ASSERT_EQUALS_RECORDS(fromTestRecord(schema, inserts[1]), r, schema, "compare records");
	}
	if (rc != RC_RM_NO_MORE_TUPLES)
		TEST_CHECK(rc);
	TEST_CHECK(closeScan(sc));

	// Select 1 record with STRING in condition b='ffff'.
	MAKE_CONS(left, stringToValue("sffff"));
	MAKE_ATTRREF(right, 1);
	MAKE_BINOP_EXPR(sel, left, right, OP_COMP_EQUAL);
	createRecord(&r, schema);
	TEST_CHECK(startScan(table, sc, sel));
	while ((rc = next(sc, r)) == RC_OK)
	{
		ASSERT_EQUALS_RECORDS(fromTestRecord(schema, inserts[5]), r, schema, "compare records");
		serializeRecord(r, schema);
	}
	if (rc != RC_RM_NO_MORE_TUPLES)
		TEST_CHECK(rc);
	TEST_CHECK(closeScan(sc));

	// Select all records, with condition being false
	MAKE_CONS(left, stringToValue("i4"));
	MAKE_ATTRREF(right, 2);
	MAKE_BINOP_EXPR(first, right, left, OP_COMP_SMALLER);
	MAKE_UNOP_EXPR(se, first, OP_BOOL_NOT);
	TEST_CHECK(startScan(table, sc, se));
	while ((rc = next(sc, r)) == RC_OK)
	{
		serializeRecord(r, schema);
		for (i = 0; i < numInserts; i++)
		{
			if (memcmp(fromTestRecord(schema, inserts[i])->data, r->data, getRecordSize(schema)) == 0)
				foundScan[i] = TRUE;
		}
	}
	if (rc != RC_RM_NO_MORE_TUPLES)
		TEST_CHECK(rc);
	TEST_CHECK(closeScan(sc));

	ASSERT_TRUE(!foundScan[0], "not greater than four");
	ASSERT_TRUE(foundScan[4], "greater than four");
	ASSERT_TRUE(foundScan[9], "greater than four");

	// clean up
	TEST_CHECK(closeTable(table));
	TEST_CHECK(deleteTable("test_table_r"));
	TEST_CHECK(shutdownRecordManager());

	freeRecord(r);
	free(table);
	free(sc);
	freeExpr(sel);
	TEST_DONE();
}


Schema *
testSchema(void)
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
