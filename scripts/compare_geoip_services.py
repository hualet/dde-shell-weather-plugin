#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

import csv
import io
import json
import math
import statistics
import sys
import time
import urllib.parse
import urllib.request
from xml.etree import ElementTree


DATA_URL = (
    "https://raw.githubusercontent.com/ip2location-com/sample-databases/"
    "main/IP2Location/DB11/ip2location-db11-sample.ipv4.csv"
)
IP_API_TEMPLATE = (
    "http://ip-api.com/json/{ip}?fields="
    "status,message,country,regionName,city,lat,lon,query"
)
KDE_TEMPLATE = "https://geoip.kde.org/v1/ubiquity?ip={ip}"


def load_rows():
    text = urllib.request.urlopen(DATA_URL, timeout=30).read().decode(
        "utf-8", "replace"
    )
    rows = list(csv.reader(io.StringIO(text)))
    return [
        row
        for row in rows
        if len(row) >= 10
        and row[2] != "-"
        and row[5] != "-"
        and row[6] != "0.000000"
        and row[7] != "0.000000"
    ]


def int_to_ip(value):
    return ".".join(str((value >> shift) & 255) for shift in (24, 16, 8, 0))


def midpoint_ip(start, end):
    return int_to_ip((start + end) // 2)


def normalize(value):
    return "".join(ch.lower() for ch in value if ch.isalnum())


def text_match(expected, actual):
    expected_value = normalize(expected)
    actual_value = normalize(actual)
    return bool(expected_value and actual_value) and (
        expected_value in actual_value or actual_value in expected_value
    )


def haversine(lat1, lon1, lat2, lon2):
    radius_km = 6371.0
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    delta_phi = math.radians(lat2 - lat1)
    delta_lambda = math.radians(lon2 - lon1)
    value = math.sin(delta_phi / 2) ** 2 + math.cos(phi1) * math.cos(phi2) * math.sin(
        delta_lambda / 2
    ) ** 2
    return 2 * radius_km * math.atan2(math.sqrt(value), math.sqrt(1 - value))


def choose_samples(rows, count):
    step = max(1, len(rows) // (count * 3))
    selected = []
    countries = set()

    for index in range(0, len(rows), step):
        row = rows[index]
        country = row[3]
        if country in countries:
            continue
        selected.append(row)
        countries.add(country)
        if len(selected) >= count:
            return selected

    for row in rows:
        if row in selected:
            continue
        selected.append(row)
        if len(selected) >= count:
            break

    return selected


def fetch_json(url):
    with urllib.request.urlopen(url, timeout=15) as response:
        return json.loads(response.read().decode("utf-8", "replace"))


def fetch_xml(url):
    with urllib.request.urlopen(url, timeout=15) as response:
        return response.read().decode("utf-8", "replace")


def parse_kde(xml_text):
    root = ElementTree.fromstring(xml_text)
    return {
        "country": root.findtext("CountryName", default=""),
        "region": root.findtext("RegionName", default=""),
        "city": root.findtext("City", default=""),
        "lat": float(root.findtext("Latitude", default="0") or 0),
        "lon": float(root.findtext("Longitude", default="0") or 0),
    }


def summarize(name, rows):
    distances = [row["distance_km"] for row in rows]
    print(
        f"[{name}] samples={len(rows)} "
        f"country={sum(row['country_match'] for row in rows)}/{len(rows)} "
        f"region={sum(row['region_match'] for row in rows)}/{len(rows)} "
        f"city={sum(row['city_match'] for row in rows)}/{len(rows)} "
        f"avg_km={statistics.mean(distances):.1f} "
        f"median_km={statistics.median(distances):.1f} "
        f"max_km={max(distances):.1f}"
    )


def main():
    sample_count = int(sys.argv[1]) if len(sys.argv) > 1 else 8
    rows = choose_samples(load_rows(), sample_count)
    results = {"ip-api": [], "kde": []}

    for row in rows:
        start = int(row[0])
        end = int(row[1])
        sample_ip = midpoint_ip(start, end)
        reference = {
            "country": row[3],
            "region": row[4],
            "city": row[5],
            "lat": float(row[6]),
            "lon": float(row[7]),
        }

        ip_api = fetch_json(IP_API_TEMPLATE.format(ip=sample_ip))
        kde = parse_kde(fetch_xml(KDE_TEMPLATE.format(ip=urllib.parse.quote(sample_ip))))

        services = {
            "ip-api": {
                "country": ip_api.get("country", ""),
                "region": ip_api.get("regionName", ""),
                "city": ip_api.get("city", ""),
                "lat": float(ip_api.get("lat", 0) or 0),
                "lon": float(ip_api.get("lon", 0) or 0),
            },
            "kde": kde,
        }

        for name, data in services.items():
            distance_km = haversine(
                reference["lat"], reference["lon"], data["lat"], data["lon"]
            )
            results[name].append(
                {
                    "ip": sample_ip,
                    "reference": reference,
                    "actual": data,
                    "country_match": text_match(reference["country"], data["country"]),
                    "region_match": text_match(reference["region"], data["region"]),
                    "city_match": text_match(reference["city"], data["city"]),
                    "distance_km": distance_km,
                }
            )

        time.sleep(1.0)

    summarize("ip-api", results["ip-api"])
    summarize("kde", results["kde"])

    print("\nDetailed mismatches:")
    for name in ("ip-api", "kde"):
        for row in results[name]:
            if row["country_match"] and row["city_match"]:
                continue
            ref = row["reference"]
            actual = row["actual"]
            print(
                f"{name} ip={row['ip']} "
                f"ref={ref['country']}/{ref['region']}/{ref['city']} "
                f"actual={actual['country']}/{actual['region']}/{actual['city']} "
                f"dist_km={row['distance_km']:.1f}"
            )


if __name__ == "__main__":
    main()
