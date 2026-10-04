#include "pch.h"

#include "Editor/EditorContext.h"
#include "Editor/EditorDirtyTracker.h"
#include "Editor/EditorProjectSession.h"

using namespace Yuicy;

TEST_SUITE("Editor")
{
	TEST_CASE("EditorDirtyTracker auto-saves with the settings of the open project")
	{
		Test::ScopedTempDirectory tempDirectory;
		EditorContext context;
		context.project = EditorProjectSession::Create(tempDirectory.GetPath() / "Game.yproj");
		REQUIRE(context.project);
		EditorAutoSaveSettings& autoSave = context.project->GetUserSettings().autoSave;

		int saveCount = 0;
		EditorDirtyTracker tracker;
		tracker.SetContext(&context);
		tracker.SetAutoSaveCallback([&saveCount] { saveCount++; });
		tracker.MarkSceneDirty();

		SUBCASE("disabled")
		{
			autoSave.enabled = false;
			autoSave.intervalSeconds = 1;
			tracker.OnUpdate(2.0f);
			CHECK(saveCount == 0);
		}

		SUBCASE("enabled")
		{
			autoSave.enabled = true;
			autoSave.intervalSeconds = 2;
			tracker.OnUpdate(1.5f);
			CHECK(saveCount == 0);
			tracker.OnUpdate(1.0f);
			CHECK(saveCount == 1);
		}

		SUBCASE("project closed")
		{
			autoSave.enabled = true;
			autoSave.intervalSeconds = 1;
			context.project.reset();
			tracker.OnUpdate(2.0f);
			CHECK(saveCount == 0);
		}
	}
}
