#include <cassert>
#include <cstring>
#include <inttypes.h>
#include <limits>
#include <stdint.h>
#include <stdio.h>

#include "msglite.h"

void pedantic_checks();
void print(const MsgLite::Message& msg);
void print(const MsgLite::Buffer& buf);

static constexpr double Inf = std::numeric_limits<double>::infinity();
static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

int main(void)
{
    pedantic_checks();

    // test Pack() and Unpack() with a simple message
    if (true) {
        auto msg = MsgLite::Message("helloworld");
        print(msg);

        MsgLite::Buffer buf;
        auto pack_successful = MsgLite::Pack(msg, buf);
        if (pack_successful) {
            printf("Pack successful\n");
            print(buf);
        } else {
            printf("Pack failed\n");
            return 1;
        }

        auto unpack_successful = MsgLite::Unpack(buf, msg);
        if (unpack_successful) {
            printf("Unpack successful\n");
        } else {
            printf("Unpack failed\n\n");
            return 1;
        }
    }

    // test with a more complex message
    if (true) {
        auto msg = MsgLite::Message(false, true, (uint8_t)1, (uint16_t)2, (uint32_t)3, (uint64_t)4, (int8_t)-1, (int16_t)-2, (int32_t)-3, (int64_t)-4, 1.0f, 2.0, Inf, NaN, "end");
        print(msg);

        MsgLite::Buffer buf;
        auto pack_successful = MsgLite::Pack(msg, buf);
        if (pack_successful) {
            printf("Pack successful\n");
            print(buf);
        } else {
            printf("Pack failed\n");
            return 1;
        }

        auto unpack_successful = MsgLite::Unpack(buf, msg);
        if (unpack_successful) {
            printf("Unpack successful\n\n");
        } else {
            printf("Unpack failed\n");
            return 1;
        }
    }

    // test with empty and largest messages
    if (true) {
        MsgLite::Message empty;
        assert(empty.size() == MsgLite::MIN_MSG_LEN);

        MsgLite::Message largest("helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello");
        MsgLite::Buffer buf;
        assert(MsgLite::Pack(largest, buf));
        assert(MsgLite::Unpack(buf, largest));
        assert(buf.len == MsgLite::MAX_MSG_LEN);
    }

    // test with broken messages
    if (true) {
        MsgLite::Buffer buf;
        MsgLite::Message broken(false);
        broken.obj[0].as.Uint8 = 2;
        assert(!MsgLite::Pack(broken, buf));

        MsgLite::Message invalid_msg;
        invalid_msg.len = 1;
        assert(!MsgLite::Pack(invalid_msg, buf)); // Should fail to pack
    }

    // test stream packer and unpacker
    MsgLite::Packer packer;
    MsgLite::Unpacker unpacker;

    if (true) {
        MsgLite::Message msg;
        msg.len = 1;
        msg.obj[0].type = MsgLite::Object::Double;
        msg.obj[0].as.Double = 1.23456;

        packer.put(msg);
        int c;
        while ((c = packer.get()) != EOF) {
            unpacker.put(c);
        }
        MsgLite::Message msg2 = unpacker.get();
        assert(msg.len == msg2.len);
        assert(msg.obj[0].type == msg2.obj[0].type);
        assert(msg.obj[0].as.Double == msg2.obj[0].as.Double);
    }

    if (true) {
        MsgLite::Message msg("helloworld");
        MsgLite::Buffer buf;
        MsgLite::Pack(msg, buf);
        assert(packer.put(msg));
        int c, ii = 0;
        while ((c = packer.get()) != -1) {
            assert(c == buf.data[ii++]);
        }
        assert(packer.get() == -1);

        msg.obj[0].type = MsgLite::Object::Bool;
        assert(MsgLite::Pack(msg, buf.data, sizeof(buf.data)) < 0);
        assert(!MsgLite::Pack(msg, buf));
        assert(!packer.put(msg));
        assert(packer.get() == -1);
    }

    // test stream unpacker with empty and largest messages
    if (true) {
        MsgLite::Buffer buf;

        MsgLite::Message empty;
        MsgLite::Pack(empty, buf);
        for (int ii = 0; ii < buf.len; ++ii) {
            if (unpacker.put(buf.data[ii])) {
                assert(ii == buf.len - 1);
                break;
            }
            assert(ii < buf.len - 1);
        }

        MsgLite::Message largest("helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello");
        MsgLite::Pack(largest, buf);
        for (int ii = 0; ii < buf.len; ++ii) {
            if (unpacker.put(buf.data[ii])) {
                assert(ii == buf.len - 1);
                break;
            }
            assert(ii < buf.len - 1);
        }
    }

    // test stream unpacker with valid data
    if (true) {
        FILE* fd = fopen("./test/data_static.bin", "rb");

        int c, cnt = 0;
        while ((c = fgetc(fd)) != EOF) {
            if (unpacker.put(c))
                cnt++;
        }
        assert(cnt == 17);
        fclose(fd);
    }

    // test stream unpacker with lossy data
    if (true) {
        FILE* fd = fopen("./test/data_robustness.bin", "rb");

        int c, cnt = 0;
        while ((c = fgetc(fd)) != EOF) {
            if (unpacker.put(c))
                cnt++;
        }
        assert(4500 <= cnt && cnt <= 5500);
        printf("Number of messages unpacked from lossy data stream: %d (should be near 5000)\n", cnt);

        fclose(fd);
    }
}

