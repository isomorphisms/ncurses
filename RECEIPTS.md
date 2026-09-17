# Build and test receipts

This file records what actually ran.  Keep compile, host runtime, Android emulator runtime, physical-device runtime, and interactive-terminal acceptance separate.

## Evidence levels

- **source inspected** — ownership/call paths were traced, but nothing was compiled or run.
- **built** — the named source/configuration compiled and linked for the named target.
- **host tested** — the named executable ran on the Linux/glibc CI host.
- **Android emulator tested** — the named Android executable ran under the recorded emulator image.
- **physical-device tested** — the named Android executable ran on a named physical device.
- **interactive accepted** — terminal/IME/resize behavior was observed in an actual interactive terminal application.

Do not promote one level into another.

## Linux/glibc wide-character core

The `modern core` workflow runs on GitHub-hosted Ubuntu 24.04 x86-64/glibc.  It configures the inherited build from `_` with wide-character support while disabling removed Ada/C++ bindings and the broad inherited test tree.

The selected source modules are `ncurses progs`.  The workflow builds the wide-character curses core and ordinary programs and then compiles and runs the focused PTY harness against the just-built library.

The focused harness exercises:

- a real pseudo-terminal;
- `newterm` and terminal-mode setup;
- ordinary WINDOW output;
- wide UTF-8 output, including `λ`;
- `wrefresh` / `doupdate` terminal emission;
- terminfo-derived cursor-up input decoded by `wgetch` as `KEY_UP`;
- ordinary character input;
- bounded empty-input timeout behavior;
- explicit resize state;
- terminal-mode restoration at normal exit.

The one-PTY form first passed at `fa011bb00cf2ca73066d1923c3426ff869a282d6`.  The workflow remained green after the later source-manifest and documentation cleanup.

## Android/Bionic ARM builds

PR #2 introduced Android/Bionic cross-build receipts.  PR #3 then tightened the boundary so the focused PTY executable is explicitly linked against the just-built archive and packaged with its exact terminfo fixture and provenance.

At exact PR #3 head `839e98129b17d54e9442f6679bb3840328de5b49`, the current Bionic validation path passed for:

- ARMv7a, API 24;
- AArch64, API 24.

The jobs use Android NDK `27.3.13750724`.  They verify the target archive members and the linked executable's ELF class/machine, PIE/interpreter/dependencies, bundle checksums, and exact-source provenance.

This is **Android/Bionic build/link evidence for ARMv7a and AArch64**.  Runtime evidence is recorded separately below.

## Android emulator runtime

PR #4 added a distinct x86_64 Android-emulator runtime path instead of relabeling the ARM cross-builds as runtime evidence.

At exact PR #4 head `27041bf6dbb79efed955e02e9a2442279f62984e`, run `35046978535` booted an Android 14 x86_64 emulator under KVM, pushed the checksummed Bionic bundle, and ran the PTY harness inside Android.

The retained runtime receipt records:

- `runtime_libc=bionic`;
- `modern_core_pty=pass`;
- `exit_status=0`;
- `android_pty_runtime=pass`;
- emulator context (`ro.kernel.qemu=1`);
- `interactive_device_acceptance=not_established`.

The emulator artifact is `ncurses-android-emulator-35046978535` (artifact ID `10427910228`).

This establishes **private-PTY execution on that Android 14 x86_64 emulator image**.  It does not establish ARM runtime behavior or physical-device behavior.

## Physical ARMv7 Android runtime

After PR #4 merged, the `bionic core` workflow passed again at exact merged commit `da8525216cb37d383cfd4905611a3ddbcb46b773`.  Run `35047148164` produced the ARMv7a API 24 artifact `bionic-api24-armv7a-35047148164` (artifact ID `10427477879`).

On 2026-09-17, that exact post-merge artifact was fetched from GitHub with `gh run download` and executed from native Termux on the operator's physical ARMv7 Android phone with:

```sh
sh ./run.sh physical
```

The bundle checksum verification passed for `COPYING`, build environment, build receipt, ELF report, executable, runner, and terminfo fixture.  The runtime output then recorded:

- `runtime_libc=bionic`;
- `modern_core_pty=pass`;
- `exit_status=0`;
- `android_pty_runtime=pass`;
- `interactive_device_acceptance=not_established`;
- `finished_utc=2026-09-17T09:21:00Z`.

The runner wrote the device-side receipt directory under native Termux.  Device serial numbers are deliberately not part of this receipt.

This establishes **physical ARMv7 Android/Bionic private-PTY execution of the exact post-merge bundle**.  It does not establish interactive terminal-app behavior, IME/keyboard behavior, or externally delivered resize behavior.

## Still unclaimed

The current receipts do **not** establish:

- AArch64 runtime on Android;
- physical AArch64 phone or tablet execution;
- interactive terminal-app behavior;
- IME/on-screen-keyboard behavior;
- genuine SIGWINCH / `KEY_RESIZE` delivery from an external window-size change;
- rotation behavior;
- mouse behavior;
- suspend/resume behavior;
- emoji/grapheme-width correctness;
- universal Android compatibility.

Those must remain separate receipts if pursued.

## Historical failures retained as evidence

The first Linux build attempt failed before configuration because inherited `configure` mines `NCURSES_MAJOR`, `NCURSES_MINOR`, and `NCURSES_PATCH` from `dist.mk`.  The release/distribution body remains deleted; a tiny `dist.mk` containing only those version values is retained temporarily as build metadata.

An early `make -C ncurses test_progs` attempt failed while linking the inherited embedded `lib_mvcur` optimizer tester.  That tester supplied its own output symbols while also linking the complete library containing the real definitions.  This was a historical test-harness collision, not a normal core build failure; the tester was removed with the optimizer it measured.

The first focused PTY test incorrectly used unrelated input/output PTYs.  Bounded `wgetch` calls returned `ERR`.  The corrected harness uses the two descriptors of one terminal PTY and is green.

## Current merged baseline

`source-first-layout` currently includes the pruning work plus PRs #2, #3, and #4.  The latest merge at the time of this receipt refresh is `da8525216cb37d383cfd4905611a3ddbcb46b773` (`Add Android emulator runtime receipt`).
