#pragma once

#include <functional>

namespace Yuicy {

	struct EditorContext;

	// 编辑器脏状态追踪器
	class EditorDirtyTracker
	{
	public:
		EditorDirtyTracker() = default;
		~EditorDirtyTracker() = default;

		// 设置所操作的上下文
		void SetContext(EditorContext* context) { m_context = context; }

		// 标记场景为"已修改"
		void MarkSceneDirty();

		// 标记项目为"已修改"
		void MarkProjectDirty();

		// 场景保存后清除脏标记
		void ClearSceneDirty();

		// 项目保存后清除脏标记
		void ClearProjectDirty();

		// 当前场景是否为脏状态
		bool IsSceneDirty() const;

		// 当前项目是否为脏状态
		bool IsProjectDirty() const;

		// AutoSave 安全时机回调（如：非 Gizmo 拖拽中）
		using SafeToSaveCallback = std::function<bool()>;
		void SetIsSafeToAutoSave(SafeToSaveCallback callback) { m_isSafeToAutoSave = std::move(callback); }

		// AutoSave 执行回调
		using AutoSaveCallback = std::function<void()>;
		void SetAutoSaveCallback(AutoSaveCallback callback) { m_autoSaveCallback = std::move(callback); }

		// 每帧调用，驱动 AutoSave 计时器
		void OnUpdate(float deltaTime);

	private:
		EditorContext* m_context = nullptr;
		float m_autoSaveTimer = 0.0f;

		SafeToSaveCallback m_isSafeToAutoSave;
		AutoSaveCallback m_autoSaveCallback;
	};

}
