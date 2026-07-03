/*
 * Step 1 of Work 5: build two random samples of the data file.
 *
 * Each sample contains EXACTLY 80% of the records of "cep.dat", chosen at random
 * and written in SHUFFLED order. This is done with a Fisher-Yates shuffle over an
 * array of record indices: after a full shuffle, the first 80% of the indices are
 * a uniformly random subset, already in random order. The two samples come from
 * two independent shuffles, so they overlap only partially -- which is what makes
 * the later join step meaningful.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RECORD_SIZE 300
#define SAMPLE_RATE 0.8
#define INPUT_FILE  "cep.dat"

/* Fisher-Yates shuffle of idx[0..n-1]. */
static void shuffle(int *idx, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }
}

/* Write the first `count` records referenced by idx[] from `input` to `path`,
 * in idx order (i.e. shuffled). Returns 0 on success, -1 on failure. */
static int write_sample(FILE *input, const char *path, int *idx, int count)
{
    FILE *out = fopen(path, "wb");
    if (out == NULL) {
        printf("Error opening %s\n", path);
        return -1;
    }
    char record[RECORD_SIZE];
    for (int i = 0; i < count; i++) {
        fseek(input, (long)idx[i] * RECORD_SIZE, SEEK_SET);
        if (fread(record, RECORD_SIZE, 1, input) != 1) {
            printf("Error reading record %d\n", idx[i]);
            fclose(out);
            return -1;
        }
        fwrite(record, RECORD_SIZE, 1, out);
    }
    fclose(out);
    return 0;
}

void generate_samples(void)
{
    FILE *input = fopen(INPUT_FILE, "rb");
    if (input == NULL) {
        printf("Error opening %s\n", INPUT_FILE);
        return;
    }

    /* Number of records in the input file. */
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    int total = (int)(size / RECORD_SIZE);
    int count = (int)(total * SAMPLE_RATE);   /* exactly 80%, floored */

    int *idx = malloc((size_t)total * sizeof(int));
    if (idx == NULL) {
        printf("Not enough memory for %d record indices\n", total);
        fclose(input);
        return;
    }
    for (int i = 0; i < total; i++) {
        idx[i] = i;
    }

    /* Seed ONCE. Re-seeding with time() before the second shuffle could reuse
       the same second and produce two identical samples. */
    srand((unsigned)time(NULL));

    shuffle(idx, total);
    write_sample(input, "sample1.dat", idx, count);

    shuffle(idx, total);      /* reshuffle -> an independent 80% subset */
    write_sample(input, "sample2.dat", idx, count);

    printf("  %d records total, %d per sample (%.0f%%)\n",
           total, count, 100.0 * SAMPLE_RATE);

    free(idx);
    fclose(input);
}
