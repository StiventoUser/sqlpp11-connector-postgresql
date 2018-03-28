#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include <sqlpp11/chrono.h>
#include <sqlpp11/data_types.h>

namespace sqlpp
{
	namespace postgresql
	{
		namespace detail
		{
			::sqlpp::chrono::day_point parse_date(const char* const date_string, const size_t len, const bool debug = false);
			::sqlpp::chrono::microsecond_point parse_datetime(const char* const date_string, const size_t len, const bool debug = false);
		}
	}
}

#endif