# Pruning ncurses toward a readable modern core

This is an archaeology branch between upstream ncurses and a later clean-sheet icurses.  The method is subtraction: keep the modern terminal model visible, remove support for worlds outside the target, and record what each removed mechanism used to solve.

## Target

Keep ordinary modern Unix/POSIX terminals, especially Linux/glibc and Android/Bionic, ptys and terminal emulators, UTF-8/wide characters, WINDOW/subwindow/pad storage, colors and attributes, resizing, tty modes, keyboard escape-sequence decoding, ordinary mouse support for now, terminfo for now, and the desired-screen/current-screen update model.

Do not optimize this branch for Windows consoles, OS/2, proprietary/ancient Unix, serial-terminal traffic tricks, every historical compatibility API, alternate language bindings, or upstream packaging/release work.

## Evidence vocabulary

- **inspected**: source ownership/references were traced
- **built**: a named configuration actually compiled
- **tested**: named tests actually ran
- **device-tested**: execution happened on a physical target device

Do not infer Android device acceptance from source inspection or a host build.

## Phase 1 — separate non-core implementations

### Ada95 and C++ bindings

**Removed:** complete `_/Ada95` and `_/c++` trees plus their source-first top-level links.

**Original problem:** language-specific wrappers, examples and build machinery for Ada95 and C++ clients.

**Why outside target:** the object of study is the C implementation of WINDOW state, refresh, terminal painting, cursor movement and input decoding.

**Replacement:** none; the C API/implementation remains.

**Inspection:** both were independent top-level modules.  Their stale configure branches are retired during build-world narrowing.

**Archaeology:** history for `Ada95/`, `c++/`, their Makefiles and configure paths.

### form, menu and panel

**Removed:** `_/form`, `_/menu`, `_/panel` and their top-level links.

**Original problem:** higher-level field/form, menu-selection and overlapping-panel libraries layered on curses windows.

**Why outside target:** they consume curses rather than implement the terminal/screen core.

**Replacement:** ordinary WINDOW/subwindow/pad, attribute, input and refresh operations.

**Inspection:** each is an independent module; inherited `configure.in` still names `panel menu form` in `modules_to_build`, a build-world debt rather than a reason to retain the libraries.

**Archaeology:** history for `form/`, `menu/`, `panel/` and their build entries.

### Win32 console and MinGW implementation

**Removed:** `_/ncurses/win32con`, `ncurses/tinfo/lib_win32con.c`, `lib_win32util.c`, MinGW-specific `nc_mingw.h`/`ncurses_mingw.h`, `win32_curses.h`, and `README.MinGW`.

**Original problem:** drive a native Windows console instead of a Unix tty/pty and adapt compilation/runtime details to MinGW.

**Why outside target:** Linux/glibc and Android/Bionic use the Unix tty + escape-sequence path.

**Replacement:** the surviving Unix path is `tinfo_driver.c`, termios/tty handling, `tty_update`, `mvcur`, and terminal sequences selected through terminfo.

**Inspection:** runtime backend code was physically isolated. `ncurses/Makefile.in` still names `nc_win32.h` and a `win32con` source directory; that compatibility header and stale generated-build references are deliberately removed with the build-world pass rather than hidden in this source deletion.

**Archaeology:** history for `ncurses/win32con/*`, `ncurses/tinfo/lib_win32*.c`, `include/nc_*mingw.h`, `include/win32_curses.h`, `include/nc_win32.h`.

### OS/2 and EMX

**Removed:** `Makefile.os2`, `README.emx`, `misc/emx.src`, the REXX `.cmd` scripts, and OS/2 DLL `.def/.ref` export tables for ncurses/form/menu/panel.

**Original problem:** configure/build ncurses in EMX, manufacture OS/2 DLL/import libraries and ordinal export tables, install an EMX terminal database, and create an OS/2 binary distribution.

**Why outside target:** OS/2/EMX is outside the POSIX tty target and its export/REXX machinery has no role in the surviving runtime.

**Replacement:** none; only the POSIX compiler/libc/termios platform model is retained.

**Inspection:** `Makefile.os2` directly owned the removed EMX data, REXX scripts, `.def/.ref` tables and `os2dist` target.

**Archaeology:** history for `Makefile.os2`, `README.emx`, `misc/emx.src`, `misc/*.cmd`, `misc/{ncurses,form,menu,panel}.{def,ref}`.

### Phase-1 validation state

Source ownership was inspected before each deletion.  No build receipt is claimed yet: the inherited Autoconf/Makefile world still contains references to deleted targets.  Those references are now dead build branches and must be removed before the branch is called build-consistent.

## Phase 2 — packaging and release engineering

### Upstream/downstream distribution machinery

**Removed:** `_/package`, `_/test/package`, imported downstream patch directory `_/m`, release announcement templates/metadata (`ANNOUNCE`, `announce.html.in`, `MANIFEST`, `dist.mk`), and `test/make-tar.sh`.

**Original problem:** describe distro packages, carry downstream packaging recipes/patches, construct release archives and announcements, and exercise tarball packaging.

**Why outside target:** none of this participates in WINDOW state, terminfo, tty input/output, screen diffing or terminal painting.  The branch is a source archaeology tree, not an ncurses distribution factory.

**Replacement:** none. Repository history remains the source for upstream packaging archaeology.

**Inspection:** the removed trees/files are package specs, distro installers/recipes, imported patch payloads, release manifests/announcement material, and tarball tooling.  Core `COPYING`, `AUTHORS`, `NEWS`, source documentation and ordinary test programs remain.

**Archaeology:** history for `package/`, `test/package/`, `m/`, `ANNOUNCE`, `announce.html.in`, `MANIFEST`, `dist.mk`, `test/make-tar.sh`.

**Validation:** file-role inspection only; this deletion does not claim a build or runtime test.

## Surviving execution path

Output:

    application edits WINDOW
        -> wnoutrefresh / wrefresh
        -> newscr desired whole-screen image
        -> doupdate / tty_update compares newscr with curscr
        -> mvcur positions physical cursor
        -> attributes + character bytes are emitted
        -> curscr remembers the resulting physical state

Input:

    terminal bytes/events
        -> input buffering
        -> escape-sequence/key recognition
        -> wgetch/getch
        -> application character or key event

The important architectural distinction is between editing in-memory screen state and realizing that state on the terminal.  Pruning must not erase it just to reduce line count.

## Build-world work still pending

Narrow `configure`, `configure.in`, Makefile templates, headers and generated-build machinery to the modern POSIX target. Remove probes/options whose only purpose is deleted bindings/libraries, Windows/OS2, ancient proprietary Unix or obsolete compilers. Identify the eventual small build inputs: compiler, POSIX/libc, termios/tty, terminfo, wide characters and source lists.

## Later candidates requiring mechanism-level inspection

- termcap-compatible `tget*` surface
- soft-label keys
- screen dump/restore compatibility
- legacy coding modes
- compatibility aliases tied only to old curses/platforms
- hard-tab/back-tab cursor movement
- baud-rate and serial-padding cost accounting
- memory-relative cursor addressing
- `cursor_to_ll` and auto-left-margin tricks
- save/restore-cursor optimization tricks
- hardware scrolling and insert/delete-line traffic optimizations
- `hardscroll.c`
- `hashmap.c` line-shift recognition (keep/simplify if useful independently)

For each later cut, record the old problem, surviving replacement if any, inspection/test evidence, and useful upstream file/function names.
