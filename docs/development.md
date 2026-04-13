# 开发与技术说明

本文档承接 README 中不适合首页展开的构建、运行时和打包细节。

## 技术栈

- C++17
- Qt6 Core / DBus / Network / Quick / QuickControls2 / Svg / Widgets
- QML
- DTK6
- DDE Shell Plugin API

## 构建要求

基础依赖：

- CMake >= 3.16
- Qt6
- DTK6
- DDE Shell 开发包
- 支持 C++17 的编译器

常规构建：

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . -j$(nproc)
```

安装：

```bash
sudo cmake --install build
```

## Debian/Deepin 打包

示例依赖：

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
```

打包命令：

```bash
dpkg-buildpackage -us -uc -b
```

构建产物会出现在项目上一级目录。

## 运行时定位策略

插件默认优先使用基于 IP 的粗定位，避免首次运行时强依赖系统定位服务。
如果 IP 定位失败，插件会回退到默认城市。

如果需要显式启用 Qt Positioning / GeoClue，可在启动前设置：

```bash
export DS_WEATHER_LOCATION_BACKEND=qt
```

Qt Positioning 或 GeoClue 不可用时，插件仍会自动回退到基于 IP 的定位流程。

## 天气数据源

- 主数据源：`MET Norway`
- 兜底数据源：`Open-Meteo`

设计背景与候选方案见 [weather-provider-candidates.md](weather-provider-candidates.md)。

## 中国地区地点搜索

对于中文地区手动搜索，项目优先使用本地 `ChinaCityDb`，避免通用地理编码服务在中文地名上的误判。
如果你需要更新这部分数据，不要手改生成结果，按项目约定更新生成脚本并重新生成数据库。

## 相关入口

- 项目首页：[`README.md`](../README.md)
- DeepWiki：<https://deepwiki.com/hualet/dde-shell-weather-plugin>
