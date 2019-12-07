/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#include <tilck_gen_headers/config_modules.h>
#include <tilck/common/basic_defs.h>
#include <tilck/mods/tty.h>
#include <tilck/kernel/ringbuf.h>
#include <tilck/kernel/sync.h>
#include <tilck/kernel/term.h>
#include <tilck/kernel/fs/vfs.h>
#include <tilck/kernel/fs/devfs.h>
#include <tilck/kernel/kb.h>

#include <termios.h>      // system header
#include <linux/kd.h>     // system header

#include "term_int.h"

#define NPAR 16 /* maximum number of CSI parameters */
#define TTY_ATTR_BOLD             (1 << 0)
#define TTY_ATTR_REVERSE          (1 << 1)

struct twfilter_ctx_t {

   struct tty *t;

   char param_bytes[64];
   char interm_bytes[64];
   char tmpbuf[16];

   bool non_default_state;
   u8 pbc; /* param bytes count */
   u8 ibc; /* intermediate bytes count */
};

void tty_input_init(struct tty *t);
void tty_kb_buf_reset(struct tty *t);
void tty_reset_filter_ctx(struct tty *t);

enum kb_handler_action
tty_keypress_handler(struct kb_dev *, struct key_event ke);

enum term_fret
serial_tty_write_filter(u8 *c, u8 *color, struct term_action *a, void *ctx_arg);

void tty_update_special_ctrl_handlers(struct tty *t);
void tty_update_default_state_tables(struct tty *t);

ssize_t
tty_read_int(struct tty *t, struct devfs_handle *h, char *buf, size_t size);

ssize_t
tty_write_int(struct tty *t, struct devfs_handle *h, char *buf, size_t size);

int
tty_ioctl_int(struct tty *t, struct devfs_handle *h, uptr request, void *argp);

bool
tty_read_ready_int(struct tty *t, struct devfs_handle *h);

void init_ttyaux(void);
void tty_create_devfile_or_panic(const char *filename, u16 major, u16 minor);

typedef bool (*tty_ctrl_sig_func)(struct tty *);

struct tty {

   struct term *term_inst;
   struct tilck_term_info term_i;

   int minor;
   char dev_filename[16];

   /* tty input */
   struct ringbuf input_ringbuf;
   struct kcond input_cond;
   int end_line_delim_count;
   bool mediumraw_mode;

   char *input_buf;
   tty_ctrl_sig_func *special_ctrl_handlers;

#if MOD_console
   u16 saved_cur_row;
   u16 saved_cur_col;
   u32 attrs;

   u8 user_color;       /* color before attrs */
   u8 c_set;            /* 0 = G0, 1 = G1     */
   const s16 *c_sets_tables[2];
   struct twfilter_ctx_t filter_ctx;
#endif

   /* tty ioctl */
   struct termios c_term;
   u32 kd_gfx_mode;

   /* tty input & output */
   u8 curr_color; /* actual color after applying attrs */
   u16 serial_port_fwd;

#if MOD_console
   term_filter default_state_funcs[256];
#endif
};

extern const struct termios default_termios;
extern struct tty *ttys[128]; /* tty0 is not a real tty */
extern int tty_tasklet_runner;
extern const s16 tty_default_trans_table[256];
extern const s16 tty_gfx_trans_table[256];
