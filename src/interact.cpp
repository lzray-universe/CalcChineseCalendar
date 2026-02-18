#include "lunar/interact.hpp"

#include<cctype>
#include<fstream>
#include<iostream>
#include<vector>

namespace fs=std::filesystem;

const std::string CFG_FILE="lun_cfg.txt";

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
		}else if(key=="default_tz"){
			cfg.default_tz=value;
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
	ofs<<"default_tz="<<cfg.default_tz<<"\n";
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
	std::getline(std::cin,line);
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

std::string pick_bsp(InterCfg&cfg){
	auto options=bsp_opts();
	std::cout<<"\n可下载的 BSP 星历：\n";
	for(std::size_t i=0;i<options.size();++i){
		std::cout<<"["<<(i+1)<<"] "<<options[i].id<<"  ("<<options[i].size<<", "
				 <<options[i].range<<")"<<std::endl;
	}
	std::cout<<"输入编号或星历名称进行下载（输入 q 返回）：";
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
		std::cout<<"未找到对应的星历选项。"<<std::endl;
		return "";
	}

	std::string def_dir=
		cfg.bsp_dir.empty()?fs::current_path().string():cfg.bsp_dir;
	std::string dir=ask_line("请输入保存目录（默认为 "+def_dir+"）：");
	if(dir.empty()){
		dir=def_dir;
	}

	std::error_code ec;
	fs::create_directories(dir,ec);
	fs::path target_dir=fs::path(dir);
	fs::path filename=fs::path(chosen->url).filename();
	fs::path target=target_dir/filename;

	if(!dl_file(chosen->url,target.string())){
		std::cout<<"下载失败，请检查网络或下载工具。"<<std::endl;
		return "";
	}

	std::cout<<"下载完成："<<target<<std::endl;
	cfg.bsp_dir=target_dir.string();
	cfg.def_bsp=target.string();
	save_cfg(cfg);
	return cfg.def_bsp;
}

std::string init_bsp(InterCfg&cfg){
	load_cfg(cfg);
	if(!cfg.def_bsp.empty()&&file_ok(cfg.def_bsp)){
		if(ask_yes_no("检测到上次使用的星历文件："+cfg.def_bsp+
						  "，是否继续使用？",
					  true)){
			return cfg.def_bsp;
		}
	}

	fs::path cwd=fs::current_path();
	std::vector<fs::path> src_dirs;
	src_dirs.push_back(cwd);

	std::string dir_input=
		ask_line("当前搜索目录为："+cwd.string()+
				 "。是否指定额外的 BSP 目录？(直接回车跳过，或输入路径)：");
	if(!dir_input.empty()){
		fs::path p(dir_input);
		if(fs::exists(p)){
			cfg.bsp_dir=p.string();
			src_dirs.push_back(p);
			save_cfg(cfg);
		}else{
			std::cout<<"目录不存在，将继续使用当前目录搜索。"<<std::endl;
		}
	}

	std::vector<fs::path> bsp_files=find_bsps(src_dirs);
	if(!bsp_files.empty()){
		std::cout<<"找到以下 BSP 文件："<<std::endl;
		for(std::size_t i=0;i<bsp_files.size();++i){
			std::cout<<"["<<(i+1)<<"] "<<bsp_files[i].filename().string()<<"  ("
					 <<bsp_files[i].string()<<")"<<std::endl;
		}
		std::cout<<"请选择要使用的星历文件编号（或输入 0 进入下载界面）：";
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
				if(cfg.bsp_dir.empty()){
					cfg.bsp_dir=bsp_files[idx-1].parent_path().string();
				}
				save_cfg(cfg);
				return cfg.def_bsp;
			}
		}
	}

	std::cout<<"未找到 BSP 文件，将进入下载界面。"<<std::endl;
	return pick_bsp(cfg);
}

