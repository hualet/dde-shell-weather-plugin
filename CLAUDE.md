# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DDE Shell weather plugin - a taskbar weather applet for the Deepin Desktop Environment.
Built with C++17 / Qt6 / QML / DTK6. Uses MET Norway as the primary weather API and Open-Meteo as fallback.
Current version: defined in `CMakeLists.txt` `project(VERSION ...)`, mirrored in `package/metadata.json` and `debian/changelog`.

## Build & Run

```bash
# Standard build
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install

# Build with tests enabled (requires QML runtime)
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON -DDS_WEATHER_ENABLE_QML_TESTS=ON
make -j$(nproc)
ctest --test-dir . --output-on-failure

# Debian package
dpkg-buildpackage -us -uc -b

# Format C++ (mandatory before committing)
clang-format -style=gnu -i src/*.cpp src/*.h
```

## Project Structure

```
src/                              # C++ source
  weatherapplet.h/cpp             # DApplet subclass - plugin entry point, settings persistence
  weatherprovider.h/cpp           # Core: weather fetching, location, parsing (~2600 lines)
  animatedsvgitem.h/cpp           # QQuickPaintedItem for animated SVG icons
  chinacitydb.h/cpp               # Auto-generated local Chinese city database (do NOT edit .cpp)
  locationsettingsdialog.h/cpp    # DTK DDialog for auto/manual location settings
package/                          # QML UI + plugin metadata
  metadata.json                   # DDE Shell plugin descriptor (id: org.deepin.ds.weather)
  main.qml                        # Main dock applet UI with hourly forecast popup
  WeatherIcon.qml                 # Weather icon wrapper with animation support
  icons/                          # ~55 animated SVG (SMIL) weather icons
tests/
  animatedsvgitem_test.cpp        # Qt Test: 4 test cases for AnimatedSvgItem
scripts/
  generate_china_cities.py        # Regenerates src/chinacitydb.cpp from GeoNames data
translations/                     # 23 locale .ts files
debian/                           # Debian packaging metadata
```

## Architecture

### Data Flow
1. `WeatherApplet::init()` creates `WeatherProvider`, restores saved location, calls `initialize()`
2. Location chain: Qt Positioning + GeoClue agent -> IP geolocation (ip-api.com) -> default Beijing
3. Weather chain: MET Norway (api.met.no) -> Open-Meteo (fallback)
4. Reverse geocoding: BigDataCloud API for city name resolution
5. City search: Open-Meteo Geocoding API + local ChinaCityDb (for Chinese queries)
6. `WeatherProvider` emits `weatherChanged()` -> QML binds to `Applet.weather.*` properties

### Key Classes
- **WeatherApplet** (DApplet): plugin lifecycle, QSettings persistence, QML type registration
- **WeatherProvider** (QObject): all weather/location logic, ~25 Q_PROPERTYs exposed to QML
- **WeatherData** (struct): city, temperature, humidity, windSpeed, weatherCode, iconName, etc.
- **AnimatedSvgItem** (QQuickPaintedItem): renders animated SVGs, plays once then stops
- **ChinaCityDb** (static): offline Chinese city search with pinyin/initials matching
- **LocationSettingsDialog** (DDialog): auto/manual location mode with city autocomplete

### Refresh Behavior
- Auto-refresh every 30 minutes via QTimer
- 15-second retry on network failure
- Deduplication window: 60 seconds
- Refreshes on system resume (D-Bus PrepareForSleep signal)
- Manual refresh via right-click menu

## Coding Conventions

- **C++ style**: GNU style via `clang-format -style=gnu` - run on all C++ files before committing
- **SPDX headers**: required on every new file (`// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.`)
- **License**: GPL-3.0-or-later for C++/build files; LGPL-3.0-or-later for QML files
- **QML**: never use duplicate `id:` values (causes runtime errors)
- **Namespace**: use `DS_USE_NAMESPACE` in headers that expose DDE types
- **No hardcoded API keys**: all weather/location APIs are key-free
- **Qt Positioning guard**: all positioning code is wrapped in `#ifdef QT_POSITIONING_LIB`

## Version Bumping

All three files must be updated together:
1. `CMakeLists.txt` - `project(... VERSION x.y.z ...)`
2. `package/metadata.json` - `Plugin.Version`
3. `debian/changelog` - new top entry with version `x.y.z-1`

Verify: `dpkg-parsechangelog -S Version`

## Dependencies

**Required**: Qt6 (Core, DBus, LinguistTools, Network, Quick, QuickControls2, Svg, Widgets), Dtk6 (Core, Widget), DDEShell
**Optional**: Qt6 Positioning/Location (enables GeoClue-based auto-location; without it, falls back to IP geolocation)
**Runtime**: dde-shell, libqt6positioning6-plugins, geoclue-2-demo

## External APIs (no keys required)

| API | Purpose | URL pattern |
|-----|---------|-------------|
| MET Norway | Primary weather data | `api.met.no/weatherapi/locationforecast/2.0/compact` |
| Open-Meteo | Fallback weather data | `api.open-meteo.com/v1/forecast` |
| ip-api.com | IP-based geolocation | `ip-api.com/json/` |
| BigDataCloud | Reverse geocoding (city name) | `api.bigdatacloud.net/data/reverse-geocode-client` |
| Open-Meteo Geocoding | City search/autocomplete | `geocoding-api.open-meteo.com/v1/search` |

## CI/CD

- **build.yml**: push/PR to main - builds in `deepin:25-builder` container, uploads install tree artifact
- **deb.yml**: version tags (`v*`) / releases - builds .deb, publishes as GitHub release asset

## Tests

Tests are opt-in (disabled by default) because they need a QML runtime:
```bash
cmake .. -DBUILD_TESTING=ON -DDS_WEATHER_ENABLE_QML_TESTS=ON
make -j$(nproc)
QT_QPA_PLATFORM=offscreen QSG_RHI_BACKEND=software ctest --output-on-failure
```

## Code Generation

`src/chinacitydb.cpp` is auto-generated - do NOT edit manually. To regenerate:
```bash
uv venv .venv
uv pip install --python .venv pypinyin opencc-python-reimplemented
.venv/bin/python3 scripts/generate_china_cities.py
```
