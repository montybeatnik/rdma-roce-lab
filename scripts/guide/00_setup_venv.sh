#!/usr/bin/env bash
set -euo pipefail

VENV_DIR="${VENV_DIR:-.venv}"
REQ_FILE="${REQ_FILE:-requirements/py.txt}"

if ! command -v python3 >/dev/null 2>&1; then
  echo "[ERR] python3 not found."
  exit 1
fi

if [ "${VENV_DIR}" = ".venv" ]; then
  fs_type="$(stat -f -c %T . 2>/dev/null || true)"
  case "${fs_type}" in
    9p|fuse|fuseblk|fuse.sshfs|virtiofs)
      echo "[INFO] Detected mounted filesystem (${fs_type}); using /home/ubuntu/.venv"
      VENV_DIR="/home/ubuntu/.venv"
      ;;
  esac
fi

py_ver="$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')"
if ! python3 -m venv --help >/dev/null 2>&1; then
  if command -v apt-get >/dev/null 2>&1; then
    if command -v sudo >/dev/null 2>&1; then
      echo "[INFO] Installing python${py_ver}-venv via apt"
      sudo apt-get update
      sudo apt-get install -y "python${py_ver}-venv" || sudo apt-get install -y python3-venv
    else
      echo "[ERR] python3-venv not available and sudo is missing."
      echo "      Install manually: apt install -y python${py_ver}-venv"
      exit 1
    fi
  else
    echo "[ERR] python3-venv not available and apt-get is missing."
    echo "      Install python${py_ver}-venv via your package manager."
    exit 1
  fi
fi

if ! python3 -c "import pyverbs" >/dev/null 2>&1; then
  if command -v apt-get >/dev/null 2>&1 && command -v sudo >/dev/null 2>&1; then
    echo "[INFO] Installing python3-pyverbs via apt"
    sudo apt-get update
    sudo apt-get install -y python3-pyverbs
  else
    echo "[WARN] pyverbs not found. Install with: sudo apt install -y python3-pyverbs"
  fi
fi

if [ -d "${VENV_DIR}" ] && [ ! -x "${VENV_DIR}/bin/python3" ]; then
  echo "[WARN] ${VENV_DIR} exists but looks incomplete."
  echo "       Remove it and re-run after installing python${py_ver}-venv."
  echo "       Example: rm -rf ${VENV_DIR}"
  exit 1
fi

recreate_venv() {
  echo "[INFO] Recreating venv in ${VENV_DIR}"
  rm -rf "${VENV_DIR}" || return 1
  python3 -m venv --system-site-packages "${VENV_DIR}"
}

if [ ! -d "${VENV_DIR}" ]; then
  echo "[INFO] Creating venv in ${VENV_DIR}"
  python3 -m venv --system-site-packages "${VENV_DIR}"
fi

echo "[INFO] Activating venv"
# shellcheck disable=SC1090
source "${VENV_DIR}/bin/activate"

SYSTEM_PYTHON="${SYSTEM_PYTHON:-/usr/bin/python3}"
if ! python -c "import pyverbs" >/dev/null 2>&1; then
  cfg="${VENV_DIR}/pyvenv.cfg"
  system_has_pyverbs=0
  if [ -x "${SYSTEM_PYTHON}" ] && "${SYSTEM_PYTHON}" -c "import pyverbs" >/dev/null 2>&1; then
    system_has_pyverbs=1
  fi
  if [ "${system_has_pyverbs}" -eq 1 ]; then
    echo "[WARN] System has pyverbs, but venv cannot import it."
    echo "[INFO] Recreating venv with system site packages enabled."
    if recreate_venv; then
      # shellcheck disable=SC1090
      source "${VENV_DIR}/bin/activate"
    else
      echo "[ERR] Could not recreate venv in ${VENV_DIR}."
      echo "      If this repo is mounted from the host, try:"
      echo "      VENV_DIR=/home/ubuntu/.venv scripts/guide/00_setup_venv.sh"
      exit 1
    fi
  elif [ -f "${cfg}" ] && grep -q "^include-system-site-packages = false" "${cfg}"; then
    echo "[WARN] Venv does not include system site packages; pyverbs won't be visible."
    if recreate_venv; then
      # shellcheck disable=SC1090
      source "${VENV_DIR}/bin/activate"
    else
      echo "[ERR] Could not recreate venv in ${VENV_DIR}."
      echo "      If this repo is mounted from the host, try:"
      echo "      VENV_DIR=/home/ubuntu/.venv scripts/guide/00_setup_venv.sh"
      exit 1
    fi
  fi
fi

echo "[INFO] Upgrading pip tooling"
python -m pip install --upgrade pip setuptools wheel

if [ -f "${REQ_FILE}" ]; then
  echo "[INFO] Installing Python dependencies from ${REQ_FILE}"
  python -m pip install -r "${REQ_FILE}"
else
  echo "[WARN] ${REQ_FILE} not found; skipping dependency install."
fi

echo "[INFO] Venv ready. Activate with: source ${VENV_DIR}/bin/activate"
