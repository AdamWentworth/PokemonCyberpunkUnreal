"""Register a managed macOS engine under the project's portable engine ID."""

import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile


def register(engine_root, manifest):
    project = Path(__file__).resolve().parents[1] / "PokemonCyberpunk.uproject"
    association = json.loads(project.read_text(encoding="utf-8"))["EngineAssociation"]
    if not isinstance(association, str) or not re.fullmatch(r"\d+\.\d+", association):
        raise ValueError("PokemonCyberpunk.uproject must use a portable major.minor EngineAssociation.")

    engine_root = engine_root.resolve(strict=True)
    version = json.loads((engine_root / "Engine/Build/Build.version").read_text(encoding="utf-8"))
    installed = f"{version['MajorVersion']}.{version['MinorVersion']}"
    if installed != association:
        raise ValueError(f"Only Unreal Engine {association}.x can be registered as {association}.")
    if not (engine_root / "Engine/Binaries/Mac/UnrealEditor.app").is_dir():
        raise ValueError("The selected installation has no macOS UnrealEditor.app.")

    # DesktopPlatform reads this file before consulting the Mac's GUID registry.
    # A plain major.minor alias in Install.ini is insufficient: its Mac reader
    # parses those keys as GUIDs. Keep the project's portable Launcher ID instead.
    data = json.loads(manifest.read_text(encoding="utf-8")) if manifest.exists() else {"InstallationList": []}
    if not isinstance(data, dict):
        raise ValueError("Invalid registration file; it was left unchanged.")
    installations = data.get("InstallationList")
    if not isinstance(installations, list) or any(not isinstance(item, dict) for item in installations):
        raise ValueError("Invalid InstallationList; the registration file was left unchanged.")
    app_name = f"UE_{association}"
    matching = [item for item in installations if item.get("AppName") == app_name]
    for item in matching:
        location = item.get("InstallLocation")
        if not isinstance(location, str) or not location:
            raise ValueError("Invalid InstallLocation; the registration file was left unchanged.")
        registered_root = Path(location).resolve()
        if registered_root != engine_root and (registered_root / "Engine/Build/Build.version").is_file():
            raise ValueError(
                f"{app_name} already points to {registered_root}. "
                "Select that engine with UE_ENGINE_ROOT or resolve its registration before setup."
            )
    registration = dict(matching[0]) if matching else {"AppName": app_name}
    registration["InstallLocation"] = str(engine_root)
    # Prefer the shared ID when the same directory also has another local ID.
    updated = [registration] + [item for item in installations if item.get("AppName") != app_name]
    if installations == updated:
        return False

    manifest.parent.mkdir(parents=True, exist_ok=True)
    backup = manifest.with_name(manifest.name + ".pokemoncyberpunk-backup")
    if manifest.exists() and not backup.exists():
        shutil.copy2(manifest, backup)
    data["InstallationList"] = updated
    temporary_path = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=manifest.parent, delete=False) as stream:
            temporary_path = Path(stream.name)
            if manifest.exists() and hasattr(os, "fchmod"):
                os.fchmod(stream.fileno(), manifest.stat().st_mode & 0o777)
            json.dump(data, stream, indent=2)
            stream.write("\n")
        os.replace(temporary_path, manifest)
    finally:
        if temporary_path is not None and temporary_path.exists():
            temporary_path.unlink()
    return True


def main():
    if sys.platform != "darwin" or len(sys.argv) != 2:
        raise ValueError("Run bash Scripts/Unreal.sh setup on macOS.")
    manifest = Path.home() / "Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat"
    changed = register(Path(sys.argv[1]), manifest)
    print("Registered the engine under the project's shared ID." if changed else "Engine registration already matches.")
    print("Registration: " + str(manifest))
    print("Restart all Unreal Editor instances, then open PokemonCyberpunk normally. No conversion is needed.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError) as error:
        sys.exit(str(error))
