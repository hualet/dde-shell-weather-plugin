# DDE Shell Weather Plugin

[![Build](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/build.yml/badge.svg)](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/build.yml)
[![Build Debian Package](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/deb.yml/badge.svg)](https://github.com/hualet/dde-shell-weather-plugin/actions/workflows/deb.yml) [![DeepWiki](https://img.shields.io/badge/DeepWiki-hualet%2Fdde--shell--weather--plugin-blue.svg?logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACwAAAAyCAYAAAAnWDnqAAAAAXNSR0IArs4c6QAAA05JREFUaEPtmUtyEzEQhtWTQyQLHNak2AB7ZnyXZMEjXMGeK/AIi+QuHrMnbChYY7MIh8g01fJoopFb0uhhEqqcbWTp06/uv1saEDv4O3n3dV60RfP947Mm9/SQc0ICFQgzfc4CYZoTPAswgSJCCUJUnAAoRHOAUOcATwbmVLWdGoH//PB8mnKqScAhsD0kYP3j/Yt5LPQe2KvcXmGvRHcDnpxfL2zOYJ1mFwrryWTz0advv1Ut4CJgf5uhDuDj5eUcAUoahrdY/56ebRWeraTjMt/00Sh3UDtjgHtQNHwcRGOC98BJEAEymycmYcWwOprTgcB6VZ5JK5TAJ+fXGLBm3FDAmn6oPPjR4rKCAoJCal2eAiQp2x0vxTPB3ALO2CRkwmDy5WohzBDwSEFKRwPbknEggCPB/imwrycgxX2NzoMCHhPkDwqYMr9tRcP5qNrMZHkVnOjRMWwLCcr8ohBVb1OMjxLwGCvjTikrsBOiA6fNyCrm8V1rP93iVPpwaE+gO0SsWmPiXB+jikdf6SizrT5qKasx5j8ABbHpFTx+vFXp9EnYQmLx02h1QTTrl6eDqxLnGjporxl3NL3agEvXdT0WmEost648sQOYAeJS9Q7bfUVoMGnjo4AZdUMQku50McDcMWcBPvr0SzbTAFDfvJqwLzgxwATnCgnp4wDl6Aa+Ax283gghmj+vj7feE2KBBRMW3FzOpLOADl0Isb5587h/U4gGvkt5v60Z1VLG8BhYjbzRwyQZemwAd6cCR5/XFWLYZRIMpX39AR0tjaGGiGzLVyhse5C9RKC6ai42ppWPKiBagOvaYk8lO7DajerabOZP46Lby5wKjw1HCRx7p9sVMOWGzb/vA1hwiWc6jm3MvQDTogQkiqIhJV0nBQBTU+3okKCFDy9WwferkHjtxib7t3xIUQtHxnIwtx4mpg26/HfwVNVDb4oI9RHmx5WGelRVlrtiw43zboCLaxv46AZeB3IlTkwouebTr1y2NjSpHz68WNFjHvupy3q8TFn3Hos2IAk4Ju5dCo8B3wP7VPr/FGaKiG+T+v+TQqIrOqMTL1VdWV1DdmcbO8KXBz6esmYWYKPwDL5b5FA1a0hwapHiom0r/cKaoqr+27/XcrS5UwSMbQAAAABJRU5ErkJggg==)](https://deepwiki.com/hualet/dde-shell-weather-plugin)

Deepin DDE Shell 的任务栏天气插件，使用 Qt6/C++/QML 构建，提供当前天气、动态图标、小时级预报和自动刷新能力。

![plugin showcase](./docs/res/showcase.png)

## 功能

- 任务栏常驻显示当前天气、温度和天气描述
- 支持动态图标，刷新后重播天气图标动画
- 支持小时级天气弹窗，查看短时温度趋势与预报
- 支持自动定位与手动地点搜索
- 中国地区手动搜索优先使用本地城市数据库，减少地名误判
- 使用 `MET Norway` 作为主天气源，`Open-Meteo` 作为运行时兜底

## 快速开始

### 构建

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . -j$(nproc)
```

### 安装

```bash
sudo cmake --install build
```

### Debian/Deepin 打包

```bash
dpkg-buildpackage -us -uc -b
```

## 文档

- [开发与技术说明](docs/development.md)
- [天气源选型说明](docs/weather-provider-candidates.md)

## 技术概览

- 后端：C++17、Qt6 Network、Qt6 DBus
- 前端：QML、Qt Quick Controls 2、DTK6
- 运行模式：DDE Shell 插件
- 数据源：MET Norway、Open-Meteo

项目主要目录：

```text
.
├── src/       C++ 后端与插件入口
├── package/   QML 界面、图标与插件元数据
├── debian/    Debian/Deepin 打包配置
└── docs/      设计、开发与补充文档
```

## 适用环境

- CMake >= 3.16
- Qt6
- DTK6
- DDE Shell 开发包
- 支持 C++17 的编译器

更完整的构建依赖、可选定位后端、打包依赖和运行时说明，见 [开发与技术说明](docs/development.md)。

## 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

GPL-3.0-or-later

## 致谢

- [MET Norway](https://api.met.no/)
- [Open-Meteo](https://open-meteo.com/)
- [DDE Shell](https://github.com/linuxdeepin/dde-shell)
