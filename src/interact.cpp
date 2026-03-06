#include "lunar/interact.hpp"

#include<cctype>
#include<functional>
#include<fstream>
#include<iostream>
#include<stdexcept>
#include<vector>

#include "lunar/i18n.hpp"
#include "lunar/i18n_interact.hpp"

namespace fs=std::filesystem;

const std::string CFG_FILE="lun_cfg.txt";

std::vector<std::string> split_bsp_list(const std::string&text){
	std::vector<std::string> out;
	std::string token;
	auto flush=[&](){
		std::string t=trim(token);
		if(!t.empty()){
			out.push_back(t);
		}
		token.clear();
	};
	for(char ch : text){
		if(ch==','||ch==';'){
			flush();
		}else{
			token.push_back(ch);
		}
	}
	flush();
	return out;
}

std::string join_bsp_list(const std::vector<std::string>&items){
	std::string out;
	for(std::size_t i=0;i<items.size();++i){
		if(i!=0){
			out.push_back(';');
		}
		out+=items[i];
	}
	return out;
}

void add_bsp_if_missing(InterCfg&cfg,const std::string&path){
	if(path.empty()){
		return;
	}
	if(path==cfg.def_bsp){
		return;
	}
	for(const auto&item : cfg.bsp_list){
		if(item==path){
			return;
		}
	}
	cfg.bsp_list.push_back(path);
}

std::string trim(const std::string&s){
	std::size_t start=0;
	while(start<s.size()&&std::isspace(static_cast<unsigned char>(s[start]))){
		++start;
	}
	std::size_t end=s.size();
	while(end>start&&std::isspace(static_cast<unsigned char>(s[end-1]))){
		--end;
	}
	return s.substr(start,end-start);
}

bool load_cfg(InterCfg&cfg){
	std::ifstream ifs(CFG_FILE);
	if(!ifs){
		return false;
	}
	std::string line;
	while(std::getline(ifs,line)){
		auto pos=line.find('=');
		if(pos==std::string::npos){
			continue;
		}
		std::string key=trim(line.substr(0,pos));
		std::string value=trim(line.substr(pos+1));
		if(key=="bsp_dir"){
			cfg.bsp_dir=value;
		}else if(key=="def_bsp"){
			cfg.def_bsp=value;
		}else if(key=="bsp_list"){
			cfg.bsp_list=split_bsp_list(value);
		}else if(key=="default_tz"){
			cfg.default_tz=value;
		}else if(key=="default_lang"){
			cfg.default_lang=value;
		}else if(key=="def_fmt"){
			cfg.def_fmt=value;
		}else if(key=="def_prety"){
			cfg.def_prety=(value=="1"||value=="true"||value=="yes");
		}
	}
	return true;
}

bool save_cfg(const InterCfg&cfg){
	std::ofstream ofs(CFG_FILE);
	if(!ofs){
		return false;
	}
	ofs<<"bsp_dir="<<cfg.bsp_dir<<"\n";
	ofs<<"def_bsp="<<cfg.def_bsp<<"\n";
	ofs<<"bsp_list="<<join_bsp_list(cfg.bsp_list)<<"\n";
	ofs<<"default_tz="<<cfg.default_tz<<"\n";
	ofs<<"default_lang="<<cfg.default_lang<<"\n";
	ofs<<"def_fmt="<<cfg.def_fmt<<"\n";
	ofs<<"def_prety="<<(cfg.def_prety?"1":"0")<<"\n";
	return true;
}

bool file_ok(const std::string&path){
	std::error_code ec;
	return fs::exists(path,ec);
}

std::vector<fs::path> find_bsps(const std::vector<fs::path>&dirs){
	std::vector<fs::path> files;
	std::error_code ec;
	for(const auto&d : dirs){
		if(!fs::exists(d,ec)||!fs::is_directory(d,ec)){
			continue;
		}
		for(const auto&entry : fs::directory_iterator(d,ec)){
			if(entry.is_regular_file()){
				if(entry.path().extension()==".bsp"){
					files.push_back(entry.path());
				}
			}
		}
	}
	return files;
}

std::string ask_line(const std::string&msg){
	std::cout<<msg;
	std::string line;
	if(!std::getline(std::cin,line)){
		if(std::cin.eof()){
			throw std::runtime_error("interactive input closed (EOF)");
		}
		throw std::runtime_error("failed to read interactive input");
	}
	return line;
}

bool ask_yes_no(const std::string&msg,bool yes_def){
	std::string suffix=yes_def?"(Y/n)":"(y/N)";
	std::string line=ask_line(msg+suffix+" ");
	if(line.empty()){
		return yes_def;
	}
	char c=static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
	if(c=='y'){
		return true;
	}
	if(c=='n'){
		return false;
	}
	return yes_def;
}

