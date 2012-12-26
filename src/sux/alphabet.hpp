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

#include "../util/parallel_vector.hpp"

namespace sux {

  namespace alphabet_tools {
    /**
     * Generate frequency table for the text span [from,to). Use the given
     * extractor to extract a symbol at each iterator position.
     */
    template <typename FreqTable, typename Iterator, typename CharExtractor>
    static FreqTable make_freq_table(Iterator from, Iterator to, CharExtractor extractor)
    {
      FreqTable freq_table {};
      std::for_each(from,to,[&freq_table,&extractor](decltype(*from) &elem) {
        ++freq_table[extractor(elem)];
      });
      return freq_table;
    }
  }

  /** Alphabet types:
   * sparse, a limited number of integers from a wide set of possible integers)
   * zero_range, a range of integers [0,K)
   * range, a range of integers [K1,K2) */
  enum class AlphabetClass { sparse, zero_range, range };

  template <AlphabetClass alphabetclass, typename Char = char>
  struct Alphabet;

  template <typename Char>
  struct Alphabet<AlphabetClass::sparse,Char>
  {
    typedef std::pair<Char,Pos> char_freq_type;
    typedef std::map<Char,Pos>  freq_table_type;

    /**
     * Adding the character frequencies of the second frequency
     * table to those of the first.
     */
    void add_char_freq_table(
        freq_table_type &main_table, const freq_table_type &add_table)
    {
      for (const char_freq_type &entry : add_table)
        main_table[entry.first] += entry.second;
    }
  };

  /**
   * A continuous integer alphabet from (Char)0 to _highest.
   */
  template <typename Char>
  struct Alphabet<AlphabetClass::zero_range,Char>
  {
    static_assert(std::is_integral<Char>::value,
        "A zero-range alphabet must be an integer alphabet");

    Char _highest;

    constexpr Alphabet(const Char highest)
    : _highest(highest)
    { }

    typedef rlxutil::parallel_vector<Char>  freq_table_type;

    /**
     * Adding the character frequencies of the second frequency
     * table to those of the first. If the alphabet is very large,
     * the operation will be parallelised.
     */
    void add_char_freq_table(
        freq_table_type &main_table, const freq_table_type &add_table)
    {
      using std::distance;
      using std::size_t;
      using It = freq_table_type::iterator;

      const It start
      { main_table.begin() };

      main_table.parallel_apply
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
  };

  /**
   * A continuous integer alphabet from _lowest to _highest. Use the
   * from_zero function to transform characters to the range
   * `[0,_highest-_lowest)`.
   */
  template <typename Char>
  struct Alphabet<AlphabetClass::range,Char>
  {
    static_assert(false,"The implementation of this isn't complete.");

    static_assert(std::is_integral<Char>::value,
        "A range alphabet must be an integer alphabet");
    Char _lowest;
    Char _highest;

    constexpr Alphabet(const Char lowest, const Char highest)
    : _lowest(lowest), _highest(highest)
    { }

    constexpr Char from_zero(const Char c)
    { return (c - _lowest); }
  };

}


#endif /* ALPHABET_HPP_ */