void print(const MsgLite::Message& msg)
{
    printf("Message length: %d\n", msg.len);

    for (int i = 0; i < msg.len; i++) {
        switch (msg.obj[i].type) {
            case MsgLite::Object::Bool: {
                if (msg.obj[i].as.Bool)
                    printf("|   %2d: True (Bool)\n", i + 1);
                else
                    printf("|   %2d: False (Bool)\n", i + 1);
                break;
            }
            case MsgLite::Object::Uint8: {
                printf("|   %2d: %" PRIu8 " (Uint8)\n", i + 1, msg.obj[i].as.Uint8);
                break;
            }
            case MsgLite::Object::Uint16: {
                printf("|   %2d: %" PRIu16 " (Uint16)\n", i + 1, msg.obj[i].as.Uint16);
                break;
            }
            case MsgLite::Object::Uint32: {
                printf("|   %2d: %" PRIu32 " (Uint32)\n", i + 1, msg.obj[i].as.Uint32);
                break;
            }
            case MsgLite::Object::Uint64: {
                printf("|   %2d: %" PRIu64 " (Uint64)\n", i + 1, msg.obj[i].as.Uint64);
                break;
            }
            case MsgLite::Object::Int8: {
                printf("|   %2d: %" PRId8 " (Int8)\n", i + 1, msg.obj[i].as.Int8);
                break;
            }
            case MsgLite::Object::Int16: {
                printf("|   %2d: %" PRId16 " (Int16)\n", i + 1, msg.obj[i].as.Int16);
                break;
            }
            case MsgLite::Object::Int32: {
                printf("|   %2d: %" PRId32 " (Int32)\n", i + 1, msg.obj[i].as.Int32);
                break;
            }
            case MsgLite::Object::Int64: {
                printf("|   %2d: %" PRId64 " (Int64)\n", i + 1, msg.obj[i].as.Int64);
                break;
            }
            case MsgLite::Object::Float: {
                printf("|   %2d: %.6f (32 bit)\n", i + 1, msg.obj[i].as.Float);
                break;
            }
            case MsgLite::Object::Double: {
                printf("|   %2d: %.6lf (64 bit)\n", i + 1, msg.obj[i].as.Double);
                break;
            }
            case MsgLite::Object::String: {
                printf("|   %2d: \"%s\" (String)\n", i + 1, msg.obj[i].as.String);
                break;
            }
            default: {
                break;
            }
        }
    }

    printf("\n");
}

