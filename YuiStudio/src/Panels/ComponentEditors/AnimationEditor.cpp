#include "pch.h"

#include "AnimationEditor.h"
#include "../../Editor/EditorContext.h"
#include "../../Editor/EditorDirtyTracker.h"
#include "../../Editor/Asset/EditorAssetManager.h"
#include "../../Utils/ContentBrowserDragDrop.h"
#include "../../Utils/EditorIconUtils.h"

#include "Yuicy/Scene/Components.h"
#include "Yuicy/Renderer/Texture.h"

#include <filesystem>
#include <algorithm>

namespace Yuicy {

	// assetManager 为 nullptr 或纹理加载失败时返回 nullptr
	static Ref<Texture2D> GetTextureAsset(EditorAssetManager* assetManager, AssetHandle handle)
	{
		if (handle == 0 || !assetManager)
			return nullptr;
		return assetManager->GetAssetAs<Texture2D>(handle);
	}

	static void InvalidateRuntimeFrames(AnimationClip& clip)
	{
		clip.Frames.clear();
	}

	// 获取纹理显示名称
	static std::string GetTextureName(EditorAssetManager* assetManager, AssetHandle handle)
	{
		if (handle == 0) return "(empty)";
		if (!assetManager) return "(unknown)";
		const auto& metadata = assetManager->GetMetadata(handle);
		return metadata.IsValid() ? metadata.filePath.filename().string() : "(missing)";
	}

	void AnimationEditor::EnsureIconsLoaded()
	{
		if (m_iconsLoaded) return;
		m_iconsLoaded = true;

		m_addIcon    = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Preparaties/add.png",   { 200, 200, 200, 255 });
		m_removeIcon = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Preparaties/sub.png",   { 200, 100, 100, 255 });
		m_upIcon     = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Preparaties/up.png",    { 200, 200, 200, 255 });
		m_downIcon   = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Preparaties/down.png",  { 200, 200, 200, 255 });
		m_closeIcon  = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Window/Close.png",      { 200, 100, 100, 255 });
	}

