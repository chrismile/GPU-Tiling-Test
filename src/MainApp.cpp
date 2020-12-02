/*
 * MainApp.cpp
 *
 *  Created on: 22.04.2017
 *      Author: Christoph Neuhauser
 */

#define GLM_ENABLE_EXPERIMENTAL
#include <climits>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <GL/glew.h>

#include <Input/Keyboard.hpp>
#include <Math/Math.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Graphics/Window.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Utils/Random/Xorshift.hpp>
#include <Utils/Timer.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Input/Mouse.hpp>
#include <Input/Keyboard.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Shader/ShaderManager.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#include <Graphics/OpenGL/GeometryBuffer.hpp>
#include <ImGui/ImGuiWrapper.hpp>

#include "MainApp.hpp"

// Defines whether to use an off-screen framebuffer (with resolution scaling) or on-screen rendering
#define RENDER_FRAMEBUFFER
const int MAX_RESOLUTION_SCALE = 32;

// Modes: TILING_MODE_BINNING, TILING_MODE_TILES
TilingTestApp::TilingTestApp() : mode(TILING_MODE_BINNING), numMultiTriangles(7) //, recording(false), videoWriter(NULL)
{
    if (!SystemGL::get()->isGLExtensionAvailable("GL_NV_shader_thread_group")) {
        Logfile::get()->writeError("Error: GL_NV_shader_thread_group not available. You won't be able to use "
                             "TILING_MODE_BINNING, however TILING_MODE_TILES should work on all systems.");
        mode = TILING_MODE_TILES;
    } else {
        //GLint warpSize, warpsPerSM, smCount;
        glGetIntegerv(GL_WARP_SIZE_NV, &warpSize);
        glGetIntegerv(GL_WARPS_PER_SM_NV, &warpsPerSM);
        glGetIntegerv(GL_SM_COUNT_NV, &smCount);
    }

    plainShader = ShaderManager->getShaderProgram({"Mesh.Vertex.Plain", "Mesh.Fragment.Plain"});
    whiteSolidShader = ShaderManager->getShaderProgram({"WhiteSolid.Vertex", "WhiteSolid.Fragment"});
    tilingIndicesShader = ShaderManager->getShaderProgram({"TilingIndices.Vertex", "TilingIndices.Fragment"});
    tilingTilesShader = ShaderManager->getShaderProgram({"TilingTiles.Vertex", "TilingTiles.Fragment"});


    // For TILING_MODE_BINNING. For more information see the enum type "TilingMode".
    std::vector<VertexPlain> triangleVertices = {
            VertexPlain(glm::vec3(-1,-1,0)),
            VertexPlain(glm::vec3(1,1,0)),
            VertexPlain(glm::vec3(-1,1,0))
    };
    GeometryBufferPtr geomBuffer = Renderer->createGeometryBuffer(
            sizeof(VertexPlain)*triangleVertices.size(), &triangleVertices.front());
    singleTriangle = ShaderManager->createShaderAttributes(tilingIndicesShader);
    singleTriangle->addGeometryBuffer(geomBuffer, "vertexPosition", ATTRIB_FLOAT, 3);
    tilingIndicesShader->setUniform("color", Color(165, 220, 84, 255));
    tilingIndicesShader->setUniform("mode", guiNewMode);


    // For TILING_MODE_TILES. For more information see the enum type "TilingMode".
    std::vector<VertexPlain> multiTriangleVertices;
    for (int i = 0; i < numMultiTriangles; i++) {
        for (size_t j = 0; j < triangleVertices.size(); j++) {
            multiTriangleVertices.push_back(triangleVertices.at(j));
        }
    }
    GeometryBufferPtr geomBufferMultiTriangleVertices = Renderer->createGeometryBuffer(
            sizeof(VertexPlain)*multiTriangleVertices.size(), &multiTriangleVertices.front());
    std::vector<uint8_t> multiTriangleColors = {
            255, 0, 0, 255, 0, 0, 255, 0, 0,
            0, 255, 0, 0, 255, 0, 0, 255, 0,
            0, 0, 255, 0, 0, 255, 0, 0, 255,
            255, 255, 0, 255, 255, 0, 255, 255, 0,
            255, 0, 255, 255, 0, 255, 255, 0, 255,
            0, 255, 255, 0, 255, 255, 0, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255
    };
    GeometryBufferPtr geomBufferMultiTriangleColors = Renderer->createGeometryBuffer(
            sizeof(uint8_t)*multiTriangleColors.size(), &multiTriangleColors.front());
    multiTriangle = ShaderManager->createShaderAttributes(tilingTilesShader);
    multiTriangle->addGeometryBuffer(geomBufferMultiTriangleVertices, "vertexPosition", ATTRIB_FLOAT, 3);
    multiTriangle->addGeometryBuffer(geomBufferMultiTriangleColors, "vertexColor", ATTRIB_UNSIGNED_BYTE, 3);

    // Atomic counter for the rendered number of fragments for TILING_MODE_TILES.
    atomicCounterBuffer = Renderer->createGeometryBuffer(sizeof(uint32_t), NULL, ATOMIC_COUNTER_BUFFER);
    tilingTilesShader->setAtomicCounterBuffer(0, atomicCounterBuffer);


    EventManager::get()->addListener(RESOLUTION_CHANGED_EVENT, [this](EventPtr event){this->resolutionChanged(event);});
    resolutionChanged(EventPtr());

}

