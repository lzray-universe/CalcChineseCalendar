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
