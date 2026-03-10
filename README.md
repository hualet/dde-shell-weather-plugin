# DDE Shell Weather Plugin

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
- **Open-Meteo API** - 天气数据源

## 构建要求

- CMake >= 3.16
- Qt6 (Core, Network, Quick, QuickControls2)
- DTK6 (Core, Widget)
- DDE Shell 开发包
- C++17 编译器

### 可选依赖

- Qt6 Positioning - 用于自动定位功能

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

## GitHub Actions

- `.github/workflows/build.yml`：在 `main/master` 分支和 PR 上做常规编译，并上传安装树产物。
- `.github/workflows/deb.yml`：支持手动触发，或在推送 `v*` tag 时构建 `deb` 并附加到 GitHub Release。
- 两个工作流都使用 `docker.io/hualet/deepin:25-builder` 作为构建容器，避免 GitHub Runner 缺少 Deepin 依赖。

## 项目结构

```
dde-shell-weather-plugin/
├── CMakeLists.txt          # 构建配置
├── weatherapplet.h/cpp     # 插件主类
├── weatherprovider.h/cpp   # 天气数据提供者
└── package/                # QML 界面资源
    ├── metadata.json       # 插件元数据
    ├── main.qml           # 主界面
    ├── WeatherIcon.qml    # 天气图标组件
    ├── ParticleWeather.qml # 粒子动画组件
    └── qmldir             # QML 模块定义
```

## API 使用

本插件使用 [Open-Meteo](https://open-meteo.com/) 免费 API 获取天气数据，无需 API 密钥。

## 许可证

GPL-3.0-or-later

## 贡献

欢迎提交 Issue 和 Pull Request！

## 致谢

- [Open-Meteo](https://open-meteo.com/) - 免费的天气 API
- [DDE Shell](https://github.com/linuxdeepin/dde-shell) - Deepin 桌面环境
