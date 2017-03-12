#include"record_mgr.h"
#include"buffer_mgr.h"
#include"storage_mgr.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PAGE_INFO_SIZE 50
#define TOMBSTONE_SIZE sizeof(bool)


/**
* This creates the schema by allocating the memory required by it and does all the initialization of the values being passed as parameter
* @param numAttr (int) - denotes the number of attributes the table has
* @param attrNames (char **) - contains the names of all the columns in the table
* @param dataTypes (DataType *) - Contains the enum datatypes
* @param typeLength (int *) - contains an array of lengths of each columns
* @param keySize (int) - Contains the size of the key
* @param keys (int *) - contains the position of the column which acts as the primary key
* @return Schema - which holds the reference to the schema which holds all the values
* @author Pavan Rao
* @Date 10/27/2015
*/
Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	Schema *newSchema = (Schema *)malloc(sizeof(Schema));

	newSchema->attrNames = attrNames;
	newSchema->dataTypes = dataTypes;
	newSchema->keyAttrs = keys;
	newSchema->keySize = keySize;
	newSchema->typeLength = typeLength;
	newSchema->numAttr = numAttr;

	return newSchema;
}

/**
* doesnt do anything
* @param mgmData (void *)
* @returns RC_OK
* @author Pavan Rao
* @date 10/27/2015
*/
RC initRecordManager(void *mgmtData)
{
	return RC_OK;
}

RC createTableInfo(RM_TABLE_INFO *tableData)
{
	tableData->firstFreeRec = (RID *)malloc(sizeof(RID));
	tableData->firstFreeRec->page = -1;
	tableData->firstFreeRec->slot = -1;
	tableData->LastRecID.page = -1;
	tableData->LastRecID.slot = -1;
	tableData->totalNumOfRec = 0;
	tableData->s = (Schema *)malloc(sizeof(Schema));
	return RC_OK;
}

/**
*
*/
RC createTable(char *name, Schema *schema)
{
	char *res = (char *)malloc(PAGE_SIZE);

	if (createPageFile(name) != RC_OK)
	{
		free(res);
		return RC_CANNOT_CREATE_FILE;
	}

	BM_BufferPool *bp = (BM_BufferPool *)malloc(sizeof(BM_BufferPool));
	BM_PageHandle *ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));

	if (initBufferPool(bp, name, 5, RS_FIFO, NULL) != RC_OK)
	{
		free(res);
		return RC_BM_BUFFERPOOL_NOT_INIT;
	}

	if (pinPage(bp, ph, 0) != RC_OK)
	{
		free(res);
		return RC_PIN_PAGE_FAILED;
	}

	RM_TABLE_INFO *tableData = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));

	createTableInfo(tableData);
	tableData->s = schema;

	serializeTableInformation(name, tableData, res);
	memcpy(ph->data, res, PAGE_SIZE);

	if (markDirty(bp, ph) != RC_OK)
	{
		free(res);
		return RC_MAKE_DIRTY_FAILED;
	}
	unpinPage(bp, ph);
	forcePage(bp, ph);
	shutdownBufferPool(bp);

	free(bp);
	free(ph);
	free(tableData);
	free(res);
	return RC_OK;
}

//Assumption the structure of the schema information is stored in the following format
//Table Name
// Number of Attributes
//Names of the Attributes each 100 bytes
//DataTypes of each column
//An array of length of each DATATYPES ex: for DT_STRING we store the length of the string
//Number of keys
//An Array of key positions
//Total number of records in the table
//RID of the first free space in the table
/**
* serializes the infomation of the schema into a string which can be stored in a file
* returns RC_OK on success else a failure code
* author Pavan Rao C R
* date 11/3/2015
* @param name - name of the file
* @param RM_TABLE_INFO - contains schema, Number of records in the table and first free space available with the RID of it
* @param res - char pointer which holds the res
*/
RC serializeTableInformation(char *name, RM_TABLE_INFO *r, char *res)
{
	int resLen = 0;
	int i;
	int nameLen = strlen(name);
	int numAttrLen = r->s->numAttr;
	char *temp = (char *)malloc(PAGE_SIZE);
	memcpy(res, "", 1);

	//to check if the string length is less than 100Bytes
	if (strlen(name) > 0)
	{
		strcpy(res, name);
	}
	else
	{
		return RC_NAME_NOT_VALID;
	}

	resLen = strlen(res);

	if (r->s->numAttr > 0)
	{
		strcpy(res + resLen, ";");
		resLen = strlen(res);
		strcpy(temp, " ");
		_itoa(r->s->numAttr, temp, 10);						//converts integer to string
		strcat(res + resLen, temp);
	}
	else
	{
		return RC_INVALID_NUM_ATTR;
	}

	resLen = strlen(res);

	//puts the names of all the attributes
	for (i = 0; i < r->s->numAttr; i++)
	{
		if (strcmp(r->s->attrNames[i], ""))
		{
			strcat(res + resLen, ";");
			resLen = strlen(res);
			strcat(res + resLen, r->s->attrNames[i]);
			resLen = strlen(res);
		}
		else
		{
			return RC_INVALID_ATTR_NAMES;
		}
	}

	//puts the type of each attribute
	for (i = 0; i < numAttrLen; i++)
	{
		if (r->s->dataTypes[i] == DT_BOOL || r->s->dataTypes[i] == DT_FLOAT
			|| r->s->dataTypes[i] == DT_INT || r->s->dataTypes[i] == DT_STRING)
		{
			strcat(res + resLen, ";");
			resLen = strlen(res);
			_itoa((int *)r->s->dataTypes[i], temp, 10);
			strcat(res + resLen, temp);
			resLen = strlen(res);
		}
		else
		{
			return RC_INVALID_DATATYPE;
		}
	}

	//puts the size of each attributes
	for (i = 0; i < r->s->numAttr; i++)
	{
		if (r->s->typeLength[i] != -1)
		{
			strcat(res + resLen, ";");
			resLen = strlen(res);
			strcpy(temp, " ");
			_itoa(r->s->typeLength[i], temp, 10);
			strcat(res + resLen, temp);
			resLen = strlen(res);
		}
		else
		{
			return RC_INVALID_TYPE_LENGTH;
		}
	}

	if (r->s->keySize > 0)
	{
		strcat(res + resLen, ";");
		resLen = strlen(res);
		strcpy(temp, " ");
		_itoa(r->s->keySize, temp, 10);
		strcat(res + resLen, temp);
		resLen = strlen(res);

		//inserting the primary keys
		for (i = 0; i < r->s->keySize; i++)
		{
			strcat(res + resLen, ";");
			resLen = strlen(res);
			strcpy(temp, " ");
			_itoa(r->s->keyAttrs[i], temp, 10);
			strcat(res + resLen, temp);
			resLen = strlen(res);
		}
	}
	else
	{
		return RC_INVALID_KEY_SIZE;
	}

	if (r->totalNumOfRec < 0)
	{
		r->totalNumOfRec = 0;
	}
	//inserts the total number of rec in the table
	strcat(res + resLen, ";");
	resLen = strlen(res);
	strcpy(temp, " ");
	_itoa(r->totalNumOfRec, temp, 10);
	strcat(res + resLen, temp);
	resLen = strlen(res);

	//inserts the first avaiable free space of the record
	if (r->totalNumOfRec == NULL)
	{
		r->firstFreeRec->page = -1;
		r->firstFreeRec->slot = -1;
		r->LastRecID.page = -1;
		r->LastRecID.slot = -1;
	}

	strcat(res + resLen, ";");
	resLen = strlen(res);
	strcpy(temp, " ");
	_itoa(r->firstFreeRec->page, temp, 10);
	strcat(res + resLen, temp);
	resLen = strlen(res);
	strcat(res + resLen, ",");
	resLen = strlen(res);
	strcpy(temp, " ");
	_itoa(r->firstFreeRec->slot, temp, 10);
	strcat(res + resLen, temp);
	resLen = strlen(res);

	//for lastRec in the table
	strcat(res + resLen, ";");
	resLen = strlen(res);
	strcpy(temp, " ");
	_itoa(r->LastRecID.page, temp, 10);
	strcat(res + resLen, temp);
	resLen = strlen(res);
	strcat(res + resLen, ",");
	resLen = strlen(res);
	strcpy(temp, " ");
	_itoa(r->LastRecID.slot, temp, 10);
	strcat(res + resLen, temp);
	resLen = strlen(res);

	//final 
	strcat(res + resLen, ";");
	resLen = strlen(res);

	if (resLen > PAGE_SIZE)
	{
		return RC_INFO_PAGE_TOO_BIG;
	}

	free(temp);
	return RC_OK;
}


