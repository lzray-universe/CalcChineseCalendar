namespace{

bool parse_spk(const std::string&ephem,double&jd_start,double&jd_end){
	EphRead reader(ephem);
	reader.load_kern();

	std::vector<int> ids=reader.spk_objects();
	if(ids.empty()){
		return false;
	}

	double min_et=std::numeric_limits<double>::infinity();
	double max_et=-std::numeric_limits<double>::infinity();

	for(int obj : ids){
		std::vector<std::pair<double,double>> cov=reader.spk_coverage(obj);
		for(const auto&it : cov){
			if(it.first<min_et){
				min_et=it.first;
			}
			if(it.second>max_et){
				max_et=it.second;
			}
		}
	}
	if(!std::isfinite(min_et)||!std::isfinite(max_et)||min_et>=max_et){
		return false;
	}
	jd_start=2451545.0+min_et/SEC_DAY;
	jd_end=2451545.0+max_et/SEC_DAY;
	return true;
}

}

