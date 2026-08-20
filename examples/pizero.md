# Pi Zero W Deployment Guide — tBCI SSVEP Experiment

Headless deployment notes for building and running the tBCI SSVEP Experiment
backend (no raylib frontend) on a Raspberry Pi Zero W, connected to a Unicorn EEG
headset over Bluetooth or a Neuropawn headset via GPIO port.

---

## 1. Bluetooth: Pairing a Device with the Pi Zero W

### Basic pairing flow

```bash
sudo bluetoothctl
power on
agent on
default-agent
scan on
```

Wait until your device appears with its MAC address (e.g. `AA:BB:CC:DD:EE:FF`), then:

```bash
scan off
pair AA:BB:CC:DD:EE:FF
trust AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF
```

Verify and exit:

```bash
info AA:BB:CC:DD:EE:FF   # should show "Connected: yes"
exit
```

> Put the target device into pairing/discoverable mode on its end *before* running
> `scan on` — most devices only stay discoverable for a minute or two.

### Fixing `org.bluez.error.notready`

This means the `hci0` adapter isn't initialized yet — common on the Zero W since its
Bluetooth chip attaches over the same UART as the serial console.

```bash
sudo rfkill unblock bluetooth
hciconfig -a                      # check if hci0 exists / is UP
sudo hciconfig hci0 up            # try bringing it up manually

systemctl status hciuart          # UART attach service — must succeed first
sudo systemctl restart hciuart    # if failed/inactive

sudo systemctl restart bluetooth  # restart bluetoothd cleanly
```

Then retry `power on` / `agent on` / `scan on` inside `bluetoothctl`.

If it still fails, check:
- `/boot/firmware/config.txt` (or `/boot/config.txt`) has **no** `dtoverlay=disable-bt` line
- `enable_uart=1` is set
- `raspi-config` → Interface Options → Serial Port: **login shell over serial = No**,
  **serial port hardware = Yes**
- `dmesg | grep -i bluetooth` / `journalctl -u hciuart -b` for the underlying error

---

## 2. Finding a Device's MAC Address

### From the Pi (bluetoothctl)

```bash
bluetoothctl devices           # everything ever discovered
bluetoothctl paired-devices    # only devices that completed pairing
```

If the list is too long to scroll back through:

```bash
bluetoothctl devices | less          # scrollable, press / to search, q to quit
bluetoothctl devices | grep -i "name"  # filter directly
```

### From a MacBook

**System Information app:**
Hold `Option` → Apple menu → **System Information** → **Bluetooth** in the sidebar →
find the device under "Devices" → its **Address** field is the MAC.

**Terminal:**

```bash
system_profiler SPBluetoothDataType | grep -A 5 -i "device name"
# or with no filter, then search:
system_profiler SPBluetoothDataType | less
```

Works even for devices with no readable/intelligible name — the address is listed
under the device entry regardless.

---

## 3. Viewing Paired Devices

**Pi:** `bluetoothctl paired-devices`

**Mac:** System Settings → Bluetooth → "My Devices", or:

```bash
system_profiler SPBluetoothDataType | grep -B 5 "Paired: Yes"
```

---

## 4. Transferring Files to the Pi

### From a USB key (Pi Zero W has no full-size USB-A port — use a micro-USB OTG adapter)

```bash
lsblk                       # or: dmesg | tail -20, right after plugging in
sudo mkdir -p /mnt/usb
sudo mount /dev/sda1 /mnt/usb   # adjust device name from lsblk

cp -r /mnt/usb/FolderName ~/
sudo umount /mnt/usb
```

Notes:
- If the key doesn't show up at all, try a **powered** USB hub — the Zero W's port
  may not supply enough current.
- NTFS/exFAT keys need `sudo apt install ntfs-3g` or `exfat-fuse exfat-utils` first.
- No auto-mount on Raspberry Pi OS Lite — manual mount is expected.

### Via scp from the Mac

```bash
scp ~/Projects/tiny-bci-ssvep-experiment/CMakeLists.txt \
    bci@192.168.8.231:~/tiny-bci-ssvep-experiment/CMakeLists.txt
```

Adjust local and remote paths to match your actual layout. Remote directory must
already exist.

---

## 5. Building the Project

### Install dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake git \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    bluez-tools
```

(Header packages are only strictly required if `BUILD_FRONTEND=ON` — see §5.4 — but
harmless to have either way.)

### Populate submodules

```bash
cd ~/tiny-bci-ssvep-experiment
git submodule update --init --recursive
```

`thirdparty/tiny_bci` is referenced via a plain `add_subdirectory`, not
`FetchContent`, so it must exist on disk before configuring.

### Add swap space (Optional)

512MB RAM is tight for compiling LSL (a C++ library) on a single-core chip.

```bash
free -h                                  # check current swap
sudo nano /etc/dphys-swapfile            # set CONF_SWAPSIZE=1024
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

### CMakeLists.txt changes (headless / no frontend build)

Add a `BUILD_FRONTEND` option (default `OFF`) so raylib and its GL/X11 dependencies
are skipped entirely on the Pi build, while still buildable with the frontend on a
Mac/desktop later:

