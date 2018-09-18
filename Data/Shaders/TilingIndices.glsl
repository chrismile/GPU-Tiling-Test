-- Vertex

#version 430 core

layout(location = 0) in vec3 vertexPosition;

void main()
{
	gl_Position = mvpMatrix * vec4(vertexPosition, 1.0);
}

-- Fragment

#version 430 core

// https://www.khronos.org/registry/OpenGL/extensions/NV/NV_shader_thread_group.txt
#extension GL_NV_shader_thread_group : enable

out vec4 fragColor;

/**
 * mode:
 * - 1: Combined thread number (red channel), warp number (green channel) and SM number (blue channel)
 * - 2: Thread number (multiplied with color.rgb)
 * - 3: Warp number (multiplied with color.rgb)
 * - 4: SM number (multiplied with color.rgb)
 */
uniform int mode = 1;
uniform vec4 color;

/**
 * gl_WarpSizeNV: Total number of threads in a warp.
 * gl_WarpsPerSMNV: Maximum number of warp executing on a SM (streaming multiprocessor).
 * gl_WarpSizeNV: Number of SMs on the GPU.
 */
void main()
{
    float threadNum = float(gl_ThreadInWarpNV)/float(gl_WarpSizeNV-1u);
    float warpNum = float(gl_WarpIDNV)/float(gl_WarpsPerSMNV-1u);
    float smNum = float(gl_SMIDNV)/float(gl_SMCountNV-1u);

    if (mode == 1) {
        fragColor = vec4(threadNum, warpNum, smNum, color.a);
    } else if (mode == 2) {
        fragColor = vec4(threadNum * color.rgb, color.a);
    } else if (mode == 3) {
        fragColor = vec4(warpNum * color.rgb, color.a);
    } else {
        fragColor = vec4(smNum * color.rgb, color.a);
    }
}

