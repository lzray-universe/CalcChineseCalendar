#include "lunar/format.hpp"

#include<cctype>
#include<cmath>
#include<iomanip>
#include<limits>
#include<sstream>
#include<stdexcept>

#include "lunar/math.hpp"

namespace{

bool is_digit(char c){ return std::isdigit(static_cast<unsigned char>(c))!=0; }

bool is_leap_year(int year){
	return (year%400==0)||(year%4==0&&year%100!=0);
}

int days_in_month(int year,int month){
	static const int kDays[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	if(month<1||month>12){
		throw std::invalid_argument("invalid date value");
	}
	if(month==2&&is_leap_year(year)){
		return 29;
	}
	return kDays[month-1];
}

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

int parse_signed_component(const std::string&s,std::size_t begin,
						   std::size_t end,const std::string&label){
	if(begin>=end){
		throw std::invalid_argument("invalid datetime: missing "+label);
	}
	bool neg=false;
	if(s[begin]=='+'||s[begin]=='-'){
		neg=(s[begin]=='-');
		++begin;
	}
	if(begin>=end){
		throw std::invalid_argument("invalid datetime: missing "+label);
	}
	long long value=0;
	const long long max_abs=
		static_cast<long long>(std::numeric_limits<int>::max())+
		(neg?1LL:0LL);
	for(std::size_t i=begin;i<end;++i){
		char c=s[i];
		if(!is_digit(c)){
			throw std::invalid_argument("invalid datetime: bad "+label);
		}
		value=value*10+static_cast<long long>(c-'0');
		if(value>max_abs){
			throw std::invalid_argument("invalid datetime: "+label+
										" out of range");
		}
	}
	long long signed_value=neg?-value:value;
	if(signed_value<std::numeric_limits<int>::min()||
	   signed_value>std::numeric_limits<int>::max()){
		throw std::invalid_argument("invalid datetime: "+label+" out of range");
	}
	return static_cast<int>(signed_value);
}

std::size_t parse_date_prefix(const std::string&text,int&year,int&month,int&day){
	if(text.empty()){
		throw std::invalid_argument("datetime text is empty");
	}
	std::size_t year_sep=
		text.find('-',((text[0]=='+'||text[0]=='-')?1u:0u));
	if(year_sep==std::string::npos){
		throw std::invalid_argument("invalid datetime, expected YEAR-MM-DD");
	}
	year=parse_signed_component(text,0,year_sep,"year");

	std::size_t month_start=year_sep+1;
	if(month_start+2>text.size()){
		throw std::invalid_argument("invalid datetime: missing month");
	}
	month=parse_fix(text,month_start,2,"month");
	std::size_t month_sep=month_start+2;
	if(month_sep>=text.size()||text[month_sep]!='-'){
		throw std::invalid_argument("invalid datetime, expected YEAR-MM-DD");
	}
	std::size_t day_start=month_sep+1;
	day=parse_fix(text,day_start,2,"day");
	return day_start+2;
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
	IsoTime out;
	int year=0;
	int month=0;
	int day=0;
	std::size_t pos=parse_date_prefix(text,year,month,day);
	if(month<1||month>12||day<1||day>days_in_month(year,month)){
		throw std::invalid_argument("invalid date value");
	}

	int hour=0;
	int minute=0;
	double second=0.0;

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
