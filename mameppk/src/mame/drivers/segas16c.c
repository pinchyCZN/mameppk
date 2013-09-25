/***************************************************************************

    Sega System 16C hardware

****************************************************************************

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "emu.h"
#include "includes/segas16c.h"


/*************************************
 *
 *  Constants
 *
 *************************************/

#define MASTER_CLOCK_10MHz		XTAL_10MHz
#define MASTER_CLOCK_8MHz		XTAL_8MHz
#define MASTER_CLOCK_25MHz		XTAL_25_1748MHz



//**************************************************************************
//  MEMORY MAPPING
//**************************************************************************

//-------------------------------------------------
//  memory_mapper - callback to handle mapping
//  requests
//-------------------------------------------------

void segas16c_state::memory_mapper(sega_315_5195_mapper_device &mapper, UINT8 index)
{
	switch (index)
	{
		case 7:	// 16k of I/O space
			mapper.map_as_handler(0x00000, 0x04000, 0xffc000, m_custom_io_r, m_custom_io_w);
			break;

		case 6:	// 4k of paletteram
			mapper.map_as_ram(0x00000, 0x01000, 0xfff000, "paletteram", write16_delegate(FUNC(segas16c_state::paletteram_w), this));
			break;

		case 5:	// 64k of tileram + 4k of textram
			mapper.map_as_ram(0x00000, 0x10000, 0xfe0000, "tileram", write16_delegate(FUNC(segas16c_state::sega_tileram_0_w), this));
			mapper.map_as_ram(0x10000, 0x01000, 0xfef000, "textram", write16_delegate(FUNC(segas16c_state::sega_textram_0_w), this));
			break;

		case 4:	// 2k of spriteram
			mapper.map_as_ram(0x00000, 0x00800, 0xfff800, "sprites", write16_delegate());
			break;

		case 3:	// 16k or 256k of work RAM
			mapper.map_as_ram(0x00000, m_workram.bytes(), ~(m_workram.bytes() - 1), "workram", write16_delegate());
			break;

		case 2:	// 3rd ROM base, or board-specific banking
			switch (m_romboard)
			{
				case ROM_BOARD_171_5704:		mapper.map_as_handler(0x00000, 0x10000, 0xff0000, read16_delegate(), write16_delegate(FUNC(segas16c_state::rom_5704_bank_w), this)); break;
				default:						assert(false);
			}
			break;

		case 1:	// 2nd ROM base, banking & math, or sound for Korean games
			switch (m_romboard)
			{
				case ROM_BOARD_171_5704:		mapper.map_as_rom(0x00000, 0x40000, 0xfc0000, "rom1base", 0x40000, write16_delegate());	break;
				default:						assert(false);
			}
			break;

		case 0:	// 1st ROM base
			switch (m_romboard)
			{
				case ROM_BOARD_171_5704:		mapper.map_as_rom(0x00000, 0x40000, 0xfc0000, "rom0base", 0000000, write16_delegate());	break;
				default:						assert(false);
			}
			break;
	}
}


//-------------------------------------------------
//  mapper_sound_r - sound port read from the
//  memory mapper chip
//-------------------------------------------------

UINT8 segas16c_state::mapper_sound_r()
{
	return 0;
}


//-------------------------------------------------
//  mapper_sound_w - sound port write from the
//  memory mapper chip
//-------------------------------------------------

void segas16c_state::mapper_sound_w(UINT8 data)
{
	soundlatch_write(data & 0xff);
	if (m_soundcpu != NULL)
		m_soundcpu->set_input_line(0, HOLD_LINE);
}



//**************************************************************************
//  MAIN CPU READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  rom_5704_bank_w - ROM board 5704 tile bank
//  selection
//-------------------------------------------------

WRITE16_MEMBER( segas16c_state::rom_5704_bank_w )
{
	if (ACCESSING_BITS_0_7)
		m_segaic16vid->segaic16_tilemap_set_bank(0, offset & 1, data & 7);
}


//-------------------------------------------------
//  standard_io_r - default I/O handler for reads
//-------------------------------------------------

READ16_MEMBER( segas16c_state::standard_io_r )
{
	offset &= 0x1fff;
	switch (offset & (0x3000/2))
	{
		case 0x1000/2:
		{
			static const char *const sysports[] = { "SERVICE", "P1", "UNUSED", "P2" };
			return ioport(sysports[offset & 3])->read();
		}

		case 0x2000/2:
			return ioport((offset & 1) ? "DSW1" : "DSW2")->read();
	}
	logerror("%06X:standard_io_r - unknown read access to address %04X\n", space.device().safe_pc(), offset * 2);
	return open_bus_r(space, 0, mem_mask);
}


//-------------------------------------------------
//  standard_io_w - default I/O handler for writes
//-------------------------------------------------

