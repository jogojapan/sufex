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
#include "../util/more_algorithm.hpp"
#include "../types/algotypes.hpp"
#include "alphabet.hpp"
#include "trigram.hpp"
#include "../util/parallelization.hpp"
#include "lexicographical_renaming.hpp"
#include "../util/sequence-tools.hpp"

namespace rlxalgo {

  namespace skew {

    template <typename T>     using noref    = typename std::remove_reference<T>::type;
    template <typename... Ts> using sequence = std::vector<Ts...>;

    /**
     * Computes the center point for a given input sequence. It's right
     * in the middle if the length of the sequence is even, and one
     * past the middle if it is odd.a
     */
    template <typename It>
    constexpr std::size_t center_of(It from, It to)
    {
      using std::distance;
      return (distance(from,to) / 2 + (distance(from,to) % 2));
    }

    template <typename Sequence>
    constexpr std::size_t center_of(Sequence seq)
    { return center_of(seq.begin(),seq.end()); }

    template <typename TextIt, typename InpVector>
    lexicographical_renaming::result_type<InpVector> rename_lexicographically(
        const TextIt                &text_from,
        const InpVector             &trigrams,
        rlxutil::parallel::portions &portions,
        std::size_t                  center)
    {
      using namespace rlx::algotypes;
      using namespace sux::trigram_tools;
      typedef ngram_vec<InpVector> vec;

      return lexicographical_renaming::apply(trigrams,portions,
          sux::trigram_tools::content_equal<elemtype<vec>::impl,chtype<vec>,postype<vec>>,
          [center,text_from,&trigrams](postype<vec> index)
                {
                   const postype<vec> pos = pos_of(text_from,trigrams[index]);
                   const postype<vec> mod = pos % 3;
                   const postype<vec> div = pos / 3;
                   return (mod == 1 ? div : center+div);
                });
    }

    template <typename TextIt, typename InpVector>
    lexicographical_renaming::result_type<InpVector> rename_lexicographically(
        const TextIt &text_from, const InpVector &trigrams, std::size_t center, unsigned threads = 4)
    {
      rlxutil::parallel::portions portions
      { trigrams.begin(), trigrams.end(), threads };

      return rename_lexicographically(text_from,trigrams,portions,center);
    }

    template <sux::TGImpl tgimpl, typename Char, typename Pos>
    struct S1TrigramImpl;

    template <typename Char, typename Pos>
    struct S1TrigramImpl<sux::TGImpl::structure,Char,Pos>
    {
      Pos   _pos;        // Position of S1-trigram
      Char  _ch;         // First character
      Pos   _renamed_s1; // Lexicographically renamed trigram that follows

      bool operator==(const S1TrigramImpl &other) const
      { return (_pos == other._pos && _ch == other._ch && _renamed_s1 == other._renamed_s1); }
    };

