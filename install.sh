#!/usr/bin/env bash
# Build accenthold and install it into ~/.local (no root needed), then point
# fcitx5 at it and restart fcitx5. Re-run after fcitx5 upgrades.
set -euo pipefail
cd "$(dirname "$0")"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

install -Dm755 build/libaccenthold.so "$HOME/.local/lib/fcitx5/libaccenthold.so"
install -Dm644 accenthold.conf "$HOME/.local/share/fcitx5/addon/accenthold.conf"

# fcitx5 only loads addon libraries from FCITX_ADDON_DIRS (default /usr/lib/fcitx5).
service=omarchy-fcitx5.service
if systemctl --user cat "$service" >/dev/null 2>&1; then
    dropin="$HOME/.config/systemd/user/$service.d"
    mkdir -p "$dropin"
    printf '[Service]\nEnvironment=FCITX_ADDON_DIRS=%s:/usr/lib/fcitx5\n' \
        "$HOME/.local/lib/fcitx5" > "$dropin/accenthold.conf"
    systemctl --user daemon-reload
    systemctl --user restart "$service"
    echo "Installed and fcitx5 restarted."
else
    echo "Installed. $service not found: start fcitx5 with"
    echo "  FCITX_ADDON_DIRS=$HOME/.local/lib/fcitx5:/usr/lib/fcitx5"
    echo "in its environment, then restart it."
fi