std::string done_back_msg(){
	return lunar::i18n::interact::text("done_back");
}

std::string back_msg(){
	return lunar::i18n::interact::text("back");
}

std::string itx(const std::string&key){
	return lunar::i18n::interact::text(key);
}

std::string itx(const std::string&key,const std::string&a0){
	return lunar::i18n::interact::textf(key,a0);
}

std::string pick_bsp(InterCfg&cfg){
	auto options=bsp_opts();
	std::cout<<"\n"<<lunar::i18n::pick("可下载的 BSP 星历：",
										 "Downloadable BSP ephemerides:",
										 "ダウンロード可能な BSP 星暦:",
										 "다운로드 가능한 BSP 천체력:")
			 <<"\n";
	for(std::size_t i=0;i<options.size();++i){
		std::cout<<"["<<(i+1)<<"] "<<options[i].id<<"  ("<<options[i].size<<", "
				 <<options[i].range<<")"<<std::endl;
	}
	std::cout<<lunar::i18n::pick("输入编号或星历名称进行下载（输入 q 返回）：",
								 "Enter index or id to download (q to back): ",
								 "番号またはIDを入力してダウンロード（q で戻る）: ",
								 "번호 또는 ID 입력 후 다운로드 (q로 돌아가기): ");
	std::string sel;
	std::getline(std::cin,sel);
	if(sel.empty()||sel=="q"||sel=="Q"){
		return "";
	}

	const BspOption*chosen=nullptr;
	if(std::isdigit(static_cast<unsigned char>(sel[0]))){
		int idx=std::stoi(sel);
		if(idx>=1&&static_cast<std::size_t>(idx)<=options.size()){
			chosen=&options[idx-1];
		}
	}
	if(!chosen){
		for(const auto&opt : options){
			if(opt.id==sel){
				chosen=&opt;
				break;
			}
		}
	}
	if(!chosen){
		std::cout<<lunar::i18n::pick("未找到对应的星历选项。",
									 "No matching ephemeris option found.",
									 "該当する星暦オプションが見つかりません。",
									 "해당 천체력 옵션을 찾지 못했습니다.")
				 <<std::endl;
		return "";
	}

	std::string def_dir=
		cfg.bsp_dir.empty()?fs::current_path().string():cfg.bsp_dir;
	std::string dir=ask_line(
		lunar::i18n::pick("请输入保存目录（默认为 ","Save directory (default ",
						  "保存先ディレクトリ（既定値 ", "저장 디렉터리(기본값 ")
		+def_dir+lunar::i18n::pick("）：","): ","): ","): "));
	if(dir.empty()){
		dir=def_dir;
	}

	std::error_code ec;
	fs::create_directories(dir,ec);
	fs::path target_dir=fs::path(dir);
	fs::path filename=fs::path(chosen->url).filename();
	fs::path target=target_dir/filename;

	if(!dl_file(chosen->url,target.string())){
		std::cout<<lunar::i18n::pick("下载失败，请检查网络或下载工具。",
									 "Download failed. Check network or downloader.",
									 "ダウンロードに失敗しました。ネットワークまたはツールを確認してください。",
									 "다운로드 실패. 네트워크 또는 다운로드 도구를 확인하세요.")
				 <<std::endl;
		return "";
	}

	std::cout<<lunar::i18n::pick("下载完成：","Downloaded: ","ダウンロード完了: ",
								 "다운로드 완료: ")
			 <<target<<std::endl;
	cfg.bsp_dir=target_dir.string();
	cfg.def_bsp=target.string();
	add_bsp_if_missing(cfg,cfg.def_bsp);
	save_cfg(cfg);
	return cfg.def_bsp;
}

