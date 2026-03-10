<!--
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
-->

# libgweather 免费天气源调研与候选列表

## 结论

本项目已将 `MET Norway (api.met.no)` 作为主天气源接入，并保留 `Open-Meteo` 作为运行时兜底。

选择依据按优先级看四件事：

1. 全球覆盖是否足够完整
2. 是否支持当前天气 + 未来预报
3. 是否可以免 API key 直接集成
4. 官方稳定性和长期维护预期是否足够好

## libgweather 当前涉及的免费天气源

`libgweather` 官方文档里的 `GWeatherProvider` 当前列出这些来源：

- `METAR`：航空气象观测
- `IWIN`：旧版 IWIN API
- `MET Norway`：挪威气象局 `api.met.no`
- `OpenWeatherMap`
- `NWS`：美国国家气象局

参考：

- `libgweather` provider 枚举文档：<https://gnome.pages.gitlab.gnome.org/libgweather/libgweather/gweather-GWeatherLocation.html>

## 候选服务优先级

### P1: MET Norway (已接入)

- 质量判断：最适合作为默认全球天气源
- 优点：全球覆盖、无需 API key、小时级预报完整、符号体系稳定、官方文档清晰
- 接入代价：需要设置规范 `User-Agent`
- 适配结论：非常适合桌面插件免配置接入

参考：

- API 文档：<https://api.met.no/weatherapi/locationforecast/2.0/documentation>
- 使用要求：<https://api.met.no/doc/TermsOfService>

### P2: NWS / api.weather.gov

- 质量判断：美国区域内质量很高
- 优点：官方数据、预报质量强、接口现代
- 缺点：覆盖范围基本只适合美国和属地，不适合作为全球默认源
- 适配结论：适合作为后续“美国区域专用 provider”

参考：

- 服务文档入口：<https://www.weather.gov/documentation/services-web-api>
- 接入要求（`User-Agent` 等）：<https://www.weather.gov/documentation/services-web-alerts>

### P3: OpenWeatherMap

- 质量判断：能力成熟，但不再适合本项目作为免配置默认源
- 优点：全球覆盖广，生态成熟
- 缺点：当前接入需要账号和 API key，免费层也带来配置和额度约束
- 适配结论：只有在项目愿意引入用户配置项时才值得接

参考：

- One Call 3.0 订阅说明：<https://openweathermap.org/api/one-call-3>

### P4: METAR / AviationWeather

- 质量判断：更像观测源，不像面向普通用户的完整天气服务
- 优点：免费、稳定、站点观测数据权威
- 缺点：偏机场站点，预报能力弱，普通城市天气体验不完整
- 适配结论：更适合作为观测补充，不适合作为主 provider

参考：

- AviationWeather Data API：<https://aviationweather.gov/data/api/>

### P5: IWIN (legacy)

- 质量判断：仅作为历史兼容信息保留
- 优点：无
- 缺点：`libgweather` 官方枚举已经明确标注为 `old API`
- 适配结论：不建议做新接入

参考：

- `libgweather` provider 枚举文档：<https://gnome.pages.gitlab.gnome.org/libgweather/libgweather/gweather-GWeatherLocation.html>

## 本项目的落地方案

- 主天气源：`MET Norway`
- 运行时兜底：`Open-Meteo`
- 候选扩展顺序：`NWS` > `OpenWeatherMap` > `METAR`

原因很直接：

- `MET Norway` 是 `libgweather` 免费源里最平衡的一项
- `NWS` 虽然质量高，但地域限制太强
- `OpenWeatherMap` 会把当前“免配置”插件改成“需要账号/API key”的产品形态
- `METAR` 更适合专业观测，不适合当前任务栏天气插件 UX
