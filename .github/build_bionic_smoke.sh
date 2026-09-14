#!/usr/bin/env bash
# Fresh cross-build and a relocatable PTY test bundle. Never runs target code.
set -euo pipefail
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${GITHUB_WORKSPACE:-}" ] && [ -f "$GITHUB_WORKSPACE/_/configure" ]; then
  root=$GITHUB_WORKSPACE
else
  root=$(cd "$script_dir/.." && pwd)
fi
abi=${1:?usage: bash .github/build_bionic_smoke.sh armv7a|aarch64}
api=24
case "$abi" in
  armv7a) target=armv7a-linux-androideabi; host=arm-linux-androideabi; machine=ARM; class=ELF32; loader=/system/bin/linker ;;
  aarch64) target=aarch64-linux-android; host=$target; machine=AArch64; class=ELF64; loader=/system/bin/linker64 ;;
  *) echo "unsupported ABI: $abi" >&2; exit 2 ;;
esac
# Select the installed NDK explicitly. Record and check its exact revision,
# rather than guessing which version a runner image happens to provide.
: "${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME to an installed Linux x86-64 NDK}"
: "${EXPECTED_NDK_REVISION:?set EXPECTED_NDK_REVISION to the selected exact NDK revision}"
ndk=$(cd "$ANDROID_NDK_HOME" && pwd)
tools=$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin
revision=$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$ndk/source.properties" | tr -d '\r')
[ "$revision" = "$EXPECTED_NDK_REVISION" ] || { echo "NDK revision mismatch: $revision" >&2; exit 1; }
for program in clang llvm-ar llvm-ranlib llvm-strip llvm-readelf llvm-nm; do test -x "$tools/$program"; done
for program in cc make tic python3 git sha256sum; do command -v "$program" >/dev/null; done
# Prefer the checkout's Git identity and cleanliness check.  Job containers may
# expose the exact checked-out workspace without its Git metadata; in that case
# the workflow must pass the exact checkout ref explicitly as SOURCE_COMMIT.
if git -C "$root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git -C "$root" diff --quiet HEAD --
  source_commit=$(git -C "$root" rev-parse HEAD)
  if [ -n "${SOURCE_COMMIT:-}" ] && [ "$source_commit" != "$SOURCE_COMMIT" ]; then
    echo "checkout/source mismatch: git=$source_commit expected=$SOURCE_COMMIT" >&2
    exit 1
  fi
else
  : "${SOURCE_COMMIT:?SOURCE_COMMIT is required when Git metadata is unavailable}"
  source_commit=$SOURCE_COMMIT
fi
work=$root/_/bionic-validation/$abi
mkdir -p "$(dirname "$work")"
mkdir "$work" || { echo "build directory already exists; use a fresh checkout: $work" >&2; exit 1; }
mkdir "$work/build" "$work/bundle"
trap 'status=$?; if [ "$status" -ne 0 ]; then for log in "$work/configure.log" "$work/make.log"; do if [ -f "$log" ]; then echo "--- $log"; tail -n 30 "$log"; fi; done; fi' EXIT
bundle=$work/bundle
{
  printf 'source_commit=%s\nabi=%s\nandroid_api=%s\nndk_revision=%s\n' "$source_commit" "$abi" "$api" "$revision"
  printf 'target=%s%s\n' "$target" "$api"
  "$tools/clang" --version
  cc --version | sed -n '1p'
  tic -V
  cat /etc/os-release
} > "$work/build-environment.txt"
(
  cd "$work/build"
  export CC="$tools/clang --target=$target$api"
  export AR="$tools/llvm-ar" RANLIB="$tools/llvm-ranlib" STRIP="$tools/llvm-strip" NM="$tools/llvm-nm"
  export CPPFLAGS=-fPIC CFLAGS=-O2 LDFLAGS= LIBS= CONFIG_SITE=/dev/null
  # Preserve PR #2's configuration assumptions, not a claim that locale.h
  # is absent from Bionic. Runtime UTF-8 behavior is tested separately.
  export ac_cv_header_locale_h=no am_cv_langinfo_codeset=no
  printf "stage=configure\n"
  "$root/_/configure" --host="$host" --with-build-cc=cc \
    --without-ada --without-cxx --without-cxx-binding --without-debug \
    --without-manpages --without-tests --disable-stripping \
    --without-shared --with-normal --enable-widec > "$work/configure.log" 2>&1
  printf "stage=build\n"
  make -j2 > "$work/make.log" 2>&1
)
printf "stage=link_and_inspect\n"
archive=$work/build/lib/libncursesw.a
test -s "$archive"
"$tools/llvm-readelf" -h "$archive" > "$work/archive-elf.txt"
# Explicit archive path: cannot silently link the system/Termux ncurses.
# openpty belongs to Bionic libc; do not carry the host's -lutil into Android.
"$tools/clang" --target="$target$api" -fPIE -pie \
  -D_DEFAULT_SOURCE -DEXPECT_BIONIC -I"$work/build/include" \
  "$root/.github/bionic_runtime_smoke.c" "$archive" \
  -Wl,--no-undefined,-Map,"$work/link.map" -o "$bundle/modern_core_smoke"
"$tools/llvm-readelf" -h -l -d "$bundle/modern_core_smoke" > "$bundle/executable-elf.txt"
python3 - "$work/archive-elf.txt" "$bundle/executable-elf.txt" "$machine" "$class" "$loader" <<'PY'
import pathlib, re, sys
archive, executable, machine, elf_class, loader = sys.argv[1:]
for name in (archive, executable):
    text = pathlib.Path(name).read_text()
    machines = re.findall(r'^\s*Machine:\s*(.*?)\s*$', text, re.M)
    classes = re.findall(r'^\s*Class:\s*(.*?)\s*$', text, re.M)
    if not machines or set(machines) != {machine} or set(classes) != {elf_class}:
        raise SystemExit(f'wrong/mixed/empty ELF architecture: {name}: {set(machines)}, {set(classes)}')
text = pathlib.Path(executable).read_text()
if not re.search(r'^\s*Type:\s+DYN\b', text, re.M):
    raise SystemExit('expected a position-independent Android executable')
if f'[Requesting program interpreter: {loader}]' not in text:
    raise SystemExit('wrong Android program interpreter')
needed = re.findall(r'\(NEEDED\).*?\[(.*?)\]', text)
if 'libc.so' not in needed or set(needed) - {'libc.so', 'libm.so', 'libdl.so'}:
    raise SystemExit(f'unexpected dynamic dependencies: {needed}')
print('ELF architecture, Android interpreter and dependencies verified:', needed)
PY
mkdir "$bundle/terminfo"
tic -o "$bundle/terminfo" "$root/.github/modern_core_smoke.ti"
test -s "$bundle/terminfo/n/ncurses-pruned-smoke"
cp "$root/.github/run_bionic_smoke.sh" "$bundle/run.sh"
cp "$root/_/COPYING" "$bundle/COPYING"
cp "$work/build-environment.txt" "$bundle/build-environment.txt"
{
  printf 'source_commit=%s\nabi=%s\nandroid_api=%s\nndk_revision=%s\n' "$source_commit" "$abi" "$api" "$revision"
  printf 'compile_link_elf=pass\nandroid_runtime=not_run\nphysical_device_acceptance=not_established\n'
  printf 'archive_sha256=%s\n' "$(sha256sum "$archive" | cut -d ' ' -f 1)"
} > "$bundle/build-receipt.txt"
(
  cd "$bundle"
  find . -type f ! -name SHA256SUMS -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > SHA256SUMS
)
printf 'Compile/link bundle: %s\nAndroid execution: NOT RUN\n' "$bundle"
