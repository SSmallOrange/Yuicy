#include "EditorOverlayRenderer.h"

#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

#include <cmath>

namespace Yuicy {

	// 背景层：Grid + Origin
	void EditorOverlayRenderer::RenderBackground(const EditorCamera& camera, const Ref<Scene>& scene)
	{
		if (!m_context || !scene)
			return;

		auto& settings = m_context->viewportSettings;
		if (!settings.showOverlay)
			return;

		// 世界网格
		if (settings.showGrid)
			DrawGrid(camera);

		// 世界原点
		if (settings.showOrigin)
			DrawWorldOrigin(camera);
	}

	// 前景层：Selection Bounds 等编辑辅助
	void EditorOverlayRenderer::RenderForeground(const EditorCamera& camera, const Ref<Scene>& scene)
	{
		if (!m_context || !scene)
			return;

		auto& settings = m_context->viewportSettings;
		if (!settings.showOverlay)
			return;

		if (settings.showCameraBounds)
			DrawCameraBounds(scene);

		if (settings.showColliders)
			DrawColliders(scene);

		if (settings.showSelectionBounds)
			DrawSelectionBounds(scene);

		if (settings.showRelationshipLines)
			DrawRelationshipLines(scene);

		if (settings.showPivot)
			DrawEntityPivot(scene);
	}

	// 世界网格绘制
	// 基于 EditorCamera 当前可见世界范围动态裁剪
	// 主网格线与次网格线使用不同透明度区分
	// 当缩放过远导致次网格线像素间距过小时自动隐藏
	void EditorOverlayRenderer::DrawGrid(const EditorCamera& camera)
	{
		auto& settings = m_context->viewportSettings;

		float zoomLevel  = camera.GetZoomLevel();
		float aspectRatio = camera.GetAspectRatio();
		const glm::vec3& camPos = camera.GetPosition();

		// 计算编辑器相机的可见世界范围
		float halfWidth  = aspectRatio * zoomLevel;
		float halfHeight = zoomLevel;

		float left   = camPos.x - halfWidth;
		float right  = camPos.x + halfWidth;
		float bottom = camPos.y - halfHeight;
		float top    = camPos.y + halfHeight;

		const float z = 0.0f;

		// 使用视口像素宽度计算每世界单位对应的像素数，用于判断细网格是否可见
		float viewportPixelWidth = m_context->viewport.size.x;
		float pixelsPerUnit = (viewportPixelWidth > 0.0f) ? (viewportPixelWidth / (2.0f * halfWidth)) : 0.0f;

		float minorStep = settings.gridMinorStep;
		float majorStep = settings.gridMajorStep;

		// 次网格线
		// 仅在像素间距 >= 4px 时绘制，避免缩放过远时变成视觉噪音
		if (minorStep > 0.0f && pixelsPerUnit > 0.0f)
		{
			float minorPixelSpacing = minorStep * pixelsPerUnit;

			if (minorPixelSpacing >= 4.0f)
			{
				glm::vec4 minorColor = { 0.5f, 0.5f, 0.5f, 0.15f };

				// 竖线
				float startX = std::floor(left / minorStep) * minorStep;
				for (float x = startX; x <= right; x += minorStep)
				{
					// 跳过与主网格重合的位置（主网格会单独绘制，避免叠加混色）
					if (majorStep > 0.0f)
					{
						float nearestMajor = std::round(x / majorStep) * majorStep;
						if (std::fabs(x - nearestMajor) < minorStep * 0.01f)
							continue;
					}
					Renderer2D::DrawLine({ x, bottom, z }, { x, top, z }, minorColor);
				}

				// 横线
				float startY = std::floor(bottom / minorStep) * minorStep;
				for (float y = startY; y <= top; y += minorStep)
				{
					if (majorStep > 0.0f)
					{
						float nearestMajor = std::round(y / majorStep) * majorStep;
						if (std::fabs(y - nearestMajor) < minorStep * 0.01f)
							continue;
					}
					Renderer2D::DrawLine({ left, y, z }, { right, y, z }, minorColor);
				}
			}
		}

		// 主网格线
		if (majorStep > 0.0f)
		{
			glm::vec4 majorColor = { 0.5f, 0.5f, 0.5f, 0.3f };

			// 竖线
			float startX = std::floor(left / majorStep) * majorStep;
			for (float x = startX; x <= right; x += majorStep)
			{
				// 跳过原点轴线（由 DrawWorldOrigin 绘制彩色轴线）
				if (std::fabs(x) < 0.001f)
					continue;
				Renderer2D::DrawLine({ x, bottom, z }, { x, top, z }, majorColor);
			}

			// 横线
			float startY = std::floor(bottom / majorStep) * majorStep;
			for (float y = startY; y <= top; y += majorStep)
			{
				if (std::fabs(y) < 0.001f)
					continue;
				Renderer2D::DrawLine({ left, y, z }, { right, y, z }, majorColor);
			}
		}
	}