WRITE16_MEMBER( segas16c_state::standard_io_w )
{
	offset &= 0x1fff;
	switch (offset & (0x3000/2))
	{
		case 0x0000/2:
			//
            //  D7 : 1 for most games, 0 for ddux, sdi, wb3
            //  D6 : 1= Screen flip, 0= Normal screen display
            //  D5 : 1= Display on, 0= Display off
            //  D4 : 0 for most games, 1 for eswat
            //  D3 : Output to lamp 2 (1= On, 0= Off)
            //  D2 : Output to lamp 1 (1= On, 0= Off)
            //  D1 : (Output to coin counter 2?)
            //  D0 : Output to coin counter 1
            //
			m_segaic16vid->segaic16_tilemap_set_flip(0, data & 0x40);
			m_sprites->set_flip(data & 0x40);
			if (!m_disable_screen_blanking)
				m_segaic16vid->segaic16_set_display_enable(data & 0x20);
			set_led_status(machine(), 1, data & 0x08);
			set_led_status(machine(), 0, data & 0x04);
			coin_counter_w(machine(), 1, data & 0x02);
			coin_counter_w(machine(), 0, data & 0x01);
			return;
	}
	logerror("%06X:standard_io_w - unknown write access to address %04X = %04X & %04X\n", space.device().safe_pc(), offset * 2, data, mem_mask);
}


//**************************************************************************
//  CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  init_generic - common initialization
//-------------------------------------------------

void segas16c_state::init_generic(segas16c_rom_board rom_board)
{
	// remember the ROM board and work RAM size
	m_romboard = rom_board;

	// configure the NVRAM to point to our workram
	m_nvram->set_base(m_workram, m_workram.bytes());

	// create default read/write handlers
	m_custom_io_r = read16_delegate(FUNC(segas16c_state::standard_io_r), this);
	m_custom_io_w = write16_delegate(FUNC(segas16c_state::standard_io_w), this);

	// point globals to allocated memory regions
	m_segaic16vid->segaic16_tileram_0 = reinterpret_cast<UINT16 *>(memshare("tileram")->ptr());
	m_segaic16vid->segaic16_textram_0 = reinterpret_cast<UINT16 *>(memshare("textram")->ptr());

	// save state
	save_item(NAME(m_atomicp_sound_count));
	save_item(NAME(m_hwc_input_value));
	save_item(NAME(m_mj_input_num));
	save_item(NAME(m_mj_last_val));
}


//-------------------------------------------------
//  init_generic_* - ROM board-specific
//  initialization
//-------------------------------------------------

DRIVER_INIT_MEMBER(segas16c_state,generic_5704) { init_generic(ROM_BOARD_171_5704); }



//**************************************************************************
//  SOUND CPU READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  upd7759_control_w - handle writes to the
//  uPD7759 control register
//-------------------------------------------------

WRITE8_MEMBER( segas16c_state::upd7759_control_w )
{
	int size = memregion("soundcpu")->bytes() - 0x10000;
	if (size > 0)
	{
		// it is important to write in this order: if the /START line goes low
        // at the same time /RESET goes low, no sample should be started
		m_upd7759->start_w(data & 0x80);
		m_upd7759->reset_w(data & 0x40);

		// banking depends on the ROM board
		int bankoffs = 0;
		switch (m_romboard)
		{
			case ROM_BOARD_171_5704:
				//
                //  D5 : Unused
                //  D4 : Unused
                //  D3 : ROM select 0=A11, 1=A12
                //  D2 : A16 for all ROMs
                //  D1 : A15 for all ROMs
                //  D0 : A14 for all ROMs
                //
				bankoffs = ((data & 0x08) >> 3) * 0x20000;
				bankoffs += (data & 0x07) * 0x4000;
				break;

			default:
				assert(false);
		}

		// set the final bank
		membank("soundbank")->set_base(memregion("soundcpu")->base() + 0x10000 + (bankoffs % size));
	}
}


//-------------------------------------------------
//  upd7759_status_r - return the uPD7759 busy
//  bit in the top bit
//-------------------------------------------------

READ8_MEMBER( segas16c_state::upd7759_status_r )
{
	return m_upd7759->busy_r() << 7;
}



//**************************************************************************
//  OTHER CALLBACKS
//**************************************************************************

//-------------------------------------------------
//  upd7759_generate_nmi - callback to signal an
//  NMI to the sound CPU
//-------------------------------------------------

void segas16c_state::upd7759_generate_nmi(device_t *device, int state)
{
	segas16c_state *driver = device->machine().driver_data<segas16c_state>();
	if (state)
		driver->m_soundcpu->set_input_line(INPUT_LINE_NMI, PULSE_LINE);
}


