#pragma once

namespace Yuicy {

	struct EditorContext;
	class EditorCommandHistory;
	class EditorSceneController;

	class EditorMenuBar
	{
	public:
		void SetContext(EditorContext* context) { m_context = context; }
		void SetSceneController(EditorSceneController* sceneController) { m_sceneController = sceneController; }
		void SetCommandHistory(EditorCommandHistory* commandHistory) { m_commandHistory = commandHistory; }
		void SetTilePalettePanelOpen(bool* open) { m_tilePalettePanelOpen = open; }

		float GetPreferredWidth(float maxWidth) const;
		void OnImGuiRender();

	private:
		void DrawFileMenu();
		void DrawGameObjectMenu();
		void DrawWindowMenu();

		void CreateEmptyEntity();
		void CreateRectangularTilemap();

	private:
		EditorContext* m_context = nullptr;
		EditorSceneController* m_sceneController = nullptr;
		EditorCommandHistory* m_commandHistory = nullptr;
		bool* m_tilePalettePanelOpen = nullptr;
	};

}
