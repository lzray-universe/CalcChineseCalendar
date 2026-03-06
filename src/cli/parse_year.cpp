std::vector<int> parse_year(const std::string&arg){
	std::vector<int> years;
	std::string tmp;
	std::vector<std::string> parts;
	std::istringstream iss(arg);
	while(std::getline(iss,tmp,',')){
		std::string p;
		for(char c : tmp){
			if(!std::isspace(static_cast<unsigned char>(c))){
				p.push_back(c);
			}
		}
		if(!p.empty()){
			parts.push_back(p);
		}
	}
	if(parts.empty()){
		throw std::invalid_argument("years argument is empty");
	}

	for(const auto&part : parts){
		auto pos=part.find('-',1);
		if(pos!=std::string::npos){
			int start=parse_int(part.substr(0,pos),"year");
			int end=parse_int(part.substr(pos+1),"year");
			if(end<start){
				throw std::invalid_argument("invalid year range: "+part);
			}
			for(int y=start;y<=end;++y){
				years.push_back(y);
			}
		}else{
			years.push_back(parse_int(part,"year"));
		}
	}

	std::vector<int> ordered;
	std::set<int> seen;
	for(int y : years){
		if(seen.insert(y).second){
			ordered.push_back(y);
		}
	}
	return ordered;
}

