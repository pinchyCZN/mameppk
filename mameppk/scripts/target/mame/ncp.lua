---------------------------------------------------------------------------
--
--   ncp.lua
--
--   Small driver-specific example makefile
--   Use make SUBTARGET=ncp to build
--
--   Copyright Nicola Salmoria and the MAME Team.
--   Visit  http://mamedev.org for licensing and usage restrictions.
--
---------------------------------------------------------------------------


--------------------------------------------------
-- Specify all the CPU cores necessary for the
-- drivers referenced in tiny.c.
--------------------------------------------------

CPUS["Z80"] = true
CPUS["I386"] = true
CPUS["MCS48"] = true
CPUS["M680X0"] = true
CPUS["ARM7"] = true
CPUS["SH2"] = true
CPUS["PIC16C5X"] = true
CPUS["DSP16A"] = true

--------------------------------------------------
-- Specify all the sound cores necessary for the
-- drivers referenced in tiny.c.
--------------------------------------------------

SOUNDS["YM2151"] = true
SOUNDS["YM2203"] = true
SOUNDS["YM2608"] = true
SOUNDS["YM2610"] = true
SOUNDS["MSM5205"] = true
SOUNDS["OKIM6295"] = true
SOUNDS["QSOUND"] = true
SOUNDS["CDDA"] = true
SOUNDS["ICS2115"] = true
SOUNDS["YMZ770"] = true
if _OPTIONS["WINUI"] == "1" then
SOUNDS["VLM5030"] = true
end

--------------------------------------------------
-- specify available video cores
--------------------------------------------------

VIDEOS["BUFSPRITE"] = true
VIDEOS["EPIC12"] = true

--------------------------------------------------
-- specify available machine cores
--------------------------------------------------

MACHINES["EEPROMDEV"] = true
MACHINES["INTELFLASH"] = true
MACHINES["WD33C93"] = true
MACHINES["TIMEKPR"] = true
MACHINES["SCSI"] = true
MACHINES["V3021"] = true
MACHINES["PD4990A_OLD"] = true
MACHINES["UPD1990A"] = true
MACHINES["Z80CTC"] = true
MACHINES["I8255"] = true

--------------------------------------------------
-- specify available bus cores
--------------------------------------------------

BUSES["SCSI"] = true
BUSES["NEOGEO"] = true

--------------------------------------------------
-- This is the list of files that are necessary
-- for building all of the drivers referenced
-- in tiny.c
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

function createProjects_mame_ncp(_target, _subtarget)
	project ("mame_ncp")
	targetsubdir(_target .."_" .. _subtarget)
	kind "StaticLib"
	uuid (os.uuid("drv-mame-ncp"))
	
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

createMAMEProjects(_target, _subtarget, "capcom")
files {
	MAME_DIR .. "src/mame/drivers/cps1.c",
	MAME_DIR .. "src/mame/video/cps1.c",
	MAME_DIR .. "src/mame/drivers/kenseim.c",
	MAME_DIR .. "src/mame/drivers/cps2.c",
	MAME_DIR .. "src/mame/machine/cps2crpt.c",
	MAME_DIR .. "src/mame/drivers/cps3.c",
	MAME_DIR .. "src/mame/audio/cps3.c",
	MAME_DIR .. "src/mame/drivers/fcrash.c",
	MAME_DIR .. "src/mame/machine/kabuki.c",
}

createMAMEProjects(_target, _subtarget, "igs")
files {
	MAME_DIR .. "src/mame/drivers/pgm.c",
	MAME_DIR .. "src/mame/video/pgm.c",
	MAME_DIR .. "src/mame/machine/pgmprot_igs027a_type1.c",
	MAME_DIR .. "src/mame/machine/pgmprot_igs027a_type2.c",
	MAME_DIR .. "src/mame/machine/pgmprot_igs027a_type3.c",
	MAME_DIR .. "src/mame/machine/pgmprot_igs025_igs012.c",
	MAME_DIR .. "src/mame/machine/pgmprot_igs025_igs022.c",
	MAME_DIR .. "src/mame/machine/pgmprot_igs025_igs028.c",
	MAME_DIR .. "src/mame/machine/pgmprot_orlegend.c",
	MAME_DIR .. "src/mame/drivers/pgm2.c",
	MAME_DIR .. "src/mame/machine/igs036crypt.c",
	MAME_DIR .. "src/mame/machine/pgmcrypt.c",
	MAME_DIR .. "src/mame/machine/igs025.c",
	MAME_DIR .. "src/mame/machine/igs022.c",
	MAME_DIR .. "src/mame/machine/igs028.c",
}

createMAMEProjects(_target, _subtarget, "neogeo")
files {
	MAME_DIR .. "src/mame/drivers/neogeo.c",
	MAME_DIR .. "src/mame/video/neogeo.c",
	MAME_DIR .. "src/mame/drivers/neogeo_noslot.c",
	MAME_DIR .. "src/mame/video/neogeo_spr.c",
	MAME_DIR .. "src/mame/machine/neocrypt.c",
	MAME_DIR .. "src/mame/machine/ng_memcard.c",
}

	--------------------------------------------------
	-- layout dependencies
	--------------------------------------------------

	dependency {
		{ MAME_DIR .. "src/mame/drivers/neogeo.c", GEN_DIR .. "mame/layout/neogeo.lh" },
		{ MAME_DIR .. "src/mame/drivers/cps3.c", GEN_DIR .. "mame/layout/sfiii2.lh" },
		{ MAME_DIR .. "src/mame/drivers/kenseim.c", GEN_DIR .. "mame/layout/kenseim.lh" },
	}

	custombuildtask {
		layoutbuildtask("mame/layout", "neogeo"),
		layoutbuildtask("mame/layout", "sfiii2"),
		layoutbuildtask("mame/layout", "kenseim"),
	}	
end

function linkProjects_mame_ncp(_target, _subtarget)
	links {
		"mame_ncp",
		"capcom",
		"igs",
		"neogeo",
	}
end
