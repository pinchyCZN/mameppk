#define MW_VERBOSE 0

/*
 * video/mystwarr.c - Konami "Pre-GX" video hardware (here there be dragons)
 *
 */

#include "emu.h"
#include "includes/konamigx.h"
#include "includes/mystwarr.h"


// create a decoding buffer to hold decodable tiles so that the ROM test will pass by
// reading the original raw data
static void mystwarr_decode_tiles(running_machine &machine)
{
	mystwarr_state *state = machine.driver_data<mystwarr_state>();
	UINT8 *s = machine.root_device().memregion("gfx1")->base();
	int len = machine.root_device().memregion("gfx1")->bytes();
	UINT8 *pFinish = s+len-3;
	UINT8 *d, *decoded;

	int gfxnum = state->m_k056832->get_gfx_num();

	decoded = auto_alloc_array(machine, UINT8, len);
	d = decoded;

	// now convert the data into a drawable format so we can decode it
	while (s < pFinish)
	{
		/* convert the whole mess to 5bpp planar in System GX's format
		       (p3 p1 p2 p0 p5)
		       (the original ROMs are stored as chunky for the first 4 bits
		       and the 5th bit is planar, which is undecodable as-is) */
		int d0 = ((s[0]&0x80)   )|((s[0]&0x08)<<3)|((s[1]&0x80)>>2)|((s[1]&0x08)<<1)|
					((s[2]&0x80)>>4)|((s[2]&0x08)>>1)|((s[3]&0x80)>>6)|((s[3]&0x08)>>3);
		int d1 = ((s[0]&0x40)<<1)|((s[0]&0x04)<<4)|((s[1]&0x40)>>1)|((s[1]&0x04)<<2)|
					((s[2]&0x40)>>3)|((s[2]&0x04)   )|((s[3]&0x40)>>5)|((s[3]&0x04)>>2);
		int d2 = ((s[0]&0x20)<<2)|((s[0]&0x02)<<5)|((s[1]&0x20)   )|((s[1]&0x02)<<3)|
					((s[2]&0x20)>>2)|((s[2]&0x02)<<1)|((s[3]&0x20)>>4)|((s[3]&0x02)>>1);
		int d3 = ((s[0]&0x10)<<3)|((s[0]&0x01)<<6)|((s[1]&0x10)<<1)|((s[1]&0x01)<<4)|
					((s[2]&0x10)>>1)|((s[2]&0x01)<<2)|((s[3]&0x10)>>3)|((s[3]&0x01)   );

		d[0] = d3;
		d[1] = d1;
		d[2] = d2;
		d[3] = d0;
		d[4] = s[4];

		s += 5;
		d += 5;
	}

	state->m_gfxdecode->gfx(gfxnum)->set_source(decoded);
}


// Mystic Warriors requires tile based blending.
K056832_CB_MEMBER(mystwarr_state::mystwarr_tile_callback)
{
	if (layer == 1) {if ((*code & 0xff00) + (*color) == 0x4101) m_cbparam++; else m_cbparam--;} //* water hack (TEMPORARY)
	*color = m_layer_colorbase[layer] | (*color >> 1 & 0x1e);
}

// for games with 5bpp tile data
K056832_CB_MEMBER(mystwarr_state::game5bpp_tile_callback)
{
	*color = m_layer_colorbase[layer] | (*color >> 1 & 0x1e);
}

// for games with 4bpp tile data
K056832_CB_MEMBER(mystwarr_state::game4bpp_tile_callback)
{
	*color = m_layer_colorbase[layer] | (*color >> 2 & 0x0f);
}

K055673_CB_MEMBER(mystwarr_state::mystwarr_sprite_callback)
{
	int c = *color;
	*color = m_sprite_colorbase | (c & 0x001f);
	*priority_mask = c & 0x00f0;
}

K055673_CB_MEMBER(mystwarr_state::metamrph_sprite_callback)
{
	int c = *color;
	int attr = c;

	c = (c & 0x1f) | m_sprite_colorbase;

	// Bit8 & 9 are effect attributes. It is not known whether the effects are generated by external logic.
	if ((attr & 0x300) != 0x300)
	{
		*color = c;
		*priority_mask = (attr & 0xe0) >> 2;
	}
	else
	{
		*color = c | 3<<K055555_MIXSHIFT | K055555_SKIPSHADOW; // reflection?
		*priority_mask = 0x1c;
	}
}

