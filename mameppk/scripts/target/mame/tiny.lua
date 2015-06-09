-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   tiny.lua
--
--   Small driver-specific example makefile
--   Use make SUBTARGET=tiny to build
--
---------------------------------------------------------------------------


--------------------------------------------------
-- Specify all the CPU cores necessary for the
-- drivers referenced in tiny.lst.
--------------------------------------------------

CPUS["Z80"] = true
CPUS["M6502"] = true
CPUS["MCS48"] = true
CPUS["MCS51"] = true
CPUS["M6800"] = true
CPUS["M6809"] = true
CPUS["M680X0"] = true
CPUS["TMS9900"] = true
CPUS["COP400"] = true
CPUS["SH2"] = true
CPUS["SH4"] = true

--------------------------------------------------
-- Specify all the sound cores necessary for the
-- drivers referenced in tiny.lst.
--------------------------------------------------

SOUNDS["SAMPLES"] = true
SOUNDS["DAC"] = true
SOUNDS["DISCRETE"] = true
SOUNDS["AY8910"] = true
SOUNDS["YM2151"] = true
SOUNDS["ASTROCADE"] = true
SOUNDS["TMS5220"] = true
SOUNDS["OKIM6295"] = true
SOUNDS["HC55516"] = true
SOUNDS["YM3812"] = true
SOUNDS["CEM3394"] = true
SOUNDS["VOTRAX"] = true
SOUNDS["YMZ770"] = true
SOUNDS["YMF278B"] = true
SOUNDS["YMZ280B"] = true
SOUNDS["YM2610"] = true
SOUNDS["YM2203"] = true
SOUNDS["YM2608"] = true

--------------------------------------------------
-- specify available video cores
--------------------------------------------------

VIDEOS["BUFSPRITE"] = true
VIDEOS["EPIC12"] = true

--------------------------------------------------
-- specify available machine cores
--------------------------------------------------

MACHINES["6821PIA"] = true
MACHINES["TTL74148"] = true
MACHINES["TTL74153"] = true
MACHINES["TTL7474"] = true
MACHINES["RIOT6532"] = true
MACHINES["PIT8253"] = true
MACHINES["Z80CTC"] = true
MACHINES["68681"] = true
MACHINES["BANKDEV"] = true
MACHINES["EEPROMDEV"] = true
MACHINES["SERFLASH"] = true
MACHINES["RTC9701"] = true


--------------------------------------------------
-- specify available bus cores
--------------------------------------------------

BUSES["CENTRONICS"] = true

--------------------------------------------------
-- This is the list of files that are necessary
-- for building all of the drivers referenced
-- in tiny.lst
--------------------------------------------------

function createMAMEProjects(_target, _subtarget, _name)
	project (_name)
	targetsubdir(_target .."_" .. _subtarget)
	kind "StaticLib"
	uuid (os.uuid("drv-" .. _target .."_" .. _subtarget .. "_" .._name))
	
	options {
		"ForceCPP",
	}
	
	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/mame",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/zlib",
		GEN_DIR  .. "mame/layout",
	}
end

