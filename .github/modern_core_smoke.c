/*
 * Linux/glibc host receipt for the modern-core archaeology branch.
 *
 * Exercise the path we actually want to preserve:
 *   WINDOW edit -> refresh/doupdate -> terminal output
 * and
 *   terminal key sequence -> wgetch -> KEY_* event.
 *
 * The test uses two ptys so terminal output can be captured independently
 * from bytes injected as terminal input.  It is not Android device evidence.
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
#include <unistd.h>

static int
fail(const char *message)
{
    fprintf(stderr, "modern-core smoke: %s\n", message);
    return EXIT_FAILURE;
}

static int
open_terminal_pair(int *master, FILE **slave_stream, const char *mode)
{
    int slave;
    struct winsize size;

    memset(&size, 0, sizeof(size));
    size.ws_row = 24;
    size.ws_col = 80;

    if (openpty(master, &slave, NULL, NULL, &size) < 0)
        return -1;

    *slave_stream = fdopen(slave, mode);
    if (*slave_stream == NULL) {
        close(slave);
        close(*master);
        return -1;
    }
    return 0;
}

static int
captured_output_contains(int master, const char *needle)
{
    char output[32768];
    size_t used = 0;
    ssize_t got;
    int flags;

    flags = fcntl(master, F_GETFL, 0);
    if (flags >= 0)
        (void) fcntl(master, F_SETFL, flags | O_NONBLOCK);

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
    return strstr(output, needle) != NULL;
}

int
main(void)
{
    int input_master = -1;
    int output_master = -1;
    FILE *input = NULL;
    FILE *output = NULL;
    SCREEN *screen = NULL;
    const char *cursor_up_key;
    cchar_t lambda;
    wchar_t lambda_text[2] = { L'λ', L'\0' };
    int first_key;
    int second_key;
    int result = EXIT_FAILURE;

    if (setlocale(LC_ALL, "") == NULL)
        return fail("could not establish locale");

    if (open_terminal_pair(&input_master, &input, "r") < 0)
        return fail("could not create input pty");
    if (open_terminal_pair(&output_master, &output, "w") < 0) {
        fclose(input);
        close(input_master);
        return fail("could not create output pty");
    }

    screen = newterm("xterm-256color", output, input);
    if (screen == NULL) {
        result = fail("newterm failed");
        goto done;
    }
    (void) set_term(screen);

    if (raw() == ERR || noecho() == ERR || keypad(stdscr, TRUE) == ERR) {
        result = fail("tty/input mode setup failed");
        goto done;
    }

    if (waddstr(stdscr, "modern-core ") == ERR) {
        result = fail("ordinary WINDOW write failed");
        goto done;
    }
    if (setcchar(&lambda, lambda_text, A_BOLD, 0, NULL) == ERR
        || wadd_wch(stdscr, &lambda) == ERR) {
        result = fail("wide-character WINDOW write failed");
        goto done;
    }
    if (wrefresh(stdscr) == ERR) {
        result = fail("refresh/doupdate path failed");
        goto done;
    }

    cursor_up_key = tigetstr("kcuu1");
    if (cursor_up_key == NULL || cursor_up_key == (char *) -1) {
        result = fail("xterm cursor-up key capability unavailable");
        goto done;
    }
    if (write(input_master, cursor_up_key, strlen(cursor_up_key))
            != (ssize_t) strlen(cursor_up_key)
        || write(input_master, "q", 1) != 1) {
        result = fail("could not inject terminal input");
        goto done;
    }

    first_key = wgetch(stdscr);
    second_key = wgetch(stdscr);
    if (first_key != KEY_UP || second_key != 'q') {
        result = fail("wgetch did not decode cursor-up plus ordinary input");
        goto done;
    }

    if (resizeterm(30, 100) == ERR || LINES != 30 || COLS != 100) {
        result = fail("resize state update failed");
        goto done;
    }

    if (endwin() == ERR) {
        result = fail("endwin failed");
        goto done;
    }
    fflush(output);

    if (!captured_output_contains(output_master, "modern-core")) {
        result = fail("terminal output did not contain painted WINDOW text");
        goto done;
    }

    result = EXIT_SUCCESS;

done:
    if (screen != NULL)
        delscreen(screen);
    if (input != NULL)
        fclose(input);
    if (output != NULL)
        fclose(output);
    if (input_master >= 0)
        close(input_master);
    if (output_master >= 0)
        close(output_master);
    return result;
}
