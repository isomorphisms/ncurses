/****************************************************************************
 * Copyright 2018-2021,2022 Thomas E. Dickey                                *
 * Copyright 1998-2016,2017 Free Software Foundation, Inc.                  *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/*
 * Physical terminal cursor movement.
 *
 * The modern-core model is deliberately small:
 *
 *     desired physical position
 *          -> absolute cursor addressing, or ordinary relative motion
 *          -> choose the shorter emitted sequence
 *          -> emit it
 *
 * Upstream ncurses also models slow serial links and many historical cursor
 * tricks.  This archaeology branch does not.  See source-first-layout/history
 * for the full optimizer.
 */

#include <curses.priv.h>

#ifndef CUR
#define CUR SP_TERMTYPE
#endif

MODULE_ID("$Id: lib_mvcur.c modern-core $")

#undef NCURSES_OUTC_FUNC
#define NCURSES_OUTC_FUNC myOutCh

#define MOVE_BUFFER_SIZE 512

/*
 * Removed here: baud-rate and $<...> transmission-delay accounting.
 *
 * Upstream prices terminal operations in fractions of transmission time and
 * parses terminfo padding markers into that price.  On the pty/terminal-
 * emulator targets for this branch, the useful comparison is the number of
 * bytes which tputs will actually emit.  Keep the historical internal
 * _nc_msec_cost entry point because tty_update also asks it for relative
 * operation costs, but its unit here is emitted bytes rather than time.
 */
static int
emitted_sequence_cost(const char *sequence)
{
    const char *cursor;
    int cost = 0;

    if (sequence == 0)
        return INFINITY;

    cursor = sequence;
    while (*cursor != '\0') {
        if (cursor[0] == '$' && cursor[1] == '<') {
            const char *end = strchr(cursor + 2, '>');
            if (end != 0) {
                cursor = end + 1;
                continue;
            }
        }
        ++cost;
        ++cursor;
    }
    return cost;
}

NCURSES_EXPORT(int)
NCURSES_SP_NAME(_nc_msec_cost) (NCURSES_SP_DCLx const char *const cap,
                                int affcnt)
{
    (void) affcnt;
    return emitted_sequence_cost(cap);
}

#if NCURSES_SP_FUNCS
NCURSES_EXPORT(int)
_nc_msec_cost(const char *const cap, int affcnt)
{
    return NCURSES_SP_NAME(_nc_msec_cost) (CURRENT_SCREEN, cap, affcnt);
}
#endif

static int
normalized_cost(NCURSES_SP_DCLx const char *const cap, int affcnt)
{
    return NCURSES_SP_NAME(_nc_msec_cost) (NCURSES_SP_ARGx cap, affcnt);
}

#ifdef TRACE
static int
trace_cost_of(NCURSES_SP_DCLx const char *capname, const char *cap, int affcnt)
{
    int result = NCURSES_SP_NAME(_nc_msec_cost) (NCURSES_SP_ARGx cap, affcnt);
    TR(TRACE_CHARPUT | TRACE_MOVE,
       ("CostOf %s %d %s", capname, result, _nc_visbuf(cap)));
    return result;
}
#define CostOf(cap,affcnt) trace_cost_of(NCURSES_SP_ARGx #cap, cap, affcnt)

