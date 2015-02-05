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

#define CLEANMASK(m)	((m & ~0x80))

/* sane modifier names */
#define SUPER		XCB_MOD_MASK_4
#define ALT		XCB_MOD_MASK_1
#define CTRL		XCB_MOD_MASK_CONTROL
#define SHIFT		XCB_MOD_MASK_SHIFT

#include <xcb/xcb.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

enum { INACTIVE, ACTIVE };

/* global variables */
static xcb_connection_t		*conn;
static xcb_screen_t		*scr;
static xcb_window_t		 focuswin;

/* proto */
static void subscribe(xcb_window_t);
static void cleanup(void);
static  int deploy(void);
static void focus(xcb_window_t, int);

#include "config.h"

static void
cleanup(void)
{
	/* graceful exit */
	if (conn != NULL)
		xcb_disconnect(conn);
}

static int
deploy(void)
{
	/* init xcb and grab events */
	uint32_t values[2];
	int mask;

	if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL)))
		return -1;

	scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	focuswin = scr->root;

#ifdef ENABLE_MOUSE
	xcb_grab_button(conn, 0, scr->root, XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
			XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE, 1, MOD);

	xcb_grab_button(conn, 0, scr->root, XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
			XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE, 3, MOD);
#endif

	mask = XCB_CW_EVENT_MASK;
	values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
	xcb_change_window_attributes_checked(conn, scr->root, mask, values);

	xcb_flush(conn);

	return 0;
}

static void
focus(xcb_window_t win, int mode)
{
	uint32_t values[1];
#ifdef DOUBLE_BORDER
	short w, h, b, o;

	if (!win)
		return;

	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn,
			xcb_get_geometry(conn, win), NULL);

	if (geom == NULL)
		return;

	w = (short)geom->width;
	h = (short)geom->height;
	b = (unsigned short)geom->border_width;
	o = (unsigned short)OUTER;

	xcb_rectangle_t inner[] = {
		/* you're not supposed to understand this. */
		{     w,0,b-o     ,h+b-   o      },
		{     w+b   +o,  0,   b  -o,     h+         b  -  o},
		{     0,h   ,w+b  -o,b-   o      },
		{     0,h   +b+      o,   w+     b-         o, b -o},
		{     w+b+o,b        +h    +o,b,b}
	};

	xcb_rectangle_t outer[] = {
		{w + b - o, 0, o, h + b * 2},
		{w + b,     0, o, h + b * 2},
		{0, h + b - o, w + b * 2, o},
		{0, h + b,     w + b * 2, o},
		{1, 1, 1, 1}
	};

	xcb_pixmap_t pmap = xcb_generate_id(conn);
	xcb_create_pixmap(conn, scr->root_depth, pmap, win, geom->width
			+ (geom->border_width * 2),
			geom->height + (geom->border_width * 2));
	xcb_gcontext_t gc = xcb_generate_id(conn);
	xcb_create_gc(conn, gc, pmap, 0, NULL);

	values[0] = OUTERCOL;
	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, values);
	xcb_poly_fill_rectangle(conn, pmap, gc, 5, outer);

	values[0] = mode ? FOCUSCOL : UNFOCUSCOL;
	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, values);
	xcb_poly_fill_rectangle(conn, pmap, gc, 5, inner);

	values[0] = pmap;
	xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXMAP, values);

	xcb_free_pixmap(conn, pmap);
	xcb_free_gc(conn, gc);
#else
	values[0] = mode ? FOCUSCOL : UNFOCUSCOL;
	xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXEL, values);
#endif

	if (mode == ACTIVE) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
				win, XCB_CURRENT_TIME);
		if (win != focuswin) {
			focus(focuswin, INACTIVE);
			focuswin = win;
		}
	}
}

static void
subscribe(xcb_window_t win)
{
	uint32_t values[2];

	/* subscribe to events */
	values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
	values[1] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
	xcb_change_window_attributes(conn, win, XCB_CW_EVENT_MASK, values);

	/* border width */
	values[0] = BORDERWIDTH;
	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

static void
events_loop(void)
{
	xcb_generic_event_t *ev;
#ifdef ENABLE_MOUSE
	uint32_t values[3];
	xcb_get_geometry_reply_t *geom;
	xcb_window_t win = 0;
#endif

	/* loop */
	for (;;) {
		ev = xcb_wait_for_event(conn);

		if (!ev)
			errx(1, "xcb connection broken");

		switch (CLEANMASK(ev->response_type)) {

		case XCB_CREATE_NOTIFY: {
			xcb_create_notify_event_t *e;
			e = (xcb_create_notify_event_t *)ev;

			if (!e->override_redirect) {
				subscribe(e->window);
				focus(e->window, ACTIVE);
			}
		} break;

		case XCB_DESTROY_NOTIFY: {
			xcb_destroy_notify_event_t *e;
			e = (xcb_destroy_notify_event_t *)ev;

			xcb_kill_client(conn, e->window);
		} break;

#ifdef ENABLE_SLOPPY
		case XCB_ENTER_NOTIFY: {
			xcb_enter_notify_event_t *e;
			e = (xcb_enter_notify_event_t *)ev;
			focus(e->event, ACTIVE);
		} break;
#endif

		case XCB_MAP_NOTIFY: {
			xcb_map_notify_event_t *e;
			e = (xcb_map_notify_event_t *)ev;

			if (!e->override_redirect) {
				xcb_map_window(conn, e->window);
				focus(e->window, ACTIVE);
			}
		} break;

#ifdef ENABLE_MOUSE
		case XCB_BUTTON_PRESS: {
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
			if (e->detail == 1) {
				values[2] = 1;
				xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0,
					0, geom->width/2, geom->height/2);
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

		case XCB_MOTION_NOTIFY: {
			xcb_query_pointer_reply_t *pointer;
			pointer = xcb_query_pointer_reply(conn,
					xcb_query_pointer(conn, scr->root), 0);
			if (values[2] == 1) {
				geom = xcb_get_geometry_reply(conn,
					xcb_get_geometry(conn, win), NULL);
				if (!geom)
					break;

				values[0] = (pointer->root_x + geom->width / 2
					> scr->width_in_pixels
					- (BORDERWIDTH*2))
					? scr->width_in_pixels - geom->width
					- (BORDERWIDTH*2)
					: pointer->root_x - geom->width / 2;
				values[1] = (pointer->root_y + geom->height / 2
					> scr->height_in_pixels
					- (BORDERWIDTH*2))
					? (scr->height_in_pixels - geom->height
					- (BORDERWIDTH*2))
					: pointer->root_y - geom->height / 2;

				if (pointer->root_x < geom->width/2)
					values[0] = 0;
				if (pointer->root_y < geom->height/2)
					values[1] = 0;

				xcb_configure_window(conn, win,
					XCB_CONFIG_WINDOW_X
					| XCB_CONFIG_WINDOW_Y, values);
				xcb_flush(conn);
			} else if (values[2] == 3) {
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
#endif

		case XCB_CONFIGURE_NOTIFY: {
			xcb_configure_notify_event_t *e;
			e = (xcb_configure_notify_event_t *)ev;

			if (e->window != focuswin)
				focus(e->window, INACTIVE);

			focus(focuswin, ACTIVE);
		} break;

		}

		xcb_flush(conn);
		free(ev);
	}
}

int
main(void)
{
	/* graceful exit */
	atexit(cleanup);

	if (deploy() < 0)
		errx(EXIT_FAILURE, "error connecting to X");

	for (;;)
		events_loop();

	return EXIT_FAILURE;
}

/* vim: set noet sw=8 sts=8: */
