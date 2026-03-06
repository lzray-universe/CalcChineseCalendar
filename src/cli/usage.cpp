std::string tool_ver(){ return "lunar-cli-2026.02"; }

void use_month(){
	std::cout
		<<"Usage:\n"
		<<"  lunar months <bsp> <years>\n"
		<<"    [--mode lunar|gregorian]\n"
		<<"    [--format json|txt|csv] [--out <path>] [--tz +08:00|Z|-05:00]\n"
		<<"    [--pretty 0|1] [--quiet] [--include-eclipses 0|1]\n"
		<<"    [--output <json>] [--output-txt <txt>]   # deprecated\n"
		<<"Examples:\n"
		<<"  lunar months D:\\de442.bsp 2025\n"
		<<"  lunar months D:\\de442.bsp 2024-2026 --mode gregorian --format "
		  "json --out months.json\n"
		<<"  lunar months D:\\de442.bsp 2025 --format csv --out months.csv "
		  "--tz Z\n"
		<<"Notes:\n"
		<<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_cal(){
	std::cout
		<<"Usage:\n"
		<<"  lunar calendar <bsp> [<years>]\n"
		<<"    [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]\n"
		<<"    [--include-months 0|1] [--include-eclipses 0|1] [--pretty 0|1] "
		  "[--quiet]\n"
		<<"Examples:\n"
		<<"  lunar calendar D:\\de442.bsp 2025\n"
		<<"  lunar calendar D:\\de442.bsp 2024-2026 --format json --out "
		  "cal.json\n"
		<<"  lunar calendar D:\\de442.bsp 2025 --format ics --out cal.ics\n"
		<<"Notes:\n"
		<<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_year(){
	std::cout
		<<"Usage:\n"
		<<"  lunar year <bsp> <year>\n"
		<<"    [--mode lunar|gregorian]\n"
		<<"    [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]\n"
		<<"    [--pretty 0|1] [--quiet]\n"
		<<"Examples:\n"
		<<"  lunar year D:\\de442.bsp 2025\n"
		<<"  lunar year D:\\de442.bsp 2025 --format json --out year-2025.json\n"
		<<"  lunar year D:\\de442.bsp 2025 --format ics --out year-2025.ics\n"
		<<"Notes:\n"
		<<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_event(){
	std::cout<<"Usage:\n"
			 <<"  lunar event <bsp> solar-term <code> <year>\n"
			 <<"    [--format json|txt|ics] [--out <path>] [--tz "
			   "+08:00|Z|-05:00] [--pretty 0|1] [--quiet] [--eclipse 0|1]\n"
			 <<"  lunar event <bsp> lunar-phase "
			   "<new_moon|fst_qtr|full_moon|lst_qtr>\n"
			 <<"    --near <YYYY-MM-DD> [--format json|txt|ics] [--out <path>] "
			   "[--tz ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]\n"
			 <<"  lunar event <bsp> lunar-eclipse --near <YYYY-MM-DD>\n"
			 <<"    [--stage any|umb|total] [--point-lat ... --point-lon ...] "
			   "[--global-vis 0|1]\n"
			 <<"  lunar event <bsp> solar-eclipse --near <YYYY-MM-DD>\n"
			 <<"    [--stage any|central] [--point-lat ... --point-lon ...] "
			   "[--global-vis 0|1]\n"
			 <<"    [--format json|txt|geojson] [--out <path>] [--tz ...] "
			   "[--pretty 0|1] [--quiet]\n"
			 <<"Examples:\n"
			 <<"  lunar event D:\\de442.bsp solar-term Z2 2025\n"
			 <<"  lunar event D:\\de442.bsp lunar-phase full_moon --near "
			   "2025-09-07\n"
			 <<"  lunar event D:\\de442.bsp lunar-eclipse --near 2025-09-07 "
			   "--global-vis 1 --global-format geojson --format json\n"
			 <<"  lunar event D:\\de442.bsp solar-eclipse --near 2026-08-12 "
			   "--point-lat 40.7 --point-lon -74.0 --format json\n"
			 <<"  lunar event D:\\de442.bsp solar-term J1 2025 --format ics "
			   "--out event.ics\n"
			 <<"Notes:\n"
			 <<"  --tz only affects display formatting, not algorithm/rules.\n";
}

void use_dl(){
	std::cout<<"Usage:\n"
			 <<"  lunar download list\n"
			 <<"  lunar download get <id> [--dir <path>]\n"
			 <<"Examples:\n"
			 <<"  lunar download list\n"
			 <<"  lunar download get de442\n"
			 <<"  lunar download get de442s --dir D:\\ephem\n";
}

void use_main(){
	std::cout<<"Usage:\n"
			 <<"  lunar --help\n"
			 <<"  lunar --version\n"
			 <<"  lunar [--eclipse-method modern|legacy] <command> ...\n"
			 <<"  lunar months   ...\n"
			 <<"  lunar calendar ...\n"
			 <<"  lunar year     ...\n"
			 <<"  lunar event    ...\n"
			 <<"  lunar at       ...\n"
			 <<"  lunar convert  ...\n"
			 <<"  lunar day      ...\n"
			 <<"  lunar monthview...\n"
			 <<"  lunar next     ...\n"
			 <<"  lunar range    ...\n"
			 <<"  lunar search   ...\n"
			 <<"  lunar eclipse  ...\n"
			 <<"  lunar festival ...\n"
			 <<"  lunar almanac  ...\n"
			 <<"  lunar info     ...\n"
			 <<"  lunar selftest ...\n"
			 <<"  lunar config   ...\n"
			 <<"  lunar completion...\n"
			 <<"  lunar download ...\n"
			 <<"\n"
			 <<"Compatibility:\n"
			 <<"  lunar <bsp> <years> [months options...]  # same as months\n"
			 <<"\n"
			 <<"Subcommand help:\n"
			 <<"  lunar months --help\n"
			 <<"  lunar calendar --help\n"
			 <<"  lunar year --help\n"
			 <<"  lunar event --help\n"
			 <<"  lunar at --help\n"
			 <<"  lunar convert --help\n"
			 <<"  lunar day --help\n"
			 <<"  lunar monthview --help\n"
			 <<"  lunar next --help\n"
			 <<"  lunar range --help\n"
			 <<"  lunar search --help\n"
			 <<"  lunar eclipse --help\n"
			 <<"  lunar festival --help\n"
			 <<"  lunar almanac --help\n"
			 <<"  lunar info --help\n"
			 <<"  lunar selftest --help\n"
			 <<"  lunar config --help\n"
			 <<"  lunar completion --help\n"
			 <<"  lunar download --help\n";
	std::cout<<"Global option:\n"
			 <<"  --eclipse-method modern|legacy  (default: modern)\n";
	std::cout<<"  --lang zh|en|ja|ko  (default: from config default_lang)\n";
}

