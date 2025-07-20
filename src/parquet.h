#ifndef C_PARQUET_PARQUET_H
#define C_PARQUET_PARQUET_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PARQUET_MAGIC "PAR1"
#define PARQUET_MAGIC_SIZE 4

typedef struct
{
    uint8_t* data;
    size_t size;
    size_t position;
} parquet_buffer_t;

typedef struct
{
    FILE* file;
    size_t file_size;
    parquet_buffer_t buffer;
} parquet_reader_t;

typedef struct
{
    uint32_t metadata_length;
    uint8_t* metadata;
} parquet_footer_t;

int parquet_open(parquet_reader_t* reader, const char* filename);
int parquet_validate_magic(parquet_reader_t* reader);
int parquet_close(parquet_reader_t* reader);
int parquet_read_footer(parquet_reader_t* reader, parquet_footer_t* footer);

#endif // C_PARQUET_PARQUET_H
