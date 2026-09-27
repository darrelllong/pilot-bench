/*
 * pilot-cli.h
 *
 * Copyright (c) 2017-2019 Yan Li <yanli@tuneup.ai>. All rights reserved.
 * The Pilot tool and library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License version 2.1 (not any other version) as published by the Free
 * Software Foundation.
 *
 * The Pilot tool and library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program in file lgpl-2.1.txt; if not, see
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * Commit 033228934e11b3f86fb0a4932b54b2aeea5c803c and before were
 * released with the following license:
 * Copyright (c) 2015, 2016, University of California, Santa Cruz, CA, USA.
 * Created by Yan Li <yanli@tuneup.ai>,
 * Department of Computer Science, Baskin School of Engineering.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Storage Systems Research Center, the
 *       University of California, nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CLI_PILOT_CLI_H_
#define CLI_PILOT_CLI_H_

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <common.h>
#include <config.h>
#include <cstring>
#include <iostream>
#include <pilot/libpilot.h>
#include <sstream>
#include <stdexcept>
#include <vector>

#define GREETING_MSG "Pilot " stringify(PILOT_VERSION_MAJOR) "." \
    stringify(PILOT_VERSION_MINOR) " (compiled by " CC_VERSION " on " __DATE__ ")"

int handle_analyze(int argc, const char** argv);
int handle_run_program(int argc, const char** argv);
int handle_detect_changepoint_edm(int argc, const char** argv);

inline std::string get_timestamp(void) {
    using namespace boost::posix_time;
    ptime now = second_clock::universal_time();

    static std::locale loc(std::cout.getloc(),
                      new time_facet("%Y%m%d_%H%M%S"));

    std::stringstream ss;
    ss.imbue(loc);
    ss << now;
    return ss.str();
}

/**
 * \brief Split a line into fields
 * \details Fields are separated by a comma or by whitespace. Whitespace next
 * to a comma is part of the separator, so "1, 2" has two fields. Whitespace
 * at the beginning and at the end of the line is ignored, which includes the
 * carriage return of files generated on Windows. Only commas can make an
 * empty field: "1,,2" has three fields.
 * @param line the line
 * @return the fields. A line that has nothing in it has one empty field.
 */
inline std::vector<std::string> split_csv_line(const std::string &line) {
    using namespace std;
    static const char * const whitespace = " \t\r\n";
    vector<string> fields;
    const size_t begin = line.find_first_not_of(whitespace);
    if (string::npos == begin) {
        fields.push_back(string());
        return fields;
    }
    const size_t end = line.find_last_not_of(whitespace) + 1;
    string field;
    size_t pos = begin;
    while (pos < end) {
        const char c = line[pos];
        if (',' == c) {
            fields.push_back(field);
            field.clear();
            pos = line.find_first_not_of(whitespace, pos + 1);
        } else if (NULL != strchr(whitespace, c)) {
            // pos < end, so there is a character that is not whitespace
            pos = line.find_first_not_of(whitespace, pos);
            // the comma that follows ends the field
            if (',' != line[pos]) {
                fields.push_back(field);
                field.clear();
            }
        } else {
            field.push_back(c);
            ++pos;
        }
    }
    fields.push_back(field);
    return fields;
}

template <typename ResultType>
std::vector<ResultType> extract_csv_fields(const std::string &csvstr,
                                           const std::vector<int> &columns) {
    using namespace std;
    using namespace boost;
    vector<string> pidata_strs = split_csv_line(csvstr);
    vector<ResultType> r(columns.size());
    for (size_t i = 0; i < columns.size(); ++i) {
        int col = columns[i];
        if (col < 0 || col >= static_cast<int>(pidata_strs.size())) {
            throw runtime_error("Malformed line");
        }
        r[i] = lexical_cast<ResultType>(pidata_strs[col]);
    }
    return r;
}

/**
 * The longest round duration that the client program can report, which is
 * ten years. It keeps a number that is not a duration, such as a throughput
 * in bytes per second, from getting into the analysis.
 */
const double MAX_ROUND_DURATION_IN_SEC = 10.0 * 366 * 24 * 3600;

/**
 * \brief Get the round duration from the output of the client program
 * @param[in] prog_stdout the output of the client program
 * @param duration_col the column (0-based) of the round duration in seconds
 * @param[out] round_duration the round duration in nanoseconds. It is 0 only
 * if the client program reported 0.
 * @return 0 on success; ERR_WL_FAIL if the column doesn't exist or is not a
 * valid duration, in which case round_duration is not changed
 */
int parse_round_duration(const std::string &prog_stdout, size_t duration_col,
                         boost::timer::nanosecond_type *round_duration);

inline void print_read_the_doc_info(void) {
    std::cerr << "To understand the math behind Pilot or read tutorials, please read the" << std::endl;
    std::cerr << "documentation at https://docs.ascar.io/" << std::endl;
}

#endif /* CLI_PILOT_CLI_H_ */
