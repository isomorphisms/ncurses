#!/system/bin/sh
# Run as: sh ./run.sh emulator|physical
# Context is operator-declared; a physical PTY run is not interactive acceptance.
set -eu
context=${1:-}
case "$context" in emulator|physical) ;; *) echo 'usage: sh ./run.sh emulator|physical' >&2; exit 2 ;; esac
bundle=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$bundle"
[ -x /system/bin/getprop ] || { echo 'requires an Android userspace, not a desktop host' >&2; exit 2; }
sdk=$(/system/bin/getprop ro.build.version.sdk)
case "$sdk" in ''|*[!0-9]*) echo 'unreadable Android API level' >&2; exit 2 ;; esac
[ "$sdk" -ge 24 ] || { echo 'this bundle requires Android API 24 or later' >&2; exit 2; }
qemu=$(/system/bin/getprop ro.kernel.qemu)
[ "$context:$qemu" != physical:1 ] || { echo 'emulator property contradicts physical label' >&2; exit 2; }
# Verify all build/binary/fixture inputs before launching, and never source
# the receipt as shell code. Each invocation gets a fresh result directory.
sha256sum -c SHA256SUMS
result=$(mktemp -d "$bundle/runtime.XXXXXX")
{
  printf 'context_operator_declared=%s\nandroid_api=%s\nro.kernel.qemu=%s\n' "$context" "$sdk" "$qemu"
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'model=%s\n' "$(/system/bin/getprop ro.product.model)"
  printf 'device_abis=%s\n' "$(/system/bin/getprop ro.product.cpu.abilist)"
  printf 'build_fingerprint=%s\n' "$(/system/bin/getprop ro.build.fingerprint)"
  printf 'binary_sha256=%s\n' "$(sha256sum modern_core_smoke | cut -d ' ' -f 1)"
  cat build-receipt.txt
  printf 'test_scope=private_pty_not_interactive_terminal\n'
} > "$result/context.txt"
chmod u+x ./modern_core_smoke
status=0
(
  unset LD_PRELOAD LD_LIBRARY_PATH
  export LC_ALL=C.UTF-8 TERM=ncurses-pruned-smoke
  export NCURSES_SMOKE_TERM=ncurses-pruned-smoke
  export TERMINFO="$bundle/terminfo" TERMINFO_DIRS="$bundle/terminfo"
  ./modern_core_smoke
) > "$result/output.txt" 2>&1 || status=$?
if [ "$status" -eq 0 ]; then
  grep -qx 'runtime_libc=bionic' "$result/output.txt" || status=1
  grep -qx 'modern_core_pty=pass' "$result/output.txt" || status=1
fi
{
  printf 'exit_status=%s\n' "$status"
  if [ "$status" -eq 0 ]; then printf 'android_pty_runtime=pass\n'; else printf 'android_pty_runtime=fail\n'; fi
  printf 'interactive_device_acceptance=not_established\n'
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$result/runtime-receipt.txt"
cat "$result/output.txt" "$result/runtime-receipt.txt"
printf 'receipt_directory=%s\n' "$result"
exit "$status"
