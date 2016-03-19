/*--------------------------------------------------------------------------------------
 Ethanon Engine (C) Copyright 2008-2013 Andre Santee
 http://ethanonengine.com/

	Permission is hereby granted, free of charge, to any person obtaining a copy of this
	software and associated documentation files (the "Software"), to deal in the
	Software without restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the
	following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
	OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------------------------------*/

#include "ParticleFXEditor.h"
#include "EditorCommon.h"

#include <sstream>

#include "../engine/Resource/ETHDirectories.h"

#include "../engine/Util/ETHASUtil.h"

#include "../engine/Shader/ETHShaderManager.h"

#include <Platform/Platform.h>

#define LOAD_BMP GS_L("Load bitmap")
#define SAVE_SYSTEM GS_L("Save")
#define SAVE_SYSTEM_AS GS_L("Save as...")
#define OPEN_SYSTEM GS_L("Open")
#define LOAD_BG GS_L("Load background")
#define WINDOW_TITLE GS_L("Ethanon ParticleFX Editor")
#define REPLAY_MESSAGE GS_L("Press space or click anywhere to play again")

#define ALPHA_MODE_PIXEL GS_L("Pixel")
#define ALPHA_MODE_ADD GS_L("Additive")
#define ALPHA_MODE_MODULATE GS_L("Modulate")

#define ANIMATION_MODE_ANIMATE GS_L("Animate")
#define ANIMATION_MODE_PICK GS_L("Pick random frame")

ParticleEditor::ParticleEditor(ETHResourceProviderPtr provider) :
	EditorBase(provider),
	m_systemAngle(0.0f),
	m_projManagerRequested(false),
	BSPHERE_COLOR(50,255,255,255),
	BSPHERE_BMP("data/bs.png")
{
}

ParticleEditor::~ParticleEditor()
{
}

void ParticleEditor::CreateParticles()
{
	m_manager = ETHParticleManagerPtr(new ETHParticleManager(m_provider, m_system, Vector3(m_v2Pos, 0), m_systemAngle, 1.0f));
}

bool ParticleEditor::ProjectManagerRequested()
{
	const bool r = m_projManagerRequested;
	m_projManagerRequested = false;
	return r;
}

void ParticleEditor::DrawParticleSystem()
{	
	m_manager->SetZPosition(0.0f);
	const VideoPtr& video = m_provider->GetVideo();
	const InputPtr& input = m_provider->GetInput();
	const Vector2 v2Screen = video->GetScreenSizeF();
	const bool overMenu = (input->GetCursorPosition(video).x<(int)m_menuWidth*2);
	if (input->GetLeftClickState() == GSKS_DOWN && !overMenu)
		m_v2Pos = input->GetCursorPositionF(video);
	else
		m_v2Pos = video->GetScreenSizeF()/2;

	if (input->GetKeyState(GSK_LEFT) == GSKS_DOWN)
		m_systemAngle+=2;
	if (input->GetKeyState(GSK_RIGHT) == GSKS_DOWN)
		m_systemAngle-=2;
	if (input->GetKeyState(GSK_UP) == GSKS_DOWN)
		m_systemAngle = 0.0f;
	if (input->GetKeyState(GSK_DOWN) == GSKS_DOWN)
		m_systemAngle = 180.0f;

	m_timer.CalcLastFrame();
	m_manager->UpdateParticleSystem(Vector3(m_v2Pos, 0), m_systemAngle, static_cast<float>(m_timer.GetElapsedTime() * 1000.0));

	video->SetScissor(Rect2D((int)m_menuWidth*2, 0, video->GetScreenSize().x-(int)m_menuWidth*2, video->GetScreenSize().y));

	// Draw bounding rect
	const Rect2Df boundingSquareSize(m_manager->ComputeBoundingSquare(m_systemAngle));
	video->DrawRectangle(
		boundingSquareSize.pos + m_v2Pos - (boundingSquareSize.size * 0.5f),
		boundingSquareSize.size,
		Color(0x11FFFFFF));

	const bool zBuffer = video->GetZBuffer();
	video->SetZBuffer(false);
	
	if (m_provider->GetShaderManager()->BeginParticlePass(m_system))
	{
		m_manager->DrawParticleSystem(Vector3(1,1,1),v2Screen.y,-v2Screen.y, ETHParticleManager::SAME_DEPTH_AS_OWNER, ETH_DEFAULT_ZDIRECTION, Vector2(0,0), 1.0f);
		m_provider->GetShaderManager()->EndParticlePass();
		
	}
	video->SetZBuffer(zBuffer);
	video->UnsetScissor();

	if (m_manager->Finished() || m_manager->Killed())
	{
		ShadowPrint(Vector2(200.0f, video->GetScreenSizeF().y/2), REPLAY_MESSAGE, GS_L("Verdana14_shadow.fnt"), gs2d::constant::WHITE);
		if (input->GetKeyState(GSK_SPACE) == GSKS_HIT || input->GetLeftClickState() == GSKS_HIT)
		{
			m_manager->Play(Vector3(m_v2Pos, 0), m_systemAngle);
			m_manager->Kill(false);
		}
	}

	str_type::stringstream ss;
	ss << m_system.bitmapFile;
	ss << GS_L(" | ") << GS_L("Active particles: ") << m_manager->GetNumActiveParticles() << GS_L("/") << m_manager->GetNumParticles();
	const float infoTextSize = m_menuSize * m_menuScale;
	ShadowPrint(Vector2(m_menuWidth*2+5,v2Screen.y-infoTextSize-m_menuSize), ss.str().c_str(), GS_L("Verdana14_shadow.fnt"), gs2d::constant::WHITE);
}

