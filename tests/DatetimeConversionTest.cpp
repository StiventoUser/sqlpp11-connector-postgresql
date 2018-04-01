#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <sqlpp11/postgresql/exception.h>
#include <sqlpp11/postgresql/postgresql.h>
#include <sqlpp11/sqlpp11.h>

#include <cassert>
#include <iostream>
#include <vector>

#include <sqlpp11/postgresql/parse_utils.h>

using namespace std::literals::chrono_literals;
using namespace date::literals;

namespace
{
	template <typename L, typename R>
	auto require_equal(int line, const L& l, const R& r) -> void
	{
		if (l != r)
		{
			std::cerr << line << ": ";
			serialize(::sqlpp::wrap_operand_t<L>{l}, std::cerr);
			std::cerr << " != ";
			serialize(::sqlpp::wrap_operand_t<R>{r}, std::cerr);
			throw std::runtime_error("Unexpected result");
		}
	}
}

int DatetimeConversionTest(int, char*[])
{
	const std::string date_string = "2016-11-10";
	const auto date_res =sqlpp::postgresql::detail::parse_date(date_string.c_str(), date_string.size(), true);

	require_equal(__LINE__, date_res, date::sys_days{ 2016_y / nov / 10 });

	std::string time_string = "2018-02-25 20:30:10";
	auto time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 0us);

	time_string = "2018-02-25 20:30:10.123456";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123456us);
  
	time_string = "2018-02-25 20:30:10.123000";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123000us);

	time_string = "2018-02-25 20:30:10.000123";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123us);

	time_string = "2018-02-25 20:30:10.1";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 100000us);

	time_string = "2018-02-25 20:30:10.123456+02";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123456us - 2h);

	time_string = "2018-02-25 20:30:10.123456-02";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123456us + 2h);

	time_string = "2018-02-25 20:30:10.123456+02:30";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123456us - 2h - 30min);

	time_string = "2018-02-25 20:30:10.123456-02:30";
	time_res = sqlpp::postgresql::detail::parse_datetime(time_string.c_str(), time_string.size(), true);

	require_equal(__LINE__, time_res, date::sys_days{ 2018_y / feb / 25 } + 20h + 30min + 10s + 123456us + 2h + 30min);

  return 1;
}
