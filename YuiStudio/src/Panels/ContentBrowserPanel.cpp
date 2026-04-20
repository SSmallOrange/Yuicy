#include "ContentBrowserPanel.h"

#include "../Editor/EditorContext.h"
#include "../Editor/EditorAssetWorkflow.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"

#include "imgui/imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>
#include <vector>

namespace Yuicy {

	namespace {

		std::filesystem::path ResolveEditorAssetPath(const std::filesystem::path& relativePath)
		{
			const std::array<std::filesystem::path, 3> candidates = {
				relativePath,
				std::filesystem::path("YuiStudio") / relativePath,
				std::filesystem::path("..") / ".." / ".." / "YuiStudio" / relativePath
			};

			for (const auto& candidate : candidates)
			{
				std::error_code ec;
				if (std::filesystem::exists(candidate, ec) && !ec)
					return candidate.lexically_normal();
			}

			return relativePath;
		}

		Ref<Texture2D> CreateSolidColorTexture(const std::array<uint8_t, 4>& color)
		{
			Ref<Texture2D> texture = Texture2D::Create(1, 1);
			uint8_t pixel[4] = { color[0], color[1], color[2], color[3] };
			texture->SetData(pixel, sizeof(pixel));
			return texture;
		}

		Ref<Texture2D> LoadIconTexture(const std::filesystem::path& relativePath, const std::array<uint8_t, 4>& fallbackColor)
		{
			const std::filesystem::path iconPath = ResolveEditorAssetPath(relativePath);

			std::error_code ec;
			if (std::filesystem::exists(iconPath, ec) && !ec)
				return Texture2D::Create(iconPath.string());

			YUICY_CORE_WARN("[ContentBrowser] Failed to load icon '{}', using fallback texture.", relativePath.string());
			return CreateSolidColorTexture(fallbackColor);
		}

		std::string ToLowerCopy(std::string value)
		{
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
			{
				return (char)std::tolower(character);
			});

			return value;
		}

