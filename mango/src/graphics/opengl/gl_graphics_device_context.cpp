//! \file      gl_graphics_device_context.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/opengl/gl_graphics_device_context.hpp>
#include <graphics/opengl/gl_graphics_resources.hpp>
#define GLFW_INCLUDE_NONE // Do not include gl headers, will be done by ourselfs later on.
#include <GLFW/glfw3.h>
#include <mango/profile.hpp>

using namespace mango;

gl_graphics_device_context::gl_graphics_device_context(display_impl::native_window_handle display_window_handle, gfx_handle<gl_graphics_state> shared_state,
                                                       gfx_handle<gl_shader_program_cache> shader_program_cache, gfx_handle<gl_framebuffer_cache> framebuffer_cache,
                                                       gfx_handle<gl_vertex_array_cache> vertex_array_cache)
    : m_display_window_handle(display_window_handle)
    , m_shared_graphics_state(shared_state)
    , m_shader_program_cache(shader_program_cache)
    , m_framebuffer_cache(framebuffer_cache)
    , m_vertex_array_cache(vertex_array_cache)
    , recording(false)
    , submitted(false)
{
}

gl_graphics_device_context::~gl_graphics_device_context()
{
    m_shared_graphics_state->internal.framebuffer_name  = -1;
    m_shared_graphics_state->internal.vertex_array_name = -1;
    // TODO
}

void gl_graphics_device_context::begin()
{
    // TODO Paul: Should we reset all state here?
    submitted = false;
    recording = true;
}

void gl_graphics_device_context::make_current()
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_display_window_handle, "Native window handle is invalid! Can not make context current!");
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(m_display_window_handle));
}

void gl_graphics_device_context::set_swap_interval(int32 swap)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    glfwSwapInterval(swap);
}

void gl_graphics_device_context::set_buffer_data(gfx_handle<const gfx_buffer> buffer_handle, int32 offset, int32 size, void* data)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_buffer>(buffer_handle), "buffer is not a gl_buffer");

    gfx_handle<const gl_buffer> buf = static_gfx_handle_cast<const gl_buffer>(buffer_handle);

    MANGO_ASSERT(offset + size <= buf->m_info.size, "Buffer access out of bounds!");
    MANGO_ASSERT((buf->m_info.buffer_access & gfx_buffer_access::buffer_access_dynamic_storage) != gfx_buffer_access::buffer_access_none, "Buffer access violation!");

    glNamedBufferSubData(buf->m_buffer_gl_handle, offset, size, data);

    // Invalidating the buffer is not required!
}

