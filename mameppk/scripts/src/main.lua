function mainProject(_target, _subtarget)
	if (_target == _subtarget) then
		project (_target .. _OPTIONS["EXTRA_SUFFIX"])
	else
		project (_target .. _OPTIONS["EXTRA_SUFFIX"] .. _subtarget)
	end	
	uuid (os.uuid(_target .. _OPTIONS["EXTRA_SUFFIX"] .."_" .. _subtarget))
	kind "ConsoleApp"

	options {
		"ForceCPP",
	}
	flags {
		"NoManifest",
		"Symbols", -- always include minimum symbols for executables 
	}

	if _OPTIONS["SYMBOLS"] then
		configuration { "mingw*" }
			postbuildcommands {
				"$(SILENT) echo Dumping symbols.",
				"$(SILENT) objdump --section=.text --line-numbers --syms --demangle $(TARGET) >$(subst .exe,.sym,$(TARGET))"
			}
	end
	
	configuration { "vs*" }
	flags {
		"Unicode",
	}

	configuration { "x64", "Release" }
		targetsuffix "64"
		if _OPTIONS["PROFILE"] then
			targetsuffix "64p"
		end

	configuration { "x64", "Debug" }
		targetsuffix "64d"
		if _OPTIONS["PROFILE"] then
			targetsuffix "64dp"
		end

	configuration { "x32", "Release" }
		targetsuffix ""
		if _OPTIONS["PROFILE"] then
			targetsuffix "p"
		end

	configuration { "x32", "Debug" }
		targetsuffix "d"
		if _OPTIONS["PROFILE"] then
			targetsuffix "dp"
		end

	configuration { "Native", "Release" }
		targetsuffix ""
		if _OPTIONS["PROFILE"] then
			targetsuffix "p"
		end

	configuration { "Native", "Debug" }
		targetsuffix "d"
		if _OPTIONS["PROFILE"] then
			targetsuffix "dp"
		end

	configuration { "mingw*" or "vs*" }
		targetextension ".exe"

	configuration { "asmjs" }
		targetextension ".bc"  

	configuration { }

	if _OPTIONS["SEPARATE_BIN"]~="1" then 
		targetdir(MAME_DIR)
	end
	
	findfunction("linkProjects_" .. _OPTIONS["target"] .. "_" .. _OPTIONS["subtarget"])(_OPTIONS["target"], _OPTIONS["subtarget"])
	links {
		"osd_" .. _OPTIONS["osd"],
		"bus",
		"optional",
		"emu",
		"dasm",
		"utils",
		"expat",
		"softfloat",
		"jpeg",
		"flac",
		"7z",
		"formats",
		"lua",
		"lsqlite3",
		"sqllite3",
		"zlib",
		"jsoncpp",
		"mongoose",
	}
	if _OPTIONS["NO_USE_MIDI"]=="0" then
		links {
			"portmidi",
		}
	end
	if (USE_BGFX == 1) then
		links {
			"bgfx"
		}
	end
	links{
		"ocore_" .. _OPTIONS["osd"],
	}
	
	override_resources = false;
	
	maintargetosdoptions(_target)

	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/" .. _target,
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/zlib",
		GEN_DIR  .. _target .. "/layout",
		GEN_DIR  .. "resource",
	}

	if _OPTIONS["targetos"]=="macosx" and (not override_resources) then
		linkoptions {
			"-sectcreate __TEXT __info_plist " .. GEN_DIR .. "/resource/" .. _target .. "-Info.plist"
		}
		custombuildtask {
			{ MAME_DIR .. "src/version.c" ,  GEN_DIR  .. "/resource/" .. _target .. "-Info.plist",    {  MAME_DIR .. "src/build/verinfo.py" }, {"@echo Emitting " .. _target .. "-Info.plist" .. "...",    PYTHON .. " $(1)  -p -b " .. _target .. " $(<) > $(@)" }},
		}
		dependency {
			{ "$(TARGET)" ,  GEN_DIR  .. "/resource/" .. _target .. "-Info.plist", true  },
		}

	end

	if _OPTIONS["targetos"]=="windows" and (not override_resources) then
		local rcfile = MAME_DIR .. "src/" .. _target .. "/osd/".._OPTIONS["osd"].."/" .. _target ..".rc"
		if not os.isfile(rcfile) then
			rcfile = MAME_DIR .. "src/" .. _target .. "/osd/windows/" .. _target ..".rc"
		end
		
		if os.isfile(rcfile) then
			files {
				rcfile,
			}
			dependency {
				{ "$(OBJDIR)/".._target ..".res" ,  GEN_DIR  .. "/resource/" .. _target .. "vers.rc", true  },
			}
		else
			files {
				MAME_DIR .. "src/osd/windows/mame.rc",
			}
			dependency {
				{ "$(OBJDIR)/mame.res" ,  GEN_DIR  .. "/resource/" .. _target .. "vers.rc", true  },
			}
		end	
	end

	files {
		MAME_DIR .. "src/".._target .."/" .. _target ..".c",
		MAME_DIR .. "src/version.c",
		GEN_DIR  .. _target .. "/" .. _subtarget .."/drivlist.c",
	}

	--FIXME
	make_drivlist(_target,_subtarget)
	
	configuration { "mingw*" }
		custombuildtask {	
			{ MAME_DIR .. "src/version.c" ,  GEN_DIR  .. "/resource/" .. _target .. "vers.rc",    {  MAME_DIR .. "src/build/verinfo.py" }, {"@echo Emitting " .. _target .. "vers.rc" .. "...",    PYTHON .. " $(1)  -r -b " .. _target .. " $(<) > $(@)" }},
		}	
	
	configuration { "vs*" }
		prebuildcommands {	
			"mkdir " .. path.translate(GEN_DIR  .. "/resource/","\\") .. " 2>NUL",
			"@echo Emitting ".. _target .. "vers.rc...",
			PYTHON .. " " .. path.translate(MAME_DIR .. "src/build/verinfo.py","\\") .. " -r -b " .. _target .. " " .. path.translate(MAME_DIR .. "src/version.c","\\") .. " > " .. path.translate(GEN_DIR  .. "/resource/" .. _target .. "vers.rc", "\\") ,
		}	
	
	 
	configuration { }

	debugdir (MAME_DIR)
	debugargs ("-window")
