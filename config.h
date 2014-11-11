#define MOD		SUPER		/* modifier (CTRL, ALT, SUPER, etc.) */
#define FOCUSCOL	0x18191A	/* focused   inner border color */
#define UNFOCUSCOL	0x141516	/* unfocused inner border color */
#define OUTER_COLOR	0x000000	/*  outer border color */
#define RESIZE_COLOR	0xC1C189	/* resize border color */
#define BORDERWIDTH	4		/*  full  border width */
#define INNER		3		/*  inner border width */
#define OUTER		1		/*  outer border width */


static struct swm_keys_t keys[] =
{
	/*   modifier          key        function    NULL      x    y       */
	{    MOD,              XK_h,      move,       NULL,    -50,  0       },
	{    MOD,              XK_n,      move,       NULL,      0,  50      },
	{    MOD,              XK_e,      move,       NULL,      0, -50      },
	{    MOD,              XK_i,      move,       NULL,     50,  0       },

	{    MOD|CTRL,         XK_h,      move,       NULL,     -1,  0       },
	{    MOD|CTRL,         XK_n,      move,       NULL,      0,  1       },
	{    MOD|CTRL,         XK_e,      move,       NULL,      0, -1       },
	{    MOD|CTRL,         XK_i,      move,       NULL,      1,  0       },

	{    MOD|SHIFT,        XK_h,      resize,     NULL,    -50,  0       },
	{    MOD|SHIFT,        XK_n,      resize,     NULL,      0,  50      },
	{    MOD|SHIFT,        XK_e,      resize,     NULL,      0, -50      },
	{    MOD|SHIFT,        XK_i,      resize,     NULL,     50,  0       },

	{    MOD|CTRL|SHIFT,   XK_h,      resize,     NULL,     -1,  0       },
	{    MOD|CTRL|SHIFT,   XK_n,      resize,     NULL,      0,  1       },
	{    MOD|CTRL|SHIFT,   XK_e,      resize,     NULL,      0, -1       },
	{    MOD|CTRL|SHIFT,   XK_i,      resize,     NULL,      1,  0       },

	/*   modifier          key        NULL        function               */
	{    MOD,              XK_q,      NULL,       killwin                },
	{    MOD,              XK_Tab,    NULL,       nextwin                },
	{    MOD|CTRL,         XK_q,      NULL,       cleanup                },
};
