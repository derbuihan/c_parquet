#include "parquet_reader.h"

#include <stdlib.h>
#include <string.h>

bool parquet_is_valid(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error: Could not open file %s\n", filename);
        return 0;
    }

    fseek(file, -PARQUET_MAGIC_LENGTH, SEEK_END);
    char magic[PARQUET_MAGIC_LENGTH];
    fread(magic, 1, PARQUET_MAGIC_LENGTH, file);
    fclose(file);

    return memcmp(magic, PARQUET_MAGIC, PARQUET_MAGIC_LENGTH) == 0;
}

ParquetFile* parquet_open(const char* filename)
{
    if (!parquet_is_valid(filename))
    {
        printf("Error: Invalid Parquet file %s\n", filename);
        return NULL;
    }

    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }

    ParquetFile* pf = calloc(1, sizeof(ParquetFile));
    pf->file = file;
    pf->filename = strdup(filename);

    // set file size
    fseek(file, 0, SEEK_END);
    pf->file_size = ftell(file);

    // set footer length
    fseek(file, -8, SEEK_END);
    fread(&pf->footer_length, sizeof(uint32_t), 1, file);

    // read footer data
    pf->footer_data = malloc(pf->footer_length);
    long footer_start = pf->file_size - pf->footer_length;
    fseek(file, footer_start, SEEK_SET);
    fread(pf->footer_data, 1, pf->footer_length, pf->file);

    return pf;
}

void parquet_close(ParquetFile* pf)
{
    if (pf)
    {
        if (pf->file)
            fclose(pf->file);
        if (pf->filename)
            free(pf->filename);
        if (pf->footer_data)
            free(pf->footer_data);
        free(pf);
    }
}

void parquet_print_basic_info(const ParquetFile* pf)
{
    if (!pf)
        return;
    printf("Parquet File: %s\n", pf->filename);
    printf("File Size: %ld bytes\n", pf->file_size);
    printf("Footer Length: %u bytes\n", pf->footer_length);
}
