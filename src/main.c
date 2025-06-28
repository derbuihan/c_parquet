#include <stdio.h>
#include "parquet_reader.h"

int main(void)
{
    int argc = 2;
    char* argv[] = {"c_parquet", "../examples/sample.parquet"};

    printf("C Parquet Reader v0.1\n");

    if (argc != 2)
    {
        printf("Usage: %s <parquet_file>\n", argv[0]);
        return 1;
    }

    printf("Reading Parquet file: %s\n", argv[1]);
    ParquetFile* pf = parquet_open(argv[1]);
    parquet_print_basic_info(pf);
    parquet_close(pf);

    return 0;
}