/**
* deserializes the infomation of the schema and table from the table information page into a a RM_TABLE_INFO datastructure
* returns RC_OK on success else a failure code
* author Pavan Rao C R
* date 11/3/2015
* @param page - table information page in form of a char *
* @param RM_TABLE_INFO - empty
*/
RC deSerializeTableInformation(char *page, RM_TABLE_INFO *infoPage)
{
	char *token;
	int i;
	char *temp1;
	i = strlen(page);
	temp1 = (char *)malloc(i + 1);
	strcpy(temp1, page);
	Schema *temp = (Schema *)malloc(sizeof(Schema));

	//retrives name of the table
	token = strtok(temp1, ";");

	//retrives the number of attributes in the table
	token = strtok(NULL, ";");

	temp->numAttr = atoi(token);
	temp->attrNames = (char **)malloc(sizeof(char*) * temp->numAttr);

	//retrives the names of all the attributes
	for (i = 0; i < temp->numAttr; i++)
	{
		token = strtok(NULL, ";");
		temp->attrNames[i] = token;
	}

	temp->dataTypes = (DataType *)malloc(sizeof(DataType) * temp->numAttr);

	//retrives the type of each record
	for (i = 0; i < temp->numAttr; i++)
	{
		token = strtok(NULL, ";");
		temp->dataTypes[i] = atoi(token);
	}

	temp->typeLength = (int *)malloc(sizeof(int) * temp->numAttr);

	//retrives the typeLength of each attribute
	for (i = 0; i < temp->numAttr; i++)
	{
		token = strtok(NULL, ";");
		temp->typeLength[i] = atoi(token);
	}

	//retrieves the number of keys
	token = strtok(NULL, ";");
	temp->keySize = atoi(token);

	temp->keyAttrs = (int *)malloc(sizeof(int) * temp->keySize);

	//retrives the keys
	for (i = 0; i < temp->keySize; i++)
	{
		token = strtok(NULL, ";");
		temp->keyAttrs[i] = atoi(token);
	}

	infoPage->s = temp;

	//retrives the total number of records
	token = strtok(NULL, ";");
	infoPage->totalNumOfRec = atoi(token);

	infoPage->firstFreeRec = (RID *)malloc(sizeof(RID));
	//retrives the RID of first free space
	token = strtok(NULL, ",");
	infoPage->firstFreeRec->page = atoi(token);
	token = strtok(NULL, ";");
	infoPage->firstFreeRec->slot = atoi(token);

	//retrives the RID of the LAST REC in the Table
	token = strtok(NULL, ",");
	infoPage->LastRecID.page = atoi(token);
	token = strtok(NULL, ";");
	infoPage->LastRecID.slot = atoi(token);
	return RC_OK;
}

/**
* This function inserts the records in the first empty spot which is available
* @param RM_TableData - Contains the details of the table
* @param Record - Contains the record that is to be inserted
* @returns - RC_OK if there are no errors
* @author Pavan Rao
* @date 11/05/2015
**/
RC insertRecord(RM_TableData *rel, Record *record)
{
	if (rel->mgmtData == NULL)
	{
		return RC_BM_BUFFERPOOL_NOT_INIT;
	}

	BM_BufferPool *buff = (BM_BufferPool *)rel->mgmtData;
	BM_PageHandle *ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));

	if (pinPage(buff, ph, 0) != RC_OK)
	{
		return RC_PIN_PAGE_FAILED;
	}

	RM_TABLE_INFO *newTable = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));
	Record *nextFreeRec = (Record *)malloc(sizeof(Record));
	PAGE_INFO *pageHeader = (PAGE_INFO *)malloc(PAGE_INFO_SIZE);
	BM_PageHandle *ph1 = MAKE_PAGE_HANDLE();
	RID prevLastRec;

	deSerializeTableInformation(ph->data, newTable);

	prevLastRec = newTable->LastRecID;

	if (newTable->firstFreeRec->page < 0 || newTable->firstFreeRec->slot < 0)
	{
		if (newTable->totalNumOfRec == 0)
		{
			//this means the table was just created
			record->id.page = 1;
			record->id.slot = 1;
			newTable->totalNumOfRec++;
			newTable->LastRecID = record->id;
		}
		else
		{
			// find the RID of last record of the table
			record->id.page = newTable->LastRecID.page;
			record->id.slot = newTable->LastRecID.slot;

			//make sure that there is enough space in the current page for placing the next record
			getNextRID(record, buff, rel);

			newTable->totalNumOfRec++;
			newTable->LastRecID = record->id;
		}
	}
	else
	{
		//else if there is a free space available 
		record->id = *(newTable->firstFreeRec);
		getRecord(rel, *(newTable->firstFreeRec), nextFreeRec);
		//write function to deserialize to obtain the RID of next Free rec
		deSerializeEmptyRec(nextFreeRec->data, &nextFreeRec->id);
		//stores the id to the 
		newTable->firstFreeRec = &nextFreeRec->id;
	}

	if (pinPage(buff, ph1, record->id.page) != RC_OK)
	{
		return RC_PIN_PAGE_FAILED;
	}

	//printf("\n %s - test rec col1\n", record->data);
	//printf("\n %s - test rec col2\n", record->data + sizeof(int));
	//printf("\n %s - test rec col3\n", record->data + sizeof(int) + 4);

	//since this function inserts the record at the first free space available it is not required to update the previous free location
	if (updateRecordWithTombStone(rel, record, FALSE, ph1->data) != RC_OK)
		return RC_INSERT_FAILED;

	//printf("\n %s - Tombstone Value\n", ph1->data + getRecordSize(rel->schema) * (record->id.slot - 1) + PAGE_INFO_SIZE);
	//printf("\n %s - THIS is col1\n", ph1->data + getRecordSize(rel->schema) * (record->id.slot - 1) + PAGE_INFO_SIZE + TOMBSTONE_SIZE);
	//printf("\n %s - THIS is col2\n", ph1->data + getRecordSize(rel->schema) * (record->id.slot - 1) + PAGE_INFO_SIZE + TOMBSTONE_SIZE + sizeof(int));
	//printf("\n %s - THIS is col3\n", ph1->data + getRecordSize(rel->schema) * (record->id.slot - 1) + PAGE_INFO_SIZE + TOMBSTONE_SIZE + sizeof(int) + 4);

	if (strcmp(ph1->data, "") == 0)
	{
		pageHeader->firstDeletedRecord.page = -1;
		pageHeader->firstDeletedRecord.slot = -1;
		pageHeader->lastRecord.page = ph1->pageNum;
		pageHeader->lastRecord.slot = 1;
	}
	else
		extractPageHeader(ph1->data, pageHeader);

	//this is to update the table info page and the pageHeader
	if ((prevLastRec.page > record->id.page) ||
		(prevLastRec.page == record->id.page && prevLastRec.slot < record->id.slot))
	{
		pageHeader->lastRecord = record->id;
		pageHeader->firstDeletedRecord.page = -1;
		pageHeader->firstDeletedRecord.slot = -1;
	}

	if (record->id.page == pageHeader->firstDeletedRecord.page
		&& record->id.slot == pageHeader->firstDeletedRecord.slot)
	{
		if (nextFreeRec->id.page > pageHeader->firstDeletedRecord.page)
		{
			pageHeader->firstDeletedRecord.page = -1;
			pageHeader->firstDeletedRecord.slot = -1;
		}
		else
		{
			// the page will remain the same only the slot number changes
			pageHeader->firstDeletedRecord.slot = nextFreeRec->id.slot;
		}
	}
	updateHeaderOfThePage(ph1->data, pageHeader);

	//printf("\n %s - THIS is col1\n", ph1->data + PAGE_INFO_SIZE + TOMBSTONE_SIZE );

	//making the page where the rec is inserted dirty so that the change is reflected when flushed
	markDirty(buff, ph1);
	unpinPage(buff, ph1);
	free(ph1);

	//making the table info page dirty
	serializeTableInformation(rel->name, newTable, ph->data);
	ph->pageNum = 0;
	markDirty(buff, ph);
	unpinPage(buff, ph);

	free(newTable);
	free(pageHeader);
	free(ph);

	return RC_OK;
}

