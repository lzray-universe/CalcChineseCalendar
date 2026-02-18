#pragma once

#include<iosfwd>
#include<string>
#include<vector>

std::string js_escape(const std::string&s);

class JsonWriter{
  public:
	explicit JsonWriter(std::ostream&os,bool pretty=true,int ind_size=2);

	void obj_begin();
	void obj_end();
	void arr_begin();
	void arr_end();

	void key(const std::string&name);

	void value(const std::string&v);
	void value(const char*v);
	void value(double v);
	void value(int v);
	void value(bool v);
	void null_val();

  private:
	struct Context{
		bool is_object=false;
		bool first=true;
		bool want_val=false;
	};

	std::ostream&os_;
	bool pretty_=true;
	int ind_size_=2;
	bool root_ok_=false;
	std::vector<Context> stack_;

	void put_indent(std::size_t depth);
	void val_begin();
};
