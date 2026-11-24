#!/usr/bin/env bash
# Remove accenthold and restart fcitx5.
set -euo pipefail
service=omarchy-fcitx5.service
rm -f "$HOME/.local/lib/fcitx5/libaccenthold.so" \
      "$HOME/.local/share/fcitx5/addon/accenthold.conf" \
      "$HOME/.config/systemd/user/$service.d/accenthold.conf"
rmdir --ignore-fail-on-non-empty "$HOME/.config/systemd/user/$service.d" 2>/dev/null || true
if [ "$(systemctl --user show -p LoadState --value "$service" 2>/dev/null)" = loaded ]; then
    systemctl --user daemon-reload
    systemctl --user restart "$service"
fi
echo "accenthold removed."
