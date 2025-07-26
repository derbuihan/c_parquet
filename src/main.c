#include <stdint.h>
#include <stdlib.h>
#include "parquet.h"

#include "thrift.h"

int parse_file_metadata(thrift_reader_t* reader)
{
    int32_t version = thrift_read_varint32(reader);
    printf("Version: %d\n", version);

    return 0;
}

int main(void)
{
    // Initialize command line arguments
    int argc = 2;
    char* argv[] = {"c_parquet", "../examples/simple.parquet"};

    printf("C Parquet Reader v0.1\n");
    if (argc != 2)
    {
        printf("Usage: %s <parquet_file>\n", argv[0]);
        return -1;
    }

    // Open the Parquet file
    parquet_reader_t* reader = parquet_open(argv[1]);
    if (!reader)
    {
        printf("Error: Could not open Parquet file '%s'.\n", argv[1]);
        return -1;
    }
    printf("Parquet file '%s' opened successfully.\n", argv[1]);

    // Read the metadata
    parquet_metadata_t* metadata = parquet_read_metadata(reader);
    if (!metadata)
    {
        printf("Error: Could not read metadata from Parquet file '%s'.\n", argv[1]);
        parquet_close(reader);
        return -1;
    }
    print_metadata(metadata);

    // Read the footer
    parquet_close(reader);

    return 0;
}
