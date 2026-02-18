#include "lunar/js_writer.hpp"

#include<cmath>
#include<iomanip>
#include<sstream>
#include<stdexcept>

std::string js_escape(const std::string&s){
	std::ostringstream oss;
	for(unsigned char c : s){
		switch(c){
		case '\"':
			oss<<"\\\"";
			break;
		case '\\':
			oss<<"\\\\";
			break;
		case '\b':
			oss<<"\\b";
			break;
		case '\f':
			oss<<"\\f";
			break;
		case '\n':
			oss<<"\\n";
			break;
		case '\r':
			oss<<"\\r";
			break;
		case '\t':
			oss<<"\\t";
			break;
		default:
			if(c<0x20){
				oss<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')
				   <<static_cast<int>(c)<<std::dec<<std::setfill(' ');
			}else{
				oss<<static_cast<char>(c);
			}
			break;
		}
	}
	return oss.str();
}

JsonWriter::JsonWriter(std::ostream&os,bool pretty,int ind_size)
	: os_(os),pretty_(pretty),ind_size_(ind_size){}

void JsonWriter::put_indent(std::size_t depth){
	if(!pretty_){
		return;
	}
	for(std::size_t i=0;i<depth*static_cast<std::size_t>(ind_size_);++i){
		os_<<' ';
	}
}

void JsonWriter::val_begin(){
	if(stack_.empty()){
		if(root_ok_){
			throw std::logic_error("multiple JSON roots");
		}
		root_ok_=true;
		return;
	}

	Context&ctx=stack_.back();
	if(ctx.is_object){
		if(!ctx.want_val){
			throw std::logic_error("value in object requires key()");
		}
		ctx.want_val=false;
		return;
	}

	if(!ctx.first){
		os_<<',';
	}
	if(pretty_){
		os_<<'\n';
		put_indent(stack_.size());
	}
	ctx.first=false;
}

void JsonWriter::obj_begin(){
	val_begin();
	os_<<'{';
	stack_.push_back({true,true,false});
}

void JsonWriter::obj_end(){
	if(stack_.empty()||!stack_.back().is_object){
		throw std::logic_error("obj_end without matching obj_begin");
	}
	Context ctx=stack_.back();
	if(ctx.want_val){
		throw std::logic_error("object key missing value");
	}
	stack_.pop_back();
	if(pretty_&&!ctx.first){
		os_<<'\n';
		put_indent(stack_.size());
	}
	os_<<'}';
}

void JsonWriter::arr_begin(){
	val_begin();
	os_<<'[';
	stack_.push_back({false,true,false});
}

void JsonWriter::arr_end(){
	if(stack_.empty()||stack_.back().is_object){
		throw std::logic_error("arr_end without matching arr_begin");
	}
	Context ctx=stack_.back();
	stack_.pop_back();
	if(pretty_&&!ctx.first){
		os_<<'\n';
		put_indent(stack_.size());
	}
	os_<<']';
}

void JsonWriter::key(const std::string&name){
	if(stack_.empty()||!stack_.back().is_object){
		throw std::logic_error("key() outside object");
	}
	Context&ctx=stack_.back();
	if(ctx.want_val){
		throw std::logic_error("previous key missing value");
	}
	if(!ctx.first){
		os_<<',';
	}
	if(pretty_){
		os_<<'\n';
		put_indent(stack_.size());
	}
	os_<<'"'<<js_escape(name)<<'"'<<':';
	if(pretty_){
		os_<<' ';
	}
	ctx.first=false;
	ctx.want_val=true;
}

void JsonWriter::value(const std::string&v){
	val_begin();
	os_<<'"'<<js_escape(v)<<'"';
}

void JsonWriter::value(const char*v){ value(std::string(v==nullptr?"":v)); }

void JsonWriter::value(double v){
	if(!std::isfinite(v)){
		null_val();
		return;
	}
	val_begin();
	std::ostringstream oss;
	oss<<std::setprecision(17)<<v;
	os_<<oss.str();
}

void JsonWriter::value(int v){
	val_begin();
	os_<<v;
}

void JsonWriter::value(bool v){
	val_begin();
	os_<<(v?"true":"false");
}

void JsonWriter::null_val(){
	val_begin();
	os_<<"null";
}
