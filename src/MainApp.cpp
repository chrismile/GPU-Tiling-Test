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

#include "MainApp.hpp"

// Defines whether to use an off-screen framebuffer (with resolution scaling) or on-screen rendering
#define RENDER_FRAMEBUFFER
static int RESOLUTION_SCALE = 1;

// Modes: TILING_MODE_INDICES, TILING_MODE_TILES
TilingTestApp::TilingTestApp() : mode(TILING_MODE_INDICES), numMultiTriangles(7) //, recording(false), videoWriter(NULL)
{
	if (!SystemGL::get()->isGLExtensionAvailable("GL_NV_shader_thread_group")) {
		Logfile::get()->writeError("Error: GL_NV_shader_thread_group not available. You won't be able to use "
							 "TILING_MODE_INDICES, however TILING_MODE_TILES should work on all systems.");
        mode = TILING_MODE_TILES;
	} else {
        GLint warpSize, warpsPerSM, smCount;
        glGetIntegerv(GL_WARP_SIZE_NV, &warpSize);
        glGetIntegerv(GL_WARPS_PER_SM_NV, &warpsPerSM);
        glGetIntegerv(GL_SM_COUNT_NV, &smCount);
        std::cout << "Warp size: " << warpSize << std::endl;
        std::cout << "Warps per SM: " << warpsPerSM << std::endl;
        std::cout << "SM count: " << smCount << std::endl;
	}

	plainShader = ShaderManager->getShaderProgram({"Mesh.Vertex.Plain", "Mesh.Fragment.Plain"});
	whiteSolidShader = ShaderManager->getShaderProgram({"WhiteSolid.Vertex", "WhiteSolid.Fragment"});
	tilingIndicesShader = ShaderManager->getShaderProgram({"TilingIndices.Vertex", "TilingIndices.Fragment"});
	tilingTilesShader = ShaderManager->getShaderProgram({"TilingTiles.Vertex", "TilingTiles.Fragment"});


	// For TILING_MODE_INDICES. For more information see the enum type "TilingMode".
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
	//	delete videoWriter;
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
	//	videoWriter = new VideoWriter("video.mp4");
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

	// Video recording enabled?
	//if (recording) {
	//	videoWriter->pushWindowFrame();
	//}
}

void TilingTestApp::renderScene()
{
	Renderer->setProjectionMatrix(matrixIdentity());
	Renderer->setModelMatrix(matrixIdentity());
	Renderer->setViewMatrix(matrixIdentity());
	if (mode == TILING_MODE_INDICES) {
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

void TilingTestApp::resolutionChanged(EventPtr event)
{
	Window *window = AppSettings::get()->getMainWindow();
	int width = window->getWidth();
	int height = window->getHeight();
	glViewport(0, 0, width, height);
	int renderWidth = width;
	int renderHeight = height;

#ifdef RENDER_FRAMEBUFFER
    // Scale screen resolution
    if (mode == TILING_MODE_INDICES) {
		renderWidth /= RESOLUTION_SCALE;
		renderHeight /= RESOLUTION_SCALE;
	}

    fbo = Renderer->createFBO();
    TextureSettings settings;
    settings.textureMagFilter = GL_NEAREST;
    renderTexture = TextureManager->createEmptyTexture(renderWidth, renderHeight, settings);
    fbo->bindTexture(renderTexture);
#else
#endif
	// Triangle covers half of screen. Thus, plan to render 1/4th of pixels on screen * #triangles.
	uint32_t maxNumPixels = static_cast<uint32_t>(renderWidth*renderHeight*numMultiTriangles/4);
	tilingTilesShader->setUniform("maxNumPixels", maxNumPixels);
}

void TilingTestApp::update(float dt)
{
	AppLogic::update(dt);

    if (Keyboard->keyPressed(SDLK_0)) {
        mode = mode == TILING_MODE_INDICES ? TILING_MODE_TILES : TILING_MODE_INDICES;
    }

    for (int i = 1; i <= 4; i++) {
        if (Keyboard->keyPressed(SDLK_0+i)) {
            tilingIndicesShader->setUniform("mode", i);
        }
	}

	// Scale screen resolution
    if (Keyboard->keyPressed(SDLK_UP) && RESOLUTION_SCALE < 64 && mode == TILING_MODE_INDICES) {
        RESOLUTION_SCALE *= 2;
        resolutionChanged(EventPtr());
    } else if (Keyboard->keyPressed(SDLK_DOWN) && RESOLUTION_SCALE > 1 && mode == TILING_MODE_INDICES) {
        RESOLUTION_SCALE /= 2;
        resolutionChanged(EventPtr());
    }
}
