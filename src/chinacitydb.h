// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariantList>

/**
 * @brief Local database of Chinese cities for offline search and completion.
 *
 * The Open-Meteo Geocoding API has poor Chinese-language support for Chinese
 * cities.  For example, searching "北京" returns obscure villages instead of
 * the capital.  This class provides a built-in dataset covering all
 * prefecture-level cities, autonomous prefectures, municipalities, and SARs
 * so that Chinese users always get reliable completion results.
 *
 * Each entry stores the city name in Chinese, an optional English name, the
 * province it belongs to, and its geographic coordinates.  The search
 * function performs prefix matching on the Chinese name and pinyin and
 * returns results in the same QVariantMap format used by the remote API
 * handler.
 */
class ChinaCityDb
{
public:
  /**
   * @brief Search for Chinese cities matching @p query.
   *
   * Matches are performed by Chinese-character prefix, full pinyin prefix
   * (case-insensitive), and pinyin-initial prefix (e.g. "bj" matches
   * "北京").  Results are ordered so that exact or shorter matches appear
   * first.
   *
   * @param query  The user input (Chinese characters or pinyin).
   * @param limit  Maximum number of results to return.
   * @return A QVariantList where each element is a QVariantMap compatible
   *         with the format expected by LocationSettingsDialog.
   */
  static QVariantList search (const QString &query, int limit = 8);

  /**
   * @brief Returns true when @p langCode looks like a Chinese locale.
   *
   * Matches "zh", "zh_CN", "zh_TW", "zh_HK", etc.
   */
  static bool isChineseLocale (const QString &langCode);
};
