#version 450

in vec2 vertex_position;

layout(location = 0) out vec4 fragment_color;

uniform UniformBufferObject {
    vec4 color;
    vec4 background_color;
};

void main() {
    fragment_color = mix(
        background_color, color, 
        bvec4(dot(vertex_position, vertex_position) < 1.0)
    );
}
