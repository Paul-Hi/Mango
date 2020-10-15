//! \file      command_buffer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_COMMAND_BUFFER_HPP
#define MANGO_COMMAND_BUFFER_HPP

#include <graphics/graphics_common.hpp>
#include <graphics/graphics_state.hpp>
#include <mango/assert.hpp>
#include <memory/linear_allocator.hpp>

namespace mango
{
    using package = void*;
    typedef void (*execute_function)(const void*);

#define BEGIN_COMMAND(name) \
    struct name##_command   \
    {                       \
        static const execute_function execute
#define END_COMMAND(name) \
    }                     \
    ;                     \
    static_assert(std::is_pod<name##_command>::value == true, #name "must be a POD.")

    // commands

    BEGIN_COMMAND(set_viewport);
    int32 x;
    int32 y;
    int32 width;
    int32 height;
    END_COMMAND(set_viewport);

    BEGIN_COMMAND(set_depth_test);
    bool enabled;
    END_COMMAND(set_depth_test);

    BEGIN_COMMAND(set_depth_func);
    compare_operation compare_operation;
    END_COMMAND(set_depth_func);

    BEGIN_COMMAND(set_polygon_mode);
    polygon_face face;
    polygon_mode mode;
    END_COMMAND(set_polygon_mode);

    BEGIN_COMMAND(bind_vertex_array);
    g_uint vertex_array_name;
    END_COMMAND(bind_vertex_array);

    BEGIN_COMMAND(bind_shader_program);
    g_uint shader_program_name;
    END_COMMAND(bind_shader_program);

    BEGIN_COMMAND(bind_single_uniform);
    int32 location;
    shader_resource_type type;
    void* uniform_value;
    int32 count;
    END_COMMAND(bind_single_uniform);

    BEGIN_COMMAND(bind_buffer);
    int32 index;
    g_uint buffer_name;
    buffer_target target;
    int64 offset;
    int64 size;
    END_COMMAND(bind_buffer);

    BEGIN_COMMAND(bind_texture);
    int32 binding;
    g_uint texture_name;
    int32 sampler_location;
    END_COMMAND(bind_texture);

    BEGIN_COMMAND(bind_image_texture);
    int32 binding;
    g_uint texture_name;
    int32 level;
    bool layered;
    int32 layer;
    base_access access;
    format element_format;
    END_COMMAND(bind_image_texture);

    BEGIN_COMMAND(bind_framebuffer);
    g_uint framebuffer_name;
    END_COMMAND(bind_framebuffer);

    BEGIN_COMMAND(add_memory_barrier);
    memory_barrier_bit barrier_bit;
    END_COMMAND(add_memory_barrier);

    // do not copy data to cmd->sync
    BEGIN_COMMAND(fence_sync);
    g_sync* sync;
    END_COMMAND(fence_sync);

    BEGIN_COMMAND(client_wait_sync);
    g_sync* sync;
    END_COMMAND(client_wait_sync);

    BEGIN_COMMAND(finish_gpu);
    END_COMMAND(finish_gpu);

    BEGIN_COMMAND(calculate_mipmaps);
    g_uint texture_name;
    END_COMMAND(calculate_mipmaps);

    BEGIN_COMMAND(clear_framebuffer);
    g_uint framebuffer_name;
    clear_buffer_mask buffer_mask;
    attachment_mask fb_attachment_mask;
    g_float r;
    g_float g;
    g_float b;
    g_float a;
    g_float depth;
    g_int stencil;
    END_COMMAND(clear_framebuffer);

    BEGIN_COMMAND(draw_arrays);
    primitive_topology topology;
    int32 first;
    int32 count;
    int32 instance_count;
    END_COMMAND(draw_arrays);

    BEGIN_COMMAND(draw_elements);
    primitive_topology topology;
    int32 first;
    int32 count;
    index_type type;
    int32 instance_count;
    END_COMMAND(draw_elements);

    BEGIN_COMMAND(dispatch_compute);
    int32 num_x_groups;
    int32 num_y_groups;
    int32 num_z_groups;
    END_COMMAND(dispatch_compute);

    BEGIN_COMMAND(set_face_culling);
    bool enabled;
    END_COMMAND(set_face_culling);

    BEGIN_COMMAND(set_cull_face);
    polygon_face face;
    END_COMMAND(set_cull_face);

    BEGIN_COMMAND(set_blending);
    bool enabled;
    END_COMMAND(set_blending);

    BEGIN_COMMAND(set_blend_factors);
    blend_factor source;
    blend_factor destination;
    END_COMMAND(set_blend_factors);

    BEGIN_COMMAND(set_polygon_offset);
    float factor;
    float units;
    END_COMMAND(set_polygon_offset);

    // ---
    // keys

    using min_key    = int8;
    using small_key  = int16;
    using medium_key = int32;
    using max_key    = int64;
    namespace command_keys
    {
        const min_key min_key_no_sort                = -1;
        const max_key max_key_to_start               = 0;
        const max_key max_key_front_to_back          = 1;
        const max_key max_key_back_to_front          = 1;
        const max_key max_key_material_front_to_back = 1;
    } // namespace command_keys

    // ---

    template <typename K>
    class command_buffer
    {
      public:
        using key = K;

        ~command_buffer()
        {
            delete[] m_keys;
            delete[] m_packages;
            delete[] m_indices;
            m_allocator.reset();
        }

        static command_buffer_ptr<K> create(int64 size)
        {
            return std::make_shared<command_buffer<K>>(size);
        }

        template <typename T>
        T* create(key k, int64 spare_memory_size = 0)
        {
            package p = package_create<T>(spare_memory_size);

            m_keys[m_idx]     = k;
            m_packages[m_idx] = p;
            m_indices[m_idx]  = m_idx;
            m_idx++;

            write_next(nullptr);
            write_execute_function(T::execute);

            return read_command<T>();
        }

        template <typename T, typename U>
        T* append(U* command, int64 spare_memory_size = 0)
        {
            package p = package_create<T>(spare_memory_size);

            write_next(command, p);

            write_next(nullptr);
            write_execute_function(T::execute);

            return read_command<T>();
        }

        template <typename T>
        void* map_spare()
        {
            return read_spare<T>();
        }

        class sort_indices
        {
          private:
            key* sorting_array;

          public:
            sort_indices(key* arr)
                : sorting_array(arr)
            {
            }
            bool operator()(key i, key j) const
            {
                return sorting_array[i] < sorting_array[j];
            }
        };

        void sort()
        {
            std::stable_sort(&m_indices[0], m_indices + m_idx, sort_indices(m_keys));
        }

        void execute()
        {
            for (int64 i = 0; i < m_idx; ++i)
            {
                m_current = m_packages[m_indices[i]];
                do
                {
                    const void* cmd = load_command();
                    (*load_execute_function())(cmd);

                    m_current = load_next();
                } while (m_current != nullptr);
            }
            m_dirty = false;
        }

        void invalidate()
        {
            clear();
            m_dirty = true;
        }

        bool dirty()
        {
            return m_dirty;
        }

        command_buffer(int64 size)
            : m_allocator(size)
            , m_idx(0)
            , m_dirty(true)
        {
            m_keys     = new key[size / 4];
            m_packages = new package[size / 4];
            m_indices  = new int64[size / 4];
            m_allocator.init();
        }

      private:
        void clear()
        {
            m_idx = 0;
            m_allocator.reset();
        }

        linear_allocator m_allocator;
        key* m_keys;
        package* m_packages;
        int64* m_indices;
        int64 m_idx;
        bool m_dirty;

        package m_current;

        static const int64 next_offset             = 0;
        static const int64 execute_function_offset = next_offset + sizeof(package);
        static const int64 command_offset          = execute_function_offset + sizeof(execute_function);

        template <typename T>
        package package_create(uint64 spare_memory)
        {
            void* mem = m_allocator.allocate(calculate_size<T>(spare_memory), 0);
            MANGO_ASSERT(mem, "Command Buffer is too small!");
            m_current = mem;
            return m_current;
        }

        template <typename T>
        int64 calculate_size(uint64 spare_memory)
        {
            return command_offset + sizeof(T) + spare_memory;
        }

        template <typename T>
        T* read_command()
        {
            return reinterpret_cast<T*>(static_cast<int8*>(m_current) + command_offset);
        }

        template <typename T>
        void write_next(T* command, package next)
        {
            *read_next(command) = next;
        }

        template <typename T>
        package* read_next(T* command)
        {
            return reinterpret_cast<package*>(reinterpret_cast<int8*>(command) - command_offset + next_offset);
        }

        package* read_next()
        {
            return reinterpret_cast<package*>(static_cast<int8*>(m_current) + next_offset);
        }

        execute_function* read_execute_function()
        {
            return reinterpret_cast<execute_function*>(static_cast<int8*>(m_current) + execute_function_offset);
        }

        template <typename T>
        void* read_spare()
        {
            return static_cast<int8*>(m_current) + command_offset + sizeof(T);
        }

        void write_next(package next)
        {
            *read_next() = next;
        }

        void write_execute_function(execute_function function)
        {
            *read_execute_function() = function;
        }

        // load

        const package load_next()
        {
            m_current = *read_next();
            return m_current;
        }

        const execute_function load_execute_function()
        {
            return *read_execute_function();
        }

        const void* load_command()
        {
            return (static_cast<int8*>(m_current) + command_offset);
        }
    };
} // namespace mango

#endif // MANGO_COMMAND_BUFFER_HPP
