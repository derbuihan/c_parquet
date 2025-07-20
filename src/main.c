#include <stdint.h>
#include <stdlib.h>
#include "parquet.h"

#include "thrift.h"

void test_compact_thrift(parquet_footer_t* footer)
{
    printf("\n=== Clean Compact Thrift Parser ===\n");

    thrift_reader_t reader;
    thrift_reader_init(&reader, footer->metadata, footer->metadata_length);

    int16_t root_last_field_id = 0; // ルートレベルのコンテキスト
    int field_count = 0;

    while (reader.position < reader.size && field_count < 10)
    {
        thrift_field_header_t header;

        printf("\n[Field %d] Position %zu:\n", field_count, reader.position);

        if (thrift_read_field_header(&reader, &header, &root_last_field_id) != 0)
            break;

        printf("  Field ID: %d, Compact Type: %d\n", header.field_id, header.type);

        if (header.type == COMPACT_TYPE_STOP)
        {
            printf("  → STOP\n");
            break;
        }
        if (thrift_skip_field(&reader, header.type, &root_last_field_id) != 0)
            break;

        field_count++;
    }

    printf("\nParsed %d fields successfully!\n", field_count);
}

int main(void)
{
    int argc = 2;
    char* argv[] = {"c_parquet", "../examples/simple.parquet"};

    printf("C Parquet Reader v0.1\n");
    if (argc != 2)
    {
        printf("Usage: %s <parquet_file>\n", argv[0]);
        return -1;
    }

    parquet_reader_t reader;
    if (parquet_open(&reader, argv[1]) != 0)
    {
        fprintf(stderr, "Error opening Parquet file: %s\n", argv[1]);
        return -1;
    }

    if (parquet_validate_magic(&reader) != 0)
    {
        fprintf(stderr, "Invalid Parquet file magic in: %s\n", argv[1]);
        parquet_close(&reader);
        return -1;
    }

    printf("Parquet file '%s' opened successfully.\n", argv[1]);

    parquet_footer_t footer;
    if (parquet_read_footer(&reader, &footer) == 0)
    {
        test_compact_thrift(&footer); // ミニマムテスト
    }

    parquet_close(&reader);

    return 0;
}
