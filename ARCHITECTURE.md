# Surviving modern-core architecture

This branch is still recognizably descended from ncurses.  The goal is to expose the modern curses execution path by subtraction, not to replace it with clean-sheet icurses yet.

## Output: application state to terminal bytes

    application
        |
        | edits cells and logical cursor
        v
    WINDOW / subwindow / pad
        |
        | wnoutrefresh / pnoutrefresh
        v
    newscr
    desired complete terminal image
        |
        | doupdate
        v
    tty_update.c
        |
        | compare desired cells/lines with curscr
        | choose changed regions and attributes
        | request physical cursor movement
        v
    lib_mvcur.c
        |
        | absolute cursor address OR ordinary relative movement
        | choose shorter emitted sequence
        v
    tputs / terminfo capability expansion
        |
        v
    pty / modern terminal emulator

After successful painting, `curscr` is ncurses' remembered image of the physical terminal.

The central boundary is:

    edit desired in-memory state
        !=
    realize desired state on the terminal

## Input: terminal bytes to application events

    pty / terminal emulator
        |
        v
    screen input fd
        |
        v
    input FIFO + key-sequence trie
        |
        | keypad mode recognizes multi-byte escape sequences
        v
    wgetch / wget_wch / getch
        |
        v
    character, KEY_* event, resize event, mouse event, ...

Raw/cbreak/echo and timeout/blocking policy surround this path rather than belonging to the WINDOW cell model.

## Main surviving source areas

### `_/ncurses/base`

Generic WINDOW and screen operations.  First-tour files include:

- `lib_newwin.c` — allocate WINDOW cell storage;
- `lib_addch.c` — mutate WINDOW cells and logical cursor;
- `lib_refresh.c` — merge WINDOW changes into `newscr`;
- `lib_getch.c` — buffered terminal input and key recognition;
- `resizeterm.c` / `wresize.c` — screen/window resize state.

See `_/ncurses/base/README` for the running-program tour.

### `_/ncurses/widechar`

Wide-character and multi-column-cell operations.  UTF-8/wide-character behavior is a retained core requirement, not a compatibility feature to prune away.

### `_/ncurses/tinfo`

Terminfo data, terminal setup, parameter expansion and low-level terminal-output helpers.  Terminfo remains because it is both functional infrastructure and useful evidence of the terminal differences ncurses models.

The old public termcap-compatible API is a separate pruning question from the terminfo parser/database itself.

### `_/ncurses/tty`

Physical-terminal realization:

- `tty_update.c` — compare `newscr` with `curscr`, paint changed terminal state, update remembered state;
- `lib_mvcur.c` — physical cursor movement, now reduced to absolute addressing versus ordinary relative up/down/left/right;
- `lib_vidattr.c` — terminal rendition for curses attributes/colors;
- `lib_twait.c` — poll/wait behavior used by input timing;
- `lib_tstp.c` — Unix terminal suspend/resume behavior.

`hardscroll.c` and `hashmap.c` still form one unfinished historical optimization family.  Hashing recognizes vertically shifted lines for a hardware insert/delete-line/scroll pre-pass before ordinary painting.  They should be removed together with their `tty_update` and SCREEN-state hooks, not piecemeal.

## Cursor movement after pruning

The surviving `lib_mvcur.c` model is deliberately small:

    current physical position + required position
        |
        +-> expand absolute cursor_address
        |
        +-> build ordinary relative up/down/left/right route
        |
        v
    compare emitted byte counts
        |
        v
    emit shorter available route

Removed from that choice are baud-rate timing, serial-padding cost accounting, hard tabs/back-tabs, memory-relative addressing, home/lower-left shortcuts, carriage-return tactics, text-overwrite movement and auto-left-margin wrap tricks.

## What current receipts establish

### Linux/glibc

The wide-character core builds and the focused PTY harness runs on GitHub-hosted Ubuntu/glibc.  The harness exercises WINDOW output, wide UTF-8 output, refresh/doupdate emission, ordinary input, terminfo arrow-key decoding, bounded timeout behavior and resize state.

### Android/Bionic ARM builds

The same pruned source cross-builds and explicitly links the PTY harness for Android/Bionic API 24 on ARMv7a and AArch64 using NDK `27.3.13750724`.  ELF/provenance checks verify that these are target binaries linked against the just-built archive.

This establishes ARM **build/link compatibility**, not ARM Android runtime behavior.

### Android emulator runtime

A separate x86_64 bundle has run successfully under an Android 14 x86_64 emulator.  The runtime receipt confirms Bionic and a passing private-PTY harness.

This establishes runtime behavior only for that emulator context.  Physical devices, ARM runtime, interactive terminal applications, IME behavior and genuine externally delivered resize events remain unclaimed.

See `RECEIPTS.md` and `BIONIC_VALIDATION.md` for exact evidence boundaries.

## Build inputs that matter to the current target

The Linux/glibc and Android/Bionic receipts show the conceptual requirements even though inherited Autoconf is still much larger:

- C compiler;
- libc/POSIX interfaces;
- termios and PTY behavior;
- terminal size / SIGWINCH support;
- terminfo data and capability expansion;
- wide-character libc support including `wcwidth`;
- the `ncurses` and `progs` source modules.

The inherited configure/build machinery still contains branches for removed bindings, libraries and old platforms.  Those are residue, not supported architecture.

## Intentionally retained modern features

Do not prune these merely to reduce line count:

- WINDOW storage;
- subwindows and derived windows;
- pads;
- refresh and desired-screen/current-screen architecture;
- keyboard escape-sequence recognition;
- raw/cbreak/echo modes;
- resize / SIGWINCH;
- colors and attributes;
- UTF-8/wide characters and column-width correctness;
- cursor visibility;
- ordinary mouse support for now;
- terminfo for now.

## Remaining archaeology questions

The largest remaining cuts are:

1. remove `hardscroll.c` + `hashmap.c` and their updater/SCREEN-state hooks together;
2. decide the public termcap-compatible API separately from terminfo parsing;
3. decide soft-label-key support;
4. remove legacy 8-bit coding mode if unnecessary for the UTF-8 target;
5. decide screen dump/restore compatibility;
6. remove dead old-platform and removed-library branches from inherited configure/Makefile machinery;
7. decide how much serial padding behavior belongs in `tputs` itself;
8. remove residual Win32/OS2 private-header/build declarations once no selected build path references them.

`PRUNING.md` is the removal ledger; this file is the current conceptual map.
