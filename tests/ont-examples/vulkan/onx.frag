#version 450
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D tex;

layout(location = 0)      in  vec4  texture_coord;
layout(location = 1)      in  vec4  model_pos;
layout(location = 2)      in  vec4  proj_pos;
layout(location = 3) flat in  uint  phase;
layout(location = 4)      in  float near;
layout(location = 5)      in  float far;
layout(location = 6)      in  vec3  near_point;
layout(location = 7)      in  vec3  far_point;
layout(location = 8)      in  vec2  left_touch;
layout(location = 9)      in  vec2  overlay_uv;
layout(location = 10)     in  mat4  view;
layout(location = 14)     in  mat4  proj;

layout(location = 0)      out vec4  color;

vec4 grid_colour(vec3 pos, float scale) {
    vec2 coord = pos.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float rgb=clamp(1-line, 0, 1)/20;
    vec4 color = vec4(rgb, rgb, rgb, 1.0) + vec4(0, 0.3, 0, 1.0);
    float minimumx = min(derivative.x, 1);
    float minimumz = min(derivative.y, 1);
    if(pos.x > -1.0 * minimumx && pos.x < 1.0 * minimumx) color = vec4(0.2,0.4,0.2,1.0);
    if(pos.z > -1.0 * minimumz && pos.z < 1.0 * minimumz) color = vec4(0.2,0.4,0.2,1.0);
    return color;
}

float linear_depth(vec4 ppos) {
    float d = (ppos.z / ppos.w) * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - d * (far - near)) / far;
}

void main() {

  if(phase == 0){ // ground plane

    float t = near_point.y / (near_point.y - far_point.y);
    vec3 pos = near_point + t * (far_point - near_point);
    vec4 ppos = proj * view * vec4(pos.xyz, 1.0);

    gl_FragDepth = ppos.z / ppos.w;

    color = grid_colour(pos, 1) * float(t > 0);

    //color.a *= 0.4 + 0.6 * max(0, (0.5 - linear_depth(ppos)));
  }
  else
  if(phase == 1){ // panels

    const vec3 light_dir= vec3(1.0,  1.0, -1.0);
    float light = max(0.8, dot(light_dir, normalize(cross(dFdx(model_pos.xyz),dFdy(model_pos.xyz)))));
    color = light * texture(tex, texture_coord.xy) + vec4(0.0, 0.0, 0.0, 1.0);
    gl_FragDepth = proj_pos.z / proj_pos.w;
  }
  else
  if(phase == 2){ // overlay

    float r = 0.1;
    float d = distance(overlay_uv, left_touch);

    if (d <= r) {
        color = vec4(1.0, 1.0, 1.0, 0.04);
        gl_FragDepth = -1.0;
    } else {
        discard;
    }
  }
}





