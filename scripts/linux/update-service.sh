#!/usr/bin/env bash
#
# update-service.sh — Met a jour morfSensor installe en service.
#
# Recupere le code (git pull), recompile, recopie le binaire dans le dossier fixe,
# puis redemarre le service. La config locale (morfnotify.json) n'est jamais
# ecrasee. Complement de install-service.sh.
#
# Usage :
#   sudo ./scripts/linux/update-service.sh           # git pull + build + restart
#   sudo ./scripts/linux/update-service.sh --no-pull # rebuild seulement
#   sudo ./scripts/linux/update-service.sh --refresh-config # sauvegarde + remplace morfnotify.json

set -euo pipefail

SERVICE_NAME="morfnotify"
UNIT_DEST="/etc/systemd/system/$SERVICE_NAME.service"
APP_DIR="${MN_APP_DIR:-/opt/morfnotify}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RUN_USER="${SUDO_USER:-$(logname 2>/dev/null || echo root)}"
NO_PULL=0
REFRESH_CONFIG=0

for arg in "$@"; do
    case "$arg" in
        --no-pull) NO_PULL=1 ;;
        --refresh-config) REFRESH_CONFIG=1 ;;
    esac
done

if [[ "${EUID}" -ne 0 ]]; then
    echo "Ce script doit etre lance avec sudo :  sudo $0 $*" >&2
    exit 1
fi
if [[ ! -f "$UNIT_DEST" ]]; then
    echo "Service '$SERVICE_NAME' non installe. Lance d'abord :  sudo ./scripts/linux/install-service.sh" >&2
    exit 1
fi

# --- Arreter avant toute mise a jour ------------------------------------
SERVICE_WAS_ACTIVE=0
if systemctl is-active --quiet "$SERVICE_NAME"; then
    SERVICE_WAS_ACTIVE=1
fi
echo "Arret de $SERVICE_NAME avant mise a jour..."
systemctl stop "$SERVICE_NAME" 2>/dev/null || true

# --- Recuperer le code (en tant que l'utilisateur) -----------------------
if [[ "$NO_PULL" -ne 1 ]]; then
    echo "git pull (utilisateur $RUN_USER)..."
    sudo -u "$RUN_USER" bash -c "cd '$REPO_ROOT' && git pull --ff-only"
fi

# --- Recompiler (preset selon l'architecture) ----------------------------
ARCH="$(uname -m)"
if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
    PRESET="linux-arm64"; BUILD_DIR="$REPO_ROOT/build-arm64"
else
    PRESET="linux";       BUILD_DIR="$REPO_ROOT/build"
fi
echo "Compilation (preset $PRESET)..."
sudo -u "$RUN_USER" bash -lc "cd '$REPO_ROOT' && cmake --preset $PRESET && cmake --build --preset $PRESET"
BIN="$BUILD_DIR/service/morfnotify"
[[ -x "$BIN" ]] || { echo "Echec : $BIN introuvable apres compilation." >&2; exit 1; }

# --- Recopier le binaire (config preservee) ------------------------------
echo "Copie du binaire vers $APP_DIR..."
install -m 0755 "$BIN" "$APP_DIR/morfnotify"
chown "$RUN_USER:$RUN_USER" "$APP_DIR/morfnotify"

# --- Installer/preserver la configuration -------------------------------
if [[ "$REFRESH_CONFIG" -eq 1 && -f "$APP_DIR/morfnotify.json" ]]; then
    BACKUP="$APP_DIR/morfnotify.json.$(date +%Y%m%d-%H%M%S).bak"
    cp -a "$APP_DIR/morfnotify.json" "$BACKUP"
    install -m 0644 "$REPO_ROOT/config/morfnotify.example.json" "$APP_DIR/morfnotify.json"
    echo "Config remplacee : $APP_DIR/morfnotify.json (sauvegarde : $BACKUP)."
elif [[ ! -f "$APP_DIR/morfnotify.json" ]]; then
    install -m 0644 "$REPO_ROOT/config/morfnotify.example.json" "$APP_DIR/morfnotify.json"
    echo "Config initiale copiee : $APP_DIR/morfnotify.json (a adapter : destinations)."
else
    echo "Config existante conservee : $APP_DIR/morfnotify.json"
fi
chown "$RUN_USER:$RUN_USER" "$APP_DIR/morfnotify.json"

# --- Redemarrer si le service tournait ----------------------------------
if [[ "$SERVICE_WAS_ACTIVE" -eq 1 ]]; then
    systemctl start "$SERVICE_NAME"
else
    echo "Service laisse arrete (il ne tournait pas avant la mise a jour)."
fi
sleep 1
echo "Mise a jour appliquee."
systemctl --no-pager --lines=0 status "$SERVICE_NAME" || true
