# Introduction

MsgLite is a format designed for reliable data transmission over loss-prone streams such as UART. It has the following features:

1. MsgLite is protected with a CRC32 checksum and implements a resilient stream unpacker, capable of filtering out corrupted bytes.

2. MsgLite is designed for microcontrollers, requiring no dynamic memory allocation or C++ STL.

3. MsgLite implements a subset of MessagePack, ensuring compatibility and simplifying communication with high-end targets like Raspberry Pi that can run Python, Golang, etc.

4. MsgLite has no external dependencies. Simply pull one header and one source file, and you're good to go.

# Demo
```cpp
#include <cstdio>

#include "msglite.h"

int main()
{
    // A demo message consists of three objects: [String, Bool, Float]
    MsgLite::Message message("helloworld", true, 3.1415926f);

    // A byte array that can hold any MsgLite message
    MsgLite::Buffer buffer;

    // Pack the message into the buffer using Pack()
    bool pack_successful = MsgLite::Pack(message, buffer);
    if (!pack_successful) {
        printf("Failed to pack the message.\n");
        return 1;
    }
    printf("Message packed successfully.\n");

    MsgLite::Unpacker unpacker;

    // Feed bytes to the stream unpacker
    for (int ii = 0; ii < buffer.len; ++ii) {
        bool message_available = unpacker.put(buffer.data[ii]);
        if (message_available) {
            printf("Message unpacked successfully.\n");
            break;
        }
    }

    return 0;
}
```

# Format
MsgLite is MessagePack and can transfer up to 15 objects per message in these formats:
```
+--------+------------------+-------------------+---------------------+
| Header | Checksum (CRC32) | Number of objects |       Objects       |
+--------+------------------+-------------------+---------------------+
|  0x92  |  0xCE + 4 bytes  |    0x90 to 0x9F   | Same as MessagePack |
+--------+------------------+-------------------+---------------------+
```
Objects can be:
- Bool
- 8/16/32/64-bit unsigned integer
- 8/16/32/64-bit signed integer
- 32/64-bit floating point number
- Strings (up to 15 characters, cannot hold '\0')

The MsgLite format ensures that each valid message, after serialization, is no longer than 247 bytes. Therefore, the unpacker can reject data that is too long, ensuring self-recovery from corrupted data.
