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
            return command_buffer_ptr();
        };

        //! \brief Executes all commands in the \a command_buffer since the lase call to execute().
        //! \details Does traverse the internal linked list and clears it while doing this.
        void execute();

        //! \brief Sets the viewport size.
        //! \param[in] x The x position of the viewport.
        //! \param[in] y The y position of the viewport.
        //! \param[in] width The width of the viewport.
        //! \param[in] height The height of the viewport.
        void set_viewport(uint32 x, uint32 y, uint32 width, uint32 height);

        //! \brief Clears attachments of a framebuffer.
        //! \param[in] buffer_mask The mask describes which buffers should be cleared.
        //! \param[in] attachment_mask The mask describes which attachments should be cleared.
        //! \param[in] r The red component to clear color attachments.
        //! \param[in] g The green component to clear color attachments.
        //! \param[in] b The blue component to clear color attachments.
        //! \param[in] a The alpha component to clear color attachments.
        //! \param[in] framebuffer The pointer to the \a framebuffer to clear. To clear the default framebuffer leave empty or pass nullptr.
        void clear_framebuffer(clear_buffer_mask buffer_mask, attachement_mask attachment_mask, g_float r, g_float g, g_float b, g_float a, framebuffer_ptr framebuffer = nullptr);

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

        //! \brief Binds all \a uniforms not included in uniform buffers for drawing.
        //! \details The void* \a uniform_data should contain values for all uniforms not included in any uniform buffer object.
        //! Order is relevant, as well as the 64 byte alignment.
        //! \param[in] uniform_data Pointer to a \a uniform_data struct. Alignment: 64 bytes.
        //! \param[in] data_size Size of the \a uniform_data struct in bytes.
        void bind_single_uniforms(void* uniform_data, g_intptr data_size);

        //! \brief Binds a \a texture for drawing.
        //! \param[in] location The binding location to bind the \a texture too.
        //! \param[in] texture A pointer to the \a texture to bind.
        //! \param[in] sampler A pointer to the \a sampler to use.
        void bind_texture(uint32 location, texture_ptr texture, sampler_ptr sampler);

        //! \brief Binds a \a framebuffer for drawing.
        //! \param[in] framebuffer The pointer to the \a framebuffer to bind.
        void bind_framebuffer(framebuffer_ptr framebuffer);

        //! \brief Draws arrays.
        //! \details All the information not given in the argument list is retrieved from the state.
        //! \param[in] topology The topology used for drawing the bound vertex data.
        //! \param[in] first The first index to start drawing from.
        //! \param[in] count The number of vertices to draw.
        //! \param[in] instance_count The number of instances to draw. For normal drawing pass 1.
        void draw_arrays(primitive_topology topology, uint32 first, uint32 count, uint32 instance_count = 1);

        //! \brief Draws elements.
        //! \details All the information not given in the argument list is retrieved from the state.
        //! \param[in] topology The topology used for drawing the bound vertex data.
        //! \param[in] first The first index to start drawing from.
        //! \param[in] count The number of indices to draw.
        //! \param[in] instance_count The number of instances to draw. For normal drawing pass 1.
        void draw_elements(primitive_topology topology, uint32 first, uint32 count, uint32 instance_count = 1);

        //! \brief Enables or disables face culling.
        //! \param[in] enabled True if face culling should be enabled, else false.
        void set_face_culling(bool enabled);

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

        // void bind_uniform_buffer(uint32 target, uint32 index, buffer_view_ptr buffer, ptr_size offset, ptr_size size);
        // void bind_texture_buffer(uint32 target, uint32 index, buffer_view_ptr buffer, ptr_size offset, ptr_size size);

        //! \brief Submits a command to the \a command_buffer.
        //! \details This is also used by the \a command_buffer internally.
        //! \param[in] args The arguments required for creating a new \a command of type \a commandT.
        template <typename commandT, typename... Args>
        void submit(Args&&... args)
        {
            unique_ptr<command> new_command = static_unique_pointer_cast<command>(make_unique<commandT>(std::forward<Args>(args)...));

            if (nullptr != m_first)
            {
                MANGO_ASSERT(nullptr != m_last, "Last command is null, while first command is not!");
                MANGO_ASSERT(nullptr == m_last->m_next, "Last command has a successor!");
                m_last->m_next = std::move(new_command);
                m_last         = new_command.get();
            }
            else
            {
                MANGO_ASSERT(nullptr == m_last, "Last command is not null, but first command is!");
                m_first = std::move(new_command);
                m_last  = m_first.get();
            }
        }

      private:
        command_buffer();
        ~command_buffer();

        //! \brief The internal state to retrieve and cache data.
        graphics_state m_state;

        //! \brief A unique_ptr to the first command.
        unique_ptr<command> m_first = nullptr;

        //! \brief Pointer to the last command.
        command* m_last = nullptr;
    };

} // namespace mango

#endif // MANGO_COMMAND_BUFFER_HPP