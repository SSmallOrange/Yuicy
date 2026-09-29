#include "pch.h"
#include "OpenGLTexture.h"
#include "OpenGLDebug.h"

#include "stb_image.h"

#include <glad/glad.h>

namespace Yuicy {

	// 会临时占用当前活动纹理单元，结束后解绑；调用方不能依赖调用之后的纹理绑定状态
	static void CreateTextureStorage(uint32_t& rendererID, GLenum internalFormat, GLenum dataFormat,
		uint32_t width, uint32_t height, const void* data)
	{
		glGenTextures(1, &rendererID);
		glBindTexture(GL_TEXTURE_2D, rendererID);

		// Filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Wrap
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// 只分配第 0 级：MIN_FILTER 改为 mipmap 过滤前必须先生成 mipmap，否则纹理不完整、采样结果为黑色
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

		glBindTexture(GL_TEXTURE_2D, 0);
		OpenGLCheckErrors("OpenGLTexture2D create");
	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		YUICY_PROFILE_FUNCTION();

		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		CreateTextureStorage(m_RendererID, m_InternalFormat, m_DataFormat, m_Width, m_Height, nullptr);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
		: m_Path(path)
	{
		YUICY_PROFILE_FUNCTION();

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);  // 垂直翻转图片，对其原点
		stbi_uc* data = nullptr;
		{
			YUICY_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
			data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		}
		YUICY_ASSERT(data, "Failed to load image!");

		m_Width = width;
		m_Height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		YUICY_ASSERT(internalFormat & dataFormat, "Format not supported!");

		// 向GPU提交纹理数据
		CreateTextureStorage(m_RendererID, internalFormat, dataFormat, m_Width, m_Height, data);

		stbi_image_free(data);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		YUICY_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
		YUICY_PROFILE_FUNCTION();

		// bytes per pixel
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		YUICY_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		YUICY_PROFILE_FUNCTION();

		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
	}
}
