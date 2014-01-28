/*

Copyright (C) 2010-2013 KWARC Group <kwarc.info>

This file is part of MathWebSearch.

MathWebSearch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MathWebSearch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MathWebSearch.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef _MWSANSWSET_HPP
#define _MWSANSWSET_HPP

/**
  * @brief File containing the header of the MwsAnswset class.
  *
  * @file MwsAnswset.hpp
  * @author Corneliu-Claudiu Prodescu
  * @date 27 Apr 2011
  */

#include <cstdio>
#include <string>
#include <vector>

#include "mws/types/Answer.hpp"

namespace mws {

struct Qvar {
    std::string name;
    FormulaPath xpath;

    Qvar(const std::string& name, const FormulaPath& formulaPath) :
        name(name), xpath(formulaPath) {}
};

/**
  * @brief <mws:answset> Answer Set
  *
  */
struct MwsAnswset
{
    /// Vector containing the MWS Answers
    std::vector<mws::types::Answer*> answers;
    /// Total number of solutions in the index
    int total;
    std::vector<Qvar> qvars;

    MwsAnswset() : total(0) {
    }

    ~MwsAnswset() {
        for (auto answer : answers) {
            delete answer;
        }
    }
};

}  // namespace mws

#endif // _MWSANSWSET_HPP
