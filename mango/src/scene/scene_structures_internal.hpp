//! \file      scene_structures_internal.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_STRUCTURES_INTERNAL
#define MANGO_SCENE_STRUCTURES_INTERNAL

#include <graphics/graphics_resources.hpp>
#include <mango/scene_structures.hpp>
#include <rendering/renderer_impl.hpp>

namespace mango
{
    //! \brief Macro to add some default operators and constructors to every internal scene structure.
#define DECLARE_SCENE_INTERNAL(c)     \
    ~c()        = default;            \
    c(const c&) = default;            \
    c(c&&)      = default;            \
    c& operator=(const c&) = default; \
    c& operator=(c&&) = default;

    //! \brief The \a texture gpu data.
    struct texture_gpu_data
    {
        //! \brief The gpu \a gfx_texture.
        gfx_handle<const gfx_texture> graphics_texture;
        //! \brief The gpu \a gfx_sampler.
        gfx_handle<const gfx_sampler> graphics_sampler;

        texture_gpu_data()
            : graphics_texture(nullptr)
            , graphics_sampler(nullptr)
        {
        }
        //! \brief The \a texture_gpu_data is an internal scene structure.
        DECLARE_SCENE_INTERNAL(texture_gpu_data);
    };

    //! \brief The \a material gpu data.
    struct material_gpu_data
    {
        //! \brief The gpu data per material.
        material_data per_material_data;

        material_gpu_data() = default;
        //! \brief The \a material_gpu_data is an internal scene structure.
        DECLARE_SCENE_INTERNAL(material_gpu_data);
    };

    //! \brief The \a primitive gpu data.
    struct primitive_gpu_data
    {
        //! \brief The \a scene_primitives \a vertex_input_descriptor describing the vertex input for the pipeline.
        //! \details The \a renderer does the pipeline setup and should also cache this.
        vertex_input_descriptor vertex_layout;
        //! \brief The \a scene_primitives \a input_assembly_descriptor describing the input assembly for the pipeline.
        //! \details The \a renderer does the pipeline setup and should also cache this.
        input_assembly_descriptor input_assembly;

        uid manager_id;

        primitive_gpu_data() {}
        //! \brief The \a primitive_gpu_data is an internal scene structure.
        DECLARE_SCENE_INTERNAL(primitive_gpu_data);
    };

    //! \brief The \a mesh gpu data.
    struct mesh_gpu_data
    {
        //! \brief The gpu data per mesh.
        model_data per_mesh_data;

        mesh_gpu_data() = default;
        //! \brief The \a mesh_gpu_data is an internal scene structure.
        DECLARE_SCENE_INTERNAL(mesh_gpu_data);
    };

    //! \brief The \a camera gpu_data.
    struct camera_gpu_data
    {
        //! \brief The gpu data per camera.
        camera_data per_camera_data;
        //! \brief The graphics uniform buffer for uploading \a camera_data.
        gfx_handle<const gfx_buffer> camera_data_buffer;

        camera_gpu_data() = default;
        //! \brief The \a camera_gpu_data is an internal scene structure.
        DECLARE_SCENE_INTERNAL(camera_gpu_data);
    };

    //! \brief The \a light gpu_data.
    struct light_gpu_data
    {
        //! \brief The gpu data for all lights.
        light_data scene_light_data;
        //! \brief The graphics uniform buffer for uploading \a light_data. Filled with data provided by the \a light_stack.
        gfx_handle<const gfx_buffer> light_data_buffer;

        light_gpu_data() = default;
        //! \brief The \a light_gpu_data is an internal scene structure.
        DECLARE_SCENE_INTERNAL(light_gpu_data);
    };

    //! \brief An internal structure holding data for rendering.
    struct render_instance
    {
        //! \brief The \a uid of the \a scene_render_instances \a scene_node.
        uid node_id;

        render_instance() = default;
        //! \brief Constructs a \a render_instance with a \a scene_node.
        //! \param[in] node The \a uid of the \a scene_node to construct the \a render_instance with.
        render_instance(const uid node)
            : node_id(node)
        {
        }
        //! \brief The \a render_instance is an internal scene structure.
        DECLARE_SCENE_INTERNAL(render_instance);
    };

#undef DECLARE_SCENE_INTERNAL
} // namespace mango

#endif // MANGO_SCENE_STRUCTURES_INTERNAL