//-------------------------------------------------
//  i8751_main_cpu_vblank - update the fake i8751
//  state if we have a handler
//-------------------------------------------------

INTERRUPT_GEN_MEMBER( segas16c_state::i8751_main_cpu_vblank )
{
	// if we have a fake 8751 handler, call it on VBLANK
	if (!m_i8751_vblank_hook.isnull())
		m_i8751_vblank_hook();
}


//**************************************************************************
//  DRIVER OVERRIDES
//**************************************************************************

//-------------------------------------------------
//  machine_reset - reset the state of the machine
//-------------------------------------------------

void segas16c_state::machine_reset()
{
	// if we have a hard-coded mapping configuration, set it now
	if (m_i8751_initial_config != NULL)
		m_mapper->configure_explicit(m_i8751_initial_config);

	// queue up a timer to either boost interleave or disable the MCU
	synchronize(TID_INIT_I8751);

	// reset tilemap state
	m_segaic16vid->segaic16_tilemap_reset(*m_screen);

	// configure sprite banks
	static const UINT8 default_banklist[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	const UINT8 *banklist = default_banklist;
	for (int banknum = 0; banknum < 16; banknum++)
		m_sprites->set_bank(banknum, banklist[banknum]);
}


//-------------------------------------------------
//  device_timer - handle device timers
//-------------------------------------------------

void segas16c_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
		// if we have a fake i8751 handler, disable the actual 8751, otherwise crank the interleave
		case TID_INIT_I8751:
			if (!m_i8751_vblank_hook.isnull())
				m_mcu->suspend(SUSPEND_REASON_DISABLE, 1);
			else if (m_mcu != NULL)
				machine().scheduler().boost_interleave(attotime::zero, attotime::from_msec(10));
			break;
	}
}



//**************************************************************************
//  I8751 SIMULATIONS
//**************************************************************************

//-------------------------------------------------
//  fz2dx_i8751_sim - simulate the I8751
//-------------------------------------------------

void segas16c_state::fz2dx_i8751_sim()
{
	// signal a VBLANK to the main CPU
	m_maincpu->set_input_line(4, HOLD_LINE);
}



//**************************************************************************
//  MAIN CPU ADDRESS MAPS
//**************************************************************************

static ADDRESS_MAP_START( system16c_map, AS_PROGRAM, 16, segas16c_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0xffffff) AM_DEVREADWRITE8("mapper", sega_315_5195_mapper_device, read, write, 0x00ff)

	// these get overwritten by the memory mapper above, but we put them here
	// so they are properly allocated and tracked for saving
	AM_RANGE(0x100000, 0x1007ff) AM_RAM AM_SHARE("sprites")
	AM_RANGE(0x200000, 0x200fff) AM_RAM AM_SHARE("paletteram")
	AM_RANGE(0x300000, 0x30ffff) AM_RAM AM_SHARE("tileram")
	AM_RANGE(0x400000, 0x400fff) AM_RAM AM_SHARE("textram")
	AM_RANGE(0x500000, 0x53ffff) AM_RAM AM_SHARE("workram")	// only change from system16b_map
ADDRESS_MAP_END



//**************************************************************************
//  SOUND CPU ADDRESS MAPS
//**************************************************************************

static ADDRESS_MAP_START( sound_map, AS_PROGRAM, 8, segas16c_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xdfff) AM_ROMBANK("soundbank")
	AM_RANGE(0xe800, 0xe800) AM_READ(soundlatch_byte_r)
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_portmap, AS_IO, 8, segas16c_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x01) AM_MIRROR(0x3e) AM_DEVREADWRITE("ym2151", ym2151_device, read, write)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x3f) AM_WRITE(upd7759_control_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3f) AM_READ(upd7759_status_r) AM_DEVWRITE("upd", upd7759_device, port_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x3f) AM_READ(soundlatch_byte_r)
ADDRESS_MAP_END



//**************************************************************************
//  I8751 MCU ADDRESS MAPS
//**************************************************************************

static ADDRESS_MAP_START( mcu_io_map, AS_IO, 8, segas16c_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x001f) AM_MIRROR(0xff00) AM_DEVREADWRITE("mapper", sega_315_5195_mapper_device, read, write)
	AM_RANGE(MCS51_PORT_P1, MCS51_PORT_P1) AM_READ_PORT("SERVICE")
ADDRESS_MAP_END



/*************************************
 *
 *  Generic port definitions
 *
 *************************************/

static INPUT_PORTS_START( system16c_generic )
	PORT_START("SERVICE")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START("UNUSED")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWA:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Game-specific port definitions
 *
 *************************************/

 static INPUT_PORTS_START( fz2dx )
	PORT_INCLUDE( system16c_generic )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) ) PORT_DIPLOCATION("SWB:3,4")
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "Unlimited" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SWB:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SWB:7,8")
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
INPUT_PORTS_END



