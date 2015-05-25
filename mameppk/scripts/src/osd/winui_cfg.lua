defines {
	"UNICODE",
	"_UNICODE",
	"OSD_WINDOWS",
	"USE_SDL=0",
	--"WIN95_MULTIMON"
	"main=utf8_main",
	"_WIN32_WINNT=0x0501",
}

configuration { "vs*" }
	flags {
		"Unicode",
	}

configuration { }

if not _OPTIONS["DONT_USE_NETWORK"] then
	defines {
		"USE_NETWORK",
		"OSD_NET_USE_PCAP",
	}
end

