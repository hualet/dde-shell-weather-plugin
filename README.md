# DDE Shell Weather Plugin

[![Build](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/build.yml/badge.svg)](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/build.yml)[![Build Debian Package](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/deb.yml/badge.svg)](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/deb.yml)

一个为 DDE Shell (Deepin Desktop Environment) 开发的天气任务栏插件。

![plugin showcase](./docs/res/showcase.png)

## 功能特性

- 🌤️ **实时天气显示** - 显示当前位置的天气状况
- 🎨 **美观界面** - 动态渐变背景，根据天气状态变化
- 📍 **自动定位** - 支持自动获取当前位置（可选）
- 🔄 **自动刷新** - 每 30 分钟自动更新天气数据
- 🌡️ **详细信息** - 显示温度、湿度、风速等信息

## 技术栈

- **C++** - 后端逻辑
- **Qt6** - UI 框架
- **QML** - 界面设计
- **DTK6** - Deepin Tool Kit
- **MET Norway API** - 主天气数据源
- **Open-Meteo API** - 兜底天气数据源

## 构建要求

- CMake >= 3.16
- Qt6 (Core, Network, Quick, QuickControls2)
- DTK6 (Core, Widget)
- DDE Shell 开发包
- C++17 编译器

### 可选依赖

- Qt6 Positioning - 可选，用于显式启用更精确的系统定位
- libqt6positioning6-plugins - 可选，启用 Qt Positioning/GeoClue 时需要
- geoclue-2-demo - 可选，启用 GeoClue agent 预启动时需要

## 构建和安装

```bash
# 1. 创建构建目录
mkdir -p build && cd build

# 2. 配置项目
cmake .. -DCMAKE_INSTALL_PREFIX=/usr

# 3. 编译
make -j$(nproc)

# 4. 安装（需要 root 权限）
sudo make install
```

## Debian/Deepin 打包

```bash
sudo apt install -y \
  build-essential \
  debhelper \
  dpkg-dev \
  cmake \
  pkg-config \
  qt6-base-dev \
  qt6-base-dev-tools \
  qt6-declarative-dev \
  qt6-positioning-dev \
  qt6-tools-dev \
  qt6-tools-dev-tools \
  libdtk6core-dev \
  libdtk6gui-dev \
  libdtk6widget-dev \
  libdde-shell-dev

dpkg-buildpackage -us -uc -b
```

构建完成后，`.deb`、`.changes`、`.buildinfo` 会出现在项目上一级目录。

生成的 `dde-shell-weather-plugin` Debian 包会额外依赖：

- `libqt6positioning6-plugins`，确保 Qt Positioning 可以加载 GeoClue 后端
- `geoclue-2-demo`，提供 `/usr/libexec/geoclue-2.0/demos/agent`

## 自动定位说明

插件默认跳过 GeoClue / Qt Positioning，直接使用基于 IP 的粗定位，避免首次定位时依赖系统定位服务。
如果 IP 定位失败，才会继续回退到默认城市。

如果你确实需要显式启用 Qt Positioning / GeoClue，可以在启动前设置环境变量：

```bash
export DS_WEATHER_LOCATION_BACKEND=qt
```

启用后，插件会在调用 `QGeoPositionInfoSource::requestUpdate()` 之前尝试启动系统自带的 GeoClue agent：

- `/usr/libexec/geoclue-2.0/demos/agent`

当位置获取成功或失败时，插件会立即退出这个 agent 进程，不再依赖 `/etc/geoclue/conf.d/*.conf`
里的静态放行配置。若 Qt Positioning / GeoClue 不可用，插件仍会自动回退到基于 IP 的粗定位。

## 项目结构

```
dde-shell-weather-plugin/
├── CMakeLists.txt                   # 构建配置
├── src/                             # C++ 源码
│   ├── weatherapplet.h/cpp          # 插件主类
│   └── weatherprovider.h/cpp        # 天气数据提供者
├── package/                         # QML 界面资源
│   ├── metadata.json                # 插件元数据
│   ├── main.qml                     # 主界面
│   ├── WeatherIcon.qml              # 天气图标组件
│   └── icons/                       # 天气图标资源
├── docs/                            # 设计和候选服务文档
└── debian/                          # Debian/Deepin 打包配置
```

## API 使用

本插件优先使用 [MET Norway / api.met.no](https://api.met.no/weatherapi/locationforecast/2.0/documentation) 获取天气数据，无需 API 密钥，只需要提供规范的 `User-Agent`。

当 `MET Norway` 不可用时，插件会自动回退到 [Open-Meteo](https://open-meteo.com/)。

基于 `libgweather` 免费天气源的候选服务优先级与选择理由，见 [docs/weather-provider-candidates.md](docs/weather-provider-candidates.md)。

## 许可证

GPL-3.0-or-later

## 贡献

欢迎提交 Issue 和 Pull Request！

## 致谢

- [MET Norway](https://api.met.no/) - 免费的全球天气 API
- [Open-Meteo](https://open-meteo.com/) - 免费的天气 API 兜底源
- [DDE Shell](https://github.com/linuxdeepin/dde-shell) - Deepin 桌面环境
