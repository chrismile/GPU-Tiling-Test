/*
 * MainApp.hpp
 *
 *  Created on: 22.04.2017
 *      Author: Christoph Neuhauser
 */

#ifndef LOGIC_MainApp_HPP_
#define LOGIC_MainApp_HPP_

#include <vector>
#include <glm/glm.hpp>

#include <Utils/AppLogic.hpp>
#include <Utils/Random/Xorshift.hpp>
#include <Math/Geometry/Point2.hpp>
#include <Graphics/Shader/ShaderAttributes.hpp>
#include <Graphics/Mesh/Mesh.hpp>
#include <Graphics/Scene/Camera.hpp>
#include <Graphics/OpenGL/TimerGL.hpp>

namespace sgl
{
	class Texture;
	typedef boost::shared_ptr<Texture> TexturePtr;
	class FramebufferObject;
	typedef boost::shared_ptr<FramebufferObject> FramebufferObjectPtr;
}

using namespace std;
using namespace sgl;

enum TilingMode {
	/**
	 * Renders a triangle spanning half of the screen and outputs in...
	 * a) The red channel the thread index in each warp (rescaled to  [0,255])
	 * b) The green channel the warp index in each SM (rescaled to  [0,255])
	 * c) The blue channel the SM index (rescaled to [0,255])
	 */
	TILING_MODE_BINNING,

	/**
	 * Renders multiple triangles (similar to https://github.com/nlguillemot/trianglebin) and outputs only a limited
	 * amount of pixels to see how the GPU processes tiled rendering.
	 */
	TILING_MODE_TILES
};

class TilingTestApp : public AppLogic
{
public:
	TilingTestApp();
	~TilingTestApp();
	void render();
	void renderScene();
	void renderGUI();
	void update(float dt);
    void processSDLEvent(const SDL_Event &event);
	void resolutionChanged(EventPtr event);

private:
	TilingMode mode;

	// Lighting & rendering
	ShaderProgramPtr plainShader;
	ShaderProgramPtr whiteSolidShader;
	ShaderProgramPtr tilingIndicesShader; // for TILING_MODE_BINNING
	ShaderProgramPtr tilingTilesShader; // for TILING_MODE_TILES

	// Objects in the scene
	ShaderAttributesPtr singleTriangle; // for TILING_MODE_BINNING
	ShaderAttributesPtr multiTriangle; // for TILING_MODE_TILES
	sgl::GeometryBufferPtr atomicCounterBuffer;
	int numMultiTriangles;

	// Rendering parameters
    uint32_t maxNumPixels;
    int renderWidth, renderHeight;
    int resolutionScale = 1;

    // User interface
    bool showSettingsWindow = true;
    float numPixelsPercent = 50; // In %
    int guiNewMode = 3; // Mode selection

    // For off-screen rendering (used for rendering at lower resolution than native)
	FramebufferObjectPtr fbo;
	TexturePtr renderTexture;
};

#endif /* LOGIC_MainApp_HPP_ */
