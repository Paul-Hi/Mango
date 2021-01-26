//! \file      command_buffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/framebuffer.hpp>
#include <graphics/graphics_state.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/profile.hpp>

using namespace mango;

//! \brief Internal \a graphics_state to limit state changes.
graphics_state m_current_state;

//! \cond NO_COND
void set_viewport(const void* data)
{
    NAMED_PROFILE_ZONE("Set Viewport");
    const set_viewport_command* cmd = static_cast<const set_viewport_command*>(data);
    if (!m_current_state.set_viewport(cmd->x, cmd->y, cmd->width, cmd->height))
        return;
    MANGO_ASSERT(cmd->x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(cmd->y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(cmd->width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(cmd->height >= 0, "Viewport height has to be positive!");

    GL_NAMED_PROFILE_ZONE("Set Viewport");
    glViewport(cmd->x, cmd->y, static_cast<g_sizei>(cmd->width), static_cast<g_sizei>(cmd->height));
}
const execute_function set_viewport_command::execute = &set_viewport;

void set_depth_test(const void* data)
{
    NAMED_PROFILE_ZONE("Set Depth Test");
    const set_depth_test_command* cmd = static_cast<const set_depth_test_command*>(data);
    if (!m_current_state.set_depth_test(cmd->enabled))
        return;
    GL_NAMED_PROFILE_ZONE("Set Depth Test");
    if (cmd->enabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}
const execute_function set_depth_test_command::execute = &set_depth_test;

void set_depth_write(const void* data)
{
    NAMED_PROFILE_ZONE("Set Depth Test");
    const set_depth_write_command* cmd = static_cast<const set_depth_write_command*>(data);
    if (!m_current_state.set_depth_write(cmd->enabled))
        return;
    GL_NAMED_PROFILE_ZONE("Set Depth Write");
    if (cmd->enabled)
    {
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDepthMask(GL_FALSE);
    }
}
const execute_function set_depth_write_command::execute = &set_depth_write;

void set_depth_func(const void* data)
{
    NAMED_PROFILE_ZONE("Set Depth Func");
    const set_depth_func_command* cmd = static_cast<const set_depth_func_command*>(data);
    if (!m_current_state.set_depth_func(cmd->operation))
        return;
    GL_NAMED_PROFILE_ZONE("Set Depth Func");
    glDepthFunc(compare_operation_to_gl(cmd->operation));
}
const execute_function set_depth_func_command::execute = &set_depth_func;

void set_polygon_mode(const void* data)
{
    NAMED_PROFILE_ZONE("Set Polygon Mode");
    const set_polygon_mode_command* cmd = static_cast<const set_polygon_mode_command*>(data);
    if (!m_current_state.set_polygon_mode(cmd->face, cmd->mode))
        return;
    GL_NAMED_PROFILE_ZONE("Set Polygon Mode");
    glPolygonMode(polygon_face_to_gl(cmd->face), polygon_mode_to_gl(cmd->mode));
}
const execute_function set_polygon_mode_command::execute = &set_polygon_mode;

void bind_vertex_array(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Vertex Array");
    const bind_vertex_array_command* cmd = static_cast<const bind_vertex_array_command*>(data);
    if (!m_current_state.bind_vertex_array(cmd->vertex_array_name))
        return;
    GL_NAMED_PROFILE_ZONE("Bind Vertex Array");
    glBindVertexArray(cmd->vertex_array_name);
}
const execute_function bind_vertex_array_command::execute = &bind_vertex_array;

void bind_shader_program(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Shader Program");
    const bind_shader_program_command* cmd = static_cast<const bind_shader_program_command*>(data);
    if (!m_current_state.bind_shader_program(cmd->shader_program_name))
        return;
    GL_NAMED_PROFILE_ZONE("Bind Shader Program");
    glUseProgram(cmd->shader_program_name);
}
const execute_function bind_shader_program_command::execute = &bind_shader_program;

void bind_single_uniform(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Single Uniform");
    const bind_single_uniform_command* cmd = static_cast<const bind_single_uniform_command*>(data);
    MANGO_ASSERT(cmd->location >= 0, "Uniform location has to be greater than 0!");

    GL_NAMED_PROFILE_ZONE("Bind Single Uniform");
    switch (cmd->type)
    {
    case shader_resource_type::fsingle:
    {
        glUniform1f(cmd->location, *static_cast<float*>(cmd->uniform_value));
        return;
    }
    case shader_resource_type::fvec2:
    {
        float* vec = static_cast<g_float*>(cmd->uniform_value);
        glUniform2f(cmd->location, vec[0], vec[1]);
        return;
    }
    case shader_resource_type::fvec3:
    {
        float* vec = static_cast<g_float*>(cmd->uniform_value);
        glUniform3f(cmd->location, vec[0], vec[1], vec[2]);
        return;
    }
    case shader_resource_type::fvec4:
    {
        float* vec = static_cast<g_float*>(cmd->uniform_value);
        glUniform4f(cmd->location, vec[0], vec[1], vec[2], vec[3]);
        return;
    }
    case shader_resource_type::isingle:
    {
        glUniform1i(cmd->location, *static_cast<g_int*>(cmd->uniform_value));
        return;
    }
    case shader_resource_type::ivec2:
    {
        int32* vec = static_cast<g_int*>(cmd->uniform_value);
        glUniform2i(cmd->location, vec[0], vec[1]);
        return;
    }
    case shader_resource_type::ivec3:
    {
        int32* vec = static_cast<g_int*>(cmd->uniform_value);
        glUniform3i(cmd->location, vec[0], vec[1], vec[2]);
        return;
    }
    case shader_resource_type::ivec4:
    {
        int32* vec = static_cast<g_int*>(cmd->uniform_value);
        glUniform4i(cmd->location, vec[0], vec[1], vec[2], vec[3]);
        return;
    }
    case shader_resource_type::mat3:
    {
        glUniformMatrix3fv(cmd->location, cmd->count, GL_FALSE, static_cast<g_float*>(cmd->uniform_value));
        return;
    }
    case shader_resource_type::mat4:
    {
        glUniformMatrix4fv(cmd->location, cmd->count, GL_FALSE, static_cast<g_float*>(cmd->uniform_value));
        return;
    }
    case shader_resource_type::bsingle:
    {
        glUniform1i(cmd->location, *static_cast<g_bool*>(cmd->uniform_value));
        return;
    }
    default:
        MANGO_LOG_ERROR("Unknown uniform type!");
    }
}
const execute_function bind_single_uniform_command::execute = &bind_single_uniform;

void bind_buffer(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Buffer");
    const bind_buffer_command* cmd = static_cast<const bind_buffer_command*>(data);
    if (!m_current_state.bind_buffer(cmd->buffer_name, cmd->index, cmd->offset))
        return;

    g_enum gl_target = buffer_target_to_gl(cmd->target);

    MANGO_ASSERT(cmd->index >= 0, "Cannot bind buffer with negative index!");
    MANGO_ASSERT(cmd->offset >= 0, "Can not bind data outside the buffer! Negative offset!");
    MANGO_ASSERT(cmd->size > 0, "Negative size is not possible!");

    GL_NAMED_PROFILE_ZONE("Bind Buffer");
    glBindBufferRange(gl_target, static_cast<g_uint>(cmd->index), cmd->buffer_name, static_cast<g_intptr>(cmd->offset), static_cast<g_sizeiptr>(cmd->size));
}
const execute_function bind_buffer_command::execute = &bind_buffer;

void bind_texture(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Texture");
    const bind_texture_command* cmd = static_cast<const bind_texture_command*>(data);
    if (!m_current_state.bind_texture(cmd->binding, cmd->texture_name))
        return;

    MANGO_ASSERT(cmd->sampler_location >= 0, "Texture sampler location has to be greater than 0!");
    MANGO_ASSERT(cmd->binding >= 0, "Texture binding has to be greater than 0!");

    GL_NAMED_PROFILE_ZONE("Bind Texture");
    glBindTextureUnit(cmd->binding, cmd->texture_name);
    glUniform1i(cmd->sampler_location, cmd->binding);
}
const execute_function bind_texture_command::execute = &bind_texture;

void bind_image_texture(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Image Texture");
    const bind_image_texture_command* cmd = static_cast<const bind_image_texture_command*>(data);
    MANGO_ASSERT(cmd->binding >= 0, "Image texture binding has to be greater than 0!");
    MANGO_ASSERT(cmd->level >= 0, "Image texture level has to be greater than 0!");
    MANGO_ASSERT(cmd->layer >= 0, "Image texture layer has to be greater than 0!");
    GL_NAMED_PROFILE_ZONE("Bind Image Texture");
    glBindImageTexture(cmd->binding, cmd->texture_name, cmd->level, cmd->layered, cmd->layer, base_access_to_gl(cmd->access), static_cast<g_enum>(cmd->element_format));
}
const execute_function bind_image_texture_command::execute = &bind_image_texture;

void bind_framebuffer(const void* data)
{
    NAMED_PROFILE_ZONE("Bind Framebuffer");
    const bind_framebuffer_command* cmd = static_cast<const bind_framebuffer_command*>(data);
    if (!m_current_state.bind_framebuffer(cmd->framebuffer_name))
        return;
    GL_NAMED_PROFILE_ZONE("Bind Framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, cmd->framebuffer_name);
}
const execute_function bind_framebuffer_command::execute = &bind_framebuffer;

void add_memory_barrier(const void* data)
{
    NAMED_PROFILE_ZONE("Add Memory Barrier");
    const add_memory_barrier_command* cmd = static_cast<const add_memory_barrier_command*>(data);
    GL_NAMED_PROFILE_ZONE("Add Memory Barrier");
    glMemoryBarrier(memory_barrier_bit_to_gl(cmd->barrier_bit));
}
const execute_function add_memory_barrier_command::execute = &add_memory_barrier;

void fence_sync(const void* data)
{
    NAMED_PROFILE_ZONE("Fence Sync");
    const fence_sync_command* cmd = static_cast<const fence_sync_command*>(data);
    GL_NAMED_PROFILE_ZONE("Fence Sync");
    if (glIsSync(*(cmd->sync)))
        glDeleteSync(*(cmd->sync));
    *(cmd->sync) = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
const execute_function fence_sync_command::execute = &fence_sync;

void client_wait_sync(const void* data)
{
    NAMED_PROFILE_ZONE("Client Wait Sync");
    const client_wait_sync_command* cmd = static_cast<const client_wait_sync_command*>(data);
    GL_NAMED_PROFILE_ZONE("Client Wait Sync");
    if (!glIsSync(*(cmd->sync)))
        return;
    int32 waiting_time = 1;
    g_enum wait_return = glClientWaitSync(*(cmd->sync), GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    while (wait_return != GL_ALREADY_SIGNALED && wait_return != GL_CONDITION_SATISFIED)
    {
        wait_return = glClientWaitSync(*(cmd->sync), GL_SYNC_FLUSH_COMMANDS_BIT, waiting_time);
        MANGO_LOG_DEBUG("Waited {0} ns.", waiting_time);
    }
}
const execute_function client_wait_sync_command::execute = &client_wait_sync;

void end_frame(const void*)
{
    NAMED_PROFILE_ZONE("End Frame");
    m_current_state.end_frame();
}
const execute_function end_frame_command::execute = &end_frame;

void calculate_mipmaps(const void* data)
{
    NAMED_PROFILE_ZONE("Calculate Mipmaps");
    const calculate_mipmaps_command* cmd = static_cast<const calculate_mipmaps_command*>(data);
    GL_NAMED_PROFILE_ZONE("Calculate Mipmaps");
    glGenerateTextureMipmap(cmd->texture_name);
}
const execute_function calculate_mipmaps_command::execute = &calculate_mipmaps;

void clear_framebuffer(const void* data)
{
    NAMED_PROFILE_ZONE("Clear Framebuffer");
    const clear_framebuffer_command* cmd = static_cast<const clear_framebuffer_command*>(data);
    GL_NAMED_PROFILE_ZONE("Clear Framebuffer");
    // TODO Paul: Check if these clear functions do always clear correct *fv, *uiv ..... etc.
    // We asume that the mask is correct and all attachments to clear are there.
    if ((cmd->buffer_mask & clear_buffer_mask::color_buffer) != clear_buffer_mask::none)
    {
        const float rgba[4] = { cmd->r, cmd->g, cmd->b, cmd->a };
        for (int32 i = 0; i < 4; ++i)
        {
            if ((cmd->fb_attachment_mask & static_cast<attachment_mask>(1 << i)) != attachment_mask::none)
            {
                glClearNamedFramebufferfv(cmd->framebuffer_name, GL_COLOR, i, rgba);
            }
        }
    }
    if ((cmd->buffer_mask & clear_buffer_mask::depth_buffer) != clear_buffer_mask::none)
    {
        if ((cmd->fb_attachment_mask & attachment_mask::depth_buffer) != attachment_mask::none)
        {
            glClearNamedFramebufferfv(cmd->framebuffer_name, GL_DEPTH, 0, &cmd->depth);
        }
    }
    if ((cmd->buffer_mask & clear_buffer_mask::stencil_buffer) != clear_buffer_mask::none)
    {
        if ((cmd->fb_attachment_mask & attachment_mask::stencil_buffer) != attachment_mask::none)
        {
            glClearNamedFramebufferiv(cmd->framebuffer_name, GL_STENCIL, 0, &cmd->stencil);
        }
    }
    if ((cmd->buffer_mask & clear_buffer_mask::depth_stencil_buffer) != clear_buffer_mask::none)
    {
        if ((cmd->fb_attachment_mask & attachment_mask::depth_stencil_buffer) != attachment_mask::none)
        {
            glClearNamedFramebufferfi(cmd->framebuffer_name, GL_DEPTH_STENCIL, 0, cmd->depth, cmd->stencil);
        }
    }
}
const execute_function clear_framebuffer_command::execute = &clear_framebuffer;

void draw_arrays(const void* data)
{
    NAMED_PROFILE_ZONE("Draw Arrays (Instanced)");
    const draw_arrays_command* cmd = static_cast<const draw_arrays_command*>(data);
    MANGO_ASSERT(cmd->first >= 0, "The first index has to be greater than 0!");
    MANGO_ASSERT(cmd->count >= 0, "The vertex count has to be greater than 0!");
    MANGO_ASSERT(cmd->instance_count >= 0, "The instance count has to be greater than 0!");

    if (cmd->instance_count > 1)
    {
        GL_NAMED_PROFILE_ZONE("Draw Arrays Instanced");
        glDrawArraysInstanced(static_cast<g_enum>(cmd->topology), cmd->first, static_cast<g_sizei>(cmd->count), static_cast<g_sizei>(cmd->instance_count));
    }
    else
    {
        GL_NAMED_PROFILE_ZONE("Draw Arrays");
        glDrawArrays(static_cast<g_enum>(cmd->topology), cmd->first, static_cast<g_sizei>(cmd->count));
    }
}
const execute_function draw_arrays_command::execute = &draw_arrays;

void draw_elements(const void* data)
{
    NAMED_PROFILE_ZONE("Draw Elements (Instanced)");
    const draw_elements_command* cmd = static_cast<const draw_elements_command*>(data);
    MANGO_ASSERT(cmd->first >= 0, "The first index has to be greater than 0!");
    MANGO_ASSERT(cmd->count >= 0, "The vertex count has to be greater than 0!");
    MANGO_ASSERT(cmd->instance_count >= 0, "The instance count has to be greater than 0!");

    if (cmd->instance_count > 1)
    {
        GL_NAMED_PROFILE_ZONE("Draw Elements Instanced");
        glDrawElementsInstanced(static_cast<g_enum>(cmd->topology), static_cast<g_sizei>(cmd->count), static_cast<g_enum>(cmd->type), (g_byte*)NULL + cmd->first,
                                static_cast<g_sizei>(cmd->instance_count));
    }
    else
    {
        GL_NAMED_PROFILE_ZONE("Draw Elements");
        glDrawElements(static_cast<g_enum>(cmd->topology), static_cast<g_sizei>(cmd->count), static_cast<g_enum>(cmd->type), (g_byte*)NULL + cmd->first);
    }
}
const execute_function draw_elements_command::execute = &draw_elements;

void dispatch_compute(const void* data)
{
    NAMED_PROFILE_ZONE("Dispatch Compute");
    const dispatch_compute_command* cmd = static_cast<const dispatch_compute_command*>(data);

    MANGO_ASSERT(cmd->num_x_groups >= 0, "The number of groups (x) has to be greater than 0!");
    MANGO_ASSERT(cmd->num_y_groups >= 0, "The number of groups (y) has to be greater than 0!");
    MANGO_ASSERT(cmd->num_z_groups >= 0, "The number of groups (z) has to be greater than 0!");

    GL_NAMED_PROFILE_ZONE("Dispatch Compute");
    glDispatchCompute(static_cast<g_uint>(cmd->num_x_groups), static_cast<g_uint>(cmd->num_y_groups), static_cast<g_uint>(cmd->num_z_groups));
}
const execute_function dispatch_compute_command::execute = &dispatch_compute;

void set_face_culling(const void* data)
{
    NAMED_PROFILE_ZONE("Set Face Culling");
    const set_face_culling_command* cmd = static_cast<const set_face_culling_command*>(data);
    if (!m_current_state.set_face_culling(cmd->enabled))
        return;
    GL_NAMED_PROFILE_ZONE("Set Face Culling");
    if (cmd->enabled)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}
const execute_function set_face_culling_command::execute = &set_face_culling;

void set_cull_face(const void* data)
{
    NAMED_PROFILE_ZONE("Set Cull Face");
    const set_cull_face_command* cmd = static_cast<const set_cull_face_command*>(data);
    if (!m_current_state.set_cull_face(cmd->face))
        return;
    GL_NAMED_PROFILE_ZONE("Set Cull Face");
    glCullFace(polygon_face_to_gl(cmd->face));
}
const execute_function set_cull_face_command::execute = &set_cull_face;

void set_blending(const void* data)
{
    NAMED_PROFILE_ZONE("Set Blending");
    const set_blending_command* cmd = static_cast<const set_blending_command*>(data);
    if (!m_current_state.set_blending(cmd->enabled))
        return;
    GL_NAMED_PROFILE_ZONE("Set Blending");
    if (cmd->enabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}
const execute_function set_blending_command::execute = &set_blending;

void set_blend_factors(const void* data)
{
    NAMED_PROFILE_ZONE("Set Blend Factors");
    const set_blend_factors_command* cmd = static_cast<const set_blend_factors_command*>(data);
    if (!m_current_state.set_blend_factors(cmd->source, cmd->destination))
        return;
    GL_NAMED_PROFILE_ZONE("Set Blend Factors");
    glBlendFunc(blend_factor_to_gl(cmd->source), blend_factor_to_gl(cmd->destination));
}
const execute_function set_blend_factors_command::execute = &set_blend_factors;

void set_polygon_offset(const void* data)
{
    NAMED_PROFILE_ZONE("Set Polygon Offset");
    const set_polygon_offset_command* cmd = static_cast<const set_polygon_offset_command*>(data);
    if (!m_current_state.set_polygon_offset(cmd->factor, cmd->units))
        return;

    GL_NAMED_PROFILE_ZONE("Set Polygon Offset");
    if (cmd->units > 1e-5)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(cmd->factor, cmd->units);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}
const execute_function set_polygon_offset_command::execute = &set_polygon_offset;

//! \endcond
