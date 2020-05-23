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
        virtual void set_viewport(uint32 x, uint32 y, uint32 width, uint32 height) override;
        virtual void update(float dt) override;
        virtual void destroy() override;
        virtual render_pipeline get_base_render_pipeline() override;

        void set_model_matrix(const glm::mat4& model_matrix) override;
        void push_material(const material_ptr& mat) override;
        void set_view_projection_matrix(const glm::mat4& view_projection) override;
        void set_environment_texture(const texture_ptr& hdr_texture) override;

      private:
        //! \brief The gbuffer of the deferred pipeline.
        framebuffer_ptr m_gbuffer;

        //! \brief The \a shader_program for the deferred geometry pass.
        //! \details This fills the g-buffer for later use in the lighting pass.
        shader_program_ptr m_scene_geometry_pass;

        //! \brief The \a shader_program for the lighting pass.
        //! \details Utilizes the g-buffer filled before.
        shader_program_ptr m_lighting_pass;

        //! \brief Uniform struct for the geometry passes vertex shader.
        struct scene_vertex_uniforms
        {
            std140_mat4 model_matrix;    //!< The model matrix.
            std140_mat3 normal_matrix;   //!< The normal matrix.
            std140_mat4 view_projection; //!< The cameras view projection matrix.
        };
        //! \brief Pointer to the currently active scene uniforms for the geometry pass vertex shader.
        scene_vertex_uniforms* m_active_scene_vertex_uniforms;
        //! \brief The uniform buffer mapping the gpu buffer to the active scene vertex uniforms.
        buffer_ptr m_scene_vertex_uniform_buffer;

        //! \brief Uniform struct for the geometry passes fragment shader to implement \a materials.
        struct scene_material_uniforms
        {
            std140_vec4 base_color;     //!< The base color (rgba). Also used as reflection color for metallic surfaces.
            std140_vec3 emissive_color; //!< The emissive color of the material if existent, else (0, 0, 0).
            g_float metallic;           //!< The metallic value of the material.
            g_float roughness;          //!< The roughness of the material.

            std140_bool base_color_texture;                   //!< Specifies, if the the base color texture is enabled.
            std140_bool occlusion_roughness_metallic_texture; //!< Specifies, if the component texture is enabled for the metallic value and the roughness value.
            std140_bool normal_texture;                       //!< Specifies, if the normal texture is enabled.
            std140_bool emissive_color_texture;               //!< Specifies, if the the emissive color texture is enabled.

            g_float padding0; //!< Padding needed for st140 layout.
            g_float padding1; //!< Padding needed for st140 layout.
        };
        //! \brief Pointer to the currently active scene material uniforms for the geometry pass fragment shader.
        scene_material_uniforms* m_active_scene_material_uniforms;
        //! \brief The uniform buffer mapping the gpu buffer to the active scene material fragment uniforms.
        buffer_ptr m_scene_material_uniform_buffer;

        //! \brief Optional additional steps of the deferred pipeline.
        shared_ptr<pipeline_step> m_pipeline_steps[mango::render_step::number_of_step_types];
    };

} // namespace mango

#endif // #define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
