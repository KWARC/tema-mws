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
#ifndef _MWS_DBC_LEVFORMULADB_HPP
#define _MWS_DBC_LEVFORMULADB_HPP

/**
  * @file MemFormulaDb.hpp
  * @brief Formula Memory Database declarations
  * @author Radu Hambasan
  * @date 11 Dec 2013
  */

#include <stdlib.h>

#include <leveldb/db.h>

#include <string>

#include "FormulaDb.hpp"

#include "mws/types/NodeInfo.hpp"


namespace mws { namespace dbc {


class LevFormulaDb : public FormulaDb {
 public:
    LevFormulaDb();
    virtual ~LevFormulaDb();

    int open(const char* path);
    int create_new(const char* path);

    virtual int insertFormula(const mws::FormulaId&   formulaId,
                              const mws::CrawlId&     crawlId,
                              const mws::FormulaPath& formulaPath);

    virtual int queryFormula(const mws::FormulaId&  formulaId,
                             unsigned               limitMin,
                             unsigned               limitSize,
                             QueryCallback          queryCallback);

 private:
    leveldb::DB* mDatabase;
    uint32_t mCounter;
};

}  // namespace dbc
}  // namespace mws

#endif  // _MWS_DBC_LEVFORMULADB_HPP
