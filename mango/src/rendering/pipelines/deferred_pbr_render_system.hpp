//! \file      deferred_pbr_render_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
#define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP

#include <rendering/render_system_impl.hpp>

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

      private:
        //! \brief The gbuffer of the deferred pipeline.
        framebuffer_ptr m_gbuffer;

        shader_program_ptr m_scene_geometry_pass;

        shader_program_ptr m_lighting_pass;

        struct scene_vertex_uniforms
        {
            std140_mat4 model_matrix;    //!< The model matrix.
            std140_mat3 normal_matrix;   //!< The normal matrix.
            std140_mat4 view_projection; //!< The cameras view projection matrix.
        };
        scene_vertex_uniforms* m_active_scene_vertex_uniforms;
        buffer_ptr m_scene_vertex_uniform_buffer;

        struct scene_material_uniforms
        {
            std140_vec4 base_color;
            g_float metallic;
            g_float roughness;

            bool base_color_texture; //!< Specifies, if the component texture is enabled for the base color value.
            bool metallic_texture;   //!< Specifies, if the component texture is enabled for the metallic value.
            bool roughness_texture;  //!< Specifies, if the component texture is enabled for the roughness value.
            bool normal_texture;     //!< Specifies, if the normal texture is enabled.
        };
        scene_material_uniforms* m_active_scene_material_uniforms;
        buffer_ptr m_scene_material_uniform_buffer;
    };

} // namespace mango

#endif // #define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
