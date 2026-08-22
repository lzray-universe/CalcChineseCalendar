#include "internal.hpp"

namespace{

struct MagnitudeBatchOptions{
	std::string ephem;
	std::string input;
	std::string output;
};

std::string require_magnitude_value(const std::vector<std::string>&args,
									std::size_t&i,const std::string&option){
	if(i+1>=args.size()){
		throw std::invalid_argument("missing value for option: "+option);
	}
	return args[++i];
}

MagnitudeBatchOptions parse_magnitude_batch(const std::vector<std::string>&args){
	if(args.empty()||args[0].rfind("--",0)==0){
		throw std::invalid_argument(
			"eclipse-magnitude requires an ephemeris path or @series");
	}
	MagnitudeBatchOptions options;
	options.ephem=args[0];
	for(std::size_t i=1;i<args.size();++i){
		if(args[i]=="--input"){
			options.input=require_magnitude_value(args,i,args[i]);
		}else if(args[i]=="--out"){
			options.output=require_magnitude_value(args,i,args[i]);
		}else{
			throw std::invalid_argument("unknown eclipse-magnitude option: "+args[i]);
		}
	}
	if(options.input.empty()){
		throw std::invalid_argument("eclipse-magnitude requires --input <tsv>");
	}
	return options;
}

std::vector<std::string> split_magnitude_fields(const std::string&line){
	std::istringstream input(line);
	std::vector<std::string> fields;
	std::string field;
	while(input>>field){
		fields.push_back(std::move(field));
	}
	return fields;
}

}

int cmd_eclipse_magnitude(const std::vector<std::string>&args){
	if(args.size()==1&&(args[0]=="-h"||args[0]=="--help")){
		use_eclipse_magnitude();
		return 0;
	}
	MagnitudeBatchOptions options=parse_magnitude_batch(args);
	std::ifstream input(options.input);
	if(!input){
		throw std::runtime_error("failed to open input: "+options.input);
	}
	std::ofstream output_file;
	std::ostream*output=&std::cout;
	if(!options.output.empty()){
		output_file.open(options.output,std::ios::binary|std::ios::trunc);
		if(!output_file){
			throw std::runtime_error("failed to open output: "+options.output);
		}
		output=&output_file;
	}

	EphRead eph(options.ephem);
	*output<<"id\tjd_tdb_max\ttype\tmag\tobscuration\n";
	std::string line;
	std::size_t line_number=0;
	while(std::getline(input,line)){
		++line_number;
		if(line.empty()||line[0]=='#'){
			continue;
		}
		auto fields=split_magnitude_fields(line);
		if(fields.size()>=2&&fields[0]=="id"&&fields[1]=="jd_tdb_max"){
			continue;
		}
		if(fields.size()!=3){
			throw std::invalid_argument(
				"invalid eclipse-magnitude input at line "+
				std::to_string(line_number)+": expected 3 columns");
		}
		double jd_tdb=0.0;
		try{
			std::size_t used=0;
			jd_tdb=std::stod(fields[1],&used);
			if(used!=fields[1].size()){
				throw std::invalid_argument("trailing characters");
			}
		}catch(const std::exception&){
			throw std::invalid_argument(
				"invalid jd_tdb_max at line "+std::to_string(line_number));
		}
		double mag=0.0;
		double obscuration=0.0;
		std::string corrected_type;
		if(!calc_solar_eclipse_magnitude_from_max(
			   eph,jd_tdb,fields[2],&mag,&obscuration,&corrected_type)){
			throw std::runtime_error(
				"failed eclipse magnitude at line "+std::to_string(line_number));
		}
		*output<<fields[0]<<'\t'<<format_num(jd_tdb)<<'\t'<<corrected_type<<'\t'
			   <<format_num(mag)<<'\t'<<format_num(obscuration)<<'\n';
	}
	return 0;
}