```cmake
cmake_minimum_required(VERSION 3.26)
project(tiny_bci_ssvep_experiment)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(SUBMODULE_PATH ${CMAKE_SOURCE_DIR}/thirdparty)

option(BUILD_FRONTEND "Build with raylib presentation frontend" OFF)

if (BUILD_FRONTEND)
  set(RAYLIB_VERSION 6.0)
  find_package(raylib ${RAYLIB_VERSION} QUIET)
  if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      raylib
      DOWNLOAD_EXTRACT_TIMESTAMP OFF
      URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED)
      set(FETCHCONTENT_QUIET NO)
      FetchContent_MakeAvailable(raylib)
    endif()
  endif()
endif()

set(SOURCES
    src/runtime_main.c
    src/trial_conductor.c
    src/microsecond_timer.c
    src/storage.c
    src/pipeline.c
    src/inference_logger.c

    src/data/trigger_source.c
    src/data/synthetic_eeg_source.c
    src/data/serial_unix.c
    src/data/serial_windows.c
    src/data/serial_utils.c
    src/data/neuropawn_eeg_source.c
    src/data/unicorn_eeg_source.c
    thirdparty/tiny_bci/producer/tbci_trigger_generator.c
    thirdparty/tiny_bci/producer/unicorn_producer.c
)

if (BUILD_FRONTEND)
  list(APPEND SOURCES src/presentation.c)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE producer)

if (BUILD_FRONTEND)
  target_link_libraries(${PROJECT_NAME} raylib)
endif()

add_subdirectory(${SUBMODULE_PATH}/tiny_bci)
target_link_libraries(${PROJECT_NAME} tiny_bci)

option(INCLUDE_LSL "Build with LSL support" ON)
if (${INCLUDE_LSL})
  include(LSL.cmake)
endif()
```
Also confirm `src/data/serial_windows.c` is fully wrapped in `#ifdef _WIN32` /
`#endif` — otherwise it won't compile on Linux and should be dropped from
`SOURCES` for this target.

### Configure and build

**Do not** pass `-DINCLUDE_LSL=OFF` — `runtime_main.c` depends directly on the LSL
inlet/outlet functions.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_FRONTEND=OFF
cmake --build . -- -j1
```

Single core on the Zero W means `-j1` is realistic regardless. The LSL build in
particular can take a long time (potentially an hour+) on this hardware — if
impractical, consider cross-compiling on a faster machine and copying the built
`liblsl` over instead.

---

## 6. Connecting the Unicorn EEG Headset over Bluetooth (RFCOMM)

The headset connects via Bluetooth SPP. On macOS this shows up automatically as
`/dev/cu.<name>`; on Linux you bind it manually to an RFCOMM device node.

```bash
# 1. Pair + trust (see §1)
bluetoothctl pair AA:BB:CC:DD:EE:FF
bluetoothctl trust AA:BB:CC:DD:EE:FF

# 2. Find the Serial Port channel number
sudo sdptool browse AA:BB:CC:DD:EE:FF
# look for a "Serial Port" entry, note its Channel (often 1)

# 3. Bind the RFCOMM device node
sudo rfcomm bind /dev/rfcomm0 AA:BB:CC:DD:EE:FF 1

# 4. Test the link before running the full program
sudo cat /dev/rfcomm0                 # or: screen /dev/rfcomm0 115200
```

Update the port in `runtime_main.c`:

```c
#define PORT "/dev/rfcomm0"   // was "/dev/cu.UN-20230805" (macOS-only path)
```

(Ideally make this a runtime argument/env var instead of a hardcoded macro, since
the rfcomm device number can shift.)

**Persistence across reboots:** `rfcomm bind` doesn't survive a reboot by itself.
Add a small systemd service or startup script that runs the bind command before
your program starts:

```bash
sudo rfcomm bind /dev/rfcomm0 AA:BB:CC:DD:EE:FF 1
```

Note: RFCOMM isn't a physical UART, so `termios` baud rate settings in
`serial_unix.c` are effectively ignored by the Bluetooth stack — this is expected
and not a bug if you see baud-rate-looking config being silently unused.

---

## 7. Running the Compiled Binary

```bash
# ensure the RFCOMM device is bound (§6) first
cd ~/tiny-bci-ssvep-experiment
./bin/tiny_bci_ssvep_experiment
```

---

## 8. Troubleshooting Reference

### `could not find git for clone of liblsl-populate`

Almost always a **stale CMake cache** — an earlier `cmake ..` ran before `git` was
installed/on PATH, and the empty `GIT_EXECUTABLE` value got cached.

```bash
which git && git --version    # confirm git is present
rm -rf build                  # nuke the build dir, don't just reconfigure in place
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_FRONTEND=OFF
```

Watch for an actual `Cloning into 'liblsl'...` message during configure. If it
still fails, check basic connectivity: `ping github.com` / `curl -I https://github.com`.

### `org.bluez.error.notready`

See §1 above.

### Build runs out of memory / gets silently killed

Add swap — see §5.3.