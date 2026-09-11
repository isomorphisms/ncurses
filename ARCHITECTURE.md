# Surviving modern-core architecture

This branch is still recognizably ncurses.  The point is to make the important execution path visible by removing unrelated and historical machinery, not to replace it with a new design yet.

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
        | choose changed regions
        | choose attributes
        | ask for physical cursor movement
        v
    lib_mvcur.c
        |
        | absolute cursor address OR ordinary relative movement
        | choose shorter emitted sequence
        v
    tputs / terminal capability expansion
        |
        v
    pty / modern terminal emulator

After successful output, `curscr` is ncurses' remembered image of what the physical terminal now contains.

The central architectural boundary is therefore:

    edit desired in-memory state
        !=
    realize desired state on the physical terminal

## Input: terminal bytes to application events

    pty / terminal emulator
        |
        v
    screen input fd
        |
        v
    input FIFO + trie/key-sequence matching
        |
        | keypad mode recognizes multi-byte escape sequences
        v
    wgetch / getch
        |
        v
    ordinary character, KEY_UP/KEY_F(...), mouse/resize event, etc.

Raw/cbreak/echo and timeout/blocking policy live around this path rather than inside the application's WINDOW cell model.

## Main surviving source areas

### `_/ncurses/base`

Generic WINDOW and screen operations.  Important first-tour files include:

- `lib_newwin.c` — allocate WINDOW cell storage
- `lib_addch.c` — mutate WINDOW cells and logical cursor
- `lib_refresh.c` — merge WINDOW changes into `newscr`
- `lib_getch.c` — input buffering and application-level key results
- `resizeterm.c` / `wresize.c` — resize screen/window state

See `_/ncurses/base/README` for the running-program tour rather than an alphabetical API list.

### `_/ncurses/widechar`

UTF-8/wide-character and multi-column-cell operations.  This branch deliberately retains this layer and builds with `--enable-widec`.

### `_/ncurses/tinfo`

Terminfo data, capability expansion, terminal setup and low-level output helpers.  Terminfo stays for now because it is both functional infrastructure and evidence of which terminal differences ncurses still models.

Historical termcap-compatible public interfaces and obsolete aliases still live here and remain an explicit pruning question; terminfo itself is not scheduled for wholesale deletion on this branch.

### `_/ncurses/tty`

Physical-terminal realization.

- `tty_update.c` — compare `newscr` with `curscr`, paint changed terminal state, and update remembered physical state
- `lib_mvcur.c` — move the physical cursor; now reduced to absolute addressing versus ordinary relative up/down/left/right
- `lib_vidattr.c` — map curses attributes/colors to terminal rendition capabilities
- `lib_twait.c` — tty waiting/polling used by input timing
- `lib_tstp.c` — Unix terminal suspend/resume behavior

`hardscroll.c` and `hashmap.c` are still present but are classified as one unfinished historical optimization family: line hashing feeds a hardware insert/delete-line/scroll pre-pass before ordinary painting.  They are not part of the intended final conceptual core.

## Cursor movement after pruning

The surviving `lib_mvcur.c` model is deliberately small:

    current physical position + required position
        |
        +-> expand absolute `cursor_address`
        |
        +-> build ordinary relative up/down/left/right route
        |
        v
    compare emitted byte counts
        |
        v
    emit shorter available route

Removed from that choice are baud-rate timing, serial padding costs, hard tabs/back-tabs, memory-relative addressing, home/lower-left shortcuts, carriage-return strategy selection, text-overwrite movement and auto-left-margin wrap tricks.

## Build inputs that actually matter to the current target

The Linux/glibc receipt demonstrates the current small conceptual requirements even though inherited Autoconf is still much larger:

- C compiler
- libc/POSIX interfaces
- termios/tty and pty behavior
- terminal size / SIGWINCH support
- terminfo source/data and capability expansion
- wide-character libc support including `wcwidth`
- the `ncurses` and `progs` source modules

Android/Bionic is a target constraint, but no Android build or physical-device acceptance has been run on this branch yet.

## Intentionally retained modern features

Do not prune simply to reduce line count:

- WINDOW storage
- subwindows and derived windows
- pads
- refresh / desired-screen versus physical-screen architecture
- keyboard escape-sequence recognition
- raw/cbreak/echo modes
- resize / SIGWINCH
- colors and attributes
- UTF-8/wide characters and column-width correctness
- cursor visibility
- ordinary mouse support for now
- terminfo for now

## Remaining archaeology questions

See `PRUNING.md` for evidence and exact removal history.  The largest remaining questions are:

1. remove `hardscroll.c` + `hashmap.c` and their updater/state hooks together;
2. decide the old public termcap-compatible API separately from terminfo parsing;
3. decide soft-label-key support;
4. remove legacy 8-bit coding mode if its remaining tty branch is unnecessary for UTF-8 terminals;
5. decide screen dump/restore compatibility;
6. remove dead old-platform branches from the inherited configure/Makefile machinery;
7. decide how much serial padding behavior belongs in `tputs` once the modern terminal model is explicit.
