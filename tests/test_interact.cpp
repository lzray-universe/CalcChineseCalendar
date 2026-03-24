#include "test_common.hpp"

#include<filesystem>
#include<fstream>
#include<iostream>
#include<sstream>
#include<stdexcept>
#include<string>

#include<gtest/gtest.h>

#include "lunar/i18n.hpp"
#include "lunar/interact.hpp"

namespace{

class ScopedCin{
public:
	explicit ScopedCin(const std::string&text):
		input_(text),old_(std::cin.rdbuf(input_.rdbuf())){}

	~ScopedCin(){ std::cin.rdbuf(old_); }

private:
	std::istringstream input_;
	std::streambuf*old_;
};

class ScopedCout{
public:
	ScopedCout():old_(std::cout.rdbuf(output_.rdbuf())){}

	~ScopedCout(){ std::cout.rdbuf(old_); }

	std::string str()const{ return output_.str(); }

private:
	std::ostringstream output_;
	std::streambuf*old_;
};

class ScopedCwd{
public:
	explicit ScopedCwd(const std::filesystem::path&path):
		old_(std::filesystem::current_path()){
		std::filesystem::current_path(path);
	}

	~ScopedCwd(){ std::filesystem::current_path(old_); }

private:
	std::filesystem::path old_;
};

class ScopedLang{
public:
	explicit ScopedLang(lunar::i18n::Lang lang):
		old_(lunar::i18n::current_lang()){
		lunar::i18n::set_lang(lang);
	}

	~ScopedLang(){ lunar::i18n::set_lang(old_); }

private:
	lunar::i18n::Lang old_;
};

std::filesystem::path make_case_dir(const std::string&name){
	std::filesystem::path dir=std::filesystem::current_path()/name;
	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
	ec.clear();
	std::filesystem::create_directories(dir,ec);
	if(ec){
		throw std::runtime_error("failed to create test dir: "+dir.string());
	}
	return dir;
}

void write_text(const std::filesystem::path&path,const std::string&text){
	std::ofstream ofs(path,std::ios::binary);
	if(!ofs){
		throw std::runtime_error("failed to open file: "+path.string());
	}
	ofs<<text;
}

}

TEST(InteractInitBsp, NoBspPathOffersDownloaderBeforeSeriesFallback){
	ScopedLang lang(lunar::i18n::Lang::En);
	const std::filesystem::path dir=make_case_dir("interact_no_bsp_case");
	{
		ScopedCwd cwd(dir);
#if LUNAR_ENABLE_SERIES_FALLBACK
		ScopedCin input("\nq\nn\n");
#else
		ScopedCin input("\nq\n");
#endif
		ScopedCout output;
		InterCfg cfg;
		const std::string ephem=init_bsp(cfg);
		EXPECT_TRUE(ephem.empty());
		EXPECT_EQ(cfg.def_bsp,"");
		EXPECT_NE(output.str().find("Downloadable BSP ephemerides:"),
				  std::string::npos);
		EXPECT_EQ(output.str().find("Switched to the built-in"),
				  std::string::npos);
	}
	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}

TEST(InteractMode, LanguageSwitchAppliesImmediatelyAndPersists){
	ScopedLang lang(lunar::i18n::Lang::Zh);
	const std::filesystem::path dir=make_case_dir("interact_lang_case");
	write_text(dir/"dummy.bsp","");
	{
		ScopedCwd cwd(dir);
		ScopedCin input("\n1\nl\n3\n\nq\n");
		ScopedCout output;
		int_mode();
		EXPECT_EQ(lunar::i18n::current_lang_code(),"en");
		EXPECT_NE(output.str().find("Thanks for using lunar. Exiting."),
				  std::string::npos);
	}
	const std::string cfg_text=read_file_text(dir/CFG_FILE);
	EXPECT_EQ(trim(txt_value(cfg_text,"default_lang")),"en");
	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}

TEST(InteractConfig, SaveCfgWritesLfOnly){
	const std::filesystem::path dir=make_case_dir("interact_cfg_save_case");
	{
		ScopedCwd cwd(dir);
		InterCfg cfg;
		cfg.def_bsp="dummy.bsp";
		cfg.bsp_list={"dummy.bsp","series"};
		cfg.default_lang="en";
		cfg.default_lunar_day_tz="+09:00";
		ASSERT_TRUE(save_cfg(cfg));

		const std::string cfg_text=read_file_text(dir/CFG_FILE);
		EXPECT_EQ(cfg_text.find('\r'),std::string::npos);
		EXPECT_EQ(trim(txt_value(cfg_text,"default_lang")),"en");
		EXPECT_EQ(trim(txt_value(cfg_text,"default_lunar_day_tz")),"+09:00");
	}
	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}

TEST(InteractConfig, LoadCfgAcceptsUtf8Bom){
	const std::filesystem::path dir=make_case_dir("interact_cfg_bom_case");
	write_text(dir/CFG_FILE,
			   "\xEF\xBB\xBF""default_lang=en\n"
			   "default_tz=+09:00\n");
	{
		ScopedCwd cwd(dir);
		InterCfg cfg;
		ASSERT_TRUE(load_cfg(cfg));
		EXPECT_EQ(cfg.default_lang,"en");
		EXPECT_EQ(cfg.default_tz,"+09:00");
	}
	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}

TEST(InteractConfig, LunarDayTzFallsBackToLanguageDefault){
	InterCfg zh_cfg;
	zh_cfg.default_lang="en";
	EXPECT_EQ(resolve_lunar_day_tz(zh_cfg),"+08:00");

	InterCfg ja_cfg;
	ja_cfg.default_lang="ja";
	EXPECT_EQ(resolve_lunar_day_tz(ja_cfg),"+09:00");

	InterCfg ko_cfg;
	ko_cfg.default_lang="ko";
	ko_cfg.default_lunar_day_tz="Z";
	EXPECT_EQ(resolve_lunar_day_tz(ko_cfg),"Z");
}

TEST(InteractMode, SkyMenuRunsAndPrintsSelectedTarget){
	if(!has_test_ephem()){
		GTEST_SKIP()<<"requires series fallback or LUNAR_TEST_BSP";
	}
	const std::filesystem::path dir=make_case_dir("interact_sky_case");
	write_text(dir/CFG_FILE,
			   "def_bsp="+test_ephem()+"\n"
			   "bsp_list="+test_ephem()+"\n"
			   "default_tz=+08:00\n");
	{
		ScopedCwd cwd(dir);
		ScopedCin input("\n16\n"
						"2025-06-01T20:00:00+08:00\n"
						"\n"
						"\n"
						"31.23\n"
						"121.47\n"
						"\n"
						"2\n"
						"sun\n"
						"1\n"
						"\n"
						"\n"
						"q\n");
		ScopedCout output;
		int_mode();
		EXPECT_NE(output.str().find("input.mode=pick"),std::string::npos);
		EXPECT_NE(output.str().find("\tsun\t"),std::string::npos);
	}
	std::error_code ec;
	std::filesystem::remove_all(dir,ec);
}