/**
*	deletes the record whose is is provided and make sures it updates the page header and table info page
*	@param - RM_TableData - Contains the table data
*	@param - RID - Contains the RID of the record that is to be deleted
*	@returns - RC_OK when success
*	@author Pavan Rao
*	@date 11/7/2015
**/
RC deleteRecord(RM_TableData *rel, RID id)
{
	if (rel->mgmtData == NULL)
		return RC_BM_BUFFERPOOL_NOT_INIT;

	int i;
	PageNumber pNum = id.page;
	RID prevFreeRID, nextFreeRID;
	int recLen = getRecordSize(rel->schema);
	Record *prevRec = (Record *)malloc(sizeof(Record));
	prevRec->data = (char *)malloc(sizeof(recLen));
	int slotNum = id.slot;
	char *recordToDelete;
	RC status; // used further below in the function

	BM_PageHandle *ph = MAKE_PAGE_HANDLE();

	BM_BufferPool *bp = (BM_BufferPool *)rel->mgmtData;

	if (pinPage(bp, ph, pNum) != RC_OK)
	{
		free(ph);
		free(prevRec->data);
		free(prevRec);
		return RC_PIN_PAGE_FAILED;
	}

	recordToDelete = ph->data;
	recordToDelete += PAGE_INFO_SIZE + (recLen + TOMBSTONE_SIZE) * (slotNum - 1);

	char *ts = (char *)malloc(TOMBSTONE_SIZE);
	int tsi;

	memmove(ts, recordToDelete, TOMBSTONE_SIZE);
	tsi = atoi(ts);

	if (tsi == 1)
	{
		free(ph);
		free(prevRec->data);
		free(prevRec);
		return RC_OK;
	}
	unpinPage(bp, ph);

	//retrive the previous and next free record ids with current record that is to be deleted as a reference
	getPreviousAndNextDeletedRecordId(rel, id, prevRec, &nextFreeRID);
	free(ph);

	ph = MAKE_PAGE_HANDLE();

	//if there are no free records prior to current record that is to be deleted
	if (prevRec->id.page == -1 || prevRec->id.slot == -1)
	{
		if (pinPage(bp, ph, 0) != RC_OK)
		{
			free(ph);
			free(prevRec->data);
			free(prevRec);
			return RC_PIN_PAGE_FAILED;
		}

		RM_TABLE_INFO *tableData = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));
		deSerializeTableInformation(ph->data, tableData);

		tableData->firstFreeRec = &id;

		serializeTableInformation(rel->name, tableData, ph->data);

		markDirty(bp, ph);
		unpinPage(bp, ph);
		free(tableData);
	}
	else
	{
		//serializing the previous empty rec data with the RID of newly freed empty rec
		serializaeEmptyRec(prevRec->data, id);
		BM_PageHandle *ph1 = MAKE_PAGE_HANDLE();
		if (pinPage(bp, ph1, prevRec->id.page) != RC_OK)
		{
			free(ph);
			free(ph1);
			free(prevRec->data);
			free(prevRec);
			return RC_PIN_PAGE_FAILED;
		}
		updateRecordWithTombStone(rel, prevRec, TRUE, ph1->data);
		markDirty(bp, ph1);
		unpinPage(bp, ph1);
		free(ph1);
	}

	//free(prevRec->data);
	free(prevRec);

	BM_PageHandle *ph2 = MAKE_PAGE_HANDLE();
	if (pinPage(bp, ph2, id.page) != RC_OK)
	{
		free(ph);
		free(ph2);
		free(prevRec->data);
		free(prevRec);
		return RC_PIN_PAGE_FAILED;
	}

	prevRec = (Record *)malloc(sizeof(Record));
	prevRec->data = (char *)malloc(getRecordSize(rel->schema));

	//pushing the RID of the next free space available
	serializaeEmptyRec(prevRec->data, nextFreeRID);

	prevRec->id = id;

	updateRecordWithTombStone(rel, prevRec, TRUE, ph2->data);



	// this is to update the header of the page if the deleted rec was the first record of the page
	PAGE_INFO *pageHeader = (PAGE_INFO *)malloc(sizeof(PAGE_INFO));

	extractPageHeader(ph2->data, pageHeader);

	if ((pageHeader->firstDeletedRecord.page == -1
		&& pageHeader->firstDeletedRecord.slot == -1) ||
		(pageHeader->firstDeletedRecord.page == id.page
			&& pageHeader->firstDeletedRecord.slot > id.slot))
	{
		pageHeader->firstDeletedRecord.page = id.page;
		pageHeader->firstDeletedRecord.slot = id.slot;

		//once the firstDeletedRecord is formed the header has to be updated
		if ((status = updateHeaderOfThePage(ph2->data, pageHeader)) != RC_OK)
		{
			free(prevRec->data);
			free(prevRec);
			free(ph);
			free(ph2);
			return status;
		}
	}

	markDirty(bp, ph2);
	unpinPage(bp, ph2);

	free(prevRec->data);
	free(prevRec);
	free(ph);
	free(ph2);
	return RC_OK;
}

