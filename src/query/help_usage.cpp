void use_day(){
	std::cout
		<<"Usage:\n"
		<<"  lunar day <bsp> <YYYY-MM-DD>\n"
		<<"    [--tz ...] [--format json|txt|csv|jsonl] [--out ...] [--pretty "
		  "0|1] [--quiet]\n"
		<<"    [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>] [--astro 0|1]\n"
		<<"    [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]\n"
		<<"    [--astro-lat deg --astro-lon deg [--astro-height m]]\n"
		<<"Examples:\n"
		<<"  lunar day D:\\de442.bsp 2025-06-01\n"
		<<"  lunar day D:\\de442.bsp 2025-06-01 --format json --out day.json\n"
		<<"Note:\n"
		<<"  Transit/occultation are topocentric and are emitted only when "
		  "--astro-lat/--astro-lon are provided.\n";
}

void use_mview(){
	std::cout<<"Usage:\n"
			 <<"  lunar monthview <bsp> <YYYY-MM>\n"
			 <<"    [--tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet] [--astro 0|1]\n"
			 <<"    [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]\n"
			 <<"    [--astro-lat deg --astro-lon deg [--astro-height m]]\n"
			 <<"Examples:\n"
			 <<"  lunar monthview D:\\de442.bsp 2025-09 --format txt\n"
			 <<"  lunar monthview D:\\de442.bsp 2025-09 --format csv --out "
			   "month.csv\n"
			 <<"Note:\n"
			 <<"  Transit/occultation are topocentric and are emitted only when "
			   "--astro-lat/--astro-lon are provided.\n";
}

void use_next(){
	std::cout<<"Usage:\n"
			 <<"  lunar next <bsp> --from <time> --count N\n"
			 <<"    [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse] [--tz ...]\n"
			 <<"    [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty "
			   "0|1] [--quiet] [--eclipse 0|1]\n"
			 <<"Examples:\n"
			 <<"  lunar next D:\\de442.bsp --from 2025-06-01T00:00:00+08:00 "
			   "--count 5\n"
			 <<"  lunar next D:\\de442.bsp --from 2025-06-01 --count 10 "
			   "--format ics --out next.ics\n";
}

void use_range(){
	std::cout<<"Usage:\n"
			 <<"  lunar range <bsp> --from <time> --to <time>\n"
			 <<"    [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse] [--tz ...]\n"
			 <<"    [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty "
			   "0|1] [--quiet] [--eclipse 0|1]\n"
			 <<"Examples:\n"
			 <<"  lunar range D:\\de442.bsp --from 2025-01-01 --to 2025-12-31\n"
			 <<"  lunar range D:\\de442.bsp --from 2025-02-01 --to 2025-03-01 "
			   "--format csv\n";
}

void use_search(){
	std::cout
		<<"Usage:\n"
		<<"  lunar search <bsp> <query> [--from <time>] [--count N]\n"
		<<"    [--tz ...] [--format json|txt|csv|ics|jsonl] [--out ...] "
		  "[--pretty 0|1] [--quiet] [--eclipse 0|1]\n"
		<<"Examples:\n"
		<<"  lunar search D:\\de442.bsp \"next full_moon\" --from 2025-06-01\n"
		<<"  lunar search D:\\de442.bsp \"next lunar_eclipse\" --from "
		  "2025-01-01 --format json\n";
}

void use_eclipse(){
	std::cout
		<<"Usage:\n"
		<<"  lunar [--eclipse-method modern|legacy] eclipse <bsp> --near <YYYY-MM-DD>\n"
		<<"  lunar eclipse <bsp> --near <YYYY-MM-DD> [--kind lunar|solar]\n"
		<<"    [--stage any|umb|total] (lunar) [--stage any|central] (solar)\n"
		<<"    [--sample-min <minutes>]\n"
		<<"    [--point-lat <deg> --point-lon <deg> [--point-height <m>]] "
		  "[--point-refine 0|1]\n"
		<<"    [--global-vis 0|1] [--grid-lat-step <deg>] [--grid-lon-step "
		  "<deg>] [--global-format json|geojson]\n"
		<<"    [--tz ...] [--format json|txt|geojson] [--out ...] [--pretty "
		  "0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar eclipse D:\\de442.bsp --near 2025-09-07 --format json\n"
		<<"  lunar eclipse D:\\de442.bsp --kind solar --near 2026-08-12 "
		  "--format json\n"
		<<"  lunar eclipse D:\\de442.bsp --near 2025-09-07 --global-vis 1 "
		  "--global-format geojson --format json\n"
		<<"  lunar eclipse D:\\de442.bsp --near 2025-09-07 --point-lat 31.23 "
		  "--point-lon 121.47 --point-height 10\n";
}

void use_fest(){
	std::cout<<"Usage:\n"
			 <<"  lunar festival <bsp> <year>\n"
			 <<"    [--tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar festival D:\\de442.bsp 2025\n"
			 <<"  lunar festival D:\\de442.bsp 2025 --format csv --out "
			   "festival.csv\n";
}

void use_alm(){
	std::cout<<"Usage:\n"
			 <<"  lunar almanac <bsp> <YYYY-MM-DD>\n"
			 <<"    [--tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet] [--lon <deg>]\n"
			 <<"Examples:\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17 --format json --out "
			   "almanac.json\n";
}

void use_info(){
	std::cout<<"Usage:\n"
			 <<"  lunar info <bsp> [--format json|txt] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar info D:\\de442.bsp\n"
			 <<"  lunar info D:\\de442.bsp --format json --out info.json\n";
}

void use_test(){
	std::cout
		<<"Usage:\n"
		<<"  lunar selftest <bsp> [--format json|txt] [--out ...] [--pretty "
		  "0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar selftest D:\\de442.bsp\n"
		<<"  lunar selftest D:\\de442.bsp --format json --out selftest.json\n";
}

void use_cfg(){
	std::cout<<"Usage:\n"
			 <<"  lunar config show [--format json|txt] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"  lunar config set <key> <value>\n"
			 <<"Keys:\n"
			 <<"  def_bsp | bsp_dir | bsp_list | default_tz | default_lang | def_fmt | "
			   "def_prety\n"
			 <<"Examples:\n"
			 <<"  lunar config show\n"
			 <<"  lunar config set default_tz +08:00\n"
			 <<"  lunar config set default_lang en\n";
}

void use_comp(){
	std::cout<<"Usage:\n"
			 <<"  lunar completion bash|zsh|fish|powershell\n"
			 <<"Examples:\n"
			 <<"  lunar completion powershell > lunar-completion.ps1\n"
			 <<"  lunar completion bash > lunar-completion.bash\n";
}