static int
trace_normalized_cost(NCURSES_SP_DCLx const char *capname,
                      const char *cap,
                      int affcnt)
{
    int result = normalized_cost(NCURSES_SP_ARGx cap, affcnt);
    TR(TRACE_CHARPUT | TRACE_MOVE,
       ("NormalizedCost %s %d %s", capname, result, _nc_visbuf(cap)));
    return result;
}
#define NormalizedCost(cap,affcnt) \
    trace_normalized_cost(NCURSES_SP_ARGx #cap, cap, affcnt)
#else
#define CostOf(cap,affcnt) \
    NCURSES_SP_NAME(_nc_msec_cost)(NCURSES_SP_ARGx cap, affcnt)
#define NormalizedCost(cap,affcnt) normalized_cost(NCURSES_SP_ARGx cap, affcnt)
#endif

static void
reset_scroll_region(NCURSES_SP_DCL0)
{
    if (change_scroll_region) {
        NCURSES_PUTP2("change_scroll_region",
                      TIPARM_2(change_scroll_region,
                               0,
                               screen_lines(SP_PARM) - 1));
    }
}

NCURSES_EXPORT(void)
NCURSES_SP_NAME(_nc_mvcur_resume) (NCURSES_SP_DCL0)
{
    if (!SP_PARM || !IsTermInfo(SP_PARM))
        return;

    if (enter_ca_mode)
        NCURSES_PUTP2("enter_ca_mode", enter_ca_mode);

    reset_scroll_region(NCURSES_SP_ARG);
    SP_PARM->_cursrow = SP_PARM->_curscol = -1;

    if (SP_PARM->_cursor != -1) {
        int cursor = SP_PARM->_cursor;
        SP_PARM->_cursor = -1;
        NCURSES_SP_NAME(curs_set) (NCURSES_SP_ARGx cursor);
    }
}

#if NCURSES_SP_FUNCS
NCURSES_EXPORT(void)
_nc_mvcur_resume(void)
{
    NCURSES_SP_NAME(_nc_mvcur_resume) (CURRENT_SCREEN);
}
#endif

NCURSES_EXPORT(void)
NCURSES_SP_NAME(_nc_mvcur_init) (NCURSES_SP_DCL0)
{
    /*
     * Cost is now emitted bytes.  Keeping this at one also lets the existing
     * tty_update character-count fields continue to use the same arithmetic.
     */
    SP_PARM->_char_padding = 1;

    SP_PARM->_cr_cost = CostOf(carriage_return, 0);

    /*
     * Removed here: home, cursor_to_ll, hard-tab/back-tab and memory-relative
     * cursor tactics.  The fields remain in SCREEN while other inherited code
     * is being pruned, but mvcur will not choose those strategies.
     */
    SP_PARM->_home_cost = INFINITY;
    SP_PARM->_ll_cost = INFINITY;
#if USE_HARD_TABS
    SP_PARM->_ht_cost = INFINITY;
    SP_PARM->_cbt_cost = INFINITY;
#endif

    SP_PARM->_cub1_cost = CostOf(cursor_left, 0);
    SP_PARM->_cuf1_cost = CostOf(cursor_right, 0);
    SP_PARM->_cud1_cost = CostOf(cursor_down, 0);
    SP_PARM->_cuu1_cost = CostOf(cursor_up, 0);

    /* tty_update still uses these insert/delete costs. */
    SP_PARM->_smir_cost = CostOf(enter_insert_mode, 0);
    SP_PARM->_rmir_cost = CostOf(exit_insert_mode, 0);
    SP_PARM->_ip_cost = insert_padding ? CostOf(insert_padding, 0) : 0;

    /*
     * Removed here: cursor_mem_address as a fallback for absolute addressing.
     * Modern terminals in this branch are expected to provide ordinary cup.
     */
    SP_PARM->_address_cursor = cursor_address;

    SP_PARM->_cup_cost = SP_PARM->_address_cursor
        ? CostOf(TIPARM_2(SP_PARM->_address_cursor, 23, 23), 1)
        : INFINITY;
    SP_PARM->_cub_cost = parm_left_cursor
        ? CostOf(TIPARM_1(parm_left_cursor, 23), 1)
        : INFINITY;
    SP_PARM->_cuf_cost = parm_right_cursor
        ? CostOf(TIPARM_1(parm_right_cursor, 23), 1)
        : INFINITY;
    SP_PARM->_cud_cost = parm_down_cursor
        ? CostOf(TIPARM_1(parm_down_cursor, 23), 1)
        : INFINITY;
    SP_PARM->_cuu_cost = parm_up_cursor
        ? CostOf(TIPARM_1(parm_up_cursor, 23), 1)
        : INFINITY;
    SP_PARM->_hpa_cost = column_address
        ? CostOf(TIPARM_1(column_address, 23), 1)
        : INFINITY;
    SP_PARM->_vpa_cost = row_address
        ? CostOf(TIPARM_1(row_address, 23), 1)
        : INFINITY;

    /* Costs below belong to the still-inherited line painter in tty_update. */
    SP_PARM->_ed_cost = NormalizedCost(clr_eos, 1);
    SP_PARM->_el_cost = NormalizedCost(clr_eol, 1);
    SP_PARM->_el1_cost = NormalizedCost(clr_bol, 1);
    SP_PARM->_dch1_cost = NormalizedCost(delete_character, 1);
    SP_PARM->_ich1_cost = NormalizedCost(insert_character, 1);

    if (back_color_erase)
        SP_PARM->_el_cost = 0;

    SP_PARM->_dch_cost = parm_dch
        ? NormalizedCost(TIPARM_1(parm_dch, 23), 1)
        : INFINITY;
    SP_PARM->_ich_cost = parm_ich
        ? NormalizedCost(TIPARM_1(parm_ich, 23), 1)
        : INFINITY;
    SP_PARM->_ech_cost = erase_chars
        ? NormalizedCost(TIPARM_1(erase_chars, 23), 1)
        : INFINITY;
    SP_PARM->_rep_cost = repeat_char
        ? NormalizedCost(TIPARM_2(repeat_char, ' ', 23), 1)
        : INFINITY;

    SP_PARM->_cup_ch_cost = SP_PARM->_address_cursor
        ? NormalizedCost(TIPARM_2(SP_PARM->_address_cursor, 23, 23), 1)
        : INFINITY;
    SP_PARM->_hpa_ch_cost = column_address
        ? NormalizedCost(TIPARM_1(column_address, 23), 1)
        : INFINITY;
    SP_PARM->_cuf_ch_cost = parm_right_cursor
        ? NormalizedCost(TIPARM_1(parm_right_cursor, 23), 1)
        : INFINITY;
    SP_PARM->_inline_cost = min(SP_PARM->_cup_ch_cost,
                                min(SP_PARM->_hpa_ch_cost,
                                    SP_PARM->_cuf_ch_cost));

    /*
     * Temporary while inherited hardware-scroll code remains: it may use the
     * terminal save/restore cursor pair.  Do not let a non-nestable smcup/sc
     * combination make that remaining path unsafe.  This block can disappear
     * with hardscroll.
     */
    if (save_cursor != 0
        && enter_ca_mode != 0
        && strstr(enter_ca_mode, save_cursor) != 0) {
        save_cursor = 0;
        restore_cursor = 0;
    }

    NCURSES_SP_NAME(_nc_mvcur_resume) (NCURSES_SP_ARG);
}

#if NCURSES_SP_FUNCS
NCURSES_EXPORT(void)
_nc_mvcur_init(void)
{
    NCURSES_SP_NAME(_nc_mvcur_init) (CURRENT_SCREEN);
}
#endif

NCURSES_EXPORT(void)
NCURSES_SP_NAME(_nc_mvcur_wrap) (NCURSES_SP_DCL0)
{
    if (!SP_PARM || !IsTermInfo(SP_PARM))
        return;

    TINFO_MVCUR(NCURSES_SP_ARGx -1,
                -1,
                screen_lines(SP_PARM) - 1,
                0);

    if (SP_PARM->_cursor != -1) {
        int cursor = SP_PARM->_cursor;
        NCURSES_SP_NAME(curs_set) (NCURSES_SP_ARGx 1);
        SP_PARM->_cursor = cursor;
    }

    if (exit_ca_mode)
        NCURSES_PUTP2("exit_ca_mode", exit_ca_mode);

    /* Reset the terminal/kernel column model before returning to the shell. */
    NCURSES_SP_NAME(_nc_outch) (NCURSES_SP_ARGx '\r');
}

#if NCURSES_SP_FUNCS
NCURSES_EXPORT(void)
_nc_mvcur_wrap(void)
{
    NCURSES_SP_NAME(_nc_mvcur_wrap) (CURRENT_SCREEN);
}
#endif

/*
 * Append count copies of one ordinary relative-motion sequence.
 */
static int
append_repeated_move(string_desc *target, const char *sequence, int count)
{
    int cost = 0;
    int one_cost;

    if (count <= 0)
        return 0;
    if (sequence == 0)
        return INFINITY;

    one_cost = emitted_sequence_cost(sequence);
    if (one_cost == INFINITY)
        return INFINITY;

    while (count-- > 0) {
        if (!_nc_safe_strcat(target, sequence))
            return INFINITY;
        cost += one_cost;
    }
    return cost;
}

/*
 * Append the shorter representation of one axis move: a parameterized
 * relative capability, or repeated one-cell moves.
 */
static int
append_axis_move(string_desc *target,
                 const char *parameterized,
                 const char *single_step,
                 int count)
{
    char parameter_buffer[MOVE_BUFFER_SIZE];
    char repeated_buffer[MOVE_BUFFER_SIZE];
    string_desc parameter_result;
    string_desc repeated_result;
    int parameter_cost = INFINITY;
    int repeated_cost = INFINITY;

    if (count <= 0)
        return 0;

    (void) _nc_str_init(&parameter_result,
                        parameter_buffer,
                        sizeof(parameter_buffer));
    if (parameterized != 0) {
        const char *expanded = TIPARM_1(parameterized, count);
        if (expanded != 0
            && _nc_safe_strcpy(&parameter_result, expanded))
            parameter_cost = emitted_sequence_cost(parameter_buffer);
    }

    (void) _nc_str_init(&repeated_result,
                        repeated_buffer,
                        sizeof(repeated_buffer));
    repeated_cost = append_repeated_move(&repeated_result, single_step, count);

    if (parameter_cost == INFINITY && repeated_cost == INFINITY)
        return INFINITY;

    if (parameter_cost <= repeated_cost) {
        if (!_nc_safe_strcat(target, parameter_buffer))
            return INFINITY;
        return parameter_cost;
    }

    if (!_nc_safe_strcat(target, repeated_buffer))
        return INFINITY;
    return repeated_cost;
}

/*
 * Build a plain relative route using only up/down/left/right motion.
 *
 * Removed here: hard tabs/back-tabs, row/column absolute sub-motions,
 * carriage-return shortcuts, text-overwrite-as-motion, cursor_home,
 * cursor_to_ll and auto-left-margin wrap tricks.
 */
static int
build_relative_move(string_desc *target,
                    int from_y,
                    int from_x,
                    int to_y,
                    int to_x)
{
    int vertical_cost;
    int horizontal_cost;

    if (to_y > from_y) {
        const char *one_down = cursor_down;

        /* A literal newline has output-mode semantics beyond cursor motion. */
        if (one_down != 0 && *one_down == '\n')
            one_down = 0;

        vertical_cost = append_axis_move(target,
                                         parm_down_cursor,
                                         one_down,
                                         to_y - from_y);
    } else {
        vertical_cost = append_axis_move(target,
                                         parm_up_cursor,
                                         cursor_up,
                                         from_y - to_y);
    }

    if (vertical_cost == INFINITY)
        return INFINITY;

    if (to_x > from_x) {
        horizontal_cost = append_axis_move(target,
                                           parm_right_cursor,
                                           cursor_right,
                                           to_x - from_x);
    } else {
        horizontal_cost = append_axis_move(target,
                                           parm_left_cursor,
                                           cursor_left,
                                           from_x - to_x);
    }

    if (horizontal_cost == INFINITY)
        return INFINITY;

    return vertical_cost + horizontal_cost;
}

static int
onscreen_mvcur(NCURSES_SP_DCLx
               int old_y,
               int old_x,
               int new_y,
               int new_x,
               NCURSES_SP_OUTC myOutCh)
{
    char absolute_buffer[MOVE_BUFFER_SIZE];
    char relative_buffer[MOVE_BUFFER_SIZE];
    string_desc absolute_result;
    string_desc relative_result;
    int absolute_cost = INFINITY;
    int relative_cost = INFINITY;
    const char *chosen = 0;

    (void) _nc_str_init(&absolute_result,
                        absolute_buffer,
                        sizeof(absolute_buffer));
    if (SP_PARM->_address_cursor != 0) {
        const char *expanded = TIPARM_2(SP_PARM->_address_cursor, new_y, new_x);
        if (expanded != 0
            && _nc_safe_strcpy(&absolute_result, expanded))
            absolute_cost = emitted_sequence_cost(absolute_buffer);
    }

    (void) _nc_str_init(&relative_result,
                        relative_buffer,
                        sizeof(relative_buffer));
    if (old_y >= 0 && old_x >= 0) {
        relative_cost = build_relative_move(&relative_result,
                                            old_y,
                                            old_x,
                                            new_y,
                                            new_x);
    }

#if defined(TRACE) || defined(NCURSES_TEST)
    if (!(_nc_optimize_enable & OPTIMIZE_MVCUR))
        relative_cost = INFINITY;
#endif

    if (absolute_cost <= relative_cost)
        chosen = absolute_buffer;
    else if (relative_cost != INFINITY)
        chosen = relative_buffer;

    if (chosen == 0)
        return ERR;

    TR(TRACE_MOVE,
       ("mvcur %s (%d,%d)->(%d,%d), %d bytes",
        chosen == absolute_buffer ? "absolute" : "relative",
        old_y,
        old_x,
        new_y,
        new_x,
        chosen == absolute_buffer ? absolute_cost : relative_cost));

    TPUTS_TRACE("mvcur");
    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx chosen, 1, myOutCh);
    SP_PARM->_cursrow = new_y;
    SP_PARM->_curscol = new_x;
    return OK;
}

