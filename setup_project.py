#!/usr/bin/env python3
"""Add the prebuilt WebGPU (Dawn) stack from dawn-setup/vendor to a project.

Usage:
    python3 setup_project.py <project_path> [--mode copy|link] [--only webgpu,glfw,...] [--force]

Components (all by default):
    webgpu     Dawn headers + static library   -> vendor/webgpu/{include,lib}
    glfw       GLFW headers + static library   -> vendor/glfw/{include,lib}
    glfw_wgpu  WebGPU surface for a GLFW window (needs webgpu, glfw)
    imgui      Dear ImGui (docking) source + GLFW/WebGPU backends (needs webgpu, glfw)
    cimgui     C bindings for ImGui (needs imgui)

Modes:
    copy  (default) copy the files into <project>/vendor; the project is self-contained.
    link  symlink each <project>/vendor/<component> to dawn-setup/vendor/<component>;
          nothing is duplicated, but the project only builds on this machine.

vendor/CMakeLists.txt is always copied (never linked) so you can edit it per project.
Its INCLUDE_* toggles default to ON for exactly the components that were added.
Prefer fetching dawn-setup with FetchContent instead (see README); this script is the
offline/self-contained route.
In your CMakeLists.txt:

    project(my_app C CXX)
    add_subdirectory(vendor)
    target_link_libraries(my_app PRIVATE webgpu glfw glfw_wgpu imgui)
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

SOURCE_VENDOR = Path(__file__).resolve().parent / "vendor"

COMPONENTS = ["webgpu", "glfw", "glfw_wgpu", "imgui", "cimgui"]
REQUIRES = {
    "webgpu": [],
    "glfw": [],
    "glfw_wgpu": ["webgpu", "glfw"],
    "imgui": ["webgpu", "glfw"],
    "cimgui": ["imgui"],
}

# vendor/CMakeLists.txt toggle -> component it controls.
TOGGLES = {"INCLUDE_DAWN": "webgpu", "INCLUDE_GLFW": "glfw", "INCLUDE_IMGUI": "imgui", "INCLUDE_CIMGUI": "cimgui"}


def resolve_components(only: str | None) -> list[str]:
    if not only:
        return [c for c in COMPONENTS if c != "cimgui"]
    wanted = {c.strip() for c in only.split(",") if c.strip()}
    unknown = wanted - set(COMPONENTS)
    if unknown:
        sys.exit(f"error: unknown component(s): {', '.join(sorted(unknown))} "
                 f"(choose from {', '.join(COMPONENTS)})")
    # Pull in dependencies so the generated CMake targets always resolve.
    changed = True
    while changed:
        changed = False
        for comp in list(wanted):
            for dep in REQUIRES[comp]:
                if dep not in wanted:
                    print(f"note: {comp} needs {dep}, adding it")
                    wanted.add(dep)
                    changed = True
    # CMake builds glfw_wgpu whenever both Dawn and GLFW are enabled.
    if {"webgpu", "glfw"} <= wanted:
        wanted.add("glfw_wgpu")
    return [c for c in COMPONENTS if c in wanted]


def remove_existing(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)


def ensure_gitignored(project: Path, entries: list[str]) -> list[str]:
    gitignore = project / ".gitignore"
    lines = gitignore.read_text().splitlines() if gitignore.exists() else []
    missing = [e for e in entries if e not in lines]
    if missing:
        with gitignore.open("a") as f:
            if lines and lines[-1] != "":
                f.write("\n")
            f.write("\n".join(missing) + "\n")
    return missing


def main() -> int:
    parser = argparse.ArgumentParser(description="Copy or link the prebuilt WebGPU stack into a project.")
    parser.add_argument("project", type=Path, help="path to the project root")
    parser.add_argument("--mode", choices=("copy", "link"), default="copy",
                        help="copy the files (default) or symlink to dawn-setup")
    parser.add_argument("--only", help="comma-separated subset of: " + ", ".join(COMPONENTS))
    parser.add_argument("--force", action="store_true",
                        help="replace components and vendor/CMakeLists.txt that already exist")
    args = parser.parse_args()

    project = args.project.expanduser().resolve()
    if not project.is_dir():
        print(f"error: {project} is not a directory", file=sys.stderr)
        return 1

    components = resolve_components(args.only)
    vendor = project / "vendor"
    cmake_file = vendor / "CMakeLists.txt"

    conflicts = [vendor / c for c in components if (vendor / c).exists() or (vendor / c).is_symlink()]
    if cmake_file.exists():
        conflicts.append(cmake_file)
    if conflicts and not args.force:
        for path in conflicts:
            print(f"error: {path} already exists", file=sys.stderr)
        print("use --force to replace", file=sys.stderr)
        return 1

    vendor.mkdir(parents=True, exist_ok=True)
    for comp in components:
        src, dst = SOURCE_VENDOR / comp, vendor / comp
        remove_existing(dst)
        if args.mode == "link":
            dst.symlink_to(src, target_is_directory=True)
            print(f"linked  vendor/{comp} -> {src}")
        else:
            shutil.copytree(src, dst)
            print(f"copied  vendor/{comp}")

    cmake_text = (SOURCE_VENDOR / "CMakeLists.txt").read_text()
    for toggle, comp in TOGGLES.items():
        cmake_text = re.sub(rf'(option\({toggle}\s+"[^"]*"\s+)(ON|OFF)\)',
                            rf'\g<1>{"ON" if comp in components else "OFF"})', cmake_text)
    cmake_file.write_text(cmake_text)
    print("wrote   vendor/CMakeLists.txt")

    # Symlinks to absolute paths on this machine are useless to anyone else.
    if args.mode == "link" and (project / ".git").exists():
        for entry in ensure_gitignored(project, [f"/vendor/{c}" for c in components]):
            print(f"added   {entry} to .gitignore")

    targets = " ".join(components)
    print("\nIn your CMakeLists.txt:\n"
          "    project(my_app C CXX)\n"
          "    add_subdirectory(vendor)\n"
          f"    target_link_libraries(my_app PRIVATE {targets})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