std::string init_bsp(InterCfg&cfg){
	load_cfg(cfg);
	if(!cfg.def_bsp.empty()&&file_ok(cfg.def_bsp)){
		if(ask_yes_no(lunar::i18n::pick("检测到上次使用的星历文件：",
										"Detected previous ephemeris: ",
										"前回使用した星暦: ",
										"이전에 사용한 천체력: ")+cfg.def_bsp+
						 lunar::i18n::pick("，是否继续使用？",
										". Continue using it?",
										"。このまま使用しますか？",
										" 계속 사용하시겠습니까?"),
					  true)){
			return cfg.def_bsp;
		}
	}

	fs::path cwd=fs::current_path();
	std::vector<fs::path> src_dirs;
	src_dirs.push_back(cwd);

	std::string dir_input=ask_line(
		lunar::i18n::pick("当前搜索目录为：","Current search directory: ",
						  "現在の検索ディレクトリ: ","현재 검색 디렉터리: ")+
		cwd.string()+
		lunar::i18n::pick("。是否指定额外的 BSP 目录？(直接回车跳过，或输入路径)：",
						  ". Specify an extra BSP directory? (Enter to skip or input path): ",
						  "。追加の BSP ディレクトリを指定しますか？（Enter でスキップ、またはパス入力）: ",
						  ". 추가 BSP 디렉터리를 지정할까요? (Enter로 건너뛰기 또는 경로 입력): "));
	if(!dir_input.empty()){
		fs::path p(dir_input);
		if(fs::exists(p)){
			cfg.bsp_dir=p.string();
			src_dirs.push_back(p);
			save_cfg(cfg);
		}else{
			std::cout<<lunar::i18n::pick("目录不存在，将继续使用当前目录搜索。",
										 "Directory does not exist. Continue with current directory.",
										 "ディレクトリが存在しないため、現在のディレクトリで続行します。",
										 "디렉터리가 없어 현재 디렉터리에서 계속 검색합니다.")
					 <<std::endl;
		}
	}

	std::vector<fs::path> bsp_files=find_bsps(src_dirs);
	if(!bsp_files.empty()){
		std::cout<<lunar::i18n::pick("找到以下 BSP 文件：",
									 "Found BSP files:",
									 "次の BSP ファイルが見つかりました:",
									 "다음 BSP 파일을 찾았습니다:")
				 <<std::endl;
		for(std::size_t i=0;i<bsp_files.size();++i){
			std::cout<<"["<<(i+1)<<"] "<<bsp_files[i].filename().string()<<"  ("
					 <<bsp_files[i].string()<<")"<<std::endl;
		}
		std::cout<<lunar::i18n::pick("请选择要使用的星历文件编号（或输入 0 进入下载界面）：",
									 "Choose a BSP index (0 to open downloader): ",
									 "使用する星暦番号を選択（0 でダウンロード画面）: ",
									 "사용할 BSP 번호를 선택하세요 (0은 다운로드 화면): ");
		std::string sel;
		std::getline(std::cin,sel);
		if(!sel.empty()&&std::isdigit(static_cast<unsigned char>(sel[0]))){
			int idx=std::stoi(sel);
			if(idx==0){
				std::string downloaded=pick_bsp(cfg);
				if(!downloaded.empty()){
					return downloaded;
				}
			}else if(idx>=1&&static_cast<std::size_t>(idx)<=bsp_files.size()){
				cfg.def_bsp=bsp_files[idx-1].string();
				add_bsp_if_missing(cfg,cfg.def_bsp);
				if(cfg.bsp_dir.empty()){
					cfg.bsp_dir=bsp_files[idx-1].parent_path().string();
				}
				save_cfg(cfg);
				return cfg.def_bsp;
			}
		}
	}

	std::cout<<lunar::i18n::pick("未找到 BSP 文件，将进入下载界面。",
								 "No BSP found. Opening downloader.",
								 "BSP が見つからないためダウンロード画面を開きます。",
								 "BSP 파일을 찾지 못해 다운로드 화면으로 이동합니다.")
			 <<std::endl;
	return pick_bsp(cfg);
}

void int_month(const std::string&ephem){
	MonthsArgs margs;
	margs.ephem=ephem;
	while(true){
		margs.years=
			ask_line(lunar::i18n::pick("请输入年份或范围（如 2024 或 2020-2025，必填）：",
									   "Enter year or range (e.g. 2024 or 2020-2025, required): ",
									   "年または範囲を入力してください（例: 2024 / 2020-2025、必須）: ",
									   "연도 또는 범위를 입력하세요 (예: 2024 또는 2020-2025, 필수): "));
		if(!margs.years.empty()){
			break;
		}
		std::cout<<lunar::i18n::pick("年份不能为空，请重新输入。",
									 "Year cannot be empty. Please retry.",
									 "年は必須です。再入力してください。",
									 "연도는 비워둘 수 없습니다. 다시 입력하세요.")
				 <<std::endl;
	}

	std::string mode_input=
		ask_line(lunar::i18n::pick(
			"选择输出模式：1) 农历(lunar)  2) 公历(gregorian) ，直接回车默认 lunar：",
			"Select output mode: 1) lunar 2) gregorian (Enter for lunar): ",
			"出力モード: 1) lunar 2) gregorian（Enter で lunar）: ",
			"출력 모드 선택: 1) lunar 2) gregorian (Enter 시 lunar): "));
	if(mode_input=="2"||mode_input=="gregorian"){
		margs.mode="gregorian";
	}else{
		margs.mode="lunar";
	}

	margs.out_json=ask_line(lunar::i18n::pick(
		"可选：输出 JSON 文件路径（留空则不输出）：",
		"Optional: output JSON file path (empty to skip): ",
		"任意: JSON 出力先（空で出力しない）: ",
		"선택: JSON 출력 파일 경로 (비우면 출력 안 함): "));
	margs.out_txt=ask_line(lunar::i18n::pick(
		"可选：输出文本文件路径（留空则不输出）：",
		"Optional: output text file path (empty to skip): ",
		"任意: テキスト出力先（空で出力しない）: ",
		"선택: 텍스트 출력 파일 경로 (비우면 출력 안 함): "));

	cli_month(margs);
	ask_line(done_back_msg());
}

