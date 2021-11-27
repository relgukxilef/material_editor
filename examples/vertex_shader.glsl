#version 450

out vec2 vertex_position;

uniform UniformBufferObject {
    float radius;
};

vec2 positions[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex] * 2.0 - 1.0, 0.0, 1.0);
    vertex_position = (positions[gl_VertexIndex] - 0.5f) / radius;
}