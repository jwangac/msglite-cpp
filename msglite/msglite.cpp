#include "msglite.h"

#include <cstddef>
#include <cstring>
#include <limits>

// pedantic checks
static_assert(sizeof(float) == 4, "float must be 32 bits");
static_assert(sizeof(double) == 8, "double must be 64 bits");
static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 float required");
static_assert(std::numeric_limits<double>::is_iec559, "IEEE 754 double required");

template <typename T>
static void to_1_bytes(T y, uint8_t* x)
{
    static_assert(sizeof(T) == 1, "Data type size must be 1 bytes");
    union {
        T t;
        uint8_t i;
    } u;
    u.t = y;
    x[0] = u.i;
}

template <typename T>
static void to_2_bytes(T y, uint8_t* x)
{
    static_assert(sizeof(T) == 2, "Data type size must be 2 bytes");
    union {
        T t;
        uint16_t i;
    } u;
    u.t = y;
    x[0] = (u.i >> 8) & 0xFF;
    x[1] = u.i & 0xFF;
}

template <typename T>
static void to_4_bytes(T y, uint8_t* x)
{
    static_assert(sizeof(T) == 4, "Data type size must be 4 bytes");
    union {
        T t;
        uint32_t i;
    } u;
    u.t = y;
    x[0] = (u.i >> 24) & 0xFF;
    x[1] = (u.i >> 16) & 0xFF;
    x[2] = (u.i >> 8) & 0xFF;
    x[3] = u.i & 0xFF;
}

template <typename T>
static void to_8_bytes(T y, uint8_t* x)
{
    static_assert(sizeof(T) == 8, "Data type size must be 8 bytes");
    union {
        T t;
        uint64_t i;
    } u;
    u.t = y;
    x[0] = (u.i >> 56) & 0xFF;
    x[1] = (u.i >> 48) & 0xFF;
    x[2] = (u.i >> 40) & 0xFF;
    x[3] = (u.i >> 32) & 0xFF;
    x[4] = (u.i >> 24) & 0xFF;
    x[5] = (u.i >> 16) & 0xFF;
    x[6] = (u.i >> 8) & 0xFF;
    x[7] = u.i & 0xFF;
}

template <typename T>
static void from_1_bytes(T& y, const uint8_t* x)
{
    static_assert(sizeof(T) == 1, "Data type size must be 1 bytes");
    union {
        T t;
        uint8_t i;
    } u;
    u.i = x[0];
    y = u.t;
}

template <typename T>
static void from_2_bytes(T& y, const uint8_t* x)
{
    static_assert(sizeof(T) == 2, "Data type size must be 2 bytes");
    union {
        T t;
        uint16_t i;
    } u;
    u.i = (uint16_t)x[0] << 8 | (uint16_t)x[1];
    y = u.t;
}

template <typename T>
static void from_4_bytes(T& y, const uint8_t* x)
{
    static_assert(sizeof(T) == 4, "Data type size must be 4 bytes");
    union {
        T t;
        uint32_t i;
    } u;
    u.i = (uint32_t)x[0] << 24 | (uint32_t)x[1] << 16 | (uint32_t)x[2] << 8 | (uint32_t)x[3];
    y = u.t;
}

template <typename T>
static void from_8_bytes(T& y, const uint8_t* x)
{
    static_assert(sizeof(T) == 8, "Data type size must be 8 bytes");
    union {
        T t;
        uint64_t i;
    } u;
    u.i = (uint64_t)x[0] << 56 | (uint64_t)x[1] << 48 | (uint64_t)x[2] << 40 | (uint64_t)x[3] << 32 | (uint64_t)x[4] << 24 | (uint64_t)x[5] << 16 | (uint64_t)x[6] << 8 | (uint64_t)x[7];
    y = u.t;
}