/**
*	Updates the page header with the new header values
*	@param CHAR * which contains the data
*	@param pageheader(PAGE_INFO) - contains the information that is to be embeded into the page
*	@returns RC_OK on success
*	@author Pavan Rao
*	@date 11/7/2015
**/
RC updateHeaderOfThePage(char *ph, PAGE_INFO *pageHeader)
{
	char *temp = (char *)malloc(PAGE_INFO_SIZE);

	//this will serialize the info that is to be put in the header
	serializePageHeader(pageHeader, temp);

	if (strlen(temp) > PAGE_INFO_SIZE)
		return RC_PAGE_HEADER_SIZE_OVERFLOW;

	memmove(ph, temp, PAGE_INFO_SIZE);

	if (strlen(ph) > PAGE_SIZE)
		return RC_UPDATE_HEADER_FAILED;

	free(temp);
	return RC_OK;
}

/**
*	Serializes the page header
*	@param pageheader(PAGE_INFO) - contains the information that is to be embeded into the page
*	@param CHAR * which is empty and has to be populated with the serialized pageheader
*	@returns RC_OK on success
*	@author Pavan Rao
*	@date 11/7/2015
**/
RC serializePageHeader(PAGE_INFO *pageHeader, char *temp)
{
	char *intermediate = (char *)malloc(10);

	_itoa(pageHeader->firstDeletedRecord.page, intermediate, 10);
	strcpy(temp, intermediate);
	strcat(temp, ",");

	_itoa(pageHeader->firstDeletedRecord.slot, intermediate, 10);
	strcat(temp, intermediate);
	strcat(temp, ";");

	_itoa(pageHeader->lastRecord.page, intermediate, 10);
	strcat(temp, intermediate);
	strcat(temp, ",");

	_itoa(pageHeader->lastRecord.slot, intermediate, 10);
	strcat(temp, intermediate);
	strcat(temp, ";");

	free(intermediate);
	return RC_OK;
}

/**
*	Extracts the header from the pageFrame
*	@param ph(char *) -> Page Frame which contains the header and the records of the Page
*	@param pageHeader(PAGE_INFO) -> empty PAGE_INFO which is to be popullated with the values extracted from the pageFrame
*	@returns RC_OK on success
*	@author Pavan Rao
*	@date 11/6/2015
**/
RC extractPageHeader(char *ph, PAGE_INFO *pageHeader)
{
	char *token;
	char *dataCopy = (char *)malloc(PAGE_INFO_SIZE);

	memmove(dataCopy, ph, PAGE_INFO_SIZE);

	token = strtok(dataCopy, ",");
	pageHeader->firstDeletedRecord.page = atoi(token);
	token = strtok(NULL, ";");
	pageHeader->firstDeletedRecord.slot = atoi(token);
	token = strtok(NULL, ",");
	pageHeader->lastRecord.page = atoi(token);
	token = strtok(NULL, ";");
	pageHeader->lastRecord.slot = atoi(token);

	free(dataCopy);
	return RC_OK;
}

/**
*	Deserializes the empty rec data which contains the RID of the next free Rec which can either
*	a valid RID or (-1,-1)
*	@param recordData (char *) contains the comma seperated RID of the next free space
*	@param RID (RID) This is the parameter which should be updated with the RID value retrieved from the previous parameter
*	@returns RC_OK on success
*	@author Pavan Rao
*	@date 11/16/2015
**/
RC deSerializeEmptyRec(char *data, RID *rid)
{
	char *token = (char *)malloc(sizeof(int));
	memmove(token, data, sizeof(int));
	rid->page = atoi(token);
	memmove(token, data + sizeof(int) + 1, sizeof(int));
	rid->slot = atoi(token);

	free(token);
	return RC_OK;
}

/**
*	serializes the RID and store it in the data of the empty rec
*	@param recordData (char *) empty
*	@param RID (RID) That contains the
*	@returns RC_OK on success
*	@author Pavan Rao
*	@date 11/16/2015
**/
RC serializaeEmptyRec(char *data, RID rid)
{
	char *token;
	int offset = 0;
	token = (char *)malloc(sizeof(int));
	_itoa(rid.page, token, 10);
	memmove(data + offset, token, sizeof(int));
	offset += sizeof(int);
	memmove(data + offset, ",", 1);
	offset += 1;
	free(token);
	token = (char *)malloc(sizeof(int));
	_itoa(rid.slot, token, 10);
	memmove(data + offset, token, sizeof(int));
	offset += sizeof(int);
	memmove(data + offset, ";", 1);

	return RC_OK;
}

RC getNextRID(Record *rec, BM_BufferPool *bm, RM_TableData *rel)
{
	BM_PageHandle *tempPage = MAKE_PAGE_HANDLE();
	PAGE_INFO *tempPageHeader = (PAGE_INFO *)malloc(PAGE_INFO_SIZE);
	int numOfRecInPage, recLen = getRecordSize(rel->schema) + TOMBSTONE_SIZE;

	if (pinPage(bm, tempPage, rec->id.page) != RC_OK)
	{
		return RC_PIN_PAGE_FAILED;
	}
	unpinPage(bm, tempPage);

	extractPageHeader(tempPage->data, tempPageHeader);

	numOfRecInPage = (int *)((PAGE_SIZE - PAGE_INFO_SIZE) / recLen);	//removing any floting point values found if any by the division operation by type casting it into int

																		//printf("Number of Records that can be stored in a PAGE = %d \n", numOfRecInPage);

																		//since this is the last page length of the data in the last + 
	if (tempPageHeader->lastRecord.slot + 1 > numOfRecInPage)
	{
		rec->id.page++;
		rec->id.slot = 1;
	}
	else
	{
		rec->id.slot++;
	}

	free(tempPage);
	free(tempPageHeader);
	return RC_OK;
}

/*
* Function to Close the table
* INPUT: Table structure
* OUTPUT: free the attributes associated with schema & clear the BufferPool and return RC_OK
* AUTHOR: Chethan
*/

RC closeTable(RM_TableData *rel)
{
	BM_BufferPool *bm = (BM_BufferPool *)rel->mgmtData;
	//free the schema
	free(rel->schema->dataTypes);
	free(rel->schema->typeLength);
	free(rel->schema->attrNames);
	free(rel->schema->keyAttrs);
	//free(rel->name);
	free(rel->schema);
	//shutdown the BufferPool associated with the table
	shutdownBufferPool(bm);
	free(bm);
	return RC_OK;
}

/*
* Function to Delete the table
* INPUT: Table Name
* OUTPUT: Delete the PageFile and return RC_OK
* AUTHOR: Chethan
*/

RC deleteTable(char *name)
{
	destroyPageFile(name); // Deleting the Page File
	return RC_OK;
}

