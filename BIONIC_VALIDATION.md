# Next Android/Bionic validation

## Baseline and scope

PR #1's pruning was merged into `source-first-layout`, not upstream `master`.
PR #2 subsequently merged at `d1f5729b372b5479eb6ba5c44291393f5f5b6dd7` on
2026-09-11. Its ARMv7a and AArch64 API 24 cross-builds passed. The existing
`RECEIPTS.md` records compile evidence, not Android execution.

The later PR #2 head `c7a594551b5334bfe2a60749af8b97755ce1fa22` also has passing
ARMv7a, AArch64 and glibc checks. The AArch64 job log identifies NDK
**27.3.13750724**. Retain that known baseline instead of upgrading toolchains
while adding a runtime test. The old build already linked ordinary programs;
the missing link test is the actual curses screen/input PTY harness.

Sources: [PR #2](https://github.com/isomorphisms/ncurses/pull/2),
[observed AArch64 job](https://github.com/isomorphisms/ncurses/actions/runs/34656071576/job/103448647286),
[NDK cross-build guidance](https://developer.android.com/ndk/guides/other_build_systems),
[Bionic API/locale notes](https://android.googlesource.com/platform/bionic/+/HEAD/docs/status.md).

No inherited implementation, removed subsystem, capability table or historical
optimizer is restored by this change. `hardscroll`/`hashmap` remain as merged.
The existing smoke is retained. The new harness is validation code, not a
replacement curses implementation.

## Acceptance levels

| Evidence | What it establishes | What it does not establish |
| --- | --- | --- |
| Existing PR #2 cross-build | Two Android ABI builds of the pruned source | Android execution |
| New build, link and ELF checks | The screen/input harness links this exact archive into an Android PIE, with the expected ABI, loader and platform dependencies | Successful startup or PTY behavior |
| New harness on GitHub-hosted Ubuntu/glibc | The test itself exercises the host-built pruned core | Bionic behavior |
| Harness in an Android emulator | That image's Bionic/PTY execution | Physical phone/tablet behavior |
| Harness on a named physical device | That device's private-PTY execution for the checks below | Interactive renderer, keyboard/IME or real resize acceptance |
| Separate interactive device observations | The explicitly recorded terminal/version/device behaviors | Universal Android compatibility |

The new path needs its own exact-source checks; shell/YAML parsing or a compiled
terminfo fixture is not a C build receipt, and a queued/skipped job is not a
pass. Hosted-Ubuntu build receipts remain separate from Android execution.

## Build a fresh bundle

The host and Bionic workflows use GitHub-hosted `ubuntu-24.04`, verify the
Ubuntu/x86-64 execution environment, and explicitly check out the PR head (or
push SHA) rather than silently attributing a synthetic merge checkout to the
head commit.

The workflows install their ordinary Ubuntu build prerequisites themselves.
The Bionic cross-build workflow downloads Android NDK r27d, verifies the pinned
archive checksum and exact revision `27.3.13750724`, and exports that selected
NDK explicitly. No self-hosted runner registration or external runner
provisioning is required.

From a clean checkout of the validation branch on a compatible Ubuntu host:

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r27d
export EXPECTED_NDK_REVISION=27.3.13750724
bash .github/build_bionic_smoke.sh armv7a
bash .github/build_bionic_smoke.sh aarch64
```

Each ABI gets a new directory under `_/bionic-validation/`; an existing build
directory is rejected to prevent stale acceptance. The native compiler runs
source generators; the NDK compiler and binutils build target code. The two
PR #2 cache assumptions are retained for comparison:
`ac_cv_header_locale_h=no` and `am_cv_langinfo_codeset=no`. They are configuration
choices, not a claim that Bionic lacks `locale.h`. Actual UTF-8 behavior must
pass the execution checks.

The script builds the surviving modules, links an explicit `libncursesw.a`
path into `modern_core_smoke`, and inspects every reported archive member's
machine/class, the executable's machine/class/PIE type, Android interpreter,
and dynamic dependencies. No system/Termux curses can replace that explicit
archive. The Android link does not import glibc's `-lutil`: Bionic provides
`openpty` in libc at the API level used here.

The workflow retains `bundle/`, configure/make logs, `config.log`, the archive
ELF report and link map. The bundle includes a uniquely named, self-contained
terminfo fixture, build provenance, checksums and a runtime runner. Native
`tic` compiles data; it is not an Android execution receipt.

## Execute on Android, separately

Use the artifact matching the Android userspace ABI. Copy the contents of its
`bundle/` into a fresh test directory under native Termux's home, or a selected
Android device/emulator's `/data/local/tmp`. Do not replace installed ncurses
or install a compiler on the phone for this test. No bundle is advertised as
available until its producing job succeeds.

From inside that bundle directory on the selected Android environment:

```sh
# Choose the truthful context. Neither label is inferred from a host build.
sh ./run.sh physical
# OR, when actually running on an Android emulator:
sh ./run.sh emulator
```

For ADB, select the device explicitly with `adb -s "$ANDROID_SERIAL"`; do not
implicitly choose whichever device happens to be attached. The program
creates its own PTY, so allocating an ADB terminal is not the test mechanism.

The runner verifies the bundle hashes, requires Android API >= 24, records
API/model/ABI list/build fingerprint and binary SHA, and writes each invocation
to a new `runtime.XXXXXX/` directory. Context is operator-declared; an emulator
property can contradict a physical label, but its absence does not prove
physical hardware. It does not collect device serial numbers.

A pass requires process exit zero, the Bionic build marker, and completion of:

- one PTY backing input and output, with raw/noecho/keypad setup;
- WINDOW output and refresh/doupdate, including emitted UTF-8 lambda bytes;
- terminfo-derived KEY_UP, ordinary `q`, and UTF-8 input via `wget_wch`;
- an empty-input timeout, explicit `resizeterm` state, and `endwin` tty restoration.

The complete test has a ten-second alarm; input waits are also bounded.
The fixture is deliberately not a complete xterm compatibility test. Keep
`context.txt`, `output.txt` and `runtime-receipt.txt` together with the original
bundle/build receipt. The old `android_runtime=not_run` field in the build
receipt describes the build stage; only the separate runtime receipt reports
subsequent execution.

Even a physical private-PTY pass leaves **interactive device acceptance
unestablished**. That later receipt must name the terminal app/version, actual
TERM and terminfo source, device/ABI/API, exact binary, and observed interactive
redraw, cursor motion, UTF-8 input/output, genuine window-size changes, and
normal exit/interrupt recovery. Calling `resizeterm` is not SIGWINCH, rotation,
or KEY_RESIZE delivery evidence. Emoji/grapheme width, mouse input and
suspend/resume are also outside this focused smoke unless separately tested.
