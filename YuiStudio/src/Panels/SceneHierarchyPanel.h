#pragma once

#include "Yuicy.h"

namespace Yuicy {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_selectionContext; }
		void SetSelectedEntity(Entity entity) { m_selectionContext = entity; }

	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);

		// 组件绘制
		void DrawVec3Control(const std::string& label, glm::vec3& values,
			float resetValue = 0.0f, float columnWidth = 100.0f);

		template<typename T>
		void DrawAddComponentEntry(const std::string& entryName);

	private:
		Ref<Scene> m_context;
		Entity m_selectionContext;
	};

}
