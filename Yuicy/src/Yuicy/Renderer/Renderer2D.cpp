#include "pch.h"
#include "Yuicy/Renderer/Renderer2D.h"

#include "Yuicy/Renderer/Shader.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Yuicy/Renderer/VertexArray.h"
#include "Yuicy/Renderer/RenderCommand.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Yuicy {
	struct QuadVertex		// 矩形顶点数据
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;	 // 纹理平铺系数
		int EntityID;
	};

	struct LineVertex		// 线段顶点数据
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct CircleVertex		// 圆形顶点数据（SDF）
	{
		glm::vec3 WorldPosition;
		glm::vec2 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;
		int EntityID;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 20000;					// Max Quad Size
		static const uint32_t MaxVertices = MaxQuads * 4;		// Max VBO Size
		static const uint32_t MaxIndices = MaxQuads * 6;		// Max EBO Size
		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxCircles = 5000;
		static const uint32_t MaxCircleVertices = MaxCircles * 4;
		static const uint32_t MaxCircleIndices = MaxCircles * 6;
		static const uint32_t MaxTextureSlots = 32;		// TODO: RenderCaps

		// 矩形顶点
		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;					// Draw Quad Index Count, Add 6 Once
		QuadVertex* QuadVertexBufferBase = nullptr;		// Quad Vertex Data
		QuadVertex* QuadVertexBufferPtr = nullptr;		// Quad Vertex Index

		// 线段顶点
		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		float LineWidth = 2.0f;

		// 圆顶点
		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;					// 0 = white texture

		std::array<glm::vec4, 4> QuadVertexPositions;

		Renderer2D::Statistics Stats = {};					// 批处理状态
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		YUICY_PROFILE_FUNCTION();

		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float, "a_TexIndex" },
			{ ShaderDataType::Float, "a_TilingFactor" },
			{ ShaderDataType::Int, "a_EntityID" }
		});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
			samplers[i] = i;

		// 矩形
		s_Data.TextureShader = Shader::Create("assets/shaders/Texture.glsl");
		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[2] = { 0.5f, 0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f, 0.5f, 0.0f, 1.0f };

		// 线段
		s_Data.LineVertexArray = VertexArray::Create();
		s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxLineVertices * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
		});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxLineVertices];
		s_Data.LineShader = Shader::Create("assets/shaders/Line.glsl");

		// 圆
		s_Data.CircleVertexArray = VertexArray::Create();
		s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxCircleVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float2, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float, "a_Thickness" },
			{ ShaderDataType::Float, "a_Fade" },
			{ ShaderDataType::Int, "a_EntityID" }
		});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
		s_Data.CircleVertexArray->SetIndexBuffer(s_Data.QuadVertexArray->GetIndexBuffer());
		s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxCircleVertices];
		s_Data.CircleShader = Shader::Create("assets/shaders/Circle.glsl");
	}

	void Renderer2D::Shutdown()
	{
		YUICY_PROFILE_FUNCTION();

		delete[] s_Data.QuadVertexBufferBase;
		delete[] s_Data.LineVertexBufferBase;
		delete[] s_Data.CircleVertexBufferBase;
	}

	void Renderer2D::BeginScene(const glm::mat4& viewProjection)
	{
		YUICY_PROFILE_FUNCTION();

		s_Data.QuadIndexCount = 0;									// Reset IndexCount
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;	// Reset Index

		s_Data.TextureSlotIndex = 1;								// 默认有一个白色纹理

		s_Data.LineVertexCount = 0;										// Reset Line
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.CircleIndexCount = 0;									// Reset Circle
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

		// 设置VP矩阵
		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", viewProjection);

		s_Data.LineShader->Bind();
		s_Data.LineShader->SetMat4("u_ViewProjection", viewProjection);

		s_Data.CircleShader->Bind();
		s_Data.CircleShader->SetMat4("u_ViewProjection", viewProjection);

		s_Data.TextureShader->Bind();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		BeginScene(camera.GetViewProjectionMatrix());
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		BeginScene(camera.GetProjection() * glm::inverse(transform));
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		BeginScene(camera.GetViewProjection());
	}

	void Renderer2D::EndScene()
	{
		YUICY_PROFILE_FUNCTION();

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		if (dataSize)
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

		uint32_t lineDataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
		if (lineDataSize)
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, lineDataSize);

		uint32_t circleDataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
		if (circleDataSize)
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, circleDataSize);

		Flush();
	}

	void Renderer2D::Flush()
	{
		if (s_Data.QuadIndexCount)
		{
			s_Data.TextureShader->Bind();

			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
				s_Data.TextureSlots[i]->Bind(i);

			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.LineVertexCount)
		{
			s_Data.LineShader->Bind();
			RenderCommand::SetLineWidth(s_Data.LineWidth);
			RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.CircleIndexCount)
		{
			s_Data.CircleShader->Bind();
			RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::FlushAndReset()  // 超过批处理上限，执行绘制，刷新缓冲
	{
		EndScene();

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TextureSlotIndex = 1;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;
	}

	// ==================== 纯色 ====================
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		YUICY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	// ==================== 纹理 ====================
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		YUICY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	// ==================== 子纹理 ====================
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, subTexture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintColor)
	{
		YUICY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, subTexture, tilingFactor, tintColor);
	}

	// ==================== 矩阵 ====================
	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		YUICY_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		DrawSprite(transform, texture, tilingFactor, tintColor, false, false, entityID);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		DrawSprite(transform, subTexture, tilingFactor, tintColor, false, false, entityID);
	}

	// ==================== 翻转纹理====================

	void Renderer2D::DrawSprite(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, bool flipX, bool flipY, int entityID)
	{
		YUICY_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		// 查找或添加纹理
		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex++] = texture;
		}

		// 计算纹理坐标（支持翻转）
		// 默认: 左下(0,0) 右下(1,0) 右上(1,1) 左上(0,1)
		float u0 = flipX ? 1.0f : 0.0f;
		float u1 = flipX ? 0.0f : 1.0f;
		float v0 = flipY ? 1.0f : 0.0f;
		float v1 = flipY ? 0.0f : 1.0f;

		glm::vec2 textureCoords[4] = {
			{ u0, v0 },  // 左下
			{ u1, v0 },  // 右下
			{ u1, v1 },  // 右上
			{ u0, v1 }   // 左上
		};

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintColor, bool flipX, bool flipY, int entityID)
	{
		YUICY_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		const Ref<Texture2D> texture = subTexture->GetTexture();
		const glm::vec2* originalTexCoords = subTexture->GetTexCoords();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		// 查找或添加纹理
		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		// 处理子纹理的翻转
		// 原始纹理坐标: [0]=左下 [1]=右下 [2]=右上 [3]=左上
		glm::vec2 textureCoords[4];

		if (!flipX && !flipY)
		{
			// 不翻转
			textureCoords[0] = originalTexCoords[0];
			textureCoords[1] = originalTexCoords[1];
			textureCoords[2] = originalTexCoords[2];
			textureCoords[3] = originalTexCoords[3];
		}
		else if (flipX && !flipY)
		{
			// 水平翻转：左右交换
			textureCoords[0] = originalTexCoords[1];
			textureCoords[1] = originalTexCoords[0];
			textureCoords[2] = originalTexCoords[3];
			textureCoords[3] = originalTexCoords[2];
		}
		else if (!flipX && flipY)
		{
			// 垂直翻转：上下交换
			textureCoords[0] = originalTexCoords[3];
			textureCoords[1] = originalTexCoords[2];
			textureCoords[2] = originalTexCoords[1];
			textureCoords[3] = originalTexCoords[0];
		}
		else
		{
			// 水平+垂直翻转
			textureCoords[0] = originalTexCoords[2];
			textureCoords[1] = originalTexCoords[3];
			textureCoords[2] = originalTexCoords[0];
			textureCoords[3] = originalTexCoords[1];
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	// ==================== 旋转矩形 ====================
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		YUICY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		YUICY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (s_Data.LineVertexCount + 2 > Renderer2DData::MaxLineVertices)
			FlushAndReset();

		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
		s_Data.Stats.LineCount++;
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		DrawRect(transform, color);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color)
	{
		std::array<glm::vec3, 4> positions = {};
		for (size_t i = 0; i < 4; i++)
			positions[i] = glm::vec3(transform * s_Data.QuadVertexPositions[i]);

		DrawLine(positions[0], positions[1], color);
		DrawLine(positions[1], positions[2], color);
		DrawLine(positions[2], positions[3], color);
		DrawLine(positions[3], positions[0], color);
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, float radius, const glm::vec4& color, int segments)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(radius));
		DrawCircle(transform, color, segments);
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, int segments)
	{
		if (segments < 3)
			segments = 3;

		for (int i = 0; i < segments; i++)
		{
			// 单位圆 + transform
			float angle = 2.0f * glm::pi<float>() * (float)i / (float)segments;
			glm::vec4 startPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };

			float nextAngle = 2.0f * glm::pi<float>() * (float)((i + 1) % segments) / (float)segments;  // % segments 保证闭合
			glm::vec4 endPosition = { glm::cos(nextAngle), glm::sin(nextAngle), 0.0f, 1.0f };

			glm::vec3 p0 = glm::vec3(transform * startPosition);
			glm::vec3 p1 = glm::vec3(transform * endPosition);
			DrawLine(p0, p1, color);
		}
	}

	void Renderer2D::FillCircle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness, float fade, int entityID)
	{
		FillCircle({ position.x, position.y, 0.0f }, radius, color, thickness, fade, entityID);
	}

	void Renderer2D::FillCircle(const glm::vec3& position, float radius, const glm::vec4& color, float thickness, float fade, int entityID)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { radius * 2.0f, radius * 2.0f, 1.0f });
		FillCircle(transform, color, thickness, fade, entityID);
	}

	void Renderer2D::FillCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, float fade, int entityID)
	{
		YUICY_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;

		if (s_Data.CircleIndexCount + 6 > Renderer2DData::MaxCircleIndices)
			FlushAndReset();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = glm::vec3(transform * s_Data.QuadVertexPositions[i]);
			s_Data.CircleVertexBufferPtr->LocalPosition = {
				s_Data.QuadVertexPositions[i].x * 2.0f,
				s_Data.QuadVertexPositions[i].y * 2.0f
			};
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}
}
