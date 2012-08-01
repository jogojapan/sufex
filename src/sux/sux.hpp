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
#include <iterator>
#include <unordered_map>

namespace sux {

  template<typename T>
  inline T& id(T &t) { return t; }
  template <typename T>
  inline const T& cid(const T& t) { return t; }

  template <typename Char, typename Pos>
  struct SuxBuilder
  {
    typedef std::pair<Char,Pos>            CharFrequency;
    typedef std::unordered_map<Char,Pos>   CharDistribution;

    /**
     * Determine the alphabet of characters used in a sequence, count the number
     * of occurrences of each, and provide accumulated counts. The result is an
     * unordered map from char to accumulated-count. The accumulated count
     * if the sum of the counts of all previous characters, NOT including the
     * present character itself. The order of characters is arbitrary.
     * The datatype used to represent frequency values is Pos.
     *
     * The CharExtractor should be a function type. It is applied to every element of
     * the sequence to extract a character from it.
     */
    template <typename Iterator, typename CharExtractor>
    static CharDistribution accumulated_charcounts(Iterator from, Iterator to, CharExtractor extractor)
    {
      /* Count character frequencies. */
      CharDistribution freq_table {};
      std::for_each(from,to,[&freq_table,&extractor](decltype(*from) &elem) {
        ++freq_table[extractor(elem)];
      });
      /* Generate accumulated distribution. */
      Pos total {};
      for (auto &cf : freq_table) {
        std::swap(total,cf.second);
        total += cf.second;
      }

      return freq_table;
    }

    /**
     * Version of <code>determine_chardistribution()</code> that uses const identity as char-extractor,
     * i.e. it assumes the input sequence is a sequence of characters.
     */
    template <typename Iterator>
    static CharDistribution accumulated_charcounts(Iterator from, Iterator to)
    {
      return accumulated_charcounts(from,to,cid<typename std::iterator_traits<Iterator>::value_type>);
    }

    template <typename Iterator, typename CharExtractor, typename Elem>
    static void bucket_sort(
        Iterator           from,
        Iterator           to,
        CharExtractor      extractor,
        CharDistribution  &bucket_sizes,
        std::vector<Elem> &to_vec)
    {
      to_vec.resize(std::distance(from,to));
      while (from < to)
      {
        to_vec[bucket_sizes[extractor(*from)]++] = *from;
        ++from;
      }
    }

    typedef std::tuple<Pos,Char,Char,Char> Trigram;
    typedef std::vector<Trigram>           Trigrams;

    /**
     * Generate a list of trigrams starting at positions not divisible by 3.
     * Each trigram is represented as a tuple <pos,char1,char2,char3>.
     * Only complete trigrams are included.
     */
    template <typename Iterator>
    static Trigrams make_23trigrams(Iterator from, Iterator to)
    {
      short pos { 0 };
      Trigrams result;
      while (std::distance(from,to) >= 5)
      {
        /* Position 0, skip. */
        ++from; ++pos;
        /* Two trigrams. */
        const Char c1 { *from };
        const Char c2 { *std::next(from,1) };
        const Char c3 { *std::next(from,2) };
        /* Position 1, copy. */
        result.emplace_back(pos,c1,c2,c3);
        ++from; ++pos;
        /* Position 2, copy. */
        const Char c4 { *std::next(from,2) };
        result.emplace_back(pos,c2,c3,c4);
        /* Next iteration. */
        ++from; ++pos;
      }
      if (std::distance(from,to) == 4)
      {
        /* Position 0, skip. */
        ++from; ++pos;
        /* One trigram. */
        const Char c1 { *from };
        const Char c2 { *std::next(from,1) };
        const Char c3 { *std::next(from,2) };
        /* Position 1, copy. */
        result.emplace_back(pos,c1,c2,c3);
        /* Position 2, no complete trigram available. Skip two. */
        from += 2;
        pos  += 2;
      }

      return result;
    }

    template <typename Elem>
    static void sort_23trigrams(std::vector<Elem> &trigrams)
    {
      /* Determine the alphabet and distribution of trigram-final characters. */
      CharDistribution bucket_sizes { determine_chardistribution(from,to,std::get<3>) };
      /* Radix sort, first pass. */
      std::vector<Elem> temp_vec {};
      bucket_sort(
          std::begin(trigrams),std::end(trigrams),std::get<3>,bucket_sizes,temp_vec);
      /* Fresh bucket size calculation. */
    }

  };

}


#endif /* SUX_HPP_ */
