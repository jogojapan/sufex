/*
 * trigram.hpp
 *
 *  Created on: 2012/07/31
 *      Author: jogojapan
 */

#ifndef TRIGRAM_HPP_
#define TRIGRAM_HPP_

#include <utility>
#include <tuple>
#include <array>
#include <vector>
#include <algorithm>
#include <iterator>
#include <map>
#include <future>
#include <cstddef>
#include <iostream>
#include <ios>
#include <thread>
#include <chrono>

#include "alphabet.hpp"
#include "../util/parallelization.hpp"
#include "../util/more_algorithm.hpp"

namespace sux {

  /** Identity function. Used as auxiliary. */
  template<typename T>
  inline T&        id(T &t) { return t; }
  template <typename T>
  inline const T& cid(const T& t) { return t; }

  /** Implementation of trigrams. */
  enum class TGImpl { tuple, arraytuple, structure, pointer };

  /** String representation of trigram implementation types. */
  template <TGImpl tgimpl> struct repr;
  template <> struct repr<TGImpl::tuple>      { static constexpr const char *str = "tuple"; };
  template <> struct repr<TGImpl::arraytuple> { static constexpr const char *str = "arraytuple"; };
  template <> struct repr<TGImpl::structure>  { static constexpr const char *str = "structure"; };
  template <> struct repr<TGImpl::pointer>    { static constexpr const char *str = "pointer"; };

  template <TGImpl tgimpl, typename Char, typename Pos>
  struct TrigramImpl;

  /** General implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl<TGImpl::tuple,Char,Pos> : public std::tuple<Pos,Char,Char,Char> {
    typedef std::tuple<Pos,Char,Char,Char> base_type;
    typedef Char                           char_type;
    typedef Pos                            pos_type;
    typedef std::vector<TrigramImpl>       vec_type;
    constexpr static TGImpl                impl = TGImpl::tuple;
  
    TrigramImpl():base_type() {}

    TrigramImpl(
        const Pos pos,
        const Char c1,
        const Char c2,
        const Char c3)
    :base_type(pos,c1,c2,c3)
    {}

    pos_type  pos() const  { return std::get<0>(*this); }
    char_type get1() const { return std::get<1>(*this); }
    char_type get2() const { return std::get<2>(*this); }
    char_type get3() const { return std::get<3>(*this); }

    bool content_equal(const TrigramImpl &other) const
    { return ((get1() == other.get1()) && (get2() == other.get2() && (get3() == other.get3()))); }
  };
  
  /** Array-based implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl<TGImpl::arraytuple,Char,Pos> : public std::tuple<Pos,std::array<Char,3>>
  {
    typedef std::tuple<Pos,std::array<Char,3>> base_type;
    typedef Char                               char_type;
    typedef Pos                                pos_type;
    typedef std::vector<TrigramImpl>           vec_type;
    constexpr static TGImpl                    impl = TGImpl::arraytuple;

    TrigramImpl():base_type() {}

    TrigramImpl(
        const Pos pos,
        const Char c1,
        const Char c2,
        const Char c3)
    :base_type(pos,{{c1,c2,c3}})
    {}

    pos_type  pos() const  { return std::get<0>(*this); }
    char_type get1() const { return std::get<1>(*this)[0]; }
    char_type get2() const { return std::get<1>(*this)[1]; }
    char_type get3() const { return std::get<1>(*this)[2]; }

    bool content_equal(const TrigramImpl &other) const
    { return (std::get<1>(*this) == std::get<1>(other)); }
  };

  /** Pointer-based implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl<TGImpl::pointer,Char,Pos>
  {
    typedef Char *                   base_type;
    typedef Char                     char_type;
    typedef Pos                      pos_type;
    typedef std::vector<TrigramImpl> vec_type;
    constexpr static TGImpl          impl = TGImpl::pointer;

    const Char *_p;

    TrigramImpl()
    : _p()
    { }

    TrigramImpl(
        const Char *cp)
    : _p(cp)
    { }

    char_type get1() const { return *_p; }
    char_type get2() const { return *(_p+1); }
    char_type get3() const { return *(_p+2); }

    bool operator==(const TrigramImpl &other)
    { return content_equal(other); }

    bool content_equal(const TrigramImpl &other) const
    { return ((get1() == other.get1()) && (get2() == other.get2() && (get3() == other.get3()))); }
  };

  namespace trigram_tools {

    /**
     * Compare the character strings of two trigrams.
     */
    template <TGImpl tgimpl, typename Char, typename Pos>
    bool content_equal(
        const TrigramImpl<tgimpl,Char,Pos> &tg1,
        const TrigramImpl<tgimpl,Char,Pos> &tg2)
    { return tg1.content_equal(tg2); }

    /**
     * Return a string representation of the implementation class
     * of a trigram.
     */
    template <TGImpl tgimpl, typename Char, typename Pos>
    const char *impl_of(const TrigramImpl<tgimpl,Char,Pos> &)
    { return repr<tgimpl>::str; }

