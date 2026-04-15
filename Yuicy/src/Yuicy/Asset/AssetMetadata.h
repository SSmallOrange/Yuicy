#pragma once

#include "Asset.h"

#include <filesystem>

namespace Yuicy {

	struct AssetMetadata
	{
		AssetHandle handle = 0;
		AssetType type = AssetType::None;
		std::filesystem::path filePath;  // 相对于 AssetDirectory 的路径

		bool isDataLoaded = false;

		bool IsValid() const { return handle != 0; }
	};

}
