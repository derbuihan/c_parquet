#include "thrift.h"

#include <stdio.h>


int thrift_reader_init(thrift_reader_t* reader, uint8_t* data, size_t size)
{
    reader->data = data;
    reader->size = size;
    reader->position = 0;
    return 0;
}

int thrift_read_varint32(thrift_reader_t* reader, uint32_t* value)
{
    *value = 0;
    uint8_t shift = 0;
    while (reader->position < reader->size)
    {
        uint8_t byte = reader->data[reader->position++];
        *value |= (byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
        if (shift >= 32)
            return -1; // Overflow
    }
    return 0;
}

int thrift_read_varint64(thrift_reader_t* reader, uint64_t* value)
{
    *value = 0;
    uint8_t shift = 0;
    while (reader->position < reader->size)
    {
        uint8_t byte = reader->data[reader->position++];
        *value |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
        if (shift >= 64)
            return -1; // Overflow
    }
    return 0;
}

int thrift_read_field_header(thrift_reader_t* reader, thrift_field_header_t* header, int16_t* last_field_id)
{
    if (reader->position >= reader->size)
        return -1;

    uint8_t first_byte = reader->data[reader->position++];
    header->type = first_byte & 0x0F;
    int8_t delta = (first_byte & 0xF0) >> 4;

    // STOPフィールドの場合は、field IDを読み込まない
    if (header->type == COMPACT_TYPE_STOP)
    {
        header->field_id = 0;
        return 0;
    }

    if (delta == 0)
    {
        if (reader->position + 1 > reader->size)
            return -1;
        header->field_id = reader->data[reader->position] | (reader->data[reader->position + 1] << 8);
        reader->position += 2;
    }
    else
    {
        header->field_id = *last_field_id + delta;
    }

    *last_field_id = header->field_id;
    return 0;
}

int32_t thrift_zigzag_decode32(uint32_t n) { return (int32_t)((n >> 1) ^ -(int32_t)(n & 1)); }

int64_t thrift_zigzag_decode64(uint64_t n) { return (int64_t)((n >> 1) ^ -(int64_t)(n & 1)); }

int thrift_skip_field(thrift_reader_t* reader, uint8_t field_type, int16_t* last_field_id)
{
    switch (field_type)
    {
    case COMPACT_TYPE_STOP:
        break;
    case COMPACT_TYPE_BOOL_TRUE:
        printf("  → BOOL value: true\n");
        break;
    case COMPACT_TYPE_BOOL_FALSE:
        printf("  → BOOL value: false\n");
        break;
    case COMPACT_TYPE_BYTE:
        uint8_t byte_value = reader->data[reader->position++];
        printf("  → BYTE value: %d\n", byte_value);
        break;
    case COMPACT_TYPE_I16:
        uint32_t varint16;
        thrift_read_varint32(reader, &varint16);
        int16_t decoded_i16 = thrift_zigzag_decode32(varint16);
        printf("  → I16 value: %d\n", decoded_i16);
        break;
    case COMPACT_TYPE_I32:
        uint32_t varint32;
        thrift_read_varint32(reader, &varint32);
        int32_t decoded_i32 = thrift_zigzag_decode32(varint32);
        printf("  → I32 value: %u\n", decoded_i32);
        break;
    case COMPACT_TYPE_I64:
        uint64_t varint64;
        thrift_read_varint64(reader, &varint64);
        int64_t decoded_i64 = thrift_zigzag_decode64(varint64);
        printf("  → I64 value: %llu\n", (long long)decoded_i64);
        break;
    case COMPACT_TYPE_DOUBLE:
        reader->position += 8;
        printf("  → DOUBLE (8 bytes)\n");
        break;
    case COMPACT_TYPE_STRING:
        uint32_t len;
        thrift_read_varint32(reader, &len);
        printf("  → STRING length: %u\n", len);

        if (len > 0 && len < 200 && reader->position + len <= reader->size)
        {
            printf("    Content: \"");
            for (uint32_t i = 0; i < len; i++)
            {
                char c = reader->data[reader->position + i];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("\"\n");
        }
        reader->position += len;
        break;
    case COMPACT_TYPE_LIST:
        if (reader->position >= reader->size)
            return -1;

        uint8_t list_header = reader->data[reader->position++];
        uint8_t element_type = list_header & 0x0F;
        uint8_t size_part = (list_header & 0xF0) >> 4;
        uint32_t size;

        if (size_part < 15)
        {
            size = size_part;
        }
        else
        {
            if (thrift_read_varint32(reader, &size) != 0)
                return -1;
        }

        printf("  → LIST: size=%u, element_type=%d\n", size, element_type);

        for (uint32_t i = 0; i < size; i++)
        {
            printf("    [%u] ", i);
            int16_t element_last_field_id = 0;
            if (thrift_skip_field(reader, element_type, &element_last_field_id) != 0)
            {
                return -1;
            }
        }
        break;
    case COMPACT_TYPE_STRUCT:
        int16_t struct_last_field_id = 0;
        thrift_field_header_t nested_header;

        printf("  → STRUCT {\n");
        while (thrift_read_field_header(reader, &nested_header, &struct_last_field_id) == 0 &&
               nested_header.type != COMPACT_TYPE_STOP)
        {
            printf("    Field ID:%d Type:%d ", nested_header.field_id, nested_header.type);

            if (thrift_skip_field(reader, nested_header.type, &struct_last_field_id) != 0)
            {
                return -1;
            }
        }
        printf("  → } STRUCT END\n");
        break;
    case COMPACT_TYPE_SET:
    case COMPACT_TYPE_MAP:
        printf("  → SET/MAP (not implemented, skipping 1 byte)\n");
        reader->position += 1;
        break;
    default:
        printf("  → Unknown type %d at position %zu, skipping 1 byte\n", field_type, reader->position);
        reader->position += 1;
        break;
    }
    return 0;
}

void thrift_print_hex(thrift_reader_t* reader, int bytes)
{
    printf("Position: %zu: ", reader->position);
    for (int i = 0; i < bytes && reader->position + i < reader->size; i++)
    {
        printf("%02x ", reader->data[reader->position + i]);
    }
    printf("\n");
}
