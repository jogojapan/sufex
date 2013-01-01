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
#include "lexicographical_renaming.hpp"

namespace sux {
  namespace skew {

    template <typename T> using noref = std::remove_reference<T>::type;

    template <typename Text, typename InpVector>
    lexicographical_renaming::result_type<InpVector> rename_lexicographically(
        Text                        &text,
        InpVector                   &trigrams,
        rlxutil::parallel::portions &portions,
        std::size_t                  center)
    {
      typedef typename rlxutil::elemtype<InpVector>::type elem_type;
      typedef typename elem_type::char_type char_type;
      typedef typename elem_type::pos_type  pos_type;

      return lexicographical_renaming::apply(trigrams,portions,
          sux::trigram_tools::content_equal<elem_type::impl,char_type,pos_type>,
          [center,&text,&trigrams](pos_type index)
                {
                   const pos_type pos = pos_of(text.begin(),trigrams[index]);
                   const pos_type mod = pos % 3;
                   const pos_type div = pos / 3;
                   return (mod == 1 ? div : center+div);
                });
    }

    template <typename InpVector,
              typename Posmap =
                  decltype(lexicographical_renaming::std_posmap<typename elem_type<InpVector>::pos_type>)>
    lexicographical_renaming::result_type<InpVector> rename_lexicographically(
        InpVector   &trigrams,
        bool        (&eq)       (const elem_type<InpVector> &, const elem_type<InpVector> &),
        unsigned    threads = 4,
        Posmap      &&posmap =
            lexicographical_renaming::std_posmap<typename elem_type<InpVector>::pos_type>,
        typename std::enable_if<lexicographical_renaming::is_posmap<Posmap>::value,int>::type = 0)
    {
      using std::forward;

      rlxutil::parallel::portions portions
      { trigrams.begin(), trigrams.end(), threads };
      return lexicographical_renaming::apply(trigrams,portions,eq,forward<Posmap>(posmap));
    }


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