    template <TGImpl tgimpl, typename Char, typename Pos>
    struct posmapper;

    template <typename Char, typename Pos>
    struct posmapper<TGImpl::tuple,Char,Pos>
    {
      static Pos pos_of(const TrigramImpl<TGImpl::tuple,Char,Pos> &trigram)
      { return trigram.pos(); }

      template <typename It>
      static Pos pos_of(It, const TrigramImpl<TGImpl::tuple,Char,Pos> &trigram)
      { return trigram.pos(); }
    };

    template <typename Char, typename Pos>
    struct posmapper<TGImpl::arraytuple,Char,Pos>
    {
      static Pos pos_of(const TrigramImpl<TGImpl::arraytuple,Char,Pos> &trigram)
      { return trigram.pos(); }

      template <typename It>
      static Pos pos_of(It, const TrigramImpl<TGImpl::arraytuple,Char,Pos> &trigram)
      { return trigram.pos(); }
    };

    template <typename Char, typename Pos>
    struct posmapper<TGImpl::pointer,Char,Pos>
    {
      template <typename It>
      static Pos pos_of(It start, const TrigramImpl<TGImpl::pointer,Char,Pos> &trigram)
      { return trigram._p - &*start; }
    };

    /**
     * Maps a trigram to the text position it refers to. This does not
     * work for `TGImpl::pointer` trigrams.
     */
    template <TGImpl tgimpl, typename Char, typename Pos>
    Pos pos_of(const TrigramImpl<tgimpl,Char,Pos> &trigram)
    { return posmapper<tgimpl,Char,Pos>::pos_of(trigram); }

    /**
     * Maps a trigram to the text position it refers to. This works
     * for all trigram types, but requires that the starting point
     * of the reference text is given as iterator.
     */
    template <typename It, TGImpl tgimpl, typename Char, typename Pos>
    Pos pos_of(It it, const TrigramImpl<tgimpl,Char,Pos> &trigram)
    { return posmapper<tgimpl,Char,Pos>::pos_of(it,trigram); }

