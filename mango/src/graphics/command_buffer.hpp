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
    //! \brief Package type definition.
    using package = void*;
    //! \brief Type definition for a command execute function.
    typedef void (*execute_function)(const void*);

    //! \cond NO_COND
#define BEGIN_COMMAND(name) \
    struct name##_command   \
    {                       \
        static const execute_function execute
#define END_COMMAND(name) \
    }                     \
    ;                     \
    static_assert(std::is_pod<name##_command>::value == true, #name "must be a POD.")
    //! \endcond

    // commands

    //! \brief Command setting the viewport.
    BEGIN_COMMAND(set_viewport);
    int32 x;      //!< Viewport x.
    int32 y;      //!< Viewport y.
    int32 width;  //!< Viewport width.
    int32 height; //!< Viewport height.
    //! \cond NO_COND
    END_COMMAND(set_viewport);
    //! \endcond

    //! \brief Command enabling or disabling depth testing.
    BEGIN_COMMAND(set_depth_test);
    bool enabled; //!< Enable/Disable depth testing.
    //! \cond NO_COND
    END_COMMAND(set_depth_test);
    //! \endcond

    //! \brief Command setting a depth comparison function.
    BEGIN_COMMAND(set_depth_func);
    compare_operation operation; //!< Depth testing compare operation.
    //! \cond NO_COND
    END_COMMAND(set_depth_func);
    //! \endcond

    //! \brief Command enabling or disabling depth writes.
    BEGIN_COMMAND(set_depth_write);
    bool enabled; //!< Enable/Disable depth write.
    //! \cond NO_COND
    END_COMMAND(set_depth_write);
    //! \endcond

    //! \brief Command settinng the polygon mode.
    BEGIN_COMMAND(set_polygon_mode);
    polygon_face face; //!< Polygon face.
    polygon_mode mode; //!< Polygon mode.
    //! \cond NO_COND
    END_COMMAND(set_polygon_mode);
    //! \endcond

    //! \brief Command binding a vertex array.
    BEGIN_COMMAND(bind_vertex_array);
    g_uint vertex_array_name; //!< Gl name of the vertex array.
    //! \cond NO_COND
    END_COMMAND(bind_vertex_array);
    //! \endcond

    //! \brief Command binding a shader programm.
    BEGIN_COMMAND(bind_shader_program);
    g_uint shader_program_name; //!< Gl name of the shader program.
    //! \cond NO_COND
    END_COMMAND(bind_shader_program);
    //! \endcond

    //! \brief Command binding a single uniform.
    BEGIN_COMMAND(bind_single_uniform);
    int32 location;            //!< Uniform location.
    shader_resource_type type; //!< Uniform shader type.
    void* uniform_value;       //!< Uniform value.
    int32 count;               //!< Number of uniform 'slost' to abuse. Used e.g. for arrays.
    //! \cond NO_COND
    END_COMMAND(bind_single_uniform);
    //! \endcond

    //! \brief Command binding a (part of a) buffer.
    BEGIN_COMMAND(bind_buffer);
    int32 index;          //!< Buffer index.
    g_uint buffer_name;   //!< Gl name of the buffer.
    buffer_target target; //!< Target of the buffer.
    int64 offset;         //!< Offset to start the binding from.
    int64 size;           //!< Size to bind.
    //! \cond NO_COND
    END_COMMAND(bind_buffer);
    //! \endcond

    //! \brief Command binding a texture.
    BEGIN_COMMAND(bind_texture);
    int32 binding;          //!< Texture binding point.
    g_uint texture_name;    //!< Gl name of the texture.
    int32 sampler_location; //!< Sampler location.
    //! \cond NO_COND
    END_COMMAND(bind_texture);
    //! \endcond

    //! \brief Command binding a (part of a) texture as image.
    BEGIN_COMMAND(bind_image_texture);
    int32 binding;         //!< Image binding point.
    g_uint texture_name;   //!< Gl name of the texture.
    int32 level;           //!< Texture level to bind as image.
    bool layered;          //!< True if layered, else False.
    int32 layer;           //!< Number of the layer.
    base_access access;    //!< Access specification.
    format element_format; //!< Texture element format.
    //! \cond NO_COND
    END_COMMAND(bind_image_texture);
    //! \endcond

    //! \brief Command binding a framebuffer.
    BEGIN_COMMAND(bind_framebuffer);
    g_uint framebuffer_name; //!< Gl name of the framebuffer.
    //! \cond NO_COND
    END_COMMAND(bind_framebuffer);
    //! \endcond

    //! \brief Command adding a memory barrier.
    BEGIN_COMMAND(add_memory_barrier);
    memory_barrier_bit barrier_bit; //!< Barrier bit.
    //! \cond NO_COND
    END_COMMAND(add_memory_barrier);
    //! \endcond

    //! \brief Command placing a fence sync point.
    BEGIN_COMMAND(fence_sync);
    g_sync* sync; //!< Pointer to the sync. Do not copy data.
    //! \cond NO_COND
    END_COMMAND(fence_sync);
    //! \endcond

    //! \brief Command sheduling a waiting operation on the cpu side.
    BEGIN_COMMAND(client_wait_sync);
    g_sync* sync; //!< Pointer to the sync. Do not copy data.
    //! \cond NO_COND
    END_COMMAND(client_wait_sync);
    //! \endcond

    //! \brief Command to be called at the end of a frame.
    BEGIN_COMMAND(end_frame);
    //! \cond NO_COND
    END_COMMAND(end_frame);
    //! \endcond

    //! \brief Command calculating a mipchain for a given texture.
    BEGIN_COMMAND(calculate_mipmaps);
    g_uint texture_name; //!< Gl name of the texture.
    //! \cond NO_COND
    END_COMMAND(calculate_mipmaps);
    //! \endcond

    //! \brief Command clearing a framebuffer.
    BEGIN_COMMAND(clear_framebuffer);
    g_uint framebuffer_name;            //!< Gl name of the framebuffer.
    clear_buffer_mask buffer_mask;      //!< Clear buffer mask.
    attachment_mask fb_attachment_mask; //!< Clear attachment mask.
    g_float r;                          //!< Clear value for red component.
    g_float g;                          //!< Clear value for green component.
    g_float b;                          //!< Clear value for blue component.
    g_float a;                          //!< Clear value for alpha component.
    g_float depth;                      //!< Clear value for depth component.
    g_int stencil;                      //!< Clear value for stencil component.
    //! \cond NO_COND
    END_COMMAND(clear_framebuffer);
    //! \endcond

    //! \brief Command draw arrays (instanced).
    BEGIN_COMMAND(draw_arrays);
    primitive_topology topology; //!< Topology type.
    int32 first;                 //!< Offset.
    int32 count;                 //!< Number of vertices.
    int32 instance_count;        //!< Number of instances.
    //! \cond NO_COND
    END_COMMAND(draw_arrays);
    //! \endcond

    //! \brief Command draw elements (instanced).
    BEGIN_COMMAND(draw_elements);
    primitive_topology topology; //!< Topology type.
    int32 first;                 //!< Offset.
    int32 count;                 //!< Number of indices.
    index_type type;             //!< Index type.
    int32 instance_count;        //!< Number of instances.
    //! \cond NO_COND
    END_COMMAND(draw_elements);
    //! \endcond

    //! \brief Command dispatching a compute shader.
    BEGIN_COMMAND(dispatch_compute);
    int32 num_x_groups; //!< Number of dispatch groups in x direction.
    int32 num_y_groups; //!< Number of dispatch groups in y direction.
    int32 num_z_groups; //!< Number of dispatch groups in z direction.
    //! \cond NO_COND
    END_COMMAND(dispatch_compute);
    //! \endcond

    //! \brief Command enabling or disabling face culling.
    BEGIN_COMMAND(set_face_culling);
    bool enabled; //!< Enable/Disable face culling.
    //! \cond NO_COND
    END_COMMAND(set_face_culling);
    //! \endcond

    //! \brief Command setting the face to cull.
    BEGIN_COMMAND(set_cull_face);
    polygon_face face; //!< Face to cull.
    //! \cond NO_COND
    END_COMMAND(set_cull_face);
    //! \endcond

    //! \brief Command enabling or disabling blending.
    BEGIN_COMMAND(set_blending);
    bool enabled; //!< Enable/Disable blending.
    //! \cond NO_COND
    END_COMMAND(set_blending);
    //! \endcond

    //! \brief Command setting blend factors.
    BEGIN_COMMAND(set_blend_factors);
    blend_factor source;      //!< Source blend factor.
    blend_factor destination; //!< Destination blend factor.
    //! \cond NO_COND
    END_COMMAND(set_blend_factors);
    //! \endcond

    //! \brief Command setting the polygon offset.
    BEGIN_COMMAND(set_polygon_offset);
    float factor; //!< Polygon offset factor.
    float units;  //!< Polygon offset units.
    //! \cond NO_COND
    END_COMMAND(set_polygon_offset);
    //! \endcond

    // ---
    // keys

    //! \brief Minimum sized key type definition.
    using min_key = uint8;
    //! \brief Small key type definition.
    using small_key = uint16;
    //! \brief Mdeium sized key type definition.
    using medium_key = uint32;
    //! \brief Maximum sized key type definition.
    using max_key = uint64;
    namespace command_keys
    {
        //! \brief Templates for keys
        enum class key_template
        {
            no_sort,                        //!< No sorting.
            max_key_material_front_to_back, //!< Sorting by material and from front to back afterwards.
            max_key_back_to_front,          //!< Sorting by from back to front.
        };

        //! \brief Creates a key of a specific template.
        //! \param[in] k_template The \a key_template to use.
        //! \return The created key.
        template <typename T>
        T create_key(key_template k_template)
        {
            switch (k_template)
            {
            case key_template::max_key_material_front_to_back:
                return 4611686018427387904ull;
            case key_template::max_key_back_to_front:
                return 4611686018427387904ull;
            case key_template::no_sort:
                return 0;
            }
            return 0;
        }

        //! \brief Constant for a non sorting key.
        const min_key no_sort = 0;

        // no_sort
        // 0

        // max_key_material_front_to_back
        // 0 - 1 { To Front, Standard, To Back}
        // 32 - 39 MaterialID - 8-Bit
        // 40 - 63 Depth - 24 Bit

        // max_key_back_to_front
        // 0 - 1 { To Front, Standard, To Back}
        // 32 - 55 (1.0 - Depth) - 24 Bit
        // 56 - 63 MaterialID - 8-Bit

        //! \brief Base mode for keys that get sorted.
        enum class base_mode : uint8
        {
            to_front = 0, //!< Sort to front of the commands.
            standard = 1, // //!< Default base mode. Sorting order determined by other values.
            to_back  = 2, //!< Sort to back of the commands.
        };

        //! \brief Adds a sorting base mode to \a max_key.
        //! \param[in,out] key The \a max_key to add the \a base_mode to.
        //! \param[in] bm The \a base_mode.
        inline void add_base_mode(max_key& key, base_mode bm)
        {
            uint64 key_mask  = ~(((1ull << 2) - 1) << 62);
            uint64 mode_mask = ((1ull << 2) - 1);
            uint64 new_bm    = uint64(bm) & mode_mask;
            key              = (key & key_mask) | (new_bm << 62);
        }

        //! \brief Adds depth to \a max_key.
        //! \param[in,out] key The \a max_key to add the depth to.
        //! \param[in] depth The depth between 0 and 1. Can also be '1 - depth' to sort from back to front.
        //! \param[in] k_template The key template. Needs to be specified so the position in the key is known.
        inline void add_depth(max_key& key, float depth, key_template k_template)
        {
            int64 pos = 0;
            switch (k_template)
            {
            case key_template::max_key_material_front_to_back:
                pos = 0;
                break;
            case key_template::max_key_back_to_front:
                pos = 8;
                break;
            default:
                MANGO_LOG_WARN("Unknown key temnplate. Can not add depth.");
                return;
            }

            uint64 key_mask  = ~(((1ull << 24) - 1) << pos);
            uint64 mode_mask = ((1ull << 24) - 1);
            uint64 m_id      = uint64(depth * 8388607.0f) & mode_mask; // TODO Paul: Depth should be converted correctly!
            key              = (key & key_mask) | (m_id << pos);
        }

        //! \brief Adds a material id to \a max_key.
        //! \param[in,out] key The \a max_key to add the material id to.
        //! \param[in] material_id The material id. (8 bit integer).
        inline void add_material(max_key& key, int8 material_id)
        {
            uint64 key_mask  = ~(((1ull << 8) - 1) << 24);
            uint64 mode_mask = ((1ull << 8) - 1);
            uint64 m_id      = uint64(material_id) & mode_mask;
            key              = (key & key_mask) | (m_id << 24);
        }
    } // namespace command_keys

    // ---

    //! \brief A buffer holding commands, their memory and having the possibility to execute them,
    template <typename K>
    class command_buffer
    {
      public:
        //! \brief Type definition key.
        using key = K;

        ~command_buffer()
        {
            delete[] m_keys;
            delete[] m_packages;
            delete[] m_indices;
            m_allocator.reset();
        }

        //! \brief Creates a \a command_buffer with a specific key.
        //! \param[in] size The size of the \a command_buffer in bytes.
        //! \return A shared_ptr to th created \a command_buffer.
        static command_buffer_ptr<K> create(int64 size)
        {
            return std::make_shared<command_buffer<K>>(size);
        }

        //! \brief Creates a command.
        //! \param[in] k The commands key.
        //! \param[in] spare_memory_size Size of additional memory the command needs (pointer etc.).
        //! \return A pointer to the created command.
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

        //! \brief Creates a command and appends it to another one.
        //! \details This is used to chain commands that need to be in a certain order. In some cases they would be sorted wrong otherwise.
        //! \param[in] command A pointer to the command to append the created one to.
        //! \param[in] spare_memory_size Size of additional memory the command needs (pointer etc.).
        //! \return A pointer to the created command.
        template <typename T, typename U>
        T* append(U* command, int64 spare_memory_size = 0)
        {
            package p = package_create<T>(spare_memory_size);

            write_next(command, p);

            write_next(nullptr);
            write_execute_function(T::execute);

            return read_command<T>();
        }

        //! \brief Reads the spare memory of the current command and returns a pointer to it.
        //! \return A pointer to the spare memory of the current command.
        template <typename T>
        void* map_spare()
        {
            return read_spare<T>();
        }

        //! \brief Sorting class for the stable sort to sort indices by key list.
        class sort_indices
        {
          private:
            //! \brief Key array used for sorting.
            key* sorting_array;

          public:
            //! \brief Constructor.
            sort_indices(key* arr)
                : sorting_array(arr)
            {
            }
            //! \brief Comparison operator. Sorts ascending.
            bool operator()(key i, key j) const
            {
                return sorting_array[i] < sorting_array[j];
            }
        };

        //! \brief Sorts the \a command_buffer by key.
        void sort()
        {
            std::stable_sort(&m_indices[0], m_indices + m_idx, sort_indices(m_keys));
        }

        //! \brief Executes the \a command_buffer.
        //! \details Executes in order when sort was called before.
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

        //! \brief Invalidates the \a command_buffer.
        //! \details If the buffer is not invalidated it could be executed more then once. Useful if updating commands every frame is not necessary.
        void invalidate()
        {
            clear();
            m_dirty = true;
        }

        //! \brief Checks if \a command_buffer is dirty (= was invalidated and since then not executed again).
        //! \return True if \a command´_buffer is dirty, else False.
        bool dirty()
        {
            return m_dirty;
        }

        //! \brief Contructs the \a command_buffer.
        //! \details Normally not called directly,  use create() instead.
        //! \param[in] size The size of the \a command_buffer in bytes.
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
        //! \brief Clears the \a command_buffer.
        //! \details Resets the linear allocator and the command index.
        void clear()
        {
            m_idx = 0;
            m_allocator.reset();
        }

        //! \brief Allocator used to store the commands.
        linear_allocator m_allocator;
        //! \brief List of keys.
        key* m_keys;
        //! \brief List of command packages.
        package* m_packages;
        //! \brief List of indices.
        int64* m_indices;
        //! \brief CUrrent command index.
        int64 m_idx;
        //! \brief Dirty flag.
        bool m_dirty;

        //! \brief Current package.
        //! \details Recently added or next executed, dependent on state, used to make the api cleaner.
        package m_current;

        //! \brief Offset of pointer to the next package in a package.
        static const int64 next_offset = 0;
        //! \brief Offset of the command execute function pointer in a package.
        static const int64 execute_function_offset = next_offset + sizeof(package);
        //! \brief Offset of the command in a package.
        static const int64 command_offset = execute_function_offset + sizeof(execute_function);

        //! \brief Creates a package with a command.
        //! \param[in] spare_memory Size of additional memory the command needs (pointer etc.).
        //! \return The new package.
        template <typename T>
        package package_create(uint64 spare_memory)
        {
            void* mem = m_allocator.allocate(calculate_size<T>(spare_memory));
            MANGO_ASSERT(mem, "Command Buffer is too small!");
            m_current = mem;
            return m_current;
        }

        //! \brief Calculates the size needed for the command package.
        //! \param[in] spare_memory Size of additional memory the command needs (pointer etc.).
        //! \return The size to allocate for the package.
        template <typename T>
        int64 calculate_size(uint64 spare_memory)
        {
            return command_offset + sizeof(T) + spare_memory;
        }

        //! \brief Reads the command from a package.
        //! \return Pointer to the command.
        template <typename T>
        T* read_command()
        {
            return reinterpret_cast<T*>(static_cast<int8*>(m_current) + command_offset);
        }

        //! \brief Writes the next package pointer for a command.
        //! \param[in] command Pointer to the command.
        //! \param[in] next The package to set as next one.
        template <typename T>
        void write_next(T* command, package next)
        {
            *read_next(command) = next;
        }

        //! \brief Reads the next package from a package.
        //! \param[in] command The command to retrieve the next package from.
        //! \return Pointer to the next package.
        template <typename T>
        package* read_next(T* command)
        {
            return reinterpret_cast<package*>(reinterpret_cast<int8*>(command) - command_offset + next_offset);
        }

        //! \brief Reads the next package from current package.
        //! \return Pointer to the next package.
        package* read_next()
        {
            return reinterpret_cast<package*>(static_cast<int8*>(m_current) + next_offset);
        }

        //! \brief Reads the execute function from the current package.
        //! \return Pointer to the execute function.
        execute_function* read_execute_function()
        {
            return reinterpret_cast<execute_function*>(static_cast<int8*>(m_current) + execute_function_offset);
        }

        //! \brief Reads the spare memory from the current package.
        //! \return Pointer to the spare memory.
        template <typename T>
        void* read_spare()
        {
            return static_cast<int8*>(m_current) + command_offset + sizeof(T);
        }

        //! \brief Writes the next package pointer for current command/package.
        //! \param[in] next The package to set as next one.
        void write_next(package next)
        {
            *read_next() = next;
        }

        //! \brief Writes the next package pointer for current command/package.
        //! \param[in] function The execute function to set.
        void write_execute_function(execute_function function)
        {
            *read_execute_function() = function;
        }

        // load

        //! \brief Loads the next package of the current command/package.
        //! \return The next package.
        package load_next()
        {
            m_current = *read_next();
            return m_current;
        }

        //! \brief Loads the execute function of the current command/package.
        //! \return The execute function.
        execute_function load_execute_function()
        {
            return *read_execute_function();
        }

        //! \brief Loads the command of the current package.
        //! \return The command.
        void* load_command()
        {
            return (static_cast<int8*>(m_current) + command_offset);
        }
    };
} // namespace mango

#endif // MANGO_COMMAND_BUFFER_HPP
