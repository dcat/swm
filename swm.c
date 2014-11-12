/**
 *      Copyright (c) 2014, Broseph <dcat (at) iotek (dot) org>
 *
 *      Permission to use, copy, modify, and/or distribute this software for any
 *      purpose with or without fee is hereby granted, provided that the above
 *      copyright notice and this permission notice appear in all copies.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *      WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *      ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *      WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *      ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *      OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

/* sane modifier names */
#define SUPER		XCB_MOD_MASK_4
#define ALT		XCB_MOD_MASK_1
#define CTRL		XCB_MOD_MASK_CONTROL
#define SHIFT		XCB_MOD_MASK_SHIFT

#define LENGTH(x)	(sizeof(x)/sizeof(*x))
#define CLEANMASK(m)	((m & ~0x80))

#include <err.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>


struct swm_keys_t {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*mfunc)(int8_t, int8_t);
	void (*func)();
	int x,y;
};

enum SWM_STATE {
	INACTIVE = 0,
	ACTIVE = 1,
	RESIZE,
};

/* global variables */
static xcb_connection_t		*conn;
static xcb_screen_t		*scr;
static xcb_drawable_t		focuswin;


/* proto */
static void move		(int8_t, int8_t);
static void resize		(int8_t, int8_t);
static void cleanup		(void);
static void killwin		(void);
static void nextwin		(void);
static void focus		(xcb_window_t, int);
static void center_pointer	(xcb_window_t);


#include "config.h"

/*
 * The functions below are clean and simple, they should be kept.
 * As a function matures and becomes clean, move them from the "messy"-section
 * to here.
 */

static void
cleanup (void) {
	/* graceful exit */
	if (conn)
		xcb_disconnect(conn);
}

static xcb_keysym_t
xcb_get_keysym (xcb_keysym_t keycode) {
	xcb_key_symbols_t *keysyms;
	xcb_keysym_t keysym;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return 0;

	keysym = xcb_key_symbols_get_keysym(keysyms, (unsigned char)keycode, 0);
	xcb_key_symbols_free(keysyms);

	return keysym;
}

static xcb_keycode_t *
xcb_get_keycode (xcb_keysym_t keysym) {
	xcb_key_symbols_t *keysyms;
	xcb_keycode_t *keycode;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return 0;

	keycode = xcb_key_symbols_get_keycode(keysyms, keysym);
	xcb_key_symbols_free(keysyms);

	return keycode;
}

/* clean END */



/*
 * Messy functions, these functions work, but are most likely ugly, and don't
 * follow standards.
 * Simplify, and quality check these functions, before moving them into the
 * clean section.
 */

static void
center_pointer (xcb_window_t win) {
	uint32_t values[1];
	xcb_get_geometry_reply_t *geom;
	geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, win), NULL);

	xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0, 0,
			(geom->width  + (BORDERWIDTH * 2)) / 2,
			(geom->height + (BORDERWIDTH * 2)) / 2);

	values[0] = XCB_STACK_MODE_ABOVE;
	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

static void
nextwin (void) {
	xcb_query_tree_reply_t *r;
	xcb_window_t *c,t = 0;
	xcb_get_window_attributes_cookie_t ac;
	xcb_get_window_attributes_reply_t *ar;

	r = xcb_query_tree_reply(conn, xcb_query_tree(conn, scr->root), 0);
	if (!r || !r->children_len)
		return;
	c = xcb_query_tree_children(r);

	for (unsigned int i=0; i < r->children_len; i++) {
		ac = xcb_get_window_attributes(conn, c[i]);
		ar = xcb_get_window_attributes_reply(conn, ac, NULL);

		if (ar)
			if (ar->map_state == XCB_MAP_STATE_VIEWABLE) {
				if (ar->override_redirect) break;
				if (r->children_len == 1) {
					t = c[i];
					break;
				}
				if (c[i] == focuswin) break;
				t = c[i];
			}
	}

	if (t) {
		focus(t, ACTIVE);
		center_pointer(t);
	}
	free(r);
}

