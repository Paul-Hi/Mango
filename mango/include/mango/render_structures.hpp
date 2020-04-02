//! \file      render_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_STRUCTURES_HPP
#define MANGO_RENDER_STRUCTURES_HPP

#include <mango/types.hpp>
#include <unordered_map>

namespace mango
{
    //! \brief Specifies the data for any \a render_command.
    using render_command_data = void;
    //! \brief Specifies the uniform value for any unifomr in the \a uniform_upload_data.
    //! \details The value can be anything but should be of the type the uniform is meant for.
    //! The \a render_system checks the shader before uploading uniform data to upload the right thing.
    using uniform_value = void;

    //! \brief Specifies the type of any \a render_command.
    //! \details This is necessary to create the correct gpu calls later on.
    enum render_command_type
    {
        vao_binding,            //!< Command will generate gpu calls to bind a vertex array object.
        shader_program_binding, //!< Command will generate gpu calls to bind and use a shader program.
        input_binding,          //!< Command will generate gpu calls to bind any resource as an input.
        output_binding,         //!< Command will generate gpu calls to bind any resource as an output. WARNING: This may break the pipeline!
        uniform_binding,        //!< Command will generate gpu calls to bind any key value pair as an uniform, if it exists and is valid.
        draw_call               //!< Command will generate gpu calls to draw bound geometry with all bound resources and uniforms.
    };

    //! \brief The command that can be submitted to any \a render_system.
    //! \details These commands get collected and in the end gpu calls will be generated and executed.
    struct render_command
    {
        render_command_type type; //!< The type to identify usage and data.
        //! \brief The data that is used to provide more information and relevant resources.
        //! \details The pointer has to be valid for the whole time this \a render_command is in use.
        //! The submitter is in charge to guarantee this.
        render_command_data* data;
    };

    //! \brief The data for a \a render_command of \a render_command_type::vao_binding
    struct vao_binding_data
    {
        uint32 handle; //!< The handle of the vertex array object that should be bound.
    };

    //! \brief The data for a \a render_command of \a render_command_type::shader_program_binding
    struct shader_program_binding_data
    {
        uint32 handle; //!< The handle of the shader program that should be used.
        //! \brief The data to check provided inputs and outputs against.
        //! \details This maps names to a pair of \a gpu_resource_types and binding locations.
        //! This should be set for every sampler and uniform input in the shader.
        std::unordered_map<string, std::pair<gpu_resource_type, uint32>> binding_data;
    };

    //! \brief The data for \a render_commands of \a render_command_type::input_binding or \a render_command_type::output_binding
    struct resource_binding_data
    {
        uint32 handle;                   //!< The handle of the resource that should be bound.
        gpu_resource_type resource_type; //!< The resources type.
        //! \brief The name of the resource in the shader program.
        //! \details There will be a check if the binding name is a valid resource sampler in the shader program.
        const char* binding_name;
    };

    //! \brief The data for a \a render_command of \a render_command_type::uniform_binding
    struct uniform_binding_data
    {
        //! \brief The name of the uniform in the shader program.
        //! \details There will be a check if the binding name is a valid uniform in the shader program.
        //! The type of \a value will be determined by the information coming from the shader.
        const char* binding_name;
        //! \brief The value that should be set for the uniform.
        //! \details For the binding procedure this value will be casted to the determined type.
        //! If the value does hold something else the behaviour is undefined.
        uniform_value* value;
    };

    //! \brief The specification for a draw call.
    //! \details This will directly translate into a gpu draw call.
    enum gpu_draw_call
    {
        clear_call,             //!< Clear the screen.
        draw_arrays,            //!< Draw not indexed vertex data.
        draw_elements,          //!< Draw indexed vertex data.
        draw_arrays_instanced,  //!< Draw not indexed vertex data instanced.
        draw_elements_instanced //!< Draw indexed vertex data instanced.
    };

    //! \brief The type specification for primitives used in draw calls.
    enum gpu_primitive_type
    {
        points         = 0, //!< Points. A point for every point.
        lines          = 1, //!< Lines. Make one line every two points.
        line_loop      = 2, //!< Line loop. A line with connection at the end to the start.
        line_strip     = 3, //!< Line strips. Make a new line for every new point.
        triangles      = 4, //!< Triangles. Make one triangle every three points.
        triangle_strip = 5, //!< Triangle strips. Make a new triangle for every new point.
        triangle_fan   = 6 //!< Triangle fan. Just strange.
    };

    //! \brief This state describes if depth testing should be enabled and if so which depth compare function is used.
    enum depth_state
    {
        depth_off,  //!< No depth test.
        depth_less, //!< Primitives pass if the incoming depth value is less than the stored depth value. Default setting.
    };
    //! \brief This state describes if faces should be culled and if so, which ones.
    enum cull_state
    {
        cull_off,      //!< No face culling.
        cull_backface, //!< Backface culling.
        cull_frontface //! Frontface culling.
    };

    //! \brief This state describes if wireframe rendering should be used.
    enum wireframe_state
    {
        wireframe_off, //!< No wireframe.
        wireframe_on   //!< Wireframe rendering is turned on.
    };

    //! \brief This describes the blend state and if it is turned on specifies the blend function to use.
    enum blend_state
    {
        blend_off,                              //!< No blending.
        blend_src_alpha_and_one_minus_src_aplha //!< Blending enabled. Standard blend function for basic transparency.
    };

    //! \brief A simple structure for the clear color specification.
    struct clear_color
    {
        float r; //!< The red component of the color.
        float g; //!< The green component of the color.
        float b; //!< The blue component of the color.
        float a; //!< The alpha component of the color.
    };

    //! \brief This should be used to inform about CHANGES in the render state.
    //! \details If there are no changes in the state this should explicitly be set to avoid update checks.
    //! Other things will be compared to the current state and used for potential updates.
    struct render_state
    {
        bool changed = false; //!< True, if the system should check for changes, else false.
        //! \brief The \a clear_color of the \a render_state.
        clear_color color_clear = { 0.0f, 0.0f, 0.0f, 1.0f };
        //! \brief The \a depth_state of the \a render_state.
        depth_state depth = depth_less;
        //! \brief The \a cull_state of the \a render_state.
        cull_state cull = cull_backface;
        //! \brief The \a wireframe_state of the \a render_state.
        wireframe_state wireframe = wireframe_off;
        //! \brief The \a blend_state of the \a render_state.
        blend_state blending = blend_off;
    };

    //! \brief The data for a \a render_command of \a render_command_type::draw_call
    struct draw_call_data
    {
        //! \brief The changes in the \a render_state if there are any.
        render_state state;
        //! \brief The \a gpu_draw_call that should be executed.
        gpu_draw_call gpu_call;
        //! \brief The \a gpu_primitive_type to use for the draw call.
        gpu_primitive_type gpu_primitive;
        //! \brief The number of points to render, when \a gpu_call draws not indexed, else the number of indices.
        uint32 count;
        //! \brief The number of instances to render.
        uint32 instances;

        // TODO Paul: These to arguments are not a good solution.
        //! \brief The component type (optional).
        uint32 component_type = 0x1405; // GL_UNSIGNED_INT
        //! \brief The byte offset (optional).
        uint32 byte_offset = 0;
    };
} // namespace mango

#endif // #define MANGO_RENDER_STRUCTURES_HPP
