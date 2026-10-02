#!/usr/bin/env python3
"""Yuicy 一键自检：格式 → 配置 → 构建 → 单元测试。

用法（在任意目录执行均可）：
    python3 scripts/check.py                  # macOS / Linux
    py scripts\\check.py                      # Windows
    python3 scripts/check.py --config release
    python3 scripts/check.py --skip format
    python3 scripts/check.py --format-base main

全部通过时退出码为 0，否则为 1。只依赖 Python 3.8+ 标准库。
"""

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
STEPS = ("format", "configure", "build", "test")
# 与 git clang-format 默认处理的 C / C++ / ObjC 扩展名中本仓库用到的部分保持一致
SOURCE_SUFFIXES = {".h", ".hpp", ".inl", ".c", ".cpp", ".mm"}


class StepResult:
    def __init__(self, name, status, seconds=0.0, note=""):
        self.name = name
        self.status = status  # PASS / FAIL / SKIP
        self.seconds = seconds
        self.note = note


def print_header(text):
    print(f"\n==> {text}", flush=True)


def resolve_presets(config):
    # 与 CMakePresets.json 中的预设名对应：Windows 为多配置的 vs2022，其余平台为单配置的 debug / release
    if sys.platform == "win32":
        return {"configure": "vs2022", "build": f"vs2022-{config}", "test": f"vs2022-{config}"}
    return {"configure": config, "build": config, "test": config}


def require_tool(name, hint):
    if shutil.which(name) is None:
        print(f"找不到 {name}：{hint}", flush=True)
        return False
    return True


def run_streaming(command):
    print_header(" ".join(command))
    try:
        return subprocess.run(command, cwd=REPO_ROOT).returncode == 0
    except FileNotFoundError:
        print(f"找不到命令 {command[0]}", flush=True)
        return False


def run_captured(command):
    completed = subprocess.run(command, cwd=REPO_ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return completed.returncode, completed.stdout.decode("utf-8", errors="replace")


def list_untracked_sources():
    # git clang-format 只看 git diff，未跟踪的新文件需要单独整文件检查
    code, output = run_captured(["git", "ls-files", "--others", "--exclude-standard"])
    if code != 0:
        return None
    return [line for line in output.splitlines() if Path(line).suffix in SOURCE_SUFFIXES]


def check_format(base):
    if not require_tool("git-clang-format", "安装 LLVM / clang-format（macOS：brew install clang-format）"):
        return False
    if not require_tool("clang-format", "安装 LLVM / clang-format（macOS：brew install clang-format）"):
        return False

    print_header(f"git clang-format --diff {base}")
    code, output = run_captured(["git", "clang-format", "--diff", base])
    print(output, end="", flush=True)
    # 旧版本 git clang-format 有差异时退出码仍为 0，因此同时检查输出
    tracked_ok = code == 0 and "diff --git" not in output
    if not tracked_ok and code not in (0, 1):
        print(f"git clang-format 异常退出（{code}）", flush=True)

    untracked = list_untracked_sources()
    if untracked is None:
        print("无法列出未跟踪文件", flush=True)
        return False
    untracked_ok = True
    if untracked:
        print_header(f"clang-format --dry-run --Werror（{len(untracked)} 个未跟踪文件）")
        code, output = run_captured(["clang-format", "--dry-run", "--Werror", *untracked])
        print(output, end="", flush=True)
        untracked_ok = code == 0

    if not (tracked_ok and untracked_ok):
        # git clang-format 拒绝修改带未暂存改动的文件，所以要先 git add
        print("格式不符合 .clang-format。已跟踪文件：git add 后执行 git clang-format；未跟踪文件：clang-format -i <文件>", flush=True)
    return tracked_ok and untracked_ok


def run_step(name, action):
    start = time.monotonic()
    ok = action()
    return StepResult(name, "PASS" if ok else "FAIL", time.monotonic() - start)


def print_summary(results):
    print("\n==================== 自检结果 ====================")
    for result in results:
        seconds = f"{result.seconds:6.1f}s" if result.status != "SKIP" else "       "
        note = f"  {result.note}" if result.note else ""
        print(f"  {result.name:<10} {result.status:<4} {seconds}{note}")
    failed = [result.name for result in results if result.status == "FAIL"]
    print(f"自检未通过：{', '.join(failed)}" if failed else "自检通过", flush=True)
    return not failed


def parse_args():
    parser = argparse.ArgumentParser(description="Yuicy 一键自检：格式 → 配置 → 构建 → 单元测试")
    parser.add_argument("--config", choices=("debug", "release"), default="debug", help="构建配置，默认 debug")
    parser.add_argument("--skip", nargs="+", choices=STEPS, default=[], metavar="STEP",
                        help=f"跳过的步骤：{' / '.join(STEPS)}")
    parser.add_argument("--format-base", default="HEAD", metavar="REF",
                        help="格式检查的比较基准，默认 HEAD（只检查未提交的改动）；检查整个分支时传 main 等")
    return parser.parse_args()


def main():
    # Windows 控制台编码不是 UTF-8 时，避免中文输出抛出 UnicodeEncodeError
    sys.stdout.reconfigure(errors="replace")
    args = parse_args()
    presets = resolve_presets(args.config)
    results = []

    # 格式检查最快、与构建无关，放在最前面并且失败后继续执行
    if "format" in args.skip:
        results.append(StepResult("format", "SKIP", note="--skip"))
    else:
        results.append(run_step("format", lambda: check_format(args.format_base)))

    # 配置 → 构建 → 测试 前一步失败时后面的结果没有意义，直接跳过
    pipeline = [
        ("configure", ["cmake", "--preset", presets["configure"]]),
        ("build", ["cmake", "--build", "--preset", presets["build"]]),
        ("test", ["ctest", "--preset", presets["test"]]),
    ]
    blocked_by = None
    for name, command in pipeline:
        if name in args.skip:
            results.append(StepResult(name, "SKIP", note="--skip"))
        elif blocked_by:
            results.append(StepResult(name, "SKIP", note=f"{blocked_by} 失败"))
        else:
            result = run_step(name, lambda command=command: run_streaming(command))
            results.append(result)
            if result.status == "FAIL":
                blocked_by = name

    passed = print_summary(results)
    # 根 CMakeLists.txt 让根目录的 compile_commands.json 指向最后一次配置的预设
    if presets["configure"] != "debug" and "configure" not in args.skip and sys.platform != "win32":
        print(f"注意：根目录 compile_commands.json 已指向 build/{presets['configure']}，执行 cmake --preset debug 可切回 Debug")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