void* gl_graphics_device_context::map_buffer_data(gfx_handle<const gfx_buffer> buffer_handle, int32 offset, int32 size)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return nullptr;
    }

    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_buffer>(buffer_handle), "buffer is not a gl_buffer");

    gfx_handle<const gl_buffer> buf = static_gfx_handle_cast<const gl_buffer>(buffer_handle);

    MANGO_ASSERT(offset + size <= buf->m_info.size, "Buffer access out of bounds!");
    MANGO_ASSERT((buf->m_info.buffer_access & gfx_buffer_access::buffer_access_mapped_access_read_write) != gfx_buffer_access::buffer_access_none, "Buffer access violation!");

    // TODO Paul: Check if that persistent and coherent stuff is correct.
    return glMapNamedBufferRange(buf->m_buffer_gl_handle, offset, size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void gl_graphics_device_context::set_texture_data(gfx_handle<const gfx_texture> texture_handle, const texture_set_description& desc, void* data)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(texture_handle), "texture is not a gl_texture");

    gfx_handle<const gl_texture> tex = static_gfx_handle_cast<const gl_texture>(texture_handle);

    MANGO_ASSERT(desc.x_offset <= tex->m_info.width, "Texture access out of bounds!");
    MANGO_ASSERT(desc.y_offset <= tex->m_info.height, "Texture access out of bounds!");
    MANGO_ASSERT(desc.z_offset <= tex->m_info.array_layers, "Texture access out of bounds!");
    MANGO_ASSERT(desc.width >= 0, "Can not set negative data width!");
    MANGO_ASSERT(desc.height >= 0, "Can not set negative data height!");
    MANGO_ASSERT(desc.depth >= 0, "Can not set negative data depth!");
    MANGO_ASSERT(desc.level <= tex->m_info.miplevels, "Texture access out of bounds!");

    gl_enum pixel_format   = gfx_format_to_gl(desc.pixel_format);
    gl_enum component_type = gfx_format_to_gl(desc.component_type);

    if (tex->m_info.array_layers > 1)
    {
        glTextureSubImage3D(tex->m_texture_gl_handle, desc.level, desc.x_offset, desc.y_offset, desc.z_offset, desc.width, desc.height, desc.depth, pixel_format, component_type, data);
    }
    else if (tex->m_info.texture_type == gfx_texture_type::texture_type_cube_map)
    {
        for (int32 i = 0; i < 6; ++i)
            glTextureSubImage3D(tex->m_texture_gl_handle, desc.level, 0, 0, i, desc.width, desc.height, desc.depth, pixel_format, component_type, data); // TODO Paul: Is this correct?
    }
    else
    {
        glTextureSubImage2D(tex->m_texture_gl_handle, desc.level, desc.x_offset, desc.y_offset, desc.width, desc.height, pixel_format, component_type, data);
    }

    // Invalidating the texture is not required!
}

void gl_graphics_device_context::set_viewport(int32 first, int32 count, const gfx_viewport* viewports)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_viewport) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Viewport is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    // Viewport to ouput can be specified in the geometry shader. Default selection is viewport 0.
    glViewportArrayv(first, count, &viewports[0].x);

    // Update the graphics state.
    std::copy(viewports, viewports + count, m_shared_graphics_state->dynamic_state_cache.viewports + first);
}

void gl_graphics_device_context::set_scissor(int32 first, int32 count, const gfx_scissor_rectangle* scissors)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_scissor) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Scissor is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    // Scissor are selected with the viewport in the geometry shader. Default selection is scissor 0.
    glScissorArrayv(first, count, &scissors[0].x_offset);

    // Update the graphics state.
    std::copy(scissors, scissors + count, m_shared_graphics_state->dynamic_state_cache.scissors + first);
}

void gl_graphics_device_context::set_line_width(float width)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_line_width) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Line Width is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    glLineWidth(width);

    // Update the graphics state.
    m_shared_graphics_state->dynamic_state_cache.line_width = width;
}

void gl_graphics_device_context::set_depth_bias(float constant_factor, float clamp, float slope_factor)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_depth_bias) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Depth Bias is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    // Not exactly obvoius if glPolygonOffsetClamp is somehow supported.
    // glPolygonOffsetClamp(slope_factor, constant_factor, clamp);
    MANGO_LOG_WARN("Clamping the depth bias is not supported in OpenGL (yet?)!");

    glPolygonOffset(slope_factor, constant_factor);

    // Update the graphics state.
    m_shared_graphics_state->dynamic_state_cache.depth.constant_bias = constant_factor;
    m_shared_graphics_state->dynamic_state_cache.depth.bias_clamp    = clamp;
    m_shared_graphics_state->dynamic_state_cache.depth.slope_bias    = slope_factor;
}

void gl_graphics_device_context::set_blend_constants(const float constants[4])
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_blend_constants) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Blend Constants is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    glBlendColor(constants[0], constants[1], constants[2], constants[3]);

    // Update the graphics state.
    m_shared_graphics_state->dynamic_state_cache.blend_constants[0] = constants[0];
    m_shared_graphics_state->dynamic_state_cache.blend_constants[1] = constants[1];
    m_shared_graphics_state->dynamic_state_cache.blend_constants[2] = constants[2];
    m_shared_graphics_state->dynamic_state_cache.blend_constants[3] = constants[3];
}

