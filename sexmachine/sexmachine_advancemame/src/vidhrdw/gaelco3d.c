/*************************************************************************

    Driver for Gaelco 3D games

    driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "gaelco3d.h"
#include "vidhrdw/poly.h"

UINT8 *gaelco3d_texture;
UINT8 *gaelco3d_texmask;
UINT32 gaelco3d_texture_size;
UINT32 gaelco3d_texmask_size;


#define MAX_POLYGONS		4096
#define MAX_POLYDATA		(MAX_POLYGONS * 21)

#define BILINEAR_FILTER		1			/* should be 1, but can be set to 0 for speed */

#define DISPLAY_TEXTURE		0
#define LOG_POLYGONS		0
#define DISPLAY_STATS		0

#define IS_POLYEND(x)		(((x) ^ ((x) >> 1)) & 0x4000)


static mame_bitmap *screenbits;
static mame_bitmap *zbuffer;
static UINT16 *palette;
static UINT32 *polydata_buffer;
static UINT32 polydata_count;

static int polygons;
static int lastscan;



/*************************************
 *
 *  Video init
 *
 *************************************/

VIDEO_START( gaelco3d )
{
	screenbits = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!screenbits)
		return 1;

	zbuffer = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 16);
	if (!zbuffer)
		return 1;

	palette = auto_malloc(32768 * sizeof(palette[0]));
	polydata_buffer = auto_malloc(MAX_POLYDATA * sizeof(polydata_buffer[0]));

	return 0;
}



/*************************************
 *
 *  TMS32031 floating point conversion
 *
 *************************************/

typedef union int_double
{
	double d;
	float f[2];
	UINT32 i[2];
} int_double;


static float dsp_to_float(UINT32 val)
{
	INT32 _mantissa = val << 8;
	INT8 _exponent = (INT32)val >> 24;
	int_double id;

	if (_mantissa == 0 && _exponent == -128)
		return 0;
	else if (_mantissa >= 0)
	{
		int exponent = (_exponent + 127) << 23;
		id.i[0] = exponent + (_mantissa >> 8);
	}
	else
	{
		int exponent = (_exponent + 127) << 23;
		INT32 man = -_mantissa;
		id.i[0] = 0x80000000 + exponent + ((man >> 8) & 0x00ffffff);
	}
	return id.f[0];
}


/*************************************
 *
 *  Polygon rendering
 *
 *************************************/

