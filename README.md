# DDE Shell Weather Plugin

一个为 DDE Shell (Deepin Desktop Environment) 开发的天气任务栏插件。

![plugin showcase](/home/hualet/projects/hualet/dde-shell-weather-plugin/docs/res/showcase.png)

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