void gl_graphics_device_context::set_stencil_compare_mask_and_reference(gfx_stencil_face_flag_bits face_mask, uint32 compare_mask, uint32 reference)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_stencil_compare_mask_reference) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Stencil Compare Mask and Stencil Reference is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    // Get from pipeline.
    // TODO Paul: Is default front okay?
    gl_enum func = gfx_compare_operator_to_gl(info.depth_stencil_state.front.compare_operator);

    if ((face_mask & gfx_stencil_face_flag_bits::stencil_face_front_and_back_bit) == gfx_stencil_face_flag_bits::stencil_face_front_and_back_bit)
        glStencilFunc(func, reference, compare_mask);
    else if ((face_mask & gfx_stencil_face_flag_bits::stencil_face_front_bit) != gfx_stencil_face_flag_bits::stencil_face_none)
        glStencilFuncSeparate(GL_FRONT, func, reference, compare_mask);
    else if ((face_mask & gfx_stencil_face_flag_bits::stencil_face_back_bit) != gfx_stencil_face_flag_bits::stencil_face_none)
    {
        func = gfx_compare_operator_to_gl(info.depth_stencil_state.back.compare_operator);
        glStencilFuncSeparate(GL_BACK, func, reference, compare_mask);
    }

    // Update the graphics state.
    m_shared_graphics_state->dynamic_state_cache.stencil.compare_face_mask   = face_mask;
    m_shared_graphics_state->dynamic_state_cache.stencil.reference_face_mask = face_mask;
    m_shared_graphics_state->dynamic_state_cache.stencil.compare_mask        = compare_mask;
    m_shared_graphics_state->dynamic_state_cache.stencil.reference           = reference;
}

void gl_graphics_device_context::set_stencil_write_mask(gfx_stencil_face_flag_bits face_mask, uint32 write_mask)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");
    graphics_pipeline_create_info info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if ((info.dynamic_state.dynamic_states & gfx_dynamic_state_flag_bits::dynamic_state_stencil_write_mask) == gfx_dynamic_state_flag_bits::dynamic_state_none)
    {
        MANGO_LOG_WARN("Stencil Write Mask is not flagged as dynamic in the pipeline and setting it will be ignored!");
        return;
    }

    if ((face_mask & gfx_stencil_face_flag_bits::stencil_face_front_and_back_bit) == gfx_stencil_face_flag_bits::stencil_face_front_and_back_bit)
        glStencilMask(write_mask);
    else if ((face_mask & gfx_stencil_face_flag_bits::stencil_face_front_bit) != gfx_stencil_face_flag_bits::stencil_face_none)
        glStencilMaskSeparate(GL_FRONT, write_mask);
    else if ((face_mask & gfx_stencil_face_flag_bits::stencil_face_back_bit) != gfx_stencil_face_flag_bits::stencil_face_none)
        glStencilMaskSeparate(GL_BACK, write_mask);

    // Update the graphics state.
    m_shared_graphics_state->dynamic_state_cache.stencil.write_face_mask = face_mask;
    m_shared_graphics_state->dynamic_state_cache.stencil.write_mask      = write_mask;
}

