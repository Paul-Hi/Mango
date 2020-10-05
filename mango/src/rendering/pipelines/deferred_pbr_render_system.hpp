//! \file      deferred_pbr_render_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
#define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP

#include <graphics/framebuffer.hpp>
#include <rendering/render_system_impl.hpp>
#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
    struct camera_data;

    //! \brief A \a render_system using a deferred base pipeline supporting physically based rendering.
    //! \details This system supports physically based materials with and without textures.
    class deferred_pbr_render_system : public render_system_impl
    {
      public:
        //! \brief Constructs the \a deferred_pbr_render_system.
        //! \param[in] context The internally shared context of mango.
        deferred_pbr_render_system(const shared_ptr<context_impl>& context);
        ~deferred_pbr_render_system();

        virtual bool create() override;
        virtual void configure(const render_configuration& configuration) override;
        virtual void setup_ibl_step(const ibl_step_configuration& configuration) override;
        virtual void setup_shadow_map_step(const shadow_step_configuration& configuration) override;
        virtual void begin_render() override;
        virtual void finish_render(float dt) override;
        virtual void set_viewport(int32 x, int32 y, int32 width, int32 height) override;
        virtual void update(float dt) override;
        virtual void destroy() override;
        virtual render_pipeline get_base_render_pipeline() override;

        void set_model_info(const glm::mat4& model_matrix, bool has_normals, bool has_tangents) override;
        void draw_mesh(const vertex_array_ptr& vertex_array, const material_ptr& mat, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count) override;
        void set_view_projection_matrix(const glm::mat4& view_projection) override;
        void set_environment_texture(const texture_ptr& hdr_texture) override;
        void submit_light(light_type type, light_data* data) override;
        void on_ui_widget() override;

        framebuffer_ptr get_backbuffer() override
        {
            return m_backbuffer;
        }

      private:
        //! \brief The gbuffer of the deferred pipeline.
        framebuffer_ptr m_gbuffer;
        //! \brief The backbuffer of the deferred pipeline.
        framebuffer_ptr m_backbuffer;
        //! \brief The hdr buffer of the deferred pipeline. Used for auto exposure.
        framebuffer_ptr m_hdr_buffer;

        //! \brief Render queue used to store rendering related commands.
        command_buffer_ptr m_render_queue;

        //! \brief The \a shader_program for the deferred geometry pass.
        //! \details This fills the g-buffer for later use in the lighting pass.
        shader_program_ptr m_scene_geometry_pass;

        //! \brief The \a shader_program for the lighting pass.
        //! \details Utilizes the g-buffer filled before. Outputs hdr.
        shader_program_ptr m_lighting_pass;

        //! \brief The \a shader_program for the luminance buffer construction.
        //! \details Constructs the 'luminance' histogram.
        shader_program_ptr m_construct_luminance_buffer;

        //! \brief The \a shader_program for the luminance buffer reduction.
        //! \details Reduces the 'luminance' histogram to the average luminance.
        shader_program_ptr m_reduce_luminance_buffer;

        //! \brief The shader storage buffer mapping for the luminance histogram
        buffer_ptr m_luminance_histogram_buffer;

        //! \brief The mapped luminance data from the histogram calculation.
        luminance_data* m_luminance_data_mapping;

        //! \brief The \a shader_program for the composing pass.
        //! \details Takes the output in the hdr_buffer and does the final composing to get it to the screen.
        shader_program_ptr m_composing_pass;

        //! \brief The prealocated size for all uniforms.
        //! \details The buffer has 3 parts and is filled every frame. 1 MiB should be enough for now.
        const int32 uniform_buffer_size = 1048576;
        //! \brief The size of one frame in the uniform buffer. Used for triple buffering.
        const int32 frame_size       = uniform_buffer_size / 3 - 512; // TODO Paul: We give some padding for large uniform alignments here...
        int32 m_current_buffer_part  = 0;                             //!< The current part of the uniform buffer in use [0, 1, 2].
        int32 m_current_buffer_start = 0;                             //!< The pointer to the start of the current part of the uniform buffer in use.
        int32 m_frame_uniform_offset;                                 //!< The current offset in the uniform memory to write to.
        int32 m_last_offset;                                          //!< The last frames offset in the uniform memory.
        g_int m_uniform_buffer_alignment;                             //!< The alignment of the structures in the uniform buffer. Gets queried from OpenGL.
        //! \brief The mapped memory to be filled with all uniforms blocks per frame.
        void* m_mapped_uniform_memory;
        //! \brief Sync objects for gpu <-> cpu synchronization.
        g_sync m_frame_buffer_sync[3];

        //! \brief The uniform buffer mapping the gpu buffer to the scene uniforms.
        buffer_ptr m_frame_uniform_buffer;

        //! \brief Uniform struct for the geometry passes vertex shader.
        struct scene_vertex_uniforms
        {
            std140_mat4 model_matrix;  //!< The model matrix.
            std140_mat3 normal_matrix; //!< The normal matrix.

            std140_bool has_normals;  //!< Specifies if the next mesh has normals as a vertex attribute.
            std140_bool has_tangents; //!< Specifies if the next mesh has tangents as a vertex attribute.

            g_float padding0; //!< Padding needed for st140 layout.
            g_float padding1; //!< Padding needed for st140 layout.
        };

        //! \brief Uniform struct for the geometry passes fragment shader to implement \a materials.
        struct scene_material_uniforms
        {
            std140_vec4 base_color;     //!< The base color (rgba). Also used as reflection color for metallic surfaces.
            std140_vec3 emissive_color; //!< The emissive color of the material if existent, else (0, 0, 0).
            g_float metallic;           //!< The metallic value of the material.
            g_float roughness;          //!< The roughness of the material.

            std140_bool base_color_texture;         //!< Specifies, if the the base color texture is enabled.
            std140_bool roughness_metallic_texture; //!< Specifies, if the component texture is enabled for the metallic value and the roughness value.
            std140_bool occlusion_texture;          //!< Specifies, if the component texture is enabled for the occlusion value.
            std140_bool packed_occlusion;           //!< Specifies, if the occlusion value is packed into the r channel of the roughness_metallic_texture.
            std140_bool normal_texture;             //!< Specifies, if the normal texture is enabled.
            std140_bool emissive_color_texture;     //!< Specifies, if the the emissive color texture is enabled.

            g_int alpha_mode;     //!< Specifies the alpha mode to render the material with.
            g_float alpha_cutoff; //!< Specifies the alpha cutoff value to render the material with.

            g_float padding0; //!< Padding needed for st140 layout.
            g_float padding1; //!< Padding needed for st140 layout.
        };

        //! \brief Uniform buffer structure for the lighting pass of the deferred pipeline.
        struct lighting_pass_uniforms
        {
            std140_mat4 inverse_view_projection; //!< Inverse camera view projection matrix.
            std140_mat4 view;                    //!< Camera view matrix.
            std140_vec3 camera_position;         //!< Camera position.
            std140_vec4 camera_params;           //!< Camera near and far plane depth value. (zw) unused atm.

            struct
            {
                //  TODO Paul: Size hardcoded.
                std140_mat4 view_projections[4]; //!< The 4 view projection matrices of the different cascades.
                std140_vec4 far_planes;          //!< The 4 far planes of the different cascade views.
                std140_vec4 cascade_info;        //!< The 3 cascade splits. w component is the cascade shadow map resolution.
                std140_vec3 direction;           //!< The direction to the light.
                std140_vec3 color;               //!< The light color.
                g_float intensity;               //!< The intensity of the directional light in lumen
                std140_bool cast_shadows;        //!< True, if shadows can be casted.
            } directional;                       //!< Data for the directional light (onyl one)
            // struct
            // {
            //     std140_vec3 position;
            //     std140_vec3 color;
            //     g_float intensity;
            // } spherical[16];

            std140_bool debug_view_enabled; //!< True, if any debug view is enabled.

            struct
            {
                std140_bool debug[9]; //!< All debug views.
                // position
                // normal
                // depth
                // base_color
                // reflection_color
                // emission
                // occlusion
                // roughness
                // metallic
            } debug_views; //!< The debug views.

            struct
            {
                std140_bool show_cascades;    //!< Show the shadow cascades.
                std140_bool draw_shadow_maps; //!< Draw the cascade shadow maps.
            } debug_options;                  //!< The debug options.

            float padding0; //!< padding.
            float padding1; //!< padding.
        };

        //! \brief The current \a lighting_pass_uniforms.
        lighting_pass_uniforms m_lp_uniforms;

        //! \brief Binds the uniform buffer of the lighting pass.
        //! \param[in,out] camera The \a camera_data of the current camera.
        void bind_lighting_pass_uniform_buffer(camera_data& camera);

        //! \brief Calculates automatic exposure and adapts physical camera parameters.
        //! \param[in,out] camera The \a camera_data of the current camera.
        void apply_auto_exposure(camera_data& camera);

        //! \brief Optional additional steps of the deferred pipeline.
        shared_ptr<pipeline_step> m_pipeline_steps[mango::render_step::number_of_step_types];

        //! \brief True if the renderer should draw wireframe, else false.
        bool m_wireframe = false;
    };

} // namespace mango

#endif // #define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