static int render_poly(UINT32 *polydata)
{
	/* these three parameters combine via A * x + B * y + C to produce a 1/z value */
	float ooz_dx = dsp_to_float(polydata[4]) * GAELCO3D_RESOLUTION_DIVIDE;
	float ooz_dy = dsp_to_float(polydata[3]) * GAELCO3D_RESOLUTION_DIVIDE;
	float ooz_base = dsp_to_float(polydata[8]);

	/* these three parameters combine via A * x + B * y + C to produce a u/z value */
	float uoz_dx = dsp_to_float(polydata[6]) * GAELCO3D_RESOLUTION_DIVIDE;
	float uoz_dy = dsp_to_float(polydata[5]) * GAELCO3D_RESOLUTION_DIVIDE;
	float uoz_base = dsp_to_float(polydata[9]);

	/* these three parameters combine via A * x + B * y + C to produce a v/z value */
	float voz_dx = dsp_to_float(polydata[2]) * GAELCO3D_RESOLUTION_DIVIDE;
	float voz_dy = dsp_to_float(polydata[1]) * GAELCO3D_RESOLUTION_DIVIDE;
	float voz_base = dsp_to_float(polydata[7]);

	/* this parameter is used to scale 1/z to a formal Z buffer value */
	float z0 = dsp_to_float(polydata[0]);

	/* these two parameters are the X,Y origin of the texture data */
	int xorigin = (INT32)polydata[12] >> 16;
	int yorigin = (INT32)(polydata[12] << 18) >> 18;

	int color = (polydata[10] & 0x7f) << 8;
	int midx = Machine->drv->screen_width/2;
	int midy = Machine->drv->screen_height/2;
	struct poly_vertex vert[3];
	rectangle clip;
	int i;

	// shut up the compiler
	(void)xorigin;
	(void)yorigin;

	if (LOG_POLYGONS && code_pressed(KEYCODE_LSHIFT))
	{
		int t;
		printf("poly: %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %08X %08X (%4d,%4d) %08X",
				(double)dsp_to_float(polydata[0]),
				(double)dsp_to_float(polydata[1]),
				(double)dsp_to_float(polydata[2]),
				(double)dsp_to_float(polydata[3]),
				(double)dsp_to_float(polydata[4]),
				(double)dsp_to_float(polydata[5]),
				(double)dsp_to_float(polydata[6]),
				(double)dsp_to_float(polydata[7]),
				(double)dsp_to_float(polydata[8]),
				(double)dsp_to_float(polydata[9]),
				polydata[10],
				polydata[11],
				(INT16)(polydata[12] >> 16), (INT16)(polydata[12] << 2) >> 2, polydata[12]);

		printf(" (%4d,%4d) %08X %08X", (INT16)(polydata[13] >> 16), (INT16)(polydata[13] << 2) >> 2, polydata[13], polydata[14]);
		for (t = 15; !IS_POLYEND(polydata[t - 2]); t += 2)
			printf(" (%4d,%4d) %08X %08X", (INT16)(polydata[t] >> 16), (INT16)(polydata[t] << 2) >> 2, polydata[t], polydata[t+1]);
		printf("\n");
	}

	/* compute the adjusted clip */
	clip.min_x = Machine->visible_area.min_x - midx;
	clip.min_y = Machine->visible_area.min_y - midy;
	clip.max_x = Machine->visible_area.max_x - midx;
	clip.max_y = Machine->visible_area.max_y - midy;

	/* extract the first two vertices */
	vert[0].x = (((INT32)polydata[13] >> 16) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
	vert[0].y = (((INT32)(polydata[13] << 18) >> 18) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
	vert[1].x = (((INT32)polydata[15] >> 16) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
	vert[1].y = (((INT32)(polydata[15] << 18) >> 18) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;

	/* loop over the remaining verticies */
	for (i = 17; !IS_POLYEND(polydata[i - 2]) && i < 1000; i += 2)
	{
		offs_t endmask = gaelco3d_texture_size - 1;
		const struct poly_scanline_data *scans;
		UINT32 tex = polydata[11];
		int x, y;

		/* extract vertex 2 */
		vert[2].x = (((INT32)polydata[i] >> 16) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
		vert[2].y = (((INT32)(polydata[i] << 18) >> 18) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;

		/* compute the scanning parameters */
		scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], &clip);
		if (scans)
		{
			int pixeloffs, zbufval, u, v;
#if (BILINEAR_FILTER)
			int paldata, r, g, b, f, tf;
#endif
			/* special case: no Z buffering and no perspective correction */
			if (color != 0x7f00 && z0 < 0 && ooz_dx == 0 && ooz_dy == 0)
			{
				float zbase = 1.0f / ooz_base;
				float uoz_step = uoz_dx * zbase;
				float voz_step = voz_dx * zbase;
				zbufval = (int)(-z0 * zbase);

				/* loop over scanlines */
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = (UINT16 *)screenbits->line[midy - y] + midx;
					UINT16 *zbuf = zbuffer->line[midy - y];
					float uoz = (uoz_dy * y + scan->sx * uoz_dx + uoz_base) * zbase;
					float voz = (voz_dy * y + scan->sx * voz_dx + voz_base) * zbase;

					for (x = scan->sx; x <= scan->ex; x++)
					{
#if (!BILINEAR_FILTER)
						u = (int)(uoz + 0.5); v = (int)(voz + 0.5);
						pixeloffs = (tex + v * 4096 + u) & endmask;
						if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
						{
							dest[x] = palette[color | gaelco3d_texture[pixeloffs]];
							zbuf[x] = zbufval;
						}
#else
						u = (int)(uoz * 256.0); v = (int)(voz * 256.0);
						pixeloffs = (tex + (v >> 8) * 4096 + (u >> 8)) & endmask;
						if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
						{
							paldata = palette[color | gaelco3d_texture[pixeloffs]];
							tf = f = (~u & 0xff) * (~v & 0xff);
							r = (paldata & 0x7c00) * f; g = (paldata & 0x03e0) * f; b = (paldata & 0x001f) * f;

							paldata = palette[color | gaelco3d_texture[pixeloffs + 1]];
							tf += f = (u & 0xff) * (~v & 0xff);
							r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

							paldata = palette[color | gaelco3d_texture[pixeloffs + 4096]];
							tf += f = (~u & 0xff) * (v & 0xff);
							r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

							paldata = palette[color | gaelco3d_texture[pixeloffs + 4097]];
							f = 0x10000 - tf;
							r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

							dest[x] = ((r >> 16) & 0x7c00) | ((g >> 16) & 0x03e0) | (b >> 16);
							zbuf[x] = zbufval;
						}
#endif
						/* advance texture params to the next pixel */
						uoz += uoz_step;
						voz += voz_step;
					}
				}
			}

			/* general case: non-alpha blended */
			else if (color != 0x7f00)
			{
				/* loop over scanlines */
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = (UINT16 *)screenbits->line[midy - y] + midx;
					UINT16 *zbuf = zbuffer->line[midy - y];
					float ooz = ooz_dy * y + scan->sx * ooz_dx + ooz_base;
					float uoz = uoz_dy * y + scan->sx * uoz_dx + uoz_base;
					float voz = voz_dy * y + scan->sx * voz_dx + voz_base;

					for (x = scan->sx; x <= scan->ex; x++)
					{
						if (ooz > 0)
						{
							/* compute Z and check the Z buffer value first */
							float z = 1.0f / ooz;
							zbufval = (int)(z0 * z);
							if (zbufval < zbuf[x] || zbufval < 0)
							{
#if (!BILINEAR_FILTER)
								u = (int)(uoz * z + 0.5); v = (int)(voz * z + 0.5);
								pixeloffs = (tex + v * 4096 + u) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									dest[x] = palette[color | gaelco3d_texture[pixeloffs]];
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#else
								u = (int)(uoz * z * 256.0); v = (int)(voz * z * 256.0);
								pixeloffs = (tex + (v >> 8) * 4096 + (u >> 8)) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									paldata = palette[color | gaelco3d_texture[pixeloffs]];
									tf = f = (~u & 0xff) * (~v & 0xff);
									r = (paldata & 0x7c00) * f; g = (paldata & 0x03e0) * f; b = (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[pixeloffs + 1]];
									tf += f = (u & 0xff) * (~v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[pixeloffs + 4096]];
									tf += f = (~u & 0xff) * (v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[pixeloffs + 4097]];
									f = 0x10000 - tf;
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									dest[x] = ((r >> 16) & 0x7c00) | ((g >> 16) & 0x03e0) | (b >> 16);
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#endif
							}
						}

						/* advance texture params to the next pixel */
						ooz += ooz_dx;
						uoz += uoz_dx;
						voz += voz_dx;
					}
				}
			}

			/* color 0x7f seems to be hard-coded as a 50% alpha blend */
			else
			{
				/* loop over scanlines */
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = (UINT16 *)screenbits->line[midy - y] + midx;
					UINT16 *zbuf = zbuffer->line[midy - y];
					float ooz = ooz_dy * y + scan->sx * ooz_dx + ooz_base;
					float uoz = uoz_dy * y + scan->sx * uoz_dx + uoz_base;
					float voz = voz_dy * y + scan->sx * voz_dx + voz_base;

					for (x = scan->sx; x <= scan->ex; x++)
					{
						if (ooz > 0)
						{
							/* compute Z and check the Z buffer value first */
							float z = 1.0f / ooz;
							zbufval = (int)(z0 * z);
							if (zbufval < zbuf[x] || zbufval < 0)
							{
#if (!BILINEAR_FILTER)
								u = (int)(uoz * z + 0.5); v = (int)(voz * z + 0.5);
								pixeloffs = (tex + v * 4096 + u) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									dest[x] = ((dest[x] >> 1) & 0x3def) + ((palette[color | gaelco3d_texture[pixeloffs]] >> 1) & 0x3def);
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#else
								u = (int)(uoz * z * 256.0); v = (int)(voz * z * 256.0);
								pixeloffs = (tex + (v >> 8) * 4096 + (u >> 8)) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									paldata = palette[color | gaelco3d_texture[pixeloffs]];
									tf = f = (~u & 0xff) * (~v & 0xff);
									r = (paldata & 0x7c00) * f; g = (paldata & 0x03e0) * f; b = (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[pixeloffs + 1]];
									tf += f = (u & 0xff) * (~v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[pixeloffs + 4096]];
									tf += f = (~u & 0xff) * (v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[pixeloffs + 4097]];
									f = 0x10000 - tf;
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = ((r >> 17) & 0x7c00) | ((g >> 17) & 0x03e0) | (b >> 17);
									dest[x] = ((dest[x] >> 1) & 0x3def) + (paldata & 0x3def);
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#endif
							}
						}

						/* advance texture params to the next pixel */
						ooz += ooz_dx;
						uoz += uoz_dx;
						voz += voz_dx;
					}
				}
			}
		}

		/* copy vertex 2 to vertex 1 -- this hardware draws in fans */
		vert[1] = vert[2];
	}

	polygons++;
	return i;
}



/*************************************
 *
 *  Scene rendering
 *
 *************************************/

void gaelco3d_render(void)
{
	int i;

	/* if frameskip is engaged, skip it */
	if (!skip_this_frame())
		for (i = 0; i < polydata_count; )
			i += render_poly(&polydata_buffer[i]);

#if DISPLAY_STATS
{
	int scan = cpu_getscanline();
	ui_popup("Polys = %4d  Timeleft = %3d", polygons, (lastscan < scan) ? (scan - lastscan) : (scan + (lastscan - Machine->visible_area.max_y)));
}
#endif

	polydata_count = 0;
	polygons = 0;
	lastscan = -1;
}



/*************************************
 *
 *  Renderer access
 *
 *************************************/

WRITE32_HANDLER( gaelco3d_render_w )
{
//logerror("%06X:gaelco3d_render_w(%d,%08X)\n", activecpu_get_pc(), offset, data);

	polydata_buffer[polydata_count++] = data;
	if (polydata_count >= MAX_POLYDATA)
		fatalerror("Out of polygon buffer space!");
#if DISPLAY_STATS
	lastscan = cpu_getscanline();
#endif
}



/*************************************
 *
 *  Palette access
 *
 *************************************/

INLINE void update_palette_entry(offs_t offset, UINT16 data)
{
	int r = (data >> 10) & 0x1f;
	int g = (data >> 5) & 0x1f;
	int b = (data >> 0) & 0x1f;
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette[offset] = data;
}


WRITE16_HANDLER( gaelco3d_paletteram_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	update_palette_entry(offset, paletteram16[offset]);
}


WRITE32_HANDLER( gaelco3d_paletteram_020_w )
{
	COMBINE_DATA(&paletteram32[offset]);
	update_palette_entry(offset*2+0, paletteram32[offset] >> 16);
	update_palette_entry(offset*2+1, paletteram32[offset]);
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( gaelco3d )
{
	int x, y;

	if (DISPLAY_TEXTURE && (code_pressed(KEYCODE_Z) || code_pressed(KEYCODE_X)))
	{
		static int xv = 0, yv = 0x1000;
		UINT8 *base = gaelco3d_texture;
		int length = gaelco3d_texture_size;

		if (code_pressed(KEYCODE_X))
		{
			base = gaelco3d_texmask;
			length = gaelco3d_texmask_size;
		}

		if (code_pressed(KEYCODE_LEFT) && xv >= 4)
			xv -= 4;
		if (code_pressed(KEYCODE_RIGHT) && xv < 4096 - 4)
			xv += 4;

		if (code_pressed(KEYCODE_UP) && yv >= 4)
			yv -= 4;
		if (code_pressed(KEYCODE_DOWN) && yv < 0x40000)
			yv += 4;

		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *dest = bitmap->line[y];
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int offs = (yv + y - cliprect->min_y) * 4096 + xv + x - cliprect->min_x;
				if (offs < length)
					dest[x] = base[offs];
				else
					dest[x] = 0;
			}
		}
		ui_popup("(%04X,%04X)", xv, yv);
	}
	else
		copybitmap(bitmap, screenbits, 0,0, 0,0, cliprect, TRANSPARENCY_NONE, 0);

	logerror("---update---\n");
}
