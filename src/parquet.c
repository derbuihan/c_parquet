#include "parquet.h"

#include <stdlib.h>
#include <string.h>

int parquet_open(parquet_reader_t* reader, const char* filename)
{
    reader->file = fopen(filename, "rb");
    if (!reader->file)
        return -1;

    // Get file size
    fseek(reader->file, 0, SEEK_END);
    reader->file_size = ftell(reader->file);
    fseek(reader->file, 0, SEEK_SET);

    reader->buffer.data = malloc(reader->file_size);
    reader->buffer.size = reader->file_size;
    reader->buffer.position = 0;

    fread(reader->buffer.data, 1, reader->file_size, reader->file);

    return 0;
}

int parquet_validate_magic(parquet_reader_t* reader)
{
    if (memcmp(reader->buffer.data, PARQUET_MAGIC, PARQUET_MAGIC_SIZE) != 0)
        return -1;

    size_t footer_offset = reader->file_size - PARQUET_MAGIC_SIZE;
    if (memcmp(reader->buffer.data + footer_offset, PARQUET_MAGIC, PARQUET_MAGIC_SIZE) != 0)
        return -1;

    return 0;
}

int parquet_close(parquet_reader_t* reader)
{
    if (reader->file)
        fclose(reader->file);
    if (reader->buffer.data)
        free(reader->buffer.data);
    return 0;
}

int parquet_read_footer(parquet_reader_t* reader, parquet_footer_t* footer)
{
    size_t footer_size_pos = reader->file_size - PARQUET_MAGIC_SIZE - 4;

    uint8_t* pos = reader->buffer.data + footer_size_pos;
    footer->metadata_length = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);

    size_t metadata_pos = footer_size_pos - footer->metadata_length;
    footer->metadata = reader->buffer.data + metadata_pos;

    return 0;
}
