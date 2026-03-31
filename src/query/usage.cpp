void use_at(){
	std::cout
		<<"Usage:\n"
		<<"  lunar at <bsp> <time>\n"
		<<"  lunar at <bsp> --time <time>\n"
		<<"  lunar at <bsp> --stdin\n"
		<<"  lunar at <bsp> --file <path>\n"
		<<"    [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
		<<"    [--lunar-day-tz Z|+08:00|-05:00]\n"
		<<"    [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] "
		  "[--quiet] [--events 0|1] [--eot-lon <deg>] [--trad "
		  "folk|ziping|purple|xieji]\n"
		<<"    [--year-boundary lichun|lunar_new_year|dongzhi] "
		  "[--month-boundary solar_term|lunar_first_day]\n"
		<<"    [--leap-month-mode ignore|inherit_previous|split_midway|"
		  "shift_to_next] [--day-boundary hour23|hour0]\n"
		<<"    [--jobs N] [--meta-once 0|1]\n"
		<<"Time formats:\n"
		<<"  YYYY-MM-DD\n"
		<<"  YYYY-MM-DDTHH:MM\n"
		<<"  YYYY-MM-DDTHH:MM:SS[.sss]\n"
		<<"  optional timezone suffix: Z or +HH:MM/-HH:MM\n"
		<<"Examples:\n"
		<<"  lunar at D:\\de442.bsp 2025-06-01T00:00:00+08:00 --format json\n"
		<<"  lunar at D:\\de442.bsp --time 2025-06-01T00:00 --input-tz +08:00 "
		  "--tz Z --lunar-day-tz +09:00\n"
		<<"  lunar at D:\\de442.bsp 2025-01-31T12:00:00+08:00 --trad ziping\n"
		<<"  lunar at D:\\de442.bsp 2025-06-01T00:00:00+08:00 --eot-lon "
		  "116.391\n"
		<<"  lunar at D:\\de442.bsp --file times.txt --format jsonl "
		  "--meta-once 1\n"
		<<"Notes:\n"
		<<"  --input-tz only parses input without timezone suffix; --tz only "
		  "affects display.\n"
		<<"  --lunar-day-tz controls which civil-day boundary is used for lunar "
		  "date mapping.\n"
		<<"  --eot-lon uses east-positive degrees; output is apparent - mean "
		  "solar time.\n";
}

void use_conv(){
	std::cout<<"Usage:\n"
			 <<"  lunar convert <bsp> <dt_or_tm>\n"
			 <<"  lunar convert <bsp> --from-lunar <year> <month_no> <day> "
			   "[--leap 0|1]\n"
			 <<"  lunar convert <bsp> --stdin\n"
			 <<"  lunar convert <bsp> --file <path>\n"
			 <<"    [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]\n"
			 <<"    [--lunar-day-tz Z|+08:00|-05:00]\n"
			 <<"    [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] "
			   "[--quiet] [--jobs N] [--meta-once 0|1]\n";
}

void use_day(){
	std::cout
		<<"Usage:\n"
		<<"  lunar day [bsp] <YYYY-MM-DD>\n"
		<<"    [--tz ...] [--lunar-day-tz ...] [--format json|txt|csv|jsonl] [--out ...] [--pretty "
		  "0|1] [--quiet]\n"
		<<"    [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>] [--trad "
		  "folk|ziping|purple|xieji]\n"
		<<"    [--year-boundary lichun|lunar_new_year|dongzhi] "
		  "[--month-boundary solar_term|lunar_first_day]\n"
		<<"    [--leap-month-mode ignore|inherit_previous|split_midway|"
		  "shift_to_next] [--day-boundary hour23|hour0]\n"
		<<"    [--astro 0|1]\n"
		<<"    [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]\n"
		<<"    [--astro-lat deg --astro-lon deg [--astro-height m]]\n"
		<<"Examples:\n"
		<<"  lunar day D:\\de442.bsp 2025-06-01\n"
		<<"  lunar day D:\\de442.bsp 2025-01-31 --trad ziping\n"
		<<"  lunar day D:\\de442.bsp 2025-01-31 --trad four_pillars\n"
		<<"  lunar day D:\\de442.bsp 2025-01-31 --lunar-day-tz +09:00\n"
		<<"  lunar day D:\\de442.bsp 2025-06-01 --format json --out day.json\n"
		<<"Note:\n"
		<<"  `--trad/--year-boundary/--month-boundary/--leap-month-mode/--day-boundary` "
		  "also accept common ASCII aliases.\n"
		<<"  Transit/occultation are topocentric and are emitted only when "
		  "--astro-lat/--astro-lon are provided.\n";
}

