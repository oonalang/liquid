#!/data/data/com.termux/files/usr/bin/bash
# Termux crash capture for CODM (rooted shell or wireless ADB to itself).
# Usage: tb-crash.sh {start|stop|snap|status|pack|help}
set -u
PKG="${PKG:-com.garena.game.codm}"
# Default the run folder next to this script, so `status`/`stop` find the same
# run in a new shell without having to re-export BASE every time.
SELF_DIR=$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" 2>/dev/null && pwd) || SELF_DIR="$HOME"
BASE="${BASE:-$SELF_DIR/capture}"
mkdir -p "$BASE" || exit 1

MODE=adb
if command -v su >/dev/null 2>&1 && su -c id >/dev/null 2>&1; then MODE=root; fi
[ -n "${FORCE_MODE:-}" ] && MODE="$FORCE_MODE"

say() { printf '[%s] %s\n' "$(date +%H:%M:%S)" "$*"; }
cur() { cat "$BASE/current" 2>/dev/null; }

# Virtual-space containers (Gspace, Island, Parallel Space) rename the process, e.g.
# com.garena.game.codm:p0, so pidof on the bare package finds nothing. Match the
# package substring in the process list as a fallback.
game_running() {
  if command -v pidof >/dev/null 2>&1 && pidof "$PKG" >/dev/null 2>&1; then return 0; fi
  ps -A -o NAME 2>/dev/null | grep -F -q "${PKG#com.}" && return 0
  ps -A -o NAME 2>/dev/null | grep -F -q "${PKG##*.}" && return 0
  return 1
}

need_tool() {
  case "$MODE" in
    root) ;;
    adb)  command -v adb >/dev/null 2>&1 || { say "adb missing: pkg install android-tools"; exit 1; }
          adb devices 2>/dev/null | grep -q 'device$' || { say "no adb device. Pair first (see help)."; exit 1; } ;;
  esac
}

do_start() {
  need_tool
  local d="$BASE/run-$(date +%Y%m%d-%H%M%S)"
  mkdir -p "$d" && echo "$d" >"$BASE/current"
  say "capture dir: $d (mode: $MODE)"

  if [ "$MODE" = root ]; then
    su -c 'logcat -b all -c' >/dev/null 2>&1
    su -c 'logcat -v threadtime -b all' >"$d/logcat.txt" 2>&1 &
  else
    adb logcat -b all -c >/dev/null 2>&1
    adb logcat -v threadtime -b all >"$d/logcat.txt" 2>&1 &
  fi
  echo $! >"$d/logcat.pid"
  say "logging to $d/logcat.txt"
  say "now start a match and let it crash. stop with: $0 stop"

  # Watchdog: when the game process disappears, snapshot the crash buffers and pack.
  (
    local i=0 seen=0
    while :; do
      sleep 2
      i=$((i + 1))
      if game_running; then
        seen=1
      elif [ "$seen" = 1 ]; then
        sleep 3
        say "game process gone - collecting crash data"
        do_snap "$d" gone
        do_stop
        exit 0
      fi
      [ "$i" -ge 1800 ] && { say "watchdog timeout (1h)"; exit 0; }
    done
  ) >"$d/watch.log" 2>&1 &
  echo $! >"$d/watch.pid"
}

do_snap() {
  local d="${1:-$(cur)}" why="${2:-manual}"
  [ -n "$d" ] && [ -d "$d" ] || { say "nothing to snapshot"; return 1; }
  say "snapshot ($why) -> $d"
  if [ "$MODE" = root ]; then
    su -c 'logcat -b crash -d' >"$d/crash-buffer.txt" 2>&1
    su -c 'cat /data/tombstones/tombstone_*' >"$d/tombstones.txt" 2>&1
    su -c 'dmesg' >"$d/kernel.txt" 2>&1
  else
    adb logcat -b crash -d >"$d/crash-buffer.txt" 2>&1
    adb shell dumpsys dropbox --print data_app_native_crash >"$d/dropbox-native.txt" 2>&1
    adb shell dumpsys dropbox --print SYSTEM_TOMBSTONE >"$d/dropbox-tombstone.txt" 2>&1
    adb shell dumpsys dropbox --print data_app_crash >"$d/dropbox-java.txt" 2>&1
  fi
  cp -f "$d/logcat.txt" "$d/logcat-tail.txt" 2>/dev/null
  [ -f "$d/logcat-tail.txt" ] && tail -c 4000000 "$d/logcat-tail.txt" >"$d/logcat-tail.tmp" &&
    mv -f "$d/logcat-tail.tmp" "$d/logcat-tail.txt"
}

do_stop() {
  local d; d="$(cur)"
  [ -n "$d" ] || { say "no run in progress"; return 0; }
  [ -f "$d/watch.pid" ] && kill "$(cat "$d/watch.pid")" 2>/dev/null
  [ -f "$d/logcat.pid" ] && kill "$(cat "$d/logcat.pid")" 2>/dev/null
  if [ "$MODE" = root ]; then su -c 'pkill -x logcat' >/dev/null 2>&1
  else adb shell pkill -x logcat >/dev/null 2>&1; fi
  sleep 1
  do_pack "$d"
}

do_pack() {
  local d="${1:-$(cur)}"
  [ -n "$d" ] && [ -d "$d" ] || { say "nothing to pack"; return 1; }
  tar -czf "$d.tar.gz" -C "$BASE" "$(basename "$d")" 2>/dev/null &&
    say "packed: $d.tar.gz ($(du -h "$d.tar.gz" | cut -f1))"
}

do_status() {
  local d; d="$(cur)"
  [ -n "$d" ] || { say "no capture yet"; return 0; }
  say "dir: $d"
  say "mode: $MODE"
  game_running && say "game: running" || say "game: not running"
  [ -f "$d/logcat.txt" ] && say "log: $(wc -l <"$d/logcat.txt" 2>/dev/null || echo 0) lines" || say "log: missing"
  [ -f "$d/crash-buffer.txt" ] && say "crash buffer: $(wc -l <"$d/crash-buffer.txt" 2>/dev/null || echo 0) lines"
}

case "${1:-help}" in
  start)  do_start ;;
  snap)   do_snap "$(cur)" manual ;;
  stop)   do_stop ;;
  pack)   do_pack "$(cur)" ;;
  status) do_status ;;
  *) cat <<'TXT'
tb-crash.sh start    start a capture run, then play until it crashes
tb-crash.sh snap     grab crash buffer / tombstones right now
tb-crash.sh stop     stop logging, collect tombstones, pack a tar.gz
tb-crash.sh status   what is running, how much was logged
tb-crash.sh pack     pack the current run again

Root: detected automatically (su). Without root, pair wireless ADB once:
  pkg install android-tools
  # Settings > Developer options > Wireless debugging > Pair device with pairing code
  adb pair 127.0.0.1:<pair-port> <pairing-code>
  adb connect 127.0.0.1:<debug-port>
  adb devices
TXT
    ;;
esac
