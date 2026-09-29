#include "pch.h"
#include "Yuicy/Core/FileDialogs.h"

#include "Yuicy/Core/Application.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace Yuicy {

	// NSOpenPanel / NSSavePanel 没有类型下拉框，FileDialogFilter::Name 在 macOS 上不显示；
	// 返回空数组表示不限制类型
	static NSArray<UTType*>* ToContentTypes(std::span<const FileDialogFilter> filters)
	{
		NSMutableArray<UTType*>* types = [NSMutableArray array];
		for (const FileDialogFilter& filter : filters)
		{
			if (filter.Extensions.empty())
				return @[];

			for (const std::string& extension : filter.Extensions)
			{
				NSString* extensionString = [NSString stringWithUTF8String:extension.c_str()];
				if (!extensionString)
					continue;

				// 未在系统中注册的扩展名（如 yui）会得到动态 UTType，同样可用于过滤
				UTType* type = [UTType typeWithFilenameExtension:extensionString];
				if (type)
					[types addObject:type];
			}
		}
		return types;
	}

	static std::optional<std::filesystem::path> RunPanel(NSSavePanel* panel)
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		NSWindow* parentWindow = glfwGetCocoaWindow(window);

		// 用 runModal 而不是 sheet：sheet 是异步回调，同步接口需要再套一层模态循环
		NSModalResponse response = [panel runModal];
		// 面板关闭后 key window 不一定回到 GLFW 窗口，不恢复的话键盘输入会丢失
		[parentWindow makeKeyAndOrderFront:nil];

		if (response != NSModalResponseOK || !panel.URL)
			return std::nullopt;

		return std::filesystem::path(panel.URL.fileSystemRepresentation);
	}

	std::optional<std::filesystem::path> FileDialogs::OpenFile(std::span<const FileDialogFilter> filters)
	{
		@autoreleasepool
		{
			NSOpenPanel* panel = [NSOpenPanel openPanel];
			panel.canChooseFiles = YES;
			panel.canChooseDirectories = NO;
			panel.allowsMultipleSelection = NO;
			panel.allowedContentTypes = ToContentTypes(filters);
			return RunPanel(panel);
		}
	}

	std::optional<std::filesystem::path> FileDialogs::SaveFile(std::span<const FileDialogFilter> filters)
	{
		@autoreleasepool
		{
			NSSavePanel* panel = [NSSavePanel savePanel];
			panel.canCreateDirectories = YES;
			// 不带扩展名时 NSSavePanel 会补上第一个类型的扩展名，并自带覆盖确认
			panel.allowedContentTypes = ToContentTypes(filters);
			return RunPanel(panel);
		}
	}

}
