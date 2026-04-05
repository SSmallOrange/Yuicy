#pragma once

#include "Yuicy/Scene/Scene.h"

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
		inline static std::string_view FileFilter = "Yuicy Scene (*.yui)\0*.yui\0";
		inline static std::string_view DefaultExtension = ".yui";

		static const char* GetSceneSerializerFileFilter() { return FileFilter.data(); }
		static const char* GetSceneSerializerDefaultExtension() { return DefaultExtension.data(); }

	private:
		void SerializeEntity(YAML::Emitter& out, Entity entity);
		void DeserializeEntities(YAML::Node& entitiesNode);

	private:
		Ref<Scene> m_scene;
	};

}
