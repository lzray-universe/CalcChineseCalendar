#include "lunar/ics.hpp"

#include<iomanip>
#include<sstream>

#include "lunar/math.hpp"

namespace{

std::string esc_ics(const std::string&s){
	std::string out;
	out.reserve(s.size()+8);
	for(char c : s){
		switch(c){
		case '\\':
			out+="\\\\";
			break;
		case ';':
			out+="\\;";
			break;
		case ',':
			out+="\\,";
			break;
		case '\n':
			out+="\\n";
			break;
		case '\r':
			break;
		default:
			out.push_back(c);
			break;
		}
	}
	return out;
}

}

std::string ics_time(double jd_utc){
	int y=0;
	int m=0;
	int d=0;
	int hh=0;
	int mm=0;
	double ss=0.0;
	jd2greg(jd_utc,y,m,d,hh,mm,ss);
	int sec=static_cast<int>(ss+0.5);
	if(sec>59){
		sec=59;
	}

	std::ostringstream oss;
	oss<<std::setfill('0')<<std::setw(4)<<y<<std::setw(2)<<m<<std::setw(2)<<d
	   <<"T"<<std::setw(2)<<hh<<std::setw(2)<<mm<<std::setw(2)<<sec<<"Z";
	return oss.str();
}

void write_ics(std::ostream&os,const std::string&prodid,
			   const std::string&cal_name,const std::vector<IcsEvent>&events,
			   const std::vector<std::string>&x_notes){
	os<<"BEGIN:VCALENDAR\r\n";
	os<<"VERSION:2.0\r\n";
	os<<"PRODID:"<<esc_ics(prodid)<<"\r\n";
	os<<"CALSCALE:GREGORIAN\r\n";
	os<<"METHOD:PUBLISH\r\n";
	os<<"X-WR-CALNAME:"<<esc_ics(cal_name)<<"\r\n";
	for(const auto&n : x_notes){
		os<<"X-LUNAR-NOTE:"<<esc_ics(n)<<"\r\n";
	}

	const std::string dtstamp=ics_time(greg2jd(2026,1,1,0,0,0.0));
	for(const auto&ev : events){
		os<<"BEGIN:VEVENT\r\n";
		os<<"UID:"<<esc_ics(ev.uid)<<"\r\n";
		os<<"DTSTAMP:"<<dtstamp<<"\r\n";
		os<<"DTSTART:"<<ics_time(ev.jd_utc)<<"\r\n";
		os<<"SUMMARY:"<<esc_ics(ev.summary)<<"\r\n";
		if(!ev.desc.empty()){
			os<<"DESCRIPTION:"<<esc_ics(ev.desc)<<"\r\n";
		}
		os<<"END:VEVENT\r\n";
	}
	os<<"END:VCALENDAR\r\n";
}