/*
* Function to Free Schema memory
* INPUT: Schema
* OUTPUT: Free the memory occupied by the schema and return RC_OK
* AUTHOR: Chethan
*/

RC freeSchema(Schema *schema)
{
	free(schema);
	return RC_OK;
}

/*
* Function to create a Record, Allocate the memory to the Record
* INPUT: Pointer to record & Schema
* OUTPUT: Creating Record and return RC_OK
* AUTHOR: Chethan
*/

RC createRecord(Record **record, Schema *schema)
{
	int record_Size = 0;
	//Calculating size of the record
	record_Size = getRecordSize(schema);

	//Allocating memory to record
	*record = (Record *)malloc(sizeof(Record));
	(*record)->data = (char *)malloc(record_Size);
	return RC_OK;
}

/*
* Function to calculate the size of the Record
* INPUT: Schema
* OUTPUT: Size of the Record
* AUTHOR: Chethan
*/

int getRecordSize(Schema *schema)
{
	int i;
	int sizeofRecord = 0;
	DataType *dt = schema->dataTypes;
	int *tpl = schema->typeLength;
	for (i = 0; i<schema->numAttr; i++) {
		switch (dt[i]) {
		case DT_INT:
			sizeofRecord += sizeof(int);
			break;
		case DT_FLOAT:
			sizeofRecord += sizeof(float);
			break;
		case DT_BOOL:
			sizeofRecord += sizeof(bool);
			break;
		case DT_STRING:
			sizeofRecord += tpl[i];
			break;
		}
	}
	return sizeofRecord;
}

/*
* Function to free the Record
* INPUT: Record
* OUTPUT: Free the memory associated with record and return RC_OK
* AUTHOR: Chethan
*/

RC freeRecord(Record *record)
{
	free(record->data);
	free(record);
	return RC_OK;
}

/*
* Function to Update the Record
* INPUT: Record
* OUTPUT: Update the Record and return RC_OK
* AUTHOR: Chethan
*/

RC updateRecord(RM_TableData *rel, Record *record)
{
	int recordLength = 0;
	RID id = record->id;
	PageNumber pNum = id.page;
	int slotNum = id.slot;
	//char *recordToUpdate = (char *)malloc(PAGE_SIZE);

	BM_PageHandle *ph5 = MAKE_PAGE_HANDLE();
	BM_BufferPool *bp;

	if ((BM_BufferPool *)rel->mgmtData != NULL)
	{
		bp = (BM_BufferPool *)rel->mgmtData;
	}
	else {
		return RC_BM_BUFFERPOOL_NOT_INIT;
	}

	recordLength = getRecordSize(rel->schema);

	if (pinPage(bp, ph5, pNum) != RC_OK)
	{
		free(ph5);
		return RC_PIN_PAGE_FAILED;
	}

	//memmove(recordToUpdate, ph->data, PAGE_SIZE);
	//recordToUpdate = recordToUpdate + recordLength * (slotNum - 1) + PAGE_INFO_SIZE;

	memmove(ph5->data + (recordLength + TOMBSTONE_SIZE) * (slotNum - 1) + TOMBSTONE_SIZE + PAGE_INFO_SIZE, record->data, recordLength);

	if (markDirty(bp, ph5) != RC_OK)
	{
		free(ph5);
		return RC_MAKE_DIRTY_FAILED;
	}

	unpinPage(bp, ph5);

	//free(recordToUpdate);
	free(ph5);
	return RC_OK;

}

/*
* Function to Update the Record
* INPUT: Record
* OUTPUT: Update the Record and return RC_OK
* AUTHOR: Chethan
*/

RC updateRecordWithTombStone(RM_TableData *rel, Record *record, bool tombStone, char *pageData)
{
	int recordLength = 0;
	RID id = record->id;
	PageNumber pNum = id.page;
	int slotNum = id.slot;

	//BM_PageHandle *ph = MAKE_PAGE_HANDLE();
	//BM_BufferPool *bp;

	/*if ((BM_BufferPool *)rel->mgmtData != NULL)
	{
	bp = (BM_BufferPool *)rel->mgmtData;
	}
	else {
	return RC_BM_BUFFERPOOL_NOT_INIT;
	}*/

	recordLength = getRecordSize(rel->schema);

	/*if (pinPage(bp, ph, pNum) != RC_OK)
	{
	free(ph);
	return RC_PIN_PAGE_FAILED;
	}*/

	//recordToUpdate = ph->data;
	//recordToUpdate = recordToUpdate + recordLength * (slotNum - 1)+ PAGE_INFO_SIZE;
	char *i = (char *)malloc(sizeof(TOMBSTONE_SIZE));
	_itoa(tombStone, i, 10);

	memmove(pageData + (recordLength + TOMBSTONE_SIZE) * (slotNum - 1) + PAGE_INFO_SIZE, i, TOMBSTONE_SIZE);

	memmove(pageData + (recordLength + TOMBSTONE_SIZE) * (slotNum - 1) + PAGE_INFO_SIZE + TOMBSTONE_SIZE, record->data, recordLength);



	/*if (markDirty(bp, ph) != RC_OK)
	{
	free(ph);
	return RC_MAKE_DIRTY_FAILED;
	}*/

	//unpinPage(bp, ph);

	//free(ph);
	return RC_OK;
}

/*
* Function to Close the Scan
* INPUT: ScanHandle
* OUTPUT: free memory associated with scan and return RC_OK
* AUTHOR: Chethan
*/

RC closeScan(RM_ScanHandle *scan)
{
	free(scan->mgmtData);
	return RC_OK;
}

/**
*This function is used to assign record data for a specific datatype depending on the type of schema.
*
*
* @author  Anand N
* @param   Schema
* @param   attrNum
* @param   Value
* @return  Value
* @since   2015-11-02
*/
RC assignAttrDataForDataType(Schema *schema, int attrNum, Value **value, char **attrData) {

	if ((*value)->dt == DT_INT) {
		//Int value
		attrData[0] = (char *)malloc(sizeof(int));
		sprintf(attrData, "%d", (*value)->v.intV);
	}
	else if ((*value)->dt == DT_STRING) {
		//String value
		attrData[0] = (char *)malloc(schema->typeLength[attrNum]);
		attrData[0] = (*value)->v.stringV;
	}
	else if ((*value)->dt == DT_FLOAT) {
		//float value
		attrData[0] = (char *)malloc(sizeof(float));
		sprintf(attrData, "%f", (*value)->v.floatV);
	}
	else {
		// boolean value
		attrData[0] = (char *)malloc(sizeof(int));
		if ((*value)->v.boolV) {
			attrData[0] = "true";
		}
		else {
			attrData[0] = "false";
		}
	}
	(*value)->dt = schema->dataTypes[attrNum];
	return RC_OK;
}

