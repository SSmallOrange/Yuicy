#include "pch.h"
#include "Yuicy/Core/PlatformUtils.h"

#import <Cocoa/Cocoa.h>

namespace Yuicy {

	// 路径最终由 Finder 等其他进程解析，相对路径必须先按本进程的工作目录转成绝对路径
	static NSString* ToAbsolutePathString(const std::filesystem::path& path)
	{
		std::error_code ec;
		std::filesystem::path absolutePath = std::filesystem::absolute(path, ec);
		if (ec)
			return nil;

		const std::string& native = absolutePath.native();
		return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:native.c_str() length:native.size()];
	}

	bool PlatformUtils::RevealInFileBrowser(const std::filesystem::path& path)
	{
		@autoreleasepool
		{
			NSString* pathString = ToAbsolutePathString(path);
			if (!pathString)
				return false;

			std::error_code ec;
			NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
			// fullPath 为 nil 时打开 rootFullPath 目录本身；rootFullPath 为 @"" 时在已有的 Finder 窗口中选中文件
			if (std::filesystem::is_directory(path, ec))
				return [workspace selectFile:nil inFileViewerRootedAtPath:pathString];

			return [workspace selectFile:pathString inFileViewerRootedAtPath:@""];
		}
	}

	bool PlatformUtils::OpenWithDefaultApp(const std::filesystem::path& path)
	{
		@autoreleasepool
		{
			NSString* pathString = ToAbsolutePathString(path);
			if (!pathString)
				return false;

			return [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:pathString]];
		}
	}

}
