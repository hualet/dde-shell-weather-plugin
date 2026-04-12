#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Generate src/chinacitydb.cpp from GeoNames data.
#
# Usage:
#   uv venv .venv
#   uv pip install --python .venv pypinyin opencc-python-reimplemented
#   .venv/bin/python3 scripts/generate_china_cities.py
#
# The script downloads CN.zip from geonames.org (if not already present),
# extracts major Chinese administrative divisions and administrative seats
# (PPLC, PPLA, PPLA2, PPLA3, PPLG, ADM3), generates Simplified Chinese names,
# pinyin, and three-level admin hierarchy (county/district → prefecture city
# → province), then writes src/chinacitydb.cpp.

import io
import pathlib
import re
import sys
import urllib.request
import zipfile

try:
    from pypinyin import lazy_pinyin, Style
    import opencc
except ImportError:
    print("Missing dependencies.  Run:")
    print("  uv pip install --python .venv pypinyin opencc-python-reimplemented")
    sys.exit(1)

SCRIPT_DIR = pathlib.Path(__file__).parent
PROJECT_DIR = SCRIPT_DIR.parent
OUTPUT_FILE = PROJECT_DIR / "src" / "chinacitydb.cpp"
CACHE_FILE = SCRIPT_DIR / "CN.txt"
GEONAMES_URL = "https://download.geonames.org/export/dump/CN.zip"

_converter = opencc.OpenCC("t2s")  # Traditional → Simplified

# Japanese shinjitai / kanji variants that appear in GeoNames alternate names
# but are not proper Simplified Chinese characters for city names.
_JP_TO_CN = str.maketrans({
    "仏": "佛", "恵": "惠", "掲": "揭", "塩": "盐", "鶏": "鸡",
    "竜": "龙", "徳": "德", "頭": "头", "図": "图", "権": "权",
    "済": "济", "処": "处", "様": "样", "実": "实", "経": "经",
    "続": "续", "広": "广", "発": "发", "変": "变", "覚": "觉",
    "応": "应", "読": "读", "豊": "丰", "録": "录", "転": "转",
    "営": "营", "浜": "滨", "楡": "榆", "渓": "溪",
    "総": "总", "収": "收", "検": "检", "証": "证", "険": "险",
    "辺": "边", "価": "价", "沢": "泽",
})


def normalize(s: str) -> str:
    return _converter.convert(s.translate(_JP_TO_CN))


def is_pure_chinese(s: str) -> bool:
    """All characters must be CJK ideographs (no katakana, latin, spaces)."""
    return bool(s) and len(s) >= 2 and all("\u4e00" <= c <= "\u9fff" for c in s)


def get_chinese_name(alt_names_str: str) -> str | None:
    if not alt_names_str:
        return None
    candidates = []
    for a in alt_names_str.split(","):
        converted = normalize(re.sub(r"\s+", "", a.strip()))
        if is_pure_chinese(converted):
            candidates.append(converted)
    if not candidates:
        return None
    for suffix in ["市", "区", "县", "旗", "省", "自治区", "自治州", "特别行政区"]:
        for n in candidates:
            if n.endswith(suffix):
                return n
    return sorted(candidates, key=len)[0]


def download_geonames():
    if CACHE_FILE.exists():
        print(f"Using cached {CACHE_FILE}")
        return
    print(f"Downloading {GEONAMES_URL} ...")
    with urllib.request.urlopen(GEONAMES_URL) as resp:
        data = resp.read()
    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        zf.extract("CN.txt", SCRIPT_DIR)
    print("Downloaded and extracted CN.txt")


def load_admin_names():
    province_names: dict[str, str] = {}
    prefecture_names: dict[tuple[str, str], str] = {}
    with open(CACHE_FILE, encoding="utf-8") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 15 or parts[8] != "CN":
                continue
            fc = parts[7]
            if fc in ("ADM1", "ADM1H"):
                name = get_chinese_name(parts[3])
                if name and parts[10]:
                    province_names[parts[10]] = name
            elif fc in ("ADM2", "ADM2H"):
                name = get_chinese_name(parts[3])
                if name and parts[10] and parts[11]:
                    prefecture_names[(parts[10], parts[11])] = name
    return province_names, prefecture_names