void gl_graphics_device_context::set_render_targets(int32 count, gfx_handle<const gfx_texture>* render_targets, gfx_handle<const gfx_texture> depth_stencil_target)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    if (depth_stencil_target)
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(depth_stencil_target), "Depth Stencil target is not a gl_texture!");

    bool default_framebuffer = false;
    gl_handle framebuffer    = 0;
    if (count == 1)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(render_targets[0]), "Render target is not a gl_texture!");
        gfx_handle<const gl_texture> rt = static_gfx_handle_cast<const gl_texture>(render_targets[0]);
        if (rt->m_texture_gl_handle == 0)
        {
            MANGO_ASSERT(depth_stencil_target && (static_gfx_handle_cast<const gl_texture>(depth_stencil_target)->m_texture_gl_handle == 0),
                         "Default framebuffer can not use another texture as depth buffer!");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            default_framebuffer = true;
            framebuffer         = 0;
        }
    }

    if (!default_framebuffer)
    {
        framebuffer = m_framebuffer_cache->get_framebuffer(count, render_targets, depth_stencil_target);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    }

    // Update the graphics state.
    MANGO_ASSERT(count < 8, "Too many color targets!"); // TODO Paul: Query max attachments.
    m_shared_graphics_state->internal.framebuffer_name = framebuffer;
    m_shared_graphics_state->color_target_count        = count;
    for (int32 i = 0; i < count; ++i)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(render_targets[i]), "Texture is not a gl_texture!");
        auto tex                                       = static_gfx_handle_cast<const gl_texture>(render_targets[i]);
        m_shared_graphics_state->set_render_targets[i] = tex;
    }

    if (depth_stencil_target != nullptr)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(depth_stencil_target), "Texture is not a gl_texture!");
        auto tex = static_gfx_handle_cast<const gl_texture>(depth_stencil_target);

        m_shared_graphics_state->set_render_targets[count] = tex;

        const gfx_format& internal = tex->m_info.texture_format;

        // TODO Paul: Pure stencil not supported.

        switch (internal)
        {
        case gfx_format::depth24_stencil8:
        case gfx_format::depth32f_stencil8:
            m_shared_graphics_state->depth_stencil_target_count++;
            break;
        case gfx_format::depth_component32f:
        case gfx_format::depth_component16:
        case gfx_format::depth_component24:
        case gfx_format::depth_component32:
            m_shared_graphics_state->depth_target_count++;
            break;
        default:
            MANGO_ASSERT(false, "Depth Stencil Target has no valid format!");
            break;
        }
    }
}

void gl_graphics_device_context::calculate_mipmaps(gfx_handle<const gfx_texture> texture_handle)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(texture_handle, "Can not calculate mipmaps for no texture!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(texture_handle), "Texture is not a gl_texture!");

    gfx_handle<const gl_texture> tex = static_gfx_handle_cast<const gl_texture>(texture_handle);

    if (tex->m_texture_gl_handle == 0)
    {
        MANGO_LOG_ERROR("Can not calculate mipmaps for swap chain texture!");
        return;
    }

    glGenerateTextureMipmap(tex->m_texture_gl_handle);
}

void gl_graphics_device_context::clear_render_target(gfx_clear_attachment_flag_bits color_attachment, float clear_color[4])
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    if (!m_shared_graphics_state->color_target_count)
        return;

    MANGO_ASSERT(m_shared_graphics_state->internal.framebuffer_name >= 0, "No valid framebuffer is bound!");
    gl_handle framebuffer = m_shared_graphics_state->internal.framebuffer_name;

    // We asume that the mask is correct and all attachments to clear are there.
    if ((color_attachment & gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers) == gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers)
    {
        for (int32 i = 0; i < m_shared_graphics_state->color_target_count; ++i) // TODO Paul: Query max attachments.
        {
            glClearNamedFramebufferfv(framebuffer, GL_COLOR, i, clear_color);
        }
    }
    else
    {
        for (uint8 i = 0; i < m_shared_graphics_state->color_target_count; ++i) // TODO Paul: Query max attachments.
        {
            uint8 shift = (static_cast<uint8>(gfx_clear_attachment_flag_bits::clear_flag_draw_buffer0) << i);
            if ((color_attachment & static_cast<gfx_clear_attachment_flag_bits>(shift)) != gfx_clear_attachment_flag_bits::clear_flag_none)
                glClearNamedFramebufferfv(framebuffer, GL_COLOR, i, clear_color);
        }
    }
}

