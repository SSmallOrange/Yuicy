#pragma once

#include "Yuicy/Renderer/Camera.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Renderer/SubTexture.h"
#include "Yuicy/Renderer/OrthographicCamera.h"

namespace Yuicy {

	class EditorCamera;

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const EditorCamera& camera);
		static void BeginScene(const OrthographicCamera& camera);
		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void EndScene();
		static void Flush();

		// 纯色
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		
		// 纹理
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f /* 纹理平铺系数 */, const glm::vec4& tintColor = glm::vec4(1.0f) /*调色因子*/);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		
		// 子纹理
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		// 矩阵
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
		
		// 翻转纹理
		static void DrawSprite(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, bool flipX, bool flipY, int entityID = -1);
		static void DrawSprite(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintColor, bool flipX, bool flipY, int entityID = -1);

		// 旋转矩形
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		// 线段
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color = glm::vec4(1.0f));

		// 线框矩形
		static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawRect(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

		// 圆
		static void DrawCircle(const glm::vec3& position, float radius, const glm::vec4& color = glm::vec4(1.0f), int segments = 32);
		static void DrawCircle(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f), int segments = 32);

		// 填充圆
		static void FillCircle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
		static void FillCircle(const glm::vec3& position, float radius, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
		static void FillCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		static float GetLineWidth();
		static void SetLineWidth(float width);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t LineCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4 + LineCount * 2; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6 + LineCount * 2; }
		};

		static void ResetStats();
		static Statistics GetStats();

	private:
		static void BeginScene(const glm::mat4& viewProjection);
		static void FlushAndReset();
	};

}
