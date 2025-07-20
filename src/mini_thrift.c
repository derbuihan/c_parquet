#include "mini_thrift.h"

#include <stdlib.h>
#include <string.h>

void mt_init(MiniThrift* mt, const uint8_t* data, size_t size)
{
    mt->data = data;
    mt->size = size;
    mt->pos = 0;
}

bool mt_read_field(MiniThrift* mt, uint8_t* type, int16_t* field_id)
{
    if (mt->pos + 3 > mt->size)
    {
        return false; // Not enough data
    }

    *type = mt->data[mt->pos++];
    if (*type == MT_STOP)
    {
        return true; // End of fields
    }

    *field_id = (mt->data[mt->pos] << 8) | mt->data[mt->pos + 1];
    mt->pos += 2;
    return true;
}

int32_t mt_read_i32(MiniThrift* mt)
{
    if (mt->pos + 4 > mt->size)
    {
        return 0; // Not enough data
    }
    int32_t value = (mt->data[mt->pos] << 24) | (mt->data[mt->pos + 1] << 16) | (mt->data[mt->pos + 2] << 8) |
        mt->data[mt->pos + 3];
    mt->pos += 4;
    return value;
}

int64_t mt_read_i64(MiniThrift* mt)
{
    if (mt->pos + 8 > mt->size)
    {
        return 0; // Not enough data
    }

    int64_t val = 0;
    for (int i = 0; i < 8; i++)
    {
        val = (val << 8) | mt->data[mt->pos++];
    }
    return val;
}

char* mt_read_string(MiniThrift* mt)
{
    int32_t len = mt_read_i32(mt);
    if (len <= 0 || mt->pos + len > mt->size)
    {
        return NULL; // Not enough data or invalid length
    }

    char* str = malloc(len + 1);
    if (!str)
    {
        return NULL; // Memory allocation failed
    }

    memcpy(str, &mt->data[mt->pos], len);
    str[len] = '\0'; // Null-terminate the string
    mt->pos += len;
    return str;
}
