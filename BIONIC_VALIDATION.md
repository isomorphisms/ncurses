# Android/Bionic validation record

## Current baseline

The pruning work from PR #1 is merged into `source-first-layout`.  Subsequent validation landed in three stages:

- PR #2 — ARMv7a and AArch64 Android/Bionic cross-build receipts;
- PR #3 — explicit PTY executable linkage, bundle/provenance checks and host execution of the same focused harness;
- PR #4 — Android 14 x86_64 emulator execution of a checksummed Bionic bundle.

The current merged `source-first-layout` head at this refresh is `da8525216cb37d383cfd4905611a3ddbcb46b773`.

No removed ncurses subsystem, historical optimizer, platform backend or language binding was restored to obtain these receipts.

## Evidence matrix

| Evidence | Established | Not established |
| --- | --- | --- |
| Ubuntu/glibc wide build | Surviving source configures and builds on the host | Bionic behavior |
| Ubuntu/glibc PTY harness | Core screen/input PTY behavior runs against the just-built pruned library | Android behavior |
| ARMv7a Bionic build/link | API-24 ARMv7a archive and PTY executable are target-built and ELF/provenance checked | ARMv7a Android execution |
| AArch64 Bionic build/link | API-24 AArch64 archive and PTY executable are target-built and ELF/provenance checked | AArch64 Android execution |
| Android 14 x86_64 emulator PTY run | The x86_64 bundle executes under Bionic in that emulator and the private-PTY harness passes | ARM runtime or physical-device behavior |
| Future physical-device PTY run | Private-PTY behavior on the named physical device | Interactive terminal/IME/resize behavior |
| Future interactive receipt | Explicitly observed behavior in a named terminal application and device context | Universal Android compatibility |

## Toolchain baseline

The Bionic path pins Android NDK `27.3.13750724` (r27d).  Keep that baseline stable while interpreting the existing receipts; upgrading the NDK is a separate change.

The cross-build uses a native compiler for ncurses build-time generators and the NDK compiler/binutils for target objects.  The two cached configuration choices retained from the known Android recipe are:

- `ac_cv_header_locale_h=no`
- `am_cv_langinfo_codeset=no`

They are build configuration choices, not assertions that current Bionic lacks the corresponding headers/interfaces.  UTF-8 behavior is checked by execution where execution evidence exists.

## ARMv7a and AArch64 bundle construction

From a clean checkout with the pinned NDK selected, the validation script supports:

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r27d
export EXPECTED_NDK_REVISION=27.3.13750724
bash .github/build_bionic_smoke.sh armv7a
bash .github/build_bionic_smoke.sh aarch64
```

Each ABI gets a fresh build directory under `_/bionic-validation/`; stale directories are rejected.

The script:

- configures and builds the surviving wide-character core;
- links `modern_core_smoke` using the explicit just-built `libncursesw.a`;
- does not substitute a system/Termux ncurses library;
- checks archive-member target architecture;
- checks executable ELF class/machine, PIE type, Android interpreter and dependencies;
- creates a self-contained terminfo fixture;
- records source/toolchain provenance and checksums;
- packages a runtime runner and build receipt.

The Android link does not use glibc's `-lutil`; Bionic supplies `openpty` from libc for the API level used here.

At exact PR #3 head `839e98129b17d54e9442f6679bb3840328de5b49`, ARMv7a and AArch64 build/link validation was green.

## Focused PTY behavior

The harness is deliberately narrower than an interactive terminal acceptance test.  It exercises:

- one PTY backing input and output;
- raw/noecho/keypad setup;
- WINDOW output;
- `wrefresh` / `doupdate` terminal emission;
- UTF-8 lambda output bytes;
- terminfo-derived `KEY_UP` recognition;
- ordinary `q` input;
- UTF-8 input through `wget_wch`;
- bounded empty-input timeout;
- explicit `resizeterm` state;
- `endwin` terminal-mode restoration.

The test has bounded waits and an overall alarm so an input failure produces a receipt rather than hanging indefinitely.

## Android emulator runtime receipt

PR #4 added an x86_64 Bionic bundle and a hosted Android-emulator job specifically for runtime evidence.

At exact PR #4 head `27041bf6dbb79efed955e02e9a2442279f62984e`, run `35046978535`:

1. booted an Android 14 x86_64 emulator under KVM;
2. pushed the exact-source checksummed bundle;
3. ran `sh ./run.sh emulator` inside Android;
4. pulled the runtime receipt and supporting logs;
5. required a Bionic runtime and explicit emulator context.

The recorded result includes:

- `runtime_libc=bionic`;
- `modern_core_pty=pass`;
- `exit_status=0`;
- `android_pty_runtime=pass`;
- `context_operator_declared=emulator`;
- `ro.kernel.qemu=1`;
- `interactive_device_acceptance=not_established`.

The retained artifact is `ncurses-android-emulator-35046978535` (artifact ID `10427910228`).

This closes the generic "no Android execution" gap, but only for the named x86_64 emulator image.

## Physical Android execution

The ARMv7a and AArch64 bundles remain candidates for physical-device testing.  A physical run should execute an exact successful bundle without recompiling or silently substituting installed ncurses code.

Inside the selected bundle directory:

```sh
sh ./run.sh physical
```

The resulting runtime directory should remain paired with the original build receipt and checksums.

A physical private-PTY pass would establish the focused PTY behavior on that named device/ABI/API.  It would still not establish interactive terminal application behavior.

## Interactive acceptance remains separate

A later interactive receipt must name the terminal application/version, actual `TERM` and terminfo source, device/ABI/API and exact tested binary, and record the observed behavior rather than inferring it from a private PTY.

Still outside the current acceptance boundary:

- IME/on-screen-keyboard behavior;
- genuine externally delivered SIGWINCH / `KEY_RESIZE`;
- rotation/window resizing;
- interactive redraw and cursor observations;
- normal recovery after user interrupt/suspend;
- mouse input;
- emoji/grapheme width;
- broader terminal-compatibility coverage.

`resizeterm()` state in the focused harness is not evidence of genuine resize delivery.

## Receipt rule

The build receipt field `android_runtime=not_run` describes the build stage that produced a bundle.  It is not retroactively rewritten when a bundle is later executed.  Runtime evidence lives in a separate runtime receipt so build and execution provenance stay distinguishable.

See `RECEIPTS.md` for the concise current acceptance ledger.
