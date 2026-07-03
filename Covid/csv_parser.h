/*
 * Minimal streaming CSV parser.
 *
 * Feed the parser chunks of bytes with CSVParser_processLines(); it keeps its
 * own state across calls, so a record may be split across several chunks. Once
 * a full row is assembled it invokes the callback with the array of fields.
 * This makes it possible to parse arbitrarily large CSV files with a small,
 * fixed-size read buffer.
 */
#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#define CSV_MAX_ROW_SIZE 4096
#define CSV_MAX_COLS 512

typedef struct {
    int state;
    char buffer[CSV_MAX_ROW_SIZE];
    int buffer_length;
    char *fields[CSV_MAX_COLS];
    int field_count;
} CSVParser;

void CSVParser_init(CSVParser *parser);
void CSVParser_processLines(CSVParser *parser, const char *input, int input_size,
                            void (*callback)(char **, int, void *), void *user_data);

#endif
