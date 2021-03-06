#if 0
static const char sccsid[] = "@(#)b_lockglue.c  4.11 98/06/16 xlockmore";
#endif

/*-
 * BUBBLE3D (C) 1998 Richard W.M. Jones.
 * b_lockglue.c: Glue to make this all work with xlockmore.
 */

#include "bubble3d.h"

/* XXX This lot should eventually be made configurable using the
 * options stuff below.
 */
struct glb_config glb_config =
{
        0,			/* transparent_p */
#if GLB_SLOW_GL
	2,			/* subdivision_depth */
#else
	3,			/* subdivision_depth */
#endif
	5,			/* nr_nudge_axes */
	0.01,			/* nudge_angle_factor */
	0.20,			/* nudge_factor */
	0.1,			/* rotation_factor */
	8,			/* create_bubbles_every */
	8,			/* max_bubbles */
	{0.7, 0.8, 0.9, 1.0},	/* p_bubble_group */
	0.5,			/* max_size */
	0.1,			/* min_size */
	0.03,			/* max_speed */
	0.005,			/* min_speed */
	1.5,			/* scale_factor */
	-4,			/* screen_bottom */
	4,			/* screen_top */
#if 0
	{0.1, 0.0, 0.4, 0.0},	/* bg_colour */
#else
	{0.0, 0.0, 0.0, 0.0},	/* bg_colour */
#endif
#if 0
	{0.7, 0.7, 0.0, 0.3}	/* bubble_colour */
#else
	{0.0, 0.0, 0.7, 0.3}	/* bubble_colour */
#endif
};

#ifdef STANDALONE
# define DEFAULTS	"*delay:	10000   \n"	\
			"*showFPS:      False   \n"

# define refresh_bubble3d 0
# define bubble3d_handle_event 0
#include "xlockmore.h"
#else
#include "xlock.h"
#include "vis.h"
#endif

#ifdef USE_GL


#define DEF_TRANSPARENT "True"
#define DEF_COLOR "random"

static Bool transparent_p;
static char *bubble_color_str;

#undef countof
#define countof(x) (sizeof((x))/sizeof((*x)))

static XrmOptionDescRec opts[] = {
  { "-transparent",  ".transparent",   XrmoptionNoArg, "True" },
  { "+transparent",  ".transparent",   XrmoptionNoArg, "False" },
  { "-color",    ".bubble3d.bubblecolor", XrmoptionSepArg, 0 },
};

static argtype vars[] = {
  {&transparent_p,   "transparent", "Transparent", DEF_TRANSPARENT, t_Bool},
  {&bubble_color_str,        "bubblecolor", "BubbleColor", DEF_COLOR, t_String},
};

ENTRYPOINT ModeSpecOpt bubble3d_opts = {countof(opts), opts, countof(vars), vars, NULL};

#ifdef USE_MODULES
ModStruct   bubbles3d_description =
{"bubbles3d",
 "init_bubble3d",
 "draw_bubble3d",
 "release_bubble3d",
 "change_bubble3d",
 "init_bubble3d",
 NULL,
 &bubble3d_opts,
 1000, 1, 2, 1, 64, 1.0, "",
 "Richard Jones's GL bubbles",
 0,
 NULL
};

#endif /* USE_MODULES */

struct context {
	GLXContext *glx_context;
	void       *draw_context;
};

static struct context *contexts = 0;

static void
parse_color (ModeInfo *mi, const char *name, const char *s, GLfloat *a)
{
  XColor c;

  if (! XParseColor (MI_DISPLAY(mi), MI_COLORMAP(mi), s, &c))
    {
      fprintf (stderr, "%s: can't parse %s color %s", progname, name, s);
      exit (1);
    }
  a[0] = c.red   / 65536.0;
  a[1] = c.green / 65536.0;
  a[2] = c.blue  / 65536.0;
}

static void
init_colors(ModeInfo *mi)
{
	if (strncasecmp(bubble_color_str, "auto", strlen("auto")) == 0) {
		glb_config.bubble_colour[0] = ((float) (NRAND(100)) / 100.0);
		glb_config.bubble_colour[1] = ((float) (NRAND(100)) / 100.0);
		/* I keep more blue */
		glb_config.bubble_colour[2] = ((float) (NRAND(50)) / 100.0) + 0.50;
	} else if (strncasecmp(bubble_color_str, "random", strlen("random")) == 0) {
	    glb_config.bubble_colour[0] = -1.0;
	} else {
		parse_color(mi, "bubble", bubble_color_str, glb_config.bubble_colour);
	}
}

static void
init(struct context *c)
{
        glb_config.transparent_p = transparent_p;
	glb_sphere_init();
	c->draw_context = glb_draw_init();
}

ENTRYPOINT void
reshape_bubble3d(ModeInfo *mi, int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (GLdouble) w / (GLdouble) h, 3, 8);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -5);
}

static void
do_display(struct context *c)
{
	glb_draw_step(c->draw_context);
}

ENTRYPOINT void
init_bubble3d(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         screen = MI_SCREEN(mi);
	struct context *c;

	if (contexts == 0) {
		contexts = (struct context *) calloc(sizeof (struct context), MI_NUM_SCREENS(mi));

		if (contexts == 0)
			return;
	}
	c = &contexts[screen];
	c->glx_context = init_GL(mi);
	init_colors(mi);
	if (c->glx_context != 0) {
		init(c);
		reshape_bubble3d(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
		do_display(c);
		glFinish();
		glXSwapBuffers(display, window);
	} else
		MI_CLEARWINDOW(mi);
}

ENTRYPOINT void
draw_bubble3d(ModeInfo * mi)
{
	struct context *c = &contexts[MI_SCREEN(mi)];
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

	MI_IS_DRAWN(mi) = True;

	if (!c->glx_context)
		return;

	glXMakeCurrent(display, window, *(c->glx_context));

        glb_config.polygon_count = 0;
	do_display(c);
        mi->polygon_count = glb_config.polygon_count;

        if (mi->fps_p) do_fps (mi);
	glFinish();
	glXSwapBuffers(display, window);
}

#ifndef STANDALONE
ENTRYPOINT void
change_bubble3d(ModeInfo * mi)
{
	/* nothing */
}
#endif /* !STANDALONE */

ENTRYPOINT void
release_bubble3d(ModeInfo * mi)
{
  if (contexts) {
    int screen;
    for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
      struct context *c = &contexts[screen];
      if (c->draw_context)
        glb_draw_end(c->draw_context);
    }
    free (contexts);
    contexts = 0;
  }
  FreeAllGL(mi);
}

XSCREENSAVER_MODULE ("Bubble3D", bubble3d)

#endif /* USE_GL */