/**
*This function gets the attribute.
*
*
* @author  Anand N
* @param   Record
* @param   Schema
* @param   attrNum
* @param   Value
* @return  RC
* @since   2015-11-02
*/
RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {

	RC returnVal = checkIfAttrNumIsValid(attrNum);
	if (returnVal != RC_OK) {
		return returnVal;
	}

	int offset = 0;
	int len, i, j;
	unsigned char *attrData = NULL;

	if (attrNum != 0)
		for (i = 1; i <= attrNum; i++)
		{
			switch (schema->dataTypes[attrNum - i])
			{
			case 0: offset += sizeof(int);	break;
			case 1: offset += schema->typeLength[attrNum - i]; break;
			case 2: offset += sizeof(float);	break;
			case 3: offset += sizeof(bool);	break;
			}
		}

	switch (schema->dataTypes[attrNum])
	{
	case 0: len = sizeof(int);	break;
	case 1: len = schema->typeLength[attrNum]; break;
	case 2: len = sizeof(float);	break;
	case 3: len = sizeof(bool);	break;
	}
	attrData = (char *)malloc(len);
	/*for (i = offset, j = 0; i < (offset + len); i++, j++)
	{
	attrData[j] = record->data[i];
	}*/
	memmove(attrData, record->data + offset, len);

	if (schema->dataTypes[attrNum] == DT_INT) {
		//Int value
		MAKE_VALUE(*value, schema->dataTypes[attrNum], atof(attrData));
		//v->dt = DT_INT;
		//v->v.intV = atof(attrData);
	}
	else if (schema->dataTypes[attrNum] == DT_STRING) {
		//String value
		Value *v = (Value *)malloc(sizeof(Value));
		(*value) = (Value *)malloc(sizeof(Value));
		v->dt = DT_STRING;
		(*value)->dt = (DataType)malloc(1);
		v->v.stringV = (char *)malloc(schema->typeLength[attrNum]);
		memmove(v->v.stringV, attrData, schema->typeLength[attrNum]);
		memset(v->v.stringV + schema->typeLength[attrNum], '\0', schema->typeLength[attrNum]);
		size_t len = 1 + strlen(v->v.stringV);
		(*value)->v.stringV = (char *)malloc(len);
		strcpy((*value)->v.stringV, v->v.stringV);
		(*value)->dt = DT_STRING;
	}
	else if (schema->dataTypes[attrNum] == DT_STRING) {
		//float value
		MAKE_VALUE(*value, schema->dataTypes[attrNum], atof(attrData));
		//v->dt = DT_FLOAT;
		//v->v.floatV = atof(attrData);
	}
	else {
		// boolean value
		MAKE_VALUE(*value, schema->dataTypes[attrNum], atoi(attrData));
		//v->dt = DT_BOOL;
		//v->v.boolV = atoi(attrData);
	}


	free(attrData);
	return RC_OK;
}

/**
*This function sets the attribute.
*
*
* @author  Anand N
* @param   Record
* @param   Schema
* @param   attrNum
* @param   Value
* @return  RC
* @since   2015-11-02
*/
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
	RC returnVal = checkIfAttrNumIsValid(attrNum);
	if (returnVal != RC_OK) {
		return returnVal;
	}

	int offset = 0;
	unsigned char *attrData = NULL;
	int len, i, j;

	/*switch (attrNum) {
	case 0:
	assignAttrDataForDataType(schema, attrNum, &value, attrData);
	memcpy(record->data + offset + TOMBSTONE_SIZE + PAGE_INFO_SIZE, attrData, sizeof(schema->dataTypes[attrNum]));
	break;
	case 1:
	offset = sizeof(schema->dataTypes[0]) + 1;
	assignAttrDataForDataType(schema, attrNum, &value, attrData);
	memcpy(record->data + offset + TOMBSTONE_SIZE + PAGE_INFO_SIZE, attrData, sizeof(schema->dataTypes[attrNum]));
	break;
	case 2:
	offset = sizeof(schema->dataTypes[1]) + 1;
	assignAttrDataForDataType(schema, attrNum, &value, attrData);
	memcpy(record->data + offset + TOMBSTONE_SIZE + PAGE_INFO_SIZE, attrData, sizeof(schema->dataTypes[attrNum]));
	break;
	}*/
	if (attrNum != 0)
		for (i = 1; i <= attrNum; i++)
		{
			switch (schema->dataTypes[attrNum - i])
			{
			case 0: offset += sizeof(int);	break;
			case 1: offset += schema->typeLength[attrNum - i]; break;
			case 2: offset += sizeof(float);	break;
			case 3: offset += sizeof(bool);	break;
			}
		}
	//assignAttrDataForDataType(schema, attrNum, &value, attrData);


	if (value->dt == DT_INT) {
		//Int value
		attrData = (char *)malloc(sizeof(int));
		sprintf(attrData, "%d", value->v.intV);
	}
	else if (value->dt == DT_STRING) {
		//String value
		attrData = (char *)malloc(schema->typeLength[attrNum]);
		memmove(attrData, value->v.stringV, schema->typeLength[attrNum]);
	}
	else if (value->dt == DT_FLOAT) {
		//float value
		attrData = (char *)malloc(sizeof(float));
		sprintf(attrData, "%f", value->v.floatV);
	}
	else {
		// boolean value
		attrData = (char *)malloc(sizeof(int));
		_itoa(value->v.boolV, attrData, 10);
	}
	switch (schema->dataTypes[attrNum])
	{
	case 0: len = sizeof(int);	break;
	case 1: len = schema->typeLength[attrNum]; break;
	case 2: len = sizeof(float);	break;
	case 3: len = sizeof(bool);	break;
	}

	/*for (i = offset, j = 0; i < (offset + len); i++, j++)
	{
		record->data[i] = attrData[j];
	}*/
	memmove(record->data + offset, attrData, 4);

	//free(attrData);
	return RC_OK;
}


/**
*This function checks if attribute number is valid.
*
*
* @author  Anand N
* @param   attrNum
* @return  RC
* @since   2015-11-02
*/
RC checkIfAttrNumIsValid(attrNum) {
	if (0 > attrNum) {
		return RC_INVALID_ATTRIBUTE_NUM;
	}
	return RC_OK;
}

/**
*This function is used to assign value for a specific datatype depending on the type of schema.
*
*
* @author  Anand N
* @param   Schema
* @param   attrNum
* @param   Value
* @param   attributeData
* @return  Value
* @since   2015-11-02
*/
RC assignValueForDataType(Schema *schema, int attrNum, Value **value, char *attrData) {
	if (schema->dataTypes[attrNum] == DT_INT) {
		//Int value
		(*value)->v.intV = atoi(attrData);
	}
	else if (schema->dataTypes[attrNum] == DT_STRING) {
		//String value
		(*value)->v.stringV = attrData;
	}
	else if (schema->dataTypes[attrNum] == DT_FLOAT) {
		//float value
		(*value)->v.floatV = strtof(attrData, NULL);
	}
	else {
		// boolean value
		if (attrData == "true") {
			(*value)->v.boolV = true;
		}
		else {
			(*value)->v.boolV = false;
		}
	}
	(*value)->dt = schema->dataTypes[attrNum];
	return RC_OK;
}

/**
*This function shuts down the record manager.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   RM_ScanHandle
* @param   Expr
* @return  RC
* @since   2015-11-02
*/
RC shutdownRecordManager() {
	return RC_OK;
}

