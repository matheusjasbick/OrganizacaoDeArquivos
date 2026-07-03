/*
 * Step 3 of Work 5: join the second sample against the index.
 *
 * For every record of "sample2.dat", the ZIP code is looked up in the B-Tree
 * index built from sample1. Records whose ZIP code is present in the index (i.e.
 * records that also appear in sample1) are written to "result.dat". This is the
 * intersection of the two samples, computed with one B-Tree lookup per record.
 */
#include <stdio.h>
#include <string.h>
#include "btree.h"

#define RECORD_SIZE 300
#define ZIP_OFFSET  290

void generate_result(void)
{
    FILE *file = fopen("sample2.dat", "rb");
    FILE *result = fopen("result.dat", "wb");

    if (file == NULL || result == NULL) {
        printf("Error opening sample2.dat / result.dat\n");
        if (file) fclose(file);
        if (result) fclose(result);
        return;
    }

    BTree *tree = BTree_Open("index.dat");
    if (tree == NULL) {
        fclose(file);
        fclose(result);
        return;
    }

    char record[RECORD_SIZE];
    char key[KEY_SIZE];
    long matches = 0;

    while (fread(record, RECORD_SIZE, 1, file) == 1) {
        memcpy(key, record + ZIP_OFFSET, KEY_SIZE);
        if (BTree_Search(tree, key) != -1) {
            fwrite(record, RECORD_SIZE, 1, result);
            matches++;
        }
    }

    printf("Matching records written to result.dat: %ld\n", matches);

    BTree_Close(tree);
    fclose(file);
    fclose(result);
}
