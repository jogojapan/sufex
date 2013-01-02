/*
 * testapp.cpp
 *
 *  Created on: 2012/12/31
 *      Author: jogojapan
 */

#include <iostream>
#include <glog/logging.h>

#include "../trigram.hpp"
#include "../lexicographical_renaming.hpp"
#include "../skew.hpp"

int main()
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
      rlxalgo::skew::make_s0_trigrams<sux::TGImpl::structure,char,pos_type>(
          text.begin(),text.end(),
          name_str.begin(),name_str.end(),
          1);

  /* Debug output. */
  for (const auto &trigram : s0_trigrams)
    {
      std::cout << trigram._pos
          << '\t' << trigram._ch
          << '\t' << trigram._renamed_s1
          << '\n';
    }
  return 0;
}
