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
#include "../types/algotypes.hpp"
#include "alphabet.hpp"
#include "trigram.hpp"
#include "../util/parallelization.hpp"
#include "lexicographical_renaming.hpp"

namespace rlxalgo {
  namespace skew {

    template <typename T> using noref = typename std::remove_reference<T>::type;

    template <typename Text, typename InpVector>
    lexicographical_renaming::result_type<InpVector> rename_lexicographically(
        const Text                  &text,
        const InpVector             &trigrams,
        rlxutil::parallel::portions &portions,
        std::size_t                  center)
    {
      using namespace rlx::algotypes;
      using namespace sux::trigram_tools;
      typedef ngram_vec<InpVector> vec;

      return lexicographical_renaming::apply(trigrams,portions,
          sux::trigram_tools::content_equal<elemtype<vec>::impl,chtype<vec>,postype<vec>>,
          [center,&text,&trigrams](postype<vec> index)
                {
                   const postype<vec> pos = pos_of(text.begin(),trigrams[index]);
                   const postype<vec> mod = pos % 3;
                   const postype<vec> div = pos / 3;
                   return (mod == 1 ? div : center+div);
                });
    }

    template <typename Text, typename InpVector>
    lexicographical_renaming::result_type<InpVector> rename_lexicographically(
        const Text &text, const InpVector &trigrams, std::size_t center, unsigned threads = 4)
    {
      rlxutil::parallel::portions portions
      { trigrams.begin(), trigrams.end(), threads };

      return rename_lexicographically(text,trigrams,portions,center);
    }

    template <sux::TGImpl tgimpl, typename Char, typename Pos>
    struct S0TrigramImpl;

    template <typename Char, typename Pos>
    struct S0TrigramImpl<sux::TGImpl::structure,Char,Pos>
    {
      Pos   _pos;        // Position of S0-trigram
      Char  _ch;         // First character
      Pos   _renamed_s1; // Lexicographically renamed trigram that follows

      bool operator==(const S0TrigramImpl &other) const
      { return (_pos == other._pos && _ch == other._ch && _renamed_s1 == other._renamed_s1); }
    };

    template <sux::TGImpl tgimpl, typename It, typename LexIt>
    typename sux::TrigramContainer
             <S0TrigramImpl
                <tgimpl,typename rlxtype::deref<It>::type,typename rlxtype::deref<LexIt>::type>>
                ::vec_type
    make_s0_trigrams(It from, It to, LexIt lex_from, LexIt lex_to, unsigned threads)
    {
      using namespace rlx::algotypes;
      using std::distance;
      using it    = char_it<It>;
      using lexit = char_it<LexIt>;

      typedef rlxutil::parallel::portions::adjustment                adjustment;
      typedef S0TrigramImpl<tgimpl,chtype<it>,chtype<lexit>>         trigram_type;
      typedef typename sux::TrigramContainer<trigram_type>::vec_type vec_type;

      if (distance(lex_from,lex_to) < distance(from,to) / 3)
        throw std::out_of_range("Length of lexicographically renamed string not large enough");

      vec_type vec(distance(from,to) / 3 + (distance(from,to) % 3 != 0 ? 1 : 0));

      rlxutil::parallel::portions portions
      { from , to , threads ,
        [](It beg, It loc, It /*end*/)
        { return (distance(beg,loc) % 3 != 0 ? adjustment::needed : adjustment::unneeded); }
      };

      typedef chtype<lexit> pos_type;

      auto futs =
          portions.apply(from,to,
              [from,lex_from,&vec](It local_from, It local_to)
              {
                pos_type pos
                { static_cast<pos_type>(distance(from,local_from)) };
                while (local_from != local_to)
                  {
                    if (pos % 3 == 0) {
                        vec[pos/3] = trigram_type
                            { pos , *local_from , *(lex_from + (pos/3)) };
                    }
                    ++pos;
                    ++local_from;
                  }
              });

      rlxutil::parallel::tools::wait_for(futs);
      return vec;
    }

    template <typename Pos, typename It>
    void make_suffix_array(It from, It to, unsigned threads = 4)
    {
      using std::numeric_limits;
      using std::is_integral;
      using std::uintmax_t;
      using std::distance;

      using namespace rlx::algotypes;
      typedef typename char_it<It>::type inp;

      static_assert(is_integral<Pos>::value,
          "The position type used for make_suffix_array must be an integral type.");
      if (static_cast<uintmax_t>(numeric_limits<Pos>::max())
          > static_cast<uintmax_t>(distance(from,to)))
          throw std::out_of_range("Attempt to use a position type with make_suffix_array that "
              "is not large enough for the given input string");

      using sux::AlphabetClass;
      using lex       = lexicographical_renaming;
      using recursion = lex::recursion;

      /* Extract 2,3-trigrams. */
      auto trigrams = sux::extract_23trigrams<Pos>(from,to);
      /* Sort them. */
      sux::sort_23trigrams<AlphabetClass::sparse>(trigrams,threads);
      /* Generate an integer alphabet for them according to
       * their sorting order. */
      auto renamed_trigrams = rename_lexicographically(trigrams,threads);

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

          stack<pair<noref<decltype(renamed_string)>,noref<decltype(renamed_trigrams)>>> workpile
              { };

          workpile.emplace(move(renamed_string),move(renamed_trigrams));
          while (!workpile.empty())
            {
              renamed_trigrams = rename_lexicographically(std::get<1>(workpile.top()),threads);
              if (lex::is<recursion::needed>(renamed_trigrams))
                {
                  /* Recursion. */
                  renamed_string = lex::move_newstring_from(renamed_trigrams);
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
