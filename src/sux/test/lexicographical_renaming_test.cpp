/*
 * lexicographical_renaming_test.cpp
 *
 *  Created on: 2013/01/01
 *      Author: jogojapan
 */


#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TrigramTest
#include <boost/test/included/unit_test.hpp>

#include <iostream>
#include <glog/logging.h>

#include "../trigram.hpp"
#include "../lexicographical_renaming.hpp"
#include "../../util/proctime.hpp"

BOOST_AUTO_TEST_CASE(sux_builder_lexicographical_renaming)
{
  typedef unsigned short pos_type;

  /* Lexicographical renaming is tested only for one particular
   * implemenatation of trigrams, `TGImpl::pointer`, because
   * tests for this are the easiest to code. Aspects of the
   * other implementations that would affect lexicographical
   * renaming are assumed to be covered by other tests. */

  typedef sux::TrigramMaker<sux::TGImpl::pointer,char,pos_type>   maker;
  typedef typename maker::trigram_type                            elem_type;
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

  std::vector<pos_type> expected
  { 0,1,2,2,2,2,3,4,4,4,5,5,5,5,5,6,6,7,8,9,10,11,12,13,14,15 };

  typedef rlxalgo::lexicographical_renaming lex;

  auto results = rlxalgo::rename_lexicographically(input,
      sux::trigram_tools::content_equal<sux::TGImpl::pointer,char,pos_type>);
  std::vector<pos_type> renamed_vec
  { lex::move_newstring_from(results) };

  for (const auto &elem : renamed_vec)
    {
      std::cout << elem << ',';
    }
  std::cout << std::endl;

  BOOST_CHECK(equal(begin(expected),end(expected),begin(renamed_vec)));
}

BOOST_AUTO_TEST_CASE(sux_builder_lexicographical_renaming_skew)
{
  using sux::trigram_tools::to_str;
  using sux::trigram_tools::pos_of;

  typedef unsigned short                                          pos_type;
  typedef sux::TrigramMaker<sux::TGImpl::pointer,char,pos_type>   maker;
  typedef typename maker::trigram_type                            elem_type;

  std::string text
  { "ruxxysaxaaabdyduuuusuxyabxbxbbsbaxuxyuxasuxytsysbbbstxusyxstauwwyqtqysxuxyssyswwbbababbwbbwwww" };

  auto trigrams =
      maker::make_23trigrams(text.begin(),text.end());

  sux::sort_23trigrams<rlx::AlphabetClass::sparse>(trigrams,1);

  std::vector<pos_type> expected
  { 33 , 48 , 38 , 2 , 46 , 30 , 23 , 45 , 40 , 8 , 6 , 34 , 32 , 23 , 53 , 49 , 8 , 28 ,
    25 , 22 , 31 , 47 , 18 , 42 , 50 , 51 , 35 , 1 , 9 , 9 , 36 , 43 , 19 , 0 , 10 , 16 ,
    29 , 34 , 3 , 15 , 11 , 4 , 44 , 39 , 34 , 27 , 20 , 12 , 41 , 54 , 26 , 37 , 17 , 52 ,
    34 , 21 , 24 , 7 , 5 , 13 , 14 };

  typedef rlxalgo::lexicographical_renaming lex;

  const std::size_t center
  { (trigrams.size() / 2) + (trigrams.size() % 2) };
  auto results = rlxalgo::rename_lexicographically(trigrams,
      sux::trigram_tools::content_equal<sux::TGImpl::pointer,char,pos_type>,1,
      [center,&text,&trigrams](pos_type index)
      {
         const pos_type pos = pos_of(text.begin(),trigrams[index]);
         const pos_type mod = pos % 3;
         const pos_type div = pos / 3;
         return (mod == 1 ? div : center+div);
      });

  auto &name_str = lex::newstring_of(results);

//  /* Debug output. */
//  for (std::size_t i = 0 ; i < trigrams.size() ; ++i)
//    {
//      std::cout << pos_of(text.begin(),trigrams[i])
//          << '\t' << (pos_of(text.begin(),trigrams[i]) % 3)
//          << '\t' << to_str(trigrams[i])
//          << '\t' << name_str[i] << '\n';
//    }

  BOOST_CHECK(equal(begin(expected),end(expected),begin(name_str)));
}
