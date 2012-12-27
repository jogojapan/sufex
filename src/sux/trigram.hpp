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

#include <iostream>
#include <thread>
#include <chrono>

#include "alphabet.hpp"
#include "../util/parallel_vector.hpp"

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
  template <> struct repr<TGImpl::tuple>      { static constexpr const char *const str = "tuple"; };
  template <> struct repr<TGImpl::arraytuple> { static constexpr const char *const str = "arraytuple"; };
  template <> struct repr<TGImpl::structure>  { static constexpr const char *const str = "structure"; };
  template <> struct repr<TGImpl::pointer>    { static constexpr const char *const str = "pointer"; };

  template <TGImpl tgimpl, typename Char, typename Pos>
  struct TrigramImpl;

  /** General implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl<TGImpl::tuple,Char,Pos> : public std::tuple<Pos,Char,Char,Char> {
    typedef std::tuple<Pos,Char,Char,Char> base_type;
    typedef Char                           char_type;
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

    Char get1() const { return std::get<1>(*this); }
    Char get2() const { return std::get<2>(*this); }
    Char get3() const { return std::get<3>(*this); }
  };
  
  /** Array-based implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl<TGImpl::arraytuple,Char,Pos> : public std::tuple<Pos,std::array<Char,3>>
  {
    typedef std::tuple<Pos,std::array<Char,3>> base_type;
    typedef Char                               char_type;
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

    char_type get1() const { return std::get<1>(*this)[0]; }
    char_type get2() const { return std::get<1>(*this)[1]; }
    char_type get3() const { return std::get<1>(*this)[2]; }
  };

  /** Pointer-based implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl<TGImpl::pointer,Char,Pos>
  {
    typedef Char *                   base_type;
    typedef Char                     char_type;
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

    bool operator==(const TrigramImpl &other) const
    { return (
        (get1() == other.get1()) &&
        (get2() == other.get2()) &&
        (get3() == other.get3())); }
  };

  template <TGImpl tgimpl, typename Char, typename Pos>
  const char *tgimpl_of(const TrigramImpl<tgimpl,Char,Pos> &)
  { return repr<tgimpl>::str; }

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
  template <typename TrigramT> struct TrigramContainer { typedef std::vector<TrigramT> vec_type; };

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
     * Generate frequency table for the text [from,to).
     */
    template <typename Iterator, typename CharExtractor, typename Mapping = CharDistribution>
    static Mapping generate_freq_table(Iterator from, Iterator to, CharExtractor extractor)
    {
      Mapping freq_table {};
      std::for_each(from,to,[&freq_table,&extractor](decltype(*from) &elem) {
        ++freq_table[extractor(elem)];
      });
      return freq_table;
    }

    /**
     * Transform a character frequency table into an accumulated
     * frequency table, in-place.
     *
     * The Mapping data type may be any data type that provides a
     * default iterator that dereferences to CharFrequency.
     */
    template <typename Mapping>
    static void accumulate_frequencies(Mapping &freq_table)
    {
      Pos total {};
      for (auto &cf : freq_table) {
        std::swap(total,cf.second);
        total += cf.second;
      }
    }

    /**
     * Determine the alphabet of characters used in a sequence, count the number
     * of occurrences of each, and provide accumulated counts. The result is an
     * ordered map from char to accumulated-count. The accumulated count
     * if the sum of the counts of all previous characters, NOT including the
     * present character itself. The order of characters is the one defined by the
     * ordering of the map (std::less<Char> by default).
     * The datatype used to represent frequency values is Pos.
     *
     * The CharExtractor should be a function type. It is applied to every element of
     * the sequence to extract a character from it.
     */
    template <typename Iterator, typename CharExtractor>
    static CharDistribution accumulated_charcounts(Iterator from, Iterator to, CharExtractor extractor)
    {
      /* Count character frequencies. */
      CharDistribution freq_table { generate_freq_table(from,to,extractor) };
      /* Generate accumulated distribution. */
      accumulate_frequencies(freq_table);
      /* Return results. */
      return freq_table;
    }

    /**
     * Version of <code>determine_chardistribution()</code> that uses const identity as
     * char-extractor, i.e. it assumes the input sequence is a sequence of characters.
     */
    template <typename Iterator>
    static CharDistribution accumulated_charcounts(Iterator from, Iterator to)
    {
      return accumulated_charcounts(from,to,cid<typename std::iterator_traits<Iterator>::value_type>);
    }

    /**
     * Perform a bucket-sort of the elements in the range [from,to), using the
     * given extractor to determine the sorting criterion. The result will
     * be sorted into to_vec. to_vec must be resize()'ed by the caller
     * before the call. bucket_sizes must be prepared by the caller so it
     * provides the size of each bucket.
     */
    template <typename Iterator, typename CharExtractor, typename Elem>
    static void bucket_sort(
        Iterator           from,
        Iterator           to,
        CharExtractor      extractor,
        CharDistribution  &bucket_sizes,
        std::vector<Elem> &to_vec)
    {
      while (from != to)
      {
        to_vec[bucket_sizes[extractor(*from)]++] = *from;
        ++from;
      }
    }

    /**
     * Perform a bucket-sort of the elements in the range [from,to), using the
     * given extractor to determine the sorting criterion. The result will
     * be sorted into to_vec. to_vec must be resize()'ed by the caller
     * before the call. bucket_sizes must be prepared by the caller so it
     * provides the size of each bucket.
     */
    template <typename Iterator, typename CharExtractor, typename Elem>
    static void bucket_sort2(
        Iterator           from,
        Iterator           to,
        CharExtractor      extractor,
        CharDistribution  &bucket_sizes,
        rlxutil::parallel_vector<Elem> &to_vec)
    {
      while (from != to)
      {
        to_vec[bucket_sizes[extractor(*from)]++] = *from;
        ++from;
      }
    }

    /**
     * Sort trigrams lexicographically. Any of the specializations of `TrigramImpl`
     * can be used to represent trigrams. The sorting operation is performed
     * in three passes of radix sort.
     */
    template <template <class,class> class Container, typename TrigramType, typename Alloc>
    static void sort_23trigrams(Container<TrigramType,Alloc> &trigrams)
    {
      /* Extractor function for the third character of every trigram. */
      auto extractor3 = [](const TrigramType &trigram) { return triget3(trigram); };
      /* Determine the alphabet and distribution of trigram-final characters. */
      CharDistribution bucket_sizes {
        accumulated_charcounts(begin(trigrams),end(trigrams),extractor3)
      };
      /* Radix sort, first pass. */
      Container<TrigramType,Alloc> temp_vec {};
      temp_vec.resize(trigrams.size());
      bucket_sort(begin(trigrams),end(trigrams),extractor3,bucket_sizes,temp_vec);
      std::swap(trigrams,temp_vec);
      /* Fresh bucket size calculation and radix sort, second pass. */
      auto extractor2 = [](const TrigramType &trigram) { return triget2(trigram); };
      bucket_sizes = accumulated_charcounts(
          std::begin(trigrams),std::end(trigrams),extractor2);
      bucket_sort(begin(trigrams),end(trigrams),extractor2,bucket_sizes,temp_vec);
      std::swap(trigrams,temp_vec);
      /* Fresh bucket size calculation and radix sort, second pass. */
      auto extractor1 = [](const TrigramType &trigram) { return triget1(trigram); };
      bucket_sizes = accumulated_charcounts(begin(trigrams),end(trigrams),extractor1);
      bucket_sort(begin(trigrams),end(trigrams),extractor1,bucket_sizes,temp_vec);
      std::swap(trigrams,temp_vec);
    }

    /**
     * Perform one pass of radix sort for trigrams, using the specified number
     * of parallel threads.
     */
    template <template <class,class> class Container, typename TrigramType, typename Extractor, typename Alloc>
    static void sort_23trigrams_one_pass(
        const Container<TrigramType,Alloc> &trigrams,
        Container<TrigramType,Alloc>       &dest_vec,
        Extractor                           extractor,
        const unsigned                      threads)
    {
      /* Determine start and end positions of each thread. */
      typedef typename Container<TrigramType,Alloc>::const_iterator It;
      const std::size_t portion { trigrams.size() / threads };
      std::vector<std::pair<It,It>> offsets(threads);
      It endpos { begin(trigrams) };
      std::generate(begin(offsets),end(offsets),[&endpos,portion]() {
        It startpos { endpos };
        endpos += portion;
        return std::make_pair(startpos,endpos);
      });
      offsets.back().second = end(trigrams);

      /* Use thread to determine the alphabet and distribution of
       * extracted characters. */
      std::vector<std::future<CharDistribution>> frqtab_future_vec {};
      for (const std::pair<It,It> &offset : offsets)
      {
        /* Character frequency count for each thread. */
        std::future<CharDistribution> fut {
          std::async(std::launch::async,
              generate_freq_table<It,Extractor,CharDistribution>,
              offset.first,offset.second,extractor)
        };
        frqtab_future_vec.push_back(std::move(fut));
      }

      /* Initialise cumulative frequencies per thread. This will
       * be filled with the correct values later. */
      std::vector<CharDistribution> tl_cumul_frqtab_vec {};

      /* Total frequency, for each character. */
      CharDistribution cumul_frqtab {};
      for (auto &it : frqtab_future_vec)
      {
        /* Move the thread-local frequency table out
         * of its future. */
        CharDistribution tl_frqtab { it.get() };
        /* Update the total frequency table. */
        for (const CharFrequency &entry : tl_frqtab)
          cumul_frqtab[entry.first] += entry.second;
        /* Move thread-local frequency table to the end of the
         * thread-local cumulative frequency table list. Note
         * that the frequencies are not really cumulative yet;
         * this will be corrected later. */
        tl_cumul_frqtab_vec.push_back(std::move(tl_frqtab));
      }

      /* Cumulate the entries of the global frequency table. */
      accumulate_frequencies(cumul_frqtab);

      /* Correct the cumulative thread-local frequencies. */
      for (CharDistribution &tl_frqtab : tl_cumul_frqtab_vec) {
        /* Swap with current version of global cumulative
         * frequency table. */
        std::swap(cumul_frqtab,tl_frqtab);
        /* Add local character frequencies of current thread to
         * new version of global table. */
        for (const CharFrequency &entry : tl_frqtab)
          cumul_frqtab[entry.first] += entry.second;
      }
      /* Radix-sorting threads. */
      std::vector<std::future<void>> sort_future_vec;
      auto tl_cumul_frqtab = tl_cumul_frqtab_vec.begin();
      for (const std::pair<It,It> &offset : offsets)
      {
        sort_future_vec.push_back(std::async(std::launch::async,
            bucket_sort<It,Extractor,TrigramType>,
            offset.first,offset.second,extractor,ref(*tl_cumul_frqtab),ref(dest_vec)));
        ++tl_cumul_frqtab;
      }
      for (auto &sort_future : sort_future_vec)
        sort_future.get();
    }

    template<AlphabetClass alphabetclass = AlphabetClass::sparse>
    struct AlphabetSpecific;

    /**
     * Sort trigrams lexicographically, using the specified number of threads.
     * Any of the specializations of `TrigramImpl` can be used to represent
     * trigrams. The sorting operation is performed in three passes of radix sort.
     */
    template <template <class,class> class Container, typename TrigramType, typename Alloc>
    static void sort_23trigrams(
        Container<TrigramType,Alloc> &trigrams,
        const unsigned                threads)
    {
      /* Sanity. */
      if (threads < 1)
        sort_23trigrams(trigrams,1);
      /* Make sure we don't create too many threads. */
      const std::size_t total { trigrams.size() };
      if ((threads > 1) && (total/threads < 1000))
        sort_23trigrams(trigrams,(total/1000==0?1:total/1000));

      /* Vector for intermediate results. */
      Container<TrigramType,Alloc> temp_vec(trigrams.size());
      /* First pass. */
      sort_23trigrams_one_pass(trigrams,temp_vec,triget3<TrigramType>,threads);
      swap(trigrams,temp_vec);
      /* Second pass. */
      sort_23trigrams_one_pass(trigrams,temp_vec,triget2<TrigramType>,threads);
      swap(trigrams,temp_vec);
      /* Third pass. */
      sort_23trigrams_one_pass(trigrams,temp_vec,triget1<TrigramType>,threads);
      swap(trigrams,temp_vec);
    }

  };

  template <typename Char, typename Pos>
  template <AlphabetClass alphaclass>
  struct TrigramSorter<Char,Pos>::AlphabetSpecific
  {
    typedef TrigramSorter<Char,Pos>                   base;
    typedef Alphabet<alphaclass,Char,Pos>             alphabet_type;
    typedef typename alphabet_type::freq_table_type   freq_table_type;
    template <typename Elem> using parallel_vector = rlxutil::parallel_vector<Elem>;

    /**
     * Perform one pass of radix sort for trigrams, using the specified number
     * of parallel threads.
     */
    template <typename TrigramType, typename Extractor>
    static void sort_23trigrams_one_pass(
        parallel_vector<TrigramType>  &trigrams,
        parallel_vector<TrigramType>  &dest_vec,
        Extractor                      extractor)
    {
      using std::vector;
      using std::make_tuple;
      using std::ref;
      using namespace rlxutil::parallel_vector_tools;
      using namespace alphabet_tools;

      typedef parallel_vector<TrigramType>      vec_type;
      typedef typename vec_type::const_iterator It;

      /* Create a frequency list of characters. */
      auto frqtab_vec = trigrams.parallel_apply
          (make_freq_table<freq_table_type,It,Extractor>,extractor);

      /* Initialise cumulative frequencies per thread. This will
       * be filled with the correct values later. */
      vector<freq_table_type> cumul_frqtab_vec
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
        alphabet_type::add_char_freq_table(cumul_frqtab,frqtab);
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
        alphabet_type::add_char_freq_table(cumul_frqtab,frqtab);
      }

      /* Radix-sorting threads. */
      auto sort_fut_vec = trigrams.parallel_apply_generate_args
          (base::bucket_sort2<It,Extractor,TrigramType>,
           arg_generator(
               [&cumul_frqtab_vec,&dest_vec,&extractor](int thread)
           {
              return make_tuple(extractor,ref(cumul_frqtab_vec[thread]),ref(dest_vec));
           })
          );

      wait_for_results(sort_fut_vec);
    }

    template <typename TrigramType>
    static void sort_23trigrams(parallel_vector<TrigramType> &trigrams)
    {
      using rlxutil::parallel_vector_tools::make_same_size_vector;

      /* Vector for intermediate results. */
      auto temp_vec = make_same_size_vector(trigrams);
      /* First pass. */
      sort_23trigrams_one_pass(trigrams,temp_vec,triget3<TrigramType>);
      swap(trigrams,temp_vec);
      /* Second pass. */
      sort_23trigrams_one_pass(trigrams,temp_vec,triget2<TrigramType>);
      swap(trigrams,temp_vec);
      /* Third pass. */
      sort_23trigrams_one_pass(trigrams,temp_vec,triget1<TrigramType>);
      swap(trigrams,temp_vec);
    }
  };

  template <TGImpl tgimpl = TGImpl::tuple, typename Char = char>
  typename TrigramMaker<tgimpl,Char,typename std::basic_string<Char>::size_type>::trigram_vec_type
  string_to_23trigrams(
      const std::basic_string<Char> &str)
  {
    return TrigramMaker<tgimpl,Char,typename std::basic_string<Char>::size_type>::make_23trigrams(
        begin(str),end(str));
  }

  template <template <class,class> class Container, TGImpl tgimpl, typename Char, typename Pos, typename Alloc>
  void sort_23trigrams(
      Container<TrigramImpl<tgimpl,Char,Pos>,Alloc> &trigrams,
      const unsigned threads = 1)
  {
    TrigramSorter<Char,Pos>::sort_23trigrams(trigrams,threads);
  }

}

#endif /* SUX_HPP_ */
