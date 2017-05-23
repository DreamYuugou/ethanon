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

#include "ProjectManager.h"
#include "EditorCommon.h"

#include "../engine/Scene/ETHScene.h"
#include "../engine/ETHEngine.h"

#include <Platform/Platform.h>

#define _S_NEW_PROJ GS_L("New project...")
#define _S_LOAD_PROJ GS_L("Load project...")

ProjectManager::ProjectManager(ETHResourceProviderPtr provider) :
	EditorBase(provider)
{
}

void ProjectManager::LoadEditor()
{
	CreateFileMenu();
	// get current program path
	m_curve = m_provider->GetVideo()->CreateSprite(GetProgramPath() + "data/curve_right.png");
}

void ProjectManager::SaveLastDir(const char *szLastDir)
{
	if (!szLastDir)
	{
		return;
	}
	SaveAttributeToInfoFile("directories", "lastProject", szLastDir);
}

std::string ProjectManager::ReadLastDir()
{
	return GetAttributeFromInfoFile("directories", "lastProject");
}

void ProjectManager::CreateFileMenu()
{
	m_fileMenu.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), _S_MENU_TITLE, m_menuSize*m_menuScale, m_menuWidth*2, true);
	m_fileMenu.AddButton(_S_NEW_PROJ);
	m_fileMenu.AddButton(_S_LOAD_PROJ);
}

std::string ProjectManager::DoEditor(SpritePtr pNextAppButton, const float lastFrameElapsedTime)
{
	GSGUI_BUTTON file_r = PlaceFileMenu();

	if (file_r.text == _S_NEW_PROJ)
	{
		SaveAs();
	}
	if (file_r.text == _S_LOAD_PROJ)
	{
		Open();
	}

	SetFileNameToTitle(m_provider->GetVideo(), _ETH_PROJECT_MANAGER_WINDOW_TITLE);

	if (GetCurrentProject() == "")
	{
		ShadowPrint(Vector2(64,128),
			GS_L("-Start a new project or load an existing one.\n") \
			GS_L("-If you start a new project, all necessary files and paths will be copied to the target folder.\n") \
			GS_L("-Media files (sprites, sounds, normals) should be manually placed into their correct directories.\n") \
			GS_L("-Create .ENT entity files before building your scenes.\n"));
	}
	else
	{
		str_type::string currentProjectFilename = 
			Platform::GetFileName(GetCurrentFile(false)).c_str();
			str_type::string sText = GS_L("Project loaded: ") + currentProjectFilename + GS_L("\n") +
			GS_L("We're ready to go. Press TAB or click the arrow to go to the Editors");
		ShadowPrint(Vector2(64,128), sText.c_str());
	}

	ProccessFileOpenRequests();
	return GetCurrentProject();
}

void ProjectManager::ProccessFileOpenRequests()
{
	if (!m_requestedProjectFileNameToOpen.empty())
	{
		SetCurrentProject(m_requestedProjectFileNameToOpen.c_str());
		SetCurrentFile(m_requestedProjectFileNameToOpen.c_str());
		SaveLastDir(Platform::GetFileDirectory(m_requestedProjectFileNameToOpen.c_str()).c_str());
	}
	m_requestedProjectFileNameToOpen.clear();
}

void ProjectManager::RequestProjectOpen(const std::string &path)
{
	m_requestedProjectFileNameToOpen = path;
}

bool ProjectManager::SaveAs()
{
	FILE_FORM_FILTER filter(GS_L("Ethanon Project files (*.ethproj)"), GS_L("ethproj"));
	char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
	std::string sLastDir = ReadLastDir();
	if (SaveForm(filter, sLastDir.c_str(), path, file))
	{
		std::string sOut;
		AddExtension(path, ".ethproj", sOut);
		std::ofstream ofs(sOut.c_str());
		if (ofs.is_open())
		{
			ofs << "Ethanon Engine project file";
			ofs.close();
		}
		SetCurrentFile(sOut.c_str());
		SetCurrentProject(sOut.c_str());
		SaveLastDir(Platform::GetFileDirectory(sOut.c_str()).c_str());
		PrepareProjectDir();
	}
	return true;
}

bool ProjectManager::Save()
{
	// dummy for a while because project files don't hold any internal data yet
	return true;
}

bool ProjectManager::Open()
{
	FILE_FORM_FILTER filter(GS_L("Ethanon Project files (*.ethproj)"), GS_L("ethproj"));
	char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
	std::string sLastDir = ReadLastDir();
	if (OpenForm(filter, sLastDir.c_str(), path, file))
	{
		RequestProjectOpen(path);
	}
	return true;
}

void ProjectManager::PrepareProjectDir()
{
	const ETH_FILE_TO_COPY fileToCopy[] = 
	{
		{"data/default_nm.png", true},
		{"data/shadow.dds", true},
		{"data/shadow.png", true},
		{"data/Verdana14_shadow.fnt", true},
		{"data/Verdana14_shadow_0.png", true},
		{"data/Verdana20.fnt", true},
		{"data/Verdana20_0.png", true},
		{"data/Verdana20_shadow.fnt", true},
		{"data/Verdana20_shadow.png", true},
		{"data/Verdana24.fnt", true},
		{"data/Verdana24_0.png", true},
		{"data/Verdana24_shadow.fnt", true},
		{"data/Verdana24_shadow.png", true},
		{"data/Verdana30.fnt", true},
		{"data/Verdana30_0.png", true},
		{"data/Verdana30_shadow.fnt", true},
		{"data/Verdana30_shadow.png", true},
		{"data/Verdana64.fnt", true},
		{"data/Verdana64_0.png", true},
		{"data/Verdana64_1.png", true},
		{"data/Verdana64_shadow.fnt", true},
		{"data/Verdana64_shadow_0.png", true},
		{"data/Verdana64_shadow_1.png", true},
		{"effects/readme.txt", true},
		{"entities/readme.txt", true},
		{"entities/normalmaps/readme.txt", true},
		{"particles/readme.txt", true},
		{"particles/particle.png", true},
		{"scenes/readme.txt", true},
		{"soundfx/readme.txt", true},
		{"app.enml", false},
		#if defined(WIN32) || defined(_WIN32) || defined(WINDOWS) || defined(__WIN32__)
		 {"machine.exe", true},
		 {"audiere.dll", true},
		 {"cg.dll", true},
		 {"cgD3D9.dll", true},
		 {"d3dx9_42.dll", true},
		#endif
		{_ETH_DEFAULT_MAIN_SCRIPT_FILE, false},
		{"eth_util.angelscript", false},
		{"Collide.angelscript", true},
	};

	const std::string programPath = m_provider->GetFileIOHub()->GetProgramDirectory();
	const std::string sProjectPath = GetCurrentProjectPath(true);
	const unsigned int size = sizeof(fileToCopy)/sizeof(ETH_FILE_TO_COPY);
	for (unsigned int t=0; t<size; t++)
	{
		const std::string sNewFolder = sProjectPath + fileToCopy[t].file;
		Platform::CreateDirectory(Platform::GetFileDirectory(sNewFolder.c_str()));
		ETHGlobal::_MoveFile(programPath + fileToCopy[t].file, sNewFolder, fileToCopy[t].overwrite);
	}
}
