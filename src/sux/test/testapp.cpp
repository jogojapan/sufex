/*
 * testapp.cpp
 *
 *  Created on: 2012/12/31
 *      Author: jogojapan
 */

#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ratio>
#include <glog/logging.h>

#include "../trigram.hpp"
#include "../alphabet.hpp"
#include "../../util/random.hpp"
#include "../../util/proctime.hpp"

int main()
{
  typedef unsigned short Pos;

  typedef sux::TrigramMaker<sux::TGImpl::pointer,char,Pos>   maker;
  typedef typename maker::trigram_type                       elem_type;
  std::vector<elem_type> input {
    elem_type { "aec" },
    elem_type { "aef" },
    elem_type { "bhj" },
    elem_type { "bhj" },
    elem_type { "bhj" },
    elem_type { "bhj" },
    elem_type { "dkh" },
    elem_type { "dnr" },
    elem_type { "dnr" },
    elem_type { "dnr" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "kuw" },
    elem_type { "kuw" },
    elem_type { "lpp" },
    elem_type { "lpy" },
    elem_type { "qqq" },
    elem_type { "qxz" },
    elem_type { "rst" },
    elem_type { "rsu" },
    elem_type { "rua" },
    elem_type { "rub" },
    elem_type { "ruc" }
  };

  std::vector<Pos> expected
  { 0,1,2,2,2,2,3,4,4,4,5,5,5,5,5,6,6,7,8,9,10,11,12,13,14,15 };

  typedef sux::lexicographical_renaming lex;

  auto results = sux::rename_lexicographically(input);
  std::vector<Pos> renamed_vec
  { lex::move_newstring_from(results) };

  return 0;
}
