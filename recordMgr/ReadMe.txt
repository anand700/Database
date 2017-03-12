*******************************************************************
Record Manager - Assignment 3
*******************************************************************

*******************************************************************
Introduction
*******************************************************************
Record Manager is an application that handles tables with a fixed schema where clients can insert records, delete records, update records, and scan through the records in a table.
We have implemented optional extension Tids and tombstone in our project.

*******************************************************************
Data Structure
*******************************************************************
RM_TABLE_INFO:
	s - a pointer that contains the information about the schema.
	totalNumOfRec - It denotes the total number of records in the table.
	firstFreeRec - a pointer to the first free record in the table.
	LastRecID - It denotes the last record id in the table.

PAGE_INFO:
	firstDeletedRecord - It denotes the first deleted record in the page.
	lastRecord - It denotes the last record in the page.

ExpressionHandler:
	cond - It denotes the condition on which scan is started.
	ridToScan - It denotes the record in which the tuples have to scanned.
	lastRecID - It denotes the last record id in the table.
	
	
*******************************************************************
Method description
*******************************************************************
Our implementation approach towards above functionalities are as follows

initRecordManager(void *mgmtData)
- Initializes the Record Manager.

shutdownRecordManager()
- Shutdown the Record Manager.

createTable(char *name, Schema *schema)
- Create a PageFile, Initialize the BufferPool, Pin the page into Bufferpool, Serialize the table information and mark the page dirty.

createTableInfo(RM_TABLE_INFO *tableData)
- Initialize the parameters of RM_Table_Info structure.
- Returns RC_OK on success.

serializeTableInformation(char *name, RM_TABLE_INFO *r, char *res)
- Serializes the infomation of the schema into a string which can be stored in a file.
- Returns RC_OK on success else a failure code.

deSerializeTableInformation(char *page, RM_TABLE_INFO *infoPage)
- Deserializes the infomation of the schema and table from the table information page into a RM_TABLE_INFO datastructure.
- Returns RC_OK on success else a failure code.

openTable(RM_TableData *rel, char *name)
- Initialize the BufferPool, Pin the page into Bufferpool and mark the page dirty.
- Deserialize the table data.
- Returns RC_OK on success else a failure code.

closeTable(RM_TableData *rel)
- Free the Schema.
- Shutdown the BufferPool associated with the table.
- Returns RC_OK on success else a failure code.

deleteTable(char *name)
- Destroy the page file.
- Returns RC_OK on success else a failure code.

getNumTuples(RM_TableData *rel)
- Deserialize the table information to get the number of blocks.
- Traverse through the blocks to obtain the total number of tuples.

insertRecord(RM_TableData *rel, Record *record)
- Inserts the record in the first empty slot available.
- Initialize the BufferPool, Pin the page into Bufferpool and deserialize the table information.
- Traverse through the table and identify RID of the last record of the table.
- Make sure that there is enough space in the current page for placing the next record.
- If there is a free space available call a function to deserialize & obtain the RID of next Free rec.
- Update the table info page and pageheader.
- Mark the page dirty, table info page dirty using the serializeTableInformation and unpin the page.
- Returns RC_OK on success else a failure code.

updateHeaderOfThePage(char *ph, PAGE_INFO *pageHeader)
- Updates the page header with the new header values.
- serialize the info that is to be inserted in the header.
- Returns RC_OK on success.

serializePageHeader(PAGE_INFO *pageHeader, char *temp)
- Serializes the page header, the info that is to be inserted in the header.
- Returns RC_OK on success.

extractPageHeader(char *ph, PAGE_INFO *pageHeader)
- Extracts the header from the pageFrame to get the info about first deleted record and last record page and slot.
- Returns RC_OK on success.

deleteRecord(RM_TableData *rel, RID id)
- Pin the page into BufferPool, Identify the Slot using the record length.
- Implement the tombstone concept to delete record and free the space.
- Mark the page dirty and unpin the page.
- Returns RC_OK on success else a failure code.

serializaeEmptyRec(char *data, RID *rid)
- serializes the RID and store it in the data of the empty rec.
- Input Record data & RID.
- Return RC_OK on success.

deSerializaeEmptyRec(char *data, RID *rid)
- Deserializes the empty rec data which contains the RID of the next free Rec which can either a valid RID or (-1,-1).
- Input recordData containing the comma seperated RID of the next free space & RID parameter which should be updated with the RID value retrieved from the previous parameter.
- Return RC_OK on success.

