#include "AudioVisualizer.h"
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include "DisjointSets.hpp"
#include <Core/Engine.h>
#include <algorithm>
#include "MeshSlicing.hpp"
#include "MeshPatches.hpp"
#include <chrono>

#include "audioprobe.hpp"
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

#include "imgui_setup.h"

#define PI 3.1415926f
static char wavPath[128];
int backgroundID = 0;
glm::vec3 backgroundColors[5] = {glm::vec3(0.1f, 0.1f, 0.15f), glm::vec3(1),
								 glm::vec3(0.33), glm::vec3(0.66), glm::vec3(0.5)};

#define GUI_FRACTION 18

float ChangeInterval(float x, float a, float b, float c, float d)
{
	return c + (x - a) / (b - a) * (d - c);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STMaudio audio;
void AudioVisualizer::STMaudioUpdate(float *minOut, float *maxOut, float *viewedSamples)
{
	
	int writeHead = audio.GetWriteHead();
	int k = 0;

	float minA, maxA;
	minA = 999; maxA = -999;
	int i = writeHead + 1;
	while (i < 32768)
	{
		if (viewedSamples[i] > maxA)
			maxA = viewedSamples[i];

		if (viewedSamples[i] < minA)
			minA = viewedSamples[i];

		if(i % 32 == 0)
		{
			maxOut[k] = maxA;
			minOut[k] = minA;
			minA = 999; maxA = -999;
			k++;
		}
		i++;
	}

	i = 0;
	while (i < writeHead + 1)
	{
		if (viewedSamples[i] > maxA)
			maxA = viewedSamples[i];

		if (viewedSamples[i] < minA)
			minA = viewedSamples[i];

		if (i % 32 == 0)
		{
			maxOut[k] = maxA;
			minOut[k] = minA;
			minA = 999; maxA = -999;
			k++;
		}
		i++;
	}

}

void AudioVisualizer::LoadShaders()
{
	{// DULL COLOR
		Shader *shader = new Shader("DullColorShader");
		shader->AddShader("Shaders/pointVertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader("Shaders/fragment.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
	{// FULL-SCREEN SHADER
		Shader *shader = new Shader("FullScreenShader");
		shader->AddShader("Shaders/fullscreenVertex.glsl", GL_VERTEX_SHADER);
		shader->AddShader("Shaders/fullscreenFragment.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	{// DULL COLOR
		Shader *shader = new Shader("Text");
		shader->AddShader("Shaders/textVertex.glsl", GL_VERTEX_SHADER);
		shader->AddShader("Shaders/textFragment.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	//DEFAULT LIT GREY SHADER(hardcoded DirLight in shader)
	{
		Shader *shader = new Shader("default");
		shader->AddShader("Shaders/defaultVertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader("Shaders/defaultFragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	//FLAT COLOR SHADER
	{
		Shader *shader = new Shader("flat");
		shader->AddShader("Shaders/defaultVertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader("Assets/Shaders/Color.FS.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	//VERTEX COLOR SHADER
	{
		Shader *shader = new Shader("VertexColor");
		shader->AddShader("Shaders/defaultVertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader("Assets/Shaders/VertexColor.FS.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

std::vector<glm::vec3> ggColors{
	glm::vec3(0,0.2,1), glm::vec3(1,0.5,0), glm::vec3(0.5,0.5,0), glm::vec3(0,1,0), glm::vec3(0,1, 0.5), glm::vec3(0,0.5,1), glm::vec3(0,0,1), glm::vec3(0,1,1),
	glm::vec3(1,0,0), glm::vec3(1,0.5,0), glm::vec3(0.5,0.5,0), glm::vec3(0,1,0), glm::vec3(0,1, 0.5), glm::vec3(0,0.5,1), glm::vec3(0,0,1), glm::vec3(0,1,1),
	glm::vec3(1,0,0), glm::vec3(1,0.5,0), glm::vec3(0.5,0.5,0), glm::vec3(0,1,0), glm::vec3(0,1, 0.5), glm::vec3(0,0.5,1), glm::vec3(0,0,1), glm::vec3(0,1,1),
	glm::vec3(1,0,0), glm::vec3(1,0.5,0), glm::vec3(0.5,0.5,0), glm::vec3(0,1,0), glm::vec3(0,1, 0.5), glm::vec3(0,0.5,1), glm::vec3(0,0,1), glm::vec3(0,1,1)
};

AudioVisualizer::AudioVisualizer()
{
	sprintf(wavPath, "WAVs/123.wav");
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	m_width = 800; m_height = 450;
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	LoadShaders();
	model_matrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	view_matrix = glm::lookAt(glm::vec3(-5, 10, 75), glm::vec3(5, 10, 0), glm::vec3(0, 1, 0));

	//wireframe draw mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	memset(keyStates, 0, 256);


	colorPickingFB.generate(m_width, m_height);

	readPixels = (GLubyte*)malloc(3 * m_width * m_height);

	fsQuad = new Sprite(shaders["FullScreenShader"], &m_width, &m_height, glm::vec3(-1, -1, 0), glm::vec3(1, 1, 0));
	textSprite = new Sprite(shaders["FullScreenShader"], &m_width, &m_height, glm::vec3(-1, -1, 0), glm::vec3(1, 1, 0));


	std::vector<glm::uvec3> reservedColors = {
		glm::uvec3(255, 0, 0), glm::uvec3(0, 255, 0), glm::uvec3(0, 0, 255),
		glm::uvec3(255, 255, 0), glm::uvec3(255, 0, 255), glm::uvec3(0, 127, 127),
		glm::uvec3(127, 0, 0), glm::uvec3(0, 127, 0), glm::uvec3(0, 0, 127),
		glm::uvec3(127, 127, 0), glm::uvec3(127, 0, 127), glm::uvec3(0, 127, 127),
		glm::uvec3(0, 128, 128), glm::uvec3(128, 0, 0), glm::uvec3(0, 128, 0),
		glm::uvec3(0, 0, 128), glm::uvec3(128, 128, 0), glm::uvec3(128, 0, 128),
		glm::uvec3(0, 128, 128), glm::uvec3(64, 64, 64), glm::uvec3(64, 64, 127),
		glm::uvec3(64, 127, 127), glm::uvec3(64, 127, 191),	glm::uvec3(191, 127, 191),
		glm::uvec3(191, 0, 191), glm::uvec3(191, 64, 191)
	};
	colorGen.SetReservedColors(reservedColors);
	
	InitAudioVisualizer();

	OnWindowResize(800, 450);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::InitAudioVisualizer()
{
	audio.Run();
	ImGui_ImplGlfwGL3_Init(window->window, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioVisualizer::~AudioVisualizer()
{
	//distruge shader
	//distruge mesh incarcat
	delete lineMesh;
	delete pointMesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::LoadMaterials()
{ 

	buttonUp= new Texture2D();
	buttonUp->Load2D("Assets/buttonUp.png", GL_REPEAT);

	buttonDown = new Texture2D();
	buttonDown->Load2D("Assets/buttonDown.png", GL_REPEAT);
	
	buttonDisabled = new Texture2D();
	buttonDisabled->Load2D("Assets/buttonDisabled.png", GL_REPEAT);

	distortionPic = new Texture2D();
	distortionPic->Load2D("Assets/distortion.png", GL_REPEAT);
	overdrivePic = new Texture2D();
	overdrivePic->Load2D("Assets/overdrive.png", GL_REPEAT);
	fuzzPic = new Texture2D();
	fuzzPic->Load2D("Assets/fuzz.png", GL_REPEAT);
	delayPic = new Texture2D();
	delayPic->Load2D("Assets/delay.png", GL_REPEAT);
	reverbPic= new Texture2D();
	reverbPic->Load2D("Assets/reverb.png", GL_REPEAT);
	compressorPic= new Texture2D();
	compressorPic->Load2D("Assets/compressor.png", GL_REPEAT);
	changeBGPic = new Texture2D();
	changeBGPic->Load2D("Assets/changeBackground.png", GL_REPEAT);

	recordPic = new Texture2D();
	recordPic->Load2D("Assets/record.png", GL_REPEAT);
	playWAVpic = new Texture2D();
	playWAVpic->Load2D("Assets/playWAV.png", GL_REPEAT);


	logoPic = new Texture2D();
	logoPic->Load2D("Assets/logo.png", GL_REPEAT);

	inTimePic = new Texture2D();
	inTimePic->Load2D("Assets/inputTimeLabel.png", GL_REPEAT);
	outTimePic = new Texture2D();
	outTimePic->Load2D("Assets/outputTimeLabel.png", GL_REPEAT);
	inFreqPic = new Texture2D();
	inFreqPic->Load2D("Assets/inputFreqLabel.png", GL_REPEAT);
	outFreqPic = new Texture2D();
	outFreqPic->Load2D("Assets/outputFreqLabel.png", GL_REPEAT);

	m_sprite = new Sprite(shaders["FullScreenShader"], &m_width, &m_height);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::LoadMeshes()
{
	lineMesh = generateLineMesh();
	pointMesh = generatePointMesh();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::Init()
{
	LoadMaterials();
	LoadMeshes();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::RenderSimpleMesh(Mesh *mesh, Shader *shader, const glm::mat4 & modelMatrix, Texture2D* texture1, Texture2D* texture2, glm::vec3 color)
{
	if (!mesh || !shader || !shader->GetProgramID())
		return;

	// render an object using the specified shader and the specified position
	glUseProgram(shader->program);

	// Bind model matrix
	GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	// Bind view matrix
	int loc_view_matrix = glGetUniformLocation(shader->program, "View");
	glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(view_matrix));

	int loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
	glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	if (texture1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1->GetTextureID());
		glUniform1i(glGetUniformLocation(shader->GetProgramID(), "texture1"), 0);
	}

	int colLoc = glGetUniformLocation(shader->GetProgramID(), "color");
	if (colLoc >= 0)
	{
		glUniform3f(colLoc, color.x, color.y, color.z);
	}
	// Draw the object
	glBindVertexArray(mesh->GetBuffers()->VAO);
	glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_SHORT, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::FrameStart()
{
}

void AudioVisualizer::DrawLine(Shader *shader, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &color, int lineWidth)
{
	glLineWidth(lineWidth);
	float dist = glm::distance(p2, p1);
	if (dist < 0.0001f)
		return;

	glm::mat4 scaleMat = glm::scale(glm::mat4(1), glm::vec3(dist));
	glm::mat4 rotationMat = glm::mat4(1);
	glm::mat4 translationMat = glm::translate(glm::mat4(1), glm::vec3(p1));
	glm::vec3 lineDir = glm::normalize(p2 - p1);
	
	float angleCosine = glm::dot(lineDir, glm::vec3(0, 1, 0));

	
	if (fabs(angleCosine + 1.f) < 0.0001f) 
	{// 180 degrees
		rotationMat = glm::rotate(glm::mat4(1), PI, glm::vec3(1, 0, 0));
	}
	else if (fabs(angleCosine - 1.f) > 0.0001f)
	{ //0 degrees doesn't require rot
		rotationMat = glm::rotate(glm::mat4(1), acos(angleCosine), glm::cross(glm::vec3(0, 1, 0), lineDir));
	}
	
	glUniformMatrix4fv(shader->GetUniformLocation(std::string("model_matrix")),
								1, false, glm::value_ptr(translationMat * rotationMat * scaleMat));
	glUniform3f(shader->GetUniformLocation(std::string("color")), color.r, color.g, color.b);
	lineMesh->draw(GL_LINES);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::ImGUIRender()
{
	ImGui_ImplGlfwGL3_NewFrame();
	// 1. Show a simple window
	bool xt = true;
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Pedalboard Settings", &xt);
	//ImGui::Text("Options:");
	//int ik = 155;
	if (audio.GetSettings().distortion)
	{
		ImGui::Text("DISTORTION:");
		ImGui::InputFloat("Timbre", &(audio.GetSettings().distortionTimbre), 0.1f);
		ImGui::InputFloat("Depth", &(audio.GetSettings().distortionDepth), 0.1f);
	}//
	if (audio.GetSettings().overdrive)
	{
		ImGui::Text("OVERDRIVE:");
		ImGui::InputFloat("Gain", &(audio.GetSettings().overdriveGain), 0.1f);
	}//
	if (audio.GetSettings().fuzz)
	{
		ImGui::Text("FUZZ:");
		ImGui::InputFloat("Gain", &(audio.GetSettings().fuzzGain), 0.1f);
		ImGui::InputFloat("Mix", &(audio.GetSettings().fuzzMix), 0.1f);
	}//
	if (audio.GetSettings().delay)
	{
		ImGui::Text("DELAY:");
		ImGui::InputInt("Delay Time", &(audio.GetSettings().delayTime), 1000);
		ImGui::InputFloat("Feedback", &(audio.GetSettings().delayFeedback), 0.1f);
	}//
	ImGui::InputText("Open WAV file path:", wavPath, 128);
	//if (audio.GetSettings().delay)
	//{
	//	ImGui::Text("DELAY:");
	//	ImGui::InputInt("Delay Time", &(audio.GetSettings().delayTime), 1000);
	//	ImGui::InputFloat("Feedback", &(audio.GetSettings().delayFeedback), 0.01f);
	//}//
	char txt[16];
	for (int i = 0; i < 9; i++)
	{
		memset(txt, 0, 16);
		sprintf(txt, "[%d]Hz", i);
		ImGui::SliderFloat(txt, &(audio.GetSettings().inputFreqKnobs[i]), 0.f, 2.f);
	}
	ImGui::End();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	ImGui::Render();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::Update(float deltaTimeSeconds)
{
	m_deltaTime = deltaTimeSeconds;

	colorPickingFB.unbind();
	glDisable(GL_DEPTH_TEST);
	
	glLineWidth(1);

	glClearColor(backgroundColors[backgroundID].r, backgroundColors[backgroundID].g, backgroundColors[backgroundID].b, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUseProgram(shaders["DullColorShader"]->GetProgramID());

	glUniformMatrix4fv(shaders["DullColorShader"]->GetUniformLocation(std::string("view_matrix")), 1, false, glm::value_ptr(glm::mat4(1)));
	glUniformMatrix4fv(shaders["DullColorShader"]->GetUniformLocation(std::string("projection_matrix")), 1, false, glm::value_ptr(glm::mat4(1)));

	STMaudioUpdate(inputAudioDataMin, inputAudioDataMax, audio.GetCircularInputBuffer());
	STMaudioUpdate(outputAudioDataMin, outputAudioDataMax, audio.GetCircularOutputBuffer());
#define TIME_SIGNAL_COLOR glm::vec3(0.514, 0.576, 0.792)
#define FREQ_SIGNAL_COLOR glm::vec3(0.9, 0.6, 0.3)

	float x;
	glViewport(m_width / GUI_FRACTION, m_height / 2, m_width / 2 - m_width / GUI_FRACTION / 2, m_height/2);
	for(int i = 0; i < 1024; i++)
	{//Draw Input Signal
		x = ChangeInterval(i, 0, 1023, -1, 1);
		DrawLine(shaders["DullColorShader"], glm::vec3(x, 0, 0), glm::vec3(x, inputAudioDataMax[i], 0), TIME_SIGNAL_COLOR);
		DrawLine(shaders["DullColorShader"], glm::vec3(x, 0, 0), glm::vec3(x, inputAudioDataMin[i], 0), TIME_SIGNAL_COLOR);
	}
	DrawBorder();

	glViewport(m_width / GUI_FRACTION / 2 + m_width / 2 , m_height / 2, m_width / 2 - m_width / GUI_FRACTION / 2, m_height / 2);
	for (int i = 0; i < 1024; i++)
	{//Draw Output Signal
		x = ChangeInterval(i, 0, 1023, -1, 1);
		DrawLine(shaders["DullColorShader"], glm::vec3(x, 0, 0), glm::vec3(x, outputAudioDataMax[i], 0), TIME_SIGNAL_COLOR);
		DrawLine(shaders["DullColorShader"], glm::vec3(x, 0, 0), glm::vec3(x, outputAudioDataMin[i], 0), TIME_SIGNAL_COLOR);
	}
	DrawBorder();

	glViewport(m_width / GUI_FRACTION, 0, m_width / 2 - m_width / GUI_FRACTION / 2, m_height / 2);
	float *magnitudes = audio.GetInputFreqMagnitudes();
	for (int i = 0; i < 256; i++)
	{//Draw Input FFT
		x = ChangeInterval(i, 0, 255, -1, 3);
		float &mag = magnitudes[i];
		DrawLine(shaders["DullColorShader"], glm::vec3(x, -1, 0), glm::vec3(x, mag * 2 - 1, 0), FREQ_SIGNAL_COLOR);
	}
	DrawBorder();

	glViewport(m_width / 2 + m_width / GUI_FRACTION / 2, 0, m_width / 2 - m_width / GUI_FRACTION / 2, m_height / 2);
	magnitudes = audio.GetOutputFreqMagnitudes();
	for (int i = 0; i < 256; i++)
	{//Draw Output FFT
		x = ChangeInterval(i, 0, 255, -1, 3);
		float &mag = magnitudes[i];
		DrawLine(shaders["DullColorShader"], glm::vec3(x, -1, 0), glm::vec3(x, mag * 2 - 1, 0), FREQ_SIGNAL_COLOR);
	}
	DrawBorder();
	ImGUIRender();
///////////DRAW POINTS
	glUseProgram(shaders["DullColorShader"]->GetProgramID());
	glDisable(GL_DEPTH_TEST);
	model_matrix = glm::mat4(1);
	glUniformMatrix4fv(shaders["DullColorShader"]->GetUniformLocation(std::string("model_matrix")), 1, false, glm::value_ptr(model_matrix));
	
	glUniform3f(shaders["DullColorShader"]->GetUniformLocation(std::string("color")), 1, 1, 1);
	
	
	glUseProgram(shaders["DullColorShader"]->GetProgramID());
	glUniformMatrix4fv(shaders["DullColorShader"]->GetUniformLocation(std::string("view_matrix")), 1, false, glm::value_ptr(view_matrix));
	glUniformMatrix4fv(shaders["DullColorShader"]->GetUniformLocation(std::string("projection_matrix")), 1, false, glm::value_ptr(projection_matrix));
	//for (int i = 0; i < allBones.size(); i++)
	//{
	//	model_matrix = glm::translate(glm::mat4(1), allBones[i]->pos);
	//	glUniform3f(shaders["DullColorShader"]->GetUniformLocation(std::string("color")), 
	//		allBones[i]->color.r, allBones[i]->color.g, allBones[i]->color.b);
	//	glUniformMatrix4fv(shaders["DullColorShader"]->GetUniformLocation(std::string("model_matrix")), 1, false, glm::value_ptr(model_matrix));
	//	pointMesh->draw(GL_POINTS);
	//	for (int j = 0; j < allBones[i]->children.size(); j++)
	//	{
	//		DrawLine(shaders["DullColorShader"], allBones[i]->pos, allBones[i]->children[j]->pos, allBones[i]->color);
	//	}
	//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////// COLOR PICKING FB ///////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	RenderButtons();
	
	colorPickingFB.bind();
	
	glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, readPixels);
	
	glDeleteTextures(1, &quadTexture);
	
	quadTexture = loadTexture(readPixels, m_width, m_height);
	
	colorPickingFB.unbind();
}
void AudioVisualizer::DrawBorder()
{
#define BORDER_COLOR glm::vec3(0.1,0.05,0.05)
	DrawLine(shaders["DullColorShader"], glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), BORDER_COLOR, 20);
	DrawLine(shaders["DullColorShader"], glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0), BORDER_COLOR, 20);
	DrawLine(shaders["DullColorShader"], glm::vec3(-1, -1, 0), glm::vec3(-1, 1, 0), BORDER_COLOR, 20);
	DrawLine(shaders["DullColorShader"], glm::vec3(1, -1, 0), glm::vec3(1, 1, 0), BORDER_COLOR, 20);
}
////////////////////////////////////////////////////////////////////////////////
void AudioVisualizer::RenderButtons()
{
	glViewport(0, 0, m_width, m_height);
	
	m_sprite->SetCorners(glm::vec3(-0.5, 0.95, 0), glm::vec3(-0.05, 0.88, 0));
	m_sprite->Render(inTimePic);
	m_sprite->SetCorners(glm::vec3(-0.5 + 1, 0.95, 0), glm::vec3(-0.05 + 1, 0.88, 0));
	m_sprite->Render(outTimePic);
	m_sprite->SetCorners(glm::vec3(-0.5, 0.95 - 1, 0), glm::vec3(-0.05, 0.88 - 1, 0));
	m_sprite->Render(inFreqPic);
	m_sprite->SetCorners(glm::vec3(-0.5 + 1, 0.95 - 1, 0), glm::vec3(-0.05 + 1, 0.88 - 1, 0));
	m_sprite->Render(outFreqPic);


	m_sprite->SetCorners(glm::vec3(-1, 1, 0), glm::vec3(-0.9, 0.6, 0));
	m_sprite->Render(logoPic);

	//////////
	m_sprite->SetCorners(glm::vec3(-1, 0.45, 0), glm::vec3(-0.9, 0.35, 0));
	m_sprite->Render(audio.GetSettings().distortion ? buttonDown : buttonUp);
	m_sprite->Render(distortionPic);

	m_sprite->SetCorners(glm::vec3(-1, 0.3, 0), glm::vec3(-0.9, 0.2, 0));
	m_sprite->Render(audio.GetSettings().overdrive ? buttonDown : buttonUp);
	m_sprite->Render(overdrivePic);

	m_sprite->SetCorners(glm::vec3(-1, 0.15, 0), glm::vec3(-0.9, 0.05, 0));
	m_sprite->Render(audio.GetSettings().fuzz ? buttonDown : buttonUp);
	m_sprite->Render(fuzzPic);

	m_sprite->SetCorners(glm::vec3(-1, 0, 0), glm::vec3(-0.9, -0.1, 0));
	m_sprite->Render(audio.GetSettings().delay ? buttonDown : buttonUp);
	m_sprite->Render(delayPic);

	m_sprite->SetCorners(glm::vec3(-1, -0.15, 0), glm::vec3(-0.9, - 0.25, 0));
	m_sprite->Render(audio.GetSettings().reverb ? buttonDown : buttonUp);
	m_sprite->Render(reverbPic);

	m_sprite->SetCorners(glm::vec3(-1, -0.3, 0), glm::vec3(-0.9, -0.4, 0));
	m_sprite->Render(audio.GetSettings().compressor ? buttonDown : buttonUp);
	m_sprite->Render(compressorPic);

	m_sprite->SetCorners(glm::vec3(-1, -0.45, 0), glm::vec3(-0.9, -0.6, 0));
	m_sprite->Render(buttonUp);
	m_sprite->Render(recordPic);

	m_sprite->SetCorners(glm::vec3(-1, -0.65, 0), glm::vec3(-0.9, -0.8, 0));
	m_sprite->Render(buttonUp);
	m_sprite->Render(playWAVpic);


	//COLOR PICKING FB
	colorPickingFB.bind();
	//
	m_sprite->SetCorners(glm::vec3(-1, 0.45, 0), glm::vec3(-0.9, 0.35, 0));
	m_sprite->Render(glm::vec3(0.25, 0.25, 0.25));

	m_sprite->SetCorners(glm::vec3(-1, 0.3, 0), glm::vec3(-0.9, 0.2, 0));
	m_sprite->Render(glm::vec3(0.25, 0.25, 0.5));

	m_sprite->SetCorners(glm::vec3(-1, 0.15, 0), glm::vec3(-0.9, 0.05, 0));
	m_sprite->Render(glm::vec3(0.25, 0.5, 0.5));

	m_sprite->SetCorners(glm::vec3(-1, 0, 0), glm::vec3(-0.9, -0.1, 0));
	m_sprite->Render(glm::vec3(0.25, 0.5, 0.75));

	m_sprite->SetCorners(glm::vec3(-1, -0.15, 0), glm::vec3(-0.9, -0.25, 0));
	m_sprite->Render(glm::vec3(0.75, 0.5, 0.75));

	m_sprite->SetCorners(glm::vec3(-1, -0.3, 0), glm::vec3(-0.9, -0.4, 0));
	m_sprite->Render(glm::vec3(0.75, 0, 0.75));

	m_sprite->SetCorners(glm::vec3(-1, -0.45, 0), glm::vec3(-0.9, -0.6, 0));
	m_sprite->Render(glm::vec3(0.75, 0.25, 0.75));

	m_sprite->SetCorners(glm::vec3(-1, -0.65, 0), glm::vec3(-0.9, -0.8, 0));
	m_sprite->Render(buttonUp);
	m_sprite->Render(playWAVpic);

	colorPickingFB.unbind();
#define	DEBUG_MODE_READPIXELS
#ifdef DEBUG_MODE_READPIXELS
	if (showColorPickingFB)
		fsQuad->Render(quadTexture);
#endif

}
//functie chemata dupa ce am terminat cadrul de desenare (poate fi folosita pt modelare/simulare)
void notifyEndFrame() {}
//functei care e chemata cand se schimba dimensiunea ferestrei initiale
void AudioVisualizer::OnWindowResize(int width, int height)
{
	//mTextOutliner.Resize(width, height);
	//reshape
	if (height == 0) height = 1;
	m_width = width; m_height = height;
	float aspect = (float)width / (float)height;
	colorPickingFB.destroy();
	colorPickingFB.generate(width, height);
	free(readPixels);
	readPixels = (GLubyte*)malloc(3 * width * height);
	aspect = (float)(m_width - m_width / GUI_FRACTION ) / (float)height;
	projection_matrix = glm::perspective(45.0f, aspect, 1.f, 200.f);
}

void AudioVisualizer::FrameEnd()
{
	//DrawCoordinatSystem();
}

// Documentation for the input functions can be found in: "/Source/Core/Window/InputController.h" or
// https://github.com/UPB-Graphics/Framework-EGC/blob/master/Source/Core/Window/InputController.h

void AudioVisualizer::OnInputUpdate(float deltaTime, int mods)
{
	float speed = 2;

	if (!window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT))
	{
		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 right = GetSceneCamera()->transform->GetLocalOXVector();
		glm::vec3 forward = GetSceneCamera()->transform->GetLocalOZVector();
		forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
	}
}

void AudioVisualizer::OnKeyPress(int key, int mods)
{
	ImGuiIO& io = ImGui::GetIO();

	io.KeysDown[key] = true;
	
	if(key < 256)
		keyStates[key] = true;
	
	if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
	{
		glfwSetInputMode(window->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		m_altDown = true;
	}else	
	if (key == 27)
	{
		exit(0);
	}
	else
	if (key == 32)
	{
		for(auto s = shaders.begin(); s != shaders.end(); s++)
		{
			s->second->Reload();
		}			
	}else
	if (key == GLFW_KEY_F3)
	{
		showColorPickingFB = !showColorPickingFB;
	}
	else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
	{
		io.KeyShift = true;
	}
}

void AudioVisualizer::OnKeyRelease(int key, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeysDown[key] = false;
	if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
	{
		glfwSetInputMode(window->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		m_altDown = false;
		return;
	}
	if(key < 256)
		keyStates[key] = false;
	
}

//void AudioVisualizer::ForceRedraw()
//{
//	FrameStart();
//	Update(0.02);
//	FrameEnd();
//	window->SwapBuffers();
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioVisualizer::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	if (m_altDown)
	{
		float dx = (float)deltaX;
		float dy = (float)deltaY;

		if (m_LMB)
		{
		}
		else if (m_RMB)
		{
		}
		else if (m_MMB)
		{
		}
	}
	else
	{
		float dx = mouseX - prev_mousePos.x;
		float dy = prev_mousePos.y - mouseY;
		
	}	
	prev_mousePos = glm::ivec2(mouseX, mouseY);
}

void AudioVisualizer::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{	
	if (button == 1)
	{
		m_LMB = true;
		if (!m_altDown)
		{
			glm::uvec3 readPx = glm::uvec3(readPixels[(m_width * (m_height - mouseY) + mouseX) * 3],
				readPixels[(m_width * (m_height - mouseY) + mouseX) * 3 + 1],
				readPixels[(m_width * (m_height - mouseY) + mouseX) * 3 + 2]);
			

			if (readPx.r == 64 && readPx.g == 64 && readPx.b == 64)
			{
				audio.GetSettings().distortion = !audio.GetSettings().distortion;
				printf("Distortion is ");
				printf(audio.GetSettings().distortion ? "ON\n" : "OFF\n");
			}
			else if (readPx.r == 64 && readPx.g == 64 && readPx.b == 127)
			{
				audio.GetSettings().overdrive = !audio.GetSettings().overdrive;
				printf("Overdrive is ");
				printf(audio.GetSettings().overdrive ? "ON\n" : "OFF\n");
				
			}
			else if (readPx.r == 64 && readPx.g == 127 && readPx.b == 127)
			{
				audio.GetSettings().fuzz = !audio.GetSettings().fuzz;
				printf("Fuzz is ");
				printf(audio.GetSettings().fuzz ? "ON\n" : "OFF\n");

			}
			else if (readPx.r == 64 && readPx.g == 127 && readPx.b == 191)
			{
				audio.GetSettings().delay = !audio.GetSettings().delay;
				printf("Delay is ");
				printf(audio.GetSettings().delay ? "ON\n" : "OFF\n");
				
			}
			else if (readPx.r == 191 && readPx.g == 127 && readPx.b == 191) 
			{
				audio.GetSettings().reverb = !audio.GetSettings().reverb;
				printf("Reverb is ");
				printf(audio.GetSettings().reverb ? "ON\n" : "OFF\n");
				
			}
			else if (readPx.r == 191 && readPx.g == 0 && readPx.b == 191)
			{
				audio.GetSettings().compressor = !audio.GetSettings().compressor;
				printf("Compressor is ");
				printf(audio.GetSettings().compressor ? "ON\n" : "OFF\n");
				
				
			}
			else if (readPx.r == 191 && readPx.g == 64 && readPx.b == 191)
			{
				audio.PlayWAV(wavPath);
			}
		}
		else
		{
			//glfwSetCursorPos(window->GetGLFWWindow(), m_width / 2, m_height / 2);
		}
	}
	else if (button == 2)
	{
		m_RMB = true;
		if (!m_altDown) {
			//????????????????? TODO 
		}
	}
	else
		m_MMB = true;

}

void AudioVisualizer::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	if (button == 1)
		m_LMB = false;
	else if (button == 2)
		m_RMB = false;
	else
		m_MMB = false;
}

void AudioVisualizer::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
}