void print(const MsgLite::Buffer& buf)
{
    printf("Bytes length: %d\n", buf.len);
    for (int ii = 0; ii < buf.len; ++ii) {
        // Print the offset at the start of each line
        if (ii % 16 == 0) {
            printf("|   %04x:  ", ii);
        }

        // Print each byte in hexadecimal
        printf("%02x ", buf.data[ii]);

        // At the end of each line or the end of the data, print ASCII characters
        if (ii % 16 == 15 || ii == buf.len - 1) {
            // Fill the rest of the line with spaces if this is the last line
            if (ii == buf.len - 1 && ii % 16 != 15) {
                for (int jj = ii % 16; jj < 15; ++jj) {
                    printf("   "); // 3 spaces for each missing byte
                }
            }

            printf(" |");
            int start = ii - (ii % 16);
            for (int jj = start; jj <= ii; ++jj) {
                if (buf.data[jj] >= 32 && buf.data[jj] <= 126) {
                    // Printable ASCII character
                    printf("%c", buf.data[jj]);
                } else {
                    // Non-printable character
                    printf(".");
                }
            }
            // Fill the rest of the ASCII section with spaces if this is the last line
            if (ii == buf.len - 1 && ii % 16 != 15) {
                for (int jj = ii % 16; jj < 15; ++jj) {
                    printf(" ");
                }
            }
            printf("|\n");
        }
    }

    printf("\n");
}

template <typename... Bytes>
void assert_buffer_equal(MsgLite::Buffer buf, int skip, Bytes... x)
{
    int len = sizeof...(x);
    assert(buf.len == skip + len);
    uint8_t y[] = { uint8_t(x)... };
    for (int ii = 0; ii < len; ii++) {
        assert(buf.data[skip + ii] == y[ii]);
    }
}

void test_object_constructors()
{
    assert(MsgLite::Object().type == MsgLite::Object::Untyped);
    assert(MsgLite::Object(false).type == MsgLite::Object::Bool);
    assert(MsgLite::Object(true).type == MsgLite::Object::Bool);
    assert(MsgLite::Object((uint8_t)0).type == MsgLite::Object::Uint8);
    assert(MsgLite::Object((uint16_t)0).type == MsgLite::Object::Uint16);
    assert(MsgLite::Object((uint32_t)0).type == MsgLite::Object::Uint32);
    assert(MsgLite::Object((uint64_t)0).type == MsgLite::Object::Uint64);
    assert(MsgLite::Object((int8_t)0).type == MsgLite::Object::Int8);
    assert(MsgLite::Object((int16_t)0).type == MsgLite::Object::Int16);
    assert(MsgLite::Object((int32_t)0).type == MsgLite::Object::Int32);
    assert(MsgLite::Object((int64_t)0).type == MsgLite::Object::Int64);
    assert(MsgLite::Object((float)0).type == MsgLite::Object::Float);
    assert(MsgLite::Object((double)0).type == MsgLite::Object::Double);
    assert(MsgLite::Object("").type == MsgLite::Object::String);

    // test overlong string, should be truncated
    MsgLite::Object x("0123456789ABCDEF");
    assert(x.as.String[14] == 'E');
    assert(x.as.String[15] != 'F' && x.as.String[15] == '\0');
    assert(strlen(x.as.String) == 15);
}

void test_object_size()
{
    assert(MsgLite::Object().size() == -1);
    assert(MsgLite::Object(false).size() == 1);
    assert(MsgLite::Object(true).size() == 1);
    assert(MsgLite::Object((uint8_t)0).size() == 2);
    assert(MsgLite::Object((uint16_t)0).size() == 3);
    assert(MsgLite::Object((uint32_t)0).size() == 5);
    assert(MsgLite::Object((uint64_t)0).size() == 9);
    assert(MsgLite::Object((int8_t)0).size() == 2);
    assert(MsgLite::Object((int16_t)0).size() == 3);
    assert(MsgLite::Object((int32_t)0).size() == 5);
    assert(MsgLite::Object((int64_t)0).size() == 9);
    assert(MsgLite::Object((float)0).size() == 5);
    assert(MsgLite::Object((double)0).size() == 9);
    assert(MsgLite::Object("").size() == 1);
    assert(MsgLite::Object("helloworld").size() == 11);

    // test overlong string
    MsgLite::Object broken("0123456789ABCDE");
    broken.as.String[15] = 'F';
    assert(broken.size() == -1);
}