void int_month(const std::string&ephem){
	MonthsArgs margs;
	margs.ephem=ephem;
	while(true){
		margs.years=
			ask_line("请输入年份或范围（如 2024 或 2020-2025，必填）：");
		if(!margs.years.empty()){
			break;
		}
		std::cout<<"年份不能为空，请重新输入。"<<std::endl;
	}

	std::string mode_input=
		ask_line("选择输出模式：1) 农历(lunar)  2) 公历(gregorian) "
				 "，直接回车默认 lunar：");
	if(mode_input=="2"||mode_input=="gregorian"){
		margs.mode="gregorian";
	}else{
		margs.mode="lunar";
	}

	margs.out_json=ask_line("可选：输出 JSON 文件路径（留空则不输出）：");
	margs.out_txt=ask_line("可选：输出文本文件路径（留空则不输出）：");

	cli_month(margs);
	ask_line("已完成，按回车返回主菜单。");
}

void int_cal(const std::string&ephem){
	CalArgs cargs;
	cargs.ephem=ephem;
	std::string years=ask_line("可选：输入年份或范围（留空使用默认）：");
	if(!years.empty()){
		cargs.has_years=true;
		cargs.years_arg=years;
	}
	cli_cal(cargs);
	ask_line("已完成，按回车返回主菜单。");
}

void int_at(const std::string&ephem){
	AtArgs aargs;
	aargs.ephem=ephem;
	while(true){
		aargs.time_raw=ask_line(
			"请输入查询时刻（例如 2025-06-01T00:00:00+08:00，必填）：");
		if(!aargs.time_raw.empty()){
			break;
		}
		std::cout<<"时刻不能为空，请重新输入。"<<std::endl;
	}

	std::string input_tz=ask_line("若输入不带时区，解析时区（默认 +08:00）：");
	if(!input_tz.empty()){
		aargs.input_tz=input_tz;
	}
	std::string display_tz=ask_line("输出显示时区（默认 +08:00）：");
	if(!display_tz.empty()){
		aargs.tz=display_tz;
	}
	std::string fmt=ask_line("输出格式：1) txt  2) json（默认 txt）：");
	if(fmt=="2"||fmt=="json"){
		aargs.format="json";
	}
	std::string near_ev=ask_line("是否输出附近节气/四相：1)是 0)否（默认1）：");
	if(!near_ev.empty()){
		aargs.events=(near_ev!="0");
	}
	aargs.out=ask_line("可选：输出文件路径（留空则输出到控制台）：");

	cli_at(aargs);
	ask_line("已完成，按回车返回主菜单。");
}

void int_conv(const std::string&ephem){
	ConvArgs cargs;
	cargs.ephem=ephem;

	std::string mode=
		ask_line("转换方向：1) 公历->农历  2) 农历->公历（默认1）：");
	if(mode=="2"){
		cargs.from_lunar=true;
		cargs.lunar_year=std::stoi(ask_line("请输入农历年份（如 2025）："));
		cargs.lun_mno=std::stoi(ask_line("请输入农历月（1-12）："));
		cargs.lunar_day=std::stoi(ask_line("请输入农历日（1-30）："));
		std::string leap=ask_line("是否闰月：1)是 0)否（默认0）：");
		cargs.leap=(leap=="1");
	}else{
		while(true){
			cargs.in_value=ask_line(
				"请输入公历日期/时刻（例如 2026-02-18 或 2025-06-01T00:00）：");
			if(!cargs.in_value.empty()){
				cargs.has_in=true;
				break;
			}
			std::cout<<"输入不能为空，请重新输入。"<<std::endl;
		}
		std::string input_tz=
			ask_line("若输入不带时区，解析时区（默认 +08:00）：");
		if(!input_tz.empty()){
			cargs.input_tz=input_tz;
		}
	}

	std::string display_tz=ask_line("输出显示时区（默认 +08:00）：");
	if(!display_tz.empty()){
		cargs.tz=display_tz;
	}
	std::string fmt=ask_line("输出格式：1) txt  2) json（默认 txt）：");
	if(fmt=="2"||fmt=="json"){
		cargs.format="json";
	}
	cargs.out=ask_line("可选：输出文件路径（留空则输出到控制台）：");

	cli_conv(cargs);
	ask_line("已完成，按回车返回主菜单。");
}

