//! \file      graphics_state.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_STATE
#define MANGO_GRAPHICS_STATE

#include <array>
#include <graphics/graphics_common.hpp>

namespace mango
{
    //! \brief Holds information about the current state of a graphics pipeline.
    //! \details This is mostly used to avoid unnecessary calls to the gpu.
    //! The calls do only change values in this state; there is nothing changed in the real graphics state.
    //! All functions return true, if the values in the current state were changed, else false.
    //! This is used to check if a real call is required.
    struct graphics_state
    {
      public:
        graphics_state();
        ~graphics_state() = default;

        //! \brief Sets the viewport size.
        //! \param[in] x The x position of the viewport. Has to be a positive value.
        //! \param[in] y The y position of the viewport. Has to be a positive value.
        //! \param[in] width The width of the viewport. Has to be a positive value.
        //! \param[in] height The height of the viewport. Has to be a positive value.
        //! \return True if state changed, else false.
        bool set_viewport(int32 x, int32 y, int32 width, int32 height);

        //! \brief Enables or disables the depth test.
        //! \param[in] enabled True if the depth test should be enabled, else false.
        //! \return True if state changed, else false.
        bool set_depth_test(bool enabled);

        //! \brief Enables or disables the depth writing.
        //! \param[in] enabled True if the depth writing should be enabled, else false.
        //! \return True if state changed, else false.
        bool set_depth_write(bool enabled);

        //! \brief Sets the \a compare_operation for depth testing.
        //! \param[in] op The \a compare_operation to use for depth testing.
        //! \return True if state changed, else false.
        bool set_depth_func(compare_operation op);

        //! \brief Sets the \a polygon_mode as well as the \a polygon_faces used for drawing.
        //! \param[in] face The \a polygon_faces to draw.
        //! \param[in] mode The \a polygon_mode used for drawing.
        //! \return True if state changed, else false.
        bool set_polygon_mode(polygon_face face, polygon_mode mode);

        //! \brief Binds a \a vertex_array for drawing.
        //! \param[in] name The name of the \a vertex_array to bind.
        //! \return True if state changed, else false.
        bool bind_vertex_array(g_uint name);

        //! \brief Binds a \a shader_program for drawing.
        //! \param[in] name The name of the \a shader_program to bind.
        //! \return True if state changed, else false.
        bool bind_shader_program(g_uint name);

        //! \brief Binds a \a texture for drawing.
        //! \param[in] binding The binding location to bind the \a texture too. Has to be a positive value.
        //! \param[in] name The name of the \a texture.
        //! \return True if state changed, else false.
        bool bind_texture(int32 binding, g_uint name);

        //! \brief Binds a \a framebuffer for drawing.
        //! \param[in] name The name of the \a framebuffer to bind.
        //! \return True if state changed, else false.
        bool bind_framebuffer(g_uint name);

        //! \brief Binds a \a buffer.
        //! \param[in] name The OpenGL name of the \a buffer.
        //! \param[in] slot The slot to bind the \a buffer to.
        //! \param[in] offset The offset to start.
        //! \return True if state changed, else false.
        bool bind_buffer(g_uint name, int32 slot, int64 offset);

        //! \brief Enables or disables face culling.
        //! \param[in] enabled True if face culling should be enabled, else false.
        //! \return True if the face culling state changed, else false.
        bool set_face_culling(bool enabled);

        //! \brief Sets the \a polygon_face for face culling.
        //! \param[in] face The \a polygon_face to use for face culling.
        //! \return True if state changed, else false.
        bool set_cull_face(polygon_face face);

        //! \brief Enables or disables blending.
        //! \param[in] enabled True if blending should be enabled, else false.
        //! \return True if the the blending state changed, else false.
        bool set_blending(bool enabled);

        //! \brief Sets the \a blend_factors for blending.
        //! \param[in] source The \a blend_factor influencing the source value.
        //! \param[in] destination The \a blend_factor influencing the destination value.
        //! \return True if state changed, else false.
        bool set_blend_factors(blend_factor source, blend_factor destination);

        //! \brief Sets the polygon offset.
        //! \param[in] factor The factor to use.
        //! \param[in] units The offset units to use or 0.0f, when disabled.
        //! \return True if state changed, else false.
        bool set_polygon_offset(float factor, float units);

        //! \brief Marks the end of one frame, so the buffer offsets can be reset.
        void end_frame() // TODO Paul: Maybe not a good solution...
        {
            for (int32 i = 0; i < max_buffer_slots; ++i)
                m_internal_state.buffer_name_offset[i] = glm::ivec2(-1, -1);
        }

        //! \brief The maximum number of texture bindings (not really, just supported by the state).
        const static int32 max_texture_bindings = 16; // TODO Paul: We should really define these things somewhere else. And query from OpenGL.

        //! \brief The maximum number of buffer slot (not really, just supported by the state).
        const static int32 max_buffer_slots = 8; // TODO Paul: We should really define these things somewhere else. And query from OpenGL.

        //! \brief Structure to cache the state of the graphics pipeline.
        struct internal_state
        {
            g_uint shader_program; //!< Cached shader program.
            g_uint framebuffer;    //!< Cached framebuffer.
            g_uint vertex_array;   //!< Cached vertex array.

            std::array<g_uint, max_texture_bindings> m_active_texture_bindings; //!< Bindings from binding points to texture names.

            glm::ivec2 buffer_name_offset[max_buffer_slots]; //!< Buffer slot names and offsets per frame.

            struct
            {
                int32 x;      //!< Viewport x position.
                int32 y;      //!< Viewport y position.
                int32 width;  //!< Viewport width.
                int32 height; //!< Viewport height.
            } viewport;       //!< Cached viewport.

            struct
            {
                polygon_face face; //!< Polygon mode face.
                polygon_mode mode; //!< Polygon mode.
            } poly_mode;           //!< Cached polygon mode.

            struct
            {
                bool enabled;                 //!< Enabled or disabled.
                compare_operation depth_func; //!< Compare operation.
            } depth_test;                     //!< Cached depth test.

            bool depth_write; //!< Cached depth write.

            struct
            {
                bool enabled;      //!< Enabled or disabled.
                polygon_face face; //!< Polygon face.
            } face_culling;        //!< Cached face cull state.

            struct
            {
                bool enabled;      //!< Enabled or disabled.
                blend_factor src;  //!< Source blend factor.
                blend_factor dest; //!< Destination blend factor.
            } blending;            //!< Cached blend state.
        } m_internal_state;        //!< The internal state.
    };
} // namespace mango

#endif // MANGO_GRAPHICS_STATE