#include "Editor.h"
#include "AssetWindow.h"
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/Profiler.h>

#if __has_include(<filesystem>)
#	include <filesystem>
#elif __has_include(<experimental/filesystem>)
#	include <experimental/filesystem>
#endif

#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>

namespace Lumos
{

#ifdef LUMOS_PLATFORM_WINDOWS
	std::string AssetWindow::m_Delimiter = "\\";
#else
	std::string AssetWindow::m_Delimiter = "/";
#endif
	AssetWindow::AssetWindow()
	{
		LUMOS_PROFILE_FUNCTION();
		m_Name = "AssetWindow";
		m_SimpleName = "Assets";

    #ifdef LUMOS_PLATFORM_IOS
		m_BaseDirPath = "Assets/";
    #else
        m_BaseDirPath = ROOT_DIR "/Sandbox/Assets/";
    #endif
		m_CurrentDirPath = m_BaseDirPath;
		m_prevDirPath = m_CurrentDirPath;
		m_lastNavPath = m_BaseDirPath;
		m_BaseProjectDir = GetFsContents(m_BaseDirPath);
		m_CurrentDir = m_BaseProjectDir;
		m_basePathLen = strlen(m_BaseDirPath.c_str());

		m_IsDragging = false;
		m_isInListView = true;
		m_updateBreadCrumbs = true;
		m_showSearchBar = false;
	}

