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

namespace mango
{
    //! \brief The base for each command in the \a command_buffer.
    class command
    {
      public:
        //! \brief The pointer to the next element. Used for creating a linked list.
        unique_ptr<command> m_next = nullptr;

        virtual ~command() = default;

        //! \brief Executes the command.
        //! \param[in] state The current state of the graphics pipeline. Used to fill missing information.
        virtual void execute(graphics_state& state) = 0;
    };

    //! \brief Builds, holds and executes a linked list of commands.
    //! \details The \a command_buffer creates \a commands and adds them to an internal linked list, which can be executed.
    class command_buffer
    {
      public:
        //! \brief Creates a new \a command_buffer and returns a pointer to it.
        //! \return A pointer to the newly created \a command_buffer.
        static command_buffer_ptr create()
        {
            return std::make_shared<command_buffer>();
        };

        command_buffer();
        ~command_buffer();

        void clear();

        //! \brief Executes all commands in the \a command_buffer since the lase call to execute().
        //! \details Does traverse the internal linked list and clears it while doing this.
        void execute();

        //! \brief Sets the viewport size.
        //! \param[in] x The x position of the viewport. Has to be a positive value.
        //! \param[in] y The y position of the viewport. Has to be a positive value.
        //! \param[in] width The width of the viewport. Has to be a positive value.
        //! \param[in] height The height of the viewport. Has to be a positive value.
        void set_viewport(int32 x, int32 y, int32 width, int32 height);

        //! \brief Clears attachments of a framebuffer.
        //! \param[in] buffer_mask The mask describes which buffers should be cleared.
        //! \param[in] att_mask The mask describes which attachments should be cleared.
        //! \param[in] r The red component to clear color attachments.
        //! \param[in] g The green component to clear color attachments.
        //! \param[in] b The blue component to clear color attachments.
        //! \param[in] a The alpha component to clear color attachments.
        //! \param[in] framebuffer The pointer to the \a framebuffer to clear. To clear the default framebuffer leave empty or pass nullptr.
        void clear_framebuffer(clear_buffer_mask buffer_mask, attachment_mask att_mask, float r, float g, float b, float a, framebuffer_ptr framebuffer = nullptr);

        //! \brief Enables or disables the depth test.
        //! \param[in] enabled True if the depth test should be enabled, else false.
        void set_depth_test(bool enabled);

        //! \brief Sets the \a compare_operation for depth testing.
        //! \param[in] op The \a compare_operation to use for depth testing.
        void set_depth_func(compare_operation op);

        //! \brief Sets the \a polygon_mode as well as the \a polygon_faces used for drawing.
        //! \param[in] face The \a polygon_faces to draw.
        //! \param[in] mode The \a polygon_mode used for drawing.
        void set_polygon_mode(polygon_face face, polygon_mode mode);

        //! \brief Binds a \a vertex_array for drawing.
        //! \param[in] vertex_array A pointer to the \a vertex_array to bind.
        void bind_vertex_array(vertex_array_ptr vertex_array);

        //! \brief Binds a \a shader_program for drawing.
        //! \param[in] shader_program A pointer to the \a shader_program to bind.
        void bind_shader_program(shader_program_ptr shader_program);

        //! \brief Binds a single \a uniform not included in uniform buffers for drawing.
        //! \details The void* \a uniform_data should contain the value for the uniform not included in any uniform buffer object.
        //! \param[in] location The uniform location to bind the value to. Has to be a positive value.
        //! \param[in] uniform_value Pointer to a value.
        //! \param[in] data_size Size of the value in bytes. Has to be a positive value.
        void bind_single_uniform(int32 location, void* uniform_value, int64 data_size);

        //! \brief Binds an \a buffer for drawing.
        //! \param[in] index The \a buffer index to bind the \a buffer to.
        //! \param[in] buffer The \a buffer to bind.
        //! \param[in] target The target of the \a buffer to bind.
        void bind_buffer(int32 index, buffer_ptr buffer, buffer_target target);

        //! \brief Binds a \a texture for drawing.
        //! \param[in] binding The binding location to bind the \a texture to. Has to be a positive value.
        //! \param[in] texture A pointer to the \a texture to bind.
        //! \param[in] uniform_location The location to bind the index integer value to. Has to be a positive value.
        void bind_texture(int32 binding, texture_ptr texture, int32 uniform_location);

        //! \brief Binds a \a texture as an image.
        //! \param[in] binding The binding location to bind the \a texture to. Has to be a positive value.
        //! \param[in] texture A pointer to the \a texture to bind.
        //! \param[in] level The level of the image that should be bound. Has to be a positive value.
        //! \param[in] layered Specifies whether a layered texture binding is to be established.
        //! \param[in] layer Spezifies the layer if \a layered is false. Has to be a positive value.
        //! \param[in] access The \a base_access type.
        //! \param[in] element_format The format used for formatted stores.
        void bind_image_texture(int32 binding, texture_ptr texture, int32 level, bool layered, int32 layer, base_access access, format element_format);

        //! \brief Binds a \a framebuffer for drawing.
        //! \param[in] framebuffer The pointer to the \a framebuffer to bind.
        void bind_framebuffer(framebuffer_ptr framebuffer);

