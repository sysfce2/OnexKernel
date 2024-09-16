#version 450
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_multiview : enable

layout(push_constant) uniform constants {
  uint phase;
} push_constants;

// TODO model[MAX_PANELS]
layout(std430, binding = 0) uniform buf0 {
  mat4 proj;
  mat4 view_l;
  mat4 view_r;
  mat4 model[32];
} uniforms;

layout(location = 0)  in  vec3  vertex;
layout(location = 1)  in  vec2  uv;

layout(location = 0)  out vec4  texture_coord;
layout(location = 1)  out vec4  model_pos;
layout(location = 2)  out vec4  proj_pos;
layout(location = 3)  out uint  phase;
layout(location = 4)  out float near;
layout(location = 5)  out float far;
layout(location = 6)  out vec3  near_point;
layout(location = 7)  out vec3  far_point;
layout(location = 8)  out vec2  overlay_uv;
layout(location = 9)  out mat4  view;
layout(location = 13) out mat4  proj;

out gl_PerVertex {
    vec4 gl_Position;
};

vec3 grid_plane[6] = vec3[] (
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 unproject(float x, float y, float z, mat4 view, mat4 proj) {
    mat4 view_inv = inverse(view);
    mat4 proj_inv = inverse(proj);
    vec4 unprojected_point =  view_inv * proj_inv * vec4(x, y, z, 1.0);
    return unprojected_point.xyz / unprojected_point.w;
}

vec2 overlay_quad[4] = vec2[](
    vec2(-1.0, -1.0), // bottom-left
    vec2( 1.0, -1.0), // bottom-right
    vec2(-1.0,  1.0), // top-left
    vec2( 1.0,  1.0)  // top-right
);

int overlay_quad_indices[6] = int[](
    2, 3, 0,
    0, 3, 1
);

void main() {

  view = gl_ViewIndex==0? uniforms.view_l: uniforms.view_r;
  proj = uniforms.proj;
  phase = push_constants.phase;

  if(phase == 0){ // ground plane

    near = 0.004;
    far = 0.17;
    vec3 p = grid_plane[gl_VertexIndex].xyz;
    near_point = unproject(p.x, p.y, 0.0, view, proj).xyz;
    far_point  = unproject(p.x, p.y, 1.0, view, proj).xyz;
    gl_Position = vec4(p, 1.0);
  }
  else
  if(phase == 1){ // panels

    texture_coord = vec4(uv, 0, 0);

    model_pos = uniforms.model[gl_InstanceIndex] *
                vec4(vertex, 1.0);

    gl_Position = proj * view * model_pos;
  }
  else
  if(phase == 2){ // overlay

    gl_Position = vec4(overlay_quad[overlay_quad_indices[gl_VertexIndex]], 0.0, 1.0);
    float aspect_ratio = 1.778;
    overlay_uv = vec2((gl_Position.x * 0.5 + 0.5)*aspect_ratio, gl_Position.y * 0.5 + 0.5);
  }
  proj_pos = gl_Position;
}