function createProjects_mame_tiny(_target, _subtarget)
	project ("mame_tiny")
	targetsubdir(_target .."_" .. _subtarget)
	kind (LIBTYPE)
	uuid (os.uuid("drv-mame-tiny"))
	
	options {
		"ForceCPP",
	}
	
	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/mame",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/zlib",
		GEN_DIR  .. "mame/layout",
	}	

	files{
		MAME_DIR .. "src/mame/machine/ticket.c",
		MAME_DIR .. "src/mame/drivers/carpolo.c",
		MAME_DIR .. "src/mame/machine/carpolo.c",
		MAME_DIR .. "src/mame/video/carpolo.c",
		MAME_DIR .. "src/mame/drivers/circus.c",
		MAME_DIR .. "src/mame/audio/circus.c",
		MAME_DIR .. "src/mame/video/circus.c",
		MAME_DIR .. "src/mame/drivers/exidy.c",
		MAME_DIR .. "src/mame/audio/exidy.c",
		MAME_DIR .. "src/mame/video/exidy.c",
		MAME_DIR .. "src/mame/audio/exidy440.c",
		MAME_DIR .. "src/mame/drivers/starfire.c",
		MAME_DIR .. "src/mame/video/starfire.c",
		MAME_DIR .. "src/mame/drivers/vertigo.c",
		MAME_DIR .. "src/mame/machine/vertigo.c",
		MAME_DIR .. "src/mame/video/vertigo.c",
		MAME_DIR .. "src/mame/drivers/victory.c",
		MAME_DIR .. "src/mame/video/victory.c",
		MAME_DIR .. "src/mame/audio/targ.c",
		MAME_DIR .. "src/mame/drivers/astrocde.c",
		MAME_DIR .. "src/mame/video/astrocde.c",
		MAME_DIR .. "src/mame/drivers/gridlee.c",
		MAME_DIR .. "src/mame/audio/gridlee.c",
		MAME_DIR .. "src/mame/video/gridlee.c",
		MAME_DIR .. "src/mame/drivers/williams.c",
		MAME_DIR .. "src/mame/machine/williams.c",
		MAME_DIR .. "src/mame/audio/williams.c",
		MAME_DIR .. "src/mame/video/williams.c",
		MAME_DIR .. "src/mame/audio/gorf.c",
		MAME_DIR .. "src/mame/audio/wow.c",
		MAME_DIR .. "src/mame/drivers/gaelco.c",
		MAME_DIR .. "src/mame/video/gaelco.c",
		MAME_DIR .. "src/mame/machine/gaelcrpt.c",
		MAME_DIR .. "src/mame/drivers/wrally.c",
		MAME_DIR .. "src/mame/machine/wrally.c",
		MAME_DIR .. "src/mame/video/wrally.c",
		MAME_DIR .. "src/mame/drivers/looping.c",
		MAME_DIR .. "src/mame/drivers/supertnk.c",
	}

createMAMEProjects(_target, _subtarget, "psikyo")
files {
	MAME_DIR .. "src/mame/drivers/psikyo.*",
	MAME_DIR .. "src/mame/video/psikyo.*",
	MAME_DIR .. "src/mame/drivers/psikyo4.*",
	MAME_DIR .. "src/mame/video/psikyo4.*",
	MAME_DIR .. "src/mame/drivers/psikyosh.*",
	MAME_DIR .. "src/mame/video/psikyosh.*",
}

createMAMEProjects(_target, _subtarget, "misc")
files {
	MAME_DIR .. "src/mame/drivers/cave.*",
	MAME_DIR .. "src/mame/video/cave.*",
	MAME_DIR .. "src/mame/drivers/cavepc.*",
	MAME_DIR .. "src/mame/drivers/cv1k.*",
}

createMAMEProjects(_target, _subtarget, "shared")
files {
	MAME_DIR .. "src/mame/machine/nmk112.*",
}
	
	--------------------------------------------------
	-- layout dependencies
	--------------------------------------------------

	dependency {
		{ MAME_DIR .. "src/mame/drivers/astrocde.c", GEN_DIR .. "mame/layout/gorf.lh" },
		{ MAME_DIR .. "src/mame/drivers/astrocde.c", GEN_DIR .. "mame/layout/seawolf2.lh" },
		{ MAME_DIR .. "src/mame/drivers/astrocde.c", GEN_DIR .. "mame/layout/spacezap.lh" },
		{ MAME_DIR .. "src/mame/drivers/astrocde.c", GEN_DIR .. "mame/layout/tenpindx.lh" },
		{ MAME_DIR .. "src/mame/drivers/circus.c", GEN_DIR .. "mame/layout/circus.lh" },
		{ MAME_DIR .. "src/mame/drivers/circus.c", GEN_DIR .. "mame/layout/crash.lh" },	
	}

	custombuildtask {
		layoutbuildtask("mame/layout", "crash"),
		layoutbuildtask("mame/layout", "circus"),
		layoutbuildtask("mame/layout", "tenpindx"),
		layoutbuildtask("mame/layout", "spacezap"),
		layoutbuildtask("mame/layout", "seawolf2"),
		layoutbuildtask("mame/layout", "gorf"),
	}	
end

function linkProjects_mame_tiny(_target, _subtarget)
	links {
		"mame_tiny",
		"psikyo",
		"misc",
		"shared",
	}
end