end

function mainProject_with_ui(_target, _subtarget)
	if (_target == _subtarget) then
		project (_target .. _OPTIONS["EXTRA_SUFFIX"] .. "ui")
	else
		project (_target .. _OPTIONS["EXTRA_SUFFIX"] .. _subtarget .. "ui")
	end	
	uuid (os.uuid(_target .. _OPTIONS["EXTRA_SUFFIX"] .."_" .. _subtarget .. "ui"))
	kind "WindowedApp"

	options {
		"ForceCPP",
	}
	flags {
		"NoManifest",
		"Symbols", -- always include minimum symbols for executables 
	}

	if _OPTIONS["SYMBOLS"] then
		configuration { "mingw*" }
			postbuildcommands {
				"$(SILENT) echo Dumping symbols.",
				"$(SILENT) objdump --section=.text --line-numbers --syms --demangle $(TARGET) >$(subst .exe,.sym,$(TARGET))"
			}
	end

	configuration { "mingw*-gcc" }
		linkoptions {
			"-municode",
			"-lmingw32",
			"-Wl,--allow-multiple-definition",
		}
		if _OPTIONS["MAME_AVI"]=="1" then
			links {
				"msvfw32",
				"avifil32",
				"avicap32",
			}
		end

	configuration { "vs*" }
	flags {
		"Unicode",
	}
	if _OPTIONS["MAME_AVI"]=="1" then
		links {
			"vfw32",
		}
	end

	configuration { "x64", "Release" }
		targetsuffix "64"
		if _OPTIONS["PROFILE"] then
			targetsuffix "64p"
		end

	configuration { "x64", "Debug" }
		targetsuffix "64d"
		if _OPTIONS["PROFILE"] then
			targetsuffix "64dp"
		end

	configuration { "x32", "Release" }
		targetsuffix ""
		if _OPTIONS["PROFILE"] then
			targetsuffix "p"
		end

	configuration { "x32", "Debug" }
		targetsuffix "d"
		if _OPTIONS["PROFILE"] then
			targetsuffix "dp"
		end

	configuration { "Native", "Release" }
		targetsuffix ""
		if _OPTIONS["PROFILE"] then
			targetsuffix "p"
		end

	configuration { "Native", "Debug" }
		targetsuffix "d"
		if _OPTIONS["PROFILE"] then
			targetsuffix "dp"
		end

	configuration { "mingw*" or "vs*" }
		targetextension ".exe"

	configuration { "asmjs" }
		targetextension ".bc"  

	configuration { }

	if _OPTIONS["SEPARATE_BIN"]~="1" then 
		targetdir(MAME_DIR)
	end

	if _OPTIONS["KAILLERA"]=="1" then
		links {
			"imm32",
		}
	end

	findfunction("linkProjects_" .. _OPTIONS["target"] .. "_" .. _OPTIONS["subtarget"])(_OPTIONS["target"], _OPTIONS["subtarget"])
	links {
		"osd_winui",
		"bus",
		"optional",
		"emu",
		"dasm",
		"utils",
		"expat",
		"softfloat",
		"jpeg",
		"flac",
		"7z",
		"formats",
		"lua",
		"lsqlite3",
		"sqllite3",
		"zlib",
		"jsoncpp",
		"mongoose",
	}
	if _OPTIONS["NO_USE_MIDI"]=="0" then
		links {
			"portmidi",
		}
	end
	if (USE_BGFX == 1) then
		links {
			"bgfx"
		}
	end
	links{
		"ocore_winui",
	}
	
	override_resources = false;
	
	maintargetosdoptions(_target)

	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/" .. _target,
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/zlib",
		GEN_DIR  .. _target .. "/layout",
		GEN_DIR  .. "resource",
	}

	override_resources = true;
	local rcfile = MAME_DIR .. "src/osd/winui/" .. _target .. "ui.rc"

	if not os.isfile(rcfile) then
		print(string.format("***** %s not found *****",rcfile))
		os.exit();
	end

	files {
		rcfile,
	}
	dependency {
		{ "$(OBJDIR)/".._target .."ui.res" ,  GEN_DIR  .. "/resource/" .. _target .. "vers.rc", true  },
	}

	files {
		MAME_DIR .. "src/".._target .."/" .. _target ..".c",
		MAME_DIR .. "src/version.c",
		GEN_DIR  .. _target .. "/" .. _subtarget .."/drivlist.c",
	}

	--FIXME
	make_drivlist(_target,_subtarget)

	configuration { "mingw*" }
		custombuildtask {	
			{ MAME_DIR .. "src/version.c" ,  GEN_DIR  .. "/resource/" .. _target .. "vers.rc",    {  MAME_DIR .. "src/build/verinfo.py" }, {"@echo Emitting " .. _target .. "vers.rc" .. "...",    PYTHON .. " $(1)  -r -b " .. _target .. " $(<) > $(@)" }},
		}	
	
	configuration { "vs*" }
		prebuildcommands {	
			"mkdir " .. path.translate(GEN_DIR  .. "/resource/","\\") .. " 2>NUL",
			"@echo Emitting ".. _target .. "vers.rc...",
			PYTHON .. " " .. path.translate(MAME_DIR .. "src/build/verinfo.py","\\") .. " -r -b " .. _target .. " " .. path.translate(MAME_DIR .. "src/version.c","\\") .. " > " .. path.translate(GEN_DIR  .. "/resource/" .. _target .. "vers.rc", "\\") ,
		}	
	
	 
	configuration { }

	debugdir (MAME_DIR)
	debugargs ("-window")
