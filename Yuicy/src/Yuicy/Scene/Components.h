#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Renderer/SubTexture.h"
#include "Yuicy/Scene/SceneCamera.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace Yuicy {

	class ScriptableEntity;

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {
		}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };    // 弧度制
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {
		}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation) * rotation * glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct SpriteRendererComponent
	{
		// 基础颜色/着色
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

		// 纹理
		Ref<Texture2D> Texture = nullptr;
		Ref<SubTexture2D> SubTexture = nullptr;

		// 纹理属性
		float TilingFactor = 1.0f;              // 纹理平铺系数

		// 翻转
		bool FlipX = false;
		bool FlipY = false;

		// 渲染排序
		int SortingOrder = 0;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;

		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {
		}

		SpriteRendererComponent(const Ref<Texture2D>& texture)
			: Texture(texture) {
		}

		SpriteRendererComponent(const Ref<Texture2D>& texture, const glm::vec4& tint)
			: Texture(texture), Color(tint) {
		}

		SpriteRendererComponent(const Ref<SubTexture2D>& subTexture)
			: SubTexture(subTexture) {
		}

		SpriteRendererComponent(const Ref<SubTexture2D>& subTexture, const glm::vec4& tint)
			: SubTexture(subTexture), Color(tint) {
		}
	};

	struct CameraComponent
	{
		Yuicy::SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity* (*InstantiateScript)() = nullptr;
		void (*DestroyScript)(NativeScriptComponent*) = nullptr;

		template<typename T>
		void Bind()
		{
			InstantiateScript = [] {
				return static_cast<ScriptableEntity*>(new T());
			};

			DestroyScript = [](NativeScriptComponent* nsc) {
				delete nsc->Instance;
				nsc->Instance = nullptr;
			};
		}
	};

	struct LuaScriptComponent
	{
		std::string ScriptPath;
		
		// 脚本实例和回调函数
		sol::table ScriptInstance;
		sol::function OnCreateFunc;
		sol::function OnUpdateFunc;
		sol::function OnDestroyFunc;
		sol::function OnCollisionEnterFunc;		// 碰撞回调
		sol::function OnCollisionExitFunc;
		sol::function OnTriggerEnterFunc;		// 触发回调
		sol::function OnTriggerExitFunc;

		bool IsLoaded = false;

		LuaScriptComponent() = default;
		LuaScriptComponent(const LuaScriptComponent&) = default;
		LuaScriptComponent(const std::string& path)
			: ScriptPath(path) {
		}
	};

	// 动画片段
	struct AnimationClip
	{
		std::string Name;                            // 动画名称
		std::vector<Ref<SubTexture2D>> Frames;       // 动画帧序列
		float FrameDuration = 0.1f;                  // 每帧持续时间（秒）
		bool Loop = true;                            // 是否循环播放

		AnimationClip() = default;
		AnimationClip(const std::string& name, float frameDuration = 0.1f, bool loop = true)
			: Name(name), FrameDuration(frameDuration), Loop(loop) {
		}

		void AddFramesFromSheet(const Ref<Texture2D>& sheet,
			const glm::vec2& startCoord,
			int frameCount,
			const glm::vec2& cellSize,
			const glm::vec2& spriteSize = {1.0f, 1.0f},
			bool horizontal = true)
		{
			for (int i = 0; i < frameCount; i++)
			{
				glm::vec2 coord = startCoord;
				if (horizontal)
					coord.x += (float)i;
				else
					coord.y += (float)i;

				Frames.push_back(SubTexture2D::CreateFromCoords(sheet, coord, cellSize, spriteSize));
			}
		}
	};

	// 动画状态
	struct AnimationState
	{
		std::string CurrentClipName;    // 当前播放的动画名称
		int CurrentFrame = 0;           // 当前帧索引
		float Timer = 0.0f;             // 帧计时器
		bool Playing = true;            // 是否正在播放
		bool Finished = false;          // 非循环动画是否已播放完毕

		void Reset()
		{
			CurrentFrame = 0;
			Timer = 0.0f;
			Playing = true;
			Finished = false;
		}
	};

	// 动画组件
	struct AnimationComponent
	{
		std::unordered_map<std::string, AnimationClip> Clips;  // 动画片段
		AnimationState State;                                  // 当前播放状态

		AnimationComponent() = default;
		AnimationComponent(const AnimationComponent&) = default;

		// 添加动画剪辑
		void AddClip(const AnimationClip& clip)
		{
			Clips[clip.Name] = clip;
			// 如果是第一个剪辑，自动设为当前动画
			if (State.CurrentClipName.empty())
				State.CurrentClipName = clip.Name;
		}

		// 播放指定动画
		void Play(const std::string& clipName, bool forceRestart = false)
		{
			// 已经在播放且不需要重启
			if (State.CurrentClipName == clipName && !forceRestart && !State.Finished)
				return;

			if (Clips.find(clipName) == Clips.end())
			{
				YUICY_CORE_WARN("Animation clip '{}' not found!", clipName);
				return;
			}

			State.CurrentClipName = clipName;
			State.Reset();
		}

		// 停止播放
		void Stop()
		{
			State.Playing = false;
		}

		// 暂停播放
		void Pause()
		{
			State.Playing = false;
		}

		// 恢复播放
		void Resume()
		{
			State.Playing = true;
		}

		// 获取当前动画剪辑
		AnimationClip* GetCurrentClip()
		{
			if (Clips.find(State.CurrentClipName) != Clips.end())
				return &Clips[State.CurrentClipName];
			return nullptr;
		}

		// 获取当前帧的纹理
		Ref<SubTexture2D> GetCurrentFrame() const
		{
			auto it = Clips.find(State.CurrentClipName);
			if (it != Clips.end() && !it->second.Frames.empty())
			{
				int frameIndex = State.CurrentFrame % it->second.Frames.size();
				return it->second.Frames[frameIndex];
			}
			return nullptr;
		}

		// 检查动画是否正在播放
		bool IsPlaying() const { return State.Playing && !State.Finished; }

		// 检查动画是否已完成
		bool IsFinished() const { return State.Finished; }

		// 检查当前是否在播放指定动画
		bool IsPlaying(const std::string& clipName) const
		{
			return State.CurrentClipName == clipName && State.Playing && !State.Finished;
		}
	};

	// ==================== 碰撞过滤层定义 ====================
	namespace CollisionLayer
	{
		enum : uint16_t
		{
			None = 0,
			Default = 1 << 0,   // 0x0001
			Player  = 1 << 1,   // 0x0002
			Enemy   = 1 << 2,   // 0x0004
			Ground  = 1 << 3,   // 0x0008
			Trigger = 1 << 4,   // 0x0010
			Bullet  = 1 << 5,   // 0x0020
			All     = 0xFFFF
		};
	}
	// ==================== 物理组件 ====================

	// 刚体组件 - 定义物理实体的类型和属性
	struct Rigidbody2DComponent
	{
		// 刚体类型 静态（地面、墙壁）/动态（玩家、NPC）/运动学（电梯、传送带）
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;

		// 物理属性
		bool FixedRotation = false;  // 是否锁定旋转

		// Box2D 运行时刚体指针
		void* RuntimeBody = nullptr;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	};

	// 矩形碰撞体组件
	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };  // 相对于实体中心的偏移
		glm::vec2 Size = { 0.5f, 0.5f };    // 半尺寸（宽高的一半）

		// 物理材质属性
		float Density = 1.0f;       // 密度，影响质量
		float Friction = 0.5f;      // 摩擦系数
		float Restitution = 0.0f;   // 弹性系数（0=不弹，1=完全弹性）
		float RestitutionThreshold = 0.5f;  // 弹性速度阈值

		// 碰撞过滤
		uint16_t CategoryBits = CollisionLayer::Default;  // 自身所属层
		uint16_t MaskBits = CollisionLayer::All;          // 可碰撞的层

		// 触发器（不产生物理响应，只检测碰撞）
		bool IsTrigger = false;

		// Box2D 运行时夹具指针
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	// 圆形碰撞体组件
	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };   // 相对于实体中心的偏移
		float Radius = 0.5f;                 // 半径

		// 物理材质属性
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// 碰撞过滤
		uint16_t CategoryBits = CollisionLayer::Default;
		uint16_t MaskBits = CollisionLayer::All;

		// 触发器
		bool IsTrigger = false;

		// Box2D 运行时夹具指针
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};
}