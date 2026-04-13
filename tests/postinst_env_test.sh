#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

DEBIAN_POSTINST_SOURCE_ONLY=1
. "$PROJECT_ROOT/debian/postinst"

assert_equals()
{
    expected="$1"
    actual="$2"
    message="$3"

    if [ "$expected" != "$actual" ]; then
        printf 'FAIL: %s\nexpected: %s\nactual: %s\n' "$message" "$expected" "$actual" >&2
        exit 1
    fi
}

assert_equals "dxcb:xcb" "$(qt_qpa_platform_for_session_type x11)" "x11 should use dxcb"
assert_equals "wayland" "$(qt_qpa_platform_for_session_type wayland)" "wayland should use wayland"
assert_equals "" "$(qt_qpa_platform_for_session_type tty)" "unknown session type should not force QPA"