// A custom implementation of strnlen,
// which is a GNU extension and may not be available everywhere.
static size_t custom_strnlen(const char* str, size_t n)
{
    const char* start = str;
    while (n-- > 0 && *str)
        str++;
    return str - start;
}

// CRC32 code derived from work by Gary S. Brown.
// https://opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/libkern/crc32.c
static const uint32_t crc32_table[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// CRC32 code derived from work by Gary S. Brown.
// https://opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/libkern/crc32.c
static uint32_t crc32(uint32_t crc, const uint8_t* buf, size_t size)
{
    crc = crc ^ ~0U;
    while (size--)
        crc = crc32_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    return crc ^ ~0U;
}

static int8_t bytes_of_type(uint8_t type_byte)
{
    switch (type_byte) {
        // Bool (false)
        case 0xC2:
            return 0;
        // Bool (true)
        case 0xC3:
            return 0;
        // Uint8
        case 0xCC:
            return 1;
        // Uint16
        case 0xCD:
            return 2;
        // Uint32
        case 0xCE:
            return 4;
        // Uint64
        case 0xCF:
            return 8;
        // Int8
        case 0xD0:
            return 1;
        // Int16
        case 0xD1:
            return 2;
        // Int32
        case 0xD2:
            return 4;
        // Int64
        case 0xD3:
            return 8;
        // Float
        case 0xCA:
            return 4;
        // Double
        case 0xCB:
            return 8;
        // String and others
        default: {
            // String
            if (0xA0 <= type_byte && type_byte <= 0xAF)
                return type_byte - 0xA0;

            // Unknown type
            return -1;
        }
    }
}

using namespace MsgLite;

Object::Object()
{
    this->type = Untyped;
}

Object::Object(bool x)
{
    this->type = Bool;
    this->as.Bool = x;
}

Object::Object(uint8_t x)
{
    this->type = Uint8;
    this->as.Uint8 = x;
}

Object::Object(uint16_t x)
{
    this->type = Uint16;
    this->as.Uint16 = x;
}

Object::Object(uint32_t x)
{
    this->type = Uint32;
    this->as.Uint32 = x;
}

Object::Object(uint64_t x)
{
    this->type = Uint64;
    this->as.Uint64 = x;
}

Object::Object(int8_t x)
{
    this->type = Int8;
    this->as.Int8 = x;
}

Object::Object(int16_t x)
{
    this->type = Int16;
    this->as.Int16 = x;
}

Object::Object(int32_t x)
{
    this->type = Int32;
    this->as.Int32 = x;
}

Object::Object(int64_t x)
{
    this->type = Int64;
    this->as.Int64 = x;
}

Object::Object(float x)
{
    this->type = Float;
    this->as.Float = x;
}

Object::Object(double x)
{
    this->type = Double;
    this->as.Double = x;
}

Object::Object(const char* x)
{
    this->type = String;
    strncpy(this->as.String, x, 15);
    this->as.String[15] = '\0';
}

// Serializes message and writes bytes to a byte array.
//
// The byte array must be at least MAX_MSG_LEN (247 bytes)!
//
// Returns length of data if serialization is successful, 0 if fails.
uint8_t MsgLite::Pack(const Message& msg, uint8_t* buf)
{
    if (msg.len > 15)
        return 0; // Error: message too long

    uint8_t pos = 0;

    // Header
    buf[pos++] = 0x92;

    // Checksum (CRC32)
    buf[pos++] = 0xCE;
    buf[pos++] = 0x00; // To be filled post-serialization
    buf[pos++] = 0x00; // To be filled post-serialization
    buf[pos++] = 0x00; // To be filled post-serialization
    buf[pos++] = 0x00; // To be filled post-serialization

    // Message Length
    buf[pos++] = 0x90 + msg.len;

    // Message Body
    for (int ii = 0; ii < msg.len; ii++) {
        switch (msg.obj[ii].type) {
            case Object::Bool: {
                // An object marked as a bool type but having a value other than
                // 0/1 (mostly due to a mistake) can cause undefined behavior.
                // Use the 'volatile' keyword to enforce a check here.
                volatile uint8_t bool_in_byte = msg.obj[ii].as.String[0];
                if (bool_in_byte != 0 && bool_in_byte != 1)
                    return 0;

                buf[pos++] = 0xC2 + msg.obj[ii].as.Bool;
                break;
            }

            case Object::Uint8: {
                buf[pos++] = 0xCC;
                to_1_bytes(msg.obj[ii].as.Uint8, &buf[pos]);
                pos += 1;
                break;
            }

            case Object::Uint16: {
                buf[pos++] = 0xCD;
                to_2_bytes(msg.obj[ii].as.Uint16, &buf[pos]);
                pos += 2;
                break;
            }

            case Object::Uint32: {
                buf[pos++] = 0xCE;
                to_4_bytes(msg.obj[ii].as.Uint32, &buf[pos]);
                pos += 4;
                break;
            }

            case Object::Uint64: {
                buf[pos++] = 0xCF;
                to_8_bytes(msg.obj[ii].as.Uint64, &buf[pos]);
                pos += 8;
                break;
            }

            case Object::Int8: {
                buf[pos++] = 0xD0;
                to_1_bytes(msg.obj[ii].as.Int8, &buf[pos]);
                pos += 1;
                break;
            }

            case Object::Int16: {
                buf[pos++] = 0xD1;
                to_2_bytes(msg.obj[ii].as.Int16, &buf[pos]);
                pos += 2;
                break;
            }

            case Object::Int32: {
                buf[pos++] = 0xD2;
                to_4_bytes(msg.obj[ii].as.Int32, &buf[pos]);
                pos += 4;
                break;
            }

            case Object::Int64: {
                buf[pos++] = 0xD3;
                to_8_bytes(msg.obj[ii].as.Int64, &buf[pos]);
                pos += 8;
                break;
            }

            case Object::Float: {
                buf[pos++] = 0xCA;
                to_4_bytes(msg.obj[ii].as.Float, &buf[pos]);
                pos += 4;
                break;
            }

            case Object::Double: {
                buf[pos++] = 0xCB;
                to_8_bytes(msg.obj[ii].as.Double, &buf[pos]);
                pos += 8;
                break;
            }

            case Object::String: {
                int len = custom_strnlen(msg.obj[ii].as.String, 16);
                if (len > 15)
                    return 0; // Error: String too long
                buf[pos++] = 0xA0 + len;
                memcpy(buf + pos, msg.obj[ii].as.String, len); // Memory areas not overlap: yes
                pos += len;
                break;
            }

            default: {
                return 0; // Error: Unknown type
            }
        }
    }

    // Checksum (CRC32)
    {
        uint32_t crc = crc32(0, buf + 6, pos - 6);
        buf[2] = (crc >> 24) & 0xFF;
        buf[3] = (crc >> 16) & 0xFF;
        buf[4] = (crc >> 8) & 0xFF;
        buf[5] = crc & 0xFF;
    }

    return pos;
}

// Serializes message and writes bytes to a buffer.
//
// Returns true if successful, false if packing fails.
bool MsgLite::Pack(const Message& msg, Buffer& buf)
{
    buf.len = Pack(msg, buf.data);
    return buf.len > 0;
}

// Low-level function that deserializes message body from a byte array.
//
// Returns three possible values below.
enum unpack_ll_status {
    unpack_ll_success,
    unpack_ll_need_more_bytes,
    unpack_ll_corrupted,
    unpack_ll_too_many_bytes
};
static unpack_ll_status unpack_ll_body(const uint8_t* buf, uint8_t len, Message& msg)
{
    if (len < MIN_MSG_LEN)
        return unpack_ll_need_more_bytes;

    if (len > MAX_MSG_LEN)
        return unpack_ll_too_many_bytes;

    // Skip verifying the header and checksum (6 bytes).
    uint8_t pos = 6;

    // Message length
    msg.len = buf[pos++] - 0x90;
    if (msg.len > 15)
        return unpack_ll_corrupted;

    // Message body
    for (int ii = 0; ii < msg.len; ii++) {
        if (pos + 1 > len)
            return unpack_ll_need_more_bytes;
        uint8_t type_byte = buf[pos++];

        switch (type_byte) {
            // Bool (false)
            case 0xC2: {
                msg.obj[ii].type = Object::Bool;
                msg.obj[ii].as.Bool = false;
                break;
            }
            // Bool (true)
            case 0xC3: {
                msg.obj[ii].type = Object::Bool;
                msg.obj[ii].as.Bool = true;
                break;
            }
            // Uint8
            case 0xCC: {
                if (pos + 1 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Uint8;
                from_1_bytes(msg.obj[ii].as.Uint8, buf + pos);
                pos += 1;
                break;
            }
            // Uint16
            case 0xCD: {
                if (pos + 2 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Uint16;
                from_2_bytes(msg.obj[ii].as.Uint16, buf + pos);
                pos += 2;
                break;
            }
            // Uint32
            case 0xCE: {
                if (pos + 4 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Uint32;
                from_4_bytes(msg.obj[ii].as.Uint32, buf + pos);
                pos += 4;
                break;
            }
            // Uint64
            case 0xCF: {
                if (pos + 8 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Uint64;
                from_8_bytes(msg.obj[ii].as.Uint64, buf + pos);
                pos += 8;
                break;
            }
            // Int8
            case 0xD0: {
                if (pos + 1 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Int8;
                from_1_bytes(msg.obj[ii].as.Int8, buf + pos);
                pos += 1;
                break;
            }
            // Int16
            case 0xD1: {
                if (pos + 2 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Int16;
                from_2_bytes(msg.obj[ii].as.Int16, buf + pos);
                pos += 2;
                break;
            }
            // Int32
            case 0xD2: {
                if (pos + 4 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Int32;
                from_4_bytes(msg.obj[ii].as.Int32, buf + pos);
                pos += 4;
                break;
            }
            // Int64
            case 0xD3: {
                if (pos + 8 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Int64;
                from_8_bytes(msg.obj[ii].as.Int64, buf + pos);
                pos += 8;
                break;
            }
            // Float
            case 0xCA: {
                if (pos + 4 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Float;
                from_4_bytes(msg.obj[ii].as.Float, buf + pos);
                pos += 4;
                break;
            }
            // Double
            case 0xCB: {
                if (pos + 8 > len)
                    return unpack_ll_need_more_bytes;
                msg.obj[ii].type = Object::Double;
                from_8_bytes(msg.obj[ii].as.Double, buf + pos);
                pos += 8;
                break;
            }
            // String and others
            default: {
                if (0xA0 <= type_byte && type_byte <= 0xAF) {
                    int str_len = type_byte - 0xA0;
                    if (pos + str_len > len)
                        return unpack_ll_need_more_bytes;
                    msg.obj[ii].type = Object::String;
                    memcpy(msg.obj[ii].as.String, buf + pos, str_len); // Memory areas not overlap: yes
                    msg.obj[ii].as.String[str_len] = '\0';
                    pos += str_len;
                    break;
                }

                // Unknown type
                return unpack_ll_corrupted;
            }
        }
    }
    return pos == len ? unpack_ll_success : unpack_ll_too_many_bytes;
}

// Deserializes data from a byte array.
//
// Returns true if successful, false if unpacking fails.
bool MsgLite::Unpack(const uint8_t* buf, uint8_t len, Message& msg)
{
    if (len < MIN_MSG_LEN || len > MAX_MSG_LEN)
        return false;

    // Header
    if (buf[0] != 0x92)
        return false;

    // Checksum
    if (buf[1] != 0xCE)
        return false;
    uint32_t crc_header, crc_body;
    from_4_bytes(crc_header, buf + 2);
    crc_body = crc32(0, buf + 6, len - 6);
    if (crc_body != crc_header)
        return false;

    // Body
    return unpack_ll_body(buf, len, msg) == unpack_ll_success;
}

// Deserializes data from a buffer.
//
// Returns true if successful, false if unpacking fails.
bool MsgLite::Unpack(const Buffer& buf, Message& msg)
{
    return Unpack(buf.data, buf.len, msg);
}

// Stream packer constructor
Packer::Packer(void)
{
    // Ensure pos > len, which makes get() returns -1.
    pos = MAX_MSG_LEN + 1;
    len = 0;
}

// 1. Call put() to serialize a message. Returns true if successful.
bool Packer::put(const Message& msg)
{
    len = Pack(msg, buf);
    if (len == 0) {
        // Ensure pos > len, which makes get() returns -1.
        pos = MAX_MSG_LEN + 1;
        return false;
    }
    pos = 0;
    return true;
}

// 2. Call get() repeatedly to get bytes. Returns -1 to indicate the end.
int Packer::get()
{
    if (pos < len)
        return buf[pos++];
    else
        return -1;
}

// Stream unpacker constructor
Unpacker::Unpacker(void)
{
    len = 0;
}

// 1. Call put() repeatedly to drive the unpacker. It returns true if a
// message has been deserialized (with a CRC32 checksum pass).
//
// Further calls to this function may corrupt the message, so a put()
// returning true should be immediately followed by a get() to
// retrieve the message.
bool Unpacker::put(uint8_t byte)
{
    if (len >= MAX_MSG_LEN)
        len = 0; // Failed, reset the unpacker

    switch (len) {
        // Header
        case 0: {
            if (byte != 0x92) {
                len = 0; // Failed, reset the unpacker
                return false;
            }
            buf[len++] = byte;
            break;
        }
        // Checksum
        case 1: {
            if (byte != 0xCE) {
                len = 0; // Failed, reset the unpacker
                return false;
            }
            crc_header = 0;
            crc_body = 0;
            buf[len++] = byte;
            break;
        }
        case 2:
        case 3:
        case 4:
        case 5: {
            crc_header = (crc_header << 8) + byte;
            buf[len++] = byte;
            break;
        }
        // Body
        default: {
            if (len == 6) {
                // Message length
                remaining_objects = byte - 0x90;
                if (remaining_objects > 15) {
                    len = 0; // Failed, reset the unpacker
                    return false;
                }
                remaining_bytes = 0;
            } else {
                if (remaining_bytes > 0) {
                    remaining_bytes--;
                } else {
                    if (remaining_objects > 0) {
                        remaining_objects--;
                        remaining_bytes = bytes_of_type(byte);
                        if (remaining_bytes < 0) {
                            len = 0; // Failed, reset the unpacker
                            return false;
                        }
                    } else {
                        len = 0; // Failed, reset the unpacker
                        return false;
                    }
                }
            }
            crc_body = crc32(crc_body, &byte, 1);
            buf[len++] = byte;
        }
    }

    if (len < MIN_MSG_LEN)
        return false; // Still too short

    if (remaining_objects > 0 || remaining_bytes > 0)
        return false; // Message not fully received

    if (crc_header != crc_body) {
        return false; // Checksum mismatch
    }

    unpack_ll_status status = unpack_ll_body(buf, len, msg);
    if (status == unpack_ll_success) {
        len = 0;
        return true;
    } else if (status == unpack_ll_need_more_bytes) {
        return false;
    } else {
        len = 0;
        return false;
    }
}

// 2. Retrieve a reference to the message. If this function does not
// follow a put() returning true, the return message can be anything.
//
// You may want to deep copy this message as further calls to put() may
// change it.
const Message& Unpacker::get(void)
{
    return msg;
}
