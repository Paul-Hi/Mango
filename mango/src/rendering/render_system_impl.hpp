//! \file      render_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_SYSTEM_IMPL_HPP
#define MANGO_RENDER_SYSTEM_IMPL_HPP

#include <core/context_impl.hpp>
//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <glm/glm.hpp>
#include <graphics/command_buffer.hpp>
#include <mango/render_system.hpp>
#include <queue>

namespace mango
{
    //! \brief Some stats regarding hardware.
    struct hardware_stats
    {
        //! \brief The graphics API version used.
        string api_version;

        struct
        {
            int32 draw_calls;    //!< The number of draw calls.
            int32 meshes;        //!< The number of meshes.
            int32 primitives;    //!< The number of primitives.
            int32 materials;     //!< The number of materials.
            int32 canvas_x;      //!< The x of the current render canvas.
            int32 canvas_y;      //!< The y of the current render canvas.
            int32 canvas_width;  //!< The width of the current render canvas.
            int32 canvas_height; //!< The height of the current render canvas.
        } last_frame;            //!< Measured stats from the last rendered frame.
    };

    //! \brief Structure to store data for adaptive exposure.
    struct luminance_data
    {
        uint32 histogram[256]; //!< The histogram data
        float luminance;       //!< Smoothed out average luminance.
    };

    enum class light_type : uint8;
    struct light_data;

    //! \brief The implementation of the \a render_system.
    //! \details This class only manages the configuration of the base \a render_system and forwards everything else to the real implementation of the specific configured one.
    class render_system_impl : public render_system
    {
      public:
        //! \brief Constructs the \a render_system_impl.
        //! \param[in] context The internally shared context of mango.
        render_system_impl(const shared_ptr<context_impl>& context);
        ~render_system_impl();

        virtual bool create() override;
        virtual void configure(const render_configuration& configuration) override;
        virtual void setup_ibl_step(const ibl_step_configuration& configuration) override;
        virtual void setup_shadow_map_step(const shadow_step_configuration& configuration) override;

        //! \brief Does all the setup and has to be called before rendering the scene.
        //! \details Adds setup and clear calls to the \a command_buffer.
        virtual void begin_render();

        //! \brief Renders the current frame.
        //! \details Calls the execute() function of the \a command_buffer, after doing some other things to it.
        //! This includes for example extra framebuffers and passes.
        //! \param[in] dt Past time since last call.
        virtual void finish_render(float dt);

        //! \brief Sets the viewport.
        //! \details  Should be called on resizing events, instead of scheduling a viewport command directly.
        //! This manages the resizing of eventually created \a framebuffers internally and schedules the \a command as well.
        //! \param[in] x The x start coordinate. Has to be a positive value.
        //! \param[in] y The y start coordinate. Has to be a positive value.
        //! \param[in] width The width of the viewport after this call. Has to be a positive value.
        //! \param[in] height The height of the viewport after this call. Has to be a positive value.
        virtual void set_viewport(int32 x, int32 y, int32 width, int32 height);

        virtual void update(float dt) override;
        virtual void destroy() override;

        //! \brief Retrieves and returns the base \a render_pipeline of the real implementation of the \a render_system.
        //! \details This needs to be overriden by th real \a render_system_impl.
        //! \return The current set base \a render_pipeline of the \a render_system.
        virtual render_pipeline get_base_render_pipeline();

        //! \brief Begin rendering a mesh.
        //! \details This has to be called before using a material and drawing a primitive.
        //! \param[in] model_matrix The model matrix to use.
        //! \param[in] has_normals Specifies if the following mesh primitives have normals as a vertex attribute.
        //! \param[in] has_tangents Specifies if the following mesh primitives have tangents as a vertex attribute.
        virtual void begin_mesh(const glm::mat4& model_matrix, bool has_normals, bool has_tangents);

        //! \brief End the model rendering.
        //! \details Should be called after all mesh primitives are drawn.
        virtual void end_mesh();

        //! \brief Use a material.
        //! \param[in] mat The \a material to use.
        virtual void use_material(const material_ptr& mat);

        //! \brief Schedules drawing of a \a mesh.
        //! \param[in] vertex_array The \a vertex_array_ptr for the next draw call.
        //! \param[in] topology The topology used for drawing the bound vertex data.
        //! \param[in] first The first index to start drawing from. Has to be a positive value.
        //! \param[in] count The number of indices to draw. Has to be a positive value.
        //! \param[in] type The \a index_type of the values in the index buffer.
        //! \param[in] instance_count The number of instances to draw. Has to be a positive value. For normal drawing pass 1.
        virtual void draw_mesh(const vertex_array_ptr& vertex_array, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count = 1);

        //! \brief Sets the \a texture for a environment.
        //! \param[in] hdr_texture The pointer to the hdr \a texture to use as an environment.
        virtual void set_environment_texture(const texture_ptr& hdr_texture);

        //! \brief Submits a light to the \a render_system.
        //! \param[in] type The submitted \a light_type.
        //! \param[in] data The \a light_data describing the submitted light.
        virtual void submit_light(light_type type, light_data* data);

        //! \brief Returns the backbuffer of the a render_system.
        //! \return The backbuffer.
        virtual framebuffer_ptr get_backbuffer();

        //! \brief Returns some stats regarding hardware.
        //! \return Some stats regarding hardware.
        inline const hardware_stats& get_hardware_stats()
        {
            MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
            return m_current_render_system->m_hardware_stats;
        }

        //! \brief Custom UI function.
        //! \details This can be called by any \a ui_widget and displays settings and debug information for the active \a render_system.
        //! This does not draw any window, so it needs one surrounding it.
        virtual void on_ui_widget();

      protected:
        //! \brief Mangos internal context for shared usage in all \a render_systems.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The hardware stats.
        hardware_stats m_hardware_stats;

      private:
        //! \brief A shared pointer to the currently used internal \a render_system.
        //! \details This is used to make runtime switching of different \a render_systems possible.
        shared_ptr<render_system_impl> m_current_render_system;
    };

} // namespace mango

#endif // #define MANGO_RENDER_SYSTEM_IMPL_HPP