K055673_CB_MEMBER(mystwarr_state::gaiapols_sprite_callback)
{
	int c = *color;

	*color = m_sprite_colorbase | (c>>4 & 0x20) | (c & 0x001f);
	*priority_mask = c & 0x00e0;
}

K055673_CB_MEMBER(mystwarr_state::martchmp_sprite_callback)
{
	int c = *color;

	// Bit8 & 9 are effect attributes. It is not known whether the effects are generated by external logic.
	if ((c & 0x3ff) == 0x11f)
		*color = K055555_FULLSHADOW;
	else
		*color = m_sprite_colorbase | (c & 0x1f);

	if (m_oinprion & 0xf0)
		*priority_mask = m_cbparam;  // use PCU2 internal priority
	else
		*priority_mask = c & 0xf0; // use color implied priority
}



TILE_GET_INFO_MEMBER(mystwarr_state::get_gai_936_tile_info)
{
	int tileno, colour;
	UINT8 *ROM = memregion("gfx4")->base();
	UINT8 *dat1 = ROM, *dat2 = ROM + 0x20000, *dat3 = ROM + 0x60000;

	tileno = dat3[tile_index] | ((dat2[tile_index]&0x3f)<<8);

	if (tile_index & 1)
		colour = (dat1[tile_index>>1]&0xf);
	else
		colour = ((dat1[tile_index>>1]>>4)&0xf);

	if (dat2[tile_index] & 0x80) colour |= 0x10;

	colour |= m_sub1_colorbase << 4;

	SET_TILE_INFO_MEMBER(0, tileno, colour, 0);
}

VIDEO_START_MEMBER(mystwarr_state, gaiapols)
{
	m_gametype = 0;

	mystwarr_decode_tiles(machine());

	konamigx_mixer_init(*m_screen, 0);

	m_k056832->set_layer_offs(0, -2+2-1, 0-1);
	m_k056832->set_layer_offs(1,  0+2, 0);
	m_k056832->set_layer_offs(2,  2+2, 0);
	m_k056832->set_layer_offs(3,  3+2, 0);

	K053936_wraparound_enable(0, 1);
	K053936GP_set_offset(0, -10,  0); // floor tiles in demo loop2 (Elaine vs. boss)

	m_ult_936_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(mystwarr_state::get_gai_936_tile_info),this), TILEMAP_SCAN_ROWS,  16, 16, 512, 512);
	m_ult_936_tilemap->set_transparent_pen(0);
}

TILE_GET_INFO_MEMBER(mystwarr_state::get_ult_936_tile_info)
{
	int tileno, colour;
	UINT8 *ROM = memregion("gfx4")->base();
	UINT8 *dat1 = ROM, *dat2 = ROM + 0x40000;

	tileno = dat2[tile_index] | ((dat1[tile_index]&0x1f)<<8);

	colour = m_sub1_colorbase;

	SET_TILE_INFO_MEMBER(0, tileno, colour, (dat1[tile_index]&0x40) ? TILE_FLIPX : 0);
}

VIDEO_START_MEMBER(mystwarr_state, dadandrn)
{
	m_gametype = 1;

	mystwarr_decode_tiles(machine());

	konamigx_mixer_init(*m_screen, 0);

	konamigx_mixer_primode(1);

	m_k056832->set_layer_offs(0, -2+4, 0);
	m_k056832->set_layer_offs(1,  0+4, 0);
	m_k056832->set_layer_offs(2,  2+4, 0);
	m_k056832->set_layer_offs(3,  3+4, 0);

	K053936_wraparound_enable(0, 1);
	K053936GP_set_offset(0, -8, 0); // Brainy's laser

	m_ult_936_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(mystwarr_state::get_ult_936_tile_info),this), TILEMAP_SCAN_ROWS,  16, 16, 512, 512);
	m_ult_936_tilemap->set_transparent_pen(0);
}

VIDEO_START_MEMBER(mystwarr_state, mystwarr)
{
	m_gametype = 0;

	mystwarr_decode_tiles(machine());

	konamigx_mixer_init(*m_screen, 0);

	m_k056832->set_layer_offs(0, -2-3, 0);
	m_k056832->set_layer_offs(1,  0-3, 0);
	m_k056832->set_layer_offs(2,  2-3, 0);
	m_k056832->set_layer_offs(3,  3-3, 0);

	m_cbparam = 0;
}

