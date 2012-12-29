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

#include "cpu/m68000/m68000.h"
#include "cpu/mcs51/mcs51.h"
#include "cpu/z80/z80.h"
#include "machine/nvram.h"
#include "machine/segaic16.h"
#include "sound/2151intf.h"
#include "sound/upd7759.h"
#include "video/segaic16.h"
#include "video/sega16sp.h"


// ======================> segas16c_state

class segas16c_state : public sega_16bit_common_base
{
public:
	// construction/destruction
	segas16c_state(const machine_config &mconfig, device_type type, const char *tag)
		: sega_16bit_common_base(mconfig, type, tag),
		  m_mapper(*this, "mapper"),
		  m_maincpu(*this, "maincpu"),
		  m_soundcpu(*this, "soundcpu"),
		  m_mcu(*this, "mcu"),
		  m_ymsnd(*this, "ymsnd"),
		  m_upd7759(*this, "upd"),
		  m_multiplier(*this, "multiplier"),
		  m_cmptimer_1(*this, "cmptimer_1"),
		  m_cmptimer_2(*this, "cmptimer_2"),
		  m_nvram(*this, "nvram"),
		  m_sprites(*this, "sprites"),
		  m_workram(*this, "workram"),
		  m_romboard(ROM_BOARD_INVALID),
		  m_tilemap_type(SEGAIC16_TILEMAP_16B),
		  m_disable_screen_blanking(false),
		  m_i8751_initial_config(NULL),
		  m_atomicp_sound_divisor(0),
		  m_atomicp_sound_count(0),
		  m_hwc_input_value(0),
		  m_mj_input_num(0),
		  m_mj_last_val(0)
	{ }

	// memory mapping
	void memory_mapper(sega_315_5195_mapper_device &mapper, UINT8 index);
	UINT8 mapper_sound_r();
	void mapper_sound_w(UINT8 data);

	// main CPU read/write handlers
	DECLARE_WRITE16_MEMBER( rom_5704_bank_w );
	DECLARE_READ16_MEMBER( rom_5797_bank_math_r );
	DECLARE_WRITE16_MEMBER( rom_5797_bank_math_w );
	DECLARE_READ16_MEMBER( unknown_rgn2_r );
	DECLARE_WRITE16_MEMBER( unknown_rgn2_w );
	DECLARE_READ16_MEMBER( standard_io_r );
	DECLARE_WRITE16_MEMBER( standard_io_w );
	DECLARE_WRITE16_MEMBER( atomicp_sound_w );

	// sound CPU read/write handlers
	DECLARE_WRITE8_MEMBER( upd7759_control_w );
	DECLARE_READ8_MEMBER( upd7759_status_r );

	// other callbacks
	static void upd7759_generate_nmi(device_t *device, int state);
	INTERRUPT_GEN_MEMBER( i8751_main_cpu_vblank );

	// ROM board-specific driver init
	DECLARE_DRIVER_INIT(generic_5704);

	// game-specific driver init
	DECLARE_DRIVER_INIT(fz2dx_8751);

	// video updates
	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	// wrappers for legacy functions (to be removed)
	template<write16_space_func _Legacy>
	WRITE16_MEMBER( legacy_wrapper ) { _Legacy(space, offset, data, mem_mask); }

protected:
	// internal types
	typedef delegate<void ()> i8751_sim_delegate;

	// timer IDs
	enum
	{
		TID_INIT_I8751
	};

	// rom board types
	enum segas16c_rom_board
	{
		ROM_BOARD_INVALID,
		ROM_BOARD_171_5704				// 171-5704 - don't know any diff between this and 171-5521
	};

	// device overrides
	virtual void video_start();
	virtual void machine_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	// internal helpers
	void init_generic(segas16c_rom_board rom_board);

	// i8751 simulations
	void fz2dx_i8751_sim();

	// devices
	required_device<sega_315_5195_mapper_device> m_mapper;
	required_device<m68000_device> m_maincpu;
	optional_device<z80_device> m_soundcpu;
	optional_device<i8751_device> m_mcu;
	optional_device<ym2151_device> m_ymsnd;
	optional_device<upd7759_device> m_upd7759;
	optional_device<sega_315_5248_multiplier_device> m_multiplier;
	optional_device<sega_315_5250_compare_timer_device> m_cmptimer_1;
	optional_device<sega_315_5250_compare_timer_device> m_cmptimer_2;
	required_device<nvram_device> m_nvram;
	required_device<sega_sys16c_sprite_device> m_sprites;

	// memory pointers
	required_shared_ptr<UINT16> m_workram;

	// configuration
	segas16c_rom_board	m_romboard;
	int					m_tilemap_type;
	read16_delegate		m_custom_io_r;
	write16_delegate	m_custom_io_w;
	bool				m_disable_screen_blanking;
	const UINT8 *		m_i8751_initial_config;
	i8751_sim_delegate	m_i8751_vblank_hook;
	UINT8				m_atomicp_sound_divisor;

	// game-specific state
	UINT8				m_atomicp_sound_count;
	UINT8				m_hwc_input_value;
	UINT8				m_mj_input_num;
	UINT8				m_mj_last_val;
};