def load_cities(province_names, prefecture_names):
    cities = []
    seen: set[tuple[str, str, str]] = set()
    allowed_place_codes = {"PPLC", "PPLA", "PPLA2", "PPLA3", "PPLG"}
    allowed_admin_codes = {"ADM3", "ADM3H"}
    with open(CACHE_FILE, encoding="utf-8") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 15 or parts[8] != "CN":
                continue
            feature_class = parts[6]
            fc = parts[7]
            if feature_class == "P":
                if fc not in allowed_place_codes:
                    continue
            elif feature_class == "A":
                if fc not in allowed_admin_codes:
                    continue
            else:
                continue
            name = get_chinese_name(parts[3])
            lat = float(parts[4]) if parts[4] else 0.0
            lon = float(parts[5]) if parts[5] else 0.0
            a1, a2 = parts[10], parts[11]
            population = int(parts[14]) if parts[14] else 0
            province = province_names.get(a1, "")
            prefecture = prefecture_names.get((a1, a2), "")
            if prefecture == name:
                prefecture = ""
            dedupe_key = (name, prefecture, province)
            if not name or dedupe_key in seen:
                continue
            seen.add(dedupe_key)
            pinyin_full = "".join(lazy_pinyin(name))
            pinyin_initials = "".join(lazy_pinyin(name, style=Style.FIRST_LETTER))
            cities.append({
                "name": name,
                "prefecture": prefecture,
                "province": province,
                "latitude": lat,
                "longitude": lon,
                "pinyin": pinyin_full,
                "initials": pinyin_initials,
                "population": population,
                "feature_code": fc,
            })
    order = {"PPLC": 0, "PPLA": 1, "PPLG": 2, "PPLA2": 3, "PPLA3": 4, "ADM3": 5, "ADM3H": 6}
    cities.sort(key=lambda c: (order.get(c["feature_code"], 9), -c["population"]))
    return cities


