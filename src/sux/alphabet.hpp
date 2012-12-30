/*
 * alphabet.hpp
 *
 *  Created on: 2012/12/22
 *      Author: gollerjo
 */

#ifndef ALPHABET_HPP_
#define ALPHABET_HPP_

#include <type_traits>
#include <iterator>

#include "../util/parallelization.hpp"

namespace sux {

  /** Alphabet types:
   * sparse, a limited number of integers from a wide set of possible integers)
   * zero_range, a range of integers [0,K) */
  enum class AlphabetClass { sparse, zero_range };

  template <AlphabetClass alphabetclass, typename Char = char, typename Freq = std::size_t>
  struct Alphabet;

  namespace alphabet_tools {
    /**
     * Generate frequency table for the text span [from,to). Use the given
     * extractor to extract a symbol at each iterator position.
     */
    template <typename FreqTable, typename Iterator, typename CharExtractor>
    static FreqTable make_freq_table(Iterator from, Iterator to, CharExtractor extractor)
    {
      FreqTable freq_table
      { };
      std::for_each(from,to,[&freq_table,&extractor](decltype(*from) &elem) {
        ++freq_table[extractor(elem)];
      });
      return freq_table;
    }
  }

  template <typename Char, typename Freq>
  struct Alphabet<AlphabetClass::sparse,Char,Freq>
  {
    typedef Char                 char_type;
    typedef Freq                 freq_type;
    typedef std::pair<Char,Freq> char_freq_type;
    typedef std::map<Char,Freq>  freq_table_type;

    /**
     * Adding the character frequencies of the second frequency
     * table to those of the first.
     */
    static void add_char_freq_table(
        freq_table_type &main_table, const freq_table_type &add_table)
    {
      for (const char_freq_type &entry : add_table)
        main_table[entry.first] += entry.second;
    }

    /**
     * Turn a character frequency table into a cumulative character
     * frequency table, i.e. the frequency of each character
     * being the sum all previous character frequencies added
     * to its own frequency.
     */
    static void make_cumulative(freq_table_type &freq_table)
    {
      freq_type total
      { 0 };
      for (auto &entry : freq_table) {
        std::swap(total,entry.second);
        total += entry.second;
      }
    }
  };

  /**
   * A continuous integer alphabet from (Char)0 to _highest.
   */
  template <typename Char, typename Freq>
  struct Alphabet<AlphabetClass::zero_range,Char,Freq>
  {
    static_assert(std::is_integral<Char>::value,
        "A zero-range alphabet must be an integer alphabet");

    Char _highest;

    constexpr Alphabet(const Char highest)
    : _highest(highest)
    { }

    typedef Char               char_type;
    typedef Freq               freq_type;
    typedef std::vector<Freq>  freq_table_type;

    /**
     * Adding the character frequencies of the second frequency
     * table to those of the first. If the alphabet is very large,
     * the operation will be parallelised.
     */
    static void add_char_freq_table(
        freq_table_type &main_table, const freq_table_type &add_table, unsigned threads = 4)
    {
      using std::distance;
      using std::size_t;
      using It = typename freq_table_type::iterator;

      rlxutil::parallel::portions portions
      { begin(main_table), end(main_table), threads };
      const It start
      { main_table.begin() };

      portions.apply
      ([start,&add_table](It from, It to)
      {
        It src_it
        { add_table.begin() + distance(start,from) };

        while (from != to)
          {
            *from += *src_it;
            ++from;
            ++src_it;
          }
      });
    }

    /**
     * Turn a character frequency table into a cumulative character
     * frequency table, i.e. the frequency of each character
     * being the sum all previous character frequencies added
     * together.
     */
    static void make_cumulative(freq_table_type &freq_table)
    {
      freq_type total
      { 0 };
      for (freq_type &freq : freq_table) {
        std::swap(total,freq);
        total += freq;
      }
    }
  };

}


#endif /* ALPHABET_HPP_ */
