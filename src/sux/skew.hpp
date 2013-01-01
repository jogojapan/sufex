/*
 * skew.hpp
 *
 *  Created on: 2013/01/01
 *      Author: jogojapan
 */

#ifndef SKEW_HPP_
#define SKEW_HPP_

#include <limits>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <stack>

#include "../util/more_type_traits.hpp"
#include "alphabet.hpp"
#include "trigram.hpp"

namespace sux {
  namespace skew {

    template <typename T> using noref = std::remove_reference<T>::type;

    template <typename Pos, typename It>
    void make_suffix_array(It from, It to, unsigned threads = 4)
    {
      using std::numeric_limits;
      using std::is_integral;
      using std::uintmax_t;
      using std::distance;

      typedef typename rlxutil::deref<It>::type char_type;

      static_assert(is_integral<Pos>::value,
          "The position type used for make_suffix_array must be an integral type.");
      if (static_cast<uintmax_t>(numeric_limits<Pos>::max())
          > static_cast<uintmax_t>(distance(from,to)))
          throw std::out_of_range("Attempt to use a position type with make_suffix_array that "
              "is not large enough for the given input string");

      using sux::AlphabetClass;
      using lex      = sux::lexicographical_renaming;
      using recusion = lex::recusion;

      /* Extract 2,3-trigrams. */
      auto trigrams = sux::extract_23trigrams<Pos>(from,to);
      /* Sort them. */
      sux::sort_23trigrams<AlphabetClass::sparse>(trigrams,threads);
      /* Generate an integer alphabet for them according to
       * their sorting order. */
      auto renamed_trigrams =
          sux::rename_lexicographically(trigrams,threads);

      using std::stack;
      using std::pair;
      using std::move;

      if (lex::is<recursion::needed>(renamed_trigrams))
        {
          auto renamed_string =
              lex::move_newstring_from(renamed_trigrams);
          auto renamed_trigrams =
              sux::extract_23trigrams<Pos>(renamed_string.begin(),renamed_string.end());
          sux::sort_23trigrams<AlphabetClass::zero_range>(renamed_trigrams,threads);

          stack<pair<noref<decltype(renamed_string)>,noref<decltype(renamed_trigrams)>> workpile
              { };

          workpile.emplace(move(renamed_string),move(renamed_trigrams));
          while (!workpile.empty())
            {
              renamed_trigrams =
                  sux::rename_lexicographically(std::get<1>(workpile.top()),threads);
              if (lex::is<recursion::needed>(renamed_trigrams))
                {
                  /* Recursion. */
                  renamed_string =
                      lex::move_newstring_from(renamed_trigrams);
                  renamed_trigrams =
                      sux::extract_23trigrams<Pos>(renamed_string.begin(),renamed_string.end());
                  sux::sort_23trigrams<AlphabetClass::zero_range>(renamed_trigrams,threads);
                  workpile.emplace(move(renamed_string),move(renamed_trigrams));
                }
              else
                {
                  /* End of recursion. Wind up the workpile. */

                }
            }
        }
    }

  }
}


#endif /* SKEW_HPP_ */
