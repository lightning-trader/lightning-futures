﻿/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
#include <string>
#include <vector>
namespace lt
{
    class string_helper
    {
    public:
        static std::vector<std::string> split(const std::string& str, char delim) {
            std::size_t previous = 0;
            std::size_t current = str.find(delim);
            std::vector<std::string> elems;
            while (current != std::string::npos) {
                if (current >= previous) {
                    elems.push_back(str.substr(previous, current - previous));
                }
                previous = current + 1;
                current = str.find(delim, previous);
            }
            if (previous != str.size()) {
                elems.push_back(str.substr(previous));
            }
            return elems;
        }
        static std::string to_string(const char* value)
        {
            return std::string(value);
        }
        static std::string to_string(const std::string& value)
        {
            return (value);
        }
        template<typename T>
        static std::string to_string(const T& value)
        {
            return std::to_string(value);
        }

        template<typename T>
        static std::string to_string(const std::vector<T>& values)
        {
            std::string result;
            for (auto value : values) {
                result.append(value);
            }
            return result;
        }
        template<typename T>
        static std::string extract_to_string(const T& value)
        {
            return to_string(value);
        }

        template<typename A, typename... Types>
        static void extract_to_string(std::vector<std::string>& data, const A& firstArg, const Types&... args) {
            data.emplace_back(to_string(firstArg));
            extract_to_string(data, args...);
        }


        template<typename T>
        static bool contains(const std::vector<T>& values, const T value) {
            return std::find(values.begin(), values.end(), value) != values.end();
        }

        template<typename... T>
        static std::string join(const std::string& separator, const std::vector<T> &... values) {
            std::vector<std::string> args;
            extract_to_string(args, values...);
            std::string result;
            for (const auto& s : args) {
                result.append(s);
                result.append(separator);
            }

            return result;
        }


        template<typename... T>
        static std::string format(const std::string& s, const T &... values) {
            std::vector<std::string> args;
            extract_to_string(args, values...);
            std::string result = s;
            char open = '{';
            char close = '}';
            bool is_open = false;
            size_t index = 0;
            size_t index_length = 0;
            size_t start = 0;

            for (size_t i = 0; i < result.length(); ++i) {
                char c = result.at(i);

                if (c == '\\') {
                    i += 1;
                }
                else if (c == open) {
                    is_open = true;
                    start = i;
                }
                else if (c == close) {
                    size_t one = result.length();
                    result.erase(start, i + 1 - start);

                    if (index_length > 0) {
                        result.insert(start, args.at(index));
                    }

                    size_t two = result.length();

                    i -= one - two;
                    is_open = false;
                    index = 0;
                    index_length = 0;
                }
                else if (is_open) {
                    int n = c - '0';

                    if (n < 0 || n > 9) {
                        continue;
                    }

                    index = (index * 10) + n;
                    ++index_length;
                }
            }

            return result;
        }
    };
}
