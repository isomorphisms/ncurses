# Pruning ncurses toward a readable modern core

This branch is an archaeology branch between upstream ncurses and a later clean-sheet icurses. The goal is to learn the implementation by subtraction: keep the modern terminal model visible, remove support for worlds we are not targeting, and leave enough explanation that the next reader can tell what used to be here and why it disappeared.

## Target

Keep the implementation aimed at ordinary modern Unix/POSIX terminals, especially Linux/glibc and Android/Bionic, ordinary ptys and terminal emulators, UTF-8/wide characters, windows and pads, colors/attributes, resizing, tty modes, keyboard escape-sequence decoding, and terminfo while it remains useful evidence.

This branch is not trying to preserve Windows consoles, OS/2, old proprietary Unix systems, serial-terminal performance tricks, every historical curses compatibility surface, language bindings, or packaging/release machinery.

## How to remove things

Remove one conceptual family at a time. Before deleting a mechanism, identify what problem it solved and whether that problem is still inside the target. Important source files should get a short tombstone comment where deleting a block would otherwise make the surviving control flow mysterious. Do not leave a comment for every deleted statement.

A tombstone should say, in ordinary language, what was removed, why ncurses had it, and why this branch no longer models that problem. Upstream history remains available in the repository history and the public ncurses source, so tombstones should explain rather than preserve dead code in comments.

## Pruning log

### Phase 1a: language bindings removed

Removed the complete Ada95 and C++ binding/demo trees, together with their source-first top-level links.

What they solved: ncurses shipped language-specific wrappers, demos, and their own supporting build machinery so Ada and C++ programs could use the C library through those interfaces.

Why they are gone here: the object of this branch is the C implementation of the terminal/screen model itself. Neither binding explains how WINDOW state, screen updating, cursor motion, terminal capabilities, or keyboard decoding work.

Replacement: none. The C implementation is the object being studied. Historical binding sources remain recoverable from repository history and upstream ncurses.

Inspection: both bindings were separate top-level source trees. The inherited configure system still contains optional binding detection/rules; those are retired during the build-world pass rather than mixed into the source-tree deletion.

Archaeology: history for `Ada95/` and `c++/`, including their Makefiles/configure machinery and examples.

### Phase 1b: form, menu and panel libraries removed

Removed `_/form`, `_/menu`, `_/panel` and their source-first top-level links.

What they solved: these are higher-level libraries built on curses. Forms manages fields and validation, menu manages selectable menu items, and panel manages overlapping-window stacking.

Why they are gone here: they are clients/layers above the WINDOW, refresh, input and terminal-painting mechanisms being studied. Their implementations are not required by the central curses execution path.

Replacement: none. Applications can still use ordinary curses windows, subwindows, pads, attributes, input and refresh directly.

Inspection: each library is an independent top-level module. The inherited `configure.in` explicitly adds `panel menu form` to `modules_to_build`; that stale traversal is removed in the build cleanup pass rather than preserved as an abstraction for deleted libraries.

Archaeology: history for `form/`, `menu/`, `panel/`, their `Makefile.in` files, and the `modules_to_build` block in `configure.in`.

Validation: source-tree separation was inspected before deletion. No build receipt is claimed yet because the inherited generated build still names removed modules; Phase 1 is not build-consistent until those references are removed.

### Phase 1c: non-Unix backends still to remove

Next cuts:

- Win32 console backend
- OS/2 build support
- MinGW-specific support that exists only for the removed Windows target

These cuts happen before the central `base`, `tty`, `tinfo`, and `widechar` implementation is changed.

### Core path being preserved while pruning

Output:

    WINDOW edits
        -> wnoutrefresh / wrefresh
        -> newscr (desired whole screen)
        -> doupdate / tty_update
        -> mvcur + terminal attribute/text output
        -> physical terminal

Input:

    terminal input
        -> byte/event buffering
        -> escape-sequence recognition
        -> wgetch/getch
        -> application character/key event

The important distinction is between editing in-memory screen state and realizing it on the terminal. Pruning must not erase that distinction merely to reduce line count.

## Later candidates, not yet removed

These need mechanism-by-mechanism investigation rather than bulk deletion:

- termcap-compatible `tget*` API
- soft-label keys
- screen dump/restore compatibility
- legacy coding modes
- hard-tab/back-tab cursor movement
- baud-rate and terminal-padding cost accounting
- memory-relative cursor addressing
- `cursor_to_ll` and auto-left-margin movement tricks
- save/restore-cursor optimization tricks
- hardware scrolling and insert/delete-line optimizations
- `hashmap.c` line-shift recognition

For each later cut, record the old problem, the surviving replacement if any, the validation/inspection evidence, and useful upstream file/function names.
