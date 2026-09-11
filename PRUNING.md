# Pruning ncurses toward a readable modern core

This is an archaeology branch between upstream ncurses and a later clean-sheet icurses. The method is subtraction: keep the modern terminal model visible, remove support for worlds outside the target, and record what each removed mechanism used to solve.

## Target

Keep ordinary modern Unix/POSIX terminals, especially Linux/glibc and Android/Bionic, ptys and terminal emulators, UTF-8/wide characters, WINDOW/subwindow/pad storage, colors and attributes, resizing, tty modes, keyboard escape-sequence decoding, ordinary mouse support for now, terminfo for now, and the desired-screen/current-screen update model.

Do not optimize this branch for Windows consoles, OS/2, proprietary/ancient Unix, serial-terminal traffic tricks, every historical compatibility API, alternate language bindings, or upstream packaging/release work.

## Evidence vocabulary

- **inspected**: source ownership/references were traced
- **built**: a named configuration actually compiled
- **tested**: named tests actually ran
- **device-tested**: execution happened on a physical target device

Do not infer Android/Bionic or physical-device acceptance from Linux/glibc source, compile or pty evidence.

## Phase 1 — separate non-core implementations

### Ada95 and C++ bindings

**Removed:** complete `_/Ada95` and `_/c++` trees plus their source-first top-level links.

**Original problem:** language-specific wrappers, examples and build machinery for Ada95 and C++ clients.

**Why outside target:** the object of study is the C implementation of WINDOW state, refresh, terminal painting, cursor movement and input decoding.

**Replacement:** none; the C API/implementation remains.

**Inspection:** both were independent top-level modules. The modern receipt explicitly configures with Ada and the C++ binding disabled.

**Archaeology:** history for `Ada95/`, `c++/`, their Makefiles and configure paths.

### form, menu and panel

**Removed:** `_/form`, `_/menu`, `_/panel` and their top-level links.

**Original problem:** higher-level field/form, menu-selection and overlapping-panel libraries layered on curses windows.

**Why outside target:** they consume curses rather than implement the terminal/screen core.

**Replacement:** ordinary WINDOW/subwindow/pad, attribute, input and refresh operations.

**Inspection:** each is an independent module. Although `configure.in` still textually appends `panel menu form` to `modules_to_build`, `CF_SRC_MODULES` only admits a module when its source `modules` file exists. After deletion the Linux configure result is `src modules... ncurses progs`; no fake/no-op replacement directories are required.

**Archaeology:** history for `form/`, `menu/`, `panel/`, their `modules`/Makefile files and the `CF_SRC_MODULES` configure path.

### Win32 console and MinGW implementation

**Removed:** `_/ncurses/win32con`, `ncurses/tinfo/lib_win32con.c`, `lib_win32util.c`, MinGW-specific `nc_mingw.h`/`ncurses_mingw.h`, `win32_curses.h`, and `README.MinGW`.

**Original problem:** drive a native Windows console instead of a Unix tty/pty and adapt compilation/runtime details to MinGW.

**Why outside target:** Linux/glibc and Android/Bionic use the Unix tty + escape-sequence path.

**Replacement:** `tinfo_driver.c`, termios/tty handling, `tty_update`, `mvcur`, and terminal sequences selected through terminfo.

**Inspection:** runtime backend code was isolated. `ncurses/Makefile.in` still contains historical Win32 names such as `nc_win32.h`; those are build-world residue, not retained target support.

**Archaeology:** history for `ncurses/win32con/*`, `ncurses/tinfo/lib_win32*.c`, `include/nc_*mingw.h`, `include/win32_curses.h`, `include/nc_win32.h`.

### OS/2 and EMX

**Removed:** `Makefile.os2`, `README.emx`, `misc/emx.src`, REXX `.cmd` scripts, and OS/2 DLL `.def/.ref` export tables for ncurses/form/menu/panel.

**Original problem:** configure/build ncurses in EMX, manufacture OS/2 DLL/import libraries and ordinal export tables, install an EMX terminal database, and create an OS/2 binary distribution.

**Why outside target:** OS/2/EMX is outside the POSIX tty target and its export/REXX machinery has no role in the surviving runtime.

**Replacement:** none; only the POSIX compiler/libc/termios platform model is retained.

**Inspection:** `Makefile.os2` directly owned the removed EMX data, REXX scripts, `.def/.ref` tables and `os2dist` target.

**Archaeology:** history for `Makefile.os2`, `README.emx`, `misc/emx.src`, `misc/*.cmd`, `misc/{ncurses,form,menu,panel}.{def,ref}`.

## Phase 2 — packaging and release engineering

### Upstream/downstream distribution machinery

**Removed:** `_/package`, `_/test/package`, imported downstream patch directory `_/m`, release announcement/metadata (`ANNOUNCE`, `announce.html.in`, `MANIFEST`), the release/tarball body formerly in `dist.mk`, and `test/make-tar.sh`.

**Original problem:** describe distro packages, carry downstream recipes/patches, construct release archives/announcements, and test tarball packaging.

**Why outside target:** none participates in WINDOW state, terminfo, tty input/output, screen diffing or terminal painting.