void int_cal(const std::string&ephem){
	CalArgs cargs;
	cargs.ephem=ephem;
	std::string years=ask_line(lunar::i18n::pick(
		"可选：输入年份或范围（留空使用默认）：",
		"Optional: input year or range (empty for default): ",
		"任意: 年または範囲を入力（空で既定値）: ",
		"선택: 연도 또는 범위 입력 (비우면 기본값): "));
	if(!years.empty()){
		cargs.has_years=true;
		cargs.years_arg=years;
	}
	cli_cal(cargs);
	ask_line(done_back_msg());
}

void int_at(const std::string&ephem){
	AtArgs aargs;
	aargs.ephem=ephem;
	while(true){
		aargs.time_raw=ask_line(
			lunar::i18n::pick("请输入查询时刻（例如 2025-06-01T00:00:00+08:00，必填）：",
							  "Enter query time (e.g. 2025-06-01T00:00:00+08:00, required): ",
							  "照会時刻を入力（例: 2025-06-01T00:00:00+08:00、必須）: ",
							  "조회 시각 입력 (예: 2025-06-01T00:00:00+08:00, 필수): "));
		if(!aargs.time_raw.empty()){
			break;
		}
		std::cout<<lunar::i18n::pick("时刻不能为空，请重新输入。",
									 "Time cannot be empty. Please retry.",
									 "時刻は必須です。再入力してください。",
									 "시각은 비워둘 수 없습니다. 다시 입력하세요.")
				 <<std::endl;
	}

	std::string input_tz=ask_line(lunar::i18n::pick(
		"若输入不带时区，解析时区（默认 +08:00）：",
		"Input timezone when time has no TZ (default +08:00): ",
		"入力値にタイムゾーンが無い場合の解釈 TZ（既定 +08:00）: ",
		"입력값에 타임존이 없을 때 해석 TZ (기본 +08:00): "));
	if(!input_tz.empty()){
		aargs.input_tz=input_tz;
	}
	std::string display_tz=ask_line(lunar::i18n::pick(
		"输出显示时区（默认 +08:00）：",
		"Display timezone (default +08:00): ",
		"表示タイムゾーン（既定 +08:00）: ",
		"표시 타임존 (기본 +08:00): "));
	if(!display_tz.empty()){
		aargs.tz=display_tz;
	}
	std::string fmt=ask_line(lunar::i18n::pick(
		"输出格式：1) txt  2) json（默认 txt）：",
		"Output format: 1) txt 2) json (default txt): ",
		"出力形式: 1) txt 2) json（既定 txt）: ",
		"출력 형식: 1) txt 2) json (기본 txt): "));
	if(fmt=="2"||fmt=="json"){
		aargs.format="json";
	}
	std::string near_ev=ask_line(lunar::i18n::pick(
		"是否输出附近节气/四相：1)是 0)否（默认1）：",
		"Include nearby terms/phases? 1)yes 0)no (default 1): ",
		"近傍の節気/月相を出力しますか？ 1)はい 0)いいえ（既定1）: ",
		"근처 절기/월상을 출력할까요? 1)예 0)아니오 (기본 1): "));
	if(!near_ev.empty()){
		aargs.events=(near_ev!="0");
	}
	aargs.out=ask_line(lunar::i18n::pick(
		"可选：输出文件路径（留空则输出到控制台）：",
		"Optional: output file path (empty for console): ",
		"任意: 出力ファイルパス（空でコンソール出力）: ",
		"선택: 출력 파일 경로 (비우면 콘솔 출력): "));

	cli_at(aargs);
	ask_line(done_back_msg());
}