TilingTestApp::~TilingTestApp()
{
    //if (videoWriter != NULL) {
    //  delete videoWriter;
    //}
}

void TilingTestApp::render()
{
    bool wireframe = false;

#ifdef RENDER_FRAMEBUFFER
    Renderer->bindFBO(fbo);
    Renderer->clearFramebuffer(GL_COLOR_BUFFER_BIT);
#endif

    //if (videoWriter == NULL && recording) {
    //  videoWriter = new VideoWriter("video.mp4");
    //}

    renderScene();

    // Wireframe mode
    if (wireframe) {
        Renderer->setModelMatrix(matrixIdentity());
        Renderer->setLineWidth(1.0f);
        Renderer->enableWireframeMode();
        renderScene();
        Renderer->disableWireframeMode();
    }

#ifdef RENDER_FRAMEBUFFER
    Renderer->unbindFBO();
    Renderer->blitTexture(renderTexture, AABB2(glm::vec2(-1,-1), glm::vec2(1,1)));
#endif

    renderGUI();

    // Video recording enabled?
    //if (recording) {
    //  videoWriter->pushWindowFrame();
    //}
}

void TilingTestApp::renderScene()
{
    Renderer->setProjectionMatrix(matrixIdentity());
    Renderer->setModelMatrix(matrixIdentity());
    Renderer->setViewMatrix(matrixIdentity());
    if (mode == TILING_MODE_BINNING) {
        Renderer->render(singleTriangle);
    } else if (mode == TILING_MODE_TILES) {
        // Clear the fragment counter buffer before rendering
        GLuint bufferID = static_cast<sgl::GeometryBufferGL*>(atomicCounterBuffer.get())->getBuffer();
        GLubyte val = 0;
        glClearNamedBufferData(bufferID, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, (const void*)&val);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        Renderer->render(multiTriangle);
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void TilingTestApp::renderGUI()
{
    ImGuiWrapper::get()->renderStart();
    //ImGuiWrapper::get()->renderDemoWindow();

    if (showSettingsWindow) {
        ImGui::Begin("Settings", &showSettingsWindow);

        // Output of GPU capabilities: warpSize, warpsPerSM, smCount
        ImGui::Text("Warp size: %d, Warps per SM: %d, #SMs: %d", warpSize, warpsPerSM, smCount);

        // Mode selection (further settings dependent on main mode)
        bool updateMode = false;
        if (ImGui::Button(mode == TILING_MODE_BINNING ? "Show Tiles" : "Show Binning")) {
            mode = mode == TILING_MODE_BINNING ? TILING_MODE_TILES : TILING_MODE_BINNING;
            resolutionChanged(EventPtr());
        }
        if (mode == TILING_MODE_BINNING) {
            ImGui::SameLine();
            if (ImGui::RadioButton("All", &guiNewMode, 1)) { updateMode = true; } ImGui::SameLine();
            if (ImGui::RadioButton("Thread ID", &guiNewMode, 2)) { updateMode = true; } ImGui::SameLine();
            if (ImGui::RadioButton("Warp ID", &guiNewMode, 3)) { updateMode = true; } ImGui::SameLine();
            if (ImGui::RadioButton("SM ID", &guiNewMode, 4)) { updateMode = true; }
        }
        if (updateMode) {
            tilingIndicesShader->setUniform("mode", guiNewMode);
        }

        // Zoom factor of off-screen rendering
        if (ImGui::SliderInt("Zoom", &resolutionScale, 1, MAX_RESOLUTION_SCALE)) {
            resolutionChanged(EventPtr());
        }

        // Percent of pixels rendered in tiles mode
        if (mode == TILING_MODE_TILES && ImGui::SliderFloat("#Pixels", &numPixelsPercent, 0, 100, "%.0f\%%")) {
            Window *window = AppSettings::get()->getMainWindow();
            int width = window->getWidth();
            int height = window->getHeight();
            maxNumPixels = static_cast<uint32_t>(width*height*numMultiTriangles/2 * numPixelsPercent/100.0f);
            tilingTilesShader->setUniform("maxNumPixels", maxNumPixels);
        }

        const char *pixelFormatNames[] = {"RGBA8", "RGBA16", "RGBA16F", "RGBA32F"};
        if (mode == TILING_MODE_TILES && ImGui::Combo("Pixel Format", &currentPixelFormat, pixelFormatNames,
                IM_ARRAYSIZE(pixelFormatNames))) {
            resolutionChanged(EventPtr());
        }

        // Color selection in binning mode (if not showing all values in different color channels in mode 1)
        if (mode == TILING_MODE_BINNING && guiNewMode != 1) {
            static ImVec4 color = ImColor(165, 220, 84, 255);
            int misc_flags = 0;
            ImGui::Text("Color widget:");
            ImGui::SameLine(); ImGuiWrapper::get()->showHelpMarker("Click on the colored square to open a color picker."
                                                                   "\nCTRL+click on individual component to input value.\n");
            if (ImGui::ColorEdit3("MyColor##1", (float*)&color, misc_flags)) {
                tilingIndicesShader->setUniform("color", colorFromFloat(color.x, color.y, color.z, 1.0));
            }
        }

        ImGui::End();
    }

    //ImGui::RadioButton
    ImGuiWrapper::get()->renderEnd();
}

void TilingTestApp::processSDLEvent(const SDL_Event &event)
{
    ImGuiWrapper::get()->processSDLEvent(event);
}

void TilingTestApp::resolutionChanged(EventPtr event)
{
    Window *window = AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();
    glViewport(0, 0, width, height);
    renderWidth = width;
    renderHeight = height;

#ifdef RENDER_FRAMEBUFFER
    // Scale screen resolution
    //if (mode == TILING_MODE_BINNING) {
    renderWidth /= resolutionScale;
    renderHeight /= resolutionScale;
    //}

    fbo = Renderer->createFBO();
    TextureSettings settings;
    if (mode == TILING_MODE_TILES) {
        if (currentPixelFormat == 0) {
            settings.internalFormat = GL_RGBA8;
            settings.pixelFormat = GL_RGBA;
            settings.pixelType = GL_UNSIGNED_INT;
        } else if (currentPixelFormat == 1) {
            settings.internalFormat = GL_RGBA16;
            settings.pixelFormat = GL_RGBA;
            settings.pixelType = GL_UNSIGNED_INT;
        } else if (currentPixelFormat == 1) {
            settings.internalFormat = GL_RGBA16F;
            settings.pixelFormat = GL_RGBA;
            settings.pixelType = GL_FLOAT;
        } else if (currentPixelFormat == 1) {
            settings.internalFormat = GL_RGBA32F;
            settings.pixelFormat = GL_RGBA;
            settings.pixelType = GL_FLOAT;
        }
    }
    settings.textureMagFilter = GL_NEAREST;
    renderTexture = TextureManager->createEmptyTexture(renderWidth, renderHeight, settings);
    fbo->bindTexture(renderTexture);
#else
#endif
    // Triangle covers half of screen. Thus, plan to render 1/4th of pixels on screen * #triangles.
    maxNumPixels = static_cast<uint32_t>(width*height*numMultiTriangles/2 * numPixelsPercent/100.0f);
    tilingTilesShader->setUniform("maxNumPixels", maxNumPixels);
}

void TilingTestApp::update(float dt)
{
    AppLogic::update(dt);

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        // Ignore inputs below
        return;
    }

    if (Keyboard->keyPressed(SDLK_0)) {
        mode = mode == TILING_MODE_BINNING ? TILING_MODE_TILES : TILING_MODE_BINNING;
    }

    for (int i = 1; i <= 4; i++) {
        if (Keyboard->keyPressed(SDLK_0+i)) {
            tilingIndicesShader->setUniform("mode", i);
            guiNewMode = i;
        }
    }

    // Scale screen resolution
    if (Keyboard->keyPressed(SDLK_UP) && resolutionScale < MAX_RESOLUTION_SCALE) {
        resolutionScale *= 2;
        resolutionChanged(EventPtr());
    } else if (Keyboard->keyPressed(SDLK_DOWN) && resolutionScale > 1) {
        resolutionScale /= 2;
        resolutionChanged(EventPtr());
    }
}
