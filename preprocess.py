import os, sys, re, string


def object_specifiers():
    object_specifiers = [
        "class",
        "enum",
        "struct",
    ]
    return object_specifiers

def ignores():
    ignores = [
        "private",
        "public",
        "protected",
    ]
    return ignores


def forwardings():
    forwardings = [
        "shared_ptr",
        "unique_ptr",
        "vector",
        "array",
    ]
    return forwardings


def access():
    access = [
        "const",
    ]
    return access


def appendices():
    appendices = [
        "&",
        "*",
        "[",
    ]
    return appendices


def delimiters():
    delimiters = {
        "(": "d_lparenthesis",
        ")": "d_rparenthesis",
        "[": "d_lbracket",
        "]": "d_rbracket",
        "{": "d_lbrace",
        "}": "d_rbrace",
        "=": "d_assign",
        ",": "d_comma",
        ";": "d_semicolon",
        "<": "d_less",
        ">": "d_greater",
        "//": "d_comment",
        "/!": "d_doc",
        ".": "d_point",
        "\r": "d_cr",
        "\n": "d_lf",
        "\t": "d_tab",
        " ": "d_space",
    }
    return delimiters


def is_whitespace(character):
    return character in ["", " ", "\t", "\r", "\n"]


def print_template_functions(struct_name):
    print('    template<>')
    print('    [[maybe_unused]]')
    print('    std::vector<field_definition> get_fields<', struct_name, '>()', sep = '')
    print('    {')
    print('        return ', struct_name, '_fields;', sep = '')
    print('    }\n')
    print('    template<>')
    print('    [[maybe_unused]]')
    print('    void* get_field_value_pointer<', struct_name, '>(const ', struct_name, '& instance, const field_definition& field)', sep = '')
    print('    {')
    print('        return ((uint8*)&instance) + field.offset;')
    print('    }\n')

def get_reference_type(ref_info):
    switch_case = {
        '' : 'value_type',
        '*' : 'c_pointer_type',
        'shared_ptr' : 'shared_pointer_type',
        'unique_ptr' : 'unique_pointer_type',
        'vector' : 'std_vector_type',
    }
    if '::' in ref_info:
        pos = ref_info.find('::')
        ref_info = ref_info[pos + 2 :]  # remove namespaces
    return switch_case.get(ref_info, 'unknown_type')




def tokenize_line(line):
    tokens = line.split(" ")
    for delimiter in delimiters().keys():
        for i in range(len(tokens)):
            token = tokens[i]
            if token != delimiter and delimiter in token:
                pos = token.find(delimiter)
                tokens[i] = " "
                before = token[:pos]
                after = token[pos + 1 :]
                tokens.append(before)
                tokens.append(delimiter)
                tokens.append(after)
            else:
                tokens[i] = " "
                tokens.append(token)

        tokens = [t for t in tokens if not is_whitespace(t)]

    return tokens


def tokenize_meta(path):
    meta = False
    start_multi_line_c = False
    multi_line_c = False
    tokens = []
    with open(path, "r") as file:
        print("    // File contains definitions for:")
        for line_count, line in enumerate(file):
            if "meta(reflect)" in line:
                print("    //   line:", line_count, "in", path)
                meta = True
            if meta:
                line_tokens = tokenize_line(line)
                start_idx = 0
                end_idx = len(line_tokens)
                if "//" in line_tokens:
                    end_idx = line_tokens.index("//")
                if "=" in line_tokens:
                    end_idx = line_tokens.index("=") + 1
                    line_tokens[end_idx - 1] = ";"  # ignore assignments
                if "/*" in line_tokens:
                    end_idx = line_tokens.index("/*")
                    start_multi_line_c = True
                if "*/" in line_tokens:
                    start_idx = line_tokens.index("*/") + 1
                    multi_line_c = False
                if not multi_line_c:
                    tokens.extend(line_tokens[start_idx:end_idx])
                if start_multi_line_c:
                    multi_line_c = True
                    start_multi_line_c = False
            if "}" in line:
                meta = False
        print('    //')
    return tokens