**Replacement:** none for release engineering. Repository history remains available for packaging archaeology.

**Important retained build metadata:** the first Linux configure receipt failed immediately after deleting `dist.mk`. Upstream `configure` mines `NCURSES_MAJOR`, `NCURSES_MINOR` and `NCURSES_PATCH` from that file. Rather than restore release machinery, this branch restored a tiny `_/dist.mk` containing only those three version values. That dependency should eventually move into a smaller build definition.

**Inspection:** removed content is package specs, installers/recipes, patch payloads, release manifests/announcement material and tarball tooling. Core `COPYING`, `AUTHORS`, `NEWS`, source documentation and ordinary tests remain.

**Archaeology:** history for `package/`, `test/package/`, `m/`, `ANNOUNCE`, `announce.html.in`, `MANIFEST`, the old `dist.mk`, and `test/make-tar.sh`.

## Phase 3 — narrow the platform/build world

### Proprietary-platform capability tables

**Removed:** `include/Caps.aix4`, `Caps.hpux11`, `Caps.osf1r5`, and `Caps.uwin`.

**Original problem:** carry alternative terminfo capability-name/ordering tables chosen to match AIX 4, HP-UX 11, OSF/1 V5, and UWIN conventions rather than ncurses' ordinary capability tables.

**Why outside target:** those operating systems/environment are outside this branch. The modern core retains `include/Caps` and `include/Caps-ncurses`, plus `Caps.keys` while keyboard capability generation remains under study.

**Replacement:** the normal ncurses capability table path.

**Inspection:** these are parallel platform-specific variants in `include/`, not runtime implementations required by Linux/glibc or Android/Bionic.

**Archaeology:** history for `include/Caps.{aix4,hpux11,osf1r5,uwin}` and the configure `--with-caps` selection machinery.

### Build/configuration residue still present

The inherited active build still contains historical branches/probes including:

- Ada compiler/binding probing and substitutions even though Ada is disabled
- C++ binding configuration and old C++ compiler workarounds even though the binding is disabled
- MinGW and OS/2 branches in generated configuration
- Solaris/proprietary-platform compiler workarounds
- Win32 dependency names such as `nc_win32.h`
- generated `configure` carrying the same broad historical world as `configure.in`

They are not being counted as supported target environments. The glibc receipt establishes that the surviving source set can already be selected as `ncurses progs` without restoring deleted source trees.

## Phase 4 — compatibility API audit

Not yet removed merely because they look old. The audit has identified these as distinct families which still need call-site/test decisions:

- public termcap-compatible `tget*`/`tgoto` spelling versus the underlying terminfo parser/data support
- soft-label-key implementation family (`lib_slk*` and wide-character SLK support)
- screen dump/restore compatibility
- `use_legacy_coding()` and the 8-bit compatibility branch still consulted by `tty_update`
- compatibility aliases in `tinfo/obsolete.c`

`read_termcap.c` is not automatically equivalent to the old public `tget*` API: parsing historical termcap data and exporting old application spellings are separate questions. Do not delete the parser simply to make the public API smaller.

## Phase 5 — simplify `lib_mvcur.c`

### What upstream was doing

Before pruning, physical cursor movement compared six tactic families:

1. absolute `cursor_address` (`cup`)
2. ordinary/local movement
3. carriage return plus local movement
4. `cursor_home` plus local movement
5. `cursor_to_ll` plus local movement
6. an auto-left-margin trick which wrapped backward through the preceding line

The local-movement builder could also use hard tabs/back-tabs, row/column sub-addressing, parameterized or one-cell moves, and even overwrite already-desired text as a cheaper way to advance the cursor. Cost was expressed in transmission time: `_char_padding` was derived from tty baud rate, comments were tuned around a 90 MHz Pentium at 9.6 Kbps, and `$<...>` terminfo padding delays contributed to the score.

### What was removed

- baud-rate-derived movement pricing
- `$<...>` padding-delay contribution to movement pricing
- hard tab/back-tab movement
- `cursor_mem_address` fallback for absolute screen positioning
- row-address/column-address sub-motions inside the relative route
- carriage-return shortcut tactic
- `cursor_home` tactic
- `cursor_to_ll` tactic
- text-overwrite-as-cursor-motion tactic
- auto-left-margin backward-wrap tactic
- the embedded historical optimizer/timing test program in `lib_mvcur.c`

### Surviving cursor algorithm

    old physical cursor + desired physical cursor
        -> construct ordinary relative up/down/left/right sequence when possible
        -> construct absolute cursor-address sequence when possible
        -> compare actual bytes tputs would emit (padding markers excluded)
        -> emit the shorter sequence
        -> remember the new physical cursor position

Parameterized relative up/down/left/right remains because it is ordinary modern terminal motion, not a historical hardware trick. One-cell relative capabilities remain as the alternate spelling. A literal newline is not treated as a generic down-one capability because newline carries output-mode semantics beyond cursor movement.

The historical internal `_nc_msec_cost` name remains temporarily because the inherited line painter also calls it; on this branch its value is an emitted-byte count rather than elapsed transmission time. Screen-update costs are still initialized for `tty_update`.

