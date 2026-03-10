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
- **MET Norway API** - 主天气数据源
- **Open-Meteo API** - 兜底天气数据源

## 构建要求

- CMake >= 3.16
- Qt6 (Core, Network, Quick, QuickControls2)
- DTK6 (Core, Widget)
- DDE Shell 开发包
- C++17 编译器

### 可选依赖

- Qt6 Positioning - 用于自动定位功能
- libqt6positioning6-plugins - Qt Positioning 运行时插件，自动定位依赖它加载 GeoClue 后端

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

生成的 `dde-shell-weather-plugin` Debian 包会额外安装：

- `/etc/geoclue/conf.d/90-dde-shell.conf`，用于放行 `dde-shell` 的 GeoClue 定位授权
- `libqt6positioning6-plugins` 运行时依赖，确保 Qt Positioning 可以加载 GeoClue 插件

## 自动定位配置

如果通过 `cmake --install` 或安装 `.deb` 部署插件，项目会自动安装 GeoClue 配置文件
`/etc/geoclue/conf.d/90-dde-shell.conf`。

如果你只是手动复制了插件文件，或者需要在现有系统上补配置，可以手动创建：

```ini
[org.deepin.ds.desktop]
allowed=true
system=true
users=
```

示例命令：

```bash
sudo install -d /etc/geoclue/conf.d
sudo tee /etc/geoclue/conf.d/90-dde-shell.conf >/dev/null <<'EOF'
[org.deepin.ds.desktop]
allowed=true
system=true
users=
EOF

sudo systemctl restart geoclue.service
systemctl --user restart dde-shell@DDE.service
```

这项配置只解决 GeoClue 对 `dde-shell` 的授权问题。若仍出现 `UpdateTimeoutError`，说明系统定位源本身没有在超时时间内返回结果。

## GitHub Actions

- `.github/workflows/build.yml`：在 `main/master` 分支和 PR 上做常规编译，并上传安装树产物。
- `.github/workflows/deb.yml`：支持手动触发、推送 `v*` tag，或发布 GitHub Release 时构建 `deb`，并将产物附加到对应 Release。
- 两个工作流都使用 `docker.io/hualet/deepin:25-builder` 作为构建容器，避免 GitHub Runner 缺少 Deepin 依赖。

## 项目结构

```
dde-shell-weather-plugin/
├── CMakeLists.txt                   # 构建配置
├── config/geoclue/90-dde-shell.conf # GeoClue 授权配置
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