void use_mview(){
	std::cout<<"Usage:\n"
			 <<"  lunar monthview [bsp] <YYYY-MM>\n"
			 <<"    [--tz ...] [--lunar-day-tz ...] [--format json|txt|csv] [--out ...] [--pretty "
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
			 <<"  lunar next [bsp] --from <time> --count N\n"
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
			 <<"  lunar range [bsp] --from <time> --to <time>\n"
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
		<<"  lunar search [bsp] <query> [--from <time>] [--count N]\n"
		<<"    [--tz ...] [--format json|txt|csv|ics|jsonl] [--out ...] "
		  "[--pretty 0|1] [--quiet] [--eclipse 0|1]\n"
		<<"Examples:\n"
		<<"  lunar search D:\\de442.bsp \"next full_moon\" --from 2025-06-01\n"
		<<"  lunar search D:\\de442.bsp \"next lunar_eclipse\" --from "
		  "2025-01-01 --format json\n";
}

void use_zodiac(){
	std::cout
		<<"Usage:\n"
		<<"  lunar zodiac <bsp> --time <time>\n"
		<<"    [--input-tz ...] [--tz ...] [--format json|txt|csv] "
		  "[--out ...] [--pretty 0|1] [--quiet]\n"
		<<"  lunar zodiac <bsp> --year <year>\n"
		<<"    [--tz ...] [--format json|txt|csv] [--out ...] "
		  "[--pretty 0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar zodiac D:\\de442.bsp --time 2025-03-20T18:01:00+08:00\n"
		<<"  lunar zodiac D:\\de442.bsp --year 2025 --format csv\n"
		<<"Notes:\n"
		<<"  Solar zodiac uses apparent geocentric solar ecliptic longitude "
		  "with light-time correction.\n"
		<<"  In --year mode, --tz defines both the display timezone and the "
		  "civil-year window used for duration clipping.\n";
}

void use_eclipse(){
	std::cout
		<<"Usage:\n"
		<<"  lunar [--eclipse-method modern|legacy] eclipse [bsp] --near <YYYY-MM-DD>\n"
		<<"  lunar eclipse [bsp] --near <YYYY-MM-DD> [--kind lunar|solar]\n"
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
			 <<"  lunar festival [bsp] <year>\n"
			 <<"    [--tz ...] [--lunar-day-tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar festival D:\\de442.bsp 2025\n"
			 <<"  lunar festival D:\\de442.bsp 2025 --format csv --out "
			   "festival.csv\n";
}

void use_alm(){
	std::cout<<"Usage:\n"
			 <<"  lunar almanac [bsp] <YYYY-MM-DD>\n"
			 <<"    [--tz ...] [--lunar-day-tz ...] [--format json|txt|csv] [--out ...] [--pretty "
			   "0|1] [--quiet] [--lon <deg>] [--trad folk|ziping|purple|xieji]\n"
			 <<"    [--year-boundary lichun|lunar_new_year|dongzhi] "
			   "[--month-boundary solar_term|lunar_first_day]\n"
			 <<"    [--leap-month-mode ignore|inherit_previous|split_midway|"
			   "shift_to_next] [--day-boundary hour23|hour0]\n"
			 <<"Examples:\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17 --trad xieji\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17 --trad old_almanac\n"
			 <<"  lunar almanac D:\\de442.bsp 2025-09-17 --format json --out "
			   "almanac.json\n"
			 <<"Note:\n"
			 <<"  Rule options accept stable keys and common ASCII aliases.\n";
}

void use_info(){
	std::cout<<"Usage:\n"
			 <<"  lunar info [bsp] [--format json|txt] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar info D:\\de442.bsp\n"
			 <<"  lunar info D:\\de442.bsp --format json --out info.json\n";
}

void use_cfg(){
	std::cout<<"Usage:\n"
			 <<"  lunar config show [--format json|txt] [--out ...] [--pretty "
			   "0|1] [--quiet]\n"
			 <<"  lunar config set <key> <value>\n"
			 <<"Keys:\n"
			 <<"  def_bsp | bsp_dir | bsp_list | default_tz | default_lang | default_lunar_day_tz | def_fmt | "
			   "hli_trad | hli_year_boundary | hli_month_boundary | "
			   "hli_leap_month_mode | hli_day_boundary | def_prety\n"
			 <<"Examples:\n"
			 <<"  lunar config show\n"
			 <<"  lunar config set default_tz +08:00\n"
			 <<"  lunar config set default_lunar_day_tz +09:00\n"
			 <<"  lunar config set hli_trad xieji\n"
			 <<"  lunar config set hli_trad old_almanac\n"
			 <<"  lunar config set hli_day_boundary hour0\n"
			 <<"  lunar config set hli_day_boundary default\n"
			 <<"  lunar config set default_lang en\n";
}

void use_comp(){
	std::cout<<"Usage:\n"
			 <<"  lunar completion bash|zsh|fish|powershell\n"
			 <<"Examples:\n"
			 <<"  lunar completion powershell > lunar-completion.ps1\n"
			 <<"  lunar completion bash > lunar-completion.bash\n";
}

