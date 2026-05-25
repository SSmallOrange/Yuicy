#include "pch.h"

#include "EditorMenuBar.h"

#include "Yuicy/Core/Application.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Scene/Entity.h"

#include "../Editor/EditorCommandHistory.h"
#include "../Editor/EditorContext.h"
#include "../Editor/EditorSceneController.h"
#include "../Editor/Commands/CreateEntityCommand.h"
#include "../Editor/Commands/CreateTilemapEntityCommand.h"

namespace Yuicy {

	namespace {

		constexpr const char* s_menuLabels[] = { "File", "GameObject", "Window" };

	}

	float EditorMenuBar::GetPreferredWidth(float maxWidth) const
	{
		ImGuiStyle& style = ImGui::GetStyle();
		float menuBarWidth = 12.0f;

		for (const char* label : s_menuLabels)
			menuBarWidth += ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f + style.ItemSpacing.x;

		return std::min(menuBarWidth, maxWidth);
	}

	void EditorMenuBar::OnImGuiRender()
	{
		if (!ImGui::BeginMenuBar())
			return;

		DrawFileMenu();
		DrawGameObjectMenu();
		DrawWindowMenu();

		ImGui::EndMenuBar();
	}

	void EditorMenuBar::DrawFileMenu()
	{
		if (!m_sceneController || !ImGui::BeginMenu("File"))
			return;

		if (ImGui::MenuItem("New Project..."))
			m_sceneController->NewProject();

		if (ImGui::MenuItem("Open Project..."))
			m_sceneController->OpenProjectDialog();

		if (ImGui::MenuItem("Save Project"))
			m_sceneController->SaveProject();

		ImGui::Separator();

		if (ImGui::MenuItem("New Scene", "Ctrl+N"))
			m_sceneController->NewScene();

		if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
			m_sceneController->OpenSceneDialog();

		if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
			m_sceneController->SaveScene();

		if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
			m_sceneController->SaveSceneAs();

		ImGui::Separator();

		if (ImGui::MenuItem("Exit"))
			Application::Get().GetWindow().Close();

		ImGui::EndMenu();
	}

	void EditorMenuBar::DrawGameObjectMenu()
	{
		if (!ImGui::BeginMenu("GameObject"))
			return;

		if (ImGui::MenuItem("Create Empty"))
			CreateEmptyEntity();

		if (ImGui::BeginMenu("2D Object"))
		{
			if (ImGui::BeginMenu("Tilemap"))
			{
				if (ImGui::MenuItem("Rectangular"))
					CreateRectangularTilemap();

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

	void EditorMenuBar::DrawWindowMenu()
	{
		if (!ImGui::BeginMenu("Window"))
			return;

		if (ImGui::BeginMenu("2D"))
		{
			if (m_tilePalettePanelOpen)
			{
				ImGui::MenuItem("Tile Palette", nullptr, m_tilePalettePanelOpen);
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::MenuItem("Tile Palette");
				ImGui::EndDisabled();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

	void EditorMenuBar::CreateEmptyEntity()
	{
		if (!m_context || !m_context->activeScene || !m_commandHistory)
			return;

		m_commandHistory->ExecuteCommand(
			CreateScope<CreateEntityCommand>(m_context->activeScene.get(), "Empty Entity", &m_context->selection));
	}

	void EditorMenuBar::CreateRectangularTilemap()
	{
		if (!m_context || !m_context->activeScene || !m_commandHistory)
			return;

		m_commandHistory->ExecuteCommand(
			CreateScope<CreateTilemapEntityCommand>(m_context->activeScene.get(), &m_context->selection, GetSelectedGridUUID()));
	}

	UUID EditorMenuBar::GetSelectedGridUUID() const
	{
		if (!m_context || !m_context->activeScene)
			return UUID(0);

		UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0)
			return UUID(0);

		Entity selectedEntity = m_context->activeScene->FindEntityByUUID(selectedUUID);
		if (!selectedEntity || !selectedEntity.HasComponent<GridComponent>())
			return UUID(0);

		return selectedUUID;
	}

}
