#include "lunar/format.hpp"

#include<cctype>
#include<cmath>
#include<iomanip>
#include<sstream>
#include<stdexcept>

#include "lunar/math.hpp"

namespace{

bool is_digit(char c){ return std::isdigit(static_cast<unsigned char>(c))!=0; }

int parse_fix(const std::string&s,std::size_t pos,std::size_t count,
			  const std::string&label){
	if(pos+count>s.size()){
		throw std::invalid_argument("invalid datetime: missing "+label);
	}
	int value=0;
	for(std::size_t i=0;i<count;++i){
		char c=s[pos+i];
		if(!is_digit(c)){
			throw std::invalid_argument("invalid datetime: bad "+label);
		}
		value=value*10+static_cast<int>(c-'0');
	}
	return value;
}

int parse_tzs(const std::string&tz){
	if(tz=="Z"||tz=="z"){
		return 0;
	}
	if(tz.size()!=6||(tz[0]!='+'&&tz[0]!='-')||tz[3]!=':'){
		throw std::invalid_argument(
			"invalid timezone suffix, expected Z or +HH:MM/-HH:MM");
	}
	int hh=parse_fix(tz,1,2,"timezone hour");
	int mm=parse_fix(tz,4,2,"timezone minute");
	if(hh>23||mm>59){
		throw std::invalid_argument("timezone suffix out of range");
	}
	int total=hh*60+mm;
	if(tz[0]=='-'){
		total=-total;
	}
	return total;
}

}

int parse_tz(const std::string&tz){ return parse_tzs(tz); }

std::string fmt_tz(int off_min){
	if(off_min==0){
		return "Z";
	}
	int mins=off_min;
	char sign='+';
	if(mins<0){
		sign='-';
		mins=-mins;
	}
	int hh=mins/60;
	int mm=mins%60;
	std::ostringstream oss;
	oss<<sign<<std::setfill('0')<<std::setw(2)<<hh<<":"<<std::setw(2)<<mm;
	return oss.str();
}

IsoTime parse_iso(const std::string&text,const std::string&default_tz){
	if(text.empty()){
		throw std::invalid_argument("datetime text is empty");
	}

	IsoTime out;
	int year=parse_fix(text,0,4,"year");
	if(text.size()<10||text[4]!='-'||text[7]!='-'){
		throw std::invalid_argument("invalid datetime, expected YYYY-MM-DD");
	}
	int month=parse_fix(text,5,2,"month");
	int day=parse_fix(text,8,2,"day");
	if(month<1||month>12||day<1||day>31){
		throw std::invalid_argument("invalid date value");
	}

	int hour=0;
	int minute=0;
	double second=0.0;

	std::size_t pos=10;
	if(pos<text.size()){
		if(text[pos]=='T'||text[pos]=='t'){
			++pos;
			hour=parse_fix(text,pos,2,"hour");
			pos+=2;
			if(pos>=text.size()||text[pos]!=':'){
				throw std::invalid_argument(
					"invalid datetime, expected ':' after hour");
			}
			++pos;
			minute=parse_fix(text,pos,2,"minute");
			pos+=2;

			int sec_int=0;
			int frac_d=0;
			int frac_value=0;
			if(pos<text.size()&&text[pos]==':'){
				++pos;
				sec_int=parse_fix(text,pos,2,"second");
				pos+=2;
				if(pos<text.size()&&text[pos]=='.'){
					++pos;
					std::size_t frac_start=pos;
					while(pos<text.size()&&is_digit(text[pos])){
						if(frac_d<9){
							frac_value=
								frac_value*10+static_cast<int>(text[pos]-'0');
							++frac_d;
						}
						++pos;
					}
					if(pos==frac_start){
						throw std::invalid_argument(
							"invalid datetime, expected digits after decimal "
							"point");
					}
				}
			}

			if(hour<0||hour>23||minute<0||minute>59||sec_int<0||sec_int>59){
				throw std::invalid_argument("invalid time value");
			}
			second=static_cast<double>(sec_int);
			if(frac_d>0){
				second+=static_cast<double>(frac_value)/std::pow(10.0,frac_d);
			}

			if(pos<text.size()){
				std::string tz_suffix=text.substr(pos);
				out.tz_off=parse_tzs(tz_suffix);
				out.has_tz=true;
				pos=text.size();
			}else{
				out.tz_off=parse_tz(default_tz);
				out.has_tz=false;
			}
		}else{
			std::string tz_suffix=text.substr(pos);
			out.tz_off=parse_tzs(tz_suffix);
			out.has_tz=true;
		}
	}else{
		out.tz_off=parse_tz(default_tz);
		out.has_tz=false;
	}

	double jd_local=greg2jd(year,month,day,hour,minute,second);
	out.jd_utc=jd_local-static_cast<double>(out.tz_off)/1440.0;
	return out;
}

std::string fmt_iso(double jd_utc,int off_min,bool with_ms){
	const double off_days=static_cast<double>(off_min)/1440.0;
	double jd_disp=jd_utc+off_days;
	const double round_days=with_ms?(0.5/(1000.0*SEC_DAY)):(0.5/SEC_DAY);
	jd_disp+=round_days;

	int year=0;
	int month=0;
	int day=0;
	int hour=0;
	int minute=0;
	double second=0.0;
	jd2greg(jd_disp,year,month,day,hour,minute,second);

	int second_i=static_cast<int>(std::floor(second+1e-12));
	if(second_i<0){
		second_i=0;
	}
	if(second_i>59){
		second_i=59;
	}

	std::ostringstream oss;
	oss<<std::setfill('0')<<std::setw(4)<<year<<"-"<<std::setw(2)<<month<<"-"
	   <<std::setw(2)<<day<<"T"<<std::setw(2)<<hour<<":"<<std::setw(2)<<minute
	   <<":"<<std::setw(2)<<second_i;

	if(with_ms){
		int ms=static_cast<int>(std::floor((second-second_i)*1000.0+1e-9));
		if(ms<0){
			ms=0;
		}
		if(ms>999){
			ms=999;
		}
		oss<<"."<<std::setw(3)<<ms;
	}

	oss<<fmt_tz(off_min);
	return oss.str();
}
