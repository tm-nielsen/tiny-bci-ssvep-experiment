#!/usr/bin/env bash
#
# run_experiment.sh
#
# Brings up Bluetooth, connects to the Unicorn headset by device name,
# keeps the RFCOMM link alive in the background, and launches the
# tiny_bci_ssvep_experiment runtime. Cleans up the Bluetooth connection
# on exit (normal or interrupted).
#
# If SERIAL_PORT is set (e.g. SERIAL_PORT=/dev/ttyUSB0), the entire
# Bluetooth/RFCOMM setup is skipped and the runtime is launched directly
# against that device.
#
set -euo pipefail

# --- Configuration -----------------------------------------------------

DEVICE_NAME="${DEVICE_NAME:-UN-2023.08.05}"   # override: DEVICE_NAME=foo ./run_experiment.sh
RFCOMM_CHANNEL="${RFCOMM_CHANNEL:-1}"         # confirmed via: sdptool browse <mac>
RFCOMM_ID=0
RFCOMM_DEV="/dev/rfcomm${RFCOMM_ID}"

SERIAL_PORT="${SERIAL_PORT:-}"                # override: SERIAL_PORT=/dev/ttyUSB0 ./runtime_pizero.sh

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${PROJECT_ROOT}/bin"
RUNTIME_BIN="tiny_bci_ssvep_experiment"

RFCOMM_PID=""
MAC=""

# --- Cleanup -------------------------------------------------------------

cleanup() {
    echo "==> Cleaning up..."
    # nothing to release if we skipped Bluetooth entirely via SERIAL_PORT
    if [[ -n "${SERIAL_PORT}" ]]; then
        return
    fi
    if [[ -n "${RFCOMM_PID}" ]]; then
        sudo kill "${RFCOMM_PID}" 2>/dev/null || true
    fi
    # Belt-and-suspenders: the PID above is sudo's wrapper PID, which doesn't
    # always propagate the signal to the actual rfcomm process underneath.
    if [[ -n "${MAC}" ]]; then
        sudo pkill -f "rfcomm connect ${RFCOMM_ID} ${MAC}" 2>/dev/null || true
    fi
    sudo rfcomm release "${RFCOMM_ID}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# --- 0. Skip Bluetooth entirely if a serial port was provided -----------

if [[ -n "${SERIAL_PORT}" ]]; then
    echo "==> SERIAL_PORT=${SERIAL_PORT} provided, skipping Bluetooth setup."

    if [[ ! -e "${SERIAL_PORT}" ]]; then
        echo "ERROR: ${SERIAL_PORT} does not exist." >&2
        echo "Check 'ls /dev/ttyUSB*' or 'ls /dev/ttyACM*' for the actual device node." >&2
        exit 1
    fi

    echo "==> Launching runtime from ${BIN_DIR}..."
    cd "${BIN_DIR}"
    ./"${RUNTIME_BIN}"
    RUNTIME_EXIT_CODE=$?

    echo "==> Runtime exited with code ${RUNTIME_EXIT_CODE}."
    exit "${RUNTIME_EXIT_CODE}"
fi

# --- 1. Ensure Bluetooth is on -------------------------------------------

echo "==> Ensuring Bluetooth is on..."
sudo rfkill unblock bluetooth

if ! systemctl is-active --quiet bluetooth; then
    echo "    bluetooth.service not active, starting it..."
    sudo systemctl start bluetooth
    sleep 2
fi

sudo hciconfig hci0 up 2>/dev/null || true
bluetoothctl power on >/dev/null

# --- 2. Look up MAC address by device name -------------------------------

echo "==> Looking up MAC address for device name '${DEVICE_NAME}'..."
MAC=$(bluetoothctl devices | grep -i "${DEVICE_NAME}" | awk '{print $2}' | head -n1)

if [[ -z "${MAC}" ]]; then
    echo "ERROR: no paired device matching '${DEVICE_NAME}' found." >&2
    echo "Run 'bluetoothctl paired-devices' to check what's actually paired." >&2
    exit 1
fi
echo "    found ${DEVICE_NAME} -> ${MAC}"

# --- 3. Connect via raw rfcomm (not bluetoothctl connect) ---------------

echo "==> Releasing any stale RFCOMM binding..."
sudo rfcomm release "${RFCOMM_ID}" 2>/dev/null || true

echo "==> Connecting to ${MAC} on channel ${RFCOMM_CHANNEL}..."
sudo rfcomm connect "${RFCOMM_ID}" "${MAC}" "${RFCOMM_CHANNEL}" &
RFCOMM_PID=$!

echo "==> Waiting for ${RFCOMM_DEV} to appear..."
for _ in $(seq 1 20); do
    if [[ -e "${RFCOMM_DEV}" ]]; then
        break
    fi
    if ! kill -0 "${RFCOMM_PID}" 2>/dev/null; then
        echo "ERROR: rfcomm connect exited early — connection failed." >&2
        exit 1
    fi
    sleep 0.5
done

if [[ ! -e "${RFCOMM_DEV}" ]]; then
    echo "ERROR: ${RFCOMM_DEV} never appeared within timeout." >&2
    exit 1
fi
echo "    ${RFCOMM_DEV} is up."

# --- 4. Launch the runtime -------------------------------------------------

echo "==> Launching runtime from ${BIN_DIR}..."
cd "${BIN_DIR}"
./"${RUNTIME_BIN}"
RUNTIME_EXIT_CODE=$?

echo "==> Runtime exited with code ${RUNTIME_EXIT_CODE}."
exit "${RUNTIME_EXIT_CODE}"