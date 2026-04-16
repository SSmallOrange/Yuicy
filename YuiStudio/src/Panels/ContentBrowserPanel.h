#pragma once

#include "Yuicy.h"

#include <filesystem>

namespace Yuicy {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();

	private:
		std::filesystem::path m_baseDirectory;
		std::filesystem::path m_currentDirectory;

		Ref<Texture2D> m_directoryIcon;
		Ref<Texture2D> m_fileIcon;
		Ref<Texture2D> m_backIcon;

		float m_thumbnailSize = 96.0f;
		float m_padding = 16.0f;
	};

}
