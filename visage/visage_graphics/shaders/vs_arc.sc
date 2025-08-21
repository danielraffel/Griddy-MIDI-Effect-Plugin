$input a_position, a_color0, a_color1, a_texcoord0, a_texcoord1, a_texcoord2
$output v_coordinates, v_dimensions, v_shader_values, v_shader_values1, v_position, v_gradient_color_pos, v_gradient_pos

#include <shader_include.sh>

uniform vec4 u_bounds;
uniform vec4 u_origin_flip;

void main() {
  vec2 minimum = a_texcoord1.xy;
  vec2 maximum = a_texcoord1.zw;
  vec2 clamped = clamp(a_position.xy + a_texcoord0.xy * 0.5, minimum, maximum);
  vec2 delta = clamped - (a_position.xy + a_texcoord0.xy * 0.5);

  v_position = clamped;
  v_gradient_color_pos = a_color0;
  v_gradient_pos = a_color1;
  v_dimensions = a_texcoord0.zw + vec2(1.0, 1.0);
  v_coordinates = a_texcoord0.xy + (2.0 * delta) / v_dimensions;
  vec2 adjusted_position = clamped * u_bounds.xy + u_bounds.zw;
  gl_Position = vec4(adjusted_position, 0.5, 1.0);
  v_shader_values = a_texcoord2;

  float center_radians = v_shader_values.z * u_origin_flip.x - u_origin_flip.y * kPi;
  float arc_radians = min(v_shader_values.w, kPi * 0.999);
  v_shader_values1.x = sin(center_radians);
  v_shader_values1.y = cos(center_radians);
  v_shader_values1.z = sin(arc_radians);
  v_shader_values1.w = cos(arc_radians);
}
