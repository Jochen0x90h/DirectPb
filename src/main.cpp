#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>


// https://protobuf.dev/programming-guides/encoding/


using namespace google::protobuf;
using namespace google::protobuf::compiler;
using namespace google::protobuf::io;


template <typename T>
int uVar(T value) {
    int count = 1;
    auto v = std::make_unsigned_t<T>(value);
    while (v >= 0x80) {
        v >>= 7;
        ++count;
    }
    return count;;
}

template <typename T>
int iVar(T value) {
    return uVar((value << 1) ^ (value >> (sizeof(T) * 8 - 1)));
}



enum class WireType {
    NONE = 0,
    I32 = 5, // fixed32, sfixed32, float
    I64 = 1, // fixed64, sfixed64, double
    VARINT = 0, // int32, int64, uint32, uint64, sint32, sint64, bool, enum
    LEN = 2, // string, bytes, embedded messages, packed repeated fields
};

// for each type, the corresponding wire type
const WireType wireTypes[] = {
    WireType::NONE,
    WireType::I64, // TYPE_DOUBLE
    WireType::I32, // TYPE_FLOAT
    WireType::VARINT, // TYPE_INT64
    WireType::VARINT, // TYPE_UINT64
    WireType::VARINT, // TYPE_INT32
    WireType::I64, // TYPE_FIXED64
    WireType::I32, // TYPE_FIXED32
    WireType::VARINT, // TYPE_BOOL
    WireType::LEN, // TYPE_STRING
    WireType::NONE, // (deprecated TYPE_GROUP)
    WireType::LEN, // TYPE_MESSAGE
    WireType::LEN, // TYPE_BYTES
    WireType::VARINT, // TYPE_UINT32
    WireType::VARINT, // TYPE_ENUM
    WireType::I32, // TYPE_SFIXED32
    WireType::I64, // TYPE_SFIXED64
    WireType::VARINT, // TYPE_SINT32
    WireType::VARINT, // TYPE_SINT64
};

class CocoGenerator : public CodeGenerator {
public:
    ~CocoGenerator() override {}

    uint64_t GetSupportedFeatures() const override {
        return FEATURE_PROTO3_OPTIONAL;
    }

    static void readValue(Printer &p, FieldDescriptor::Type type, absl::string_view name,
        absl::string_view len = "len")
    {
        auto vars = p.WithVars({{"name", name}, {"len", len}});
        switch (type) {
        case FieldDescriptor::TYPE_BOOL:
            p.Emit("$name$ = r.uVar<uint32_t>() != 0;\n");
            break;
        case FieldDescriptor::TYPE_INT32:
        case FieldDescriptor::TYPE_UINT32:
            p.Emit("$name$ = r.uVar<uint32_t>();\n");
            break;
        case FieldDescriptor::TYPE_INT64:
        case FieldDescriptor::TYPE_UINT64:
            p.Emit("$name$ = r.uVar<uint64_t>();\n");
            break;
        case FieldDescriptor::TYPE_SINT32:
            p.Emit("$name$ = r.iVar<int32_t>();\n");
            break;
        case FieldDescriptor::TYPE_SINT64:
            p.Emit("$name$ = r.iVar<int64_t>();\n");
            break;

        case FieldDescriptor::TYPE_FIXED32:
            p.Emit("$name$ = r.u32L();\n");
            break;
        case FieldDescriptor::TYPE_SFIXED32:
            p.Emit("$name$ = r.i32L();\n");
            break;
        case FieldDescriptor::TYPE_FLOAT:
            p.Emit("$name$ = r.f32L();\n");
            break;

        case FieldDescriptor::TYPE_FIXED64:
            p.Emit("$name$ = r.u64L();\n");
            break;
        case FieldDescriptor::TYPE_SFIXED64:
            p.Emit("$name$ = r.o64L();\n");
            break;
        case FieldDescriptor::TYPE_DOUBLE:
            p.Emit("$name$ = r.f64L();\n");
            break;

        case FieldDescriptor::TYPE_STRING:
        case FieldDescriptor::TYPE_BYTES:
            p.Emit("$name$.resize($len$);\n");
            p.Emit("r.data($name$);\n");
            break;
        case FieldDescriptor::TYPE_MESSAGE:
            p.Emit("$name$.read(coco::BufferReader(r, $len$));\n");
            break;
        }
    }

