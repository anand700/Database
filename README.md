****************************************************************************************
Introduction - Partial Database
****************************************************************************************
Consists of the following components:
1. Storage Manager - Storage manager is an application which handles Create, Open, Close, Destroy and all the read/write funtionalities to the files that are stored in the disk.

2. Record Manager - Record Manager is an application that handles tables with a fixed schema where clients can insert records, delete records, update records, and scan through the records in a table. We have implemented optional extension Tids and tombstone in our project.

3. Buffer Manager - Buffer Manager is an application that manages a buffer of blocks in memory including reading/flushing to disk and block replacement.(flushing blocks to disk to make space for reading new blocks from disk)

4. B+ Tree Manager - A B+ tree is an n-ary tree with a variable but often large number of children per node. A B+ tree consists of a root, internal nodes and leaves. The root may be either a leaf or a node with two or more children
****************************************************************************************
