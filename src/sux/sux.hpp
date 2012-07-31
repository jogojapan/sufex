/*
 * sux.hpp
 *
 *  Created on: 2012/07/31
 *      Author: jogojapan
 */

#ifndef SUX_HPP_
#define SUX_HPP_

#include <utility>
#include <tuple>
#include <vector>
#include <algorithm>

namespace sux {

  template <typename Char, typename Pos>
  class SuxBuilder
  {
    typedef std::tuple<Pos,Char,Char,Char> Trigram;

    template <typename Iterator>
    static std::vector<Trigram> make_23trigrams(Iterator from, Iterator to)
    {
      short pos { 0 };
      std::vector<Trigram> result;
      if (std::distance(from,to) >= 5)
      {
        while (from != to)
        {
          /* Position 0, skip. */
          ++from; ++pos;
          /* Two trigrams. */
          const Char c1 { *from };
          const Char c2 { *(from+1) };
          const Char c3 { *(from+2) };
          /* Position 1, copy. */
          result.push_back(std::make_tuple(pos,c1,c2,c3));
          ++from; ++pos;
          /* Position 2, copy. */
          const Char c4 { *(from+3) };
          result.push_back(std::make_tuple(pos,c2,c3,c4));
          /* Next iteration. */
          ++from; ++pos;
        }
      std::for_each(from,to,[&pos](const Char &c) {
        ;
      });
      }
    }

  };

}


#endif /* SUX_HPP_ */