void int_conv(const std::string&ephem){
	ConvArgs cargs;
	cargs.ephem=ephem;

	std::string mode=
		ask_line(lunar::i18n::pick(
			"转换方向：1) 公历->农历  2) 农历->公历（默认1）：",
			"Direction: 1) Gregorian->Lunar 2) Lunar->Gregorian (default 1): ",
			"変換方向: 1) 西暦->旧暦 2) 旧暦->西暦（既定1）: ",
			"변환 방향: 1) 양력->음력 2) 음력->양력 (기본 1): "));
	if(mode=="2"){
		cargs.from_lunar=true;
		cargs.lunar_year=std::stoi(ask_line(lunar::i18n::pick(
			"请输入农历年份（如 2025）：",
			"Enter lunar year (e.g. 2025): ",
			"旧暦の年を入力（例: 2025）: ",
			"음력 연도 입력 (예: 2025): ")));
		cargs.lun_mno=std::stoi(ask_line(lunar::i18n::pick(
			"请输入农历月（1-12）：",
			"Enter lunar month (1-12): ",
			"旧暦の月を入力（1-12）: ",
			"음력 월 입력 (1-12): ")));
		cargs.lunar_day=std::stoi(ask_line(lunar::i18n::pick(
			"请输入农历日（1-30）：",
			"Enter lunar day (1-30): ",
			"旧暦の日を入力（1-30）: ",
			"음력 일 입력 (1-30): ")));
		std::string leap=ask_line(lunar::i18n::pick(
			"是否闰月：1)是 0)否（默认0）：",
			"Leap month? 1)yes 0)no (default 0): ",
			"閏月ですか？ 1)はい 0)いいえ（既定0）: ",
			"윤달인가요? 1)예 0)아니오 (기본 0): "));
		cargs.leap=(leap=="1");
	}else{
		while(true){
			cargs.in_value=ask_line(
				lunar::i18n::pick("请输入公历日期/时刻（例如 2026-02-18 或 2025-06-01T00:00）：",
								  "Enter Gregorian date/time (e.g. 2026-02-18 or 2025-06-01T00:00): ",
								  "西暦日付/時刻を入力（例: 2026-02-18 または 2025-06-01T00:00）: ",
								  "양력 날짜/시각 입력 (예: 2026-02-18 또는 2025-06-01T00:00): "));
			if(!cargs.in_value.empty()){
				cargs.has_in=true;
				break;
			}
			std::cout<<lunar::i18n::pick("输入不能为空，请重新输入。",
										 "Input cannot be empty. Please retry.",
										 "入力は必須です。再入力してください。",
										 "입력값은 비워둘 수 없습니다. 다시 입력하세요.")
					 <<std::endl;
		}
		std::string input_tz=
			ask_line(lunar::i18n::pick(
				"若输入不带时区，解析时区（默认 +08:00）：",
				"Input timezone when value has no TZ (default +08:00): ",
				"入力値にタイムゾーンが無い場合の解釈 TZ（既定 +08:00）: ",
				"입력값에 타임존이 없을 때 해석 TZ (기본 +08:00): "));
		if(!input_tz.empty()){
			cargs.input_tz=input_tz;
		}
	}

	std::string display_tz=ask_line(lunar::i18n::pick(
		"输出显示时区（默认 +08:00）：",
		"Display timezone (default +08:00): ",
		"表示タイムゾーン（既定 +08:00）: ",
		"표시 타임존 (기본 +08:00): "));
	if(!display_tz.empty()){
		cargs.tz=display_tz;
	}
	std::string fmt=ask_line(lunar::i18n::pick(
		"输出格式：1) txt  2) json（默认 txt）：",
		"Output format: 1) txt 2) json (default txt): ",
		"出力形式: 1) txt 2) json（既定 txt）: ",
		"출력 형식: 1) txt 2) json (기본 txt): "));
	if(fmt=="2"||fmt=="json"){
		cargs.format="json";
	}
	cargs.out=ask_line(lunar::i18n::pick(
		"可选：输出文件路径（留空则输出到控制台）：",
		"Optional: output file path (empty for console): ",
		"任意: 出力ファイルパス（空でコンソール出力）: ",
		"선택: 출력 파일 경로 (비우면 콘솔 출력): "));

	cli_conv(cargs);
	ask_line(done_back_msg());
}

