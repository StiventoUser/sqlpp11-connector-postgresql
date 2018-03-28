#include "sqlpp11/postgresql/parse_utils.h"

#include <iostream>
#include <vector>

namespace sqlpp
{
	namespace postgresql
	{
		namespace detail
		{
			// same parsing logic as SQLite connector
			// PostgreSQL will return one of those (using the default ISO client):
			//
			// 2010-10-11 01:02:03 - ISO timestamp without timezone
			// 2011-11-12 01:02:03.123456 - ISO timesapt with sub-second (microsecond) precision
			// 1997-12-17 07:37:16-08 - ISO timestamp with timezone
			// 1992-10-10 01:02:03-06:30 - for some timezones with non-hour offset
			// 1900-01-01 - date only
			// we do not support time-only values !

			const auto date_digits = std::vector<char>{ 1, 1, 1, 1, 0, 1, 1, 0, 1, 1 };  // 2016-11-10
			const auto time_digits = std::vector<char>{ 0, 1, 1, 0, 1, 1, 0, 1, 1 };     // ' 13:12:11'
			const auto tz_digits = std::vector<char>{ 0, 1, 1 };                         // -05
			const auto tz_min_digits = std::vector<char>{ 0, 1, 1 };                     // :30

			auto check_digits(const char* text, const std::vector<char>& digitFlags) -> bool
			{
				for (const auto digitFlag : digitFlags)
				{
					if (digitFlag)
					{
						if (not std::isdigit(*text))
						{
							return false;
						}
					}
					else
					{
						if (std::isdigit(*text) or *text == '\0')
						{
							return false;
						}
					}
					++text;
				}
				return true;
			}

			::sqlpp::chrono::day_point parse_date(const char* const date_string, const size_t len, const bool debug)
			{
				if (len >= date_digits.size() && check_digits(date_string, date_digits))
				{
					const auto ymd =
						::date::year(std::atoi(date_string)) / std::atoi(date_string + 5) / std::atoi(date_string + 8);
					return ::sqlpp::chrono::day_point(ymd);
				}
				else
				{
					if (debug)
						std::cerr << "PostgreSQL debug: got invalid date '" << date_string << "'" << std::endl;
					return {};
				}
			}

			::sqlpp::chrono::microsecond_point parse_datetime(const char* const date_string, const size_t len, const bool debug)
			{
				::sqlpp::chrono::microsecond_point value;

				if (len >= date_digits.size() && check_digits(date_string, date_digits))
				{
					const auto ymd =
						::date::year(std::atoi(date_string)) / std::atoi(date_string + 5) / std::atoi(date_string + 8);
					value = ::sqlpp::chrono::day_point(ymd);
				}
				else
				{
					if (debug)
						std::cerr << "PostgreSQL debug: got invalid date_time" << std::endl;
					return {};
				}

				auto date_time_size = date_digits.size() + time_digits.size();
				const auto time_string = date_string + date_digits.size();
				if ((len >= date_time_size) && check_digits(date_string, date_digits))
				{
					// not the ' ' (or standard: 'T') prefix for times
					value += std::chrono::hours(std::atoi(time_string + 1)) + std::chrono::minutes(std::atoi(time_string + 4)) +
						std::chrono::seconds(std::atoi(time_string + 7));
				}
				else
				{
					return {};
				}

				bool has_ms = false;
				if ((len > date_time_size) && (time_string[time_digits.size()] == '.'))
				{
					has_ms = true;
					const auto ms_string = time_string + time_digits.size() + 1;

					int digits_count = 0;
					while (ms_string[digits_count] != '\0' && ms_string[digits_count] != '-' && ms_string[digits_count] != '+')
					{
						if (!std::isdigit(ms_string[digits_count]))
						{
							if (debug)
								std::cerr << "PostgreSQL debug: got invalid date_time" << std::endl;
							return {};
						}

						++digits_count;
					}

					if (digits_count == 0)
					{
						if (debug)
							std::cerr << "PostgreSQL debug: got invalid date_time" << std::endl;
						return {};
					}

					date_time_size += digits_count;

					int pg_ms_num = std::atoi(ms_string);

					while (digits_count++ < 6)
						pg_ms_num *= 10;

					value += std::chrono::microseconds(pg_ms_num);
				}
				if (len >= (date_time_size + tz_digits.size()))
				{
					const auto tz_string = date_string + date_time_size;
					const auto zone_hour = std::atoi(tz_string);
					auto zone_min = 0;

					if ((len >= date_time_size + tz_digits.size() + tz_min_digits.size()) &&
						check_digits(tz_string + tz_digits.size(), tz_min_digits))
					{
						zone_min = std::atoi(tz_string + tz_digits.size() + 1);
					}
					// ignore -00:xx, as there currently is no timezone using it, and hopefully never will be
					if (zone_hour >= 0)
					{
						value -= std::chrono::hours(zone_hour) + std::chrono::minutes(zone_min);
					}
					else
					{
						value += std::chrono::hours(zone_hour) + std::chrono::minutes(zone_min);
					}
				}
				if (debug)
				{
					auto ts = std::chrono::system_clock::to_time_t(value);
					std::tm* tm = std::localtime(&ts);
					std::string time_str{ "1900-01-01 00:00:00 CEST" };
					strftime(const_cast<char*>(time_str.data()), time_str.size(), "%F %T %Z", tm);
					std::cerr << "PostgreSQL debug: calculated timestamp " << time_str << std::endl;
				}
			}
		}
	}
}