	// 世界原点绘制
	void EditorOverlayRenderer::DrawWorldOrigin(const EditorCamera& camera)
	{
		float zoomLevel   = camera.GetZoomLevel();
		float aspectRatio = camera.GetAspectRatio();
		const glm::vec3& camPos = camera.GetPosition();

		float halfWidth  = aspectRatio * zoomLevel;
		float halfHeight = zoomLevel;

		float left   = camPos.x - halfWidth;
		float right  = camPos.x + halfWidth;
		float bottom = camPos.y - halfHeight;
		float top    = camPos.y + halfHeight;

		const float z = -0.1f;  // 略高于网格，确保轴线可见

		// 贯穿视口的轴线
		glm::vec4 xAxisColor = { 0.8f, 0.2f, 0.2f, 0.4f };
		glm::vec4 yAxisColor = { 0.2f, 0.8f, 0.2f, 0.4f };

		// X 轴
		Renderer2D::DrawLine({ left, 0.0f, z }, { right, 0.0f, z }, xAxisColor);
		// Y 轴
		Renderer2D::DrawLine({ 0.0f, bottom, z }, { 0.0f, top, z }, yAxisColor);

		// 原点处的短十字高亮标记
		const float crossLength = 0.15f;
		glm::vec4 xBright = { 1.0f, 0.2f, 0.2f, 0.8f };
		glm::vec4 yBright = { 0.2f, 1.0f, 0.2f, 0.8f };

		Renderer2D::DrawLine({ 0.0f, 0.0f, z }, { crossLength, 0.0f, z }, xBright);
		Renderer2D::DrawLine({ 0.0f, 0.0f, z }, { 0.0f, crossLength, z }, yBright);
	}

	// 相机可视范围绘制
	void EditorOverlayRenderer::DrawCameraBounds(const Ref<Scene>& scene)
	{
		UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();

		auto view = scene->GetAllEntitiesWith<TransformComponent, CameraComponent>();
		for (auto entityHandle : view)
		{
			Entity entity(entityHandle, scene.get());
			auto& cameraComp = entity.GetComponent<CameraComponent>();

			float orthoSize = cameraComp.Camera.GetOrthographicSize();
			float aspect = cameraComp.Camera.GetAspectRatio();
			if (aspect <= 0.0f)
				continue;

			// orthoSize
			TransformComponent worldTC = scene->GetWorldSpaceTransform(entity);
			float boundsWidth  = orthoSize * aspect;
			float boundsHeight = orthoSize;

			glm::mat4 boundsTransform = glm::translate(glm::mat4(1.0f), worldTC.Translation)
				* glm::toMat4(glm::quat(worldTC.Rotation))
				* glm::scale(glm::mat4(1.0f), { boundsWidth, boundsHeight, 1.0f });

			// 颜色：Primary / 非 Primary / 选中高亮
			bool isSelected = (entity.GetUUID() == selectedUUID);
			glm::vec4 color;

			if (cameraComp.Primary)
				color = isSelected ? glm::vec4{ 1.0f, 0.85f, 0.0f, 0.9f } : glm::vec4{ 1.0f, 0.85f, 0.0f, 0.6f };
			else
				color = isSelected ? glm::vec4{ 0.0f, 0.85f, 1.0f, 0.9f } : glm::vec4{ 0.6f, 0.6f, 0.6f, 0.5f };

			Renderer2D::DrawRect(boundsTransform, color);
		}
	}