void gl_graphics_device_context::clear_depth_stencil(gfx_clear_attachment_flag_bits depth_stencil, float clear_depth, int32 clear_stencil)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    if (!(m_shared_graphics_state->depth_target_count + m_shared_graphics_state->stencil_target_count + m_shared_graphics_state->depth_stencil_target_count))
        return;

    gl_handle framebuffer = m_shared_graphics_state->internal.framebuffer_name;

    // TODO Paul: Check if these clear functions do always clear correct *fv, *uiv ..... etc.
    // We asume that the mask is correct and all attachments to clear are there.
    if ((depth_stencil & gfx_clear_attachment_flag_bits::clear_flag_depth_buffer) != gfx_clear_attachment_flag_bits::clear_flag_none)
        glClearNamedFramebufferfv(framebuffer, GL_DEPTH, 0, &clear_depth);

    if ((depth_stencil & gfx_clear_attachment_flag_bits::clear_flag_stencil_buffer) != gfx_clear_attachment_flag_bits::clear_flag_none)
        glClearNamedFramebufferiv(framebuffer, GL_STENCIL, 0, &clear_stencil);

    if ((depth_stencil & gfx_clear_attachment_flag_bits::clear_flag_depth_stencil_buffer) == gfx_clear_attachment_flag_bits::clear_flag_depth_stencil_buffer)
        glClearNamedFramebufferfi(framebuffer, GL_DEPTH_STENCIL, 0, clear_depth, clear_stencil);
}

void gl_graphics_device_context::set_vertex_buffers(int32 count, gfx_handle<const gfx_buffer>* buffers, int32* bindings, int32* offsets)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    // Creation of vertex arrays will be done later before drawing since we may also need an index buffer.

    // Update the graphics state.
    MANGO_ASSERT(count < 16, "Too many vertex buffer bindings!"); // TODO Paul: Query max vertex buffers. GL_MAX_VERTEX_ATTRIB_BINDINGS
    m_shared_graphics_state->vertex_buffer_count = count;
    for (int32 i = 0; i < count; ++i)
    {
        m_shared_graphics_state->set_vertex_buffers[i] = { buffers[i], bindings[i], offsets[i] };
    }

    m_shared_graphics_state->internal.vertex_array_name = -1; // Invalidates.
}

void gl_graphics_device_context::set_index_buffer(gfx_handle<const gfx_buffer> buffer_handle, gfx_format index_type)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    // Creation of vertex arrays will be done later before drawing since we also need vertex buffers.

    // Update the graphics state.
    m_shared_graphics_state->set_index_buffer = buffer_handle;
    m_shared_graphics_state->index_type       = index_type;

    m_shared_graphics_state->internal.vertex_array_name = -1; // Invalidates.
}