VIDEO_START_MEMBER(mystwarr_state, metamrph)
{
	m_gametype = 0;

	mystwarr_decode_tiles(machine());

	konamigx_mixer_init(*m_screen, 0);

	// other reference, floor at first boss
	m_k056832->set_layer_offs(0, -2+4, 0); // text
	m_k056832->set_layer_offs(1,  0+4, 0); // attract sea
	m_k056832->set_layer_offs(2,  2+4, 0); // attract red monster in background of sea
	m_k056832->set_layer_offs(3,  3+4, 0); // attract sky background to sea
}

VIDEO_START_MEMBER(mystwarr_state, viostorm)
{
	m_gametype = 0;

	mystwarr_decode_tiles(machine());

	konamigx_mixer_init(*m_screen, 0);

	m_k056832->set_layer_offs(0, -2+1, 0);
	m_k056832->set_layer_offs(1,  0+1, 0);
	m_k056832->set_layer_offs(2,  2+1, 0);
	m_k056832->set_layer_offs(3,  3+1, 0);
}

VIDEO_START_MEMBER(mystwarr_state, martchmp)
{
	m_gametype = 0;

	mystwarr_decode_tiles(machine());

	konamigx_mixer_init(*m_screen, 0);

	m_k056832->set_layer_offs(0, -2-4, 0);
	m_k056832->set_layer_offs(1,  0-4, 0);
	m_k056832->set_layer_offs(2,  2-4, 0);
	m_k056832->set_layer_offs(3,  3-4, 0);

	m_k054338->invert_alpha(0);
}



UINT32 mystwarr_state::screen_update_mystwarr(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int i, old, blendmode=0;

	if (m_cbparam<0) m_cbparam=0; else if (m_cbparam>=32) blendmode=(1<<16|GXMIX_BLEND_FORCE)<<2; //* water hack (TEMPORARY)

	for (i = 0; i < 4; i++)
	{
		old = m_layer_colorbase[i];
		m_layer_colorbase[i] = m_k055555->K055555_get_palette_index(i)<<4;
		if( old != m_layer_colorbase[i] ) m_k056832->mark_plane_dirty(i);
	}

	m_sprite_colorbase = m_k055555->K055555_get_palette_index(4)<<5;

	konamigx_mixer(screen, bitmap, cliprect, 0, 0, 0, 0, blendmode, 0, 0);
	return 0;
}

UINT32 mystwarr_state::screen_update_metamrph(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int i, old;

	for (i = 0; i < 4; i++)
	{
		old = m_layer_colorbase[i];
		m_layer_colorbase[i] = m_k055555->K055555_get_palette_index(i)<<4;
		if (old != m_layer_colorbase[i]) m_k056832->mark_plane_dirty(i);
	}

	m_sprite_colorbase = m_k055555->K055555_get_palette_index(4)<<4;

	konamigx_mixer(screen, bitmap, cliprect, 0, GXSUB_K053250 | GXSUB_4BPP, 0, 0, 0, 0, 0);
	return 0;
}

UINT32 mystwarr_state::screen_update_martchmp(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int i, old, blendmode;

	for (i = 0; i < 4; i++)
	{
		old = m_layer_colorbase[i];
		m_layer_colorbase[i] = m_k055555->K055555_get_palette_index(i)<<4;
		if (old != m_layer_colorbase[i]) m_k056832->mark_plane_dirty(i);
	}

	m_sprite_colorbase = m_k055555->K055555_get_palette_index(4)<<5;

	m_cbparam = m_k055555->K055555_read_register(K55_PRIINP_8);
	m_oinprion = m_k055555->K055555_read_register(K55_OINPRI_ON);

	// not quite right
	blendmode = (m_oinprion==0xef && m_k054338->register_r(K338_REG_PBLEND)) ? ((1<<16|GXMIX_BLEND_FORCE)<<2) : 0;

	konamigx_mixer(screen, bitmap, cliprect, 0, 0, 0, 0, blendmode, 0, 0);
	return 0;
}



WRITE16_MEMBER(mystwarr_state::ddd_053936_enable_w)
{
	if (ACCESSING_BITS_8_15)
	{
		m_roz_enable = data & 0x0100;
		m_roz_rombank = (data & 0xc000)>>14;
	}
}