    template <TGImpl tgimpl, typename Char, typename Pos>
    std::basic_string<Char> to_str(const TrigramImpl<tgimpl,Char,Pos> &trigram)
    {
      std::basic_string<Char> result
      { triget1(trigram) , triget2(trigram) , triget3(trigram) };
      return result;
    }

  }

  /**
   * Return the first character of a trigram, regardless of the implementation
   * of the trigram.
   */
  template <typename TrigramT>
  static typename TrigramT::char_type triget1(const TrigramT &tri) { return tri.get1(); }
  /**
   * Return the second character of a trigram, regardless of the implementation
   * of the trigram.
   */
  template <typename TrigramT>
  static typename TrigramT::char_type triget2(const TrigramT &tri) { return tri.get2(); }
  /**
   * Return the third character of a trigram, regardless of the implementation
   * of the trigram.
   */
  template <typename TrigramT>
  static typename TrigramT::char_type triget3(const TrigramT &tri) { return tri.get3(); }

  /** The container type for trigram lists. */
  template <typename TrigramT> struct TrigramContainer
  { typedef std::vector<TrigramT> vec_type; };

  template <TGImpl tgimpl, typename Char, typename Pos>
  struct TrigramMaker
  {
    typedef TrigramImpl<tgimpl,Char,Pos>                      trigram_type;
    typedef typename TrigramContainer<trigram_type>::vec_type trigram_vec_type;

    /**
     * Generate a list of trigrams starting at positions not divisible by 3.
     * Each trigram is represented as a tuple <pos,char1,char2,char3>.
     * Only complete trigrams are included.
     */
    template <typename Iterator>
    static trigram_vec_type make_23trigrams(Iterator from, Iterator to)
    {
      short pos { 0 };
      trigram_vec_type result;
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

  };

  template <typename Char, typename Pos>
  struct TrigramMaker<TGImpl::pointer,Char,Pos>
  {
    typedef TrigramImpl<TGImpl::pointer,Char,Pos>             trigram_type;
    typedef typename TrigramContainer<trigram_type>::vec_type trigram_vec_type;

    /**
     * Generate a list of trigrams starting at positions not divisible by 3.
     * Each trigram is represented as a tuple <pos,char1,char2,char3>.
     * Only complete trigrams are included.
     */
    template <typename Iterator>
    static trigram_vec_type make_23trigrams(Iterator from, Iterator to)
    {
      trigram_vec_type result;
      while (std::distance(from,to) >= 5)
      {
        /* Position 0, skip. */
        ++from;
        /* Position 1, copy. */
        result.emplace_back(&*from);
        ++from;
        /* Position 2, copy. */
        result.emplace_back(&*from);
        /* Next iteration. */
        ++from;
      }
      if (std::distance(from,to) == 4)
      {
        /* Position 0, skip. */
        ++from;
        /* Position 1, copy. */
        result.emplace_back(&*from);
        /* Position 2, no complete trigram available. Skip two. */
        from += 2;
      }

      return result;
    }

  };

  /**
   * Lexicographic sorting of trigrams, using one or multiple threads.
   */
  template <typename Char, typename Pos>
  struct TrigramSorter
  {
    typedef std::pair<Char,Pos> CharFrequency;
    typedef std::map<Char,Pos>  CharDistribution;

    /**
     * Perform a bucket-sort of the elements in the range [from,to), using the
     * given extractor to determine the sorting criterion. The result will
     * be sorted into to_vec. to_vec must be resize()'ed by the caller
     * before the call. bucket_sizes must be prepared by the caller so it
     * provides the size of each bucket.
     */
    template <typename Iterator, typename CharExtractor>
    static void bucket_sort(
        Iterator           from,
        Iterator           to,
        CharExtractor      extractor,
        CharDistribution  &bucket_sizes,
        Iterator           dest)
    {
      while (from != to)
      {
        *(dest + bucket_sizes[extractor(*from)]++) = *from;
        ++from;
      }
    }

    template<rlx::AlphabetClass alphabetclass = rlx::AlphabetClass::sparse>
    struct AlphabetSpecific;

  };

  template <typename Char, typename Pos>
  template <rlx::AlphabetClass alphaclass>
  struct TrigramSorter<Char,Pos>::AlphabetSpecific
  {
    typedef TrigramSorter<Char,Pos>                   base;
    typedef rlx::Alphabet<alphaclass,Char,Pos>        alphabet_type;
    typedef typename alphabet_type::freq_table_type   freq_table_type;

    /**
     * Perform one pass of radix sort for trigrams, using the specified number
     * of parallel threads.
     */
    template <typename It, typename Extractor>
    static void parallel_bucket_sort
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

      typedef deref<It> elem_type;

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
          (from,to,base::bucket_sort<It,Extractor>,
           arg_generator(
               [&cumul_frqtab_vec,dest,&extractor](int thread)
               { return make_tuple(extractor,ref(cumul_frqtab_vec[thread]),dest); })
          );

      wait_for(sort_fut_vec);
    }

    template <typename TrigramType>
    static void sort_23trigrams(std::vector<TrigramType> &trigrams, unsigned num_threads)
    {
      /* Portions for threads. */
      rlxutil::parallel::portions portions
      { begin(trigrams), end(trigrams), num_threads };
      /* Vector for intermediate results. */
      std::vector<TrigramType> temp_vec(trigrams.size());
      /* First pass. */
      parallel_bucket_sort(begin(trigrams),end(trigrams),begin(temp_vec),triget3<TrigramType>,portions);
      swap(trigrams,temp_vec);
      /* Second pass. */
      parallel_bucket_sort(begin(trigrams),end(trigrams),begin(temp_vec),triget2<TrigramType>,portions);
      swap(trigrams,temp_vec);
      /* Third pass. */
      parallel_bucket_sort(begin(trigrams),end(trigrams),begin(temp_vec),triget1<TrigramType>,portions);
      swap(trigrams,temp_vec);
    }
  };

  /**
   * Extract the 2,3-trigrams from a `std::basic_string`.
   */
  template <TGImpl tgimpl = TGImpl::arraytuple, typename Char = char>
  typename TrigramMaker<tgimpl,Char,typename std::basic_string<Char>::size_type>::trigram_vec_type
  string_to_23trigrams(
      const std::basic_string<Char> &str)
  {
    return TrigramMaker<tgimpl,Char,typename std::basic_string<Char>::size_type>::make_23trigrams(
        begin(str),end(str));
  }

  /**
   * Extract the 2,3-trigrams from a character sequence represented by two
   * input iterators.
   */
  template <typename Pos, TGImpl tgimpl = TGImpl::arraytuple, typename It>
  typename TrigramMaker<tgimpl,rlxtype::deref<It>,Pos>::trigram_vec_type
  extract_23trigrams(It from, It to)
  {
    typedef rlxtype::deref<It> char_type;
    return TrigramMaker<tgimpl,char_type,Pos>::make_23trigrams(from,to);
  }

  /**
   * Assuming that the first argument is a random-access container
   * containing the 2,3-trigrams of a string, this sorts them
   * lexicographically, using three passes of multi-threaded
   * radix-sort.
   *
   * Note that we did not provide a (from,to)-style iterator-
   * based interface for this, because internally, the sorting
   * procedure will create another container of the same size
   * and same type as the given one. This wouldn't be possible if
   * only iterators were available.
   */
  template <rlx::AlphabetClass alphaclass, typename Vector>
  void sort_23trigrams(Vector &trigrams, unsigned num_threads)
  {
    typedef Vector vec_type;
    typedef typename vec_type::value_type     trigram_type;
    typedef typename trigram_type::char_type  char_type;
    typedef typename trigram_type::pos_type   pos_type;

    TrigramSorter<char_type,pos_type>::template AlphabetSpecific<alphaclass>::sort_23trigrams(
        trigrams,num_threads);
  }

}

#endif /* SUX_HPP_ */