    static void sizeValue(Printer &p, WireType wireType, absl::string_view name) {
        auto vars = p.WithVars({{"name", name}});

        // add value size
        switch (wireType) {
        case WireType::I32:
            p.Emit("size += 4;\n");
            break;
        case WireType::I64:
            p.Emit("size += 8;\n");
            break;
        case WireType::VARINT:
            p.Emit("size += uVarSize($name$);\n");
            break;
        case WireType::LEN:
            p.Emit("size += uVarSize($name$.size()) + $name$.size();\n");
            break;
        }
    }

    static void writeValue(Printer &p, FieldDescriptor::Type type, absl::string_view name) {
        auto vars = p.WithVars({{"name", name}});

        // serialize value
        switch (type) {
        case FieldDescriptor::TYPE_BOOL:
            p.Emit("w.u8($name$ ? 1 : 0);\n");
            break;
        case FieldDescriptor::TYPE_INT32:
        case FieldDescriptor::TYPE_INT64:
        case FieldDescriptor::TYPE_UINT32:
        case FieldDescriptor::TYPE_UINT64:
            p.Emit("w.uVar($name$);\n");
            break;
        case FieldDescriptor::TYPE_SINT32:
        case FieldDescriptor::TYPE_SINT64:
            p.Emit("w.iVar($name$);\n");
            break;

        case FieldDescriptor::TYPE_FIXED32:
            p.Emit("w.u32L($name$);\n");
            break;
        case FieldDescriptor::TYPE_SFIXED32:
            p.Emit("w.i32L($name$);\n");
            break;
        case FieldDescriptor::TYPE_FLOAT:
            p.Emit("w.f32L($name$);\n");
            break;

        case FieldDescriptor::TYPE_FIXED64:
            p.Emit("w.u64L($name$);\n");
            break;
        case FieldDescriptor::TYPE_SFIXED64:
            p.Emit("w.i64L($name$);\n");
            break;
        case FieldDescriptor::TYPE_DOUBLE:
            p.Emit("w.f64L($name$);\n");
            break;

        case FieldDescriptor::TYPE_STRING:
        case FieldDescriptor::TYPE_BYTES:
            p.Emit("w.uVar($name$.size());\n");
            p.Emit("w.data($name$);\n");
            break;
        case FieldDescriptor::TYPE_MESSAGE:
            p.Emit("w.uVar($name$.size());\n");
            p.Emit("$name$.write(w);\n");
            break;
        }
    }