    template <sux::TGImpl tgimpl, typename It, typename LexIt>
    sequence<S1TrigramImpl<tgimpl,rlxtype::deref<It>,rlxtype::deref<LexIt>>>
    make_s1_trigrams(It from, It to, LexIt lex_from, LexIt lex_to, unsigned threads)
    {
      using namespace rlx::algotypes;
      using std::distance;
      using it    = char_it<It>;
      using lexit = char_it<LexIt>;

      typedef rlxutil::parallel::portions::adjustment                adjustment;
      typedef S1TrigramImpl<tgimpl,chtype<it>,chtype<lexit>>         trigram_type;

      if (distance(lex_from,lex_to) < distance(from,to) / 3)
        throw std::out_of_range("Length of lexicographically renamed string not large enough");

      sequence<trigram_type>
      vec(distance(from,to) / 3 + (distance(from,to) % 3 != 0 ? 1 : 0));

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

    template <typename It>
    void sort_s1_trigrams(It from, It to, rlxutil::parallel::portions &portions)
    {
      using rlx::AlphabetClass;
      using namespace rlx::alphabet_tools;
      using rlx::algotypes::elemtype;

      typedef typename rlx::algotypes::ngram_it<It>::type it;

      /* For the first pass, based on lex array. */
      auto succ_extractor = [](const elemtype<it> &trigram)
      { return trigram._renamed_s1; };

      /* For the second pass, based on the characters themselves. */
      auto s1_extractor = [](const elemtype<it> &trigram)
      { return trigram._ch; };

      /* Temporary container. */
      sequence<elemtype<it>> dest_vec(distance(from,to));

      /* First pass. */ // TODO. Use AlphabetClass::zero_range here. But: make_freq_table isn't ready for that yet.
      rlx::paralgo::bucket_sort<AlphabetClass::sparse>(
          from,to,dest_vec.begin(),succ_extractor,portions);

      /* We re-use the portions object for the second pass, although it
       * is applied to dest_vec this time. This is ok, because portion
       * boundaries do not really matter for bucket sort. */

      /* Second pass. */
      rlx::paralgo::bucket_sort<AlphabetClass::sparse>(
          dest_vec.begin(),dest_vec.end(),from,s1_extractor,portions);
    }

    template <typename It>
    void sort_s1_trigrams(It from, It to, unsigned threads = 4)
    {
      rlxutil::parallel::portions portions
      { from , to , threads };
      sort_s1_trigrams(from,to,portions);
    }

    template <typename It, typename Extractor = rlx::id<rlxtype::deref<It>> >
    struct inserter
    {
      inserter(It it):_it(it),_ex()
      { }
      template <typename Ex>
      inserter(It it, Ex &&ex):_it(it),_ex(ex)
      { }
      inserter &operator*()
      { return *this; }
      inserter &operator++()
      { return *this; }
      inserter &operator++(int)
      { return *this; }
      inserter &operator=(const rlxtype::deref<It> &val)
      { *(_it++) = _ex(val); return *this; }
      inserter &operator=(rlxtype::deref<It> &&val)
      { *(_it++) = _ex(std::move(val)); return *this; }
    private:
      It        _it;  // internal iterator
      Extractor _ex;  // extractor function
    };


    template <typename Pos, typename It>
    sequence<Pos> make_suffix_array(It from, It to, unsigned threads = 4)
    {
      using std::numeric_limits;
      using std::is_integral;
      using std::uintmax_t;
      using std::distance;

      using namespace rlx::algotypes;
      typedef char_it<It> inp;

      static_assert(is_integral<Pos>::value,
          "The position type used for make_suffix_array must be an integral type.");
      if (static_cast<uintmax_t>(numeric_limits<Pos>::max())
          > static_cast<uintmax_t>(distance(from,to)))
          throw std::out_of_range("Attempt to use a position type with make_suffix_array that "
              "is not large enough for the given input string");

      using rlx::AlphabetClass;
      using rlx::Alphabet;
      using lex       = lexicographical_renaming;
      using recursion = lex::recursion;

      /* Extract 2,3-trigrams. */
      auto trigrams = sux::extract_23trigrams<Pos>(from,to);
      /* Sort them. */
      Alphabet<AlphabetClass::sparse,chtype<inp>,Pos> alphabet
      { };
      sux::sort_23trigrams(trigrams,alphabet,threads);
      /* Generate an integer alphabet for them according to
       * their sorting order. */
      auto new_names = rename_lexicographically(
          from,trigrams,center_of(from,to),threads);

      using std::size_t;
      using std::get;
      using std::stack;
      using std::pair;
      using std::move;
      using sux::triget1;
      using sux::triget2;
      using sux::triget3;
      using rlxtype::itertype;
      using rlx::seqtools::size;
      using namespace sux::trigram_tools;

      /* We'll move the end result into this object: */
      sequence<Pos> result;

      if (lex::is<recursion::needed>(new_names))
        {
          auto renamed_string =
              lex::move_newstring_from(new_names);
          auto alphabet_size  =
              lex::alphsize(new_names);
          auto renamed_trigrams =
              sux::extract_23trigrams<Pos>(renamed_string.begin(),renamed_string.end());

          Alphabet<AlphabetClass::zero_range,Pos,Pos> alphabet
          { alphabet_size };

          sux::sort_23trigrams(renamed_trigrams,alphabet,threads);

          stack<pair<noref<decltype(renamed_string)>,noref<decltype(renamed_trigrams)>>>
          workpile
          { };

          workpile.emplace(move(renamed_string),move(renamed_trigrams));
          while (!workpile.empty())
            {
              auto &rec_text            = get<0>(workpile.top());
              auto center               = center_of(rec_text);
              auto &rec_sorted_trigrams = get<1>(workpile.top());
              auto rec_renamed = rename_lexicographically(
                  begin(rec_text),rec_sorted_trigrams,center,threads);
              if (lex::is<recursion::needed>(rec_renamed))
                {
                  /* Recursion. */
                  auto new_rec_text = lex::move_newstring_from(rec_renamed);
                  auto rec_alphsize = lex::alphsize(rec_renamed);
                  auto rec_trigrams =
                      sux::extract_23trigrams<Pos>(new_rec_text.begin(),new_rec_text.end());
                  Alphabet<AlphabetClass::zero_range,Pos,Pos> rec_alphabet
                  { rec_alphsize };
                  sux::sort_23trigrams(rec_trigrams,rec_alphabet,threads);
                  workpile.emplace(move(new_rec_text),move(rec_trigrams));
                }
              else
                {
                  auto rec_new_names  = lex::move_newstring_from(rec_renamed);
                  /* End of recursion. Wind up the workpile. */
                  for (;;)
                    {
                      auto s1 =
                          make_s1_trigrams<sux::TGImpl::structure>(
                              begin(rec_text),end(rec_text),begin(rec_new_names),end(rec_new_names),threads);
                      /* Sort S1. */
                      sort_s1_trigrams(begin(s1),end(s1),threads);
                      /* Merge S1 and S12 into the suffix array for rec_text. */
                      sequence<Pos> sux_array
                      ((size_t)distance(begin(rec_text),end(rec_text)));

                      auto rec_s23 = move(get<1>(workpile.top()));
                      typedef rlxtype::elemtype<decltype(rec_s23)> s23_elem_type;
                      typedef rlxtype::elemtype<decltype(s1)>      s1_elem_type;
                      /* Center of the renamed lexicographical names array. */
                      const Pos center = rec_new_names.size() / 2 + (rec_new_names.size() % 2 == 1 ? 1 : 0);
                      /* Merge the S1 and the S23 array. */
                      /*  TODO. CONTINUE HERE. Use an 'inserter' instead of
                       * begin(sux_array). */
                      rlx::seqalgo::merge_sorted(
                          begin(rec_s23),end(rec_s23),begin(s1),end(s1),begin(sux_array),
                          [center,&rec_new_names,&rec_text]
                           (const s23_elem_type &lhs, const s1_elem_type &rhs)
                          {
                            if (triget1(lhs) < rhs._ch)
                              return true;
                            if (triget1(lhs) == rhs._ch) {
                              if (pos_of(lhs) % 3 == 1)
                                {
                                  Pos lhs_lex = rec_new_names[center + pos_of(lhs)/3];
                                  Pos rhs_lex = rhs._renamed_s1;
                                  return (lhs_lex < rhs_lex);
                                }
                              else
                                {
                                  Pos lhs_ch = triget2(lhs);
                                  Pos rhs_ch = rec_text[rhs._pos + 1];
                                  if (lhs_ch < rhs_ch)
                                    return true;
                                  Pos lhs_lex = rec_new_names[pos_of(lhs)/3 + 1];
                                  Pos rhs_lex = rec_new_names[center + rhs._pos/3];
                                  return (lhs_lex < rhs_lex);
                                }
                            }
                          });

                      /* Take the current work item off the pile; it's done. */
                      workpile.pop();

                      if (!workpile.empty())
                        {
                          /* We re-use the sequence `rec_lex` to store the inversion
                           * of the suffix array we've just computed. */
                          rec_new_names.resize(size(sux_array));

                          rlxutil::parallel::portions portions
                          { begin(sux_array) , end(sux_array) , threads };

                          typedef itertype<decltype(sux_array)> suxit;
                          portions.apply(begin(sux_array),end(sux_array),
                              [&rec_new_names](suxit suxfrom, suxit suxto, suxit suxbeg)
                              {
                                while (suxfrom != suxto) {
                                  rec_new_names[*suxfrom] = distance(suxbeg,suxfrom);
                                  ++suxfrom;
                                }
                              },begin(sux_array));
                        }
                      else
                        {
                          result = move(sux_array);
                          break;
                        }
                    }
                }
            }
        }
      else
        {
          /* This is the case when the very first pass of trigram sorting gave
           * us a perfect suffix array for S23 with no duplicate trigrams. */

          /* Use the lexicographically renamed string as inverse of S23. */
          auto inv_sux = lex::move_newstring_from(new_names);
          /* We make an S1 string. */
          auto s1 =
              make_s1_trigrams<sux::TGImpl::structure>(
                  from,to,begin(inv_sux),end(inv_sux),threads);
          /* Sort S1. */
          sort_s1_trigrams(begin(s1),end(s1),threads);
          /* Merge S1 and S12 into the suffix array for rec_text. */
          sequence<Pos> sux_array
          ((size_t)distance(from,to));

          typedef rlxtype::elemtype<decltype(trigrams)> s23_elem_type;
          typedef rlxtype::elemtype<decltype(s1)>       s1_elem_type;

          /* Center of the renamed lexicographical names array. */
          const Pos center = inv_sux.size() / 2 + (inv_sux.size() % 2 == 1 ? 1 : 0);
          /* Merge the S1 and the S23 array. */
          rlx::seqalgo::merge_sorted(
              begin(trigrams),end(trigrams),begin(s1),end(s1),begin(sux_array),
              [center,&inv_sux,&trigrams](const s23_elem_type &lhs, const s1_elem_type &rhs)
              {
                if (triget1(lhs) < rhs._ch)
                  return true;
                if (triget1(lhs) == rhs._ch) {
                  if (pos_of(lhs) % 3 == 1)
                    {
                      Pos lhs_lex = inv_sux[center + pos_of(lhs)/3];
                      Pos rhs_lex = rhs._renamed_s1;
                      return (lhs_lex < rhs_lex);
                    }
                  else
                    {
                      Pos lhs_ch = triget2(lhs);
                      Pos rhs_ch = trigrams[rhs._pos + 1];
                      if (lhs_ch < rhs_ch)
                        return true;
                      Pos lhs_lex = inv_sux[pos_of(lhs)/3 + 1];
                      Pos rhs_lex = inv_sux[center + rhs._pos/3];
                      return (lhs_lex < rhs_lex);
                    }
                }
              });

          result = move(sux_array);
        }

      return result;
    }


  } // skew

} // rlxalgo


#endif /* SKEW_HPP_ */
