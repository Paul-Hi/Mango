from io import TextIOWrapper
from math import ceil, log2
import os
from typing import Tuple
from typing import List
import re


def component_size_count_row_col(type: str) -> Tuple[int, int, int, int]:
    if type == 'float' or type == 'int' or type == 'uint' or type == 'int32' or type == 'uint32' or type == 'bool':
        return 4, 1, 0, 0
    if type == 'vec2' or type == 'ivec2' or type == 'uvec2' or type == 'bvec2':
        return 4, 2, 0, 0
    if type == 'vec3' or type == 'ivec3' or type == 'uvec3' or type == 'bvec3':
        return 4, 3, 0, 0
    if type == 'vec4' or type == 'ivec4' or type == 'uvec4' or type == 'bvec4':
        return 4, 4, 0, 0
    if type == 'mat2':
        return 4, 4, 2, 2
    if type == 'mat3':
        return 4, 9, 3, 3
    if type == 'mat4':
        return 4, 16, 4, 4

    if type == 'double':
        return 8, 1, 0, 0
    if type == 'dvec2':
        return 8, 2, 0, 0
    if type == 'dvec3':
        return 8, 3, 0, 0
    if type == 'dvec4':
        return 8, 4, 0, 0
    if type == 'dmat2':
        return 8, 4, 2, 2
    if type == 'dmat3':
        return 8, 9, 3, 3
    if type == 'dmat4':
        return 8, 16, 4, 4

    return 0, 0, 0, 0


def offset_align(unaligned_offset: int, alignment: int) -> int:
    return ((unaligned_offset + (alignment - 1)) & ~(alignment - 1))


class buffer:
    def __init__(self, name, layout):
        self.name = name
        self.layout = layout
        self.elements = []


class buffer_element:
    def __init__(self, name, type):
        self.name = name
        self.type = type
        self.comp_size = 0
        self.comp_count = 0

        self.rows = 0
        self.cols = 0

        self.array_size = 0  # 0 -> no array, > 0 -> fixed size array, -1 -> dynamic size array

        self.alignment = 0
        self.buffer_offset = 0
        self.byte_size = 0

# https://gist.github.com/teoxoy/936891c16c2a3d1c3c5e7204ac6cd76c


def po(n): return pow(2, ceil(log2(n)))
def round_up(k, n): return ceil(n / k) * k


def align_and_size_of(element: buffer_element, layout: str) -> Tuple[int, int]:

    if layout == 'std140':
        if element.array_size == 0:
            if element.rows == 0:  # cols have to be 0 as well!
                return po(element.comp_count * element.comp_size), element.comp_count * element.comp_size
            else:
                assert(element.rows == element.cols,
                       'Only quadratic matrices supported atm.')
                align = round_up(16, element.rows * element.comp_size)
                return align, align * element.cols
        elif element.array_size > 0:
            align_of_e = po(element.comp_count * element.comp_size)
            size_of_e = element.comp_count * element.comp_size
            align = round_up(16, align_of_e)
            return align, round_up(align, size_of_e * element.array_size)
        else:  # array_size == -1 -> dynamic
            raise NotImplementedError
    elif layout == 'std430':
        if element.array_size == 0:
            if element.rows == 0:  # cols have to be 0 as well!
                return po(element.comp_count * element.comp_size), element.comp_count * element.comp_size
            else:
                assert(element.rows == element.cols,
                       'Only quadratic matrices supported atm.')
                return po(element.rows * element.comp_size), element.comp_count * element.comp_size
        elif element.array_size > 0:
            align_of_e = po(element.comp_count * element.comp_size)
            size_of_e = element.comp_count * element.comp_size
            align = round_up(16, align_of_e)
            return align_of_e, round_up(align, size_of_e * element.array_size)
        else:  # array_size == -1 -> dynamic
            raise NotImplementedError
    else:
        print('Error, unknown std layout', layout)


def offset_of(element: buffer_element, current_offset: int) -> Tuple[int, int]:
    buffer_offset = offset_align(current_offset, element.alignment)
    return buffer_offset, buffer_offset + element.byte_size


def cpp_type(type: str):
    if type == 'int':
        return 'std140_int32'
    if type == 'uint':
        return 'std140_uint32'
    return 'std140_' + type


def cpp_array_type(type: str, n: int):
    return 'std140_array<' + cpp_type(type) + ', ' + str(n) + '>'


