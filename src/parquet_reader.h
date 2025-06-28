#ifndef PARQUET_READER_H
#define PARQUET_READER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define PARQUET_MAGIC "PAR1"
#define PARQUET_MAGIC_LENGTH 4

typedef struct
{
    FILE* file;
    char* filename;
    long file_size;
    uint32_t footer_length;
    uint8_t* footer_data;
} ParquetFile;

bool parquet_is_valid(const char* filename);
ParquetFile* parquet_open(const char* filename);
void parquet_close(ParquetFile* pf);
void parquet_print_basic_info(const ParquetFile* pf);

#endif // PARQUET_READER_H
