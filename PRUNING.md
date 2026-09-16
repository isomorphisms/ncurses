# Pruning ncurses toward a readable modern core

This repository branch is an archaeology stage between inherited ncurses and a later clean-sheet icurses.  The method is subtraction: preserve the modern curses execution model, remove machinery for declared-out-of-scope worlds, and record what each removed mechanism used to solve.

The authoritative upstream/original implementation remains available from `master` and repository history.

## Target

Keep:

- modern Unix/POSIX terminal behavior;
- Linux/glibc;
- Android/Bionic;
- ordinary PTYs and modern terminal emulators;
- UTF-8 / wide characters and column-width correctness;
- WINDOW/subwindow/pad storage;
- colors and attributes;
- raw/cbreak/echo terminal modes;
- resize/SIGWINCH support;
- ordinary keyboard escape-sequence recognition;
- ordinary mouse support for now;
- terminfo for now;
- desired-screen versus remembered-physical-screen updating.

Do not optimize this branch for native Windows consoles, OS/2, proprietary/ancient Unix variants, slow serial terminals, historical hardware terminals, removed language bindings, removed higher-level libraries, or upstream packaging/release production.

## Evidence vocabulary

- **inspected** — source ownership/call paths were traced;
- **built** — the named target configuration compiled/linked;
- **host tested** — the named executable ran on Linux/glibc;
- **Android emulator tested** — the named Android executable ran in the recorded emulator context;
- **physical-device tested** — the named Android executable ran on named physical hardware;
- **interactive accepted** — behavior was observed inside a named interactive terminal application.

Do not promote evidence across these boundaries.

Current evidence includes Linux/glibc host execution, ARMv7a/AArch64 Bionic build/link receipts, and an Android 14 x86_64 emulator private-PTY runtime receipt.  Physical-device and interactive-terminal acceptance remain unclaimed.  Exact receipts live in `RECEIPTS.md`.

# Phase 1 — separate non-core implementations

## Ada95 and C++ bindings

**Removed:** the complete inherited `_/Ada95` and `_/c++` implementation trees and their top-level source-first facades.

**Original problem:** language-specific wrappers, demos and build machinery for Ada95/C++ clients.

**Why outside target:** the object of study is the C curses implementation: WINDOW state, refresh, tty realization, cursor movement and input decoding.

**Replacement:** none.  The C implementation/API remains.

**Remaining residue:** inherited configuration still contains Ada/C++ probing/options even though the selected modern build disables them.  That is build-system archaeology, not retained target functionality.

**Archaeology:** history for `Ada95/`, `c++/`, their Makefiles and configure branches.

## form, menu and panel libraries

**Removed:** `_/form`, `_/menu`, `_/panel` and their top-level facades.

**Original problem:** higher-level field/form, menu-selection and panel-stacking libraries layered on curses windows.

**Why outside target:** they consume curses rather than implement the terminal/screen core.

**Replacement:** ordinary WINDOW/subwindow/pad, attributes, input and refresh operations.

**Build observation:** inherited `configure.in` still textually mentions `panel menu form`, but `CF_SRC_MODULES` only admits source modules whose module files exist.  The supported configure receipts select `ncurses progs`; no fake replacement directories were added.

**Archaeology:** history for `form/`, `menu/`, `panel/` and `CF_SRC_MODULES`.

## Win32 console and MinGW implementation

**Removed:** native Win32 console implementation sources, Win32/MinGW-specific implementation headers and the MinGW README; dead Win32 source-manifest groups were also removed.

**Original problem:** drive a native Windows console rather than the Unix tty/escape-sequence path and adapt ncurses to MinGW/MSVC environments.

**Why outside target:** Linux/glibc and Android/Bionic use the POSIX tty/PTY + terminal-sequence path.

**Replacement:** terminal setup, termios/PTY handling, `tty_update`, `lib_mvcur`, and terminfo-selected sequences.

**Remaining residue:** private/configuration headers and Makefile/configure templates still contain some Windows-oriented declarations/names such as `nc_win32.h`.  These are cleanup candidates only after the selected Linux/Bionic builds demonstrate they are unused.

**Archaeology:** history for `ncurses/win32con`, `ncurses/tinfo/lib_win32*`, MinGW headers, `include/nc_win32.h`, and associated configure branches.

## OS/2 and EMX

**Removed:** OS/2/EMX make/build files, EMX terminal data, REXX scripts and OS/2 DLL export/ref tables.