def generate_meta(tokens):
    _ignores = ignores()
    _delimiters = delimiters().keys()
    _object_specifiers = object_specifiers()
    _forwardings = forwardings()
    _appendices = appendices()
    _access = access() # unused at the moment
    enum_types = []
    name = False
    field_definition = False
    struct_name = None
    member = [''] * 3
    fields_end = False
    for token in tokens:
        if any(p in token for p in _ignores):
            continue
        elif token in _object_specifiers:
            field_definition = True
        elif any(p in token for p in _appendices) and not name:
            member[0] = token[-1:]
            member[1] = token[:-1]
            name = True
            continue
        elif any(p in token for p in _forwardings) and not name:
            member[0] = token
            continue
        elif token not in _delimiters:
            if field_definition:
                struct_name = token
                print("    std::vector<field_definition> ", struct_name, "_fields = {", sep="")
                field_definition = False
                fields_end = False
                continue
            idx = 1
            if name:
                idx = 2
            elif '::' in token:
                pos = token.find('::')
                token = token[pos + 2 :]  # remove namespaces from names
            name = not name
            member[idx] = token
        else:
            if token == "}":
                print("    ", token, sep="", end=";\n\n")
                print_template_functions(struct_name)
                name = False
                member = [None] * 3
                fields_end = True
                struct_name = None
                continue
            if token == ";" and not fields_end:
                print("        {", end=" ")
                print(' ', get_reference_type(member[0]), end=", ")
                if member[1] != '':
                    type_member = "type_" + member[1]
                    if not type_member in enum_types:
                        enum_types.append(type_member)
                    print(type_member, end=", ")
                if member[2] != '':
                    print('"', member[2], '"', sep="", end=", ")

                print("offsetof(", struct_name, ", ", member[2], sep="", end=") ")
                print("},")
                member = [''] * 3
    return enum_types


def main(argv):
    print("Creating reflection data.")
    paths = [#"mango/include/mango/scene_ecs.hpp",
             "mango/src/ui/ui_system_impl.hpp"
            ]

    enum_types = []

    with open("tmp.txt", "w") as tmp_file:
        sys.stdout = tmp_file
        tokens = []
        for path in paths:
            tokens.extend(tokenize_meta(path)[4:])
        enum_types.extend(generate_meta(tokens))

    with open("tmp.txt") as tmp_file:
        with open("mango/include/mango/meta.hpp", "w") as out_file:
            sys.stdout = out_file
            print("//! \\file      meta.hpp")
            print("//! \\author    Paul Himmler")
            print("//! \\version   1.0")
            print("//! \\date      2020")
            print("//! \\copyright Apache License 2.0\n")
            print("#ifndef MANGO_META_HPP")
            print("#define MANGO_META_HPP\n")

            print("#include <mango/types.hpp>")
            print("#include <stddef.h>")
            print("#include <vector>\n")

            print("// Parsed files")
            print("#include <mango/scene_ecs.hpp>\n")
            print("#include <ui/ui_system_impl.hpp>\n")

            print("namespace mango")
            print("{")
            # forward
            print("    enum meta_t")
            print("    {")
            for e_t in enum_types:
                print("        ", e_t, sep="", end=",\n")
            print("    };")
            print("    enum reference_t")
            print("    {")
            print("        value_type,")
            print("        c_pointer_type,")
            print("        shared_pointer_type,")
            print("        unique_pointer_type,")
            print("        std_vector_type")
            print("    };")
            print("    struct field_definition")
            print("    {")
            print("        reference_t ref_type;")
            print("        meta_t meta_type;")
            print("        string name;")
            print("        uint32 offset;")
            print("    };\n")
            print('    template<typename T>')
            print('    std::vector<field_definition> get_fields();')
            print('    template<typename T>')
            print('    void* get_field_value_pointer(const T& instance, const field_definition& field);\n')

            for line in tmp_file:
                out_file.write(line)

            print("} // namespace mango\n")
            print("#endif // MANGO_META_HPP")
    os.remove("tmp.txt")

if __name__ == "__main__":
    main(sys.argv[1:])

