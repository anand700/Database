*******************************************************************
SIMPLE STORAGE MANAGER - ASSIGNMENT 1
*******************************************************************

*******************************************************************
INTRODUCTION
*******************************************************************
Storage manager is an application which handles Create, Open, Close, Destroy and all the read/write funtionalities to the files that are stored in the disk.

*******************************************************************
DATA STRUCTURE
*******************************************************************
SM_FileHandler:
	fileName - a pointer that contains the File Name that is opened (default : NULL)
	totalNumPages - It denotes the number of pages in a file of size PAGE_SIZE and the range of this variable is 1 to N (default : 0)
	curPagePos - It denotes the present page in which the file descriptor is pointing to and ranges from 0 to N (default : 0)
	mgmtInfo - It is a pointer which holds the file descriptor of the opened file (default : NULL)

*******************************************************************
FUNCTIONS DESCRIPTION
*******************************************************************
Our implementation is as follows:

createPageFile
- Checks if the file name is valid.
- Creates the file and checks for its successfull execution.
- If the file is created successfully then it fills the complete first page with "\0".
- Once the above file is filled with "\0" then the file descriptor is stored in mgmtinfo of SM_fileHandler for further reference and is kept until the method closePageFile is called.

openPageFile
- opens the file in read write mode and checks if this execution is successfull.
- If the file was opened successfully then the SM_FileHandler is updated with respective informations.
- Last byte location of the file is found by using seek and tell methods using which the total number of pages is determined.
- This information is also in turn stored in the SM_fileHandler.
- The mgmInfo is again updated and the file is not closed.

closePageFile
- Checks if the file handler, that is sent as a parameter is valid and if it is valid then the mgmtinfo member variable is extracted and is closed as it contains the file descriptor of the file that was being used throughout the application.
- file handle is made null and is made available for further file usage.


destroyPageFile
- Checks if the file name that is sent is valid and not null.
- If that is valid then the filename is removed and checked for its successfull execution.

readBlock
- Check the validitiy of Filehandler.
- Check if the page number which is sent as a parameter is not greater than total number of pages in the SM_FileHandler.
- Traverse through the file to the page number requested and read the block and store the data in mempage variable which is again sent as a parameter.
- mgmtinfo is again updated in the FileHandler as the file cursor is updated every time a page is read, wrote or Traversed.

getBlockPos
- Checks the validity of the SM_FileHandler.
- Checks if total number of pages in the file is greater than zero.
- returns the value of the curPagePos in the filehandler.

readFirstBlock 
- Checks the validity of the SM_FileHandler and mgmtInfo in the Handler.
- Resets the file cursor to the beginning of the file irrespective of the file pointer.
- Once the location is set, the first page is read and the data read is stored in mempage which is also a variable that is sent as a parameter.
- curPagePos is updated to the 2nd page once this is completed.

readPreviousBlock 
- Checks the validity of the SM_FileHandler and mgmtInfo in the Handler.
- Postion of the first byte of the previous page is calculated using PAGE_SIZE and curPagePos.
- Position of cursor is set to the previously calculated byte location.
- Once the location is set, the page is read and the data read is stored in mempage which is also a variable that is sent as a parameter.

readCurrentBlock
- Checks the validity of the SM_FileHandler and mgmtInfo in the Handler.
- The block at which the cursor lies is read to a variable which is sent as a parameter to the method.
- Once the location is set the page is read and the data read is stored in mempage which is also a variable that is sent as a parameter.
- curPagePos is updated by incrementing it by 1.

readNextBlock 
- Checks the validity of the SM_FileHandler and mgmtInfo in the Handler.
- Postion of the first byte of the next page is calculated using PAGE_SIZE and curPagePos.
- Position of cursor is set to the previously calculated byte location.
- Once the location is set, the page is read and the data read is stored in mempage which is also a variable that is sent as a parameter.
- curPagePos is updated by incrementing it by 2.

readLastBlock 
- Checks the validity of the SM_FileHandler and mgmtInfo in the Handler.
- Resets the file cursor to the end of the file irrespective of the file pointer.
- Once the byte position of the end of file is determined then the file is decremented by PAGE_SIZE value to determine the first byte location of last page.
- Last page data is read and stored in mempage variable which is sent as a parameter to method.
- curPagePos is updated to the TotalPageNumber value once this is completed.

writeBlock 
- Check the validity of Filehandler and mgmtinfo.
- ensure the capacity if writing to page greater than the total number of pages.
- if page number parameter is greater than total number of pages then append blocks till the number of pages is equal to page number mentioned in parameter. 
- Traverse through the file to the page number requested and write to the block the data that is present in mempage variable which is sent as a parameter.
- mgmtinfo is again updated in the FileHandler as the file cursor is updated every time a page is read, wrote or Traversed.