def cpp_str(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def generate_cpp(cities: list) -> str:
    lines = [
        "// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.",
        "// SPDX-License-Identifier: GPL-3.0-or-later",
        "",
        "// clang-format off",
        "// This file is auto-generated by scripts/generate_china_cities.py.",
        "// Do not edit manually.",
        "",
        '#include "chinacitydb.h"',
        "",
        "#include <QString>",
        "#include <QVariantList>",
        "#include <QVariantMap>",
        "",
        "namespace {",
        "",
        "struct CityEntry",
        "{",
        "  const char *name;       // Chinese city name (UTF-8, Simplified)",
        "  const char *prefecture; // Prefecture-level city (地级市), may be empty",
        "  const char *province;   // Province (省/自治区), may be empty",
        "  const char *pinyin;     // Full pinyin without tones, lower-case",
        "  const char *initials;   // Pinyin initials, lower-case",
        "  float latitude;",
        "  float longitude;",
        "};",
        "",
        "// NOLINTBEGIN(modernize-avoid-c-arrays)",
        "constexpr CityEntry kCities[] = {",
    ]
    for c in cities:
        lines.append(
            f'  {{ u8"{cpp_str(c["name"])}", u8"{cpp_str(c["prefecture"])}", '
            f'u8"{cpp_str(c["province"])}", '
            f'"{cpp_str(c["pinyin"])}", "{cpp_str(c["initials"])}", '
            f'{c["latitude"]:.4f}f, {c["longitude"]:.4f}f }},'
        )
    lines += [
        "};",
        "// NOLINTEND(modernize-avoid-c-arrays)",
        "",
        f"constexpr int kCityCount = {len(cities)};",
        "",
        "// Strip common administrative suffixes so \"北京\" matches \"北京市\".",
        "QString",
        "stripAdminSuffix (const QString &name)",
        "{",
        "  static const QLatin1String kSuffixes[] = {",
        "    QLatin1String (\"市\"), QLatin1String (\"区\"), QLatin1String (\"县\"),",
        "    QLatin1String (\"旗\"), QLatin1String (\"省\"), QLatin1String (\"自治区\"),",
        "    QLatin1String (\"自治州\"), QLatin1String (\"特别行政区\"),",
        "  };",
        "  for (const auto &suffix : kSuffixes)",
        "    {",
        "      if (name.endsWith (suffix))",
        "        return name.chopped (suffix.size ());",
        "    }",
        "  return name;",
        "}",
        "",
        "} // namespace",
        "",
        "QVariantList",
        "ChinaCityDb::search (const QString &query, int limit)",
        "{",
        "  if (query.isEmpty () || limit <= 0)",
        "    return {};",
        "",
        "  const QString q = query.trimmed ();",
        "  const QString qLower = q.toLower ();",
        "  const QString qStripped = stripAdminSuffix (q);",
        "",
        "  QVariantList results;",
        "  results.reserve (limit);",
        "",
        "  for (int i = 0; i < kCityCount && results.size () < limit; ++i)",
        "    {",
        "      const CityEntry &e = kCities[i];",
        "      const QString name = QString::fromUtf8 (e.name);",
        "      const QString pinyin = QLatin1String (e.pinyin);",
        "      const QString initials = QLatin1String (e.initials);",
        "",
        "      const bool chineseMatch =",
        "          name.startsWith (q, Qt::CaseInsensitive)",
        "          || name.startsWith (qStripped, Qt::CaseInsensitive)",
        "          || stripAdminSuffix (name).startsWith (q, Qt::CaseInsensitive)",
        "          || stripAdminSuffix (name).startsWith (qStripped, Qt::CaseInsensitive);",
        "",
        "      const bool pinyinMatch =",
        "          pinyin.startsWith (qLower)",
        "          || initials.startsWith (qLower);",
        "",
        "      if (!chineseMatch && !pinyinMatch)",
        "        continue;",
        "",
        "      const QString prefecture = QString::fromUtf8 (e.prefecture);",
        "      const QString province   = QString::fromUtf8 (e.province);",
        "      const QString shortName  = name;",
        "",
        "      // Build 1-, 2-, or 3-level display name:",
        "      //   county:     辉县, 新乡市, 河南省",
        "      //   prefecture: 武汉市, 湖北省",
        "      //   provincial: 北京市",
        "      QString displayName = name;",
        "      if (!prefecture.isEmpty () && prefecture != name)",
        "        displayName += QStringLiteral (\", \") + prefecture;",
        "      if (!province.isEmpty () && province != name && province != prefecture)",
        "        displayName += QStringLiteral (\", \") + province;",
        "",
        "      results.append (QVariantMap {",
        "          { QStringLiteral (\"name\"),        name },",
        "          { QStringLiteral (\"shortName\"),   shortName },",
        "          { QStringLiteral (\"displayName\"), displayName },",
        "          { QStringLiteral (\"latitude\"),    static_cast<double> (e.latitude) },",
        "          { QStringLiteral (\"longitude\"),   static_cast<double> (e.longitude) },",
        "          { QStringLiteral (\"country\"),     QStringLiteral (\"中国\") },",
        "          { QStringLiteral (\"admin1\"),      province },",
        "      });",
        "    }",
        "",
        "  return results;",
        "}",
        "",
        "bool",
        "ChinaCityDb::isChineseLocale (const QString &langCode)",
        "{",
        "  return langCode.startsWith (QLatin1String (\"zh\"),",
        "                              Qt::CaseInsensitive);",
        "}",
        "",
        "// clang-format on",
    ]
    return "\n".join(lines) + "\n"


def main():
    download_geonames()
    print("Loading admin names ...")
    province_names, prefecture_names = load_admin_names()
    print("Loading cities ...")
    cities = load_cities(province_names, prefecture_names)
    print(f"Found {len(cities)} cities")
    cpp = generate_cpp(cities)
    OUTPUT_FILE.write_text(cpp, encoding="utf-8")
    print(f"Written to {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
