#ifndef C_PARQUET_PARQUET_H
#define C_PARQUET_PARQUET_H

#include <stdint.h>
#include <stdio.h>

#define PARQUET_MAGIC "PAR1"
#define PARQUET_MAGIC_SIZE 4

typedef struct {
  FILE* file;
  size_t file_size;
  uint8_t* buffer;
} parquet_reader_t;

typedef struct {
  int32_t version;
  int64_t num_rows;
} parquet_metadata_t;

parquet_reader_t* parquet_open(const char* filename);
int parquet_close(parquet_reader_t* reader);
parquet_metadata_t* parquet_read_metadata(parquet_reader_t* reader);
void print_metadata(const parquet_metadata_t* metadata);

#endif  // C_PARQUET_PARQUET_H
