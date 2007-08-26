/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "texture.h"

#include "constants.h"
#include "graphic.h"
#include "layered_filesystem.h"
#include "wexception.h"


/**
 * Create a texture, taking the pixel data from a Pic.
 * Currently it converts a 16 bit pic to a 8 bit texture. This should
 * be changed to load a 8 bit file directly, however.
 */
Texture::Texture
(const char            & fnametmpl,
 const uint              frametime,
 const SDL_PixelFormat & format)
{
	m_colormap = 0;
	m_nrframes = 0;
	m_pixels = 0;
	m_frametime = frametime;
	is_32bit = format.BytesPerPixel == 4;

	// Load the pictures one by one
	char fname[256];

	for (;;) {
		int nr = m_nrframes;
		char *p;

		// create the file name by reverse-scanning for '?' and replacing
		snprintf(fname, sizeof(fname), "%s", &fnametmpl);
		p = fname + strlen(fname);
		while (p > fname) {
			if (*--p != '?')
				continue;

			*p = '0' + (nr % 10);
			nr = nr / 10;
		}

		if (nr) // cycled up to maximum possible frame number
			break;

		// is the frame actually there?
		if (!g_fs->FileExists(fname))
			break;

		// Load it
		SDL_Surface* surf;

		m_texture_picture =fname;

		try
		{
			surf = LoadImage(fname);
		}
		catch (std::exception& e)
		{
			log("WARNING: Failed to load texture frame %s: %s\n", fname, e.what());
			break;
		}

		if (surf->w != TEXTURE_WIDTH || surf->h != TEXTURE_HEIGHT) {
			SDL_FreeSurface(surf);
			log("WARNING: %s: texture must be %ix%i pixels big\n", fname, TEXTURE_WIDTH, TEXTURE_HEIGHT);
			break;
		}

		// Determine color map if it's the first frame
		if (!m_nrframes) {
			if (surf->format->BitsPerPixel == 8)
				m_colormap = new Colormap(*surf->format->palette->colors, format);
			else {
				SDL_Color pal[256];

				log("WARNING: %s: using 332 default palette\n", fname);

				for (int r=0;r<8;r++)
					for (int g=0;g<8;g++)
						for (int b=0;b<4;b++) {
							pal[(r<<5) | (g<<2) | b].r=r<<5;
							pal[(r<<5) | (g<<2) | b].g=g<<5;
							pal[(r<<5) | (g<<2) | b].b=b<<6;
						}

				m_colormap = new Colormap(*pal, format);
			}
		}

		// Convert to our palette
		SDL_Palette palette;
		SDL_PixelFormat fmt;

		palette.ncolors = 256;
		palette.colors = m_colormap->get_palette();

		memset(&fmt, 0, sizeof(fmt));
		fmt.BitsPerPixel = 8;
		fmt.BytesPerPixel = 1;
		fmt.palette = &palette;

		SDL_Surface* cv = SDL_ConvertSurface(surf, &fmt, 0);

		// Add the frame
		m_pixels = (uchar*)realloc(m_pixels, TEXTURE_WIDTH*TEXTURE_HEIGHT*(m_nrframes+1));
		m_curframe = &m_pixels[TEXTURE_WIDTH*TEXTURE_HEIGHT*m_nrframes];
		m_nrframes++;

		SDL_LockSurface(cv);

		for (int y = 0; y < TEXTURE_HEIGHT; y++)
			memcpy(m_curframe + y*TEXTURE_WIDTH, (Uint8*)cv->pixels + y*cv->pitch, TEXTURE_WIDTH);

		SDL_UnlockSurface(cv);

		SDL_FreeSurface(cv);
		SDL_FreeSurface(surf);
	}

	if (!m_nrframes)
		throw wexception("%s: texture has no frames", &fnametmpl);
}

/**
 * Clean up.
 */
Texture::~Texture ()
{
	delete m_colormap;
	free(m_pixels);
}

/**
 * Return the basic terrain colour to be used in the minimap.
*/
Uint32 Texture::get_minimap_color(const char shade) {
	uchar clr = m_pixels[0]; // just use the top-left pixel
	uint table = (uchar)shade;

	return is_32bit ?
		static_cast<const Uint32 * const>(m_colormap->get_colormap())
		[clr | (table << 8)]
		:
		static_cast<const Uint16 * const>(m_colormap->get_colormap())
		[clr | (table << 8)];
}

/**
 * Set the current frame according to the game time.
 */
void Texture::animate(uint time)
{
	int frame = (time / m_frametime) % m_nrframes;

	uchar* lastframe = m_curframe;

	m_curframe = &m_pixels[TEXTURE_WIDTH*TEXTURE_HEIGHT*frame];
	if (lastframe != m_curframe)
		m_was_animated = true;
}