void gl_graphics_device_context::bind_pipeline(gfx_handle<const gfx_pipeline> pipeline_handle)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    if (!pipeline_handle)
    {
        glUseProgram(0);
        return;
    }

    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_pipeline>(pipeline_handle), "Pipeline is not a gl_pipeline!");
    gfx_handle<const gl_graphics_pipeline> graphics_pipeline = std::dynamic_pointer_cast<const gl_graphics_pipeline>(pipeline_handle);
    gfx_handle<const gl_compute_pipeline> compute_pipeline   = std::dynamic_pointer_cast<const gl_compute_pipeline>(pipeline_handle);
    MANGO_ASSERT(graphics_pipeline || compute_pipeline, "Pipeline is not of a valid type!");
    if (graphics_pipeline)
    {
        const graphics_pipeline_create_info& info = graphics_pipeline->m_info;

        // Shader
        gl_handle shader_program = m_shader_program_cache->get_shader_program(info.shader_stage_descriptor);
        glUseProgram(shader_program);

        // Input State has to be done, when set_vertex_buffers is called since we specify settings for the vertex arrays.
        // TODO Paul: Check if we could use the information to preorder the possible vertex arrays to save time later on.
        // Input Assembly is required on (indexed) draw calls and can not be set here.

        // Viewport State
        // Viewport to ouput can be specified in the geometry shader. Default selection is viewport 0.
        glViewportArrayv(0, info.viewport_state.viewport_count, &info.viewport_state.viewports[0].x);
        // Scissor are selected with the viewport in the geometry shader. Default selection is scissor 0.
        glScissorArrayv(0, info.viewport_state.scissor_count, &info.viewport_state.scissors[0].x_offset);

        // Raster State
        glPolygonMode(GL_FRONT_AND_BACK, gfx_polygon_mode_to_gl(info.rasterization_state.polygon_mode)); // TODO Paul: Always Front And Back?
        if (info.rasterization_state.cull_mode != gfx_cull_mode_flag_bits::mode_none)                    // TODO Paul: Add descriptor option maybe?
        {
            glEnable(GL_CULL_FACE);
            if ((info.rasterization_state.cull_mode & gfx_cull_mode_flag_bits::mode_front_and_back) == gfx_cull_mode_flag_bits::mode_front_and_back)
                glCullFace(GL_FRONT_AND_BACK);
            else if ((info.rasterization_state.cull_mode & gfx_cull_mode_flag_bits::mode_front) != gfx_cull_mode_flag_bits::mode_none)
                glCullFace(GL_FRONT);
            else if ((info.rasterization_state.cull_mode & gfx_cull_mode_flag_bits::mode_back) != gfx_cull_mode_flag_bits::mode_none)
                glCullFace(GL_BACK);
        }
        else
            glDisable(GL_CULL_FACE);
        glFrontFace(info.rasterization_state.front_face == gfx_front_face::counter_clockwise ? GL_CCW : GL_CW);
        if (info.rasterization_state.enable_depth_bias)
            glPolygonOffset(info.rasterization_state.depth_bias_slope_factor, info.rasterization_state.constant_depth_bias);
        glLineWidth(info.rasterization_state.line_width);

        // Depth Stencil State
        if (info.depth_stencil_state.enable_depth_test)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(gfx_compare_operator_to_gl(info.depth_stencil_state.depth_compare_operator));
        }
        else
            glDisable(GL_DEPTH_TEST);
        glDepthMask(info.depth_stencil_state.enable_depth_write);
        if (info.depth_stencil_state.enable_stencil_test)
        {
            glEnable(GL_STENCIL_TEST);
            gl_enum func  = gfx_compare_operator_to_gl(info.depth_stencil_state.front.compare_operator);
            gl_enum fail  = gfx_stencil_operation_to_gl(info.depth_stencil_state.front.fail_operation);
            gl_enum pass  = gfx_stencil_operation_to_gl(info.depth_stencil_state.front.pass_operation);
            gl_enum depth = gfx_stencil_operation_to_gl(info.depth_stencil_state.front.depth_fail_operation);
            glStencilOpSeparate(GL_FRONT, fail, pass, depth);
            glStencilFuncSeparate(GL_FRONT, func, info.depth_stencil_state.front.reference, info.depth_stencil_state.front.compare_mask);
            glStencilMaskSeparate(GL_FRONT, info.depth_stencil_state.front.write_mask);
            func  = gfx_compare_operator_to_gl(info.depth_stencil_state.back.compare_operator);
            fail  = gfx_stencil_operation_to_gl(info.depth_stencil_state.back.fail_operation);
            pass  = gfx_stencil_operation_to_gl(info.depth_stencil_state.back.pass_operation);
            depth = gfx_stencil_operation_to_gl(info.depth_stencil_state.back.depth_fail_operation);
            glStencilOpSeparate(GL_BACK, fail, pass, depth);
            glStencilFuncSeparate(GL_BACK, func, info.depth_stencil_state.back.reference, info.depth_stencil_state.back.compare_mask);
            glStencilMaskSeparate(GL_BACK, info.depth_stencil_state.back.write_mask);
        }
        else
            glDisable(GL_STENCIL_TEST);

        // Blend State
        if (info.blend_state.enable_logical_operation)
        {
            glEnable(GL_COLOR_LOGIC_OP);
            glLogicOp(gfx_logic_operator_to_gl(info.blend_state.logic_operator));
        }
        else
        {
            glDisable(GL_COLOR_LOGIC_OP);
            // Blending is only enabled, when logic operation is disabled, so we can do that in here.
            if (info.blend_state.blend_description.enable_blend)
            {
                glEnable(GL_BLEND);
                glBlendColor(info.blend_state.blend_constants[0], info.blend_state.blend_constants[1], info.blend_state.blend_constants[2], info.blend_state.blend_constants[3]);
                glBlendEquationSeparate(gfx_blend_operation_to_gl(info.blend_state.blend_description.color_blend_operation),
                                        gfx_blend_operation_to_gl(info.blend_state.blend_description.alpha_blend_operation));
                glBlendFuncSeparate(
                    gfx_blend_factor_to_gl(info.blend_state.blend_description.src_color_blend_factor), gfx_blend_factor_to_gl(info.blend_state.blend_description.dst_color_blend_factor),
                    gfx_blend_factor_to_gl(info.blend_state.blend_description.src_alpha_blend_factor), gfx_blend_factor_to_gl(info.blend_state.blend_description.dst_alpha_blend_factor));
            }
            else
                glDisable(GL_BLEND);
        }
        bool r, g, b, a;
        create_gl_color_mask(info.blend_state.blend_description.color_write_mask, r, g, b, a);
        glColorMask(r, g, b, a);

        // Dynamic state - Nothing to do here.
        // TODO Paul: Check if pipeline does set some dynamic states accidently or breaks while trying to set them.

        // Render Output Description is required when the targets are set.
        m_framebuffer_cache->prepare(info.output_description);
    }
    else if (compute_pipeline)
    {
        const compute_pipeline_create_info& info = compute_pipeline->m_info;

        // Shader
        gl_handle shader_program = m_shader_program_cache->get_shader_program(info.shader_stage_descriptor);
        glUseProgram(shader_program);
    }

    // Update the graphics state.
    m_shared_graphics_state->bound_pipeline               = static_gfx_handle_cast<const gl_pipeline>(pipeline_handle);
    m_shared_graphics_state->pipeline_resources_submitted = false;
}