void run_dint(const std::string&ephem){
	std::string date=ask_line("请输入公历日期 YYYY-MM-DD：");
	if(date.empty()){
		throw std::invalid_argument("日期不能为空");
	}
	std::vector<std::string> args={ephem,date};
	std::string fmt=ask_line("输出格式：1) txt 2) json（默认 txt）：");
	if(fmt=="2"||fmt=="json"){
		args.push_back("--format");
		args.push_back("json");
	}
	cmd_day(args);
	ask_line("已完成，按回车返回主菜单。");
}

void run_nint(const std::string&ephem){
	std::string from=
		ask_line("请输入起始时刻（例如 2025-06-01T00:00:00+08:00）：");
	if(from.empty()){
		throw std::invalid_argument("起始时刻不能为空");
	}
	std::string count=ask_line("事件数量（默认 5）：");
	if(count.empty()){
		count="5";
	}
	std::vector<std::string> args={ephem,"--from",from,"--count",count};
	cmd_next(args);
	ask_line("已完成，按回车返回主菜单。");
}

void run_fint(const std::string&ephem){
	std::string year=ask_line("请输入农历年份（例如 2025）：");
	if(year.empty()){
		throw std::invalid_argument("年份不能为空");
	}
	std::vector<std::string> args={ephem,year};
	cmd_fest(args);
	ask_line("已完成，按回车返回主菜单。");
}

void run_iint(const std::string&ephem){
	std::vector<std::string> args={ephem};
	cmd_info(args);
	ask_line("已完成，按回车返回主菜单。");
}

void run_tint(const std::string&ephem){
	std::vector<std::string> args={ephem};
	int rc=cmd_test(args);
	std::cout<<"selftest exit code: "<<rc<<std::endl;
	ask_line("已完成，按回车返回主菜单。");
}

std::string init_bspq(InterCfg&cfg){
	std::string ephem=init_bsp(cfg);
	while(ephem.empty()){
		std::cout<<"未成功选择星历文件，是否重试？"<<std::endl;
		if(!ask_yes_no("继续重试？",true)){
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
		std::cout<<"未选择星历文件，程序退出。"<<std::endl;
		return;
	}

	while(true){
		std::cout<<"\n当前星历文件："<<ephem<<"\n\n";
		std::cout<<"请选择要执行的操作：\n";
		std::cout<<"[1] 计算农历 / 公历月份信息（months）\n";
		std::cout<<"[2] 计算整年节气与月相日历（calendar）\n";
		std::cout<<"[3] 任意时刻查询（月相/节气/农历）（at）\n";
		std::cout<<"[4] 公历转农历 / 农历转公历（convert）\n";
		std::cout<<"[5] 日历单日视图（day）\n";
		std::cout<<"[6] 后续事件查询（next）\n";
		std::cout<<"[7] 传统节日（festival）\n";
		std::cout<<"[8] 星历信息（info）\n";
		std::cout<<"[9] 自检（selftest）\n";
		std::cout<<"[d] 重新选择 / 下载 BSP 星历文件\n";
		std::cout<<"[h] 查看命令行使用帮助\n";
		std::cout<<"[q] 退出程序\n";
		std::string choice=ask_line("请输入选项：");
		if(choice=="1"){
			try{
				int_month(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 months 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="2"){
			try{
				int_cal(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 calendar 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="3"){
			try{
				int_at(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 at 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="4"){
			try{
				int_conv(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 convert 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="5"){
			try{
				run_dint(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 day 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="6"){
			try{
				run_nint(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 next 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="7"){
			try{
				run_fint(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 festival 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="8"){
			try{
				run_iint(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 info 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="9"){
			try{
				run_tint(ephem);
			}catch(const std::exception&ex){
				std::cout<<"运行 selftest 时出错："<<ex.what()<<std::endl;
				ask_line("按回车返回主菜单。");
			}
		}else if(choice=="d"||choice=="D"){
			std::string new_ephem=init_bspq(cfg);
			if(!new_ephem.empty()){
				ephem=new_ephem;
			}
		}else if(choice=="h"||choice=="H"){
			use_main();
			ask_line("按回车返回主菜单。");
		}else if(choice=="q"||choice=="Q"){
			std::cout<<"感谢使用，程序即将退出。"<<std::endl;
			break;
		}else{
			std::cout<<"无效的选项，请重新输入。"<<std::endl;
		}
	}
}
