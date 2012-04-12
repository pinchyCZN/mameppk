/***************************************************************************

    Sun Electronics Arabian hardware

    driver by Dan Boris

    Games supported:
        * Arabian [2 sets]

    Known bugs:
        * none at this time

****************************************************************************

    Memory map

****************************************************************************

    ========================================================================
    CPU #1 (Arabian)
    ========================================================================
    0000-7FFF   R     xxxxxxxx    Program ROM
    8000-BFFF   R/W   xxxxxxxx    Bitmap RAM
    C000        R     ----xxxx    Coin inputs
    C200        R     ----xxxx    Option switches
    D000-DFFF   R/W   xxxxxxxx    Custom microprocessor RAM
    E000          W   ----xxxx    BSEL Bank select
    E001          W   xxxxxxxx    DMA ROM start address low
    E002          W   xxxxxxxx    DMA ROM start address high
    E003          W   xxxxxxxx    DMA RAM start address low
    E004          W   xxxxxxxx    DMA RAM start address high
    E005          W   xxxxxxxx    Picture size/DMA start low
    E006          W   xxxxxxxx    Picture size/DMA start high
    ========================================================================
    C800          W   xxxxxxxx    Sound chip address
    CA00        R/W   xxxxxxxx    Sound chip data
    ========================================================================
    Interrupts:
        NMI not connected
        IRQ generated by VBLANK
    ========================================================================

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/mb88xx/mb88xx.h"
#include "includes/arabian.h"
#include "sound/ay8910.h"

/* constants */
#define MAIN_OSC		XTAL_12MHz


/*************************************
 *
 *  Audio chip output ports
 *
 *************************************/

static WRITE8_DEVICE_HANDLER( ay8910_porta_w )
{
	arabian_state *state = device->machine().driver_data<arabian_state>();

	/*
        bit 7 = ENA
        bit 6 = ENB
        bit 5 = /ABHF
        bit 4 = /AGHF
        bit 3 = /ARHF
    */
	state->m_video_control = data;
}


static WRITE8_DEVICE_HANDLER( ay8910_portb_w )
{
	/*
        bit 5 = /IREQ to custom CPU
        bit 4 = /SRES to custom CPU
        bit 1 = coin 2 counter
        bit 0 = coin 1 counter
    */

	cputag_set_input_line(device->machine(), "mcu", MB88_IRQ_LINE, data & 0x20 ? CLEAR_LINE : ASSERT_LINE);
	cputag_set_input_line(device->machine(), "mcu", INPUT_LINE_RESET, data & 0x10 ? CLEAR_LINE : ASSERT_LINE);

	/* clock the coin counters */
	coin_counter_w(device->machine(), 1, ~data & 0x02);
	coin_counter_w(device->machine(), 0, ~data & 0x01);
}



/*************************************
 *
 *  Custom CPU (MB8841 MCU)
 *
 *************************************/

READ8_MEMBER(arabian_state::mcu_port_r_r)
{

	UINT8 val = m_mcu_port_r[offset];

	/* RAM mode is enabled */
	if (offset == 0)
		val |= 4;

	return val;
}

WRITE8_MEMBER(arabian_state::mcu_port_r_w)
{

	if (offset == 0)
	{
		UINT32 ram_addr = ((m_mcu_port_p & 7) << 8) | m_mcu_port_o;

		if (~data & 2)
			m_custom_cpu_ram[ram_addr] = 0xf0 | m_mcu_port_r[3];

		m_flip_screen = data & 8;
	}

	m_mcu_port_r[offset] = data & 0x0f;
}

READ8_MEMBER(arabian_state::mcu_portk_r)
{
	UINT8 val = 0xf;

	if (~m_mcu_port_r[0] & 1)
	{
		UINT32 ram_addr = ((m_mcu_port_p & 7) << 8) | m_mcu_port_o;
		val = m_custom_cpu_ram[ram_addr];
	}
	else
	{
		static const char *const comnames[] = { "COM0", "COM1", "COM2", "COM3", "COM4", "COM5" };
		UINT8 sel = ((m_mcu_port_r[2] << 4) | m_mcu_port_r[1]) & 0x3f;
		int i;

		for (i = 0; i < 6; ++i)
		{
			if (~sel & (1 << i))
			{
				val = input_port_read(machine(), comnames[i]);
				break;
			}
		}
	}

	return val & 0x0f;
}

WRITE8_MEMBER(arabian_state::mcu_port_o_w)
{
	UINT8 out = data & 0x0f;

	if (data & 0x10)
		m_mcu_port_o = (m_mcu_port_o & 0x0f) | (out << 4);
	else
		m_mcu_port_o = (m_mcu_port_o & 0xf0) | out;
}

