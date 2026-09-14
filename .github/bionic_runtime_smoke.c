/*
 * Focused PTY smoke for the surviving modern curses path.
 * The executable reports its libc; execution context belongs in the receipt.
 * A private PTY does not establish an interactive terminal/device acceptance.
 */
#define _XOPEN_SOURCE 600
#define _XOPEN_SOURCE_EXTENDED 1

#include <curses.h>
#include <term.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#if defined(EXPECT_BIONIC) && !defined(__BIONIC__)
#error "The Android smoke must be compiled against Bionic, not host libc"
#endif

static int
fail(const char *message)
{
    fprintf(stderr, "modern-core smoke: %s\n", message);
    return EXIT_FAILURE;
}

static int
open_terminal_pair(int *master, FILE **input, FILE **output)
{
    int slave;
    int input_fd;
    struct winsize size;

    memset(&size, 0, sizeof(size));
    size.ws_row = 24;
    size.ws_col = 80;
    if (openpty(master, &slave, NULL, NULL, &size) < 0)
        return -1;
    input_fd = dup(slave);
    if (input_fd < 0) {
        close(slave);
        close(*master);
        return -1;
    }
    *input = fdopen(input_fd, "r");
    *output = fdopen(slave, "w");
    if (*input == NULL || *output == NULL) {
        if (*input != NULL)
            fclose(*input);
        else
            close(input_fd);
        if (*output != NULL)
            fclose(*output);
        else
            close(slave);
        close(*master);
        return -1;
    }
    return 0;
}

static int
captured_output_matches(int master)
{
    char output[32768];
    size_t used = 0;
    ssize_t got;
    int flags = fcntl(master, F_GETFL, 0);

    if (flags < 0 || fcntl(master, F_SETFL, flags | O_NONBLOCK) < 0)
        return 0;
    while (used + 1 < sizeof(output)) {
        got = read(master, output + used, sizeof(output) - used - 1);
        if (got > 0) {
            used += (size_t) got;
            continue;
        }
        if (got < 0 && errno == EINTR)
            continue;
        break;
    }
    output[used] = '\0';
    /* Check actual UTF-8 bytes, not just success from wadd_wch. */
    return strstr(output, "modern-core") != NULL
        && strstr(output, "\xce\xbb") != NULL;
}

int
main(void)
{
    int master = -1;
    FILE *input = NULL;
    FILE *output = NULL;
    SCREEN *screen = NULL;
    const char *cursor_up_key;
    const char *terminal = getenv("NCURSES_SMOKE_TERM");
    cchar_t lambda;
    wchar_t lambda_text[2] = { 0x03bb, 0 };
    wint_t wide_key = 0;
    int first_key;
    int second_key;
    int ended = 0;
    int result = EXIT_FAILURE;
    struct termios original, restored;
    struct timespec before, after;
    long elapsed_ms;

    /* Bound the whole test, including writes and initialization. */
    alarm(10);
    setvbuf(stdout, NULL, _IONBF, 0);
#ifdef __BIONIC__
    puts("runtime_libc=bionic");
#elif defined(__GLIBC__)
    puts("runtime_libc=glibc");
#else
    puts("runtime_libc=other");
#endif
    if (terminal == NULL || *terminal == '\0')
        terminal = "xterm-256color";
    if (setlocale(LC_ALL, "") == NULL)
        return fail("could not establish locale");
    if (open_terminal_pair(&master, &input, &output) < 0)
        return fail("could not create terminal pty");
    if (tcgetattr(fileno(output), &original) < 0) {
        result = fail("could not read original tty modes");
        goto done;
    }
    screen = newterm(terminal, output, input);
    if (screen == NULL) {
        result = fail("newterm failed (check TERMINFO fixture)");
        goto done;
    }
    (void) set_term(screen);
    if (raw() == ERR || noecho() == ERR || keypad(stdscr, TRUE) == ERR) {
        result = fail("tty/input mode setup failed");
        goto done;
    }
    wtimeout(stdscr, 1000);
    if (waddstr(stdscr, "modern-core ") == ERR) {
        result = fail("ordinary WINDOW write failed");
        goto done;
    }
    if (setcchar(&lambda, lambda_text, A_BOLD, 0, NULL) == ERR
        || wadd_wch(stdscr, &lambda) == ERR) {
        result = fail("wide-character WINDOW write failed");
        goto done;
    }
    if (wrefresh(stdscr) == ERR || fflush(output) != 0) {
        result = fail("refresh/doupdate path failed");
        goto done;
    }
    cursor_up_key = tigetstr("kcuu1");
    if (cursor_up_key == NULL || cursor_up_key == (char *) -1) {
        result = fail("cursor-up key capability unavailable");
        goto done;
    }
    if (write(master, cursor_up_key, strlen(cursor_up_key))
            != (ssize_t) strlen(cursor_up_key)
        || write(master, "q\xce\xbb", 3) != 3) {
        result = fail("could not inject terminal input");
        goto done;
    }
    first_key = wgetch(stdscr);
    second_key = wgetch(stdscr);
    if (first_key != KEY_UP || second_key != 'q') {
        result = fail("KEY_UP / ordinary key decoding failed");
        goto done;
    }
    if (wget_wch(stdscr, &wide_key) != OK || wide_key != 0x03bb) {
        result = fail("UTF-8 input did not decode to lambda");
        goto done;
    }
    wtimeout(stdscr, 50);
    if (clock_gettime(CLOCK_MONOTONIC, &before) < 0
        || wgetch(stdscr) != ERR
        || clock_gettime(CLOCK_MONOTONIC, &after) < 0) {
        result = fail("empty-input timeout failed");
        goto done;
    }
    elapsed_ms = (after.tv_sec - before.tv_sec) * 1000L
        + (after.tv_nsec - before.tv_nsec) / 1000000L;
    if (elapsed_ms < 10 || elapsed_ms > 2000) {
        result = fail("empty-input wait was immediate or excessive");
        goto done;
    }
    if (resizeterm(30, 100) == ERR || LINES != 30 || COLS != 100) {
        result = fail("explicit resize state update failed");
        goto done;
    }
    if (endwin() == ERR) {
        result = fail("endwin failed");
        goto done;
    }
    ended = 1;
    if (fflush(output) != 0 || !captured_output_matches(master)) {
        result = fail("terminal output missing ASCII or UTF-8 WINDOW text");
        goto done;
    }
    if (tcgetattr(fileno(output), &restored) < 0
        || original.c_iflag != restored.c_iflag
        || original.c_oflag != restored.c_oflag
        || original.c_cflag != restored.c_cflag
        || original.c_lflag != restored.c_lflag
        || memcmp(original.c_cc, restored.c_cc, sizeof(original.c_cc)) != 0) {
        result = fail("endwin did not restore tty modes");
        goto done;
    }
    puts("modern_core_pty=pass");
    result = EXIT_SUCCESS;
done:
    if (screen != NULL) {
        if (!ended)
            (void) endwin();
        delscreen(screen);
    }
    if (input != NULL)
        fclose(input);
    if (output != NULL)
        fclose(output);
    if (master >= 0)
        close(master);
    alarm(0);
    return result;
}