### Temporary dependency left deliberately

The save/restore-cursor suppression block remains only because inherited hardware-scroll code can still use save/restore cursor while `enter_ca_mode` may contain the same non-nestable capability. Remove that block with hardware scrolling rather than before it.

**Archaeology:** source-first history for `ncurses/tty/lib_mvcur.c`, especially `relative_move`, `onscreen_mvcur`, `_nc_msec_cost`, `_nc_mvcur_init`, and the former `MAIN`/`NCURSES_TEST` tester.

**Validation:** after the rewrite, the Linux/glibc wide-character configure step and full `make -j2` core/progs build succeeded at exact head `205c11841889fedfaf80b85309bd3c2105027ff0`. Runtime pty smoke evidence is recorded separately below when green.

## Phase 6 — tty screen updater audit

### `hardscroll.c`

`hardscroll.c` explicitly describes itself as a first-stage hardware-scrolling optimizer. It recognizes vertical line movement and emits insert-line/delete-line/scroll operations so the later `doupdate` pass has fewer line transformations to paint. Its motivating common case is screen-oriented editors on terminals where traffic is expensive.

For this branch that is a deletion candidate: the desired modern conceptual core can repaint changed regions without first transforming the physical terminal by hardware line shifts.

### `hashmap.c`

`hashmap.c` is not an independent whole-screen diff engine. Its own documentation says its old/new line mapping is for the “vertical-motion optimizer” and directs the reader to `hardscroll.c`; its standalone test also links `hardscroll.c`. Therefore if hardware scrolling is removed, `hashmap.c` should be removed with it rather than retained as mysterious generic hashing.

### Surviving updater after that planned cut

The intended remaining path is:

    newscr desired whole screen
        -> compare each changed line/region with curscr
        -> position physical cursor
        -> set attributes
        -> emit replacement characters / simple line edits
        -> update curscr remembered physical state

The actual `tty_update.c` call site and associated SCREEN hash/old-line state have not yet been removed. Until that cut lands, hard scrolling and hashmap remain explicitly listed as unfinished archaeology, not as desired modern features.

## Surviving execution path

Output:

    application edits WINDOW
        -> wnoutrefresh / wrefresh
        -> newscr desired whole-screen image
        -> doupdate / tty_update compares newscr with curscr
        -> mvcur positions physical cursor
        -> attributes + character bytes are emitted
        -> curscr remembers resulting physical state

Input:

    terminal bytes/events
        -> input buffering
        -> escape-sequence/key recognition
        -> wgetch/getch
        -> application character or key event

The central distinction is between editing in-memory screen state and realizing it on the terminal.

## Build and test receipts

### Linux/glibc wide build

Configuration used by `.github/workflows/modern-core.yml`:

    ./configure \
        --without-ada \
        --without-cxx \
        --without-cxx-binding \
        --without-debug \
        --without-manpages \
        --without-tests \
        --enable-widec

Observed on GitHub Ubuntu 24.04 x86-64/glibc with GCC 13.3:

- configure succeeded
- wide-character/wcwidth probes succeeded
- selected source modules were `ncurses progs`
- `make -j2` built the wide static curses library and ordinary programs successfully
- after the `lib_mvcur.c` rewrite, that configure/build still succeeded at exact head `205c11841889fedfaf80b85309bd3c2105027ff0`

### Retired broad internal test target

An earlier receipt ran `make -C ncurses test_progs`. Normal configure/build succeeded, but the target failed while linking the embedded `lib_mvcur` optimizer tester: that source deliberately supplied its own `tputs`, `putp`, `_nc_outch` and `delay_output` while also linking the complete library containing the real definitions. This was a test-harness/link collision, not a normal library build failure. The embedded historical optimizer tester has now been removed with the optimizer it measured.

### Focused modern-core pty smoke test

`.github/modern_core_smoke.c` is the replacement focused receipt. On a Linux host it uses real ptys and is intended to exercise:

- ordinary WINDOW text
- a wide character
- refresh/doupdate terminal output captured from a pty
- terminfo-derived arrow-key bytes decoded by `wgetch`
- ordinary character input
- resize state update

The test uses bounded input waits so a key-decoding problem fails instead of hanging CI.

### Android/Bionic

No Android/Bionic compilation or physical-device execution has been run in this branch yet. Bionic remains an explicit source target, not an accepted device target.

## Remaining suspicious/historical mechanisms

- hardware scrolling and insert/delete-line traffic optimization in `hardscroll.c`
- `hashmap.c` and SCREEN hash/old-line state feeding hardscroll
- save/restore-cursor scroll optimization dependency
- termcap-compatible public API family
- soft-label keys
- screen dump/restore compatibility
- legacy 8-bit coding mode
- old compatibility aliases/obsolete terminfo entry points
- remaining old-platform branches in Autoconf/generated configure/Makefile templates
- Win32-only residue such as `nc_win32.h` if no surviving POSIX build dependency remains
- serial/padding behavior in `tputs` itself (separate from the removed mvcur cost model)

Each future cut should continue to record the old problem, surviving replacement if any, inspection/test evidence, and useful upstream file/function names.
