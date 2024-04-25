#pragma once

#include <cstddef>
#include <cstdint>

namespace MsgLite {
    struct Object {
        enum {
            Untyped, // Default value, not used in real messages
            Bool,    // Boolean type, can be either true or false
            Uint8,   // Unsigned 8-bit integer
            Uint16,  // Unsigned 16-bit integer
            Uint32,  // Unsigned 32-bit integer
            Uint64,  // Unsigned 64-bit integer
            Int8,    // Signed 8-bit integer
            Int16,   // Signed 16-bit integer
            Int32,   // Signed 32-bit integer
            Int64,   // Signed 64-bit integer
            Float,   // 32-bit floating point number
            Double,  // 64-bit floating point number
            String   // Character array of up to 15 bytes (excluding '\0')
        } type;

        union {
            bool Bool;
            uint8_t Uint8;
            uint16_t Uint16;
            uint32_t Uint32;
            uint64_t Uint64;
            int8_t Int8;
            int16_t Int16;
            int32_t Int32;
            int64_t Int64;
            float Float;
            double Double;
            char String[16];
        } as;

        // Constructors
        Object(void);
        Object(bool x);
        Object(uint8_t x);
        Object(uint16_t x);
        Object(uint32_t x);
        Object(uint64_t x);
        Object(int8_t x);
        Object(int16_t x);
        Object(int32_t x);
        Object(int64_t x);
        Object(float x);
        Object(double x);
        Object(const char* x); // String will be trimmed to a maximum of 15 bytes.

        // Returns byte size after serialization, -1 if invalid type.
        int8_t size() const;

        // Checks if lhs and rhs are valid and equal (type and value).
        // Ensures that after serialization, they are the same byte array.
        //
        // Remark: NaN != NaN but Object(NaN) == Object(NaN)
        friend bool operator==(const Object& lhs, const Object& rhs);

        // Converting functions that return true if types match.
        bool cast_to(bool& x);
        bool cast_to(uint8_t& x);
        bool cast_to(uint16_t& x);
        bool cast_to(uint32_t& x);
        bool cast_to(uint64_t& x);
        bool cast_to(int8_t& x);
        bool cast_to(int16_t& x);
        bool cast_to(int32_t& x);
        bool cast_to(int64_t& x);
        bool cast_to(float& x);
        bool cast_to(double& x);
        bool cast_to(char* x); // Assumes sizeof(x) >= 16

        // Dummy converting functions that do nothing and return false.
        bool cast_to(const bool& x);
        bool cast_to(const uint8_t& x);
        bool cast_to(const uint16_t& x);
        bool cast_to(const uint32_t& x);
        bool cast_to(const uint64_t& x);
        bool cast_to(const int8_t& x);
        bool cast_to(const int16_t& x);
        bool cast_to(const int32_t& x);
        bool cast_to(const int64_t& x);
        bool cast_to(const float& x);
        bool cast_to(const double& x);
        bool cast_to(const char* x);
    };

    bool operator==(const Object& lhs, const Object& rhs);

    struct Message {
        uint8_t len;
        Object obj[15];

        // Constructors
        Message()
        {
            len = 0;
        }
        template <typename Type, typename... Types>
        Message(Type first, Types... others)
        {
            static_assert(1 + sizeof...(others) <= 15, "The number of objects exceeds the limit.");
            len = 1 + sizeof...(others);
            Object tmp[] = { Object(first), Object(others)... };
            for (int ii = 0; ii < len; ii++) {
                obj[ii] = tmp[ii];
            }
        }

        // Returns byte size after serialization, -1 if invalid message.
        int16_t size() const;
    };

    const int MIN_MSG_LEN = (1 + (1 + 4) + (1 + 0));             // = 7
    const int MAX_MSG_LEN = (1 + (1 + 4) + (1 + 15 * (15 + 1))); // = 247

    // Buffer provides a byte array capable of storing any valid message.
    struct Buffer {
        uint8_t len;
        uint8_t data[MAX_MSG_LEN];
    };

    // Serializes message and writes bytes to a byte array.
    //
    // Returns length of data if serialization is successful, -1 if fails.
    int16_t Pack(const Message& msg, uint8_t* buf, uint8_t len);

    // Serializes message and writes bytes to a buffer.
    //
    // Returns true if successful, false if packing fails.
    bool Pack(const Message& msg, Buffer& buf);

    // Deserializes data from a byte array.
    //
    // Returns true if successful, false if unpacking fails.
    bool Unpack(const uint8_t* buf, uint8_t len, Message& msg);

    // Deserializes data from a buffer.
    //
    // Returns true if successful, false if unpacking fails.
    bool Unpack(const Buffer& buf, Message& msg);

    // Stream packer.
    class Packer {
    public:
        // 1. Call put() to serialize a message. Returns true if successful.
        bool put(const Message& msg);
        // 2. Call get() repeatedly to get bytes. Returns -1 to indicate the end.
        int get(void);

        Packer(void);

    private:
        uint8_t raw_buf[MAX_MSG_LEN];
        uint8_t pos, len;
    };

    // Stream unpacker.
    class Unpacker {
    public:
        // 1. Call put() repeatedly to drive the unpacker. It returns true if a
        // message has been deserialized (with a CRC32 checksum pass).
        //
        // Further calls to this function may corrupt the message, so a put()
        // returning true should be immediately followed by a get() to
        // retrieve the message.
        bool put(uint8_t byte);

        // 2. Retrieve a reference to the message. If this function does not
        // follow a put() returning true, the return message can be anything.
        //
        // You may want to deep copy this message as further calls to put() may
        // change it.
        const Message& get(void);

        Unpacker(void);

    private:
        uint8_t raw_buf[MAX_MSG_LEN];
        uint8_t len;
        int8_t remaining_objects, remaining_bytes;
        uint32_t crc_header, crc_body;
        Message msg;
    };

    // Checksum function used by MsgLite
    uint32_t CRC32B(uint32_t crc, const uint8_t* buf, size_t size);
}