static int
_nc_real_mvcur(NCURSES_SP_DCLx
               int old_y,
               int old_x,
               int new_y,
               int new_x,
               NCURSES_SP_OUTC myOutCh)
{
    NCURSES_CH_T old_attributes;
    int code;

    TR(TRACE_CALLS | TRACE_MOVE,
       (T_CALLED("_nc_real_mvcur(%p,%d,%d,%d,%d)"),
        (void *) SP_PARM,
        old_y,
        old_x,
        new_y,
        new_x));

    if (SP_PARM == 0) {
        code = ERR;
    } else if (old_y == new_y && old_x == new_x) {
        code = OK;
    } else {
        /* Normalize coordinates implied by terminal right-edge wrap. */
        if (new_x >= screen_columns(SP_PARM)) {
            new_y += new_x / screen_columns(SP_PARM);
            new_x %= screen_columns(SP_PARM);
        }

        /*
         * Local cursor sequences are safest in normal attributes; restore the
         * original attributes immediately after positioning.
         */
        old_attributes = SCREEN_ATTRS(SP_PARM);
        if ((AttrOf(old_attributes) & A_ALTCHARSET)
            || (AttrOf(old_attributes) && !move_standout_mode)) {
            VIDPUTS(SP_PARM, A_NORMAL, 0);
        }

        if (old_x >= screen_columns(SP_PARM)) {
            int lines_crossed = (old_x + 1) / screen_columns(SP_PARM);

            old_y += lines_crossed;
            if (old_y >= screen_lines(SP_PARM))
                lines_crossed -= old_y - screen_lines(SP_PARM) - 1;

            if (lines_crossed > 0) {
                if (carriage_return)
                    NCURSES_PUTP2("carriage_return", carriage_return);
                else
                    myOutCh(NCURSES_SP_ARGx '\r');
                old_x = 0;

                while (lines_crossed-- > 0) {
                    if (newline)
                        NCURSES_PUTP2("newline", newline);
                    else
                        myOutCh(NCURSES_SP_ARGx '\n');
                }
            }
        }

        if (old_y > screen_lines(SP_PARM) - 1)
            old_y = screen_lines(SP_PARM) - 1;
        if (new_y > screen_lines(SP_PARM) - 1)
            new_y = screen_lines(SP_PARM) - 1;

        code = onscreen_mvcur(NCURSES_SP_ARGx
                              old_y,
                              old_x,
                              new_y,
                              new_x,
                              myOutCh);

        if (!SameAttrOf(old_attributes, SCREEN_ATTRS(SP_PARM)))
            VIDPUTS(SP_PARM,
                    AttrOf(old_attributes),
                    GetPair(old_attributes));
    }

    returnCode(code);
}

