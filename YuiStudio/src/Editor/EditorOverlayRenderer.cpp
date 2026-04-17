#include "EditorOverlayRenderer.h"

#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

namespace Yuicy {

	void EditorOverlayRenderer::Render(const EditorCamera& camera, const Ref<Scene>& scene)
	{
		if (!m_context || !scene)
			return;

		auto& settings = m_context->viewportSettings;

		// 世界原点
		if (settings.showOrigin)
			DrawWorldOrigin();

		// 选中实体外框
		if (settings.showSelectionBounds)
			DrawSelectionBounds(scene);
	}

	void EditorOverlayRenderer::DrawWorldOrigin()
	{
		const float length = 0.15f;
		const float z = 0.1f;  // 略高于场景内容，确保可见

		// X 轴 - 红色
		Renderer2D::DrawLine(
			{ 0.0f, 0.0f, z },
			{ length, 0.0f, z },
			{ 1.0f, 0.2f, 0.2f, 0.8f }
		);

		// Y 轴 - 绿色
		Renderer2D::DrawLine(
			{ 0.0f, 0.0f, z },
			{ 0.0f, length, z },
			{ 0.2f, 1.0f, 0.2f, 0.8f }
		);
	}

	void EditorOverlayRenderer::DrawSelectionBounds(const Ref<Scene>& scene)
	{
		if (!m_context->selection.HasEntitySelection())
			return;

		UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0)
			return;

		Entity selectedEntity = scene->FindEntityByUUID(selectedUUID);
		if (!selectedEntity || !selectedEntity.HasComponent<TransformComponent>())
			return;

		// 获取世界空间变换矩阵
		glm::mat4 worldTransform = scene->GetWorldSpaceTransformMatrix(selectedEntity);

		// 橙色选中框
		glm::vec4 selectionColor = { 1.0f, 0.6f, 0.0f, 1.0f };
		Renderer2D::DrawRect(worldTransform, selectionColor);
	}

}