void run_dint(const std::string&ephem){
	std::string date=ask_line(lunar::i18n::pick(
		"请输入公历日期 YYYY-MM-DD：",
		"Enter Gregorian date YYYY-MM-DD: ",
		"西暦日付 YYYY-MM-DD を入力: ",
		"양력 날짜 YYYY-MM-DD 입력: "));
	if(date.empty()){
		throw std::invalid_argument(lunar::i18n::pick(
			"日期不能为空","date cannot be empty",
			"日付は必須です","날짜는 비워둘 수 없습니다"));
	}
	std::vector<std::string> args={ephem,date};
	std::string fmt=ask_line(lunar::i18n::pick(
		"输出格式：1) txt 2) json（默认 txt）：",
		"Output format: 1) txt 2) json (default txt): ",
		"出力形式: 1) txt 2) json（既定 txt）: ",
		"출력 형식: 1) txt 2) json (기본 txt): "));
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}
	cmd_day(args);
	ask_line(done_back_msg());
}

void run_nint(const std::string&ephem){
	std::string from=
		ask_line(lunar::i18n::pick(
			"请输入起始时刻（例如 2025-06-01T00:00:00+08:00）：",
			"Enter start time (e.g. 2025-06-01T00:00:00+08:00): ",
			"開始時刻を入力（例: 2025-06-01T00:00:00+08:00）: ",
			"시작 시각 입력 (예: 2025-06-01T00:00:00+08:00): "));
	if(from.empty()){
		throw std::invalid_argument(lunar::i18n::pick(
			"起始时刻不能为空","start time cannot be empty",
			"開始時刻は必須です","시작 시각은 비워둘 수 없습니다"));
	}
	std::string count=ask_line(lunar::i18n::pick(
		"事件数量（默认 5）：",
		"Event count (default 5): ",
		"イベント件数（既定 5）: ",
		"이벤트 개수 (기본 5): "));
	if(count.empty()){
		count="5";
	}
	std::vector<std::string> args={ephem,"--from",from,"--count",count};
	cmd_next(args);
	ask_line(done_back_msg());
}

void run_fint(const std::string&ephem){
	std::string year=ask_line(lunar::i18n::pick(
		"请输入农历年份（例如 2025）：",
		"Enter lunar year (e.g. 2025): ",
		"旧暦年を入力（例: 2025）: ",
		"음력 연도 입력 (예: 2025): "));
	if(year.empty()){
		throw std::invalid_argument(lunar::i18n::pick(
			"年份不能为空","year cannot be empty",
			"年は必須です","연도는 비워둘 수 없습니다"));
	}
	std::vector<std::string> args={ephem,year};
	cmd_fest(args);
	ask_line(done_back_msg());
}

void run_iint(const std::string&ephem){
	std::vector<std::string> args={ephem};
	cmd_info(args);
	ask_line(done_back_msg());
}

void run_tint(const std::string&ephem){
	std::vector<std::string> args={ephem};
	int rc=cmd_test(args);
	std::cout<<itx("msg.selftest_code")<<rc<<std::endl;
	ask_line(done_back_msg());
}

void run_mvint(const std::string&ephem){
	std::string ym=ask_line(itx("prompt.monthview_ym"));
	if(ym.empty()){
		throw std::invalid_argument(itx("err.empty_required","YYYY-MM"));
	}
	std::vector<std::string> args={ephem,ym};
	std::string fmt=ask_line(itx("prompt.format_txt_json_csv"));
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}else if(fmt=="3"||fmt=="csv"){
		args.push_back("--format");
		args.push_back("csv");
	}
	std::string out=ask_line(itx("prompt.out_file"));
	if(!out.empty()){
		args.push_back("--out");
		args.push_back(out);
	}
	cmd_mview(args);
	ask_line(done_back_msg());
}

void run_rint(const std::string&ephem){
	std::string from=ask_line(itx("prompt.range_from"));
	if(from.empty()){
		throw std::invalid_argument(itx("err.empty_required","--from"));
	}
	std::string to=ask_line(itx("prompt.range_to"));
	if(to.empty()){
		throw std::invalid_argument(itx("err.empty_required","--to"));
	}
	std::vector<std::string> args={ephem,"--from",from,"--to",to};
	std::string kinds=ask_line(itx("prompt.kinds_optional"));
	if(!kinds.empty()){
		args.push_back("--kinds");
		args.push_back(kinds);
	}
	std::string fmt=ask_line(itx("prompt.format_event"));
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}else if(fmt=="3"||fmt=="csv"){
		args.push_back("--format");
		args.push_back("csv");
	}else if(fmt=="4"||fmt=="ics"){
		args.push_back("--format");
		args.push_back("ics");
	}else if(fmt=="5"||fmt=="jsonl"){
		args.push_back("--format");
		args.push_back("jsonl");
	}
	std::string out=ask_line(itx("prompt.out_file"));
	if(!out.empty()){
		args.push_back("--out");
		args.push_back(out);
	}
	cmd_range(args);
	ask_line(done_back_msg());
}