def parse_shader_buffers(files: List[str]) -> List[buffer]:
    buffers: List[buffer] = list()

    for file in files:
        with open(file, 'r') as f:
            lines = f.readlines()

        current = buffer('', '')
        level = 0
        current_offset = 0
        active_buffer = False
        defines = dict()
        for line in lines:
            offset = line.find('{')
            if offset >= 0:
                level += 1
                offset = line.find('}', offset)
                if offset >= 0:
                    level = 1
                continue

            offset = line.find('}')
            if offset >= 0:
                level -= 1
                if level == 0:
                    active_buffer = False
                continue

            if level == 0:
                pattern = re.compile(
                    r'[a-zA-Z]+\([a-zA-Z]+\s+=\s+(?:[a-zA-Z]+(?:_[a-zA-Z]+)*),\s+([a-zA-Z]+[0-9]+)\)\s+([a-zA-Z]+)\s+((?:[a-zA-Z]+(?:_[a-zA-Z]+)+))', re.IGNORECASE)
                matched = pattern.match(line)
                if matched:
                    if len(current.elements) != 0:
                        buffers.append(current)

                    current_offset = 0
                    current = buffer(matched.group(3), matched.group(1))
                    active_buffer = True
                    continue
                pattern = re.compile(
                    r'\s*#define+\s+([_a-zA-Z0-9]+)\s+([0-9]+).*', re.IGNORECASE)
                matched = pattern.match(line)
                if matched:
                    defines[matched.group(1)] = matched.group(2)
                    continue

            if level == 1 and active_buffer:
                pattern = re.compile(
                    r'\s*([a-zA-Z]+(?:[0-9]+)?)\s+([a-zA-Z]+(?:_[a-zA-Z]+)*)(\[([_a-zA-Z0-9]+)*\])?;.*', re.IGNORECASE)
                matched = pattern.match(line)
                if matched:
                    element = buffer_element(
                        matched.group(2), matched.group(1))

                    element.array_size = 0
                    if matched.group(4):
                        element.array_size = int(matched.group(4)) if matched.group(
                            4).isnumeric() else int(defines[matched.group(4)])
                    elif matched.group(3):
                        element.array_size = -1

                    element.comp_size, element.comp_count, element.rows, element.cols = component_size_count_row_col(
                        element.type)
                    element.alignment, element.byte_size = align_and_size_of(
                        element, current.layout)
                    element.buffer_offset, current_offset = offset_of(
                        element, current_offset)
                    current.elements.append(element)
                    continue

        if len(current.elements) != 0:
            buffers.append(current)

    return buffers

# returns string std140 type, standard type
def get_primitivestd140_type_data() -> List[Tuple[str, str]]:
    return [
        ('std140_uint32', 'uint32'),
        ('std140_int32', 'int32'),
        ('std140_float', 'float'),
        ('std140_double', 'double'),
        ('std140_bool', 'bool'),
        ('std140_vec2', 'vec2'),
        ('std140_vec3', 'vec3'),
        ('std140_vec4', 'vec4'),
        ('std140_ivec2', 'ivec2'),
        ('std140_ivec3', 'ivec3'),
        ('std140_ivec4', 'ivec4'),
        ('std140_uvec2', 'uvec2'),
        ('std140_uvec3', 'uvec3'),
        ('std140_uvec4', 'uvec4'),
        ('std140_dvec2', 'dvec2'),
        ('std140_dvec3', 'dvec3'),
        ('std140_dvec4', 'dvec4'),
        ('std140_bvec2', 'bvec2'),
        ('std140_bvec3', 'bvec3'),
        ('std140_bvec4', 'bvec4'),
        ('std140_mat2', 'mat2'),
        ('std140_mat3', 'mat3'),
        ('std140_mat4', 'mat4'),
        ('std140_dmat2', 'dmat2'),
        ('std140_dmat3', 'dmat3'),
        ('std140_dmat4', 'dmat4'),
    ]


