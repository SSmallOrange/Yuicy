#pragma once

#include "Yuicy/Asset/AssetRegistry.h"

#include <filesystem>
#include <optional>

namespace Yuicy {

	// 资产注册表文件（.yregistry）的读写，路径按 UTF-8 存储、以 '/' 分隔
	class AssetRegistrySerializer
	{
	public:
		// 条目按 Handle 升序写出；写入失败时记录错误并返回 false
		static bool Serialize(const AssetRegistry& registry, const std::filesystem::path& filepath);
		// 文件无法读取或格式错误时记录错误并返回 std::nullopt；Handle 为 0 或类型无法识别的条目跳过
		static std::optional<AssetRegistry> Deserialize(const std::filesystem::path& filepath);
	};

}