void run_sint(const std::string&ephem){
	std::string query=ask_line(itx("prompt.search_query"));
	if(query.empty()){
		throw std::invalid_argument(itx("err.empty_required","query"));
	}
	std::vector<std::string> args={ephem,query};
	std::string from=ask_line(itx("prompt.search_from"));
	if(!from.empty()){
		args.push_back("--from");
		args.push_back(from);
	}
	std::string count=ask_line(itx("prompt.search_count"));
	if(!count.empty()){
		args.push_back("--count");
		args.push_back(count);
	}
	std::string fmt=ask_line(itx("prompt.format_event"));
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}else if(fmt=="3"||fmt=="csv"){
		args.push_back("--format");
		args.push_back("csv");
	}else if(fmt=="4"||fmt=="ics"){
		args.push_back("--format");
		args.push_back("ics");
	}else if(fmt=="5"||fmt=="jsonl"){
		args.push_back("--format");
		args.push_back("jsonl");
	}
	std::string out=ask_line(itx("prompt.out_file"));
	if(!out.empty()){
		args.push_back("--out");
		args.push_back(out);
	}
	cmd_search(args);
	ask_line(done_back_msg());
}

void run_eint(const std::string&ephem){
	std::string near_date=ask_line(itx("prompt.eclipse_near"));
	if(near_date.empty()){
		throw std::invalid_argument(itx("err.empty_required","--near"));
	}
	std::vector<std::string> args={ephem,"--near",near_date};
	std::string kind=ask_line(itx("prompt.eclipse_kind"));
	if(kind=="2"||kind=="solar"){
		args.push_back("--kind");
		args.push_back("solar");
	}
	std::string stage=ask_line(itx("prompt.eclipse_stage"));
	if(!stage.empty()){
		args.push_back("--stage");
		args.push_back(stage);
	}
	std::string global=ask_line(itx("prompt.eclipse_global"));
	if(global=="1"||global=="yes"||global=="y"){
		args.push_back("--global-vis");
		args.push_back("1");
	}
	std::string fmt=ask_line(itx("prompt.format_eclipse"));
	if(fmt=="2"||fmt=="txt"){
		args.push_back("--format");
		args.push_back("txt");
	}else if(fmt=="3"||fmt=="geojson"){
		args.push_back("--format");
		args.push_back("geojson");
	}
	std::string out=ask_line(itx("prompt.out_file"));
	if(!out.empty()){
		args.push_back("--out");
		args.push_back(out);
	}
	cmd_eclipse(args);
	ask_line(done_back_msg());
}

void run_aint(const std::string&ephem){
	std::string date=ask_line(itx("prompt.almanac_date"));
	if(date.empty()){
		throw std::invalid_argument(itx("err.empty_required","date"));
	}
	std::vector<std::string> args={ephem,date};
	std::string fmt=ask_line(itx("prompt.format_txt_json_csv"));
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}else if(fmt=="3"||fmt=="csv"){
		args.push_back("--format");
		args.push_back("csv");
	}
	std::string out=ask_line(itx("prompt.out_file"));
	if(!out.empty()){
		args.push_back("--out");
		args.push_back(out);
	}
	cmd_alm(args);
	ask_line(done_back_msg());
}

void run_cfgint(){
	std::string act=ask_line(itx("prompt.config_action"));
	if(act=="2"||act=="set"){
		std::string key=ask_line(itx("prompt.config_key"));
		if(key.empty()){
			throw std::invalid_argument(itx("err.empty_required","config key"));
		}
		std::string value=ask_line(itx("prompt.config_value"));
		if(value.empty()){
			throw std::invalid_argument(itx("err.empty_required","config value"));
		}
		std::vector<std::string> args={"set",key,value};
		cmd_cfg(args);
		ask_line(done_back_msg());
		return;
	}
	std::vector<std::string> args={"show"};
	std::string fmt=ask_line(itx("prompt.format_txt_json"));
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}
	std::string out=ask_line(itx("prompt.out_file"));
	if(!out.empty()){
		args.push_back("--out");
		args.push_back(out);
	}
	cmd_cfg(args);
	ask_line(done_back_msg());
}

void run_pint(){
	std::string shell=ask_line(itx("prompt.comp_shell"));
	std::vector<std::string> args={"powershell"};
	if(shell=="1"||shell=="bash"){
		args[0]="bash";
	}else if(shell=="2"||shell=="zsh"){
		args[0]="zsh";
	}else if(shell=="3"||shell=="fish"){
		args[0]="fish";
	}
	cmd_comp(args);
	ask_line(done_back_msg());
}