//**************************************************************************
//  SOUND CONFIGURATIONS
//**************************************************************************

static const upd775x_interface upd7759_config =
{
	&segas16c_state::upd7759_generate_nmi
};



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static GFXDECODE_START( segas16c )
	GFXDECODE_ENTRY( "gfx1", 0, gfx_8x8x3_planar,	0, 1024 )
GFXDECODE_END



/*************************************
 *
 *  Generic machine drivers
 *
 *************************************/

static MACHINE_CONFIG_START( system16c, segas16c_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, MASTER_CLOCK_10MHz)
	MCFG_CPU_PROGRAM_MAP(system16c_map)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", segas16c_state, irq4_line_hold)

	MCFG_CPU_ADD("soundcpu", Z80, MASTER_CLOCK_10MHz/2)
	MCFG_CPU_PROGRAM_MAP(sound_map)
	MCFG_CPU_IO_MAP(sound_portmap)

	MCFG_NVRAM_ADD_0FILL("nvram")

	MCFG_SEGA_315_5195_MAPPER_ADD("mapper", "maincpu", segas16c_state, memory_mapper, mapper_sound_r, mapper_sound_w)

	/* video hardware */
	MCFG_GFXDECODE(segas16c)
	MCFG_PALETTE_LENGTH(2048*3)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(MASTER_CLOCK_25MHz/4, 400, 0, 320, 262, 0, 224)
	MCFG_SCREEN_UPDATE_DRIVER(segas16c_state, screen_update)

	MCFG_SEGA_SYS16C_SPRITES_ADD("sprites")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("ymsnd", YM2151, MASTER_CLOCK_8MHz/2)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_SOUND_ADD("upd", UPD7759, UPD7759_STANDARD_CLOCK)
	MCFG_SOUND_CONFIG(upd7759_config)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( system16c_8751, system16c )
	MCFG_CPU_MODIFY("maincpu")
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", segas16c_state, i8751_main_cpu_vblank)

	MCFG_CPU_ADD("mcu", I8751, MASTER_CLOCK_8MHz)
	MCFG_CPU_IO_MAP(mcu_io_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", segas16c_state, irq0_line_pulse)
MACHINE_CONFIG_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
    Fantasy Zone II System16C
    CPU: 68000
    ROM Board: 171-5704 custom
*/
ROM_START( fz2dx )
	ROM_REGION( 0xc0000, "maincpu", 0 ) // 68000 code
	ROM_LOAD16_WORD_SWAP( "fz2_s16c.p00", 0x00000, 0x40000, CRC(B7D16C1D) SHA1(7587A0E4FA64664F53D7BA48D711B6D26ADD6220) )
	ROM_LOAD16_WORD_SWAP( "fz2_s16c.p01", 0x80000, 0x40000, CRC(2C47487C) SHA1(0E3A524DAE50E5B099396EF712CE45EA147B424B) )

	ROM_REGION( 0x60000, "gfx1", 0 ) // tiles
	ROM_LOAD( "fz2_s16c.bg",  0x00000, 0x60000, CRC(C092DC23) SHA1(CC7980B8AF9FED7A1CDF70CBBE6A2A67BB79594F) )

	ROM_REGION16_BE( 0x200000, "sprites", 0 ) // sprites
	ROM_LOAD( "fz2_s16c.obj", 0x000000, 0x200000, CRC(57753F79) SHA1(7566CEF4344FBE7FB7ADC476113CD6E8780AEEF4) )

	ROM_REGION( 0x50000, "soundcpu", 0 ) // sound CPU
	ROM_LOAD( "fz2_s16c.snd", 0x00000, 0x30000, CRC(0ED30EC1) SHA1(EDF2DBD6A35394849E0419C518C6FB0F4ACCB9D1) )

	ROM_REGION( 0x1000, "mcu", 0 ) // decryption key
	ROM_LOAD( "317-0000.bin", 0x00000, 0x1000, NO_DUMP )
ROM_END


//-------------------------------------------------
//  init_* - game-specific initialization
//-------------------------------------------------

DRIVER_INIT_MEMBER(segas16c_state,fz2dx_8751)
{
	DRIVER_INIT_CALL(generic_5704);
	m_i8751_vblank_hook = i8751_sim_delegate(FUNC(segas16c_state::fz2dx_i8751_sim), this);
}



//**************************************************************************
//  GAME DRIVERS
//**************************************************************************

//    YEAR, NAME,       PARENT,   MACHINE,             INPUT,    INIT,               MONITOR,COMPANY,FULLNAME,FLAGS
GAME( 1987, fz2dx, 0, system16c_8751, fz2dx,  segas16c_state,fz2dx_8751, ROT0, "Sega / M2", "Fantasy Zone II DX", 0 )
