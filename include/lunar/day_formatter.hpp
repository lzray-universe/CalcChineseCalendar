#pragma once

#include<iosfwd>
#include<string>

#include "lunar/models.hpp"

namespace lunar::core{

void format_day_output(std::ostream&os,const DayResult&result,
					   const std::string&format,bool pretty);

}
