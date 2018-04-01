#include "sqlpp11/postgresql/parse_utils.h"

#include <iostream>
#include <vector>

#include <date/date.h>

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

				value = parse_date(date_string, len, debug);

				// TODO check for empty?



				auto date_time_size = date_digits.size() + time_digits.size();
				const auto time_string = date_string + date_digits.size();

				if ((len >= date_time_size) && check_digits(time_string, time_digits))
				{
					const auto hourOffset = 1; // ' 12'
					const auto minuteOffset = hourOffset + 3; // ' 12:13'
					const auto secondOffset = minuteOffset + 3; // ' 12:13:14'

					// not the ' ' (or standard: 'T') prefix for times
					value += std::chrono::hours(std::atoi(time_string + hourOffset)) +
						std::chrono::minutes(std::atoi(time_string + minuteOffset)) +
						std::chrono::seconds(std::atoi(time_string + secondOffset));
				}
				else
				{
					return {};
				}

				if (len > date_time_size && date_string[date_time_size] == '.')
				{
					const auto ms_string = date_string + date_time_size + 1;

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

					date_time_size += digits_count + 1;

					int pg_ms_num = std::atoi(ms_string);

					while (digits_count++ < 6)
						pg_ms_num *= 10;

					value += std::chrono::microseconds(pg_ms_num);
				}

				if (len > date_time_size)
				{
					const auto tz_string = date_string + date_time_size;
					const auto tz_colon = tz_string + 3;

					const auto zone_hour = std::atoi(tz_string);
					auto zone_min = 0;

					if (tz_colon[0] == ':')
					{
						zone_min = std::atoi(tz_colon + 1);
					}

					if (zone_hour >= 0)
					{
						value -= std::chrono::hours(zone_hour) + std::chrono::minutes(zone_min);
					}
					else
					{
						value += std::chrono::hours(-zone_hour) + std::chrono::minutes(zone_min);
					}
				}
				if (debug)
				{
					std::cerr << "PostgreSQL debug: calculated timestamp " << ::date::format("%F %T", value) << std::endl;
				}

				return value;
			}
		}
	}
}