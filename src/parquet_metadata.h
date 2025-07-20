#ifndef PARQUET_METADATA_H
#define PARQUET_METADATA_H

#include "parquet_reader.h"

typedef struct
{
    int32_t version;
    int64_t num_rows;
    char* created_by; // Optional, can be NULL
} BinaryMetadata;

BinaryMetadata* parse_binary_metadata(const ParquetFile* pf);
// void print_binary_metadata(const BinaryMetadata* meta);
// void free_binary_metadata(BinaryMetadata* meta);

#endif // PARQUET_METADATA_H
