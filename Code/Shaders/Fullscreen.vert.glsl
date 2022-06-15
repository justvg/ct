#version 450

layout (location = 0) out vec2 TexCoords;

layout (location = 0) in vec2 Position;

void main()
{
    TexCoords = Position * vec2(0.5, -0.5) + vec2(0.5, 0.5);

    gl_Position = vec4(Position, 0.0, 1.0);
}