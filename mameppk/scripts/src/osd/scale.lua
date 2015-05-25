if _OPTIONS["ARCHITECTURE"]~="_x64" then
	defines {
		"USE_MMX_INTERP_SCALE",
	}
end

includedirs {
	MAME_DIR .. "src/osd/windows/scale",
}
files {
	MAME_DIR .. "src/osd/windows/scale/scale.*",
	MAME_DIR .. "src/osd/windows/scale/2xpm.*",
	MAME_DIR .. "src/osd/windows/scale/2xsai.*",
	MAME_DIR .. "src/osd/windows/scale/hq2x.*",
	MAME_DIR .. "src/osd/windows/scale/hq3x.*",
	MAME_DIR .. "src/osd/windows/scale/scale2x.*",
	MAME_DIR .. "src/osd/windows/scale/scale3x.*",
	MAME_DIR .. "src/osd/windows/scale/scanline.*",
	MAME_DIR .. "src/osd/windows/scale/snes9x_render.*",
	MAME_DIR .. "src/osd/windows/scale/vba_hq2x.*",
	MAME_DIR .. "src/osd/windows/scale/xbrz.*",
}