/**
*This function shuts down the record manager.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   name
* @return  RC
* @since   2015-11-02
*/
RC openTable(RM_TableData *rel, char *name) {

	RM_TableData *tableData = NULL;
	Schema *schema;
	BM_BufferPool *bm;
	bm = MAKE_POOL();
	initBufferPool(bm, name, 5, RS_FIFO, NULL);

	BM_PageHandle *bm_ph = MAKE_PAGE_HANDLE();
	RM_TABLE_INFO *rmTableInfo = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));


	if (pinPage(bm, bm_ph, 0) != RC_OK) {
		free(bm_ph);
		return RC_PIN_PAGE_FAILED;
	}
	deSerializeTableInformation(bm_ph->data, rmTableInfo);

	rel->schema = rmTableInfo->s;
	rel->name = name;
	rel->mgmtData = bm;

	unpinPage(bm, bm_ph);

	//free(bm);
	free(bm_ph);
	return RC_OK;
}

/**
*This function gets the number of tuples.
*
*
* @author  Anand N
* @param   RM_TableData
* @return  number of tuples
* @since   2015-11-02
*/
int getNumTuples(RM_TableData *rel) {

	BM_PageHandle *bmPh = MAKE_PAGE_HANDLE();
	BM_BufferPool *bm = (BM_BufferPool *)rel->mgmtData;
	RM_TABLE_INFO *rmTableInfo = NULL;

	if (pinPage(bm, bmPh, 0) != RC_OK) {
		free(bmPh);
		return RC_PIN_PAGE_FAILED;
	}
	deSerializeTableInformation(bmPh, rmTableInfo);

	unpinPage(bm, bmPh);
	free(bmPh);

	return rmTableInfo->totalNumOfRec;
}

/**
*This function gets the number of tuples.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   RID
* @param   Record
* @return  RC
* @since   2015-11-05
*/
RC getRecord(RM_TableData *rel, RID id, Record *record) {

	int recordLength = 0;
	PageNumber pNum = id.page;
	int slotNum = id.slot;
	int offSet = 0;

	BM_PageHandle *ph = MAKE_PAGE_HANDLE();
	BM_BufferPool *bp = (BM_BufferPool *)rel->mgmtData;
	record->id = id;
	recordLength = getRecordSize(rel->schema);
	if (pinPage(bp, ph, pNum) != RC_OK) {
		free(ph);
		return RC_PIN_PAGE_FAILED;
	}
	offSet = (recordLength + TOMBSTONE_SIZE) * (slotNum - 1);
	//return empty if we encounter tombstone else populate record data
	char *tombstoneData = (char *)malloc(TOMBSTONE_SIZE);
	memmove(tombstoneData, ph->data + offSet + PAGE_INFO_SIZE, TOMBSTONE_SIZE);
	if (memcmp(tombstoneData, "1", TOMBSTONE_SIZE) == 0) {
		record->data = "";
	}
	else {
		memmove(record->data, ph->data + offSet + TOMBSTONE_SIZE + PAGE_INFO_SIZE, recordLength);
	}

	unpinPage(bp, ph);

	return RC_OK;
}

/**
*This function gets the number of tuples.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   RID
* @param   Record
* @return  RC
* @since   2015-11-05
*/
RC getRecordWithEmptyRecData(RM_TableData *rel, RID id, Record *record) {

	int recordLength = 0;
	PageNumber pNum = id.page;
	int slotNum = id.slot;
	int offSet = 0;
	char *recordToUpdate = NULL;

	BM_PageHandle *ph = MAKE_PAGE_HANDLE();
	BM_BufferPool *bp = (BM_BufferPool *)rel->mgmtData;
	record->id = id;
	recordLength = getRecordSize(rel->schema);
	if (pinPage(bp, ph, pNum) != RC_OK) {
		free(ph);
		return RC_PIN_PAGE_FAILED;
	}
	offSet = (recordLength + TOMBSTONE_SIZE) * (slotNum - 1);
	//printf(" \n %s \n", ph->data + offSet + TOMBSTONE_SIZE + PAGE_INFO_SIZE);
	memmove(record->data, ph->data + offSet + TOMBSTONE_SIZE + PAGE_INFO_SIZE, recordLength);

	return RC_OK;
}

/**
*This function gives the RID of the previous deleted record.
* RID.slot=-1 and RID.page=-1 implies there is no previousRID.
*
* @author  Anand N
* @param   RM_TableData
* @param   RID rid of the record to be deleted
* @param   RID prev result Rid
* @param   RID next result Rid
* @param   Record
* @return  RC
* @since   2015-11-05
*/
RC getPreviousAndNextDeletedRecordId(RM_TableData *rel, RID id, Record *prevRec, RID *nextResultRid) {
	BM_PageHandle *bmPh = MAKE_PAGE_HANDLE();
	BM_BufferPool *bm = (BM_BufferPool *)rel->mgmtData;
	int flag = 0;
	prevRec->id.page = -1;
	prevRec->id.slot = -1;
	prevRec->data = "";
	nextResultRid->page = -1;
	nextResultRid->slot = -1;
	int offSet = 0;
	int recordLength = 0;
	recordLength = getRecordSize(rel->schema);

	RID currRid;
	RID prevRid;
	//get the page info
	RM_TABLE_INFO *rmTableInfo = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));
	rmTableInfo->s = (Schema *)malloc(sizeof(Schema));

	if (pinPage(bm, bmPh, 0) != RC_OK) {
		free(bmPh);
		return RC_PIN_PAGE_FAILED;
	}
	deSerializeTableInformation(bmPh->data, rmTableInfo);
	unpinPage(bm, bmPh);

	//from RM_TableInfo if there are no free rec i.e, if FFR id points to -1,-1 then return -1,-1 for the prevFreeRec and NextFreeRec
	if (rmTableInfo->firstFreeRec->page == -1 || rmTableInfo->firstFreeRec->slot == -1)
	{
		free(bmPh);
		free(rmTableInfo);
		return RC_OK;
	}

	// If the record to be deleted is the lesser than record to be deleted then return that 
	// there is no previousRID.
	if (rmTableInfo->firstFreeRec->page > id.page
		|| (rmTableInfo->firstFreeRec->page == id.page && rmTableInfo->firstFreeRec->slot > id.slot)) {
		nextResultRid->page = rmTableInfo->firstFreeRec->page;
		nextResultRid->slot = rmTableInfo->firstFreeRec->slot;
		free(bmPh);
		free(rmTableInfo);
		return RC_OK;
	}


	currRid.page = rmTableInfo->firstFreeRec->page;
	currRid.slot = rmTableInfo->firstFreeRec->slot;
	prevRid = currRid;
	while (id.page >= currRid.page && flag == 0) {
		//For each page
		if (pinPage(bm, bmPh, currRid.page) != RC_OK) {
			free(bmPh);
			return RC_PIN_PAGE_FAILED;
		}
		if (id.page == currRid.page) {
			//When record is in the same page
			do {
				//If the slots are in the same page
				char *recordData = (char *)malloc(recordLength);
				offSet = (recordLength + TOMBSTONE_SIZE) * (currRid.slot - 1);
				memcpy(recordData, bmPh->data + offSet + TOMBSTONE_SIZE + PAGE_INFO_SIZE, recordLength);
				prevRid = currRid;
				deSerializeEmptyRec(recordData, &currRid);
				prevRec->id = prevRid;
				prevRec->data = recordData;
				nextResultRid->page = currRid.page;
				nextResultRid->slot = currRid.slot;

				if (currRid.page == -1 && currRid.slot == -1)
				{
					free(bmPh);
					unpinPage(bm, bmPh);
					return RC_OK;
				}
			} while (currRid.page == prevRid.page && currRid.slot < id.slot);
			flag = 1;
		}
		else {
			//Extract last rec of the page
			PAGE_INFO *pageInfo = NULL;
			extractPageHeader(bmPh, pageInfo);
			int lastRecordSlotInPage = pageInfo->lastRecord.slot;
			//When record is not in the same page
			do {
				//If the slots are in the same page
				char *recordData = NULL;
				offSet = (recordLength + TOMBSTONE_SIZE) * (currRid.slot - 1);
				memcpy(recordData, bmPh->data + offSet + TOMBSTONE_SIZE + PAGE_INFO_SIZE, recordLength);
				prevRid = currRid;
				deSerializeEmptyRec(recordData, &currRid);
				prevRec->data = recordData;
				prevRec->id = prevRid;
				nextResultRid->page = currRid.page;
				nextResultRid->slot = currRid.slot;
				if (currRid.page == -1 && currRid.slot == -1)
				{
					free(bmPh);
					unpinPage(bm, bmPh);
					return RC_OK;
				}
			} while (currRid.page == prevRid.page && currRid.slot <= lastRecordSlotInPage);
			if (currRid.page > id.page || (currRid.page == id.page && currRid.slot > id.slot)) {
				flag = 1;
			}
			else {
				prevRid = currRid;
			}
		}
		unpinPage(bm, bmPh);
	}

	free(bmPh);
	return RC_OK;
}

