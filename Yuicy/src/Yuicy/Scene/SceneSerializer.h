#pragma once

#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Core/FileDialogs.h"

#include <span>

namespace YAML {
	class Emitter;
	class Node;
}

namespace Yuicy {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);

	public:
		inline static const FileDialogFilter FileFilter{ "Yuicy Scene", { "yui" } };
		inline static std::string_view DefaultExtension = ".yui";

		static std::span<const FileDialogFilter> GetSceneSerializerFileFilter() { return { &FileFilter, 1 }; }
		static const char* GetSceneSerializerDefaultExtension() { return DefaultExtension.data(); }

	private:
		void SerializeEntity(YAML::Emitter& out, Entity entity);
		void DeserializeEntities(YAML::Node& entitiesNode);

	private:
		Ref<Scene> m_scene;
	};

}
