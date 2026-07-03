#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "btree.h"

/* Allocates a fresh, empty page in memory. */
static BTreePage *btree_alloc_page(void)
{
    BTreePage *page = malloc(sizeof(BTreePage));
    page->element_count = 0;
    page->left_child = 0;
    for (int i = 0; i < MAX_ELEMENTS; i++) {
        memset(page->elements[i].key, '\0', KEY_SIZE);
        page->elements[i].right_child = 0;
        page->elements[i].record_offset = 0;
    }
    return page;
}

static void btree_free_page(BTreePage *page)
{
    if (page) {
        free(page);
    }
}

/*
 * Opens an existing index file, or creates one initialized with a header and an
 * empty root page. The file is left open in read/write ("rb+") mode.
 */
BTree *BTree_Open(const char *file_name)
{
    BTree *tree = NULL;
    BTreeHeader *header = NULL;
    BTreePage *root = NULL;
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        file = fopen(file_name, "wb");
        if (!file) {
            fprintf(stderr, "File %s could not be created\n", file_name);
            return NULL;
        }
        header = malloc(sizeof(BTreeHeader));
        header->root = sizeof(BTreeHeader);   /* root sits right after the header */
        fwrite(header, sizeof(BTreeHeader), 1, file);
        root = btree_alloc_page();
        fwrite(root, sizeof(BTreePage), 1, file);
        btree_free_page(root);
        free(header);
    }
    fclose(file);
    file = fopen(file_name, "rb+");
    tree = malloc(sizeof(BTree));
    tree->file = file;
    tree->header = malloc(sizeof(BTreeHeader));
    fread(tree->header, sizeof(BTreeHeader), 1, file);
    return tree;
}

static int btree_compare(const void *e1, const void *e2)
{
    return strncmp(((BTreeElement *)e1)->key, ((BTreeElement *)e2)->key, KEY_SIZE);
}

static void btree_write_header(BTree *tree)
{
    fseek(tree->file, 0, SEEK_SET);
    fwrite(tree->header, sizeof(BTreeHeader), 1, tree->file);
}

void BTree_Close(BTree *tree)
{
    if (tree) {
        btree_write_header(tree);
        fclose(tree->file);
        free(tree->header);
        free(tree);
    }
}

/*
 * Splits a full page. `overflow` is the element that did not fit. The upper half
 * of the elements plus the overflow move to a brand-new page; the middle element
 * is returned so the caller can push it up into the parent.
 */
static BTreeElement *btree_split(BTree *tree, long page_offset,
                                 BTreePage *page, BTreeElement *overflow)
{
    int i;
    BTreeElement aux;
    BTreeElement *promoted;
    BTreePage *new_page;

    /* If the overflow element is smaller than the last one on the page, swap
       them and re-sort so the true largest element ends up in the overflow. */
    if (btree_compare(overflow, &page->elements[page->element_count - 1]) < 0) {
        aux = *overflow;
        *overflow = page->elements[page->element_count - 1];
        page->elements[page->element_count - 1] = aux;
        qsort(page->elements, page->element_count, sizeof(BTreeElement), btree_compare);
    }

    /* Move the upper half of the elements into the new page. */
    new_page = btree_alloc_page();
    for (i = MAX_ELEMENTS / 2 + 1; i < MAX_ELEMENTS; i++) {
        new_page->elements[new_page->element_count] = page->elements[i];
        new_page->element_count++;
    }
    new_page->elements[new_page->element_count] = *overflow;
    new_page->element_count++;
    page->element_count = MAX_ELEMENTS / 2;

    fseek(tree->file, page_offset, SEEK_SET);
    fwrite(page, sizeof(BTreePage), 1, tree->file);

    /* Build the element that will be promoted to the parent. */
    promoted = malloc(sizeof(BTreeElement));
    new_page->left_child = page->elements[MAX_ELEMENTS / 2].right_child;
    memcpy(promoted->key, page->elements[MAX_ELEMENTS / 2].key, KEY_SIZE);
    promoted->record_offset = page->elements[MAX_ELEMENTS / 2].record_offset;

    /* Append the new page at the end of the file; its offset becomes the right
       child of the promoted element. */
    fseek(tree->file, 0, SEEK_END);
    promoted->right_child = ftell(tree->file);
    fwrite(new_page, sizeof(BTreePage), 1, tree->file);
    btree_free_page(new_page);
    return promoted;
}

/*
 * Recursively inserts a key. Returns NULL when the subtree absorbed the key, or
 * the element that must be pushed up when the child page had to split.
 */