def write_type_defines(f: TextIOWrapper):
    # simple first
    f.write('using std140_uint32 = uint32;' + '\n')
    f.write('using std140_int32  = int32;' + '\n')
    f.write('using std140_float  = float;' + '\n')
    f.write('using std140_double = double;' + '\n')
    f.write('using std140_vec2   = vec2;' + '\n')
    f.write('using std140_vec3   = vec3;' + '\n')
    f.write('using std140_vec4   = vec4;' + '\n')
    f.write('using std140_ivec2  = ivec2;' + '\n')
    f.write('using std140_ivec3  = ivec3;' + '\n')
    f.write('using std140_ivec4  = ivec4;' + '\n')
    f.write('using std140_uvec2  = uvec2;' + '\n')
    f.write('using std140_uvec3  = uvec3;' + '\n')
    f.write('using std140_uvec4  = uvec4;' + '\n')
    f.write('using std140_dvec2  = dvec2;' + '\n')
    f.write('using std140_dvec3  = dvec3;' + '\n')
    f.write('using std140_dvec4  = dvec4;' + '\n')
    f.write('using std140_mat2   = mat2;' + '\n')
    f.write('using std140_mat4   = mat4;' + '\n')
    f.write('using std140_dmat2  = dmat2;' + '\n')
    f.write('using std140_dmat4  = dmat4;' + '\n')
    f.write('\n')
    # std140 bool has to be 4 bytes in size
    f.write('struct std140_bool' + '\n')
    f.write('{' + '\n')
    f.write('    std140_bool(const bool& b)' + '\n')
    f.write('    {' + '\n')
    f.write('        pad = 0;' + '\n')
    f.write('        v   = b;' + '\n')
    f.write('    }' + '\n')
    f.write('    std140_bool()' + '\n')
    f.write('    {' + '\n')
    f.write('        pad = 0;' + '\n')
    f.write('        v   = false;' + '\n')
    f.write('    }' + '\n')
    f.write('    operator bool&()' + '\n')
    f.write('    {' + '\n')
    f.write('        return v;' + '\n')
    f.write('    }' + '\n')
    f.write('    void operator=(const bool& o)' + '\n')
    f.write('    {' + '\n')
    f.write('        pad = 0;' + '\n')
    f.write('        v   = o;' + '\n')
    f.write('    }' + '\n')
    f.write('\n')
    f.write('  private:' + '\n')
    f.write('    union' + '\n')
    f.write('    {' + '\n')
    f.write('        bool v;' + '\n')
    f.write('        uint32 pad;' + '\n')
    f.write('    };' + '\n')
    f.write('};' + '\n')
    f.write('\n')
    f.write('using std140_bvec2 = Eigen::Vector2<std140_bool>;' + '\n')
    f.write('using std140_bvec3 = Eigen::Vector3<std140_bool>;' + '\n')
    f.write('using std140_bvec4 = Eigen::Vector4<std140_bool>;' + '\n')
    f.write('\n')
    f.write('template <typename T>' + '\n')
    f.write('struct std140_matrix3' + '\n')
    f.write('{' + '\n')
    f.write('    std140_matrix3(const Eigen::Matrix3<T>& mat)' + '\n')
    f.write('        : m(Eigen::Matrix3<T>::Zero())' + '\n')
    f.write('    {' + '\n')
    f.write('        m.block<0, 0>(3, 3) = mat;' + '\n')
    f.write('    }' + '\n')
    f.write('    std140_matrix3()' + '\n')
    f.write('        : m(Eigen::Matrix<T, 4, 3>::Zero())' + '\n')
    f.write('    {' + '\n')
    f.write('    }' + '\n')
    f.write('    void operator=(const Eigen::Matrix3<T>& o)' + '\n')
    f.write('    {' + '\n')
    f.write('        m.block<3, 3>(0, 0) = o;' + '\n')
    f.write('    }' + '\n')
    f.write('    operator Eigen::Matrix3<T>&()' + '\n')
    f.write('    {' + '\n')
    f.write('        return m.block<3, 3>(0, 0);' + '\n')
    f.write('    }' + '\n')
    f.write('\n')
    f.write('  private:' + '\n')
    f.write('    Eigen::Matrix<T, 4, 3> m;' + '\n')
    f.write('};' + '\n')
    f.write('\n')
    f.write('using std140_mat3  = std140_matrix3<float>;' + '\n')
    f.write('using std140_dmat3 = std140_matrix3<double>;' + '\n')
