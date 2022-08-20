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


def align_and_size_of_(comp_count, comp_size, rows, cols, array_size, layout):
    if layout == 'std140':
        if array_size == 0:
            if rows == 0:  # cols have to be 0 as well!
                return po(comp_count * comp_size), comp_count * comp_size
            else:
                assert(rows == cols,
                       'Only quadratic matrices supported atm.')
                align = round_up(16, rows * comp_size)
                return align, align * cols
        elif array_size > 0:
            align_of_e, size_of_e = align_and_size_of_(
                comp_count, comp_size, rows, cols, 0, layout)
            align = round_up(16, align_of_e)
            return align, round_up(align, size_of_e * array_size)
        else:  # array_size == -1 -> dynamic
            raise NotImplementedError
    elif layout == 'std430':
        if array_size == 0:
            if rows == 0:  # cols have to be 0 as well!
                return po(comp_count * comp_size), comp_count * comp_size
            else:
                assert(rows == cols,
                       'Only quadratic matrices supported atm.')
                return po(rows * comp_size), comp_count * comp_size
        elif array_size > 0:
            align_of_e, size_of_e = align_and_size_of_(
                comp_count, comp_size, rows, cols, 0, layout)
            return align_of_e, round_up(align_of_e, size_of_e * array_size)
        else:  # array_size == -1 -> dynamic
            raise NotImplementedError
    else:
        print('Error, unknown std layout', layout)


def align_and_size_of(element: buffer_element, layout: str) -> Tuple[int, int]:
    return align_and_size_of_(element.comp_count, element.comp_size, element.rows, element.cols, element.array_size, layout)


def offset_of(element: buffer_element, current_offset: int) -> Tuple[int, int]:
    buffer_offset = offset_align(current_offset, element.alignment)
    return buffer_offset, buffer_offset + element.byte_size


def cpp_type(type: str):
    if type == 'int':
        return 'sl_int32'
    if type == 'uint':
        return 'sl_uint32'
    return 'sl_' + type


def cpp_array_type(type: str, n: int, layout: str):
    return cpp_type(type) + '_array_' + layout + '<' + str(n) + '>'


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

# returns string sl type, standard type


def get_primitive_sl_type_data() -> List[Tuple[str, str]]:
    return [
        ('sl_uint32', 'uint32'),
        ('sl_int32', 'int32'),
        ('sl_float', 'float'),
        ('sl_double', 'double'),
        ('sl_bool', 'bool'),
        ('sl_vec2', 'vec2'),
        ('sl_vec3', 'vec3'),
        ('sl_vec4', 'vec4'),
        ('sl_ivec2', 'ivec2'),
        ('sl_ivec3', 'ivec3'),
        ('sl_ivec4', 'ivec4'),
        ('sl_uvec2', 'uvec2'),
        ('sl_uvec3', 'uvec3'),
        ('sl_uvec4', 'uvec4'),
        ('sl_dvec2', 'dvec2'),
        ('sl_dvec3', 'dvec3'),
        ('sl_dvec4', 'dvec4'),
        ('sl_bvec2', 'bvec2'),
        ('sl_bvec3', 'bvec3'),
        ('sl_bvec4', 'bvec4'),
        ('sl_mat2', 'mat2'),
        ('sl_mat3', 'mat3'),
        ('sl_mat4', 'mat4'),
        ('sl_dmat2', 'dmat2'),
        ('sl_dmat3', 'dmat3'),
        ('sl_dmat4', 'dmat4'),
    ]