**Original problem:** build ncurses in EMX, manufacture OS/2 DLL/import libraries, carry OS/2 terminal data and package binary distributions.

**Why outside target:** no role in the POSIX tty/PTY target.

**Replacement:** none.

**Remaining residue:** generic private/configuration code still carries some OS/2/EMX conditionals; those are candidates for a later build/private-header cleanup.

**Archaeology:** history for `Makefile.os2`, `README.emx`, `misc/emx.src`, `misc/*.cmd`, and OS/2 export tables.

# Phase 2 — packaging and release engineering

## Distribution machinery

**Removed:** distro/package trees, downstream patch payloads, release announcement/manifest machinery, tarball packaging tests and the release-production body of `dist.mk`.

**Original problem:** build and distribute upstream/downstream ncurses packages and release archives.

**Why outside target:** none participates in WINDOW state, input, terminal diffing, cursor movement or terminal output.

**Replacement:** none.  Repository history remains the packaging archaeology source.

## Tiny `dist.mk` retained temporarily

The first Linux configure receipt failed after `dist.mk` was deleted because inherited `configure` mines `NCURSES_MAJOR`, `NCURSES_MINOR` and `NCURSES_PATCH` from that file before doing useful work.

A tiny `_/dist.mk` containing only those version values is therefore retained as build metadata.  The release Makefile body remains deleted.  Moving these version constants into a smaller eventual build definition is still desirable.

# Phase 3 — narrow the platform/build world

## Proprietary-platform capability tables

**Removed:** alternate capability tables for AIX4, HP-UX11, OSF/1 and UWIN.

**Original problem:** match those systems' terminfo capability-name/order conventions.

**Why outside target:** those environments are outside the branch target.

**Replacement:** normal `include/Caps` / `include/Caps-ncurses` paths.

## Build/configuration residue still present

Inherited Autoconf/generated build machinery remains much broader than the source set we actually support.  Known residue includes:

- Ada compiler/binding probes and substitutions;
- C++ binding/compiler compatibility probes;
- textual form/menu/panel module selection;
- Windows/MinGW/MSVC/OS2 branches;
- proprietary/old-Unix compiler workarounds;
- Win32 dependency names in templates;
- generated `configure` carrying the same historical selection world.

These branches are not counted as supported targets.  Current receipts demonstrate that the modern source set selects/builds as `ncurses progs` on Linux/glibc and cross-builds for Android/Bionic.

Do not replace this residue with generic no-op abstraction layers.  Remove one dead build family at a time and keep generated/configure inputs consistent.

# Phase 4 — compatibility API audit

These families remain intentionally visible pending call-site/test decisions:

## Public termcap-compatible API

`tget*`/`tgoto` compatibility spelling is a different question from parsing termcap data.  `read_termcap.c` must not be deleted merely because the old public API looks historical.

## Soft-label keys

`lib_slk*` and wide-character SLK support remain.  Determine internal users and ordinary modern application expectations before removing them.

## Screen dump/restore

Historical putwin/screen dump/restore interfaces remain a compatibility family to audit separately.

## Legacy coding mode

`use_legacy_coding()` and its 8-bit branches still affect tty character handling.  For a UTF-8-focused modern branch this is suspicious, but remove it only with focused wide-character/output checks.

## Obsolete aliases

`tinfo/obsolete.c` and related compatibility spellings remain candidates after internal/public use is separated.

# Phase 5 — simplified `lib_mvcur.c`

## Upstream model before pruning

The inherited cursor optimizer compared multiple strategy families: absolute addressing, local movement, carriage-return/home/lower-left shortcuts, an auto-left-margin wrap trick, hard tabs/back-tabs, row/column sub-addressing and even overwriting desired text to advance the cursor cheaply.

Its cost model was expressed in old transmission-time terms, including tty baud rate and `$<...>` padding delays.  Comments explicitly reflected slow-terminal assumptions.

## Removed cursor mechanisms

- baud-rate-derived movement pricing;
- padding-delay contribution to cursor-route pricing;
- hard-tab/back-tab movement;
- memory-relative cursor addressing fallback;
- row/column sub-address shortcuts inside the relative route;
- carriage-return strategy;
- `cursor_home` strategy;
- `cursor_to_ll` strategy;
- text-overwrite-as-motion;
- auto-left-margin backward wrapping;
- embedded optimizer/timing test program.

