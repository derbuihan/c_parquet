#include "parquet_metadata.h"

#include <stdlib.h>

#include "mini_thrift.h"

BinaryMetadata* parse_binary_metadata(const ParquetFile* pf)
{
    if (!pf || !pf->footer_data)
    {
        return NULL;
    }

    MiniThrift mt;
    mt_init(&mt, pf->footer_data, pf->footer_length);

    BinaryMetadata* meta = malloc(sizeof(BinaryMetadata));

    uint8_t type;
    int16_t field_id;

    while (mt_read_field(&mt, &type, &field_id))
    {
        printf("Field ID: %d, Type: %d\n", field_id, type);
        if (type == MT_STOP)
        {
            break; // End of fields
        }

        switch (field_id)
        {
        case 1: // version
            if (type == MT_I32)
            {
                meta->version = mt_read_i32(&mt);
            }
            break;
        case 3: // num_rows
            if (type == MT_I64)
            {
                meta->num_rows = mt_read_i64(&mt);
            }
            break;
        case 6: // created_by
            if (type == MT_STRING)
            {
                meta->created_by = mt_read_string(&mt);
            }
            break;
        default:
            break; // Ignore unknown fields
        }
    }
    return meta;
}

// void print_binary_metadata(const BinaryMetadata* meta);
// void free_binary_metadata(BinaryMetadata* meta);
