/*
 * Work 4 - COVID-19 Data Aggregation from a Large CSV
 *
 * Streams the "owid-covid-data.csv" file (Our World in Data) through a small
 * table-driven CSV parser and aggregates, per South American country, the total
 * number of confirmed cases and deaths. The file is read in fixed-size chunks,
 * so memory usage stays constant regardless of the file size.
 *
 * The "total_cases" / "total_deaths" columns are cumulative. The final total
 * for a country is the value reported on the MOST RECENT date on which the
 * column was non-empty -- NOT the maximum ever seen. This matters because the
 * death series is occasionally revised downward (e.g. Chile: a peak of 64,497
 * corrected to a final 61,508); taking the maximum would report the stale peak
 * and over-count the continent total.
 *
 * Build: gcc covid_analysis.c csv_parser.c -o covid_analysis -lm
 * Run:   ./covid_analysis
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv_parser.h"

#define READ_BUFFER_SIZE 8192
#define MAX_COUNTRIES 32
#define TARGET_CONTINENT "South America"

/* Column indices in the Our World in Data CSV layout. */
#define COL_CONTINENT    1
#define COL_LOCATION     2
#define COL_DATE         3
#define COL_TOTAL_CASES  4
#define COL_TOTAL_DEATHS 7
#define MIN_COLUMNS      8

#define DATE_LEN 16   /* "YYYY-MM-DD" plus slack */

typedef struct {
    char location[64];
    double total_cases;
    double total_deaths;
    char date_cases[DATE_LEN];   /* date of the latest non-empty total_cases  */
    char date_deaths[DATE_LEN];  /* date of the latest non-empty total_deaths */
} CountryData;

typedef struct {
    int row_count;                        /* data rows that matched the filter */
    int header_skipped;                   /* becomes 1 after the header row     */
    int overflow;                         /* set if MAX_COUNTRIES was exceeded  */
    CountryData countries[MAX_COUNTRIES];
    int country_count;
} Accumulator;

/* Returns the index of a country, creating a fresh entry the first time.
 * Returns -1 (and flags overflow) if the table is full -- so the caller never
 * silently drops a country from the total. */
static int country_index(Accumulator *acc, const char *name)
{
    for (int i = 0; i < acc->country_count; i++) {
        if (strcmp(acc->countries[i].location, name) == 0) {
            return i;
        }
    }
    if (acc->country_count >= MAX_COUNTRIES) {
        acc->overflow = 1;
        return -1;
    }
    int i = acc->country_count++;
    strncpy(acc->countries[i].location, name, sizeof(acc->countries[i].location) - 1);
    acc->countries[i].location[sizeof(acc->countries[i].location) - 1] = '\0';
    acc->countries[i].total_cases = 0;
    acc->countries[i].total_deaths = 0;
    acc->countries[i].date_cases[0] = '\0';
    acc->countries[i].date_deaths[0] = '\0';
    return i;
}

/* Called by the CSV parser once per assembled row. */
static void on_row(char **cols, int ncols, void *user_data)
{
    Accumulator *acc = (Accumulator *)user_data;

    /* The very first row is the header: skip it. */
    if (!acc->header_skipped) {
        acc->header_skipped = 1;
        return;
    }

    if (ncols < MIN_COLUMNS) {
        return;
    }
    if (strcmp(cols[COL_CONTINENT], TARGET_CONTINENT) != 0) {
        return;
    }

    int idx = country_index(acc, cols[COL_LOCATION]);
    if (idx < 0) {              /* table full: overflow already flagged */
        return;
    }

    const char *date = cols[COL_DATE];

    /* Keep the value from the most recent date on which the column is present.
     * ISO dates (YYYY-MM-DD) compare correctly with strcmp, and each column is
     * tracked independently in case cases and deaths end on different dates.
     * strcmp(date, "") > 0 for any real date, so the first value initialises. */
    if (cols[COL_TOTAL_CASES][0] != '\0' &&
        strcmp(date, acc->countries[idx].date_cases) > 0) {
        acc->countries[idx].total_cases = atof(cols[COL_TOTAL_CASES]);
        snprintf(acc->countries[idx].date_cases, DATE_LEN, "%s", date);
    }
    if (cols[COL_TOTAL_DEATHS][0] != '\0' &&
        strcmp(date, acc->countries[idx].date_deaths) > 0) {
        acc->countries[idx].total_deaths = atof(cols[COL_TOTAL_DEATHS]);
        snprintf(acc->countries[idx].date_deaths, DATE_LEN, "%s", date);
    }

    acc->row_count++;
}

int main(void)
{
    Accumulator acc;
    acc.row_count = 0;
    acc.header_skipped = 0;
    acc.overflow = 0;
    acc.country_count = 0;

    char *buffer = malloc(READ_BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Error: not enough memory for the read buffer.\n");
        return 1;
    }

    CSVParser parser;
    CSVParser_init(&parser);

    FILE *file = fopen("owid-covid-data.csv", "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open 'owid-covid-data.csv'.\n");
        free(buffer);
        return 1;
    }

    int read = fread(buffer, 1, READ_BUFFER_SIZE, file);
    while (read > 0) {
        CSVParser_processLines(&parser, buffer, read, on_row, &acc);
        read = fread(buffer, 1, READ_BUFFER_SIZE, file);
    }
    fclose(file);

    /* Flush any final row not terminated by a newline. */
    CSVParser_processLines(&parser, "\n", 1, on_row, &acc);

    free(buffer);

    printf("============================================================\n");
    printf("  TOTAL CASES AND DEATHS - SOUTH AMERICA (COVID-19)\n");
    printf("============================================================\n");
    printf("%-30s %15s %15s\n", "Country", "Total cases", "Total deaths");
    printf("------------------------------------------------------------\n");

    double sum_cases = 0;
    double sum_deaths = 0;

    for (int i = 0; i < acc.country_count; i++) {
        printf("%-30s %15.0f %15.0f\n",
               acc.countries[i].location,
               acc.countries[i].total_cases,
               acc.countries[i].total_deaths);
        sum_cases += acc.countries[i].total_cases;
        sum_deaths += acc.countries[i].total_deaths;
    }

    printf("============================================================\n");
    printf("%-30s %15.0f %15.0f\n", "GRAND TOTAL", sum_cases, sum_deaths);
    printf("============================================================\n");
    printf("Data rows processed: %d\n", acc.row_count);
    printf("Countries found: %d\n", acc.country_count);

    if (acc.overflow) {
        fprintf(stderr,
                "\nWARNING: more than %d countries matched the filter; the total "
                "above is INCOMPLETE. Increase MAX_COUNTRIES and re-run.\n",
                MAX_COUNTRIES);
        return 2;
    }

    return 0;
}
