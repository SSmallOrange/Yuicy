#!/usr/bin/env bash
# 用法：
#   ./GenerateProject.sh                                                   # 导出 Debug 配置的编译数据库
#   ./GenerateProject.sh export-compile-commands --export-compile-commands-config=Release
#   PREMAKE5=/path/to/premake5 ./GenerateProject.sh                        # 使用指定的 premake5
#
# premake5 首次运行时下载到 bin/tools/ 并校验 SHA256；升级版本时需同步修改 premake/premake.ps1（Windows）。

set -euo pipefail

readonly PREMAKE_VERSION="5.0.0-beta7"
readonly PREMAKE_SHA256_MACOSX="f7a6d4978960aefbc0b2c522f211eedf014e6a2783b04a096c3716213c1bff67"
readonly PREMAKE_SHA256_LINUX="805114ae7002fe90b643630aa0947476565071bfcefa237d97f95df836e0f2f1"

cd "$(dirname "$0")"

sha256_of() {
	if command -v shasum >/dev/null 2>&1; then
		shasum -a 256 "$1" | awk '{ print $1 }'
	else
		sha256sum "$1" | awk '{ print $1 }'
	fi
}

ensure_premake() {
	if [[ -n "${PREMAKE5:-}" ]]; then
		echo "$PREMAKE5"
		return
	fi

	local os expected
	case "$(uname -s)" in
		Darwin) os="macosx"; expected="$PREMAKE_SHA256_MACOSX" ;;
		Linux)  os="linux";  expected="$PREMAKE_SHA256_LINUX" ;;
		*)      echo "error: unsupported host '$(uname -s)', use GenerateProject.bat on Windows" >&2; exit 1 ;;
	esac

	local dir="bin/tools/premake-$PREMAKE_VERSION"
	local exe="$dir/premake5"
	if [[ -x "$exe" ]]; then
		echo "$exe"
		return
	fi

	local url="https://github.com/premake/premake-core/releases/download/v$PREMAKE_VERSION/premake-$PREMAKE_VERSION-$os.tar.gz"
	tmp="$(mktemp -d)"
	trap 'rm -rf "${tmp:-}"' EXIT

	echo "Downloading premake $PREMAKE_VERSION ($os)..." >&2
	curl -fsSL --proto '=https' --tlsv1.2 -o "$tmp/premake.tar.gz" "$url"

	local actual
	actual="$(sha256_of "$tmp/premake.tar.gz")"
	if [[ "$actual" != "$expected" ]]; then
		echo "error: premake checksum mismatch (expected $expected, got $actual)" >&2
		exit 1
	fi

	tar -xzf "$tmp/premake.tar.gz" -C "$tmp"
	mkdir -p "$dir"
	mv "$tmp/premake5" "$exe"
	chmod +x "$exe"
	echo "$exe"
}

premake="$(ensure_premake)"

if [[ $# -eq 0 ]]; then
	set -- export-compile-commands
fi

"$premake" "$@"