void run_with_err(const std::string&cmd_name,const std::function<void()>&fn){
	try{
		fn();
	}catch(const std::exception&ex){
		std::cout<<itx("err.cmd_failed",cmd_name)<<ex.what()<<std::endl;
		ask_line(back_msg());
	}
}

std::string init_bspq(InterCfg&cfg){
	std::string ephem=init_bsp(cfg);
	while(ephem.empty()){
		std::cout<<itx("retry_select_ephem")<<std::endl;
		if(!ask_yes_no(itx("retry_prompt"),true)){
			break;
		}
		ephem=init_bsp(cfg);
	}
	return ephem;
}

void int_mode(){
	InterCfg cfg;
	std::string ephem=init_bspq(cfg);
	if(ephem.empty()){
		std::cout<<itx("no_ephem_exit")<<std::endl;
		return;
	}

	while(true){
		std::cout<<"\n"<<itx("current_ephem",ephem)<<"\n\n";
		std::cout<<itx("choose_action")<<"\n";
		std::cout<<"[1] "<<itx("menu.months")<<" (months)\n";
		std::cout<<"[2] "<<itx("menu.calendar")<<" (calendar)\n";
		std::cout<<"[3] "<<itx("menu.at")<<" (at)\n";
		std::cout<<"[4] "<<itx("menu.convert")<<" (convert)\n";
		std::cout<<"[5] "<<itx("menu.day")<<" (day)\n";
		std::cout<<"[6] "<<itx("menu.next")<<" (next)\n";
		std::cout<<"[7] "<<itx("menu.festival")<<" (festival)\n";
		std::cout<<"[8] "<<itx("menu.info")<<" (info)\n";
		std::cout<<"[9] "<<itx("menu.selftest")<<" (selftest)\n";
		std::cout<<"[10] "<<itx("menu.monthview")<<" (monthview)\n";
		std::cout<<"[11] "<<itx("menu.range")<<" (range)\n";
		std::cout<<"[12] "<<itx("menu.search")<<" (search)\n";
		std::cout<<"[13] "<<itx("menu.eclipse")<<" (eclipse)\n";
		std::cout<<"[14] "<<itx("menu.almanac")<<" (almanac)\n";
		std::cout<<"[15] "<<itx("menu.config")<<" (config)\n";
		std::cout<<"[16] "<<itx("menu.completion")<<" (completion)\n";
		std::cout<<"[d] "<<itx("menu.switch_bsp")<<"\n";
		std::cout<<"[h] "<<itx("menu.help")<<"\n";
		std::cout<<"[q] "<<itx("menu.exit")<<"\n";
		std::string choice=ask_line(itx("input_select"));

		if(choice=="1"){
			run_with_err("months",[&](){ int_month(ephem); });
		}else if(choice=="2"){
			run_with_err("calendar",[&](){ int_cal(ephem); });
		}else if(choice=="3"){
			run_with_err("at",[&](){ int_at(ephem); });
		}else if(choice=="4"){
			run_with_err("convert",[&](){ int_conv(ephem); });
		}else if(choice=="5"){
			run_with_err("day",[&](){ run_dint(ephem); });
		}else if(choice=="6"){
			run_with_err("next",[&](){ run_nint(ephem); });
		}else if(choice=="7"){
			run_with_err("festival",[&](){ run_fint(ephem); });
		}else if(choice=="8"){
			run_with_err("info",[&](){ run_iint(ephem); });
		}else if(choice=="9"){
			run_with_err("selftest",[&](){ run_tint(ephem); });
		}else if(choice=="10"){
			run_with_err("monthview",[&](){ run_mvint(ephem); });
		}else if(choice=="11"){
			run_with_err("range",[&](){ run_rint(ephem); });
		}else if(choice=="12"){
			run_with_err("search",[&](){ run_sint(ephem); });
		}else if(choice=="13"){
			run_with_err("eclipse",[&](){ run_eint(ephem); });
		}else if(choice=="14"){
			run_with_err("almanac",[&](){ run_aint(ephem); });
		}else if(choice=="15"){
			run_with_err("config",[&](){ run_cfgint(); });
		}else if(choice=="16"){
			run_with_err("completion",[&](){ run_pint(); });
		}else if(choice=="d"||choice=="D"){
			std::string new_ephem=init_bspq(cfg);
			if(!new_ephem.empty()){
				ephem=new_ephem;
			}
		}else if(choice=="h"||choice=="H"){
			use_main();
			ask_line(back_msg());
		}else if(choice=="q"||choice=="Q"){
			std::cout<<itx("exit")<<std::endl;
			break;
		}else{
			std::cout<<itx("invalid_option")<<std::endl;
		}
	}
}
