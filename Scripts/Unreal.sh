#!/bin/bash
set -euo pipefail

action="${1:-open}"
case "$action" in
    setup|build|open|generate|doctor) ;;
    *) echo "Usage: bash Scripts/Unreal.sh [setup|open|build|generate|doctor]" >&2; exit 1 ;;
esac
if [ "$#" -gt 1 ]; then
    echo "Usage: bash Scripts/Unreal.sh [setup|open|build|generate|doctor]" >&2
    exit 1
fi
if [ "$(uname -s)" != Darwin ]; then
    echo "This script requires macOS. On Windows, use Scripts/Unreal.ps1." >&2
    exit 1
fi

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
project="$project_dir/PokemonCyberpunk.uproject"
association="$(/usr/bin/plutil -extract EngineAssociation raw -o - "$project")"
if [[ ! "$association" =~ ^[0-9]+\.[0-9]+$ ]]; then
    echo "PokemonCyberpunk.uproject must use a portable major.minor EngineAssociation, not a local GUID." >&2
    exit 1
fi

engine_root="${UE_ENGINE_ROOT:-}"
if [ -z "$engine_root" ]; then
    # Read the Launcher's registration before checking conventional locations.
    manifest="$HOME/Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat"
    candidates=()
    if [ -f "$manifest" ]; then
        index=0
        while app_name="$(/usr/bin/plutil -extract "InstallationList.$index.AppName" raw -o - "$manifest" 2>/dev/null)"; do
            if [ "$app_name" = "UE_$association" ]; then
                candidates+=("$(/usr/bin/plutil -extract "InstallationList.$index.InstallLocation" raw -o - "$manifest")")
            fi
            index=$((index + 1))
        done
    fi
    candidates+=("/Users/Shared/Epic Games/UE_$association"
        "/Users/Shared/Epic Games/UE_$association.0"
        "/Users/Shared/Epic Games/UE_$association.0_SI")
    for candidate in "${candidates[@]}"; do
        version_file="$candidate/Engine/Build/Build.version"
        if [ -f "$version_file" ]; then
            major="$(/usr/bin/plutil -extract MajorVersion raw -o - "$version_file")"
            minor="$(/usr/bin/plutil -extract MinorVersion raw -o - "$version_file")"
            if [ "$major.$minor" = "$association" ]; then
                engine_root="$candidate"
                break
            fi
        fi
    done
fi
if [ -z "$engine_root" ] || [ ! -f "$engine_root/Engine/Build/Build.version" ]; then
    echo "UE $association.x not found. Set UE_ENGINE_ROOT to the installation folder containing Engine/." >&2
    exit 1
fi
engine_root="$(cd "$engine_root" && pwd)"
version_file="$engine_root/Engine/Build/Build.version"
major="$(/usr/bin/plutil -extract MajorVersion raw -o - "$version_file")"
minor="$(/usr/bin/plutil -extract MinorVersion raw -o - "$version_file")"
patch="$(/usr/bin/plutil -extract PatchVersion raw -o - "$version_file")"
if [ "$major.$minor" != "$association" ]; then
    echo "UE $association.x is required by PokemonCyberpunk.uproject; found $major.$minor.$patch at $engine_root." >&2
    exit 1
fi

batch_dir="$engine_root/Engine/Build/BatchFiles/Mac"
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app"
for required in "$batch_dir/Build.sh" "$batch_dir/GenerateProjectFiles.sh"; do
    if [ ! -f "$required" ]; then
        echo "Incomplete macOS engine installation: missing $required." >&2
        exit 1
    fi
done
if [ ! -d "$editor" ]; then
    echo "Incomplete macOS engine installation: missing $editor." >&2
    exit 1
fi
echo "Project: $project"
echo "Using Unreal Engine $major.$minor.$patch (association $association): $engine_root"
if [ "$action" = doctor ]; then
    echo "Engine and project checks passed. Run build to validate Xcode and the native SDK."
    exit 0
fi
if [ "$action" = setup ]; then
    exec /usr/bin/python3 "$project_dir/Scripts/RegisterEngineMac.py" "$engine_root"
fi
if [ "$action" = generate ]; then
    exec /bin/bash "$batch_dir/GenerateProjectFiles.sh" "-project=$project" -game -engine
fi

# Build the native editor module before opening this checkout directly.
/bin/bash "$batch_dir/Build.sh" PokemonCyberpunkEditor Mac Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
if [ "$action" = open ]; then
    /usr/bin/open -n -a "$editor" --args "$project"
fi