updateRecord(RM_TableData *rel, Record *record)
- Pin the page into BufferPool, Identify the Slot using the record length.
- Update the Slot identified with the new Record data.
- Mark the page dirty and unpin the page.
- Returns RC_OK on success else a failure code.

getRecord(RM_TableData *rel, RID id, Record *record)
- Pin the page into BufferPool, Identify the Slot using the record length and using the tombstone concept.
- Update the the record data with the identified page data.
- Mark the page dirty and unpin the page.
- Returns RC_OK on success else a failure code.

startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
- Initialize the Scan handle based on the condition represented by Expression handler.
- Pin the page and deserialize the table information.
- Returns RC_OK on success else a failure code.

next(RM_ScanHandle *scan, Record *record)
- Returns the next tuple that fulfills the scan condition.
- Initialize the Scan handle based on the condition represented by Expression handler.
- Using the Expression handle scan through the record until the search satisfies the condition and break once the data is retrieved.
- Returns RC_OK on success and RC_RM_NO_MORE_TUPLES once the scan is completed.

closeScan(RM_ScanHandle *scan)
- Free memory associated with the Scan Handle and return RC_OK.

getRecordSize(Schema *schema)
- For each of the attributes identified from the schema we find the corresponding datatype and sum the size occupied by it in bytes.
- Return the total size occupied by all the attribute datatypes in byte.

createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
- Allocate the memory and Initialize the values for all the schema attributes being passed.
- Return the newly created schema.

freeSchema(Schema *schema)
- Free memory associated with the Schema and return RC_OK.

createRecord(Record **record, Schema *schema)
- Calculate the size of the record from the schema information and allocate the memory to the record.

freeRecord(Record *record)
- Free memory associated with the Record and return RC_OK.

getAttr(Record *record, Schema *schema, int attrNum, Value **value)
- Checks if attrNum is valid.
- Based on the arrtNum copy the record data to the attribute data and assign value for the datatype.
- Return RC_OK.

setAttr(Record *record, Schema *schema, int attrNum, Value *value)
- Checks if attrNum is valid.
- Based on the arrtNum aasign the attribute data for datatype and copy the attribute data to the record data.
- Return RC_OK.

checkIfAttrNumIsValid(attrNum)
- checks if attribute number is valid.

assignValueForDataType(Schema *schema, int attrNum, Value **value, char *attrData)
- Assign value for a specific datatype depending on the type of schema.
- Return RC_OK.

assignAttrDataForDataType(Schema *schema, int attrNum, Value **value, char *attrData)
- Assign record data for a specific datatype depending on the type of schema.
- Return RC_OK.

extractPageHeader(char *ph, PAGE_INFO *pageHeader)
- Extracts the header from the pageFrame.
- Input Page Frame which contains the header and the records of the Page & empty PAGE_INFO which is to be popullated with the values extracted from the pageFrame.
- Return RC_OK on success.

previousDeletedRecordId(RM_ScanHandle *scan, Record *record)
- Returns the RID of the previous deleted record.
- RID.slot=-1 and RID.page=-1 implies there is no previousRID.
- Return RC_OK on success.

getPreviousAndNextDeletedRecordId(RM_TableData *rel, RID id, RID *prevResultRid, RID *nextResultRid)
- Returns the RID of the previous and Next deleted record.
- RID.slot=-1 and RID.page=-1 implies there is no previousRID.
- Input RM_TableData, RID rid of the record to be deleted, RID prev result Rid, RID next result Rid & Record
- Return RC_OK on success.

getNextRID(Record *rec, BM_BufferPool *bm, BM_PageHandle *ph)
- Function that returns the next record's ID.



*********************************************************************
PROCEDURE TO RUN THE PROJECT
*********************************************************************
There are two makefiles in the project (one for test_assign3_1.c and one for test_assign3_2.c).

Steps to run the makefiles are as follows:

FIRST MAKEFILE (Makefile1):
1) Open the Linux terminal, navigate to the directory with assignment contents using the command 'cd'
2) Execute the make file as below: 
		make -f Makefile1

			
SECOND MAKEFILE (Makefile2):
1) Open the Linux terminal, navigate to the directory with assignment contents using the command 'cd'
2) Execute the make file as below: 
		make -f Makefile2