	void AnimationEditor::Draw(AnimationComponent& component, EditorDirtyTracker* dt)
	{
		EnsureIconsLoaded();

		EditorAssetManager* assetManager = m_editorContext ? m_editorContext->GetAssetManager() : nullptr;

		const float smallIconSize = 14.0f;

		// 小图标按钮 Helper lambda
		auto iconButton = [](const char* id, Ref<Texture2D> icon, float size, const ImVec4& tint = ImVec4(1,1,1,1)) -> bool
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
			ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)icon->GetRendererID());
			bool clicked = ImGui::ImageButton(id, texID, ImVec2(size, size), ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImVec4(0,0,0,0), tint);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
			return clicked;
		};

		// Default Clip Dropdown
		if (ImGui::BeginCombo("Default Clip", component.DefaultClipName.empty() ? "(none)" : component.DefaultClipName.c_str()))
		{
			for (auto& [name, clip] : component.Clips)
			{
				bool isSelected = (name == component.DefaultClipName);
				if (ImGui::Selectable(name.c_str(), isSelected))
				{
					component.DefaultClipName = name;
					if (dt) dt->MarkSceneDirty();
				}
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();

		// Add Clip Button
		if (iconButton("##addClip", m_addIcon, smallIconSize))
		{
			std::string baseName = "New Clip";
			std::string newName = baseName;
			int counter = 1;
			while (component.Clips.count(newName))
				newName = baseName + " " + std::to_string(counter++);

			AnimationClip newClip;
			newClip.Name = newName;
			component.AddClip(newClip);
			if (dt) dt->MarkSceneDirty();
		}
		ImGui::SameLine();
		ImGui::Text("Add Clip");

		// Per-Clip Editing
		std::string clipToRemove;
		std::string renameOldName, renameNewName;
		std::vector<std::string> clipNames;
		for (auto& [name, clip] : component.Clips)
			clipNames.push_back(name);

		for (const auto& clipName : clipNames)
		{
			auto& clip = component.Clips[clipName];

			ImGui::PushID(clipName.c_str());

			bool isDefault = (clipName == component.DefaultClipName);
			std::string header = clipName;
			if (isDefault) header += " (default)";

			if (ImGui::TreeNode("##clip", "%s", header.c_str()))
			{
				// Clip Name
				char nameBuffer[256] = {};
				std::memcpy(nameBuffer, clipName.c_str(), std::min(clipName.size(), sizeof(nameBuffer) - 1));
				if (ImGui::InputText("Clip Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					std::string newName(nameBuffer);
					if (!newName.empty() && newName != clipName && component.Clips.find(newName) == component.Clips.end())
					{
						renameOldName = clipName;
						renameNewName = newName;
					}
				}

				// Frame Duration + FPS
				if (ImGui::DragFloat("Frame Duration", &clip.FrameDuration, 0.001f, 0.001f, 10.0f, "%.3f s"))
					if (dt) dt->MarkSceneDirty();
				float fps = (clip.FrameDuration > 0.0f) ? (1.0f / clip.FrameDuration) : 0.0f;
				if (ImGui::DragFloat("FPS", &fps, 0.1f, 0.1f, 120.0f, "%.1f"))
				{
					clip.FrameDuration = (fps > 0.0f) ? (1.0f / fps) : 0.1f;
					if (dt) dt->MarkSceneDirty();
				}

				// Loop
				if (ImGui::Checkbox("Loop", &clip.Loop))
					if (dt) dt->MarkSceneDirty();

				ImGui::Separator();

				// Frame List
				ImGui::Text("Frames: %d", (int)clip.FrameDefinitions.size());

				int frameToRemove = -1;
				int frameMoveFrom = -1, frameMoveTo = -1;

				for (int i = 0; i < (int)clip.FrameDefinitions.size(); i++)
				{
					ImGui::PushID(i);
					auto& frameDef = clip.FrameDefinitions[i];

					std::string texName = GetTextureName(assetManager, frameDef.TextureHandle);
					std::string frameHeader = "Frame " + std::to_string(i) + ": " + texName;

					bool frameOpen = ImGui::TreeNode("##frame", "%s", frameHeader.c_str());

					if (frameOpen)
					{
						// Action buttons
						if (i > 0)
						{
							if (iconButton("##up", m_upIcon, smallIconSize)) { frameMoveFrom = i; frameMoveTo = i - 1; }
						}
						else
						{
							ImGui::Dummy(ImVec2(smallIconSize + 4, smallIconSize));
						}
						ImGui::SameLine();

						if (i < (int)clip.FrameDefinitions.size() - 1)
						{
							if (iconButton("##down", m_downIcon, smallIconSize)) { frameMoveFrom = i; frameMoveTo = i + 1; }
						}
						else
						{
							ImGui::Dummy(ImVec2(smallIconSize + 4, smallIconSize));
						}
						ImGui::SameLine();

						if (iconButton("##closeFrame", m_closeIcon, smallIconSize))
							frameToRemove = i;

						// UV Min / Max editing
						bool uvChanged = false;
						uvChanged |= ImGui::DragFloat2("UV Min", &frameDef.UVMin.x, 0.01f, 0.0f, 1.0f, "%.3f");
						uvChanged |= ImGui::DragFloat2("UV Max", &frameDef.UVMax.x, 0.01f, 0.0f, 1.0f, "%.3f");

						if (uvChanged)
						{
							InvalidateRuntimeFrames(clip);
							if (dt) dt->MarkSceneDirty();
						}

						ImGui::TreePop();
					}

					ImGui::PopID();
				}

				// Apply frame reorder
				if (frameMoveFrom >= 0 && frameMoveTo >= 0)
				{
					std::swap(clip.FrameDefinitions[frameMoveFrom], clip.FrameDefinitions[frameMoveTo]);
					InvalidateRuntimeFrames(clip);
					if (dt) dt->MarkSceneDirty();
				}

				// Apply frame removal
				if (frameToRemove >= 0)
				{
					clip.FrameDefinitions.erase(clip.FrameDefinitions.begin() + frameToRemove);
					InvalidateRuntimeFrames(clip);
					if (dt) dt->MarkSceneDirty();
				}

				// Add Single Frame (drag-drop)
				ImGui::Button("Drop Texture to Add Frame", ImVec2(ImGui::GetContentRegionAvail().x, 0));
				if (ImGui::BeginDragDropTarget())
				{
					if (std::optional<std::filesystem::path> droppedPath = ContentBrowserDragDrop::AcceptPayload())
					{
						std::filesystem::path filepath = *droppedPath;

						if (assetManager && assetManager->GetAssetTypeFromPath(filepath) == AssetType::Texture)
						{
							AssetHandle handle = assetManager->ImportAsset(filepath);
							if (handle != 0)
							{
								AnimationFrameDefinition frameDef;
								frameDef.TextureHandle = handle;
								clip.FrameDefinitions.push_back(frameDef);
								InvalidateRuntimeFrames(clip);
								if (dt) dt->MarkSceneDirty();
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				// Add from Spritesheet
				if (ImGui::Button("Add from Spritesheet...", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
				{
					m_sheetTargetClip = clipName;
					m_openSheetPopup = true;
				}

				ImGui::Separator();

				// Remove Clip button
				if (iconButton("##removeClip", m_removeIcon, smallIconSize, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)))
					clipToRemove = clipName;
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Remove Clip");

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		// Spritesheet Import Window
		if (m_openSheetPopup)
		{
			ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Spritesheet Import", &m_openSheetPopup))
			{
				ImGui::TextDisabled("Target Clip: %s", m_sheetTargetClip.c_str());
				ImGui::Separator();

				// Spritesheet texture slot
				std::string sheetName = GetTextureName(assetManager, m_sheetTextureHandle);
				ImGui::Text("Texture: %s", sheetName.c_str());

				ImGui::Button("Drop Spritesheet Here", ImVec2(ImGui::GetContentRegionAvail().x, 0));
				if (ImGui::BeginDragDropTarget())
				{
					if (std::optional<std::filesystem::path> droppedPath = ContentBrowserDragDrop::AcceptPayload())
					{
						std::filesystem::path filepath = *droppedPath;

						if (assetManager && assetManager->GetAssetTypeFromPath(filepath) == AssetType::Texture)
						{
							AssetHandle handle = assetManager->ImportAsset(filepath);
							if (handle != 0)
								m_sheetTextureHandle = handle;
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::Separator();

				// Grid params
				int cellSize[2] = { m_sheetCellWidth, m_sheetCellHeight };
				if (ImGui::InputInt2("Cell Size (px)", cellSize))
				{
					m_sheetCellWidth = std::max(1, cellSize[0]);
					m_sheetCellHeight = std::max(1, cellSize[1]);
				}

				int startCoord[2] = { m_sheetStartX, m_sheetStartY };
				if (ImGui::InputInt2("Start Coord", startCoord))
				{
					m_sheetStartX = std::max(0, startCoord[0]);
					m_sheetStartY = std::max(0, startCoord[1]);
				}

				ImGui::InputInt("Frame Count", &m_sheetFrameCount);
				m_sheetFrameCount = std::max(1, m_sheetFrameCount);

				ImGui::Checkbox("Horizontal", &m_sheetHorizontal);
				ImGui::SameLine();
				ImGui::TextDisabled("(uncheck for vertical)");

				// Sheet info
				if (m_sheetTextureHandle != 0)
				{
					Ref<Texture2D> sheetTex = GetTextureAsset(assetManager, m_sheetTextureHandle);
					if (sheetTex)
					{
						ImGui::Separator();
						ImGui::TextDisabled("Sheet: %d x %d px  |  Grid: %d x %d cells",
							sheetTex->GetWidth(), sheetTex->GetHeight(),
							sheetTex->GetWidth() / std::max(1, m_sheetCellWidth),
							sheetTex->GetHeight() / std::max(1, m_sheetCellHeight));
					}
				}

				ImGui::Separator();

				// Generate + Close
				bool canGenerate = (m_sheetTextureHandle != 0 && m_sheetFrameCount > 0
					&& !m_sheetTargetClip.empty() && component.Clips.count(m_sheetTargetClip));

				if (!canGenerate) ImGui::BeginDisabled();
				if (ImGui::Button("Generate", ImVec2(120, 0)))
				{
					auto& targetClip = component.Clips[m_sheetTargetClip];
					Ref<Texture2D> sheetTex = GetTextureAsset(assetManager, m_sheetTextureHandle);
					if (sheetTex)
					{
						float texW = (float)sheetTex->GetWidth();
						float texH = (float)sheetTex->GetHeight();
						float cellW = (float)m_sheetCellWidth;
						float cellH = (float)m_sheetCellHeight;

						for (int i = 0; i < m_sheetFrameCount; i++)
						{
							float gridX = (float)m_sheetStartX;
							float gridY = (float)m_sheetStartY;
							if (m_sheetHorizontal)
								gridX += (float)i;
							else
								gridY += (float)i;

							AnimationFrameDefinition frameDef;
							frameDef.TextureHandle = m_sheetTextureHandle;
							frameDef.UVMin = { (gridX * cellW) / texW, (gridY * cellH) / texH };
							frameDef.UVMax = { ((gridX + 1.0f) * cellW) / texW, ((gridY + 1.0f) * cellH) / texH };

							targetClip.FrameDefinitions.push_back(frameDef);
						}

						InvalidateRuntimeFrames(targetClip);
						if (dt) dt->MarkSceneDirty();
					}
					m_openSheetPopup = false;
				}
				if (!canGenerate) ImGui::EndDisabled();

				ImGui::SameLine();
				if (ImGui::Button("Close", ImVec2(120, 0)))
					m_openSheetPopup = false;
			}
			ImGui::End();
		}

		// Deferred clip removal
		if (!clipToRemove.empty())
		{
			component.Clips.erase(clipToRemove);
			if (component.DefaultClipName == clipToRemove)
				component.DefaultClipName = component.Clips.empty() ? "" : component.Clips.begin()->first;
			if (component.State.CurrentClipName == clipToRemove)
			{
				component.State.CurrentClipName = component.DefaultClipName;
				component.State.Reset();
			}
			m_previewPlaying = false;
			m_previewFrame = 0;
			if (dt) dt->MarkSceneDirty();
		}

		// Deferred clip rename
		if (!renameOldName.empty() && !renameNewName.empty())
		{
			auto it = component.Clips.find(renameOldName);
			if (it != component.Clips.end())
			{
				AnimationClip clipData = std::move(it->second);
				clipData.Name = renameNewName;
				component.Clips.erase(it);
				component.Clips[renameNewName] = std::move(clipData);

				if (component.DefaultClipName == renameOldName)
					component.DefaultClipName = renameNewName;
				if (component.State.CurrentClipName == renameOldName)
					component.State.CurrentClipName = renameNewName;

				if (dt) dt->MarkSceneDirty();
			}
		}

		// Animation Preview
		if (!component.Clips.empty())
		{
			ImGui::Separator();
			ImGui::Text("Preview");

			std::string previewClipName = component.DefaultClipName;
			if (previewClipName.empty() || component.Clips.find(previewClipName) == component.Clips.end())
				previewClipName = component.Clips.begin()->first;

			auto& previewClip = component.Clips[previewClipName];

			if (!previewClip.FrameDefinitions.empty())
			{
				int frameCount = (int)previewClip.FrameDefinitions.size();

				// Advance preview
				if (m_previewPlaying)
				{
					m_previewTimer += ImGui::GetIO().DeltaTime;
					if (m_previewTimer >= previewClip.FrameDuration)
					{
						m_previewTimer -= previewClip.FrameDuration;
						m_previewFrame++;
						if (m_previewFrame >= frameCount)
							m_previewFrame = previewClip.Loop ? 0 : frameCount - 1;
					}
				}

				if (m_previewFrame >= frameCount)
					m_previewFrame = 0;

				const auto& frameDef = previewClip.FrameDefinitions[m_previewFrame];
				Ref<Texture2D> frameTexture = GetTextureAsset(assetManager, frameDef.TextureHandle);

				if (frameTexture)
				{
					ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)frameTexture->GetRendererID());
					ImGui::Image(texID, ImVec2(64, 64),
						ImVec2(frameDef.UVMin.x, frameDef.UVMax.y),
						ImVec2(frameDef.UVMax.x, frameDef.UVMin.y));
				}
				else
				{
					ImGui::Button("--", ImVec2(64, 64));
				}

				ImGui::SameLine();
				ImGui::BeginGroup();
				if (ImGui::Button(m_previewPlaying ? "Pause" : "Play"))
				{
					m_previewPlaying = !m_previewPlaying;
					if (m_previewPlaying)
						m_previewTimer = 0.0f;
				}
				ImGui::Text("Frame: %d / %d", m_previewFrame + 1, frameCount);
				ImGui::Text("Clip: %s", previewClipName.c_str());
				ImGui::EndGroup();
			}
			else
			{
				ImGui::TextDisabled("No frames in clip");
			}
		}
	}

}