        //! \brief Adds a memory barrier.
        //! \param[in] barrier_bit The \a memory_barrier_bit to add the barrier to.
        void add_memory_barrier(memory_barrier_bit barrier_bit);

        //! \brief Locks a \a buffer after a modification.
        //! \param[in] buffer The pointer to the \a buffer to lock.
        void lock_buffer(buffer_ptr buffer);

        //! \brief Waits for a \a buffer after a series of gl calls.
        //! \param[in] buffer The pointer to the \a buffer to wait for.
        void wait_for_buffer(buffer_ptr buffer);

        //! \brief Calukates the mipmaps for the \a texture.
        //! \details This is used to recalculate the mipmaps after the pixels where changed by a compute shader.
        //! \param[in] texture The pointer to the \a texture to calculate the mipmaps.
        void calculate_mipmaps(texture_ptr texture);

        //! \brief Draws arrays.
        //! \details All the information not given in the argument list is retrieved from the state.
        //! \param[in] topology The topology used for drawing the bound vertex data.
        //! \param[in] first The first index to start drawing from. Has to be a positive value.
        //! \param[in] count The number of vertices to draw. Has to be a positive value.
        //! \param[in] instance_count The number of instances to draw. Has to be a positive value. For normal drawing pass 1.
        void draw_arrays(primitive_topology topology, int32 first, int32 count, int32 instance_count = 1);

        //! \brief Draws elements.
        //! \details All the information not given in the argument list is retrieved from the state.
        //! \param[in] topology The topology used for drawing the bound vertex data.
        //! \param[in] first The first index to start drawing from. Has to be a positive value.
        //! \param[in] count The number of indices to draw. Has to be a positive value.
        //! \param[in] type The \a index_type of the values in the index buffer.
        //! \param[in] instance_count The number of instances to draw. Has to be a positive value. For normal drawing pass 1.
        void draw_elements(primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count = 1);

        //! \brief Enables or disables face culling.
        //! \param[in] enabled True if face culling should be enabled, else false.
        void set_face_culling(bool enabled);

        //! \brief Dispatches the bound compute \a shader.
        //! \param[in] num_x_groups The number of work groups in x direction. Has to be a positive value.
        //! \param[in] num_y_groups The number of work groups in y direction. Has to be a positive value.
        //! \param[in] num_z_groups The number of work groups in z direction. Has to be a positive value.
        void dispatch_compute(int32 num_x_groups, int32 num_y_groups, int32 num_z_groups);

        //! \brief Sets the \a polygon_face for face culling.
        //! \param[in] face The \a polygon_face to use for face culling.
        void set_cull_face(polygon_face face);

        //! \brief Enables or disables blending.
        //! \param[in] enabled True if blending should be enabled, else false.
        void set_blending(bool enabled);

        //! \brief Sets the \a blend_factors for blending.
        //! \param[in] source The \a blend_factor influencing the source value.
        //! \param[in] destination The \a blend_factor influencing the destination value.
        void set_blend_factors(blend_factor source, blend_factor destination);

        // void bind_texture_buffer(uint32 target, uint32 index, buffer_view_ptr buffer, ptr_size offset, ptr_size size);

        //! \brief Submits a command to the \a command_buffer.
        //! \details This is also used by the \a command_buffer internally.
        //! \param[in] args The arguments required for creating a new \a command of type \a commandT.
        template <typename commandT, typename... Args>
        void submit(Args&&... args)
        {
            unique_ptr<command> new_command = mango::static_unique_pointer_cast<command>(mango::make_unique<commandT>(std::forward<Args>(args)...));

            if (nullptr != m_first)
            {
                MANGO_ASSERT(nullptr != m_last, "Last command is null, while first command is not!");
                MANGO_ASSERT(nullptr == m_last->m_next, "Last command has a successor!");
                m_last->m_next = std::move(new_command);
                m_last         = m_last->m_next.get();
            }
            else
            {
                MANGO_ASSERT(nullptr == m_last, "Last command is not null, but first command is!");
                m_first = std::move(new_command);
                m_last  = m_first.get();
            }
        }

        inline unique_ptr<command> head()
        {
            return std::move(m_first);
        }

        void attach(command_buffer_ptr& other);

        //! \brief Returns the current \a graphics_state for building the \a command queue.
        //! \return The current building \a graphics_state.
        inline graphics_state& get_state()
        {
            return m_building_state;
        }

      private:
        //! \brief Building state to build the \a command_buffer.
        //! \details This state is fictional and used for building up the \a commands.
        //! It is used to avoid redundant pipeline changes and calls.
        graphics_state m_building_state;

        //! \brief Execution state used while executing the \a command_buffer.
        //! \details This is the real state on the gpu used to mirror the state on the cpu while executing calls.
        graphics_state m_execution_state;

        //! \brief A unique_ptr to the first command.
        unique_ptr<command> m_first;

        //! \brief Pointer to the last command.
        command* m_last;
    };

} // namespace mango

#endif // MANGO_COMMAND_BUFFER_HPP