# Build and test receipts

These receipts distinguish what actually ran from target claims.  In particular, Linux/glibc evidence is not Android/Bionic evidence.

## Linux/glibc wide-character core

A GitHub Ubuntu 24.04 x86-64/glibc runner configured the inherited build from `_` with:

```sh
./configure \
    --without-ada \
    --without-cxx \
    --without-cxx-binding \
    --without-debug \
    --without-manpages \
    --without-tests \
    --enable-widec
```

The configuration selected the surviving `ncurses progs` source modules.  `make -j2` built the wide-character curses core and ordinary programs successfully.

The source-manifest cleanup commit `24b029af76d8bb0557e6736b3de8da3b0d9b10ef`, which removes the dead Win32 source groups after their implementations were deleted, passed the complete `modern core` workflow.  Therefore the manifest cleanup did not break the supported Linux/glibc source selection.

## Focused pty execution

`.github/modern_core_smoke.c` runs against the just-built library on a real pseudo-terminal.  It exercises:

- `newterm("xterm-256color", ...)` on a pty;
- raw/noecho/keypad input setup;
- ordinary WINDOW text output;
- a wide `λ` written through `setcchar`/`wadd_wch`;
- `wrefresh`, therefore the WINDOW -> desired screen -> `doupdate` terminal-output path;
- terminfo lookup of the cursor-up key sequence;
- injection of that sequence plus `q` through the pty master;
- `wgetch` decoding to `KEY_UP` and then ordinary character `q`;
- `resizeterm(30, 100)` and the resulting `LINES`/`COLS` state;
- capture of emitted terminal output containing the WINDOW text.

The one-pty version first passed at exact commit `fa011bb00cf2ca73066d1923c3426ff869a282d6`.  The same smoke remained green after the architecture documentation and after the dead Win32 source-manifest entries were removed; the pull-request workflow for `24b029af76d8bb0557e6736b3de8da3b0d9b10ef` completed successfully.

The input waits are bounded so failure to decode a terminal sequence returns a test failure rather than hanging CI.

## Earlier failures retained as evidence

The first build attempt failed before configuration because upstream `configure` mines `NCURSES_MAJOR`, `NCURSES_MINOR`, and `NCURSES_PATCH` from `dist.mk`.  The release/distribution body of `dist.mk` remains deleted; the branch restored only those three version values as temporary build metadata.

An earlier attempt to use `make -C ncurses test_progs` failed while linking ncurses' embedded historical `lib_mvcur` optimizer tester.  That tester deliberately defined its own `tputs`, `putp`, `_nc_outch`, and `delay_output` while the target also linked the complete library containing the real definitions.  This was a test-harness linker collision, not a normal library build failure.  The embedded tester was subsequently removed with the historical cursor optimizer it measured.

The first focused pty test incorrectly used separate ptys for input and output.  ncurses initialized terminal modes on the output terminal, leaving the unrelated input pty with unsuitable line discipline; bounded `wgetch` calls returned `ERR`.  The corrected test uses duplicate slave descriptors from one pty, matching an ordinary terminal, and is green.

## Android/Bionic

No Android/Bionic compilation and no physical Android device execution has been run for this branch.  Android/Bionic remains an explicit compatibility target, but there is no Android acceptance receipt here.
