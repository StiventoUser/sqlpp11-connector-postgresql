/**
 * Copyright © 2014-2015, Matthijs Möhlmann
 * Copyright © 2015-2016, Bartosz Wieczorek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sqlpp11/exception.h>
#include <sqlpp11/postgresql/bind_result.h>
#include "sqlpp11/postgresql/parse_utils.h"

#include <date/date.h>
#include <iostream>
#include <sstream>

#include "detail/prepared_statement_handle.h"

#if defined(_WIN32) || defined(_WIN64)
#pragma warning (disable:4800)  // int to bool
#endif

namespace sqlpp
{
  namespace postgresql
  {
    bind_result_t::bind_result_t(const std::shared_ptr<detail::statement_handle_t>& handle) : _handle(handle)
    {
      if (this->_handle && this->_handle->debug())
      {
        // cerr
        std::cerr << "PostgreSQL debug: constructing bind result, using handle at: " << this->_handle.get()
                  << std::endl;
      }
    }

    bool bind_result_t::next_impl()
    {
      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: accessing next row of handle at " << _handle.get() << std::endl;
      }

      // Fetch total amount
      if (_handle->totalCount == 0U)
      {
        _handle->totalCount = _handle->result.records_size();
        if (_handle->totalCount == 0U)
          return false;
      }
      else
      {
        // Next row
        if (_handle->count < (_handle->totalCount - 1))
        {
          _handle->count++;
        }
        else
        {
          return false;
        }
      }

      // Really needed?
      if (_handle->fields == 0U)
      {
        _handle->fields = _handle->result.field_count();
      }

      return true;
    }

    void bind_result_t::_bind_boolean_result(size_t _index, signed char* value, bool* is_null)
    {
      auto index = static_cast<int>(_index);
      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: binding boolean result at index: " << index << std::endl;
      }

      *is_null = _handle->result.isNull(_handle->count, index);
      *value = _handle->result.getValue<bool>(_handle->count, index);
    }

    void bind_result_t::_bind_floating_point_result(size_t _index, double* value, bool* is_null)
    {
      auto index = static_cast<int>(_index);
      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: binding floating_point result at index: " << index << std::endl;
      }

      *is_null = _handle->result.isNull(_handle->count, index);
      *value = _handle->result.getValue<double>(_handle->count, index);
    }

    void bind_result_t::_bind_integral_result(size_t _index, int64_t* value, bool* is_null)
    {
      auto index = static_cast<int>(_index);
      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: binding integral result at index: " << index << std::endl;
      }

      *is_null = _handle->result.isNull(_handle->count, index);
      *value = _handle->result.getValue<unsigned long long>(_handle->count, index);
    }

    void bind_result_t::_bind_text_result(size_t _index, const char** value, size_t* len)
    {
      auto index = static_cast<int>(_index);
      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: binding text result at index: " << index << std::endl;
      }

      if (_handle->result.isNull(_handle->count, index))
      {
        *value = nullptr;
        *len = 0;
      }
      else
      {
        *value = _handle->result.getValue<const char*>(_handle->count, index);
        *len = _handle->result.length(_handle->count, index);
      }
    }

    void bind_result_t::_bind_date_result(size_t _index, ::sqlpp::chrono::day_point* value, bool* is_null)
    {
      auto index = static_cast<int>(_index);

      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: binding date result at index: " << index << std::endl;
      }

      *is_null = _handle->result.isNull(_handle->count, index);

      if (!(*is_null))
      {
        const auto date_string = _handle->result.getValue<const char*>(_handle->count, index);

        if (_handle->debug())
        {
          std::cerr << "PostgreSQL debug: date string: " << date_string << std::endl;
        }
        auto len = static_cast<size_t>(_handle->result.length(_handle->count, index));

		*value = detail::parse_date(date_string, len, _handle->debug());
      }
      else
      {
        *value = {};
      }
    }

    // always returns local time for timestamp with time zone
    void bind_result_t::_bind_date_time_result(size_t _index, ::sqlpp::chrono::microsecond_point* value, bool* is_null)
    {
      auto index = static_cast<int>(_index);
      if (_handle->debug())
      {
        std::cerr << "PostgreSQL debug: binding date_time result at index: " << index << std::endl;
      }

      *is_null = _handle->result.isNull(_handle->count, index);

      if (!(*is_null))
      {
        const auto date_string = _handle->result.getValue(_handle->count, index);

        if (_handle->debug())
        {
          std::cerr << "PostgreSQL debug: got date_time string: " << date_string << std::endl;
        }

		auto len = static_cast<size_t>(_handle->result.length(_handle->count, index));

		*value = detail::parse_datetime(date_string, len, _handle->debug());
      }
      else
      {
        *value = {};
      }
    }
  }
}
