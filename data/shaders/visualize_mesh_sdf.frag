#version 450
#extension GL_ARB_separate_shader_objects : enable
#define GLOBAL_SDF_MAX_OBJECT_COUNT 64
layout(location = 0) in vec2 in_texcoord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform texture3D mesh_sdf_textures[GLOBAL_SDF_MAX_OBJECT_COUNT];
layout(binding = 1) uniform sampler sdf_sampler;

layout(std140, binding = 2) buffer MeshSignDistanceFieldData
{
    vec4 bounds_position[GLOBAL_SDF_MAX_OBJECT_COUNT];
    vec4 bounds_distance[GLOBAL_SDF_MAX_OBJECT_COUNT];
    mat4 model_matrix_inv[GLOBAL_SDF_MAX_OBJECT_COUNT];
    float resolution;
    float global_sdf_distance;
    int sdf_count;
}
mesh_sdf_data;

layout(std140, set = 0, binding = 3) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
}
view_buffer;

layout(push_constant) uniform ConstantBlock
{
    vec2 viewport_size;
}
constant;

struct Ray
{
    vec3 world_position;
    vec3 world_direction;
};

struct HitResult
{
    vec3 color;
    float hit_time;
    float step_count;

};

vec2 line_hit_box(vec3 line_start, vec3 line_end, vec3 box_min, vec3 box_max)
{
    vec3 inv_direction = 1.0 / normalize(line_end - line_start);
    vec3 t_min = (box_min - line_start) * inv_direction;
    vec3 t_max = (box_max - line_start) * inv_direction;
    vec3 min_intersections = min(t_min, t_max);
    vec3 max_intersections = max(t_min, t_max);
    vec2 intersections;
    intersections.x = max(min_intersections.x, max(min_intersections.y, min_intersections.z));// 较小的交点
    intersections.y = min(max_intersections.x, min(max_intersections.y, max_intersections.z));// 较大的交点
    return intersections;
}

vec3 get_mesh_sdf_uv(vec3 world_position, int sdf_index)
{
    // 转换到模型坐标
    vec4 model_position = mesh_sdf_data.model_matrix_inv[sdf_index] * vec4(world_position, 1.0);
    // 转换boundingbox到模型坐标
    vec4 bounding_min = mesh_sdf_data.model_matrix_inv[sdf_index] * vec4(mesh_sdf_data.bounds_position[sdf_index].xyz - mesh_sdf_data.bounds_distance[sdf_index].xyz, 1.0);
    vec4 bounding_max = mesh_sdf_data.model_matrix_inv[sdf_index] * vec4(mesh_sdf_data.bounds_position[sdf_index].xyz + mesh_sdf_data.bounds_distance[sdf_index].xyz, 1.0);
    // 转换到uv坐标
    vec3 bounding_size = bounding_max.xyz - bounding_min.xyz;
    vec3 position_in_bounds = (model_position - bounding_min).xyz;
    return clamp(position_in_bounds / bounding_size, vec3(0.0), vec3(1.0));
}

// vec3 get_mesh_sdf_uv_raw(vec3 world_position, int sdf_index)
// {
//     // 转换到模型坐标
//     vec4 model_position = mesh_sdf_data.model_matrix_inv[sdf_index] * vec4(world_position, 1.0);
//     // 转换boundingbox到模型坐标
//     vec4 bounding_min = mesh_sdf_data.model_matrix_inv[sdf_index] * vec4(mesh_sdf_data.bounds_position[sdf_index].xyz - mesh_sdf_data.bounds_distance[sdf_index].xyz, 1.0);
//     vec4 bounding_max = mesh_sdf_data.model_matrix_inv[sdf_index] * vec4(mesh_sdf_data.bounds_position[sdf_index].xyz + mesh_sdf_data.bounds_distance[sdf_index].xyz, 1.0);
//     // 转换到uv坐标
//     vec3 bounding_size = bounding_max.xyz - bounding_min.xyz;
//     vec3 position_in_bounds = (model_position - bounding_min).xyz;
//     position_in_bounds / bounding_size;
// }

bool in_bounding_box(vec3 world_position, int sdf_index)
{
    vec3 bounds_distance = mesh_sdf_data.bounds_distance[sdf_index].xyz;
    vec3 bounds_position = mesh_sdf_data.bounds_position[sdf_index].xyz;
    vec3 position_in_bounds = world_position - bounds_position;
    return all(lessThan(abs(position_in_bounds), bounds_distance));
}

vec3 sample_mesh_sdf(vec3 world_position, int sdf_index)
{
    vec3 uv = get_mesh_sdf_uv(world_position, sdf_index);
    return textureLod(sampler3D(mesh_sdf_textures[sdf_index], sdf_sampler), uv, 0.0).xyz;
}

HitResult ray_trace(Ray ray, int sdf_index)
{
    HitResult hit;

    vec3 bounds_distance = mesh_sdf_data.bounds_distance[sdf_index].xyz;
    vec3 bounds_position = mesh_sdf_data.bounds_position[sdf_index].xyz;
    float max_distance = 1e4;
    vec3 voxel_size_vec = bounds_distance * 2.0 / mesh_sdf_data.resolution;
    float voxel_size = min(voxel_size_vec.x, min(voxel_size_vec.y, voxel_size_vec.z));
    vec3 end_position = ray.world_position + ray.world_direction * max_distance;
    vec3 world_position = ray.world_position;
    // 是否和bounding box相交
    vec2 intersections = line_hit_box(world_position, end_position, bounds_position - bounds_distance, bounds_position + bounds_distance);

    // 不相交
    if (intersections.x >= intersections.y)
    {
        hit.hit_time = -1.0;
        return hit;
    }

    // 如果和bounding box相交，直接从bounding box开始步进
    float step_time = intersections.x;

    for (float step_i = 0.0; step_i < mesh_sdf_data.resolution; ++step_i)
    {
        vec3 step_position = world_position + ray.world_direction * step_time;// step_time 累计步进长度

        vec3 uv = get_mesh_sdf_uv(step_position, sdf_index);
        float sdf_distance = textureLod(sampler3D(mesh_sdf_textures[sdf_index], sdf_sampler), uv, 0.0).r;// 光线当前步进到的位置
        float min_surface_thickness = 1e-5;
        if (sdf_distance < min_surface_thickness)// 如果接近物体表面，或者进入物体内部
        {
            hit.hit_time = max(step_time + sdf_distance - min_surface_thickness, 0.0);
            hit.step_count = step_i;
            // hit.color = vec3(1.0, 0.0, 0.0);
            break;
        }
        // else
        // {
        //     hit.hit_time = max(step_time + sdf_distance - min_surface_thickness, 0.0);
        //     hit.step_count = step_i;
        //     hit.color = vec3(0.0, 1.0, 0.0);
        //     break;
        // }

        step_time += voxel_size;// voxel_size 每次步进长度
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
    ray.world_position = view_buffer.view_position.xyz;
    ray.world_direction = normalize(target_position.xyz - view_buffer.view_position.xyz);

    out_color = vec4(1.0, 1.0, 1.0, 1.0);
    float min_distance = mesh_sdf_data.global_sdf_distance;
    for (int i = 0; i < mesh_sdf_data.sdf_count; ++i)
    {
        HitResult hit = ray_trace(ray, i);
        if (hit.hit_time < min_distance && hit.hit_time > 0.0)
        {
            min_distance = hit.hit_time;
            //out_color = vec4(hit.color * 100, 1.0);
            out_color = vec4(vec3(hit.step_count / mesh_sdf_data.resolution), 1.0);
        }
    }
}