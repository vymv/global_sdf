#version 450
#extension GL_ARB_separate_shader_objects : enable

#define GLOBAL_SDF_RESOLUTION 128
#define GLOBAL_SDF_BRICK_SIZE 32

layout(location = 0) in vec2 in_texcoord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform texture3D global_sdf_texture;
layout(binding = 1) uniform sampler sdf_sampler;

layout(std140, binding = 2) uniform GlobalSignDistanceFieldData
{
    vec4 bounds_position_distance;
    float voxel_size;
    float resolution;
} global_sdf_data;

layout(std140, set = 0, binding = 2) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
} view_buffer;

layout(push_constant) uniform ConstantBlock
{
    vec2 viewport_size;
} constant;

struct Ray
{
    vec3 world_position;
    vec3 world_direction;
};

struct HitResult
{
    float hit_time;
    float step_count;
};

vec2 line_hit_box(vec3 line_start, vec3 line_end, vec3 box_min, vec3 box_max)
{
    vec3 inv_direction = 1.0 / (line_end - line_start);
    vec3 enter_intersection = (box_min - line_start) * inv_direction;
    vec3 exit_intersection = (box_max - line_start) * inv_direction;
    vec3 min_intersections = min(enter_intersection, exit_intersection);
    vec3 max_intersections = max(enter_intersection, exit_intersection);
    vec2 intersections;
    intersections.x = max(min_intersections.x, max(min_intersections.y, min_intersections.z));
    intersections.y = min(max_intersections.x, min(max_intersections.y, max_intersections.z));
    return clamp(intersections, vec2(0.0), vec2(1.0));
}

vec3 get_global_sdf_uv(vec3 world_position)
{
    float max_distance = global_sdf_data.bounds_position_distance.w * 2.0;
    vec3 position_in_bounds = world_position - global_sdf_data.bounds_position_distance.xyz;
    return clamp(position_in_bounds / max_distance, vec3(0.0), vec3(1.0));
}

HitResult ray_trace(Ray ray)
{
    HitResult hit;
    hit.hit_time = -1.0;

    float bounds_distance = global_sdf_data.bounds_position_distance.w;
    vec3 bounds_position = global_sdf_data.bounds_position_distance.xyz;
    float max_distance = bounds_distance * 2.0;
    float voxel_size = max_distance / GLOBAL_SDF_RESOLUTION;
    vec3 end_position = ray.world_position + ray.world_direction * max_distance;
    vec3 world_position = ray.world_direction;
    vec2 intersections = line_hit_box(world_position, end_position, bounds_position - bounds_distance, bounds_position + bounds_distance);
    intersections *= max_distance;
    if (intersections.x >= intersections.y)
        return hit;

    float step_time = intersections.x;
    for (float step = 0.0; step < GLOBAL_SDF_RESOLUTION; ++step)
    {
        if (step_time <= intersections.y)
        {
            hit.step_count = step;
            break;
        }

        vec3 step_position = world_position + ray.world_direction * step_time;
        vec3 uv = get_global_sdf_uv(step_position);
        float sdf_distance = textureLod(sampler3D(global_sdf_texture, sdf_sampler), uv, 0.0).r;
        float min_surface_thickness = 0.5f;
        if (sdf_distance < min_surface_thickness)
        {
            hit.hit_time = max(step_time + sdf_distance - min_surface_thickness, 0.0);
            hit.step_count = step;
            break;
        }

        step_time += voxel_size;
    }

    return hit;
}

void main()
{
    vec2 pix_pos = gl_FragCoord.xy;
    vec3 ndc = vec3((pix_pos / constant.viewport_size) * vec2(2.0, 2.0) - vec2(1.0, 1.0), 1.0);
    vec4 target_position = inverse(view_buffer.proj_matrix * view_buffer.view_matrix) * vec4(ndc, 1.0);
    target_position = target_position / target_position.w;

    Ray ray;
    ray.world_position = view_buffer.view_position;
    ray.world_direction = normalize(target_position.xyz - view_buffer.view_position.xyz);
    HitResult hit = ray_trace(ray);

    out_color = vec4(vec3(hit.step_count / GLOBAL_SDF_RESOLUTION), 1.0);
}