static void
focus (xcb_window_t win, int mode) {
	uint32_t values[1];
	short w, h, b, o;

	if (!win)
		return;

	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn,
			xcb_get_geometry(conn, win), NULL);

	if (mode == RESIZE) {
		values[0] = RESIZE_COLOR;
		xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXEL,
				values);
		return;
	}

	if (!geom)
		return;

	w = (short)geom->width;
	h = (short)geom->height;
	b = (unsigned short)BORDERWIDTH;
	o = (unsigned short)OUTER;

	xcb_rectangle_t inner[] = {
		/* you're not supposed to understand this. */
		{     w,0,b-o     , h+    b-     o},
		{     w+b   +o,  0,   b  -o,     h+         b  -  o},
		{     0,h   ,w+b  -o,b-   o      },
		{     0,h   +b+      o,   w+     b-         o, b -o},
		{     w+b+o,b        +h    +o,b,b}
	};

	xcb_rectangle_t outer[] = {
		{w + b - o, 0, o, h + b * 2},
		{w + b,     0, o, h + b * 2},
		{0, h + b - o, w + b * 2, o},
		{1, 1, 1, 1}
	};

	xcb_pixmap_t pmap = xcb_generate_id(conn);
	xcb_create_pixmap(conn, scr->root_depth, pmap, win, geom->width
			+ (BORDERWIDTH * 2), geom->height + (BORDERWIDTH * 2));
	xcb_gcontext_t gc = xcb_generate_id(conn);
	xcb_create_gc(conn, gc, pmap, 0, NULL);

	values[0] = OUTER_COLOR;
	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, values);
	xcb_poly_fill_rectangle(conn, pmap, gc, 5, outer);

	values[0] = mode ? FOCUSCOL : UNFOCUSCOL;
	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, values);
	xcb_poly_fill_rectangle(conn, pmap, gc, 5, inner);

	values[0] = pmap;
	xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXMAP, values);

	if (mode) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
			win, XCB_CURRENT_TIME);
		if (win != focuswin) {
			focus(focuswin, INACTIVE);
			focuswin = win;
		}
	}

	xcb_free_pixmap(conn, pmap);
	xcb_free_gc(conn, gc);
}

static void
setup_win (xcb_window_t win) {
	uint32_t values[2];

	values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
	xcb_change_window_attributes_checked(conn, win, XCB_CW_EVENT_MASK,
			values);

	/* border width */
	values[0] = BORDERWIDTH;
	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

static void
move (int8_t x, int8_t y) {
	uint32_t values[2];
	int real;
	xcb_window_t win = focuswin;
	xcb_get_geometry_reply_t *geom;

	if (!win || win == scr->root)
		return;

	geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, win), NULL);
	if (!geom)
		return;

	values[0] = x ? geom->x + x : geom->x;
	values[1] = y ? geom->y + y : geom->y;

	if (x)
	{
		real = geom->width + (BORDERWIDTH * 2);
		if (geom->x + x < 1)
			values[0] = 0;
		if (geom->x + x > scr->width_in_pixels - real)
			values[0] = scr->width_in_pixels - real;
	}

	if (y)
	{
		real = geom->height + (BORDERWIDTH * 2);
		if (geom->y + y < 1)
			values[1] = 0;
		if (geom->y + y > scr->height_in_pixels - real)
			values[1] = scr->height_in_pixels - real;
	}

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			| XCB_CONFIG_WINDOW_Y, values);

	center_pointer(win);

	free(geom);
}

static void
resize (int8_t x, int8_t y) {
	uint32_t values[2];
	int real;
	xcb_get_geometry_reply_t *geom;
	xcb_drawable_t win = focuswin;

	if (!win || win == scr->root)
		return;
	geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, win), NULL);

	values[0] = x ? geom->width + x : geom->width;
	values[1] = y ? geom->height + y : geom->height;

	if (x)
	{
		real = geom->width + (BORDERWIDTH * 2);
		if (geom->x + real + x > scr->width_in_pixels)
			values[0] = scr->width_in_pixels - geom->x -
				(BORDERWIDTH * 2);
	}

	if (y)
	{
		real = geom->height + (BORDERWIDTH *2);
		if (geom->y + real + y > scr->height_in_pixels)
			values[1] = scr->height_in_pixels - geom->y -
				(BORDERWIDTH * 2);
	}

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_WIDTH
		| XCB_CONFIG_WINDOW_HEIGHT, values);

	focus(win, ACTIVE);
	center_pointer(win);

	free(geom);
}

static void
killwin (void) {
	if (!focuswin || focuswin == scr->root)
		return;
	xcb_kill_client(conn, focuswin);
	nextwin();
}

static void
grab_keys (void) {
	xcb_keycode_t *keycode;

	for (unsigned int i=0; i < LENGTH(keys); i++) {
		keycode = xcb_get_keycode(keys[i].keysym);
		for (unsigned int k=0; keycode[k] != XCB_NO_SYMBOL; k++) {
			xcb_grab_key(conn, 1, scr->root, keys[i].mod,
				keycode[k], XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);
		}
		free(keycode);
	}
}

