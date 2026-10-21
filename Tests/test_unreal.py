"""Regression checks with isolated engine fixtures; no installed Unreal required."""

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ASSOCIATION = json.loads((ROOT / "PokemonCyberpunk.uproject").read_text())["EngineAssociation"]
MAJOR, MINOR = map(int, ASSOCIATION.split("."))
spec = importlib.util.spec_from_file_location("registration", ROOT / "Scripts/RegisterEngineMac.py")
registration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(registration)


def make_engine(root, patch=4, minor=MINOR):
    version = root / "Engine/Build/Build.version"
    version.parent.mkdir(parents=True, exist_ok=True)
    version.write_text(json.dumps({"MajorVersion": MAJOR, "MinorVersion": minor, "PatchVersion": patch}))
    (root / "Engine/Binaries/Mac/UnrealEditor.app").mkdir(parents=True, exist_ok=True)
    return root


class RegistrationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="PokemonCyberpunk tests ")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.engine = make_engine(self.base / "Engine with spaces")
        self.manifest = self.base / "Launcher/LauncherInstalled.dat"

    def write_manifest(self, data):
        self.manifest.parent.mkdir(parents=True, exist_ok=True)
        self.manifest.write_text(json.dumps(data))

    def test_preserves_metadata_backup_and_is_idempotent(self):
        original = {"ExtraMetadata": {"keep": True}, "InstallationList": [
            {"AppName": "UE_other", "InstallLocation": "/other", "ArtifactId": "keep"},
            {"AppName": f"UE_{ASSOCIATION}", "InstallLocation": str(self.engine), "ItemId": "keep"},
        ]}
        self.write_manifest(original)
        before = self.manifest.read_bytes()
        self.assertTrue(registration.register(self.engine, self.manifest))
        after = self.manifest.read_bytes()
        result = json.loads(after)
        self.assertEqual(result["ExtraMetadata"], original["ExtraMetadata"])
        self.assertEqual(result["InstallationList"][0]["ItemId"], "keep")
        self.assertEqual(result["InstallationList"][1], original["InstallationList"][0])
        backup = self.manifest.with_name(self.manifest.name + ".pokemoncyberpunk-backup")
        self.assertEqual(backup.read_bytes(), before)
        self.assertFalse(registration.register(self.engine, self.manifest))
        self.assertEqual(self.manifest.read_bytes(), after)
        self.assertEqual(backup.read_bytes(), before)

    def test_creates_registration_without_existing_manifest(self):
        self.assertTrue(registration.register(self.engine, self.manifest))
        self.assertEqual(json.loads(self.manifest.read_text())["InstallationList"], [
            {"AppName": f"UE_{ASSOCIATION}", "InstallLocation": str(self.engine.resolve())}
        ])

    def test_rejects_another_engine_series(self):
        make_engine(self.engine, minor=MINOR + 1)
        with self.assertRaises(ValueError):
            registration.register(self.engine, self.manifest)
        self.assertFalse(self.manifest.exists())

    def test_rejects_conflicting_installation_without_changes(self):
        other = make_engine(self.base / "Other engine")
        self.write_manifest({"InstallationList": [
            {"AppName": f"UE_{ASSOCIATION}", "InstallLocation": str(other)}
        ]})
        before = self.manifest.read_bytes()
        with self.assertRaisesRegex(ValueError, "already points"):
            registration.register(self.engine, self.manifest)
        self.assertEqual(self.manifest.read_bytes(), before)
        self.assertFalse(self.manifest.with_name(self.manifest.name + ".pokemoncyberpunk-backup").exists())

    def test_allows_relocation_of_missing_installation(self):
        self.write_manifest({"InstallationList": [
            {"AppName": f"UE_{ASSOCIATION}", "InstallLocation": str(self.base / "No longer installed")}
        ]})
        self.assertTrue(registration.register(self.engine, self.manifest))

    def test_rejects_malformed_manifest_without_changes(self):
        for data in ([], {}, {"InstallationList": {}}, {"InstallationList": [42]},
                     {"InstallationList": [{"AppName": f"UE_{ASSOCIATION}"}]}):
            with self.subTest(data=data):
                self.write_manifest(data)
                before = self.manifest.read_bytes()
                with self.assertRaises(ValueError):
                    registration.register(self.engine, self.manifest)
                self.assertEqual(self.manifest.read_bytes(), before)


