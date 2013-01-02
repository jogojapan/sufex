/*
 * skew_test.cpp
 *
 *  Created on: 2013/01/02
 *      Author: jogojapan
 */


#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TrigramTest
#include <boost/test/included/unit_test.hpp>

#include <iostream>
#include <glog/logging.h>

#include "../trigram.hpp"
#include "../lexicographical_renaming.hpp"
#include "../skew.hpp"

BOOST_AUTO_TEST_CASE(sux_builder_skew_make_s0_test)
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

  sux::sort_23trigrams<sux::AlphabetClass::sparse>(trigrams,1);

  const std::size_t center
  { (trigrams.size() / 2) + (trigrams.size() % 2) };

  auto renamed =
      rlxalgo::skew::rename_lexicographically(text,trigrams,center,1);
  auto &name_str =
      rlxalgo::lexicographical_renaming::newstring_of(renamed);

  auto s0_trigrams =
      rlxalgo::skew::make_s0_trigrams<sux::TGImpl::structure>(
          text.begin(),text.end(),
          name_str.begin(),name_str.end(),
          1);

//  /* Debug output. */
//  for (const auto &trigram : s0_trigrams)
//    {
//      std::cout << trigram._pos
//          << '\t' << trigram._ch
//          << '\t' << name_str[trigram._renamed_s1]
//          << '\n';
//    }

  std::remove_reference<decltype(s0_trigrams)>::type expected
  { {0,'r',33},{3,'x',48},{6,'a',38},{9,'a',2},{12,'d',46},{15,'u',30},{18,'u',23},{21,'x',45},
    {24,'b',40},{27,'x',8},{30,'s',6},{33,'x',34},{36,'y',32},{39,'a',23},{42,'x',53},{45,'s',49},
    {48,'b',8},{51,'s',28},{54,'u',25},{57,'x',22},{60,'a',31},{63,'w',47},{66,'t',18},
    {69,'s',42},{72,'x',50},{75,'s',51},{78,'w',35},{81,'b',1},{84,'a',9},{87,'w',9},{90,'w',36},
    {93,'w',43}
  };

  BOOST_CHECK(equal(begin(expected),end(expected),begin(s0_trigrams)));
}