static BTreeElement *btree_insert_recursive(BTree *tree, long page_offset,
                                            char key[KEY_SIZE], long record_offset)
{
    int i;
    long child_offset;
    BTreeElement *result = NULL;
    BTreeElement *split_element = NULL;
    BTreeElement overflow;

    BTreePage *page = btree_alloc_page();
    fseek(tree->file, page_offset, SEEK_SET);
    fread(page, sizeof(BTreePage), 1, tree->file);

    if (page->left_child == 0) {          /* leaf page */
        if (page->element_count < MAX_ELEMENTS) {   /* fits in the leaf */
            memcpy(page->elements[page->element_count].key, key, KEY_SIZE);
            page->elements[page->element_count].record_offset = record_offset;
            page->element_count++;
            qsort(page->elements, page->element_count, sizeof(BTreeElement), btree_compare);
            fseek(tree->file, page_offset, SEEK_SET);
            fwrite(page, sizeof(BTreePage), 1, tree->file);
        } else {                                    /* does not fit: split */
            memcpy(overflow.key, key, KEY_SIZE);
            overflow.record_offset = record_offset;
            overflow.right_child = 0;
            result = btree_split(tree, page_offset, page, &overflow);
        }
    } else {                               /* internal page */
        child_offset = page->left_child;
        for (i = 0; i < page->element_count; i++) {
            if (strncmp(key, page->elements[i].key, KEY_SIZE) < 0) {
                break;
            }
            child_offset = page->elements[i].right_child;
        }
        split_element = btree_insert_recursive(tree, child_offset, key, record_offset);
        if (split_element) {               /* the child split; absorb its median */
            if (page->element_count < MAX_ELEMENTS) {
                page->elements[page->element_count] = *split_element;
                page->element_count++;
                qsort(page->elements, page->element_count, sizeof(BTreeElement), btree_compare);
                fseek(tree->file, page_offset, SEEK_SET);
                fwrite(page, sizeof(BTreePage), 1, tree->file);
                free(split_element);
            } else {                       /* this page also overflows */
                result = btree_split(tree, page_offset, page, split_element);
                free(split_element);
            }
        }
    }

    btree_free_page(page);
    return result;
}

void BTree_Insert(BTree *tree, char key[KEY_SIZE], long record_offset)
{
    BTreeElement *split_element =
        btree_insert_recursive(tree, tree->header->root, key, record_offset);

    if (split_element) {
        /* The root split: grow the tree by one level. */
        BTreePage *new_root = btree_alloc_page();
        new_root->element_count = 1;
        new_root->elements[0] = *split_element;
        new_root->left_child = tree->header->root;
        fseek(tree->file, 0, SEEK_END);
        tree->header->root = ftell(tree->file);
        fwrite(new_root, sizeof(BTreePage), 1, tree->file);
        btree_write_header(tree);
        btree_free_page(new_root);
        free(split_element);
    }
}

void BTree_PrintDebug(BTree *tree)
{
    long page_offset;
    int i;
    BTreePage *page = btree_alloc_page();
    fseek(tree->file, sizeof(BTreeHeader), SEEK_SET);
    page_offset = ftell(tree->file);
    while (fread(page, sizeof(BTreePage), 1, tree->file)) {
        printf(page_offset == tree->header->root ? "* " : "  ");
        printf("%5ld => %5ld|", page_offset, page->left_child);
        for (i = 0; i < page->element_count; i++) {
            printf("(%.8s)|%5ld|", page->elements[i].key, page->elements[i].right_child);
        }
        printf("\n");
        page_offset = ftell(tree->file);
    }
    btree_free_page(page);
}

static long btree_search_recursive(BTree *tree, long page_offset, char key[KEY_SIZE])
{
    if (page_offset == 0) {
        return -1;
    }
    BTreePage *page = btree_alloc_page();
    fseek(tree->file, page_offset, SEEK_SET);
    fread(page, sizeof(BTreePage), 1, tree->file);

    long child_offset = page->left_child;
    for (int i = 0; i < page->element_count; i++) {
        if (strncmp(key, page->elements[i].key, KEY_SIZE) == 0) {
            long result = page->elements[i].record_offset;
            btree_free_page(page);
            return result;
        }
        if (strncmp(key, page->elements[i].key, KEY_SIZE) < 0) {
            break;
        }
        child_offset = page->elements[i].right_child;
    }
    btree_free_page(page);
    return btree_search_recursive(tree, child_offset, key);
}

long BTree_Search(BTree *tree, char key[KEY_SIZE])
{
    return btree_search_recursive(tree, tree->header->root, key);
}
