# PROJECT KNOWLEDGE BASE

**Generated:** 2026-03-10
**Commit:** e7b52f1
**Branch:** main

## OVERVIEW

DDE Shell weather plugin - Deepin Desktop Environment taskbar weather applet using Qt6/C++/QML with Open-Meteo API.

## STRUCTURE

```
./
├── CMakeLists.txt          # Build config
├── weatherapplet.h/cpp     # Main applet class (DApplet)
├── weatherprovider.h/cpp   # Weather data provider (QNetworkAccessManager)
└── package/               # QML UI
    ├── main.qml           # Main UI
    ├── WeatherIcon.qml    # Weather icon with animations
    └── metadata.json      # Plugin metadata
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add new weather field | `weatherprovider.h` + `weatherprovider.cpp` | Add to WeatherData struct, add Q_PROPERTY |
| UI changes | `package/main.qml` | Main UI layout |
| Icon animations | `package/WeatherIcon.qml` | Weather condition icons |
| Build system | `CMakeLists.txt` | Qt6/DTK6 dependencies |

## CODE MAP

| Symbol | Type | File |
|--------|------|------|
| WeatherApplet | class | weatherapplet.h |
| WeatherProvider | class | weatherprovider.h |
| WeatherData | struct | weatherprovider.h |
| DS_USE_NAMESPACE | macro | All .h files |

## CONVENTIONS (MANDATORY)

- **QML**: Never use duplicate `id:` values
- **C++**: Run `clang-format -style=gnu` after every C++ edit
- **License**: SPDX header required on every new file (`// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.`)
- **Namespace**: Use `DS_USE_NAMESPACE` in header files

## ANTI-PATTERNS

- QML duplicate IDs → runtime errors
- Missing SPDX license header → rejected
- C++ without clang-format → style violation
- Hardcoded API keys → use Open-Meteo (no key required)

## COMMANDS

```bash
# Build
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install

# Format C++
clang-format -style=gnu -i weatherprovider.cpp weatherapplet.cpp
```

## NOTES

- Uses Open-Meteo free API (no key needed)
- Auto-refresh every 30 minutes via QTimer
- WMO weather codes mapped to icons/animations
- Supports optional Qt6 Positioning for auto-location