## Surviving cursor algorithm

    current physical position + desired position
        -> build ordinary relative up/down/left/right route when possible
        -> expand absolute cursor_address when possible
        -> compare emitted byte counts
        -> emit the shorter available route
        -> remember the resulting physical position

Parameterized relative moves remain because they are ordinary modern terminal motion.  One-cell capabilities remain as alternate spellings.

The historical `_nc_msec_cost` name remains temporarily because line painting also uses it; on this branch the cursor path no longer models real transmission time.

## Save/restore dependency

A save/restore-cursor suppression block remains because the inherited hardware-scroll path can still depend on those capabilities.  Remove that with hardware scrolling, not independently.

# Phase 6 — tty screen updater audit

## `hardscroll.c`

`hardscroll.c` is explicitly a hardware scrolling optimization.  It recognizes vertical line movement and tries insert/delete-line or scroll operations before ordinary line painting, historically reducing traffic for screen editors and slow terminals.

This is outside the intended conceptual core, but it is still wired into `doupdate` and associated SCREEN state.

## `hashmap.c`

`hashmap.c` is not an independent generic diff engine.  Its old/new line mapping exists to feed the vertical-motion optimizer, and its standalone historical test links `hardscroll.c`.

Therefore `hardscroll.c`, `hashmap.c`, their hash/old-line SCREEN state, updater call sites and save/restore dependencies should be removed as one conceptual cut.

Do not delete only the source files while leaving dead updater/state hooks.

## Intended updater after that cut

    newscr desired whole screen
        -> identify changed lines/regions against curscr
        -> position physical cursor
        -> set attributes
        -> emit replacement characters / retained simple line edits
        -> update curscr remembered physical state

The desired-screen/current-screen architecture remains central.

# Surviving execution path

## Output

    application edits WINDOW
        -> wnoutrefresh / wrefresh
        -> newscr desired whole-screen image
        -> doupdate / tty_update compares newscr with curscr
        -> lib_mvcur positions physical cursor
        -> attributes + character bytes are emitted through terminfo/tputs
        -> curscr remembers resulting physical state

## Input

    terminal bytes/events
        -> screen input fd / FIFO
        -> escape-sequence/key recognition
        -> wgetch / wget_wch / getch
        -> application character or KEY_* event

The core distinction is editing in-memory desired state versus realizing that state on the terminal.

# Current build/runtime receipts

## Linux/glibc

The `modern core` workflow on GitHub-hosted Ubuntu 24.04 x86-64/glibc configures/builds the wide-character `ncurses progs` source set and runs the focused PTY harness against the just-built library.

The harness covers ordinary and wide WINDOW output, refresh/doupdate emission, terminfo arrow-key decoding, ordinary/UTF-8 input, bounded timeout behavior, resize state and tty restoration.

## Android/Bionic ARM builds

At exact PR #3 head `839e98129b17d54e9442f6679bb3840328de5b49`, ARMv7a and AArch64 API-24 Bionic builds/link checks passed using Android NDK `27.3.13750724`.

Those are ARM build/link receipts, not ARM runtime receipts.

## Android x86_64 emulator runtime

At exact PR #4 head `27041bf6dbb79efed955e02e9a2442279f62984e`, run `35046978535` executed the checksummed PTY bundle under Bionic in an Android 14 x86_64 emulator and recorded `android_pty_runtime=pass`.

This is an emulator private-PTY receipt.  It is not physical-device or interactive-terminal acceptance.

See `RECEIPTS.md` and `BIONIC_VALIDATION.md` for the precise evidence boundary.

# Remaining suspicious/historical mechanisms

- hardware scrolling and insert/delete-line traffic optimization (`hardscroll.c`);
- `hashmap.c` and SCREEN hash/old-line state feeding hardscroll;
- save/restore-cursor dependency retained for that optimizer;
- public termcap-compatible API family;
- soft-label keys;
- screen dump/restore compatibility;
- legacy 8-bit coding mode;
- obsolete compatibility aliases/entry points;
- removed-library and old-platform branches in Autoconf/generated configure/Makefile templates;
- residual Win32/OS2 private-header declarations;
- serial/padding behavior in `tputs` itself, separate from the already-removed mvcur transmission-cost model.

Each future cut should record the old problem, why it is outside target, the surviving replacement if any, test/inspection evidence and useful upstream file/function names.
