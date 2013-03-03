/*
 * more_algorithm.hpp
 *
 *  Created on: 2013/01/01
 *      Author: jogojapan
 */

#ifndef MORE_ALGORITHM_HPP_
#define MORE_ALGORITHM_HPP_

#include "more_type_traits.hpp"

namespace rlxutil {
  namespace algorithm {

    template <typename It>
    rlxtype::deref<It> make_cumulative(It from, It to)
    {
      using std::swap;
      typedef rlxtype::deref<It> elem_type;

      elem_type total
      { };

      while (from != to)
        {
          total += *from;
          *from = total;
          ++from;
        }

      return total;
    }



  }
}

namespace rlx {

  /** Identity function. Used as auxiliary. */
  template<typename T>
  inline T&&        id(T &&t) { return t; }
  template <typename T>
  inline const T&& cid(const T&& t) { return t; }

  /**
   * Sequential algorithms, i.e. common algorithms performed without the
   * use of parallelization (in other words, single-threaded algorithms).
   */
  namespace seqalgo {

    /**
     * Perform a bucket-sort of the elements in the range [from,to), using the
     * given extractor to determine the sorting criterion. The result will
     * be sorted into a random-access container starting at `dest_from`. It is
     * the caller's responsibility to ensure there is enough space in that container.
     * `bucket_sizes` is a cumulative frequency table of the value of
     * `extractor(item)` for each `item` in the range [from,to). It must be prepared
     * by the caller as well.
     */
    template <typename It, typename DestIt, typename Extractor, typename CumulFreqTable>
    void bucket_sort(
        It from, It to, DestIt dest_from, Extractor extractor, CumulFreqTable &bucket_sizes)
    {
      while (from != to)
      {
        *(dest_from + bucket_sizes[extractor(*from)]++) = *from;
        ++from;
      }
    }

    template <typename Compare, typename It1, typename It2>
    constexpr bool is_comparator()
    {
      using std::decay;
      using std::is_same;
      using std::is_integral;
      using rlxtype::deref;
      typedef rlxtype::function_traits<Compare> traits;
      typedef typename traits::result_type      result;
      typedef typename traits::template arg<0>::type     arg0;
      typedef typename traits::template arg<1>::type     arg1;

      return (traits::arity == 2
          && is_integral<result>::value
          && is_same<decay<arg0>,decay<deref<It1>>>::value
          && is_same<decay<arg1>,decay<deref<It2>>>::value);
    }

    template <typename It1, typename It2, typename DestIt, typename Compare>
    void merge_sorted(
        It1 from1, It1 to1, It2 from2, It2 to2, DestIt dest_from, Compare &&compare)
    {
      static_assert(is_comparator<Compare,It1,It2>,
          "Attempt to use merge_sorted() with an invalid comparator function");

      std::size_t pos
      { 0 };

      while (from1 != to1 && from2 != to2)
        {
          if (compare(*from1,*from2))
              *(dest_from + pos++) = *(from1++);
          else
            *(dest_from + pos++) = *(from2++);
        }
      while (from1 != to1)
        *(dest_from + pos++) = *(from1++);
      while (from2 != to2)
        *(dest_from + pos++) = *(from2++);
    }

  }

  /**
   * Parallelized algorithms, i.e. algorithms that take an rlx::parallel::portions
   * object as additional argument and perform one parallel thread for each
   * portion of the input data.
   */
  namespace paralgo {

    /**
     * Perform one pass of radix sort for trigrams, using the specified number
     * of parallel threads.
     */
    template <AlphabetClass alphaclass, typename Pos = std::size_t, typename It = char*, typename Extractor = char(char*)>
    static void bucket_sort
    (It from, It to, It dest, Extractor extractor,
        const rlxutil::parallel::portions &portions)
    {
      using std::ref;
      using std::make_tuple;
      using rlxtype::deref;
      using rlxtype::is_compatible;
      using namespace rlx::alphabet_tools;
      using rlxutil::parallel::tools::arg_generator;
      using rlxutil::parallel::tools::wait_for;
      using rlxtype::function_traits;

      typedef deref<It>                                          elem_type;
      typedef typename function_traits<Extractor>::result_type   char_type;
      typedef Alphabet<alphaclass,char_type,Pos>                 alphabet_type;
      typedef typename alphabet_type::freq_table_type            freq_table_type;

      /* Create a frequency list of characters. */
      auto frqtab_vec = portions.apply
          (from,to,make_freq_table<freq_table_type,It,Extractor>,extractor);

      /* Initialise cumulative frequencies per thread. This will
       * be filled with the correct values later. */
      std::vector<freq_table_type> cumul_frqtab_vec
      { };

      /* Total frequency, for each character. */
      freq_table_type cumul_frqtab
      { };
      for (auto &frqtab_fut : frqtab_vec)
      {
        /* Move the thread-local frequency table out
         * of its future. */
        freq_table_type frqtab
        { frqtab_fut.get() };
        /* Add its character frequencies to the total table. */
        alphabet_type::add_char_freq_table(cumul_frqtab,frqtab,portions.num());
        /* Move thread-local frequency table to the end of the
         * thread-local cumulative frequency table list. Note
         * that the frequencies are not really cumulative yet;
         * this will be corrected later. */
        cumul_frqtab_vec.push_back(std::move(frqtab));
      }

      /* Cumulate the entries of the global frequency table. */
      alphabet_type::make_cumulative(cumul_frqtab);

      /* Correct the cumulative thread-local frequencies. */
      for (freq_table_type &frqtab : cumul_frqtab_vec) {
        /* Swap with current version of global cumulative
         * frequency table. */
        std::swap(cumul_frqtab,frqtab);
        alphabet_type::add_char_freq_table(cumul_frqtab,frqtab,portions.num());
      }

      /* Radix-sorting threads. */
      auto sort_fut_vec = portions.apply_dynargs
          (from,to,seqalgo::bucket_sort<It,It,Extractor,freq_table_type>,
           arg_generator(
               [&cumul_frqtab_vec,dest,&extractor](int thread)
           { return make_tuple(dest,extractor,ref(cumul_frqtab_vec[thread])); })
          );

      wait_for(sort_fut_vec);
    }

  } // paralgo

} // rlx


#endif /* MORE_ALGORITHM_HPP_ */