void ParticleEditor::SetupMenu()
{
	m_fileMenu.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), GS_L("Menu"), m_menuSize*m_menuScale, m_menuWidth*2, true);
	m_fileMenu.AddButton(SAVE_SYSTEM);
	m_fileMenu.AddButton(SAVE_SYSTEM_AS);
	m_fileMenu.AddButton(OPEN_SYSTEM);
	m_fileMenu.AddButton(LOAD_BMP);
	m_fileMenu.AddButton(LOAD_BG);
	m_fileMenu.AddButton(_S_GOTO_PROJ);

	m_alphaModes.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth*2, true, false, false);
	m_alphaModes.AddButton(ALPHA_MODE_PIXEL, true);
	m_alphaModes.AddButton(ALPHA_MODE_ADD, false);
	m_alphaModes.AddButton(ALPHA_MODE_MODULATE, false);

	m_animationModes.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth*2, true, false, false);
	m_animationModes.AddButton(ANIMATION_MODE_ANIMATE, true);
	m_animationModes.AddButton(ANIMATION_MODE_PICK, false);

	SetMenuConstants();
}

void ParticleEditor::SetMenuConstants()
{
	m_particles.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_particles.SetConstant(m_system.nParticles);
	m_particles.SetScrollAdd(1);
	m_particles.SetClamp(true, 0, 10000);
	m_particles.SetText(GS_L("Ptcles:"));

	m_repeats.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_repeats.SetScrollAdd(1);
	m_repeats.SetConstant(m_system.repeat);
	m_repeats.SetClamp(true, 0, 9999999);
	m_repeats.SetText(GS_L("Repeats:"));

	m_gravity[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_gravity[0].SetConstant(m_system.gravityVector.x);
	m_gravity[0].SetClamp(false, 0, 0);
	m_gravity[0].SetText(GS_L("Grav X:"));
	m_gravity[0].SetDescription(GS_L("Gravity vector"));
	m_gravity[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_gravity[1].SetConstant(m_system.gravityVector.y);
	m_gravity[1].SetClamp(false, 0, 0);
	m_gravity[1].SetText(GS_L("Grav Y:"));
	m_gravity[1].SetDescription(GS_L("Gravity vector"));

	m_direction[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_direction[0].SetConstant(m_system.directionVector.x);
	m_direction[0].SetClamp(false, 0, 0);
	m_direction[0].SetText(GS_L("Dir X:"));
	m_direction[0].SetDescription(GS_L("Particle direction"));
	m_direction[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_direction[1].SetConstant(m_system.directionVector.y);
	m_direction[1].SetClamp(false, 0, 0);
	m_direction[1].SetText(GS_L("Dir Y:"));
	m_direction[1].SetDescription(GS_L("Particle direction"));

	m_randDir[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randDir[0].SetConstant(m_system.randomizeDir.x);
	m_randDir[0].SetClamp(false, 0, 0);
	m_randDir[0].SetText(GS_L("R Dir X:"));
	m_randDir[0].SetDescription(GS_L("Particle direction randomizer"));
	m_randDir[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randDir[1].SetConstant(m_system.randomizeDir.y);
	m_randDir[1].SetClamp(false, 0, 0);
	m_randDir[1].SetText(GS_L("R Dir Y:"));
	m_randDir[1].SetDescription(GS_L("Particle direction randomizer"));

	m_startPoint[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_startPoint[0].SetConstant(m_system.startPoint.x);
	m_startPoint[0].SetClamp(false, 0, 0);
	m_startPoint[0].SetText(GS_L("Start X:"));
	m_startPoint[0].SetScrollAdd(1.0f);
	m_startPoint[0].SetDescription(GS_L("Particle starting position"));
	m_startPoint[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_startPoint[1].SetConstant(m_system.startPoint.y);
	m_startPoint[1].SetClamp(false, 0, 0);
	m_startPoint[1].SetText(GS_L("Start Y:"));
	m_startPoint[1].SetScrollAdd(1.0f);
	m_startPoint[1].SetDescription(GS_L("Particle starting position"));

	m_randStart[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randStart[0].SetConstant(m_system.randStartPoint.x);
	m_randStart[0].SetClamp(false, 0, 0);
	m_randStart[0].SetText(GS_L("R StartX:"));
	m_randStart[0].SetScrollAdd(1.0f);
	m_randStart[0].SetDescription(GS_L("Starting position randomizer"));
	m_randStart[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randStart[1].SetConstant(m_system.randStartPoint.y);
	m_randStart[1].SetClamp(false, 0, 0);
	m_randStart[1].SetText(GS_L("R StartY:"));
	m_randStart[1].SetScrollAdd(1.0f);
	m_randStart[1].SetDescription(GS_L("Starting position randomizer"));

	m_color0[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color0[0].SetConstant(m_system.color0.w);
	m_color0[0].SetClamp(true, 0, 1);
	m_color0[0].SetText(GS_L("Color0.A:"));
	m_color0[0].SetDescription(GS_L("Starting color alpha (transparency from 0.0 to 1.0)"));
	m_color0[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color0[1].SetConstant(m_system.color0.x);
	m_color0[1].SetClamp(true, 0, 8);
	m_color0[1].SetText(GS_L("Color0.R:"));
	m_color0[1].SetDescription(GS_L("Starting color red component (from 0.0 to 1.0)"));
	m_color0[2].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color0[2].SetConstant(m_system.color0.y);
	m_color0[2].SetClamp(true, 0, 8);
	m_color0[2].SetText(GS_L("Color0.G:"));
	m_color0[2].SetDescription(GS_L("Starting color green component (from 0.0 to 1.0)"));
	m_color0[3].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color0[3].SetConstant(m_system.color0.z);
	m_color0[3].SetClamp(true, 0, 8);
	m_color0[3].SetText(GS_L("Color0.B:"));
	m_color0[3].SetDescription(GS_L("Starting color blue component (from 0.0 to 1.0)"));

	m_color1[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color1[0].SetConstant(m_system.color1.w);
	m_color1[0].SetClamp(true, 0, 1);
	m_color1[0].SetText(GS_L("Color1.A:"));
	m_color1[0].SetDescription(GS_L("Ending color alpha (transparency)"));
	m_color1[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color1[1].SetConstant(m_system.color1.x);
	m_color1[1].SetClamp(true, 0, 8);
	m_color1[1].SetText(GS_L("Color1.R:"));
	m_color1[1].SetDescription(GS_L("Ending color red component (from 0.0 to 1.0)"));
	m_color1[2].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color1[2].SetConstant(m_system.color1.y);
	m_color1[2].SetClamp(true, 0, 8);
	m_color1[2].SetText(GS_L("Color1.G:"));
	m_color1[2].SetDescription(GS_L("Ending color green component (from 0.0 to 1.0)"));
	m_color1[3].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_color1[3].SetConstant(m_system.color1.z);
	m_color1[3].SetClamp(true, 0, 8);
	m_color1[3].SetText(GS_L("Color1.B:"));
	m_color1[3].SetDescription(GS_L("Ending color blue component (from 0.0 to 1.0)"));

	m_luminance[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_luminance[0].SetConstant(m_system.emissive.x);
	m_luminance[0].SetClamp(true, 0, 1);
	m_luminance[0].SetText(GS_L("Emissive.R:"));
	m_luminance[0].SetDescription(GS_L("Self-illumination color red component (color = diffuse*min(ambient+luminance,1))"));
	m_luminance[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_luminance[1].SetConstant(m_system.emissive.y);
	m_luminance[1].SetClamp(true, 0, 1);
	m_luminance[1].SetText(GS_L("Emissive.G:"));
	m_luminance[1].SetDescription(GS_L("Self-illumination color green component (color = diffuse*min(ambient+luminance,1))"));
	m_luminance[2].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_luminance[2].SetConstant(m_system.emissive.z);
	m_luminance[2].SetClamp(true, 0, 1);
	m_luminance[2].SetText(GS_L("Emissive.B:"));
	m_luminance[2].SetDescription(GS_L("Self-illumination color blue component (color = diffuse*min(ambient+luminance,1))"));

	m_size.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_size.SetConstant(m_system.size);
	m_size.SetClamp(true, 0, 9999999.0f);
	m_size.SetText(GS_L("Size:"));
	m_size.SetScrollAdd(1.0f);
	m_size.SetDescription(GS_L("Starting size"));

	m_growth.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_growth.SetConstant(m_system.growth);
	m_growth.SetClamp(false, 0, 0);
	m_growth.SetText(GS_L("Size+:"));
	m_growth.SetDescription(GS_L("Particle growth"));

	m_lifeTime.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_lifeTime.SetConstant(m_system.lifeTime);
	m_lifeTime.SetClamp(true, 0, 9999999.0f);
	m_lifeTime.SetText(GS_L("Time:"));
	m_lifeTime.SetScrollAdd(50.0f);
	m_lifeTime.SetDescription(GS_L("Particle life time (in milliseconds)"));

	m_randLifeTime.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randLifeTime.SetConstant(m_system.randomizeLifeTime);
	m_randLifeTime.SetClamp(true, 0, 9999999.0f);
	m_randLifeTime.SetText(GS_L("R Time:"));
	m_randLifeTime.SetScrollAdd(50.0f);
	m_randLifeTime.SetDescription(GS_L("Particle life time randomizer (in milliseconds)"));

	m_angle.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_angle.SetConstant(m_system.angleDir);
	m_angle.SetClamp(false, 0, 0);
	m_angle.SetText(GS_L("Angle Dir:"));
	m_angle.SetDescription(GS_L("Rotating direction"));

	m_randAngle.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randAngle.SetConstant(m_system.randAngle);
	m_randAngle.SetClamp(false, 0, 0);
	m_randAngle.SetText(GS_L("R Angle Dir:"));
	m_randAngle.SetDescription(GS_L("Rotating direction randomizer"));

	m_randSize.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randSize.SetConstant(m_system.randomizeSize);
	m_randSize.SetClamp(true, 0, 1000000);
	m_randSize.SetText(GS_L("R Size:"));
	m_randSize.SetScrollAdd(1.0f);
	m_randSize.SetDescription(GS_L("Particle size randomizer"));

	m_randAngleStart.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_randAngleStart.SetConstant(m_system.randAngleStart);
	m_randAngleStart.SetClamp(true, 0, 360);
	m_randAngleStart.SetText(GS_L("R Angle St:"));
	m_randAngleStart.SetScrollAdd(1.0f);
	m_randAngleStart.SetDescription(GS_L("Starting angle randomizer (from 0 to 360)"));

	m_minSize.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_minSize.SetConstant(m_system.minSize);
	m_minSize.SetClamp(false, 0, 0);
	m_minSize.SetText(GS_L("Min Size:"));
	m_minSize.SetScrollAdd(1.0f);
	m_minSize.SetDescription(GS_L("Particle minimum size"));

	m_maxSize.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_maxSize.SetConstant(m_system.maxSize);
	m_maxSize.SetClamp(false, 0, 0);
	m_maxSize.SetText(GS_L("Max Size:"));
	m_maxSize.SetScrollAdd(1.0f);
	m_maxSize.SetDescription(GS_L("Particle maximum size"));

	m_angleStart.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_angleStart.SetConstant(m_system.angleStart);
	m_angleStart.SetClamp(true, 0, 360);
	m_angleStart.SetText(GS_L("S Angle:"));
	m_angleStart.SetScrollAdd(1.0f);
	m_angleStart.SetDescription(GS_L("Particle starting angle"));

	m_allAtOnce.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_allAtOnce.SetConstant((m_system.allAtOnce) ? 1.0f : 0.0f);
	m_allAtOnce.SetClamp(true, 0, 1);
	m_allAtOnce.SetText(GS_L("All at once:"));
	m_allAtOnce.SetScrollAdd(1.0f);
	m_allAtOnce.SetDescription(GS_L("All-at-once particle relasing (1=true/0=false)"));

	m_spriteCut[0].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_spriteCut[0].SetConstant(static_cast<float>(m_system.spriteCut.x));
	m_spriteCut[0].SetClamp(true, 1, 9999);
	m_spriteCut[0].SetText(GS_L("Sprite columns:"));
	m_spriteCut[0].SetScrollAdd(1.0f);
	m_spriteCut[0].SetDescription(GS_L("Number of columns of the animated sprite"));
	m_spriteCut[1].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 9, false);
	m_spriteCut[1].SetConstant(static_cast<float>(m_system.spriteCut.y));
	m_spriteCut[1].SetClamp(true, 1, 9999);
	m_spriteCut[1].SetText(GS_L("Sprite rows:"));
	m_spriteCut[1].SetScrollAdd(1.0f);
	m_spriteCut[1].SetDescription(GS_L("Number of rows of the animated sprite"));

	m_alphaModes.ResetButtons();
	if (m_system.alphaMode == Video::AM_PIXEL)
		m_alphaModes.ActivateButton(ALPHA_MODE_PIXEL);
	if (m_system.alphaMode == Video::AM_ADD)
		m_alphaModes.ActivateButton(ALPHA_MODE_ADD);
	if (m_system.alphaMode == Video::AM_MODULATE)
		m_alphaModes.ActivateButton(ALPHA_MODE_MODULATE);

	m_animationModes.ResetButtons();
	if (m_system.animationMode == ETHParticleSystem::PLAY_ANIMATION)
		m_animationModes.ActivateButton(ANIMATION_MODE_ANIMATE);
	if (m_system.animationMode == ETHParticleSystem::PICK_RANDOM_FRAME)
		m_animationModes.ActivateButton(ANIMATION_MODE_PICK);
}

void ParticleEditor::SaveAs()
{
	char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
	if (SaveSystem(path, file))
	{
		strcpy(path, ETHGlobal::AppendExtensionIfNeeded(path, ".par").c_str());

		TiXmlDocument doc;
		TiXmlDeclaration *pDecl = new TiXmlDeclaration(GS_L("1.0"), GS_L(""), GS_L(""));
		doc.LinkEndChild(pDecl);

		TiXmlElement *pElement = new TiXmlElement(GS_L("Ethanon"));
		doc.LinkEndChild(pElement);

		m_system.WriteToXMLFile(doc.RootElement());
		const str_type::string filePath = path;
		doc.SaveFile(filePath);

		SetCurrentFile(path);
		m_untitled = false;
	}
}

void ParticleEditor::Save()
{
	TiXmlDocument doc;
	TiXmlDeclaration *pDecl = new TiXmlDeclaration(GS_L("1.0"), GS_L(""), GS_L(""));
	doc.LinkEndChild(pDecl);

	TiXmlElement *pElement = new TiXmlElement(GS_L("Ethanon"));
	doc.LinkEndChild(pElement);

	m_system.WriteToXMLFile(doc.RootElement());
	const str_type::string filePath = GetCurrentFile(true);
	doc.SaveFile(filePath);

	m_untitled = false;
}

void ParticleEditor::StopAllSoundFXs()
{
}

void ParticleEditor::Clear()
{
	m_system.Reset();
	ResetSystem();
	SetMenuConstants();

	SetCurrentFile(_BASEEDITOR_DEFAULT_UNTITLED_FILE);
	m_untitled = true;
}

void ParticleEditor::ParticlePanel()
{
	std::string programPath = GetCurrentProjectPath(false);
	const VideoPtr& video = m_provider->GetVideo();
	const InputPtr& input = m_provider->GetInput();

	GSGUI_BUTTON file_r = m_fileMenu.PlaceMenu(Vector2(0,m_menuSize*2));
	if (file_r.text == LOAD_BMP)
	{
		char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
		if (OpenParticleBMP(path, file))
		{
			ETHGlobal::CopyFileToProject(programPath, path, ETHDirectories::GetParticlesDirectory(), m_provider->GetFileManager());
			m_system.bitmapFile = file;
			m_provider->GetGraphicResourceManager()->ReleaseResource(m_system.bitmapFile);
			m_manager = ETHParticleManagerPtr(
				new ETHParticleManager(m_provider, m_system, Vector3(m_v2Pos, 0), m_systemAngle, 1.0f));
		}
	}

	if (file_r.text == SAVE_SYSTEM)
	{
		if (m_untitled)
			SaveAs();
		else
			Save();
	}

	if (file_r.text == SAVE_SYSTEM_AS)
	{
		SaveAs();
	}

	if (file_r.text == LOAD_BG)
	{
		char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
		if (OpenParticleBMP(path, file))
		{
			m_backgroundSprite = video->CreateSprite(path);
		}	
	}

	if (file_r.text == OPEN_SYSTEM)
	{
		m_systemAngle = 0.0f;
		char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
		if (OpenSystem(path, file))
		{
			m_manager = ETHParticleManagerPtr(
				new ETHParticleManager(m_provider, path, Vector3(m_v2Pos, 0), m_systemAngle));
			m_manager->SetZPosition(0.0f);
			m_manager->Kill(false);
			m_system = *m_manager->GetSystem();
			SetMenuConstants();

			SetCurrentFile(path);
			m_untitled = false;
		}
	}

	if (file_r.text == _S_GOTO_PROJ)
	{
		m_projManagerRequested = true;
	}

	if (input->GetKeyState(GSK_K) == GSKS_HIT)
		m_manager->Kill(!m_manager->Killed());
	
	if (!m_fileMenu.IsActive())
	{
		Vector2 v2ScreenDim = video->GetScreenSizeF();
		float menu = m_menuSize*m_menuScale+(m_menuSize*2);

		// places the alpha mode menu
		ShadowPrint(Vector2(v2ScreenDim.x-m_alphaModes.GetWidth(), menu), GS_L("Alpha mode:"), GS_L("Verdana14_shadow.fnt"), gs2d::constant::WHITE);
		menu += m_menuSize;
		m_alphaModes.PlaceMenu(Vector2(v2ScreenDim.x-m_alphaModes.GetWidth(), menu)); menu += m_alphaModes.GetNumButtons()*m_menuSize;

		// sets the alpha mode according to the selected item
		if (m_alphaModes.GetButtonStatus(ALPHA_MODE_PIXEL))
			m_system.alphaMode = Video::AM_PIXEL;
		if (m_alphaModes.GetButtonStatus(ALPHA_MODE_ADD))
			m_system.alphaMode = Video::AM_ADD;
		if (m_alphaModes.GetButtonStatus(ALPHA_MODE_MODULATE))
			m_system.alphaMode = Video::AM_MODULATE;

		// places the sprite cut fields to the right
		menu += m_menuSize/2;
		ShadowPrint(Vector2(v2ScreenDim.x-m_alphaModes.GetWidth(), menu), GS_L("Sprite cut:"), GS_L("Verdana14_shadow.fnt"), gs2d::constant::WHITE);
		menu += m_menuSize;
		m_system.spriteCut.x = Max(1, static_cast<int>(m_spriteCut[0].PlaceInput(Vector2(v2ScreenDim.x-m_alphaModes.GetWidth(),menu)))); menu += m_menuSize;
		m_system.spriteCut.y = Max(1, static_cast<int>(m_spriteCut[1].PlaceInput(Vector2(v2ScreenDim.x-m_alphaModes.GetWidth(),menu)))); menu += m_menuSize;

		// if there is sprite animation in the particle system, places the animation mode selector
		if (m_system.spriteCut.x > 1 || m_system.spriteCut.y > 1)
		{
			menu += m_menuSize/2;
			m_animationModes.PlaceMenu(Vector2(v2ScreenDim.x-m_animationModes.GetWidth(), menu)); menu += m_animationModes.GetNumButtons()*m_menuSize;

			if (m_animationModes.GetButtonStatus(ANIMATION_MODE_ANIMATE))
				m_system.animationMode = ETHParticleSystem::PLAY_ANIMATION;
			if (m_animationModes.GetButtonStatus(ANIMATION_MODE_PICK))
				m_system.animationMode = ETHParticleSystem::PICK_RANDOM_FRAME;
		}

		// inputs all data
		menu = m_menuSize*m_menuScale+(m_menuSize*2);
		menu += m_menuSize/2;
		ShadowPrint(Vector2(0.0f,menu), GS_L("Particles:"), GS_L("Verdana14_shadow.fnt"), gs2d::constant::WHITE);
		int nParticles = m_particles.PlaceInput(Vector2(m_menuWidth,menu)); menu += m_menuSize;

		ShadowPrint(Vector2(0.0f,menu), GS_L("Repeats:"), GS_L("Verdana14_shadow.fnt"), gs2d::constant::WHITE);
		m_system.repeat = m_repeats.PlaceInput(Vector2(m_menuWidth,menu)); menu += m_menuSize;
		menu += m_menuSize/2;

		if (nParticles != m_system.nParticles && nParticles > 0)
		{
			m_system.nParticles = nParticles;
			m_manager = ETHParticleManagerPtr(
				new ETHParticleManager(m_provider, m_system, Vector3(m_v2Pos, 0), m_systemAngle, 1.0f));
			m_manager->Kill(false);
		}

		m_system.gravityVector.x = m_gravity[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.gravityVector.y = m_gravity[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;

		m_system.directionVector.x = m_direction[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.directionVector.y = m_direction[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;

		m_system.randomizeDir.x = m_randDir[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.randomizeDir.y = m_randDir[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.startPoint.x = m_startPoint[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.startPoint.y = m_startPoint[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.randStartPoint.x = m_randStart[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.randStartPoint.y = m_randStart[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.color0.w = m_color0[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.color0.x = m_color0[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.color0.y = m_color0[2].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.color0.z = m_color0[3].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.color1.w = m_color1[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.color1.x = m_color1[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.color1.y = m_color1[2].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.color1.z = m_color1[3].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.size = m_size.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.growth = m_growth.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.randomizeSize = m_randSize.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.lifeTime = m_lifeTime.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.randomizeLifeTime = m_randLifeTime.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.angleStart = m_angleStart.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.angleDir = m_angle.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.randAngle = m_randAngle.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.randAngleStart = m_randAngleStart.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.minSize = m_minSize.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.maxSize = m_maxSize.PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_system.emissive.x = m_luminance[0].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.emissive.y = m_luminance[1].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		m_system.emissive.z = m_luminance[2].PlaceInput(Vector2(0.0f,menu), Vector2(0.0f, v2ScreenDim.y-m_menuSize), m_menuSize); menu += m_menuSize;
		menu += m_menuSize/2;

		m_manager->SetSystem(m_system);
	}
}

std::string ParticleEditor::DoEditor(SpritePtr pNextAppButton)
{
	if (!m_manager)
		CreateParticles();
	const VideoPtr& video = m_provider->GetVideo();
	video->SetCameraPos(Vector2(0,0));
	video->SetBGColor(m_background);
	if (m_backgroundSprite)
		m_backgroundSprite->DrawShaped(Vector2(0,0), video->GetScreenSizeF(), gs2d::constant::WHITE, gs2d::constant::WHITE, gs2d::constant::WHITE, gs2d::constant::WHITE);
	ParticlePanel();
	DrawParticleSystem();
	SetFileNameToTitle(video, WINDOW_TITLE);
	return "particle";
}

void ParticleEditor::LoadEditor()
{
	m_sphereSprite = m_provider->GetVideo()->CreateSprite(m_provider->GetFileIOHub()->GetProgramDirectory() + "/" + BSPHERE_BMP);
	ResetSystem();
	SetupMenu();
}

void ParticleEditor::ResetSystem()
{
	m_system.alphaMode = Video::AM_PIXEL;
	m_system.nParticles = 50;
	m_system.gravityVector = Vector2(0.0f, 0.01f);
	m_system.directionVector = Vector2(0.0f,-2.5f);
	m_system.randomizeDir = Vector2(1.5f, 0.2f);

	m_system.startPoint = Vector3(0.0f, 0.0f, 0.0f);
	m_system.randStartPoint = Vector2(0.0f, 0.0f);

	m_system.color0 = Vector4(1, 1, 0, 1);
	m_system.color1 = Vector4(1, 0, 0, 0);

	m_system.lifeTime = 1200.0f;
	m_system.randomizeLifeTime = 1000.0f;

	m_system.angleDir = 0.0f;
	m_system.randAngle = 0.0f;
	m_system.size = 8.0f;
	m_system.randomizeSize = 0.0f;
	m_system.growth = 1.5f;
	m_system.minSize = 0.0f;
	m_system.maxSize = 1000.0f;
	m_system.randAngleStart = 0.0f;
	m_system.angleStart = 0.0f;
	m_system.emissive = Vector3(1,1,1);
	m_system.allAtOnce = false;

	m_manager = ETHParticleManagerPtr(
		new ETHParticleManager(
			m_provider,
			m_system,
			Vector3(m_v2Pos, 0),
			m_systemAngle,
			1.0f));
}

bool ParticleEditor::OpenParticleBMP(char *oFilePathName, char *oFileName)
{
	FILE_FORM_FILTER filter(GS_L("Supported image files"), GS_L("png,bmp,tga,jpg,jpeg,dds"));
	std::string initDir = GetCurrentProjectPath(true) + "particles/";
	Platform::FixSlashes(initDir);
	return OpenForm(filter, initDir.c_str(), oFilePathName, oFileName);
}

bool ParticleEditor::OpenSystem(char *oFilePathName, char *oFileName)
{
	FILE_FORM_FILTER filter(GS_L("Particle effect file (*.par)"), GS_L("par"));
	std::string initDir = GetCurrentProjectPath(true) + "effects/";
	Platform::FixSlashes(initDir);
	return OpenForm(filter, initDir.c_str(), oFilePathName, oFileName);
}

bool ParticleEditor::SaveSystem(char *oFilePathName, char *oFileName)
{
	FILE_FORM_FILTER filter(GS_L("Particle effect file (*.par)"), GS_L("par"));
	std::string initDir = GetCurrentProjectPath(true) + "effects/";
	Platform::FixSlashes(initDir);
	return SaveForm(filter, initDir.c_str(), oFilePathName, oFileName);
}
