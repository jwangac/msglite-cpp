#include <cassert>
#include <inttypes.h>
#include <limits>
#include <stdint.h>
#include <stdio.h>

#include "msglite.h"

void print(const MsgLite::Message& msg);

static constexpr double Inf = std::numeric_limits<double>::infinity();
static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

int main(void)
{
    if (true) {
        auto msg = MsgLite::Message("helloworld");
        print(msg);

        MsgLite::Buffer buf;
        auto pack_successful = MsgLite::Pack(msg, buf);
        if (pack_successful) {
            printf("Pack successful:\n");
            printf("Bytes length: %d\n", buf.len);
            for (int ii = 0; ii < buf.len; ii++)
                printf("%02x ", buf.data[ii]);
            printf("\n\n");
        } else {
            printf("Pack failed\n");
            return 1;
        }

        MsgLite::Message msg2;
        auto unpack_successful = MsgLite::Unpack(buf, msg2);
        if (unpack_successful) {
            printf("Unpack successful:\n");
            print(msg2);
        } else {
            printf("Unpack failed\n");
            return 1;
        }
    }

    if (true) {
        auto msg = MsgLite::Message(false, true, (uint8_t)1, (uint16_t)2, (uint32_t)3, (uint64_t)4, (int8_t)-1, (int16_t)-2, (int32_t)-3, (int64_t)-4, 1.0f, 2.0, Inf, NaN, "end");
        print(msg);

        MsgLite::Buffer buf;
        auto pack_successful = MsgLite::Pack(msg, buf);
        if (pack_successful) {
            printf("Pack successful:\n");
            printf("Bytes length: %d\n", buf.len);
            for (int ii = 0; ii < buf.len; ii++)
                printf("%02x ", buf.data[ii]);
            printf("\n\n");
        } else {
            printf("Pack failed\n");
            return 1;
        }

        MsgLite::Message msg2;
        auto unpack_successful = MsgLite::Unpack(buf, msg2);
        if (unpack_successful) {
            printf("Unpack successful:\n");
            print(msg2);
        } else {
            printf("Unpack failed\n");
            return 1;
        }
    }

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
            printf("%x ", c);
            unpacker.put(c);
        }
        printf("\n");
        print(msg);

        print(unpacker.get());
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

    if (true) {
        MsgLite::Message largest("helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello", "helloworldhello");
        MsgLite::Buffer buf;
        assert(MsgLite::Pack(largest, buf));
        assert(MsgLite::Unpack(buf, largest));
        assert(buf.len == MsgLite::MAX_MSG_LEN);
    }

    if (true) {
        MsgLite::Buffer buf;
        MsgLite::Message broken(false);
        broken.obj[0].as.Uint8 = 2;
        assert(!MsgLite::Pack(broken, buf));
    }

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

    if (true) {
        FILE* fd = fopen("./test/data_robustness.bin", "rb");

        int c, cnt = 0;
        while ((c = fgetc(fd)) != EOF) {
            if (unpacker.put(c))
                cnt++;
        }
        assert(4500 <= cnt && cnt <= 5500);
        printf("Number of messages unpacked from lossy data stream: %d\n", cnt);

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