	void EditorOverlayRenderer::DrawColliders(const Ref<Scene>& scene)
	{
		glm::vec4 colliderColor = { 0.0f, 0.7f, 0.0f, 0.8f };
		glm::vec4 triggerColor  = { 0.9f, 0.3f, 0.9f, 0.8f };

		// BoxCollider2D
		{
			auto view = scene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
			for (auto entityHandle : view)
			{
				Entity entity(entityHandle, scene.get());
				auto& bc = entity.GetComponent<BoxCollider2DComponent>();

				glm::mat4 worldTransform = scene->GetWorldSpaceTransformMatrix(entity);

				glm::mat4 boundsTransform = worldTransform
					* glm::translate(glm::mat4(1.0f), glm::vec3(bc.Offset, 0.0f))
					* glm::scale(glm::mat4(1.0f), glm::vec3(bc.Size * 2.0f, 1.0f));

				Renderer2D::DrawRect(boundsTransform, bc.IsTrigger ? triggerColor : colliderColor);
			}
		}

		// CircleCollider2D
		{
			auto view = scene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
			for (auto entityHandle : view)
			{
				Entity entity(entityHandle, scene.get());
				auto& cc = entity.GetComponent<CircleCollider2DComponent>();

				TransformComponent worldTC = scene->GetWorldSpaceTransform(entity);

				// 取 X/Y 轴较大的 scale 分量作为半径缩放
				float maxScale = glm::max(glm::abs(worldTC.Scale.x), glm::abs(worldTC.Scale.y));
				float diameter = cc.Radius * maxScale * 2.0f;

				glm::mat4 boundsTransform = glm::translate(glm::mat4(1.0f), worldTC.Translation)
					* glm::toMat4(glm::quat(worldTC.Rotation))
					* glm::translate(glm::mat4(1.0f), glm::vec3(cc.Offset * maxScale, 0.0f))
					* glm::scale(glm::mat4(1.0f), glm::vec3(diameter, diameter, 1.0f));

				Renderer2D::DrawCircle(boundsTransform, cc.IsTrigger ? triggerColor : colliderColor);
			}
		}
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

		// 有 CameraComponent 的实体由 DrawCameraBounds 提供选中高亮
		if (m_context->viewportSettings.showCameraBounds && selectedEntity.HasComponent<CameraComponent>())
			return;

		// 获取世界空间变换矩阵
		glm::mat4 worldTransform = scene->GetWorldSpaceTransformMatrix(selectedEntity);

		// 橙色选中框
		glm::vec4 selectionColor = { 1.0f, 0.6f, 0.0f, 1.0f };
		Renderer2D::DrawRect(worldTransform, selectionColor);
	}

	void EditorOverlayRenderer::DrawEntityPivot(const Ref<Scene>& scene)
	{
		if (!m_context->selection.HasEntitySelection())
			return;

		UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0)
			return;

		Entity selectedEntity = scene->FindEntityByUUID(selectedUUID);
		if (!selectedEntity || !selectedEntity.HasComponent<TransformComponent>())
			return;

		TransformComponent worldTC = scene->GetWorldSpaceTransform(selectedEntity);
		glm::vec3 pos = worldTC.Translation;

		// 根据实体旋转计算局部 X/Y 轴方向
		glm::mat4 rotMat = glm::toMat4(glm::quat(worldTC.Rotation));
		glm::vec3 localX = glm::vec3(rotMat[0]);
		glm::vec3 localY = glm::vec3(rotMat[1]);

		const float pivotLength = 0.12f;
		const float z = -0.05f;
		glm::vec3 pivotPos = { pos.x, pos.y, z };

		glm::vec4 xColor = { 1.0f, 0.3f, 0.3f, 0.9f };
		glm::vec4 yColor = { 0.3f, 1.0f, 0.3f, 0.9f };

		Renderer2D::DrawLine(pivotPos, pivotPos + localX * pivotLength, xColor);
		Renderer2D::DrawLine(pivotPos, pivotPos + localY * pivotLength, yColor);
	}

	void EditorOverlayRenderer::DrawRelationshipLines(const Ref<Scene>& scene)
	{
		UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();

		glm::vec4 defaultColor  = { 0.6f, 0.6f, 0.6f, 0.4f };
		glm::vec4 selectedColor = { 1.0f, 0.7f, 0.2f, 0.7f };

		const float z = -0.02f;

		auto view = scene->GetAllEntitiesWith<TransformComponent, RelationshipComponent>();
		for (auto entityHandle : view)
		{
			Entity entity(entityHandle, scene.get());
			const auto& children = entity.Children();
			if (children.empty())
				continue;

			TransformComponent parentWorldTC = scene->GetWorldSpaceTransform(entity);
			glm::vec3 parentPos = { parentWorldTC.Translation.x, parentWorldTC.Translation.y, z };

			bool isSelected = (entity.GetUUID() == selectedUUID);

			for (UUID childUUID : children)
			{
				Entity childEntity = scene->FindEntityByUUID(childUUID);
				if (!childEntity || !childEntity.HasComponent<TransformComponent>())
					continue;

				bool childSelected = (childUUID == selectedUUID);
				glm::vec4 color = (isSelected || childSelected) ? selectedColor : defaultColor;

				TransformComponent childWorldTC = scene->GetWorldSpaceTransform(childEntity);
				glm::vec3 childPos = { childWorldTC.Translation.x, childWorldTC.Translation.y, z };

				Renderer2D::DrawLine(parentPos, childPos, color);
			}
		}
	}

}
