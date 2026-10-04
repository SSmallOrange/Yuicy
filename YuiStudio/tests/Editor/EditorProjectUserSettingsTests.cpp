#include "pch.h"

#include "Editor/EditorProjectUserSettings.h"

#include <fstream>
#include <sstream>

using namespace Yuicy;

namespace {

	std::string ReadTextFile(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream content;
		content << stream.rdbuf();
		return content.str();
	}

}

TEST_SUITE("Editor")
{
	TEST_CASE("EditorProjectUserSettings round-trip preserves AutoSave")
	{
		Test::ScopedTempDirectory tempDirectory;

		EditorProjectUserSettings source;
		source.autoSave.enabled = true;
		source.autoSave.intervalSeconds = 42;
		REQUIRE(EditorProjectUserSettingsSerializer::Serialize(source, tempDirectory.GetPath()));

		const std::filesystem::path filepath = EditorProjectUserSettingsSerializer::GetFilePath(tempDirectory.GetPath());
		CHECK(filepath == tempDirectory.GetPath() / ".yuistudio" / "EditorUserSettings.yaml");
		CHECK(std::filesystem::is_regular_file(filepath));

		const std::optional<EditorProjectUserSettings> loaded = EditorProjectUserSettingsSerializer::Deserialize(tempDirectory.GetPath());
		REQUIRE(loaded);
		CHECK(loaded->autoSave.enabled);
		CHECK(loaded->autoSave.intervalSeconds == 42);
	}

	TEST_CASE("EditorProjectUserSettings fills defaults for missing keys")
	{
		Test::ScopedTempDirectory tempDirectory;
		tempDirectory.WriteFile(".yuistudio/EditorUserSettings.yaml", "AutoSave:\n  Enabled: true\n");

		const std::optional<EditorProjectUserSettings> loaded = EditorProjectUserSettingsSerializer::Deserialize(tempDirectory.GetPath());
		REQUIRE(loaded);
		CHECK(loaded->autoSave.enabled);
		CHECK(loaded->autoSave.intervalSeconds == EditorAutoSaveSettings().intervalSeconds);
	}

	TEST_CASE("EditorProjectUserSettings ignores its own directory in git")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path gitignore = tempDirectory.GetPath() / ".yuistudio" / ".gitignore";

		SUBCASE("generated gitignore ignores everything in the directory")
		{
			REQUIRE(EditorProjectUserSettingsSerializer::Serialize({}, tempDirectory.GetPath()));
			CHECK(ReadTextFile(gitignore).find("\n*\n") != std::string::npos);
		}

		SUBCASE("existing gitignore is not overwritten")
		{
			tempDirectory.WriteFile(".yuistudio/.gitignore", "custom\n");
			REQUIRE(EditorProjectUserSettingsSerializer::Serialize({}, tempDirectory.GetPath()));
			CHECK(ReadTextFile(gitignore) == "custom\n");
		}
	}

	TEST_CASE("EditorProjectUserSettings rejects unreadable files")
	{
		Test::ScopedTempDirectory tempDirectory;

		SUBCASE("missing file")
		{
			CHECK_FALSE(EditorProjectUserSettingsSerializer::Deserialize(tempDirectory.GetPath()));
		}

		SUBCASE("malformed YAML")
		{
			tempDirectory.WriteFile(".yuistudio/EditorUserSettings.yaml", "AutoSave: [unclosed\n");
			CHECK_FALSE(EditorProjectUserSettingsSerializer::Deserialize(tempDirectory.GetPath()));
		}

		SUBCASE("wrong value type")
		{
			tempDirectory.WriteFile(".yuistudio/EditorUserSettings.yaml", "AutoSave:\n  IntervalSeconds: often\n");
			CHECK_FALSE(EditorProjectUserSettingsSerializer::Deserialize(tempDirectory.GetPath()));
		}
	}
}