def write_type_defines(f: TextIOWrapper):
    # simple first
    f.write('using sl_uint32 = uint32;' + '\n')
    f.write('using sl_int32  = int32;' + '\n')
    f.write('using sl_float  = float;' + '\n')
    f.write('using sl_double = double;' + '\n')
    f.write('using sl_vec2   = vec2;' + '\n')
    f.write('using sl_vec3   = vec3;' + '\n')
    f.write('using sl_vec4   = vec4;' + '\n')
    f.write('using sl_ivec2  = ivec2;' + '\n')
    f.write('using sl_ivec3  = ivec3;' + '\n')
    f.write('using sl_ivec4  = ivec4;' + '\n')
    f.write('using sl_uvec2  = uvec2;' + '\n')
    f.write('using sl_uvec3  = uvec3;' + '\n')
    f.write('using sl_uvec4  = uvec4;' + '\n')
    f.write('using sl_dvec2  = dvec2;' + '\n')
    f.write('using sl_dvec3  = dvec3;' + '\n')
    f.write('using sl_dvec4  = dvec4;' + '\n')
    f.write('using sl_mat2   = mat2;' + '\n')
    f.write('using sl_mat4   = mat4;' + '\n')
    f.write('using sl_dmat2  = dmat2;' + '\n')
    f.write('using sl_dmat4  = dmat4;' + '\n')
    f.write('\n')
    # sl bool has to be 4 bytes in size
    f.write('struct sl_bool' + '\n')
    f.write('{' + '\n')
    f.write('    sl_bool(const bool& b)' + '\n')
    f.write('    {' + '\n')
    f.write('        pad = 0;' + '\n')
    f.write('        v   = b;' + '\n')
    f.write('    }' + '\n')
    f.write('    sl_bool()' + '\n')
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
    f.write('using sl_bvec2 = Eigen::Vector2<sl_bool>;' + '\n')
    f.write('using sl_bvec3 = Eigen::Vector3<sl_bool>;' + '\n')
    f.write('using sl_bvec4 = Eigen::Vector4<sl_bool>;' + '\n')
    f.write('\n')
    f.write('template <typename T>' + '\n')
    f.write('struct sl_matrix3' + '\n')
    f.write('{' + '\n')
    f.write('    sl_matrix3(const Eigen::Matrix3<T>& mat)' + '\n')
    f.write('        : m(Eigen::Matrix<T, 4, 3>::Zero())' + '\n')
    f.write('    {' + '\n')
    f.write('        m.template block<3, 3>(0, 0) = mat;' + '\n')
    f.write('    }' + '\n')
    f.write('    sl_matrix3()' + '\n')
    f.write('        : m(Eigen::Matrix<T, 4, 3>::Zero())' + '\n')
    f.write('    {' + '\n')
    f.write('    }' + '\n')
    f.write('    void operator=(const Eigen::Matrix3<T>& o)' + '\n')
    f.write('    {' + '\n')
    f.write('        m.template block<3, 3>(0, 0) = o;' + '\n')
    f.write('    }' + '\n')
    f.write('    operator Eigen::Matrix3<T>&()' + '\n')
    f.write('    {' + '\n')
    f.write('        return m.template block<3, 3>(0, 0);' + '\n')
    f.write('    }' + '\n')
    f.write('\n')
    f.write('  private:' + '\n')
    f.write('    Eigen::Matrix<T, 4, 3> m;' + '\n')
    f.write('};' + '\n')
    f.write('\n')
    f.write('using sl_mat3  = sl_matrix3<float>;' + '\n')
    f.write('using sl_dmat3 = sl_matrix3<double>;' + '\n')
# TODO: Dynamic sized arrays in general (might do later, since we do not use them atm)
    # base for sl_arrays constrained to the right types
    primitive_sl_type_data = get_primitive_sl_type_data()

    layouts = ["std140", "std430"]

    # std140
    for tp in primitive_sl_type_data:
        f.write('template <ptr_size N>' + '\n')
        f.write('struct ' + tp[0] + '_array_' + layouts[0] + '\n')
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
        f.write('    ' + tp[0] + '_array_' +
                layouts[0] + '() = default;' + '\n')
        f.write('    ~' + tp[0] + '_array_' +
                layouts[0] + '() = default;' + '\n')
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
        comp_size, comp_count, rows, cols = component_size_count_row_col(tp[1])
        align_of_e, size_of_e = align_and_size_of_(comp_count, comp_size, rows, cols, 0, layouts[0])
        array_align = round_up(16, align_of_e)
        stride = round_up(array_align, size_of_e)
        rest = stride - size_of_e
        assert(rest % 4 == 0)
        i = 0
        while rest > 0:
            f.write('        sl_uint32 pad' + str(i) + ' = 0;' + '\n')
            i += 1
            rest -= 4
        f.write('    };' + '\n')
        f.write('\n')
        f.write('    internal_element data[N];' + '\n')
        f.write('};' + '\n')
        f.write('\n')

    # std430
    for tp in primitive_sl_type_data:
        f.write('template <ptr_size N>' + '\n')
        f.write('struct ' + tp[0] + '_array_' + layouts[1] + '\n')
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
        f.write('    ' + tp[0] + '_array_' +
                layouts[1] + '() = default;' + '\n')
        f.write('    ~' + tp[0] + '_array_' +
                layouts[1] + '() = default;' + '\n')
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
        comp_size, comp_count, rows, cols = component_size_count_row_col(tp[1])
        align_of_e, size_of_e = align_and_size_of_(comp_count, comp_size, rows, cols, 0, layouts[1])
        stride = round_up(align_of_e, size_of_e)
        rest = stride - size_of_e
        assert(rest % 4 == 0)
        i = 0
        while rest > 0:
            f.write('        sl_uint32 pad' + str(i) + ' = 0;' + '\n')
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
             './res/shader/include/ibl_gen.glsl',
             './res/shader/include/cubemap.glsl',
             './res/shader/include/fxaa_data.glsl',
             './res/shader/include/luminance.glsl',
             './res/shader/include/hi_z.glsl',
             ]
    buffers = parse_shader_buffers(files)

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
        f.write('//! \cond NO_COND')
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
                    f.write('    sl_uint32 pad' +
                            str(padding_counter) + ';' + '\n')
                    padding_counter += 1
                    current_end += 4

                if element.array_size == 0:
                    f.write('    ' + cpp_type(element.type) + ' ' +
                            element.name + ';' + '\n')
                elif element.array_size > 0:
                    f.write('    ' + cpp_array_type(element.type, element.array_size,
                            buffer.layout) + ' ' + element.name + ';' + '\n')
                else:
                    raise NotImplementedError
                current_end += element.byte_size

            f.write('};' + '\n')

        f.write('//! \encond')
        f.write('\n')
        f.write('\n')
        f.write('} // namespace mango' + '\n')
        f.write('\n')
        f.write('#endif // MANGO_SHADER_INTEROP_HPP' + '\n')


if __name__ == '__main__':
    generate_interop()
