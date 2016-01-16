#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"

#define MAKE_TREE()	((Tree *) malloc (sizeof(Tree)))

#define MAKE_TREE_INFO() ((TreeInfo *) malloc (sizeof(TreeInfo)))

typedef struct Tree {
	struct Tree *parent;
	struct Tree **ptrs;
	struct Tree *next;
	struct Tree *prev;
	RID *records;
	bool isLeafNode;
	int *keys;
	int Keysize;
} Tree;

typedef struct TreeInfo {
	int numOfInserts;
	int numberOfValuesPerNode;
	int numOfNodes;
	void *mgmtData;
	void *bufferMgrPtr;
} TreeInfo;

//Track the key node and the position
typedef struct TrackKey {
	struct Tree *node;
	int pos;
} TrackKey;

// structure for accessing btrees
typedef struct BTreeHandle {
	DataType keyType;
	char *idxId;
	void *mgmtData;
} BTreeHandle;

typedef struct BT_ScanHandle {
	BTreeHandle *tree;
	void *mgmtData;
} BT_ScanHandle;

// init and shutdown index manager
extern RC initIndexManager(void *mgmtData);
extern RC shutdownIndexManager();

// create, destroy, open, and close an btree index
extern RC createBtree(char *idxId, DataType keyType, int n);
extern RC openBtree(BTreeHandle **tree, char *idxId);
extern RC closeBtree(BTreeHandle *tree);
extern RC deleteBtree(char *idxId);

// access information about a b-tree
extern RC getNumNodes(BTreeHandle *tree, int *result);
extern RC getNumEntries(BTreeHandle *tree, int *result);
extern RC getKeyType(BTreeHandle *tree, DataType *result);

// index access
extern RC findKey(BTreeHandle *tree, Value *key, RID *result);
extern RC insertKey(BTreeHandle *tree, Value *key, RID rid);
extern RC deleteKey(BTreeHandle *tree, Value *key);
extern RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle);
extern RC nextEntry(BT_ScanHandle *handle, RID *result);
extern RC closeTreeScan(BT_ScanHandle *handle);

// debug and test functions
extern char *printTree(BTreeHandle *tree);

#endif // BTREE_MGR_H
