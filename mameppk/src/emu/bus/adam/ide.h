// license:BSD-3-Clause
// copyright-holders:Curt Coder
/**********************************************************************

    Micro Innovations Powermate IDE Hard Disk emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __ADAM_IDE__
#define __ADAM_IDE__

#include "emu.h"
#include "exp.h"
#include "bus/centronics/ctronics.h"
#include "machine/ataintf.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> powermate_ide_device

class powermate_ide_device :  public device_t,
								public device_adam_expansion_slot_card_interface
{
public:
	// construction/destruction
	powermate_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device-level overrides
	virtual void device_start();

	// device_adam_expansion_slot_card_interface overrides
	virtual UINT8 adam_bd_r(address_space &space, offs_t offset, UINT8 data, int bmreq, int biorq, int aux_rom_cs, int cas1, int cas2);
	virtual void adam_bd_w(address_space &space, offs_t offset, UINT8 data, int bmreq, int biorq, int aux_rom_cs, int cas1, int cas2);

private:
	required_device<ata_interface_device> m_ata;
	required_device<centronics_device> m_centronics;

	UINT16 m_ata_data;
};


// device type definition
extern const device_type ADAM_IDE;



#endif