@unittest.skipUnless(sys.platform in ("win32", "darwin"), "Native Windows or macOS script required")
class HostScriptTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="PokemonCyberpunk scripts ")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.engine = make_engine(self.base / "Engine with spaces")
        self.log = self.base / "arguments.txt"
        self.env = dict(os.environ, UE_ENGINE_ROOT=str(self.engine),
                        UE_TEST_LOG=str(self.log), UE_TEST_EXIT="0")
        self.project_before = (ROOT / "PokemonCyberpunk.uproject").read_bytes()
        self.addCleanup(self.assert_project_unchanged)
        if sys.platform == "win32":
            batch = self.engine / "Engine/Build/BatchFiles/Build.bat"
            batch.parent.mkdir(parents=True)
            batch.write_bytes(b'@echo off\r\n> "%UE_TEST_LOG%" echo %*\r\nexit /b %UE_TEST_EXIT%\r\n')
            editor = self.engine / "Engine/Binaries/Win64/UnrealEditor.exe"
            editor.parent.mkdir(parents=True)
            editor.touch()
            self.command = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                            "-File", str(ROOT / "Scripts/Unreal.ps1")]
            self.build_tool = batch
        else:
            batch_dir = self.engine / "Engine/Build/BatchFiles/Mac"
            batch_dir.mkdir(parents=True)
            for name in ("Build.sh", "GenerateProjectFiles.sh"):
                (batch_dir / name).write_text('printf "%s\\n" "$@" > "$UE_TEST_LOG"\nexit "$UE_TEST_EXIT"\n')
            self.command = ["/bin/bash", str(ROOT / "Scripts/Unreal.sh")]
            self.build_tool = batch_dir / "Build.sh"

    def assert_project_unchanged(self):
        self.assertEqual((ROOT / "PokemonCyberpunk.uproject").read_bytes(), self.project_before)

    def run_script(self, *args):
        # Deliberately run outside the checkout to exercise project path resolution.
        return subprocess.run(self.command + list(args), cwd=self.base, env=self.env,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    def assert_success(self, result):
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_doctor_accepts_patch_releases_without_building(self):
        for patch in (0, 4, 9):
            make_engine(self.engine, patch=patch)
            result = self.run_script("doctor")
            self.assert_success(result)
            self.assertIn(f"{ASSOCIATION}.{patch}", result.stdout)
            self.assertIn(str(ROOT / "PokemonCyberpunk.uproject"), result.stdout)
            self.assertFalse(self.log.exists())

    def test_discovers_launcher_engine_outside_default_location(self):
        self.env.pop("UE_ENGINE_ROOT", None)
        if sys.platform == "win32":
            self.env["ProgramData"] = str(self.base / "ProgramData")
            manifest = Path(self.env["ProgramData"]) / "Epic/UnrealEngineLauncher/LauncherInstalled.dat"
        else:
            self.env["HOME"] = str(self.base / "User home")
            manifest = Path(self.env["HOME"]) / "Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat"
        wrong_engine = make_engine(self.base / "Wrong engine", minor=MINOR + 1)
        manifest.parent.mkdir(parents=True)
        manifest.write_text(json.dumps({"InstallationList": [
            {"AppName": "Unrelated", "InstallLocation": str(wrong_engine)},
            {"AppName": f"UE_{ASSOCIATION}", "InstallLocation": str(wrong_engine)},
            {"AppName": f"UE_{ASSOCIATION}", "InstallLocation": str(self.engine)},
        ]}))
        result = self.run_script("doctor")
        self.assert_success(result)
        self.assertIn(str(self.engine), result.stdout)
        self.assertFalse(self.log.exists())

    def test_build_uses_existing_project_and_native_platform(self):
        self.assert_success(self.run_script("build"))
        arguments = self.log.read_text()
        for expected in ("PokemonCyberpunkEditor", "Development", str(ROOT / "PokemonCyberpunk.uproject"),
                         "-WaitMutex", "-NoHotReloadFromIDE",
                         "Win64" if sys.platform == "win32" else "Mac"):
            self.assertIn(expected, arguments)

    def test_generate_uses_existing_project(self):
        self.assert_success(self.run_script("generate"))
        arguments = self.log.read_text()
        self.assertIn(str(ROOT / "PokemonCyberpunk.uproject"), arguments)
        self.assertIn("-game", arguments)
        self.assertNotIn("PokemonCyberpunkEditor", arguments)

    def test_failed_build_stops_open_and_propagates_exit_code(self):
        self.env["UE_TEST_EXIT"] = "7"
        result = self.run_script("open")
        self.assertEqual(result.returncode, 7, result.stdout)

    def test_wrong_explicit_engine_fails_without_fallback(self):
        make_engine(self.engine, minor=MINOR + 1)
        result = self.run_script("build")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(f"UE {ASSOCIATION}.x is required", result.stdout)
        self.assertFalse(self.log.exists())

    def test_missing_engine_fails_without_fallback(self):
        self.env["UE_ENGINE_ROOT"] = str(self.base / "Missing engine")
        self.assertNotEqual(self.run_script("build").returncode, 0)
        self.assertFalse(self.log.exists())

    def test_incomplete_engine_fails_before_build(self):
        self.build_tool.unlink()
        result = self.run_script("build")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Incomplete", result.stdout)
        self.assertFalse(self.log.exists())

    def test_invalid_actions_and_extra_arguments_fail(self):
        for arguments in (("invalid",), ("build", "extra")):
            with self.subTest(arguments=arguments):
                self.assertNotEqual(self.run_script(*arguments).returncode, 0)
                self.assertFalse(self.log.exists())


if __name__ == "__main__":
    unittest.main()
