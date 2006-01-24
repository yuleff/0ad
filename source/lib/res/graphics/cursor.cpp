#include "precompiled.h"

#include <string.h>
#include <sstream>

// On Windows, allow runtime choice between system cursors and OpenGL
// cursors (Windows = more responsive, OpenGL = more consistent with what
// the game sees)
#if OS_WIN
# define ALLOW_SYS_CURSOR 1
#else
# define ALLOW_SYS_CURSOR 0
#endif

#include "lib/ogl.h"
#include "sysdep/sysdep.h"	// sys_cursor_*
#include "../res.h"
#include "ogl_tex.h"
#include "cursor.h"


static void* load_sys_cursor(const char* filename, int hx, int hy)
{
#if !ALLOW_SYS_CURSOR
	return 0;
#else
	Tex t;
	if(tex_load(filename, &t) < 0)
		return 0;

	{
	void* sys_cursor = 0;	// return value

	// convert to required BGRA format.
	const uint flags = (t.flags | TEX_BGR) & ~TEX_DXT;
	if(tex_transform_to(&t, flags) < 0)
		goto fail;
	void* bgra_img = tex_get_data(&t);
	if(!bgra_img)
		goto fail;

	if(sys_cursor_create(t.w, t.h, bgra_img, hx, hy, &sys_cursor) < 0)
		goto fail;

	(void)tex_free(&t);
	return sys_cursor;
	}

fail:
	debug_warn("failed");
	(void)tex_free(&t);
	return 0;
#endif
}


// no init is necessary because this is stored in struct Cursor, which
// is 0-initialized by h_mgr.
class GLCursor
{
	Handle ht;

	uint w, h;
	uint hotspotx, hotspoty;

public:
	LibError create(const char* filename, uint hotspotx_, uint hotspoty_)
	{
		ht = ogl_tex_load(filename);
		RETURN_ERR(ht);

		(void)ogl_tex_get_size(ht, &w, &h, 0);

		hotspotx = hotspotx_; hotspoty = hotspoty_;

		(void)ogl_tex_set_filter(ht, GL_NEAREST);
		(void)ogl_tex_upload(ht);
		return ERR_OK;
	}

	void destroy()
	{
		// note: we're stored in a resource => ht is initially 0 =>
		// this is safe, no need for an is_valid flag
		(void)ogl_tex_free(ht);
	}

	void draw(uint x, uint y) const
	{
		(void)ogl_tex_bind(ht);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// OpenGL's coordinate system is "upside-down"; correct for that.
		y = g_yres - y;

		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex2i( x-hotspotx,   y+hotspoty   );
		glTexCoord2i(1, 0); glVertex2i( x-hotspotx+w, y+hotspoty   );
		glTexCoord2i(1, 1); glVertex2i( x-hotspotx+w, y+hotspoty-h );
		glTexCoord2i(0, 1); glVertex2i( x-hotspotx,   y+hotspoty-h );
		glEnd();
	}

	int validate() const
	{
		const uint A = 128;	// no cursor is expected to get this big
		if(w > A || h > A || hotspotx > A || hotspoty > A)
			return ERR_1;
		if(ht < 0)
			return ERR_2;
		return ERR_OK;
	}
};


struct Cursor
{
	void* sys_cursor;

	// valid iff sys_cursor == 0.
	GLCursor gl_cursor;
};

H_TYPE_DEFINE(Cursor);

static void Cursor_init(Cursor* UNUSED(c), va_list UNUSED(args))
{
}

static void Cursor_dtor(Cursor* c)
{
	// (note: these are safe, no need for an is_valid flag)

	if(c->sys_cursor)
		WARN_ERR(sys_cursor_free(c->sys_cursor));
	else
		c->gl_cursor.destroy();
}

static LibError Cursor_reload(Cursor* c, const char* name, Handle)
{
	char filename[VFS_MAX_PATH];

	// read pixel offset of the cursor's hotspot [the bit of it that's
	// drawn at (g_mouse_x,g_mouse_y)] from file.
	uint hotspotx = 0, hotspoty = 0;
	{
		snprintf(filename, ARRAY_SIZE(filename), "art/textures/cursors/%s.txt", name);
		FileIOBuf buf; size_t size;
		RETURN_ERR(vfs_load(filename, buf, size));
		std::stringstream s(std::string((const char*)buf, size));
		s >> hotspotx >> hotspoty;
		(void)file_buf_free(buf);
	}

	// load actual cursor
	snprintf(filename, ARRAY_SIZE(filename), "art/textures/cursors/%s.dds", name);
	// .. try loading as system cursor (2d, hardware accelerated)
	c->sys_cursor = load_sys_cursor(filename, hotspotx, hotspoty);
	// .. fall back to GLCursor (system cursor code is disabled or failed)
	if(!c->sys_cursor)
		RETURN_ERR(c->gl_cursor.create(filename, hotspotx, hotspoty));

	return ERR_OK;
}

static LibError Cursor_validate(const Cursor* c)
{
	// note: system cursors have no state to speak of, so we don't need to
	// validate them.

	if(!c->sys_cursor)
		RETURN_ERR(c->gl_cursor.validate());
	return ERR_OK;
}

static LibError Cursor_to_string(const Cursor* c, char* buf)
{
	const char* type = c->sys_cursor? "sys" : "gl";
	snprintf(buf, H_STRING_LEN, "(%s)", type);
	return ERR_OK;
}


// note: these standard resource interface functions are not exposed to the
// caller. all we need here is storage for the sys_cursor / GLCursor and
// a name -> data lookup mechanism, both provided by h_mgr.
// in other words, we continually create/free the cursor resource in
// cursor_draw and trust h_mgr's caching to absorb it.

static Handle cursor_load(const char* name)
{
	return h_alloc(H_Cursor, name, 0);
}

static LibError cursor_free(Handle& h)
{
	return h_free(h, H_Cursor);
}


// draw the specified cursor at the given pixel coordinates
// (origin is top-left to match the windowing system).
// uses a hardware mouse cursor where available, otherwise a
// portable OpenGL implementation.
LibError cursor_draw(const char* name, int x, int y)
{
	// Use 'null' to disable the cursor
	if(!name)
	{
		WARN_ERR(sys_cursor_set(0));
		return ERR_OK;
	}

	Handle hc = cursor_load(name);
	CHECK_ERR(hc);
	H_DEREF(hc, Cursor, c);

	if(c->sys_cursor)
		WARN_ERR(sys_cursor_set(c->sys_cursor));
	else
		c->gl_cursor.draw(x, y);

	(void)cursor_free(hc);
	return ERR_OK;
}
