//! \file      deferred_pbr_render_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
#define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP

#include <rendering/render_system_impl.hpp>
#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
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
        virtual void begin_render() override;
        virtual void finish_render() override;
        virtual void set_viewport(int32 x, int32 y, int32 width, int32 height) override;
        virtual void update(float dt) override;
        virtual void destroy() override;
        virtual render_pipeline get_base_render_pipeline() override;

        void set_model_info(const glm::mat4& model_matrix, bool has_normals, bool has_tangents) override;
        void draw_mesh(const material_ptr& mat, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count) override;
        void set_view_projection_matrix(const glm::mat4& view_projection) override;
        void set_environment_texture(const texture_ptr& hdr_texture, float render_level) override;

        framebuffer_ptr get_backbuffer() override
        {
            return m_backbuffer;
        }

      private:
        //! \brief The gbuffer of the deferred pipeline.
        framebuffer_ptr m_gbuffer;
        //! \brief The backbuffer of the deferred pipeline.
        framebuffer_ptr m_backbuffer;

        //! \brief The \a shader_program for the deferred geometry pass.
        //! \details This fills the g-buffer for later use in the lighting pass.
        shader_program_ptr m_scene_geometry_pass;

        //! \brief The \a shader_program for the lighting pass.
        //! \details Utilizes the g-buffer filled before.
        shader_program_ptr m_lighting_pass;

        //! \brief The prealocated size for all uniforms.
        //! \details The buffer is filled every frame. 1 MiB should be enough for now.
        const int32 uniform_buffer_size = 1048576;
        int32 m_frame_uniform_offset;     //!< The current offset in the uniform memory to write to.
        g_int m_uniform_buffer_alignment; //!< The alignment of the structures in the uniform buffer. Gets queried from OpenGL.
        //! \brief The mapped memory to be filled with all uniforms blocks per frame.
        void* m_mapped_uniform_memory;

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

        //! \brief Optional additional steps of the deferred pipeline.
        shared_ptr<pipeline_step> m_pipeline_steps[mango::render_step::number_of_step_types];
    };

} // namespace mango

#endif // #define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
