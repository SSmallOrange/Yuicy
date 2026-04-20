#pragma once

#include "../EditorCommand.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

#include <glm/glm.hpp>

namespace Yuicy {

	// Transform 修改命令
	// 支持 Gizmo 连续拖拽合并：同一实体的连续 Transform 修改会合并为一条命令
	class SetTransformCommand : public IEditorCommand
	{
	public:
		SetTransformCommand(Scene* scene, UUID entityUUID,
			const glm::vec3& oldTranslation, const glm::vec3& oldRotation, const glm::vec3& oldScale,
			const glm::vec3& newTranslation, const glm::vec3& newRotation, const glm::vec3& newScale)
			: m_scene(scene), m_entityUUID(entityUUID)
			, m_oldTranslation(oldTranslation), m_oldRotation(oldRotation), m_oldScale(oldScale)
			, m_newTranslation(newTranslation), m_newRotation(newRotation), m_newScale(newScale)
		{
		}

		void Execute() override
		{
			ApplyTransform(m_newTranslation, m_newRotation, m_newScale);
		}

		void Undo() override
		{
			ApplyTransform(m_oldTranslation, m_oldRotation, m_oldScale);
		}

		std::string GetName() const override { return "Set Transform"; }

	private:
		void ApplyTransform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
		{
			if (!m_scene)
				return;

			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (!entity)
				return;

			auto& tc = entity.GetComponent<TransformComponent>();
			tc.Translation = translation;
			tc.Rotation = rotation;
			tc.Scale = scale;
		}

		Scene* m_scene = nullptr;
		UUID m_entityUUID;

		glm::vec3 m_oldTranslation, m_oldRotation, m_oldScale;
		glm::vec3 m_newTranslation, m_newRotation, m_newScale;
	};

}