# TODO: std430 differn for arrays ... -also naming needs to somehow reflect that ...
# TODO: Dynamic sized arrays in general (might do later, since we do not use them atm)
# TODO: Include generated file and test under real conditions! :D
    # base for std140_arrays constrained to the right types
    primitivestd140_type_data = get_primitivestd140_type_data()
    f.write('template <typename T, ptr_size N>' + '\n')
    f.write('struct std140_array' + '\n')
    f.write('{' + '\n')
    f.write('    static_assert(' + '\n')

    for tp in primitivestd140_type_data:
        f.write('                  std::is_same<' +
                tp[0] + ', T>::value ||' + '\n')
    f.write('                  0' + '\n')
    f.write('                  , "Type is not allowed.");' + '\n')
    f.write('};' + '\n')
    f.write('\n')

    for tp in primitivestd140_type_data:
        f.write('template <ptr_size N>' + '\n')
        f.write('struct std140_array<' + tp[0] + ', N>' + '\n')
        f.write('{' + '\n')
        f.write('    void fill_from_list(const ' +
                tp[1] + '* list, uint32 count)' + '\n')
        f.write('    {' + '\n')
        f.write('        MANGO_ASSERT(count == N, "List size not correct!");' + '\n')
        f.write('        for (uint32 i = 0; i < N; ++i)' + '\n')
        f.write('        {' + '\n')
        f.write('            data[i] = internal_element(list[i]);' + '\n')
        f.write('        }' + '\n')
        f.write('    }' + '\n')
        f.write('\n')
        f.write('    ' + tp[1] + '& operator[](uint32 i)' + '\n')
        f.write('    {' + '\n')
        f.write('        MANGO_ASSERT(i < N, "Index out of bounds!");' + '\n')
        f.write('        return data[i].value;' + '\n')
        f.write('    }' + '\n')
        f.write('\n')
        f.write('    std140_array() = default;' + '\n')
        f.write('    ~std140_array() = default;' + '\n')
        f.write('\n')
        f.write('private:' + '\n')
        f.write('    struct internal_element' + '\n')
        f.write('    {' + '\n')
        f.write('        internal_element() = default;' + '\n')
        f.write('        internal_element(const ' + tp[0] + '& v)' + '\n')
        f.write('        {' + '\n')
        f.write('            value = v;' + '\n')
        f.write('        };' + '\n')
        f.write('        ' + tp[0] + ' value;' + '\n')
        comp_size, comp_count, _, _ = component_size_count_row_col(tp[1])
        align_of_e = po(comp_count * comp_size)
        size_of_e = comp_count * comp_size
        array_align = round_up(16, align_of_e)
        stride = round_up(array_align, size_of_e)
        rest = stride - size_of_e
        assert(rest % 4 == 0)
        i = 0
        while rest > 0:
            f.write('        std140_uint32 pad' + str(i) + ' = 0;' + '\n')
            i += 1
            rest -= 4
        f.write('    };' + '\n')
        f.write('\n')
        f.write('    internal_element data[N];' + '\n')
        f.write('};' + '\n')
        f.write('\n')


def generate_interop():
    files = ['./res/shader/include/camera.glsl',
             './res/shader/include/light.glsl',
             './res/shader/include/material.glsl',
             './res/shader/include/model.glsl',
             './res/shader/include/renderer.glsl',
             './res/shader/include/shadow.glsl',
             './res/shader/include/luminance.glsl']
    buffers = parse_shader_buffers(files)
    # TODO: Organize missing reflected files!

    if not os.path.exists('./mango/gen'):
        os.mkdir('./mango/gen')
    out_file = './mango/gen/shader_interop.hpp'
    with open(out_file, 'w') as f:
        # copyright, defines, includes
        f.write('//! \\file      shader_interop.hpp' + '\n')
        f.write('//! \\author    Paul Himmler' + '\n')
        f.write('//! \\version   1.0' + '\n')
        f.write('//! \date      2022' + '\n')
        f.write('//! \copyright Apache License 2.0' + '\n')
        f.write('\n')
        f.write('#ifndef MANGO_SHADER_INTEROP_HPP' + '\n')
        f.write('#define MANGO_SHADER_INTEROP_HPP' + '\n')
        f.write('\n')
        f.write('#include <mango/types.hpp>' + '\n')
        f.write('#include <mango/assert.hpp>' + '\n')
        f.write('\n')
        f.write('namespace mango' + '\n')
        f.write('{' + '\n')
        f.write('\n')
        f.write(
            '// This file is generated. Do NOT change until you want to loose it!\n')
        f.write('\n')

        # define types
        write_type_defines(f)

        for buffer in buffers:
            f.write('\n')
            f.write('struct ' + buffer.name + '\n')
            f.write('{' + '\n')

            current_end = 0
            padding_counter = 0
            for element in buffer.elements:
                while current_end < element.buffer_offset:
                    f.write('    std140_uint32 pad' +
                            str(padding_counter) + ';' + '\n')
                    padding_counter += 1
                    current_end += 4

                if element.array_size == 0:
                    f.write('    ' + cpp_type(element.type) + ' ' +
                            element.name + ';' + '\n')
                elif element.array_size > 0:
                    f.write('    ' + cpp_array_type(element.type,
                            element.array_size) + ' ' + element.name + ';' + '\n')
                else:
                    raise NotImplementedError
                current_end += element.byte_size

            f.write('};' + '\n')

        f.write('\n')
        f.write('} // namespace mango' + '\n')
        f.write('\n')
        f.write('#endif // MANGO_SHADER_INTEROP_HPP' + '\n')


if __name__ == '__main__':
    generate_interop()
