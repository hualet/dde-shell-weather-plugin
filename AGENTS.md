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
├── debian/                   # Debian packaging metadata
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
| Update China region database | `scripts/generate_china_cities.py` + `src/chinacitydb.cpp` + `tests/chinacitydb_test.cpp` | Edit the generator, never hand-edit the generated database; verify with `chinacitydb_tests` |
| Version bump / release | `CMakeLists.txt` + `package/metadata.json` + `debian/changelog` | Keep upstream and Debian package versions in sync |

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
- **ChinaCityDb**: Never edit `src/chinacitydb.cpp` manually; update `scripts/generate_china_cities.py` and regenerate
- **China districts/counties**: Keep district/county dedupe keyed by `name + prefecture + province` so homonymous entries like `长安区` and `市中区` are preserved
- **Version updates**: Always update `CMakeLists.txt`, `package/metadata.json`, and `debian/changelog` together

## ANTI-PATTERNS

- QML duplicate IDs -> runtime errors
- Missing SPDX license header -> rejected
- C++ without clang-format -> style violation
- Hardcoded API keys -> use Open-Meteo (no key required)
- Hand-editing `src/chinacitydb.cpp` -> next regeneration will overwrite it
- Name-only dedupe for China districts/counties -> drops valid homonymous districts from search results

## COMMANDS

```bash
# Build
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install

# Format C++
clang-format -style=gnu -i src/weatherprovider.cpp src/weatherapplet.cpp

# Check Debian package version
dpkg-parsechangelog -S Version

# Regenerate the China region database
uv venv .venv
uv pip install --python .venv pypinyin opencc-python-reimplemented
.venv/bin/python3 scripts/generate_china_cities.py

# Verify China city/district search coverage
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=ON
cmake --build . --target chinacitydb_tests -j$(nproc)
ctest -R chinacitydb_tests --output-on-failure
```

## VERSION BUMP CHECKLIST

When bumping the release version:

1. Update `project(... VERSION x.y.z ...)` in `CMakeLists.txt`
2. Update `Plugin.Version` in `package/metadata.json`
3. Add a new top entry in `debian/changelog` using Debian package version `x.y.z-1`
4. Verify `dpkg-parsechangelog -S Version` matches the expected Debian version
5. Run a build after the version bump to catch packaging or metadata regressions

## NOTES

- Uses MET Norway as primary weather API, Open-Meteo as fallback
- Auto-refresh every 30 minutes via QTimer
- WMO weather codes mapped to icons/animations
- Chinese manual-location queries prefer local `ChinaCityDb`; Open-Meteo geocoding is unreliable for Chinese place names
- Missing Chinese districts/counties are usually a generator coverage issue, not a QML completer bug
- `scripts/CN.txt` is a downloaded GeoNames cache for regeneration and usually should stay local unless explicitly vendoring it
- Supports optional Qt6 Positioning for auto-location