int main (void) {
	uint32_t mask=0, values[3];
	xcb_window_t root;
	xcb_generic_event_t *ev;
	xcb_get_geometry_reply_t *geom;
	xcb_window_t win = 0;

	/* graceful exit */
	atexit(cleanup);

	if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL)))
		errx(1, "error connecting to xcb");

	scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	focuswin = root = scr->root;

	grab_keys();

	xcb_grab_button(conn, 0, scr->root, XCB_EVENT_MASK_BUTTON_PRESS | 
		XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, 
		XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE, 1, MOD);

	xcb_grab_button(conn, 0, scr->root, XCB_EVENT_MASK_BUTTON_PRESS | 
		XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, 
		XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE, 3, MOD);


	mask = XCB_CW_EVENT_MASK;
	values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
	xcb_change_window_attributes_checked(conn, root, mask, values);
	xcb_flush(conn);

	/* loop */
	for (;;) {
		ev = xcb_wait_for_event(conn);

		if (ev == NULL)
			errx(1, "xcb connection broken");

		switch (ev->response_type & ~0x80) {

		case XCB_CREATE_NOTIFY: {
			xcb_create_notify_event_t *e;
			e = (xcb_create_notify_event_t *)ev;

			if (!e->override_redirect) {
				setup_win(e->window);
				focus(e->window, ACTIVE);
			}
		} break;

		case XCB_DESTROY_NOTIFY: {
			xcb_destroy_notify_event_t *e;
			e = (xcb_destroy_notify_event_t *)ev;

			xcb_kill_client(conn, e->window);
		} break;

		case XCB_KEY_PRESS: {
			xcb_key_press_event_t *e;
			e = (xcb_key_press_event_t *)ev;

			xcb_keysym_t keysym = xcb_get_keysym(e->detail);

			for (unsigned int i=0; i < LENGTH(keys); i++) {
				if (keys[i].keysym == keysym
				&& CLEANMASK(keys[i].mod) == CLEANMASK(e->state)
				&& keys[i].mfunc) {
					keys[i].mfunc(keys[i].x, keys[i].y);
				}
				else if (keys[i].keysym == keysym
				&& CLEANMASK(keys[i].mod) == CLEANMASK(e->state)
				&& keys[i].func) {
					keys[i].func();
				}
			}
		} break;

		case XCB_ENTER_NOTIFY: {
			xcb_enter_notify_event_t *e;
			e = (xcb_enter_notify_event_t *)ev;

			focus(e->event, ACTIVE);
		} break;

		case XCB_MAP_NOTIFY: {
			xcb_map_notify_event_t *e;
			e = (xcb_map_notify_event_t *)ev;

			if (!e->override_redirect) {
				xcb_map_window(conn, e->window);
				xcb_set_input_focus(conn,
						XCB_INPUT_FOCUS_POINTER_ROOT,
						e->window, XCB_CURRENT_TIME);
			}
		} break;
		case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t *e;
			e = ( xcb_button_press_event_t *)ev;
			win = e->child; 
			if (!win || win == scr->root)
				break;
			values[0] = XCB_STACK_MODE_ABOVE;
			xcb_configure_window(conn, win,
					XCB_CONFIG_WINDOW_STACK_MODE, values);
			geom = xcb_get_geometry_reply(conn,
					xcb_get_geometry(conn, win), NULL);
			if (1 == e->detail) {
				values[2] = 1; 
				xcb_warp_pointer(conn, XCB_NONE, win,
					0, 0, 0, 0, 1, 1);
			} else {
				values[2] = 3; 
				xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0,
						0, geom->width, geom->height);
			}
			xcb_grab_pointer(conn, 0, scr->root,
				XCB_EVENT_MASK_BUTTON_RELEASE
				| XCB_EVENT_MASK_BUTTON_MOTION
				| XCB_EVENT_MASK_POINTER_MOTION_HINT,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
				scr->root, XCB_NONE, XCB_CURRENT_TIME);
			xcb_flush(conn);
		} break;

		case XCB_MOTION_NOTIFY:
		{
			xcb_query_pointer_reply_t *pointer;
			pointer = xcb_query_pointer_reply(conn,
					xcb_query_pointer(conn, scr->root), 0);
			if (values[2] == 1) {
				geom = xcb_get_geometry_reply(conn,
					xcb_get_geometry(conn, win), NULL);
				if (!geom) {
					break;
				}
				values[0] = (pointer->root_x + geom->width
					> scr->width_in_pixels
					- (BORDERWIDTH*2))
					? scr->width_in_pixels - geom->width
					- (BORDERWIDTH*2)
					: pointer->root_x;
				values[1] = (pointer->root_y + geom->height
					> scr->height_in_pixels
					- (BORDERWIDTH*2))
					? (scr->height_in_pixels - geom->height
					- (BORDERWIDTH*2))
					: pointer->root_y;
				xcb_configure_window(conn, win,
					XCB_CONFIG_WINDOW_X
					| XCB_CONFIG_WINDOW_Y, values);
				xcb_flush(conn);
			} else if (values[2] == 3) {
				focus(win, RESIZE);
				geom = xcb_get_geometry_reply(conn,
					xcb_get_geometry(conn, win), NULL);
				values[0] = pointer->root_x - geom->x;
				values[1] = pointer->root_y - geom->y;
				xcb_configure_window(conn, win,
					XCB_CONFIG_WINDOW_WIDTH
					| XCB_CONFIG_WINDOW_HEIGHT, values);
				xcb_flush(conn);
			}
		} break;

		case XCB_BUTTON_RELEASE:
			focus(win, ACTIVE);
			xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
		break;
		}

	xcb_flush(conn);
	free(ev);
	}
}

