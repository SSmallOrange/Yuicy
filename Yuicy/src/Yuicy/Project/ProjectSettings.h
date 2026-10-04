#pragma once

#include "Yuicy/Physics/Physics2DSettings.h"
#include "Yuicy/Renderer/Renderer2DSettings.h"

#include <filesystem>
#include <string>

namespace Yuicy {

	// 运行时使用的项目级配置
	struct ProjectSettings
	{
		std::string Name = "Untitled";
		std::filesystem::path AssetDirectory = "Assets";  // 相对项目目录
		std::filesystem::path StartScene;                 // 相对资产目录，空表示未设置

		Renderer2DSettings Renderer2D;
		Physics2DSettings Physics2D;
	};

}