/**
*This function starts the scan.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   RM_ScanHandle
* @param   Expr
* @return  RC
* @since   2015-11-02
*/
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {

	ExpressionHandler* expressionHandler = (ExpressionHandler*)malloc(sizeof(ExpressionHandler));
	BM_PageHandle *bmPh = MAKE_PAGE_HANDLE();
	BM_BufferPool *bm = (BM_BufferPool *)rel->mgmtData;
	RM_TABLE_INFO *rmTableInfo = (RM_TABLE_INFO *)malloc(sizeof(RM_TABLE_INFO));
	rmTableInfo->s = (Schema *)malloc(sizeof(Schema));

	scan->rel = rel;
	expressionHandler->cond = (Expr *)cond;
	expressionHandler->ridToScan.page = 1;
	expressionHandler->ridToScan.slot = 1;

	//Pin page 0 to get the last rec Id.
	if (pinPage(bm, bmPh, 0) != RC_OK) {
		free(bmPh);
		return RC_PIN_PAGE_FAILED;
	}
	deSerializeTableInformation(bmPh->data, rmTableInfo);
	expressionHandler->lastRecID = rmTableInfo->LastRecID;
	unpinPage(bm, bmPh);
	free(bmPh);

	scan->mgmtData = (ExpressionHandler*)expressionHandler;

	return RC_OK;
}

/**
*This function gets the next record.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   RID
* @param   Record
* @return  RC
* @since   2015-11-05
*/
RC next(RM_ScanHandle *scan, Record *record) {

	RM_TableData *rel = scan->rel;
	Value *value;
	int flag = 0;
	BM_PageHandle *bmPh = MAKE_PAGE_HANDLE();
	char *bmPh1 = (char *)malloc(PAGE_SIZE);
	BM_BufferPool *bm = (BM_BufferPool *)rel->mgmtData;
	ExpressionHandler* expressionHandler = (ExpressionHandler*)scan->mgmtData;
	Expr *cond = (Expr *)expressionHandler->cond;
	RID id;

	// if the slot or page is greater than the last RID then return invalid page or slot.
	if (expressionHandler->ridToScan.slot > expressionHandler->lastRecID.slot 
			|| expressionHandler->ridToScan.page > expressionHandler->lastRecID.page) {
		unpinPage(bm, bmPh);
		free(bmPh);
		return RC_RM_NO_MORE_TUPLES;
	}

	int recordLength = getRecordSize(rel->schema);
	int i, j;

	for (i = expressionHandler->ridToScan.page; i <= expressionHandler->lastRecID.page; i++)
	{
		if (pinPage(bm, bmPh, i) != RC_OK) {
			free(bmPh);
			return RC_PIN_PAGE_FAILED;
		}
		memmove(bmPh1, bmPh->data, PAGE_SIZE);
		for (j = (expressionHandler->ridToScan.slot); j <= expressionHandler->lastRecID.slot; j++)
		{
			id.page = i;
			id.slot = j;
			int offSet = 0;
			offSet = (recordLength + TOMBSTONE_SIZE) * (j - 1);
			//return empty if we encounter tombstone else populate record data
			char *tombstoneData = (char *)malloc(TOMBSTONE_SIZE);
			memmove(tombstoneData, bmPh1 + offSet + PAGE_INFO_SIZE, TOMBSTONE_SIZE);
			if (memcmp(tombstoneData, "1", TOMBSTONE_SIZE) == 0) {
				record->data = "";
			}
			else {
				memmove(record->data, bmPh1 + offSet + TOMBSTONE_SIZE + PAGE_INFO_SIZE, recordLength);
			}

			evalExpr(record, rel->schema, cond, &value);
			if (record->data == "" ||  !value->v.boolV)
			{
				//If not ok continue with the loop
				continue;
			}
			else {
				// If ok record->data has the next record which satisfies the condition
				// and break since data is retrieved
				flag = 1;
				break;
			}
		}
		free(bmPh1);
		if (flag == 1) {
			break;
		}
		unpinPage(bm, bmPh);
	}
	expressionHandler->ridToScan.page = id.page;
	expressionHandler->ridToScan.slot = id.slot + 1;
	expressionHandler->cond = cond;
	scan->mgmtData = expressionHandler;
	if (flag == 1)
		return RC_OK;
	else
		return RC_RM_NO_MORE_TUPLES;
}

/**
*This function checks if record at position given in RID is a tombstone or not.
*
*
* @author  Anand N
* @param   RM_TableData
* @param   RID
* @return  1 if true else 0
* @since   2015-11-05
*/
int checkIfTombstoneEncountered(RM_TableData *rel, RID id) {
	BM_PageHandle *bmPh = MAKE_PAGE_HANDLE();
	BM_BufferPool *bm = (BM_BufferPool *)rel->mgmtData;
	int recordLength = getRecordSize(rel->schema);
	int offSet = 0;
	if (pinPage(bm, bmPh, id.page) != RC_OK) {
		free(bmPh);
		return RC_PIN_PAGE_FAILED;
	}
	char *tombStoneData = (char *)malloc(sizeof(10));;
	offSet = (recordLength + TOMBSTONE_SIZE) * (id.slot - 1);
	memcpy(tombStoneData, bmPh->data + offSet + PAGE_INFO_SIZE, sizeof(TOMBSTONE_SIZE));
	if (memcmp(tombStoneData, "1", TOMBSTONE_SIZE) == 0) {
		return 1;
	}
	unpinPage(bm, bmPh);
	return 0;
}
