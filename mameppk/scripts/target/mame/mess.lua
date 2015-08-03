-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   mamemess.lua
--
--   MESS target makefile
--
---------------------------------------------------------------------------

--------------------------------------------------
-- specify available sound cores; some of these are
-- only for MAME and so aren't included
--------------------------------------------------

SOUNDS["VRC6"] = true

--------------------------------------------------
-- specify available machine cores
--------------------------------------------------

MACHINES["YM2148"] = true

--------------------------------------------------
-- specify available bus cores
--------------------------------------------------

BUSES["A7800"] = true
BUSES["A800"] = true
BUSES["A8SIO"] = true
BUSES["GAMEBOY"] = true
BUSES["GAMEGEAR"] = true
BUSES["GBA"] = true
BUSES["GENERIC"] = true
BUSES["MIDI"] = true
BUSES["MEGADRIVE"] = true
BUSES["MSX_SLOT"] = true
BUSES["NEOGEO"] = true
BUSES["NES"] = true
BUSES["NES_CTRL"] = true
BUSES["PCE"] = true
BUSES["PSX_CONTROLLER"] = true
BUSES["SATURN"] = true
BUSES["SEGA8"] = true
BUSES["SMS_CTRL"] = true
BUSES["SMS_EXP"] = true
BUSES["SNES"] = true
BUSES["SNES_CTRL"] = true
BUSES["VCS"] = true
BUSES["VCS_CTRL"] = true
BUSES["WSWAN"] = true

--------------------------------------------------
-- this is the list of driver libraries that
-- comprise MESS plus messdriv.*", which contains
-- the list of drivers
--------------------------------------------------
function linkProjects_mame_mess(_target, _subtarget)
	links {
		"ascii",
		"atari",
		"bandai",
		"funtech",
		"nec",
		"nintendo",
		"sega",
		"snk",
		"sony",
	}
end

function createMESSProjects(_target, _subtarget, _name)
	project (_name)
	targetsubdir(_target .."_" .. _subtarget)
	kind (LIBTYPE)
	uuid (os.uuid("drv-" .. _target .."_" .. _subtarget .. "_" .._name))
	
	options {
		"ForceCPP",
	}
	
	defines {
		"MESS",
	}

	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/mess",
		MAME_DIR .. "src/mame",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "3rdparty",
		GEN_DIR  .. "mess/layout",
		GEN_DIR  .. "mame/layout",
		MAME_DIR .. "src/emu/cpu/m68000",
	}
	if _OPTIONS["with-bundled-zlib"] then
		includedirs {
			MAME_DIR .. "3rdparty/zlib",
		}
	end
end
	
function createProjects_mame_mess(_target, _subtarget)

createMESSProjects(_target, _subtarget, "ascii")
files {             
	MAME_DIR .. "src/mess/drivers/msx.c",
	MAME_DIR .. "src/mess/machine/msx.c",
	MAME_DIR .. "src/mess/machine/msx_matsushita.c",
	MAME_DIR .. "src/mess/machine/msx_s1985.c",
	MAME_DIR .. "src/mess/machine/msx_switched.c",
	MAME_DIR .. "src/mess/machine/msx_systemflags.c", 
}

createMESSProjects(_target, _subtarget, "atari")
files {             
	MAME_DIR .. "src/mess/drivers/a2600.c",     
	MAME_DIR .. "src/mess/drivers/a7800.c",
	MAME_DIR .. "src/mess/video/maria.c", 
	MAME_DIR .. "src/mess/drivers/atari400.c",
	MAME_DIR .. "src/mess/machine/atarifdc.c", 
}

createMESSProjects(_target, _subtarget, "bandai")
files {            
	MAME_DIR .. "src/mess/drivers/wswan.c",
	MAME_DIR .. "src/mess/audio/wswan_snd.c",
	MAME_DIR .. "src/mess/machine/wswan.c",
	MAME_DIR .. "src/mess/video/wswan_video.c", 
}
	
	--------------------------------------------------
	-- layout dependencies
	--------------------------------------------------

	dependency {
		{ MAME_DIR .. "src/mess/drivers/wswan.c", GEN_DIR .. "mess/layout/wswan.lh" },
	}

	custombuildtask {
		layoutbuildtask("mess/layout", "wswan"),
	}

createMESSProjects(_target, _subtarget, "funtech")
files {           
	MAME_DIR .. "src/mess/drivers/supracan.c",  
}

createMESSProjects(_target, _subtarget, "nec")
files {               
	MAME_DIR .. "src/mess/drivers/pce.c",
	MAME_DIR .. "src/mess/machine/pce.c",
	MAME_DIR .. "src/mess/machine/pce_cd.c", 
}

createMESSProjects(_target, _subtarget, "nintendo")
files {          
	MAME_DIR .. "src/mess/drivers/gb.c",
	MAME_DIR .. "src/mess/audio/gb.c",
	MAME_DIR .. "src/mess/machine/gb.c",
	MAME_DIR .. "src/mess/video/gb_lcd.c", 
	MAME_DIR .. "src/mess/drivers/gba.c",
	MAME_DIR .. "src/mess/video/gba.c", 
	MAME_DIR .. "src/mess/drivers/nes.c",
	MAME_DIR .. "src/mess/machine/nes.c",
	MAME_DIR .. "src/mess/video/nes.c", 
	MAME_DIR .. "src/mess/drivers/snes.c",
	MAME_DIR .. "src/mess/machine/snescx4.c", 
}

createMESSProjects(_target, _subtarget, "sega")
files {              
	MAME_DIR .. "src/mess/drivers/megadriv.c",  
	MAME_DIR .. "src/mess/drivers/saturn.c",    
	MAME_DIR .. "src/mess/drivers/segapico.c",  
	MAME_DIR .. "src/mess/drivers/sms.c",
	MAME_DIR .. "src/mess/machine/sms.c", 
	MAME_DIR .. "src/mess/machine/mega32x.c",   
	MAME_DIR .. "src/mess/machine/megacd.c",    
	MAME_DIR .. "src/mess/machine/megacdcd.c",  
}
	
	--------------------------------------------------
	-- layout dependencies
	--------------------------------------------------

	dependency {
		{ MAME_DIR .. "src/mess/drivers/sms.c", GEN_DIR .. "mess/layout/sms1.lh" },
		{ MAME_DIR .. "src/mess/machine/megacd.c", GEN_DIR .. "mess/layout/megacd.lh" },
	}

	custombuildtask {
		layoutbuildtask("mess/layout", "sms1"),
		layoutbuildtask("mess/layout", "megacd"),
	}

createMESSProjects(_target, _subtarget, "snk")
files {               
	MAME_DIR .. "src/mess/drivers/ng_aes.c",    
	MAME_DIR .. "src/mess/drivers/ngp.c",
	MAME_DIR .. "src/mess/video/k1ge.c", 
}

createMESSProjects(_target, _subtarget, "sony")
files {              
	MAME_DIR .. "src/mess/drivers/pockstat.c",
	MAME_DIR .. "src/mess/drivers/psx.c",
	MAME_DIR .. "src/mess/machine/psxcd.c", 
}

end