		std::string GetDisplayPath(const ProjectConfig& config, const std::filesystem::path& baseDirectory, const std::filesystem::path& currentDirectory)
		{
			if (baseDirectory.empty() || currentDirectory.empty())
				return config.AssetDirectory;

			const std::filesystem::path relativePath = currentDirectory.lexically_normal().lexically_relative(baseDirectory.lexically_normal());
			if (relativePath.empty() || relativePath == ".")
				return config.AssetDirectory;

			return config.AssetDirectory + "/" + relativePath.generic_string();
		}

	}

	ContentBrowserPanel::ContentBrowserPanel()
	{
		m_directoryIcon = LoadIconTexture("assets/textures/Editor/ContentBrowser/Folder.png", { 237, 190, 67, 255 });
		m_fileIcon = LoadIconTexture("assets/textures/Editor/ContentBrowser/File.png", { 181, 187, 196, 255 });
		m_backIcon = LoadIconTexture("assets/textures/Editor/Generic/Back.png", { 200, 200, 200, 255 });
	}

	void ContentBrowserPanel::ResetNavigation()
	{
		m_baseDirectory.clear();
		m_currentDirectory.clear();
		m_selectedPath.clear();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		Ref<Project> activeProject = Project::GetActive();
		if (!activeProject)
		{
			ImGui::TextDisabled("No active project.");
			ImGui::TextWrapped("Open or create a project to browse the asset directory.");
			ImGui::End();
			return;
		}

		const std::filesystem::path activeAssetDirectory = activeProject->GetAssetDirectory().lexically_normal();
		if (m_baseDirectory.empty() || m_baseDirectory != activeAssetDirectory)
		{
			m_baseDirectory = activeAssetDirectory;
			m_currentDirectory = activeAssetDirectory;
			m_selectedPath.clear();
		}

		if (std::error_code ec; m_currentDirectory.empty() || !std::filesystem::exists(m_currentDirectory, ec))
			m_currentDirectory = m_baseDirectory;

		if (std::error_code ec; !std::filesystem::exists(m_baseDirectory, ec))
		{
			ImGui::TextDisabled("Asset directory is unavailable.");
			ImGui::TextWrapped("%s", m_baseDirectory.string().c_str());
			ImGui::End();
			return;
		}

		const float headerLineHeight = 18.0f;
		const float defaultFontSize = ImGui::GetFontSize();
		const float headerFontScale = defaultFontSize > 0.0f ? headerLineHeight / defaultFontSize : 1.0f;
		ImGui::SetWindowFontScale(headerFontScale);

		if (m_currentDirectory != m_baseDirectory)
		{
			ImTextureID backTexID = reinterpret_cast<ImTextureID>((uintptr_t)m_backIcon->GetRendererID());
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.0f, 0.0f });
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
			if (ImGui::ImageButton("##BackBtn", backTexID, ImVec2{ headerLineHeight, headerLineHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
				m_currentDirectory = m_currentDirectory.parent_path();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();

			ImGui::SameLine();
		}

		const std::string currentPathLabel = GetDisplayPath(activeProject->GetConfig(), m_baseDirectory, m_currentDirectory);
		std::vector<char> currentPathBuffer(currentPathLabel.begin(), currentPathLabel.end());
		currentPathBuffer.push_back('\0');

		// Render the current directory as a read-only field so it can be selected and copied.
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.0f, 0.0f });
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##CurrentPath", currentPathBuffer.data(), currentPathBuffer.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopStyleVar();
		ImGui::SetWindowFontScale(1.0f);
		ImGui::Separator();

		if (ImGui::BeginPopupContextWindow("ContentBrowserContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Refresh"))
			{
				std::error_code refreshError;
				if (!std::filesystem::exists(m_currentDirectory, refreshError) || refreshError)
					m_currentDirectory = m_baseDirectory;
			}

			if (ImGui::MenuItem("Show in Explorer"))
			{
				if (m_assetWorkflow)
					m_assetWorkflow->RevealInExplorer(m_currentDirectory);
			}

			ImGui::EndPopup();
		}

		std::vector<std::filesystem::directory_entry> directoryEntries;
		std::error_code ec;
		for (std::filesystem::directory_iterator it(m_currentDirectory, std::filesystem::directory_options::skip_permission_denied, ec), end;
			 it != end;
			 it.increment(ec))
		{
			if (ec)
			{
				YUICY_CORE_WARN("[ContentBrowser] Failed to iterate directory '{}': {}", m_currentDirectory.string(), ec.message());
				ec.clear();
				continue;
			}

			directoryEntries.emplace_back(*it);
		}

		std::sort(directoryEntries.begin(), directoryEntries.end(), [](const auto& lhs, const auto& rhs)
		{
			std::error_code lhsError;
			std::error_code rhsError;
			const bool lhsDirectory = lhs.is_directory(lhsError);
			const bool rhsDirectory = rhs.is_directory(rhsError);

			if (lhsDirectory != rhsDirectory)
				return lhsDirectory > rhsDirectory;

			return ToLowerCopy(lhs.path().filename().string()) < ToLowerCopy(rhs.path().filename().string());
		});

		const float cellSize = m_thumbnailSize + m_padding;
		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.0f, 0.0f });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ m_padding, m_padding });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.2f, 0.2f, 0.35f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.2f, 0.2f, 0.55f });

		ImGui::Columns(columnCount, nullptr, false);

		for (const auto& entry : directoryEntries)
		{
			std::error_code typeError;
			const bool isDirectory = entry.is_directory(typeError);
			const std::filesystem::path& entryPath = entry.path();
			const std::string filename = entryPath.filename().string();
			const Ref<Texture2D>& icon = isDirectory ? m_directoryIcon : m_fileIcon;
			const ImTextureID iconTextureID = reinterpret_cast<ImTextureID>((uintptr_t)icon->GetRendererID());

			ImGui::PushID(filename.c_str());

			// 选中项高亮
			bool isSelected = (m_selectedPath == entryPath);
			if (isSelected)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.18f, 0.30f, 0.52f, 0.65f });

			ImGui::ImageButton("##ContentBrowserItem", iconTextureID, ImVec2{ m_thumbnailSize, m_thumbnailSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			if (isSelected)
				ImGui::PopStyleColor();

			// 双击行为：目录→进入，文件→根据类型打开
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (isDirectory)
				{
					m_currentDirectory /= entryPath.filename();
				}
				else if (m_assetWorkflow)
				{
					m_assetWorkflow->OpenAsset(entryPath);
				}
			}
			// 单击选择
			else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				m_selectedPath = entryPath;

				if (!isDirectory && m_context)
				{
					auto assetManager = Project::GetEditorAssetManager();
					if (assetManager)
					{
						AssetHandle handle = assetManager->GetAssetHandleFromFilePath(entryPath);
						m_context->selection.selectedAsset = handle;
						m_context->selection.ClearEntitySelection();
					}
				}
				else if (isDirectory && m_context)
				{
					m_context->selection.ClearAssetSelection();
				}
			}

			// 文件拖拽
			if (!isDirectory && ImGui::BeginDragDropSource())
			{
				const auto& nativePath = entryPath.native();
				ImGui::SetDragDropPayload(
					"CONTENT_BROWSER_ITEM",
					nativePath.c_str(),
					(nativePath.size() + 1) * sizeof(std::filesystem::path::value_type));
				ImGui::TextUnformatted(filename.c_str());
				ImGui::EndDragDropSource();
			}

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cellSize);
			ImGui::TextWrapped("%s", filename.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		// 点击空白区域取消选择
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
		{
			m_selectedPath.clear();
			if (m_context)
				m_context->selection.ClearAssetSelection();
		}

		ImGui::End();
	}

}
