# Pruning ncurses toward a readable modern core

This branch is an archaeology branch between upstream ncurses and a later clean-sheet icurses.  The goal is to learn the implementation by subtraction: keep the modern terminal model visible, remove support for worlds we are not targeting, and leave enough explanation that the next reader can tell what used to be here and why it disappeared.

## Target

Keep the implementation aimed at ordinary modern Unix/POSIX terminals, especially Linux/glibc and Android/Bionic, ordinary ptys and terminal emulators, UTF-8/wide characters, windows and pads, colors/attributes, resizing, tty modes, keyboard escape-sequence decoding, and terminfo while it remains useful evidence.

This branch is not trying to preserve Windows consoles, OS/2, old proprietary Unix systems, serial-terminal performance tricks, every historical curses compatibility surface, language bindings, or packaging/release machinery.

## How to remove things

Remove one conceptual family at a time.  Before deleting a mechanism, identify what problem it solved and whether that problem is still inside the target.  Important source files should get a short tombstone comment where deleting a block would otherwise make the surviving control flow mysterious.  Do not leave a comment for every deleted statement.

A tombstone should say, in ordinary language, what was removed, why ncurses had it, and why this branch no longer models that problem.  Upstream history remains available in `master` history and the public ncurses source, so tombstones should explain rather than preserve dead code in comments.

## Pruning log

### Phase 1: separate implementations and add-on libraries

Planned first cuts:

- Ada95 binding
- C++ binding
- form library
- menu library
- panel library
- Win32 console backend
- OS/2 build support
- MinGW-specific support that exists only for the removed Windows target

These are being removed before the central `base`, `tty`, `tinfo`, and `widechar` implementation is changed.  Their absence should make the remaining architecture easier to see without yet changing the core WINDOW -> desired screen -> physical terminal path.

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

The important distinction is between editing in-memory screen state and realizing it on the terminal.  Pruning must not erase that distinction merely to reduce line count.

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

For each later cut, record the old problem, the surviving replacement if any, and the evidence used to decide that the old mechanism is outside the target.
