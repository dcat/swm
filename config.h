#define MOD		SUPER
#define FOCUSCOL	0x18191A
#define UNFOCUSCOL	0x141516
#define OUTER_COLOR	0x000000
#define RESIZE_COLOR	0xC1C189
#define BORDERWIDTH	4
#define INNER		3
#define OUTER		1


static struct swm_keys_t keys[] =
{
	/*   modifier          key        function      arguments    */
	{    MOD,              XK_h,      move,       NULL,    -50,  0       },
	{    MOD,              XK_l,      move,       NULL,     50,  0       },
	{    MOD,              XK_j,      move,       NULL,      0,  50      },
	{    MOD,              XK_k,      move,       NULL,      0, -50      },

	{    MOD|CTRL,         XK_h,      move,       NULL,     -1,  0       },
	{    MOD|CTRL,         XK_l,      move,       NULL,      1,  0       },
	{    MOD|CTRL,         XK_j,      move,       NULL,      0,  1       },
	{    MOD|CTRL,         XK_k,      move,       NULL,      0, -1       },

	{    MOD|SHIFT,        XK_h,      resize,     NULL,    -50,  0       },
	{    MOD|SHIFT,        XK_l,      resize,     NULL,     50,  0       },
	{    MOD|SHIFT,        XK_j,      resize,     NULL,      0,  50      },
	{    MOD|SHIFT,        XK_k,      resize,     NULL,      0, -50      },

	{    MOD|CTRL|SHIFT,   XK_h,      resize,     NULL,     -1,  0       },
	{    MOD|CTRL|SHIFT,   XK_l,      resize,     NULL,      1,  0       },
	{    MOD|CTRL|SHIFT,   XK_j,      resize,     NULL,      0,  1       },
	{    MOD|CTRL|SHIFT,   XK_k,      resize,     NULL,      0, -1       },
	{    MOD,              XK_q,      NULL,       killwin                },
	{    MOD,              XK_Tab,    NULL,       nextwin                },
	{    MOD|CTRL,         XK_q,      NULL,       cleanup                },
};
