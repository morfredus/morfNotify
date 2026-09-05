#!/usr/bin/env python3
"""Install, update or remove this service — on Linux, Windows or a Raspberry Pi.

    ./service.py install       build if needed, install, start
    ./service.py update        rebuild, replace the binary, restart
    ./service.py uninstall     deregister, keeping the application directory
    ./service.py status        what the system says about it

Replaces install-service.sh, install-service.ps1, update-service.sh,
update-service.ps1, uninstall-service.sh and uninstall-service.ps1. The four
steps are the same everywhere and live in one place; only the service manager
differs, and that is the one thing each platform backend describes.

What this service is -- its name, its directory, its configurations -- is
declared in service.json, next to this file.

The orchestration is vendored under third_party/morf/morfdeploy, so a clone of
this project alone remains installable without fetching anything else.
Resynchronise it with scripts/sync-morf.sh; `morf doctor` reports drift.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def _install_alert_bridge(repo: Path) -> None:
    """Installe le pont d'alerte systemd -> morfNotify (morf-alert@.service + le
    script morf-alert). morfNotify possede et installe ses PROPRES artefacts : aucun
    autre projet ni morfdeploy n'est touche (chaque projet reste maitre de son unite).
    Les services declarent seulement `OnFailure=morf-alert@%n.service` dans la leur.
    Linux + root uniquement ; best-effort, ne casse jamais l'install en cas de souci.
    """
    import platform
    import shutil
    import subprocess

    if platform.system() != "Linux":
        return
    if hasattr(os, "geteuid") and os.geteuid() != 0:
        print("[morfNotify] pont d'alerte non installe : root requis "
              "(relancer install/update via sudo).", file=sys.stderr)
        return

    unit_src   = repo / "scripts" / "linux" / "morf-alert@.service"
    script_src = repo / "scripts" / "morf-alert"
    if not unit_src.is_file() or not script_src.is_file():
        print("[morfNotify] pont d'alerte : artefacts introuvables, ignore.", file=sys.stderr)
        return

    # Le script vit hors /opt (comme le helper privilegie), dans un dossier stable
    # que l'unite generique reference en dur.
    script_dir = Path("/usr/lib/morfsystem/morfnotify")
    script_dst = script_dir / "morf-alert"
    unit_dst   = Path("/etc/systemd/system/morf-alert@.service")

    script_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(script_src, script_dst)
    script_dst.chmod(0o755)
    shutil.copy2(unit_src, unit_dst)
    unit_dst.chmod(0o644)
    subprocess.run(["systemctl", "daemon-reload"], check=False)
    print(f"  alert bridge installed: {unit_dst} + {script_dst}")
sys.path.insert(0, str(HERE / "third_party" / "morf"))

try:
    from morfdeploy.cli import main
except ImportError as exc:  # pragma: no cover - only when the copy is missing
    print(
        f"Cannot load the vendored deployment core: {exc}\n"
        f"Expected under {HERE / 'third_party' / 'morf' / 'morfdeploy'}.\n"
        "Restore it with:  ./scripts/sync-morf.sh",
        file=sys.stderr,
    )
    raise SystemExit(2) from exc

if __name__ == "__main__":
    # The repository root is this file's directory, so the command works from
    # anywhere -- including from sudo, whose working directory is not
    # necessarily the one the person was standing in.
    args = sys.argv[1:]
    rc = main([*args, "--repo", str(HERE)])
    # Etape propre a morfNotify : apres une install/update reussie, poser le pont
    # d'alerte systemd -> morfNotify. C'est morfNotify (le hub de notifications) qui
    # fournit et installe cette petite couche generique ; les services s'y raccordent
    # par une simple ligne OnFailure dans leur propre unite.
    if rc == 0 and args and args[0] in ("install", "update"):
        try:
            _install_alert_bridge(HERE)
        except Exception as exc:  # jamais bloquer l'install pour le pont
            print(f"[morfNotify] pont d'alerte systemd non installe: {exc}", file=sys.stderr)
    sys.exit(rc)