writeCurrentBlock
- Check the validity of Filehandler and mgmtinfo.
- Traverse through the file to the current position given in the file handler and write to the block the data that is present in mempage variable which is sent as a parameter.
- mgmtinfo is again updated in the FileHandler as the file cursor is updated every time a page is read, wrote or Traversed.

appendEmptyBlock 
- Checks the validity of the Sm_FileHandler and mgmtInfo in the handler.
- The File cursor is traversed to the end of file and then an empty page is added and is filled with "\0" characters.
- The Success of the above execution is verified.
- TotalNumberOfPages is incremented by 1 value.

ensureCapacity 
- Checks the validity of the Sm_FileHandler and mgmtInfo in the handler.
- Initiates a while loop which terminates when the SM_FileHandle’s totalNumPages becomes equal to numberOfPages which is an integer value sent as a parameter to the method.
- Inside the while loop appendEmptyBlock function call is made which ensure those many number of pages are added which In turn ensures the required number of pages exists

*******************************************************************
ADDITIONAL FUNCTIONS DESCRIPTION
*******************************************************************

checkIfFileNameIsValid
- Checks if the file name is valid.
- File name is checked for NULL and if equals returns Invalid File Name.
- Else return File Name is Valid.

checkIfFileHandleIsInitiated
- Checks if File Handle is initiated.
- Checks all the values of File Handle for Null and if equals return File Handle is not initiated.
- Else return File Handle is initiated.


checkIfPageNumberIsValid
- Checks if Page Number is Valid.
- Checks if Page number is lesser than zero and if condition satisfies return Invalid Page Number.
- Else return Page Number is Valid.

writeFromBlock
- write from Block writes more than one block unlike writeBlock() function.
It does the following:
- Checks the validity of Filehandler and mgmtinfo.
- ensure the capacity if writing to page greater than the total number of pages.
- if page number parameter is greater than total number of pages then append blocks till the number of pages is equal to page number mentioned in parameter. 
- if page number parameter is in the last position, this function takes care of adding the number of pages present in the mempage as well which makes it different from the write block function.
- Traverse through the file to the page number requested and write to the block the data that is present in mempage variable which is sent as a parameter.
- mgmtinfo is again updated in the FileHandler as the file cursor is updated every time a page is read, wrote or Traversed.

writeFromCurrentBlock
- write from current Block writes more than one block unlike writeCurrentBlock() function.
It does the following:
- Check the validity of Filehandler and mgmtinfo.
- ensure the capacity if writing to page greater than the total number of pages.
- if page number parameter is in the last position, this function takes care of adding the number of pages present in the mempage as well which makes it different from the write block function.
- Traverse through the file to the current position given in the file handler and write to the block the data that is present in mempage variable which is sent as a parameter.
- mgmtinfo is again updated in the FileHandler as the file cursor is updated every time a page is read, wrote or Traversed.

*******************************************************************
ADDITIONAL TEST CASES DESCRIPTION
*******************************************************************

testTwoFilesOpenFunctionWithSameFH
- tests if two files can be opened having same file handle.

testTwoFilesOpenFunctionWithDifferentFH
- tests if two files can be opened having different file handles.

testOpenFileIfFileAlreadyInUse
- tests if file already in use can be opened in openpagefile() function.

testReadBlockForPageNumberNotExisting
- tests if readblock() function can be executed if the number of page mentioned in the parameter is exceeding the total number of pages.
- tests if readblock() function can be executed if the number of page mentioned in the parameter is invalid.



testReadNextBlockWhenNextBlockIsAbsent
- tests if readNextblock() function can be executed if next block is not existing.

testMethodsForNullValues
- tests if null values are present in the parameters of all the functions defined in storage_manager.c.

testWriteFromBlock
- tests if write is possible to more than one block from the page number which is passed as a parameter.

testWriteFromCurrentBlock
- tests if write is possible to more than one block from current block.

*******************************************************************
HOW TO RUN MAKE FILES IN LINUX
*******************************************************************
- open terminal in linux.
- navigate to the directory where the files are present.
- execute command given below for make file 1:
	make –f MakeFile1
- delete all the *.o files to execute make file 2.
- execute command given below for make file 2:
	make –f MakeFile2
- Note: MakeFile1 executes the test_assign1_1.c, MakeFile2 executes the test_assign1_2.c and make sure all *.o files are deleted before execution.


*******************************************************************
END. THANK YOU.
*******************************************************************


