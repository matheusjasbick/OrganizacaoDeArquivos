/*
 * Step 2 of Work 5: build a B-Tree index over the first sample.
 *
 * Reads every record of "sample1.dat", extracts its ZIP code (8 bytes at offset
 * 290 of the 300-byte record) and inserts it into the B-Tree stored in
 * "index.dat", together with the byte offset of the record. After this step the
 * index alone is enough to test whether a given ZIP code appears in sample1.
 */
#include <stdio.h>
#include <string.h>
#include "btree.h"

#define RECORD_SIZE 300
#define ZIP_OFFSET  290   /* byte position of the ZIP code inside a record */

void build_index(void)
{
    FILE *file = fopen("sample1.dat", "rb");
    if (file == NULL) {
        printf("Error opening sample1.dat\n");
        return;
    }

    /* Start from a clean index so re-runs never build on top of a stale tree. */
    remove("index.dat");
    BTree *tree = BTree_Open("index.dat");
    if (tree == NULL) {
        fclose(file);
        return;
    }

    char record[RECORD_SIZE];
    char key[KEY_SIZE];
    long offset = 0;

    while (fread(record, RECORD_SIZE, 1, file) == 1) {
        memcpy(key, record + ZIP_OFFSET, KEY_SIZE);
        BTree_Insert(tree, key, offset);
        offset += RECORD_SIZE;
    }

    BTree_Close(tree);
    fclose(file);
}
