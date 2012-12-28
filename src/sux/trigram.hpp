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
#include <ios>
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

    Char get1() const { return std::get<1>(*this); }
    Char get2() const { return std::get<2>(*this); }
    Char get3() const { return std::get<3>(*this); }

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
  { typedef rlxutil::parallel_vector<TrigramT> vec_type; };

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
    template <typename Iterator, typename CharExtractor, typename Elem>
    static void bucket_sort(
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

    template<AlphabetClass alphabetclass = AlphabetClass::sparse>
    struct AlphabetSpecific;

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
          (base::bucket_sort<It,Extractor,TrigramType>,
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

/* This version of sort_23trigrams is commented out as it causes
 * an internal compiler error with GCC. */
//  template <AlphabetClass alphaclass, typename... Args>
//  void sort_23trigrams(
//      rlxutil::parallel_vector<Args...> &trigrams)
//  {
//    typedef rlxutil::parallel_vector<Args...> vec_type;
//    typedef typename vec_type::value_type     trigram_type;
//    typedef typename trigram_type::char_type  char_type;
//    typedef typename trigram_type::pos_type   pos_type;
//
//    TrigramSorter<char_type,pos_type>::template AlphabetSpecific<alphaclass>::sort_23trigrams(trigrams);
//  }

  template <AlphabetClass alphaclass, typename ParVector>
  void sort_23trigrams(ParVector &trigrams)
  {
    typedef ParVector vec_type;
    typedef typename vec_type::value_type     trigram_type;
    typedef typename trigram_type::char_type  char_type;
    typedef typename trigram_type::pos_type   pos_type;

    TrigramSorter<char_type,pos_type>::template AlphabetSpecific<alphaclass>::sort_23trigrams(trigrams);
  }

  /**
   * Create a new vector of new lexicographical names.
   */
  template <typename Elem, std::size_t min_portion, typename Alloc>
  std::pair< bool , rlxutil::parallel_vector<typename Elem::pos_type,min_portion,Alloc> >
  lexicographical_renaming(rlxutil::parallel_vector<Elem,min_portion,Alloc> &trigrams)
  {
    using std::size_t;
    using std::make_pair;
    using std::make_tuple;
    using std::accumulate;
    using rlxutil::parallel_vector;
    using rlxutil::parallel_vector_tools;

    typedef parallel_vector<Elem,min_portion,Alloc> vec_type;
    typedef typename vec_type::const_iterator       It;

    /* Ensure that threads generated by the parallel vector
     * implementation start and end at boundaries that do
     * not cause incorrect lexical renaming (the trigram
     * at the beginning of a thread must not be identical
     * to the trigram at the end of the previous thread.) */
    trigrams.thread_boundary_adjustment(
        [](It /*beg*/,It it,It end)
        {
          if (it != end) {
            It nx = next(it);
            if ((nx != end) && content_equal(*it,*nx))
              return vec_type::adjustment::needed;
          }
          return vec_type::adjustment::unneeded;
        });

    /* Position type of the existing vector becomes char type. */
    typedef typename Elem::pos_type new_char_type;

    /* Make a new vector, same in size and threads number as the existing one. */
    auto dest_vec = make_same_size_vector_of<new_char_type>(trigrams);

    /* Fill the new vector with the new lexical names. Each thread
     * will start from 0 as the first new name. The future returned
     * by each thread provides the total number of unique names
     * created by that thread. */
    auto dest_futs =
        trigrams.apply_parallel(
            [](It from, It to, It beg, It dest)
            {
              using std::distance;
              using trigram_tools::content_equal;
              typedef typename Elem::pos_type pos_type;

              pos_type current_name
              { 0 };

              if (from == to)
                return current_name;

              dest += distance(beg,from);
              *dest = current_name;
              ++dest;
              It next = std::next(from);
              while (from != to)
                {
                  if (!content_equal(*from,*next))
                    ++current_name;
                  *dest = current_name;
                  ++dest;
                  from = next;
                  next = std::next(from);
                }

              /* Return the current name, i.e. the total
               * number of unique names. */
              return current_name;
            },
            trigrams.begin(),dest_vec.begin()
        );

    /* Wait for the threads to finish and add up all the totals. */
    pos_type total_names =
        accumulate(dest_futs.begin(),dest_futs.end(),
            [](pos_type total, const future<pos_type> &fut)
            { return total + fut.get(); }
    );

    /* From the second thread on, all threads require post-correction
     * since the names they created start at 0 but need to start at
     * wherever the previous thread ended. */
    auto correction_futs =
        dest_vec.apply_parallel_generate_args(
            [](It from, It to, bool skip, size_t start)
            {
              if (skip)
                return;
              while (from != to)
                *from += start;
            },
            [](size_t thread)
            {
              if (thread == 0)
                return make_tuple(true,(size_t)0);
              return make_tuple(false,dest_futs[thread-1].get());
            }
        );

    /* Wait for threads to finish. */
    wait_for_results(correction_futs);

    /* Return a pair { bool , vec } where bool is true if all new
     * lexicographical names were unique, and vec is the vector with
     * the new names. */
    return { (total_names == (pos_type)trigrams.size()) , dest_vec };
  }

}

#endif /* SUX_HPP_ */