    static void writeTemplateParameters(Printer &p, const Descriptor *type, const std::string &prefix, bool &first) {
        int fieldCount = type->field_count();
        for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
            const FieldDescriptor *field = type->field(fieldIndex);
            FieldDescriptor::Type type = field->type();
            auto name = prefix + field->name();

            if (field->is_repeated()) {
                if (first) {
                    first = false;
                    p.Emit("template <");
                } else {
                    p.Emit(", ");
                }
                p.Emit({{"name", name}}, "int A_$name$");
            }
            if (type == FieldDescriptor::TYPE_STRING || type == FieldDescriptor::TYPE_BYTES) {
                if (first) {
                    first = false;
                    p.Emit("template <");
                } else {
                    p.Emit(", ");
                }
                p.Emit({{"name", name}}, "int B_$name$");
            } else if (type == FieldDescriptor::TYPE_MESSAGE) {
                auto messageType = field->message_type();
                writeTemplateParameters(p, messageType, prefix + messageType->name() + '_', first);
            }
        }
    }

    static void addTemplateParameters(std::string &s, const Descriptor *type, const std::string &prefix, bool &first) {
        int fieldCount = type->field_count();
        for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
            const FieldDescriptor *field = type->field(fieldIndex);
            FieldDescriptor::Type type = field->type();
            auto name = prefix + field->name();

            if (field->is_repeated()) {
                if (first) {
                    first = false;
                    s += '<';
                } else {
                    s += ", ";
                }
                s += "A_" + name;
            }
            if (type == FieldDescriptor::TYPE_STRING || type == FieldDescriptor::TYPE_BYTES) {
                if (first) {
                    first = false;
                    s += '<';
                } else {
                    s += ", ";
                }
                s += "B_" + name;
            } else if (type == FieldDescriptor::TYPE_MESSAGE) {
                auto messageType = field->message_type();
                addTemplateParameters(s, messageType, prefix + messageType->name() + '_', first);
            }
        }
    }

    bool Generate(const FileDescriptor* file,
        const std::string& parameter,
        GeneratorContext* context,
        std::string* error) const override
    {
        std::string path = file->name() + ".hpp";
        auto stream = context->Open(path);
        Printer::Options options;
        options.spaces_per_indent = 4;
        Printer p(stream, options);


        int typeCount = file->message_type_count();
        for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex) {
            const Descriptor *type = file->message_type(typeIndex);
            int fieldCount = type->field_count();

            // template parameters (maximum string and array lengths)
            bool first = true;
            writeTemplateParameters(p, type, "", first);
            if (!first)
                p.Emit(">\n");


            // class
            p.Emit({{"name", type->name()}}, "class $name$ {\n");
            p.Emit("public:\n");
            p.Indent();

            // fields
            int wireTypeFlags = 0;
            for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                const FieldDescriptor *field = type->field(fieldIndex);
                auto name = field->name();
                auto type = field->type();
                auto wireType = wireTypes[int(type)];
                std::string cppType = field->cpp_type_name();

                if (type == FieldDescriptor::TYPE_STRING) {
                    // use fixed size string buffer
                    cppType = "coco::StringBuffer<B_" + name + ">";
                } else if (type == FieldDescriptor::TYPE_BYTES) {
                    // use fixed size data buffer
                    cppType = "coco::DataBuffer<uint8_t, B_" + name + ">";
                } else if (type == FieldDescriptor::TYPE_MESSAGE) {
                    // get name of message type
                    cppType = field->message_type()->full_name();
                    bool first = true;
                    auto messageType = field->message_type();
                    addTemplateParameters(cppType, messageType, messageType->name() + '_', first);
                    if (!first)
                        cppType += ">";
                }

                if (field->has_presence()) {
                    p.Emit({{"type", cppType}}, "std::optional<$type$>");
                } else if (field->is_repeated()) {
                    p.Emit({{"name", name}, {"type", cppType}}, "coco::ArrayBuffer<$type$, A_$name$>");
                } else {
                    p.Emit(cppType);
                }
                p.Emit({{"name", name}}, " $name$;\n");

                wireTypeFlags |= 1 << int(wireType);
            }
            p.Emit("\n");


            // read method
            p.Emit("void read(coco::BufferReader &r) {\n");
            p.Indent();
            p.Emit("while (!r.atEnd()) {\n");
            p.Indent();
            p.Emit("int x = r.uVar<int>();\n");
            p.Emit("int id = x >> 3;\n");
            p.Emit("int wireType = x & 7;\n");

            p.Emit("switch (wireType) {\n");

            // I32
            p.Emit("case 5: // I32\n");
            p.Indent();
            if (wireTypeFlags & (1 << int(WireType::I32))) {
                p.Emit("switch (id) {\n");
                for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                    const FieldDescriptor *field = type->field(fieldIndex);
                    int id = field->number();
                    FieldDescriptor::Type type = field->type();
                    auto wireType = wireTypes[int(type)];

                    if (!field->is_repeated() && wireType == WireType::I32) {
                        p.Emit({{"id", std::to_string(id)}}, "case $id$:\n");
                        p.Indent();
                        readValue(p, type, "this->" + field->name());
                        p.Emit("break;\n");
                        p.Outdent();
                    }
                }
                p.Emit("default:\n");
                p.Indent();
                p.Emit("r.skip(4);\n");
                p.Outdent();
                p.Emit("}\n"); // switch (id)
            } else {
                p.Emit("r.skip(4);\n");
            }
            p.Emit("break;\n");
            p.Outdent();

            // I64
            p.Emit("case 1: // I64\n");
            p.Indent();
            if (wireTypeFlags & (1 << int(WireType::I64))) {
                p.Emit("switch (id) {\n");
                for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                    const FieldDescriptor *field = type->field(fieldIndex);
                    int id = field->number();
                    FieldDescriptor::Type type = field->type();
                    auto wireType = wireTypes[int(type)];

                    if (!field->is_repeated() && wireType == WireType::I64) {
                        p.Emit({{"id", std::to_string(id)}}, "case $id$:\n");
                        p.Indent();
                        readValue(p, type, "this->" + field->name());
                        p.Emit("break;\n");
                        p.Outdent();
                    }
                }
                p.Emit("default:\n");
                p.Indent();
                p.Emit("r.skip(8);\n");
                p.Outdent();
                p.Emit("}\n"); // switch (id)
            } else {
                p.Emit("r.skip(8);\n");
            }
            p.Emit("break;\n");
            p.Outdent();

            // VARINT
            p.Emit("case 0: // VARINT\n");
            p.Indent();
            if (wireTypeFlags & (1 << int(WireType::VARINT))) {
                p.Emit("switch (id) {\n");
                for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                    const FieldDescriptor *field = type->field(fieldIndex);
                    int id = field->number();
                    FieldDescriptor::Type type = field->type();
                    auto wireType = wireTypes[int(type)];

                    if (!field->is_repeated() && wireType == WireType::VARINT) {
                        p.Emit({{"id", std::to_string(id)}}, "case $id$:\n");
                        p.Indent();
                        readValue(p, type, "this->" + field->name());
                        p.Emit("break;\n");
                        p.Outdent();
                    }
                }
                p.Emit("default:\n");
                p.Indent();
                p.Emit("r.uVar<uint32_t>();\n");
                p.Outdent();
                p.Emit("}\n"); // switch (id)
            } else {
                p.Emit("r.uVar<uint32_t>();\n");
            }
            p.Emit("break;\n");
            p.Outdent();

            // LEN
            p.Emit("case 2: // LEN\n");
            p.Indent();
            if (wireTypeFlags & (1 << int(WireType::LEN))) {
                p.Emit("{\n");
                p.Indent();
                p.Emit("int len = r.uVar<int>();\n");
                p.Emit("uint8_t *end = r + len;\n");
                p.Emit("switch (id) {\n");
                for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                    const FieldDescriptor *field = type->field(fieldIndex);
                    int id = field->number();
                    FieldDescriptor::Type type = field->type();
                    auto wireType = wireTypes[int(type)];
                    std::string name = "this->" + field->name();
                    //auto vars = p.WithVars({{"name", field->has_presence() ? "(*" + name + ")" : name}});
                    auto vars = p.WithVars({{"name", name}, {"len", "len"}});

                    if (field->is_repeated()) {
                        p.Emit({{"id", std::to_string(id)}}, "case $id$:\n");
                        p.Indent();
                        switch (wireType) {
                        case WireType::I32:
                            p.Emit("$name$.resize(len / 4);\n");
                            p.Emit("for (auto &v : $name$) {\n");
                            p.Indent();
                            readValue(p, type, "v");
                            p.Outdent();
                            p.Emit("}\n");
                            break;
                        case WireType::I64:
                            p.Emit("$name$.resize(len / 8);\n");
                            p.Emit("for (auto &v : $name$) {\n");
                            p.Indent();
                            readValue(p, type, "v");
                            p.Outdent();
                            p.Emit("}\n");
                            break;
                        case WireType::VARINT:
                            p.Emit("while (r - begin < len && $name$.size() < $name$.capacity()) {\n");
                            p.Indent();
                            readValue(p, type, name + ".emplace_back()");
                            p.Outdent();
                            p.Emit("}\n");
                            break;
                        case WireType::LEN:
                            p.Emit("while (r - begin < len && $name$.size() < $name$.capacity()) {\n");
                            p.Indent();
                            p.Emit("int len2 = r.uVar();\n");
                            p.Emit("uint8_t *end2 = r + len2;\n");
                            p.Emit("auto &v = $name$.emplace_back();\n");
                            readValue(p, type, "v", "len2");
                            p.Emit("r.set(end2);\n");
                            p.Outdent();
                            p.Emit("}\n");
                            break;
                        }
                        p.Emit("break;\n");
                        p.Outdent();
                    } else if (wireType == WireType::LEN) {
                        p.Emit({{"id", std::to_string(id)}}, "case $id$:\n");
                        p.Indent();
                        if (field->has_presence()) {
                            p.Emit("{\n");
                            p.Indent();
                            p.Emit("auto &v = $name$.emplace();\n");
                            readValue(p, type, "v");
                            p.Outdent();
                            p.Emit("}\n");
                        } else {
                            readValue(p, type, name);
                        }
                        p.Emit("break;\n");
                        p.Outdent();
                    }
                }

                p.Emit("}\n"); // switch (id)
                p.Emit("r.set(end);\n");

                p.Outdent();
                p.Emit("}\n"); // scope for int len
            } else {
                p.Emit("r.skip(r.uVar<int>());\n");
            }
            p.Emit("break;\n");
            p.Outdent();

            // default
            p.Emit("default:\n");
            p.Indent();
            p.Emit("return;\n");
            p.Outdent();

            p.Emit("}\n"); // switch (wireType)

            p.Outdent();
            p.Emit("}\n"); // while (!r.atEnd())

            p.Outdent();
            p.Emit("}\n\n"); // void read()


            // size method
            p.Emit("int size() {\n");
            p.Indent();
            p.Emit("int size = 0;\n");
            for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                const FieldDescriptor *field = type->field(fieldIndex);
                int id = field->number();
                FieldDescriptor::Type type = field->type();
                auto wireType = wireTypes[int(type)];
                std::string name = "this->" + field->name();
                auto vars = p.WithVars({{"name", name}});

                if (field->is_repeated() || (!field->has_presence() && wireType == WireType::LEN))
                    p.Emit("if (!$name$.empty()) {\n");
                else
                    p.Emit("if ($name$) {\n");
                p.Indent();

                if (!field->is_repeated()) {
                    // add size of type and id
                    p.Emit({{"size", std::to_string(uVar(id << 3 | int(wireType)))}}, "size += $size$;\n");

                    // add size of value
                    if (!field->has_presence()) {
                        if (wireType != WireType::LEN) {
                            // scalar
                            sizeValue(p, wireType, name);
                        } else {
                            // string, bytes or message
                            p.Emit("auto &v = $name$;\n");
                            sizeValue(p, wireType, "v");
                        }
                    } else {
                        if (wireType != WireType::LEN) {
                            // optional scalar
                            sizeValue(p, wireType, '*' + name);
                        } else {
                            // optional string, bytes or message
                            p.Emit("auto &v = *$name$;\n");
                            sizeValue(p, wireType, "v");
                        }
                    }
                } else if (wireType != WireType::LEN) {
                    // repeated scalar type

                    // add size of type and id
                    p.Emit({{"size", std::to_string(uVar(id << 3 | int(WireType::LEN)))}}, "size += $size$;\n");

                    // add array length
                    switch (wireType) {
                    case WireType::I32:
                        p.Emit("int s = $name$.size() * 4;\n");
                        p.Emit("size += uVarLen(s) + s;\n");
                        break;
                    case WireType::I64:
                        p.Emit("int s = $name$.size() * 8;\n");
                        p.Emit("size += uVarLen(s) + s;\n");
                        break;
                    case WireType::VARINT:
                        p.Emit("int s = 0;\n");
                        p.Emit("for (auto &v : $name$) {\n");
                        p.Indent();
                        if (type == FieldDescriptor::TYPE_SINT32 || type == FieldDescriptor::TYPE_SINT64)
                            p.Emit("s += iVarSize(v);\n");
                        else
                            p.Emit("s += uVarSize(v);\n");
                        p.Outdent();
                        p.Emit("}\n");
                        p.Emit("size += w.uVarSize(s) + s;\n");
                        break;
                    }
                } else {
                    // repeated string, bytes or message

                    // add size of array contents
                    p.Emit("for (auto &v : $name$) {\n");
                    p.Indent();

                    // add size of type and id
                    p.Emit({{"size", std::to_string(uVar(id << 3 | int(wireType)))}}, "size += $size$;\n");

                    // add size of value
                    sizeValue(p, wireType, "v");

                    p.Outdent();
                    p.Emit("}\n");
                }
                p.Outdent();
                p.Emit("}\n"); // if ($name$)
            }
            p.Emit("return size;\n");
            p.Outdent();
            p.Emit("}\n\n"); // int size()


            // write method
            p.Emit("void write(coco::BufferWriter &w) {\n");
            p.Indent();
            for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
                const FieldDescriptor *field = type->field(fieldIndex);
                int id = field->number();
                FieldDescriptor::Type type = field->type();
                auto wireType = wireTypes[int(type)];
                //FieldDescriptor::CppType cppType = field->cpp_type();
                std::string name = "this->" + field->name();
                auto vars = p.WithVars({{"name", name}});

                if (field->is_repeated() || (!field->has_presence() && wireType == WireType::LEN))
                    p.Emit("if (!$name$.empty()) {\n");
                else
                    p.Emit("if ($name$) {\n");
                p.Indent();

                if (!field->is_repeated()) {
                    // serialize type and id
                    p.Emit({{"id", std::to_string(id)}, {"wireType", std::to_string(int(wireType))}}, "w.uVar(($id$ << 3) | $wireType$);\n");

                    // serialize value
                    if (!field->has_presence()) {
                        if (wireType != WireType::LEN) {
                            // scalar
                            writeValue(p, type, name);
                        } else {
                            // string, bytes or message
                            p.Emit("auto &v = $name$;\n");
                            writeValue(p, type, "v");
                        }
                    } else {
                        if (wireType != WireType::LEN) {
                            // optional scalar
                            writeValue(p, type, '*' + name);
                        } else {
                            // optional string, bytes or message
                            p.Emit("auto &v = *$name$;\n");
                            writeValue(p, type, "v");
                        }
                    }
                } else if (wireType != WireType::LEN) {
                    // repeated scalar type

                    // serialize type and id
                    p.Emit({{"id", std::to_string(id)}, {"wireType", std::to_string(int(WireType::LEN))}}, "w.uVar(($id$ << 3) | $wireType$);\n");

                    // serialize array length
                    switch (wireType) {
                    case WireType::I32:
                        p.Emit("w.uVar($name$.size() * 4);\n");
                        break;
                    case WireType::I64:
                        p.Emit("w.uVar($name$.size() * 8);\n");
                        break;
                    case WireType::VARINT:
                        p.Emit("int s = 0;\n");
                        p.Emit("for (auto &v : $name$) {\n");
                        p.Indent();
                        if (type == FieldDescriptor::TYPE_SINT32 || type == FieldDescriptor::TYPE_SINT64)
                            p.Emit("s += iVarSize(v);\n");
                        else
                            p.Emit("s += uVarSize(v);\n");
                        p.Outdent();
                        p.Emit("}\n");
                        p.Emit("w.uVar(s);\n");
                        break;
                    }

                    // serialize array contents
                    p.Emit("for (auto &v : $name$) {\n");
                    p.Indent();

                    // serialize value
                    writeValue(p, type, "v");

                    p.Outdent();
                    p.Emit("}\n");
                } else {
                    // repeated string, bytes or message

                    // serialize array contents
                    p.Emit("for (auto &v : $name$) {\n");
                    p.Indent();

                    // serialize type and id
                    p.Emit({{"id", std::to_string(id)}, {"wireType", std::to_string(int(wireType))}}, "w.uVar(($id$ << 3) | $wireType$);\n");

                    // serialize value
                    writeValue(p, type, "v");

                    p.Outdent();
                    p.Emit("}\n");
                }
                p.Outdent();
                p.Emit("}\n"); // if ($name$)
            }
            p.Outdent();
            p.Emit("}\n"); // void write()

            p.Outdent();
            p.Emit("};\n\n"); // class
        }

        return true;
    }
};

// https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/compiler/plugin.proto
// A plugin executable needs only to be placed somewhere in the path.  The
// plugin should be named "protoc-gen-$NAME", and will then be used when the
// flag "--${NAME}_out" is passed to protoc.
int main(int argc, char *argv[]) {
    CocoGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