	void AssetWindow::OnImGui()
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::Begin(m_SimpleName.c_str());
		{
			ImGui::Columns(2, "AB", true);            

			ImGui::BeginChild("##folders_common");
			{
				if(ImGui::CollapsingHeader("Assets://", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					if(ImGui::TreeNode("Contents"))
					{
						for(int i = 0; i < m_BaseProjectDir.size(); i++)
						{
							if(ImGui::TreeNode(m_BaseProjectDir[i].filename.c_str()))
							{
								auto dirData = ReadDirectory(m_BaseProjectDir[i].absolutePath.c_str());
								for(int d = 0; d < dirData.size(); d++)
								{
									if(!dirData[d].isFile)
									{
										if(ImGui::TreeNode(dirData[d].filename.c_str()))
										{
											ImGui::TreePop();
										}
									}
									else
									{
										auto parentDir = GetParentPath(dirData[d].absolutePath);
										ImGui::Indent();
										ImGui::Selectable(dirData[d].filename.c_str(), false);
										ImGui::Unindent();
									}
								}
								ImGui::TreePop();
							}

							if(m_IsDragging && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
							{
								m_MovePath = m_BaseProjectDir[i].absolutePath.c_str();
							}
						}
						ImGui::TreePop();
					}

					if(ImGui::IsMouseDown(1))
					{
						ImGui::OpenPopup("window");
					}
				}

				ImGui::EndChild();
			}

			if(ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
				if(data)
				{
					std::string file = (char*)data->Data;
					if(MoveFile(file, m_MovePath))
					{
						LUMOS_LOG_INFO("Moved File: " + file + " to " + m_MovePath);
						m_CurrentDir = ReadDirectory(m_CurrentDirPath);
					}
					m_IsDragging = false;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetColumnWidth() - 12, 250));
			{
				RenderBreadCrumbs();
				ImGui::EndChild();

				ImGui::BeginChild("Scrolling");

				if(!m_isInListView)
					ImGui::Columns(17, nullptr, false);

				for(int i = 0; i < m_CurrentDir.size(); i++)
				{
					if(m_CurrentDir.size() > 0)
					{
						if(!m_CurrentDir[i].isFile)
						{
							if(!m_isInListView)
								RenderDircGridView(i);
							else
								RenderDircListView(i);
						}
						else
						{
							if(!m_isInListView)
								RenderFileGridView(i);
							else
								RenderFileListView(i);
						}

						ImGui::NextColumn();
					}
				}

				ImGui::EndChild();
				ImGui::EndChild();
			}

			if(ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
				if(data)
				{
					std::string a = (char*)data->Data;
					if(MoveFile(a, m_MovePath))
					{
						LUMOS_LOG_INFO("Moved File: " + a + " to " + m_MovePath);
					}
					m_IsDragging = false;
				}
				ImGui::EndDragDropTarget();
			}

			if(ImGui::BeginMenuBar())
			{
				if(ImGui::BeginMenu("Create"))
				{
					if(ImGui::MenuItem("Import New Asset", "Ctrl + O"))
					{
						m_Editor->OpenFile();
					}

					if(ImGui::MenuItem("Refresh", "Ctrl + R"))
					{
						auto data = GetFsContents(m_BaseDirPath);
						for(int i = 0; i < data.size(); i++)
						{
							LUMOS_LOG_INFO(data[i].filename);
						}
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::End();
		}
	}

	void AssetWindow::RenderBreadCrumbs()
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth() - 100, 30));
		{
			if(m_isInListView)
			{
				if(ImGui::Button(ICON_MDI_VIEW_GRID))
				{
					m_isInListView = !m_isInListView;
				}
				ImGui::SameLine();
			}
			else
			{
				if(ImGui::Button(ICON_MDI_VIEW_LIST))
				{
					m_isInListView = !m_isInListView;
				}
				ImGui::SameLine();
			}

			if(ImGui::Button(ICON_MDI_MAGNIFY))
			{
				m_showSearchBar = !m_showSearchBar;

				if(m_showSearchBar)
				{
				}
				else
				{
				}
			}
			ImGui::SameLine();

			if(m_showSearchBar)
			{
				char buff[100] = {0};
				ImGui::SameLine();
				ImGui::PushItemWidth(200);
				ImGui::InputTextWithHint(inputText, inputHint, buff, 100);
				ImGui::PopItemWidth();
				ImGui::SameLine();
			}

			if(ImGui::Button(ICON_MDI_ARROW_LEFT))
			{
				if(strlen(m_CurrentDirPath.c_str()) != m_basePathLen)
				{
					m_prevDirPath = GetParentPath(m_CurrentDirPath);
					m_CurrentDirPath = m_prevDirPath;
					m_CurrentDir = ReadDirectory(m_CurrentDirPath);
				}
			}
			ImGui::SameLine();
			if(ImGui::Button(ICON_MDI_ARROW_RIGHT))
			{
				m_prevDirPath = GetParentPath(m_CurrentDirPath);
				m_CurrentDirPath = m_lastNavPath;
				m_CurrentDir = ReadDirectory(m_lastNavPath);
			}
			ImGui::SameLine();

			GetDirectories(m_CurrentDirPath);

			for(int i = 0; i < m_DirectoryCount; i++)
			{
				if(m_Directories[i] != m_BaseDirPath)
				{
					ImGui::TextUnformatted(ICON_MDI_CHEVRON_RIGHT);
				}
				ImGui::SameLine();
				ImGui::TextUnformatted(m_Directories[i].c_str());
				ImGui::SameLine();
			}

			ImGui::SameLine();

			ImGui::Dummy(ImVec2(ImGui::GetColumnWidth() - 400, 0));

			ImGui::SameLine();
		}
	}

	void AssetWindow::RenderFileListView(int dirIndex)
	{
		LUMOS_PROFILE_FUNCTION();
		auto fileID = GetParsedAssetID(m_CurrentDir[dirIndex].fileType);

		ImGui::TextUnformatted(m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath));
		ImGui::SameLine();
		if(ImGui::Selectable(m_CurrentDir[dirIndex].filename.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if(ImGui::IsMouseDoubleClicked(0))
			{
				m_Editor->FileOpenCallback(m_CurrentDir[dirIndex].absolutePath);
			}
		}

		if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::TextUnformatted(m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath));

			ImGui::SameLine();
			ImGui::TextUnformatted(m_CurrentDir[dirIndex].filename.c_str());
			size_t size = sizeof(const char*) + strlen(m_CurrentDir[dirIndex].absolutePath.c_str());
			ImGui::SetDragDropPayload("selectable", m_CurrentDir[dirIndex].absolutePath.c_str(), size);
			m_IsDragging = true;
			ImGui::EndDragDropSource();
		}
	}

	void AssetWindow::RenderFileGridView(int dirIndex)
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::BeginGroup();

		auto fileID = GetParsedAssetID(m_CurrentDir[dirIndex].fileType);

		ImGui::Button(m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath));
		auto fname = m_CurrentDir[dirIndex].filename;
		auto newFname = StripExtras(fname);

		ImGui::TextWrapped("%s", newFname.c_str());
		ImGui::EndGroup();

		if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::TextUnformatted(m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath));
			ImGui::SameLine();

			ImGui::TextUnformatted(m_CurrentDir[dirIndex].filename.c_str());
			size_t size = sizeof(const char*) + strlen(m_CurrentDir[dirIndex].absolutePath.c_str());
			ImGui::SetDragDropPayload("selectable", m_CurrentDir[dirIndex].absolutePath.c_str(), size);
			m_IsDragging = true;
			ImGui::EndDragDropSource();
		}
	}

	void AssetWindow::RenderDircListView(int dirIndex)
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::TextUnformatted(ICON_MDI_FOLDER);
		ImGui::SameLine();

		if(ImGui::Selectable(m_CurrentDir[dirIndex].filename.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if(ImGui::IsMouseDoubleClicked(0))
			{
				m_prevDirPath = m_CurrentDir[dirIndex].absolutePath;
				m_CurrentDirPath = m_CurrentDir[dirIndex].absolutePath;
				m_CurrentDir = ReadDirectory(m_CurrentDir[dirIndex].absolutePath);
			}
		}

		if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
		{
			ImGui::TextUnformatted(ICON_MDI_FOLDER);
			ImGui::SameLine();
			ImGui::TextUnformatted(m_CurrentDir[dirIndex].filename.c_str());
			size_t size = sizeof(const char*) + strlen(m_CurrentDir[dirIndex].absolutePath.c_str());
			ImGui::SetDragDropPayload("selectable", m_CurrentDir[dirIndex].absolutePath.c_str(), size);
			m_IsDragging = true;
			ImGui::EndDragDropSource();
		}
	}

	void AssetWindow::RenderDircGridView(int dirIndex)
	{
		LUMOS_PROFILE_FUNCTION();
		//ImGui::BeginGroup();
		//ImGui::TextUnformatted(ICON_MDI_FOLDER);

		auto fname = m_CurrentDir[dirIndex].filename;
		auto newFname = StripExtras(fname);
		//	ImGui::TextWrapped("%s", newFname.c_str());
		//	ImGui::EndGroup();

		if(ImGui::Selectable((std::string(ICON_MDI_FOLDER "/n/n") + newFname).c_str(), false, 0, ImVec2(70, 70)))
		{
			if(ImGui::IsMouseDoubleClicked(0))
			{
				m_prevDirPath = m_CurrentDir[dirIndex].absolutePath;
				m_CurrentDirPath = m_CurrentDir[dirIndex].absolutePath;
				m_CurrentDir = ReadDirectory(m_CurrentDir[dirIndex].absolutePath);
			}
		}
	}

	void AssetWindow::RenderBottom()
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::BeginChild("##nav", ImVec2(ImGui::GetColumnWidth() - 12, 23));
		{
			ImGui::EndChild();
		}
	}

	std::vector<DirectoryInformation> AssetWindow::GetFsContents(const std::string& path)
	{
		LUMOS_PROFILE_FUNCTION();
		std::vector<DirectoryInformation> dInfo;

		for(const auto& entry : std::filesystem::directory_iterator(path))
		{
			bool isDir = std::filesystem::is_directory(entry);
			auto test = std::vector<std::string>();
			const char del = *m_Delimiter.c_str();

			auto dir_data = ParseFilename(entry.path().string(), del, test);
			auto fileExt = ParseFiletype(dir_data);

			if(isDir)
			{
				DirectoryInformation d(dir_data, fileExt, entry.path().string(), false);
				dInfo.push_back(d);
			}
			else
			{
				DirectoryInformation d(dir_data, fileExt, entry.path().string(), true);
				dInfo.push_back(d);
			}
		}

		return dInfo;
	}

	std::vector<DirectoryInformation> AssetWindow::ReadDirectory(const std::string& path)
	{
		std::vector<DirectoryInformation> dInfo;

		for(const auto& entry : std::filesystem::directory_iterator(path))
		{
			bool isDir = std::filesystem::is_directory(entry);

			auto test = std::vector<std::string>();
			const char del = *m_Delimiter.c_str();

			auto dir_data = ParseFilename(entry.path().string(), del, test);
			auto fileExt = ParseFiletype(dir_data);

			if(isDir)
			{
				DirectoryInformation d(dir_data, fileExt, entry.path().string(), false);
				dInfo.push_back(d);
			}
			else
			{
				DirectoryInformation d(dir_data, fileExt, entry.path().string(), true);
				dInfo.push_back(d);
			}
		}
		return dInfo;
	}

	std::vector<DirectoryInformation> AssetWindow::ReadDirectoryRecursive(const std::string& path)
	{
		LUMOS_PROFILE_FUNCTION();
		std::vector<DirectoryInformation> dInfo;

		for(const auto& entry : std::filesystem::recursive_directory_iterator(path))
		{
			bool isDir = std::filesystem::is_directory(entry);

			auto test = std::vector<std::string>();
			const char del = *m_Delimiter.c_str();

			auto dir_data = ParseFilename(entry.path().string(), del, test);

			if(isDir)
			{
				DirectoryInformation d(dir_data, ".Lumos", entry.path().string(), false);
				dInfo.push_back(d);
			}
			else
			{
				DirectoryInformation d(dir_data, ".Lumos", entry.path().string(), true);
				dInfo.push_back(d);
			}
		}

		return dInfo;
	}

	std::string AssetWindow::GetParentPath(const std::string& path)
	{
		LUMOS_PROFILE_FUNCTION();
		auto p = std::filesystem::path(path);
		return p.parent_path().string();
	}

	 void AssetWindow::GetDirectories(const std::string& path)
	{
		LUMOS_PROFILE_FUNCTION();
		m_DirectoryCount = 0;
		size_t start;
		size_t end = 0;

		while((start = path.find_first_not_of(m_Delimiter.c_str(), end)) != std::string::npos)
		{
			end = path.find(m_Delimiter.c_str(), start);
			m_Directories[m_DirectoryCount] = path.substr(start, end - start);
			m_DirectoryCount++;
		}
}

	std::vector<std::string> AssetWindow::SearchFiles(const std::string& query)
	{
		return std::vector<std::string>();
	}

	bool AssetWindow::MoveFile(const std::string& filePath, const std::string& movePath)
	{
		LUMOS_PROFILE_FUNCTION();
		std::string s = "move " + filePath + " " + movePath.c_str();
    #ifndef LUMOS_PLATFORM_IOS
		system(s.c_str());
    #endif

		std::vector<std::string> data;

		const char del = *m_Delimiter.c_str();
		if(std::filesystem::exists(std::filesystem::path(movePath + m_Delimiter + ParseFilename(filePath, del, data))))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	std::string AssetWindow::StripExtras(const std::string& filename)
	{
		LUMOS_PROFILE_FUNCTION();
		std::vector<std::string> out;
		size_t start;
		size_t end = 0;

		while((start = filename.find_first_not_of(".", end)) != std::string::npos)
		{
			end = filename.find(".", start);
			out.push_back(filename.substr(start, end - start));
		}

		if(out[0].length() >= 8)
		{
			auto cutFilename = "     " + out[0].substr(0, 5) + "...";
			return cutFilename;
		}

		auto filenameLength = out[0].length();
		auto paddingToAdd = 9 - filenameLength;

		std::string newFileName;

		for(int i = 0; i <= paddingToAdd; i++)
		{
			newFileName += " ";
		}

		newFileName += out[0];

		return newFileName;
	}

	std::string AssetWindow::ParseFilename(const std::string& str, const char delim, std::vector<std::string>& out)
	{
		LUMOS_PROFILE_FUNCTION();
		size_t start;
		size_t end = 0;

		while((start = str.find_first_not_of(delim, end)) != std::string::npos)
		{
			end = str.find(delim, start);
			out.push_back(str.substr(start, end - start));
		}

		return out[out.size() - 1];
	}

	std::string AssetWindow::ParseFiletype(const std::string& filename)
	{
		LUMOS_PROFILE_FUNCTION();
		size_t start;
		size_t end = 0;
		std::vector<std::string> out;

		while((start = filename.find_first_not_of(".", end)) != std::string::npos)
		{
			end = filename.find(".", start);
			out.push_back(filename.substr(start, end - start));
		}

		return out[out.size() - 1];
	}

}