void gl_graphics_device_context::submit_pipeline_state_resources()
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a gl_pipeline!");

    static_gfx_handle_cast<const gl_pipeline>(m_shared_graphics_state->bound_pipeline)->submit_pipeline_resources(m_shared_graphics_state);

    m_shared_graphics_state->pipeline_resources_submitted = true;
}

void gl_graphics_device_context::draw(int32 vertex_count, int32 index_count, int32 instance_count, int32 base_vertex, int32 base_instance, int32 index_offset)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a graphics pipeline!");

    auto& info = static_gfx_handle_cast<const gl_graphics_pipeline>(m_shared_graphics_state->bound_pipeline)->m_info;

    if (m_shared_graphics_state->internal.vertex_array_name < 0) // Invalid
    {
        vertex_array_data_descriptor desc;
        desc.input_descriptor    = &info.vertex_input_state;
        desc.vertex_count        = vertex_count;
        desc.index_count         = index_count;
        desc.vertex_buffer_count = m_shared_graphics_state->vertex_buffer_count;
        desc.vertex_buffers      = &m_shared_graphics_state->set_vertex_buffers[0];
        desc.index_buffer        = &m_shared_graphics_state->set_index_buffer;
        desc.index_type          = gfx_format::invalid;
        if (index_count > 0)
            desc.index_type = m_shared_graphics_state->index_type;

        gl_enum vertex_array = m_vertex_array_cache->get_vertex_array(desc);

        m_shared_graphics_state->internal.vertex_array_name = vertex_array;
    }
    glBindVertexArray(m_shared_graphics_state->internal.vertex_array_name);

    MANGO_ASSERT(index_count == 0 || m_shared_graphics_state->set_index_buffer, "Indexed drawing without an index buffer bound");
    MANGO_ASSERT(base_vertex >= 0, "The base vertex index has to be greater than 0!");
    MANGO_ASSERT(vertex_count >= 0, "The vertex count has to be greater than 0!");
    MANGO_ASSERT(base_instance >= 0, "The base instance has to be greater than 0!");
    MANGO_ASSERT(instance_count >= 0, "The instance count has to be greater than 0!");
    MANGO_ASSERT(index_offset >= 0, "The offset for the indices has to be greater than 0!");

    const gfx_primitive_topology& topology = info.input_assembly_state.topology;

    if (index_count == 0)
    {
        // draw arrays
        glDrawArraysInstancedBaseInstance(gfx_primitive_topology_to_gl(topology), base_vertex, vertex_count, instance_count, base_instance);
    }
    else
    {
        // draw elements
        gl_enum type = gfx_format_to_gl(m_shared_graphics_state->index_type);
        glDrawElementsInstancedBaseVertexBaseInstance(gfx_primitive_topology_to_gl(topology), index_count, type, (unsigned char*)NULL + index_offset, instance_count, base_vertex, base_instance);
    }
}

