#pragma once
#include <Component/SimpleScene.h>
#include <Component/Transform/Transform.h>
#include <Core/GPU/Mesh.h>
#include <Core/GPU/CubeMapFBO.h>
#include <Core/GPU/Texture2D.h>
#include <Core/GPU/ShadowCubeMapFBO.h>
#include "Gizmo.hpp"
#include "Camera.hpp"
#include "Grid.hpp"
#include "FullscreenQuad.hpp"
#include <Core/GPU/Framebuffer.hpp>
#include "ColorGenerator.hpp"
#include <Core\GPU\Sprite.hpp>
#include "DisjointSets.hpp"
#include <unordered_map>
#include <set>
typedef std::vector<VertexFormat> TVertexList;
typedef std::vector<uint32_t> TIndexList;
typedef std::vector<glm::vec3> CurvePointList;

using namespace std;

class AudioVisualizer : public SimpleScene
{
	enum ActiveToolType { SELECT_TOOL, MOVE_TOOL, ROTATE_TOOL, PLANE_SLICE_TOOL};
	public:
		AudioVisualizer();
		~AudioVisualizer();

		void Init() override;
	private:
		void LoadMaterials();
		void LoadMeshes();
		void LoadShaders();

		void FrameStart() override;
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;
		void STMaudioUpdate(float *minOut, float *maxOut, float *fullBuffer);
		void DrawLine(Shader *shader, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &color = glm::vec3(1), int lineWidth = 2);
		void InitAudioVisualizer();
		void RenderSimpleMesh(Mesh *mesh, Shader *shader, const glm::mat4 &modelMatrix, Texture2D* texture1 = NULL, Texture2D* texture2 = NULL, glm::vec3 color = glm::vec3(0, 0, 0));
		//void ForceRedraw();
		void DrawBorder();
		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;
		void RenderButtons();
		void ImGUIRender();
private:
	float inputAudioDataMin[1024], inputAudioDataMax[1024];
	float outputAudioDataMin[1024], outputAudioDataMax[1024];

	glm::mat4 model_matrix, view_matrix, projection_matrix;
	BaseMesh *lineMesh;
	BaseMesh *pointMesh;
	Sprite *fsQuad, *textSprite;

	bool keyStates[256];
	bool m_LMB = false, m_RMB = false, m_MMB = false;
	bool m_altDown = false;

	float m_deltaTime;
	int m_width = 1280, m_height = 720;

	
	lab::Framebuffer colorPickingFB;
	GLubyte *readPixels;
	unsigned int quadTexture;
	glm::ivec2 prev_mousePos;
	ColorGenerator colorGen;
	Texture2D *buttonUp, *buttonDown, *buttonDisabled, *logoPic;
	Texture2D *inTimePic, *outTimePic, *inFreqPic, *outFreqPic;
	Texture2D *distortionPic, *overdrivePic, *fuzzPic, *reverbPic, *delayPic, *compressorPic, *recordPic, *changeBGPic, *playWAVpic;
	Sprite *m_sprite;
	bool showColorPickingFB = false;
	
	ColorGenerator colorGenerator;
};