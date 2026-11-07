/*
 * jappeos_core, Core system management daemon for JappeOS.
 * Copyright (C) 2026  The JappeOS team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdlib>
#include <iosfwd>
#include <sstream>
#include <string>
#include <execinfo.h>
#include <cxxabi.h>

namespace JappeStudios::JappeOS::DaemonCore::Utils
{
    class StackTrace final
    {
    public:
        StackTrace() = delete;

        static std::string Print(unsigned int max_frames = 64)
        {
            std::ostringstream oss;

            // Storage array for stack trace address data
            void* addrlist[max_frames + 1];

            // Retrieve current stack addresses
            const int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
            if (addrlen == 0)
            {
                oss << "<empty, possibly corrupt>\n";
                return oss.str();
            }

            // Resolve addresses into strings containing "filename(function+address)"
            char** symbols = backtrace_symbols(addrlist, addrlen);
            if (!symbols)
            {
                oss << "<backtrace_symbols failed>\n";
                return oss.str();
            }

            // Iterate over returned symbol lines
            for (int i = 0; i < addrlen; i++)
            {
                std::string sym(symbols[i]);
                std::string function_name = sym;

                // Try to extract the mangled name between '(' and '+'
                const size_t begin = sym.find('(');
                size_t end   = sym.find('+', begin);
                if (begin != std::string::npos && end != std::string::npos)
                {
                    std::string mangled = sym.substr(begin + 1, end - begin - 1);
                    int status = 0;
                    char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
                    if (status == 0 && demangled)
                    {
                        function_name = sym.substr(0, begin + 1) + demangled + sym.substr(end);
                        std::free(demangled);
                    }
                }

                oss << "[" << i << "] " << function_name << "\n";
            }

            free(symbols);
            return oss.str();
        }
    };
}