void gl_graphics_device_context::dispatch(int32 x, int32 y, int32 z)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_shared_graphics_state->bound_pipeline, "No Pipeline is currently bound!");
    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_compute_pipeline>(m_shared_graphics_state->bound_pipeline), "Pipeline is not a compute pipeline!");

    glDispatchCompute(x, y, z);
}

void gl_graphics_device_context::end()
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    recording = false;
}

void gl_graphics_device_context::barrier(const barrier_description& desc)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    glMemoryBarrier(gfx_barrier_bit_to_gl(desc.barrier_bit));
}

gfx_handle<const gfx_semaphore> gl_graphics_device_context::fence(const semaphore_create_info& info)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return nullptr;
    }

    return make_gfx_handle<const gl_semaphore>(std::forward<const semaphore_create_info&>(info));
}

void gl_graphics_device_context::client_wait(gfx_handle<const gfx_semaphore> semaphore)
{
    PROFILE_ZONE;
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    if (!semaphore)
        return;

    MANGO_ASSERT(std::dynamic_pointer_cast<const gl_semaphore>(semaphore), "Semaphore is not a gl_semaphore!");

    GLsync sync_object = static_cast<GLsync>(static_gfx_handle_cast<const gl_semaphore>(semaphore)->m_semaphore_gl_handle);

    if (!glIsSync(sync_object))
        return;
    int32 waiting_sum   = 0;
    int32 waiting_time  = 1;
    gl_enum wait_return = glClientWaitSync(sync_object, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    while (wait_return != GL_ALREADY_SIGNALED && wait_return != GL_CONDITION_SATISFIED)
    {
        wait_return = glClientWaitSync(sync_object, GL_SYNC_FLUSH_COMMANDS_BIT, waiting_time);
        waiting_sum += waiting_time;
    }
    // if (waiting_sum > 0)
    //    MANGO_LOG_DEBUG("Waited {0} ns.", waiting_time);
}

void gl_graphics_device_context::wait(gfx_handle<const gfx_semaphore> semaphore)
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    GLsync sync_object = static_cast<GLsync>(static_gfx_handle_cast<const gl_semaphore>(semaphore)->m_semaphore_gl_handle);

    if (!glIsSync(sync_object))
        return;
    glWaitSync(sync_object, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(sync_object);
}

void gl_graphics_device_context::present()
{
    if (!recording)
    {
        MANGO_LOG_WARN("Device context is not recording {0}!", __LINE__);
        return;
    }

    MANGO_ASSERT(m_display_window_handle, "Native window handle is invalid! Can not present the frame!");
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_display_window_handle));
    GL_PROFILE_COLLECT;
}

void gl_graphics_device_context::submit()
{
    if (recording)
    {
        MANGO_LOG_WARN("Device context is recording! Call end() before submitting the context!");
        return;
    }

    submitted = true;
}