void test_object_serialization()
{
    MsgLite::Buffer buf;

    assert(!MsgLite::Pack(MsgLite::Object(), buf));

    assert(MsgLite::Pack(MsgLite::Object(false), buf));
    assert_buffer_equal(buf, 7, 0xC2);

    assert(MsgLite::Pack(MsgLite::Object(true), buf));
    assert_buffer_equal(buf, 7, 0xC3);

    assert(MsgLite::Pack(MsgLite::Object((uint8_t)0x01), buf));
    assert_buffer_equal(buf, 7, 0xCC, 0x01);

    assert(MsgLite::Pack(MsgLite::Object((uint16_t)0x0123), buf));
    assert_buffer_equal(buf, 7, 0xCD, 0x01, 0x23);

    assert(MsgLite::Pack(MsgLite::Object((uint32_t)0x01234567), buf));
    assert_buffer_equal(buf, 7, 0xCE, 0x01, 0x23, 0x45, 0x67);

    assert(MsgLite::Pack(MsgLite::Object((uint64_t)0x0123456789ABCDEF), buf));
    assert_buffer_equal(buf, 7, 0xCF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF);

    assert(MsgLite::Pack(MsgLite::Object((int8_t)0x01), buf));
    assert_buffer_equal(buf, 7, 0xD0, 0x01);

    assert(MsgLite::Pack(MsgLite::Object((int16_t)0x0123), buf));
    assert_buffer_equal(buf, 7, 0xD1, 0x01, 0x23);

    assert(MsgLite::Pack(MsgLite::Object((int32_t)0x01234567), buf));
    assert_buffer_equal(buf, 7, 0xD2, 0x01, 0x23, 0x45, 0x67);

    assert(MsgLite::Pack(MsgLite::Object((int64_t)0x0123456789ABCDEF), buf));
    assert_buffer_equal(buf, 7, 0xD3, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF);

    assert(MsgLite::Pack(MsgLite::Object(85.125f), buf));
    assert_buffer_equal(buf, 7, 0xCA, 0x42, 0xAA, 0x40, 0x00);

    assert(MsgLite::Pack(MsgLite::Object(85.125), buf));
    assert_buffer_equal(buf, 7, 0xCB, 0x40, 0x55, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00);

    assert(MsgLite::Pack(MsgLite::Object("helloworld"), buf));
    assert_buffer_equal(buf, 7, 0xAA, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x77, 0x6F, 0x72, 0x6C, 0x64);
}

void test_checksum()
{
    const uint8_t x[] = "123456789";
    assert(MsgLite::CRC32B(0, x, 9) == 0xCBF43926);
}

void test_parse()
{
    char s[16] = "world";
    uint8_t x = 0xFF;
    double y = Inf;

    MsgLite::Message msg("hello", "from", "apple");
    assert(!msg.parse());
    assert(!msg.parse("world"));
    assert(!msg.parse("hello"));
    assert(!msg.parse("hello", "from"));
    assert(!msg.parse("hello", "from", "who"));
    assert(!msg.parse("hello", "from", "world"));
    assert(!msg.parse("hello", "from", x));
    assert(!msg.parse("hello", "from", y));
    assert(!msg.parse(x, y));
    assert(msg.parse("hello", "from", "apple"));
    assert(msg.parse("hello", "from", s));
    assert(strcmp(s, "apple") == 0);

    MsgLite::Message msg2((uint8_t)1, 2.0);
    assert(!msg2.parse("hello"));
    assert(msg2.parse(x, y));
    assert(x == 1 && y == 2.0);

    MsgLite::Message msg3("hello", (uint8_t)3, 4.0);
    assert(!msg3.parse(x, y));
    assert(!msg3.parse("world", x, y));
    assert(msg3.parse("hello", x, y));
    assert(x == 3 && y == 4.0);

    const uint8_t magic = 0xAB;
    assert(!MsgLite::Message((uint8_t)0x00).parse(magic));
    assert(MsgLite::Message((uint8_t)0xAB).parse(magic));

    // empty message
    assert(MsgLite::Message().parse());
}

void pedantic_checks()
{
    test_object_constructors();
    test_object_size();
    test_object_serialization();
    test_checksum();
    test_parse();
}