end

function make_drivlist(_target, _subtarget)
	if _subtarget=="mame" or _subtarget=="ncp" then
		if _subtarget=="mame" and _OPTIONS["MAMEMESS"]=="1" then
			custombuildtask {
				{ MAME_DIR .. "src/".._target .."/%.lst" ,  GEN_DIR  .. _target .. "/" .. _subtarget .."/%.lst",    {  MAME_DIR .. "src/build/makelist.py" }, {"@echo Generating $@...", "@echo #include \"$<\" > $@.h", "$(CC) $(DEFINES) -I. -E $@.h -o $@" }},
				{ MAME_DIR .. "src/mess/mess.lst" ,  GEN_DIR  .. _target .. "/" .. _subtarget .."/mess.lst",    {  MAME_DIR .. "src/build/makelist.py" }, {"@echo Generating $@...", "@echo #include \"$<\" > $@.h", "$(CC) $(DEFINES) -I. -E $@.h -o $@" }},
				{ GEN_DIR .. _target .. "/" .. _subtarget .."/mame.lst",  GEN_DIR  .. _target .. "/" .. _subtarget .."/drivlist.c",
					{ GEN_DIR .. _target .. "/" .. _subtarget .."/mamedecrypted.lst",
					  GEN_DIR .. _target .. "/" .. _subtarget .."/mamehb.lst",
					  GEN_DIR .. _target .. "/" .. _subtarget .."/mameplus.lst",
					  GEN_DIR .. _target .. "/" .. _subtarget .."/mess.lst",
					  MAME_DIR .. "src/build/makelist.py" }, {"@echo Building driver list...",    "echo " .. PYTHON .. " " .. MAME_DIR .. "src/build/makelist.py" .. " " .. _OPTIONS["USE_DRIVER_SWITCH"] .. " $^ > $@",PYTHON .. " " .. MAME_DIR .. "src/build/makelist.py" .. " " .. _OPTIONS["USE_DRIVER_SWITCH"] .. " $^ > $@" }},
			}
		else
			custombuildtask {
				{ MAME_DIR .. "src/".._target .."/%.lst" ,  GEN_DIR  .. _target .. "/" .. _subtarget .."/%.lst",    {  MAME_DIR .. "src/build/makelist.py" }, {"@echo Generating $@...", "@echo #include \"$<\" > $@.h", "$(CC) $(DEFINES) -I. -E $@.h -o $@" }},
				{ GEN_DIR .. _target .. "/" .. _subtarget .."/mame.lst",  GEN_DIR  .. _target .. "/" .. _subtarget .."/drivlist.c",
					{ GEN_DIR .. _target .. "/" .. _subtarget .."/mamedecrypted.lst",
					  GEN_DIR .. _target .. "/" .. _subtarget .."/mamehb.lst",
					  GEN_DIR .. _target .. "/" .. _subtarget .."/mameplus.lst",
					  MAME_DIR .. "src/build/makelist.py" }, {"@echo Building driver list...",    "echo " .. PYTHON .. " " .. MAME_DIR .. "src/build/makelist.py" .. " " .. _OPTIONS["USE_DRIVER_SWITCH"] .. " $^ > $@",PYTHON .. " " .. MAME_DIR .. "src/build/makelist.py" .. " " .. _OPTIONS["USE_DRIVER_SWITCH"] .. " $^ > $@" }},
			}
		end
	else
		custombuildtask {
			{ MAME_DIR .. "src/".._target .."/" .. _subtarget ..".lst" ,  GEN_DIR  .. _target .. "/" .. _subtarget .."/" .. _subtarget ..".lst",    {  MAME_DIR .. "src/build/makelist.py" }, {"@echo Generating $@...", "@echo #include \"$<\" > $@.h", "$(CC) -I. -E $@.h -o $@" }},
			{ GEN_DIR .. _target .."/" .. _subtarget .."/" .. _subtarget ..".lst" ,  GEN_DIR  .. _target .. "/" .. _subtarget .."/drivlist.c",    {  MAME_DIR .. "src/build/makelist.py" }, {"@echo Building driver list...",    PYTHON .. " " .. MAME_DIR .. "src/build/makelist.py" .. " " .. _OPTIONS["USE_DRIVER_SWITCH"] .. " $^ > $@" }},
		}
	end
end
