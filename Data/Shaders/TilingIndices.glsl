/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2018 - 2021, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

-- Vertex

#version 430 core

layout(location = 0) in vec3 vertexPosition;

void main() {
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
void main() {
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