WRITE16_MEMBER(mystwarr_state::ddd_053936_clip_w)
{
	int old, clip_x, clip_y, size_x, size_y;
	int minx, maxx, miny, maxy;

	if (offset == 1)
	{
		if (ACCESSING_BITS_8_15) K053936GP_clip_enable(0, data & 0x0100);
	}
	else
	{
		old = m_clip;
		COMBINE_DATA(&m_clip);
		if (m_clip != old)
		{
			clip_x = (m_clip & 0x003f) >> 0;
			clip_y = (m_clip & 0x0fc0) >> 6;
			size_x = (m_clip & 0x3000) >> 12;
			size_y = (m_clip & 0xc000) >> 14;

			switch (size_x)
			{
				case 0x3: size_x = 1; break;
				case 0x2: size_x = 2; break;
				default:  size_x = 4; break;
			}

			switch (size_y)
			{
				case 0x3: size_y = 1; break;
				case 0x2: size_y = 2; break;
				default:  size_y = 4; break;
			}

			minx = clip_x << 7;
			maxx = ((clip_x + size_x) << 7) - 1;
			miny = clip_y << 7;
			maxy = ((clip_y + size_y) << 7) - 1;

			K053936GP_set_cliprect(0, minx, maxx, miny, maxy);
		}
	}
}

// reference: 223e5c in gaiapolis (ROMs 34j and 36m)
READ16_MEMBER(mystwarr_state::gai_053936_tilerom_0_r)
{
	UINT8 *ROM1 = (UINT8 *)memregion("gfx4")->base();
	UINT8 *ROM2 = (UINT8 *)memregion("gfx4")->base();

	ROM1 += 0x20000;
	ROM2 += 0x20000+0x40000;

	return ((ROM1[offset]<<8) | ROM2[offset]);
}

READ16_MEMBER(mystwarr_state::ddd_053936_tilerom_0_r)
{
	UINT8 *ROM1 = (UINT8 *)memregion("gfx4")->base();
	UINT8 *ROM2 = (UINT8 *)memregion("gfx4")->base();

	ROM2 += 0x40000;

	return ((ROM1[offset]<<8) | ROM2[offset]);
}

// reference: 223e1a in gaiapolis (ROM 36j)
READ16_MEMBER(mystwarr_state::ddd_053936_tilerom_1_r)
{
	UINT8 *ROM = (UINT8 *)memregion("gfx4")->base();

	return ROM[offset/2];
}

// reference: 223db0 in gaiapolis (ROMs 32n, 29n, 26n)
READ16_MEMBER(mystwarr_state::gai_053936_tilerom_2_r)
{
	UINT8 *ROM = (UINT8 *)memregion("gfx3")->base();

	offset += (m_roz_rombank * 0x100000);

	return ROM[offset/2]<<8;
}

READ16_MEMBER(mystwarr_state::ddd_053936_tilerom_2_r)
{
	UINT8 *ROM = (UINT8 *)memregion("gfx3")->base();

	offset += (m_roz_rombank * 0x100000);

	return ROM[offset]<<8;
}

UINT32 mystwarr_state::screen_update_dadandrn(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)/* and gaiapols */
{
	int i, newbase, dirty, rozmode;

	if (m_gametype == 0)
	{
		m_sprite_colorbase = (m_k055555->K055555_get_palette_index(4)<<4)&0x7f;
		rozmode = GXSUB_4BPP;
	}
	else
	{
		m_sprite_colorbase = (m_k055555->K055555_get_palette_index(4)<<3)&0x7f;
		rozmode = GXSUB_8BPP;
	}

	if (m_k056832->get_layer_association())
	{
		for (i=0; i<4; i++)
		{
			newbase = m_k055555->K055555_get_palette_index(i)<<4;
			if (m_layer_colorbase[i] != newbase)
			{
				m_layer_colorbase[i] = newbase;
				m_k056832->mark_plane_dirty(i);
			}
		}
	}
	else
	{
		for (dirty=0, i=0; i<4; i++)
		{
			newbase = m_k055555->K055555_get_palette_index(i)<<4;
			if (m_layer_colorbase[i] != newbase)
			{
				m_layer_colorbase[i] = newbase;
				dirty = 1;
			}
		}
		if (dirty) m_k056832->mark_all_tilemaps_dirty();

	}

	m_last_psac_colorbase = m_sub1_colorbase;
	m_sub1_colorbase = m_k055555->K055555_get_palette_index(5);

	if (m_last_psac_colorbase != m_sub1_colorbase)
	{
		m_ult_936_tilemap->mark_all_dirty();

		if (MW_VERBOSE)
			popmessage("K053936: PSAC colorbase changed");
	}

	konamigx_mixer(screen, bitmap, cliprect, (m_roz_enable) ? m_ult_936_tilemap : 0, rozmode, 0, 0, 0, 0, 0);
	return 0;
}
