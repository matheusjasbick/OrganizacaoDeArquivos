/*
 * Work 5 - ZIP Code Join with a B-Tree Index
 *
 * Orchestrates the three steps of the exercise:
 *   1. generate_samples() - split "cep.dat" into two overlapping samples.
 *   2. build_index()      - build a B-Tree index ("index.dat") over sample1.dat.
 *   3. generate_result()  - write to "result.dat" the records of sample2 whose
 *                           ZIP code is found in the index (the intersection).
 *
 * Build: gcc main.c sampling.c build_index.c join.c btree.c -o btree_join
 * Run:   ./btree_join
 */
#include <stdio.h>

void generate_samples(void);
void build_index(void);
void generate_result(void);

int main(void)
{
    printf("Step 1: generating random samples (sample1.dat, sample2.dat)...\n");
    generate_samples();

    printf("Step 2: building the B-Tree index (index.dat)...\n");
    build_index();

    printf("Step 3: joining sample2.dat against the index...\n");
    generate_result();

    printf("Done.\n");
    return 0;
}