WRITE8_MEMBER(arabian_state::mcu_port_p_w)
{
	m_mcu_port_p = data & 0x0f;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, AS_PROGRAM, 8, arabian_state )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(arabian_videoram_w)
	AM_RANGE(0xc000, 0xc000) AM_MIRROR(0x01ff) AM_READ_PORT("IN0")
	AM_RANGE(0xc200, 0xc200) AM_MIRROR(0x01ff) AM_READ_PORT("DSW1")
	AM_RANGE(0xd000, 0xd7ff) AM_MIRROR(0x0800) AM_RAM AM_BASE(m_custom_cpu_ram)
	AM_RANGE(0xe000, 0xe007) AM_MIRROR(0x0ff8) AM_WRITE(arabian_blitter_w) AM_BASE(m_blitter)
ADDRESS_MAP_END



/*************************************
 *
 *  Main CPU port handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_io_map, AS_IO, 8, arabian_state )
	AM_RANGE(0xc800, 0xc800) AM_MIRROR(0x01ff) AM_DEVWRITE_LEGACY("aysnd", ay8910_address_w)
	AM_RANGE(0xca00, 0xca00) AM_MIRROR(0x01ff) AM_DEVWRITE_LEGACY("aysnd", ay8910_data_w)
ADDRESS_MAP_END



/*************************************
 *
 *  MCU port handlers
 *
 *************************************/

