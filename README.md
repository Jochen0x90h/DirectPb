# DirectPb

Direct ProtoBuffers

Plugin for protoc that generates C++ code with only little external requirements. Only a Reader and Writer class are required that can
read and write the basic types (int/uint/float with 32/64 bits), varint and string/data.

## Features
* Fixed size types using templates, no allocation
* Compatible with coco::BufferReader and coco::BufferWriter (see [coco-device](https://github.com/Jochen0x90h/coco-device))
