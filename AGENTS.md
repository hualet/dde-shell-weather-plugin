# PROJECT KNOWLEDGE BASE

**Generated:** 2026-03-10
**Commit:** 935b558
**Branch:** main

## OVERVIEW

DDE Shell weather plugin - Deepin Desktop Environment taskbar weather applet using Qt6/C++/QML with MET Norway as the primary backend and Open-Meteo as fallback.

## STRUCTURE

```
./
├── CMakeLists.txt            # Build config
├── src/                      # C++ source files
│   ├── weatherapplet.h/cpp   # Main applet class (DApplet)
│   └── weatherprovider.h/cpp # Weather data provider (QNetworkAccessManager)
└── package/                  # QML UI
    ├── main.qml              # Main UI
    ├── WeatherIcon.qml       # Weather icon with animations
    └── metadata.json         # Plugin metadata
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add new weather field | `src/weatherprovider.h` + `src/weatherprovider.cpp` | Add to WeatherData struct, add Q_PROPERTY |
| UI changes | `package/main.qml` | Main UI layout |
| Icon animations | `package/WeatherIcon.qml` | Weather condition icons |
| Build system | `CMakeLists.txt` | Qt6/DTK6 dependencies |

## CODE MAP

| Symbol | Type | File |
|--------|------|------|
| WeatherApplet | class | src/weatherapplet.h |
| WeatherProvider | class | src/weatherprovider.h |
| WeatherData | struct | src/weatherprovider.h |
| DS_USE_NAMESPACE | macro | src/weatherapplet.h |

## CONVENTIONS (MANDATORY)

- **QML**: Never use duplicate `id:` values
- **C++**: Run `clang-format -style=gnu` after every C++ edit
- **License**: SPDX header required on every new file (`// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.`)
- **Namespace**: Use `DS_USE_NAMESPACE` in headers that expose DDE types

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
clang-format -style=gnu -i src/weatherprovider.cpp src/weatherapplet.cpp
```

## NOTES

- Uses MET Norway as primary weather API, Open-Meteo as fallback
- Auto-refresh every 30 minutes via QTimer
- WMO weather codes mapped to icons/animations
- Supports optional Qt6 Positioning for auto-location