static ADDRESS_MAP_START( mcu_io_map, AS_IO, 8, arabian_state )
	AM_RANGE(MB88_PORTK,  MB88_PORTK ) AM_READ(mcu_portk_r)
	AM_RANGE(MB88_PORTO,  MB88_PORTO ) AM_WRITE(mcu_port_o_w)
	AM_RANGE(MB88_PORTP,  MB88_PORTP ) AM_WRITE(mcu_port_p_w)
	AM_RANGE(MB88_PORTR0, MB88_PORTR3) AM_READWRITE(mcu_port_r_r, mcu_port_r_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( arabian )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )                    /* also adds 1 credit */
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )            PORT_DIPLOCATION("SW1:!1")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )          PORT_DIPLOCATION("SW1:!2")
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )      PORT_DIPLOCATION("SW1:!3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )       PORT_DIPLOCATION("SW1:!4")    /* Carry Bowls to Next Life */
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )             /* not reset "ARABIAN" letters */
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )             /* reset "ARABIAN" letters */
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coinage ) )          PORT_DIPLOCATION("SW1:!5,!6,!7,!8")
	PORT_DIPSETTING(    0x10, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x20, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x30, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x40, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x50, "A 1/1 B 1/4" )
	PORT_DIPSETTING(    0x60, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x70, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x80, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x90, "A 1/2 B 1/4" )
	PORT_DIPSETTING(    0xa0, "A 1/2 B 1/5" )
	PORT_DIPSETTING(    0xe0, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0xb0, "A 1/2 B 1/10" )
	PORT_DIPSETTING(    0xc0, "A 1/2 B 1/11" )
	PORT_DIPSETTING(    0xd0, "A 1/2 B 1/12" )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START("COM0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* IN3 : "AUX-S" */

	PORT_START("COM1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_8WAY

	PORT_START("COM2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* IN9 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* IN10 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* IN11 */

	PORT_START("COM3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_8WAY PORT_COCKTAIL

	PORT_START("COM4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* IN17 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* IN18 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* IN19 */

	PORT_START("COM5")
	PORT_DIPNAME( 0x01, 0x01, "Coin Counters" )             PORT_DIPLOCATION("SW2:!1")
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )      PORT_DIPLOCATION("SW2:!2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )       PORT_DIPLOCATION("SW2:!3,!4")
	PORT_DIPSETTING(    0x0c, "30k 70k 40k+" )              /* last bonus life at 870k : max. 22 bonus lives */
	PORT_DIPSETTING(    0x04, "20k only" )
	PORT_DIPSETTING(    0x08, "40k only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
INPUT_PORTS_END

static INPUT_PORTS_START( arabiana )
	PORT_INCLUDE( arabian )

	PORT_MODIFY("COM5")
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )       PORT_DIPLOCATION("SW2:!3,!4")
	PORT_DIPSETTING(    0x0c, "20k 50k 150k 100k+" )        /* last bonus life at 850k : max. 10 bonus lives */
	PORT_DIPSETTING(    0x04, "20k only" )
	PORT_DIPSETTING(    0x08, "40k only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static const ay8910_interface ay8910_config =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(ay8910_porta_w),
	DEVCB_HANDLER(ay8910_portb_w)
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_START( arabian )
{
	arabian_state *state = machine.driver_data<arabian_state>();

	state->save_item(NAME(state->m_mcu_port_o));
	state->save_item(NAME(state->m_mcu_port_p));
	state->save_item(NAME(state->m_mcu_port_r));
}

static MACHINE_RESET( arabian )
{
	arabian_state *state = machine.driver_data<arabian_state>();

	state->m_video_control = 0;
}

static MACHINE_CONFIG_START( arabian, arabian_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, MAIN_OSC/4)
	MCFG_CPU_PROGRAM_MAP(main_map)
	MCFG_CPU_IO_MAP(main_io_map)
	MCFG_CPU_VBLANK_INT("screen", irq0_line_hold)

	MCFG_CPU_ADD("mcu", MB8841, MAIN_OSC/3/2)
	MCFG_CPU_IO_MAP(mcu_io_map)

	MCFG_QUANTUM_TIME(attotime::from_hz(6000))

	MCFG_MACHINE_START(arabian)
	MCFG_MACHINE_RESET(arabian)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(256, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 255, 11, 244)
	MCFG_SCREEN_UPDATE_STATIC(arabian)

	MCFG_PALETTE_LENGTH(256*32)

	MCFG_PALETTE_INIT(arabian)
	MCFG_VIDEO_START(arabian)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("aysnd", AY8910, MAIN_OSC/4/2)
	MCFG_SOUND_CONFIG(ay8910_config)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( arabian )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ic1rev2.87", 0x0000, 0x2000, CRC(5e1c98b8) SHA1(1775b7b125dde3502aefcf6221662e82f55b3f2a) )
	ROM_LOAD( "ic2rev2.88", 0x2000, 0x2000, CRC(092f587e) SHA1(a722a61d35629ff4087c7a5e4c98b3ab51d6322b) )
	ROM_LOAD( "ic3rev2.89", 0x4000, 0x2000, CRC(15145f23) SHA1(ae250116b57455ed84948cd9a6bdda86b2ac3e16) )
	ROM_LOAD( "ic4rev2.90", 0x6000, 0x2000, CRC(32b77b44) SHA1(9d7951e723bc65e3d607f89836f1436b99f2585b) )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "tvg-91.ic84", 0x0000, 0x2000, CRC(c4637822) SHA1(0c73d9a4db925421a535784780ad93bb0f091051) )
	ROM_LOAD( "tvg-92.ic85", 0x2000, 0x2000, CRC(f7c6866d) SHA1(34f545c5f7c152cd59f7be0a72105f739852cd6a) )
	ROM_LOAD( "tvg-93.ic86", 0x4000, 0x2000, CRC(71acd48d) SHA1(cd0bffed351b14c9aebbfc1d3d4d232a5b91a68f) )
	ROM_LOAD( "tvg-94.ic87", 0x6000, 0x2000, CRC(82160b9a) SHA1(03511f6ebcf22ba709a80a565e71acf5bdecbabb) )

	ROM_REGION( 0x800, "mcu", 0 )
	ROM_LOAD( "sun-8212.ic3", 0x000, 0x800, CRC(8869611e) SHA1(c6443f3bcb0cdb4d7b1b19afcbfe339c300f36aa) )
ROM_END


ROM_START( arabiana )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tvg-87.ic1", 0x0000, 0x2000, CRC(51e9a6b1) SHA1(a2e6beab5380eed56972f5625be21b01c7e2082a) )
	ROM_LOAD( "tvg-88.ic2", 0x2000, 0x2000, CRC(1cdcc1ab) SHA1(46886d53cc8a1c1d540fd0e1ddf1811fb256c1f3) )
	ROM_LOAD( "tvg-89.ic3", 0x4000, 0x2000, CRC(b7b7faa0) SHA1(719418b7b7c057acb6d3060cf7061ffacf00798c) )
	ROM_LOAD( "tvg-90.ic4", 0x6000, 0x2000, CRC(dbded961) SHA1(ecc09fa95f6dd58c4ac0e095a89ffd3aae681da4) )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "tvg-91.ic84", 0x0000, 0x2000, CRC(c4637822) SHA1(0c73d9a4db925421a535784780ad93bb0f091051) )
	ROM_LOAD( "tvg-92.ic85", 0x2000, 0x2000, CRC(f7c6866d) SHA1(34f545c5f7c152cd59f7be0a72105f739852cd6a) )
	ROM_LOAD( "tvg-93.ic86", 0x4000, 0x2000, CRC(71acd48d) SHA1(cd0bffed351b14c9aebbfc1d3d4d232a5b91a68f) )
	ROM_LOAD( "tvg-94.ic87", 0x6000, 0x2000, CRC(82160b9a) SHA1(03511f6ebcf22ba709a80a565e71acf5bdecbabb) )

	ROM_REGION( 0x800, "mcu", 0 )
	ROM_LOAD( "sun-8212.ic3", 0x000, 0x800, CRC(8869611e) SHA1(c6443f3bcb0cdb4d7b1b19afcbfe339c300f36aa) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1983, arabian,  0,       arabian, arabian,  0, ROT270, "Sun Electronics",                 "Arabian",         GAME_SUPPORTS_SAVE )
GAME( 1983, arabiana, arabian, arabian, arabiana, 0, ROT270, "Sun Electronics (Atari license)", "Arabian (Atari)", GAME_SUPPORTS_SAVE )
