/*
 * Disk-based B-Tree index.
 *
 * The whole tree lives in a single file. Byte 0 holds a small header pointing at
 * the current root page; every other page is a fixed-size block written at a
 * byte offset that is used as its "pointer". Each element stores a key (a ZIP
 * code) and the byte offset of the matching record in the original data file,
 * so a successful search yields the position of the full record.
 */
#ifndef BTREE_H
#define BTREE_H

#define KEY_SIZE 8       /* key length in bytes (a ZIP code)          */
#define MAX_ELEMENTS 300 /* maximum number of elements stored per page */

#include <stdio.h>

typedef struct _BTree BTree;
typedef struct _BTreeHeader BTreeHeader;
typedef struct _BTreeElement BTreeElement;
typedef struct _BTreePage BTreePage;

struct _BTreeHeader {
    long root;           /* byte offset of the root page */
};

struct _BTree {
    BTreeHeader *header;
    FILE *file;
};

struct _BTreeElement {
    char key[KEY_SIZE];
    long record_offset;  /* offset of the record in the data file      */
    long right_child;    /* byte offset of the page to the right of key */
};

struct _BTreePage {
    int element_count;
    long left_child;                    /* leftmost child; 0 means a leaf */
    BTreeElement elements[MAX_ELEMENTS];
};

BTree *BTree_Open(const char *file_name);
void BTree_Close(BTree *tree);
void BTree_Insert(BTree *tree, char key[KEY_SIZE], long record_offset);
void BTree_PrintDebug(BTree *tree);
long BTree_Search(BTree *tree, char key[KEY_SIZE]);

#endif
