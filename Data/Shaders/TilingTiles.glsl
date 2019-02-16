-- Vertex

#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

out vec4 triangleColor;

void main()
{
    triangleColor = vec4(vertexColor, 1.0);
    gl_Position = mvpMatrix * vec4(vertexPosition, 1.0);
}

-- Fragment

#version 430 core

in vec4 triangleColor;

out vec4 fragColor;

// Number of fragments render so far in this frame
layout(binding = 0, offset = 0) uniform atomic_uint fragCounter;
// Maximum number of pixels to render in one frame
uniform uint maxNumPixels;

void main()
{
    uint insertIndex = atomicCounterIncrement(fragCounter);
    if (insertIndex >= maxNumPixels) {
        discard;
    }

    fragColor = triangleColor;
}

