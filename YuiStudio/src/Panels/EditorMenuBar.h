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

		float GetPreferredWidth(float maxWidth) const;
		void OnImGuiRender();

	private:
		void DrawFileMenu();
		void DrawGameObjectMenu();

		void CreateEmptyEntity();
		void CreateRectangularTilemap();

	private:
		EditorContext* m_context = nullptr;
		EditorSceneController* m_sceneController = nullptr;
		EditorCommandHistory* m_commandHistory = nullptr;
	};

}