NCURSES_EXPORT(int)
NCURSES_SP_NAME(_nc_mvcur) (NCURSES_SP_DCLx
                            int old_y,
                            int old_x,
                            int new_y,
                            int new_x)
{
    int result = _nc_real_mvcur(NCURSES_SP_ARGx
                                old_y,
                                old_x,
                                new_y,
                                new_x,
                                NCURSES_SP_NAME(_nc_outch));

    if ((SP_PARM != 0) && (SP_PARM->_endwin == ewInitial))
        NCURSES_SP_NAME(_nc_flush) (NCURSES_SP_ARG);
    return result;
}

#if NCURSES_SP_FUNCS
NCURSES_EXPORT(int)
_nc_mvcur(int old_y, int old_x, int new_y, int new_x)
{
    return NCURSES_SP_NAME(_nc_mvcur) (CURRENT_SCREEN,
                                       old_y,
                                       old_x,
                                       new_y,
                                       new_x);
}
#endif

#if defined(USE_TERM_DRIVER)
NCURSES_EXPORT(int)
TINFO_MVCUR(NCURSES_SP_DCLx int old_y, int old_x, int new_y, int new_x)
{
    int result = _nc_real_mvcur(NCURSES_SP_ARGx
                                old_y,
                                old_x,
                                new_y,
                                new_x,
                                NCURSES_SP_NAME(_nc_outch));

    if ((SP_PARM != 0) && (SP_PARM->_endwin == ewInitial))
        NCURSES_SP_NAME(_nc_flush) (NCURSES_SP_ARG);
    NCURSES_SP_NAME(_nc_flush) (NCURSES_SP_ARG);
    return result;
}
#else
NCURSES_EXPORT(int)
NCURSES_SP_NAME(mvcur) (NCURSES_SP_DCLx
                        int old_y,
                        int old_x,
                        int new_y,
                        int new_x)
{
    return _nc_real_mvcur(NCURSES_SP_ARGx
                          old_y,
                          old_x,
                          new_y,
                          new_x,
                          NCURSES_SP_NAME(_nc_putchar));
}

#if NCURSES_SP_FUNCS
NCURSES_EXPORT(int)
mvcur(int old_y, int old_x, int new_y, int new_x)
{
    return NCURSES_SP_NAME(mvcur) (CURRENT_SCREEN,
                                   old_y,
                                   old_x,
                                   new_y,
                                   new_x);
}
#endif
#endif

#if defined(TRACE) || defined(NCURSES_TEST)
NCURSES_EXPORT_VAR(int) _nc_optimize_enable = OPTIMIZE_ALL;
#endif
