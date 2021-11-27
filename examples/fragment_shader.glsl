#version 450

in vec3 vertex_color;
in vec3 vertex_background_color;
in vec2 vertex_position;

layout(location = 0) out vec4 fragment_color;

void main() {
    vec3 color = mix(
        vertex_background_color, 
        vertex_color, 
        bvec3(dot(vertex_position, vertex_position) < 1.0)
    );
    fragment_color = vec4(color, 1.0);
}
