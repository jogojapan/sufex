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

#include <iostream>
#include <ios>

#include "alphabet.hpp"
#include "../util/parallelization.hpp"

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
    template <typename Iterator, typename CharExtractor, typename Elem>
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
      using rlxutil::deref;
      using rlxutil::is_compatible;
      using namespace alphabet_tools;
      using rlxutil::parallel::tools::arg_generator;
      using rlxutil::parallel::tools::wait_for;

      typedef typename deref<It>::type elem_type;

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
          (from,to,base::bucket_sort<It,Extractor,elem_type>,
           arg_generator(
               [&cumul_frqtab_vec,dest,&extractor](int thread)
           {
              return make_tuple(extractor,ref(cumul_frqtab_vec[thread]),dest);
           })
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

  template <AlphabetClass alphaclass, typename Vector>
  void sort_23trigrams(Vector &trigrams, unsigned num_threads)
  {
    typedef Vector vec_type;
    typedef typename vec_type::value_type     trigram_type;
    typedef typename trigram_type::char_type  char_type;
    typedef typename trigram_type::pos_type   pos_type;

    TrigramSorter<char_type,pos_type>::template AlphabetSpecific<alphaclass>::sort_23trigrams(
        trigrams,num_threads);
  }

  struct lexicographical_renaming
  {
    enum class recursion : bool { unneeded , needed };
    /**
     * Create new lexicographical names in another container.
     * @return A flag that indicates whether a distinct lexicographical
     *    name was given to each unique trigram (recusion::unneeded), or
     *    not (recusion::needed).
     */
    template <typename It, typename DestIt>
    static recursion apply(
        It start, It end,
        rlxutil::parallel::portions &portions,
        DestIt dest_start, DestIt dest_end)
    {
      using std::move;
      using std::size_t;
      using std::future;
      using std::make_pair;
      using std::make_tuple;
      using std::accumulate;
      using std::distance;
      using trigram_tools::content_equal;

      using rlxutil::deref;
      using rlxutil::is_compatible;
      using rlxutil::parallel::tools::wait_for;
      using rlxutil::parallel::tools::arg_generator;

      typedef typename deref<It>::type      elem_type;
      typedef typename elem_type::pos_type  pos_type;
      typedef typename deref<DestIt>::type  dest_type;
      typedef rlxutil::parallel::portions::adjustment adjustment;

      /* Verify that the destination vector has the right element
       * type. */
      static_assert(is_compatible<dest_type,pos_type>::value,
          "Attempt to perform lexicographical renaming where the "
          "elements of the destination vector do not have the right "
          "data type to store position information from the source "
          "vector");

      if (distance(start,end) != distance(dest_start,dest_end))
        throw std::out_of_range("Attempt to perform lexicographical "
            "renaming into a destination vector that does not have "
            "the right size.");

      /* Ensure that threads generated by the parallel vector
       * implementation start and end at boundaries that do
       * not cause incorrect lexical renaming (the trigram
       * at the beginning of a thread must not be identical
       * to the trigram at the end of the previous thread.) */
      portions.assign(
          start,end,portions.num(),
          [](It /*beg*/,It it,It end)
          {
            if (it != end) {
                It nx = next(it);
                if ((nx != end) && content_equal(*it,*nx))
                  return adjustment::needed;
            }
            return adjustment::unneeded;
          });

      /* Fill the new vector with the new lexical names. Each thread
       * will start from 0 as the first new name. The future returned
       * by each thread provides the total number of unique names
       * created by that thread. */
      auto dest_futs =
          portions.apply(start,end,
              [](It from, It to, It beg, DestIt dest_it)
              {
                using trigram_tools::content_equal;

                pos_type current_name
                { 0 };

                if (from == to)
                  return current_name;

                dest_it += distance(beg,from);
                *dest_it = current_name;
                ++dest_it;
                It next = std::next(from);
                while (next != to)
                  {
                    if (!content_equal(*from,*next))
                      ++current_name;
                    *dest_it = current_name;
                    ++dest_it;
                    from = next;
                    next = std::next(from);
                  }

                /* Return the current name, i.e. the total
                 * number of unique names. */
                return current_name;
              },
              start,dest_start
          );

      /* Wait for the threads to finish and add up all the totals. */
      std::vector<pos_type> total_vec(dest_futs.size());
      std::transform(dest_futs.begin(),dest_futs.end(),total_vec.begin(),
          [](future<pos_type> &fut)
          {
        return fut.get();
          });
//      pos_type total_names =
//          accumulate(total_vec.begin(),total_vec.end(),(pos_type)0,
//              [](pos_type total, future<pos_type> &fut)
//              { return total + fut.get(); }
//          );
      pos_type total_names =
          accumulate(total_vec.begin(),total_vec.end(),(pos_type)0);

      /* From the second thread on, all threads require post-correction
       * since the names they created start at 0 but need to start at
       * wherever the previous thread ended. */
      auto correction_futs =
          portions.apply_dynargs(dest_start,dest_end,
              [](DestIt from, DestIt to, bool skip, size_t start)
              {
                std::cerr << "Called with skip == " << std::boolalpha << skip << std::endl;
                if (!skip)
                  while (from != to) {
                      *from += start;
                      ++from;
                  }
              },
              arg_generator([&total_vec](size_t thread)
              {
                std::cerr << "Making args for thread " << thread << std::endl;
                if (thread == 0)
                  return make_tuple(true,(pos_type)0);
                else {
                    std::cerr << "Starting value " << total_vec[thread-1] << std::endl;
                    return make_tuple(false,(pos_type)(total_vec[thread-1]+thread));
                }
              })
          );

      /* Wait for threads to finish. */
      wait_for(correction_futs);

      /* Return a pair { bool , vec } where bool is true if all new
       * lexicographical names were unique, and vec is the vector with
       * the new names. */
      pos_type original_size = distance(start,end);
      return (total_names == original_size) ? recursion::unneeded : recursion::needed;
    }

    /**
     * Given the container type for the input trigrams
     * (e.g. std::vector<TrigramImpl<...>>), determines
     * the type we expect the elements of the destination
     * container for lexicographical renaming to have.
     */
    template <typename InputVector>
    struct std_dest_element
    {
      typedef typename rlxutil::deref<decltype(std::declval<InputVector>().begin())>::type elem_type;
      typedef typename elem_type::pos_type type;
    };

    template <typename InpVector>
    using result_type =
        std::pair<recursion,std::vector<typename std_dest_element<InpVector>::type>>;

    /**
     * The `apply()` function returns a result that includes information
     * about whether further recusion is required, as well as the actual
     * vector of lexicographical names. Use the function below to
     * extract information about whether recusion is needed:
     *
     *     is<recusion::needed>(result)
     */
    template <recursion val, typename OutVector>
    static bool is(const std::pair<recursion,OutVector> &result)
    { return (result.first == val); }

    /**
     * Extract the new (lexicographically renamed) string (i.e.,
     * chracter vector) from the results of lexicographical
     * renaming. This moves the string out of the result; so
     * it can be done only once for any result object. (Later
     * attempts to do it again will return an empty vector.)
     *
     *     auto result = lexicographical_renaming<...>.apply();
     *     parallel_vector<...> vec = move_newstring_from(result);
     */
    template <typename OutVector>
    static typename std::remove_reference<OutVector>::type &&
    move_newstring_from(std::pair<recursion,OutVector> &result)
    { return std::move(result.second); }

    /**
     * Access to the new (lexicographically renamed) string.
     */
    template <typename OutVector>
    static const OutVector &newstring_of(
        const std::pair<recursion,OutVector> &result)
    { return result.second; }

    /**
     * Access to the new (lexicographically renamed) string.
     */
    template <typename OutVector>
    static OutVector &newstring_of(std::pair<recursion,OutVector> &result)
    { return result.second; }

    /**
     * Perform lexicographical renaming on a lexicographically sorted vector
     * of trigrams. This produces a string (represented as parallel_vector
     * of characters) that contains one character (i.e. one integer) for
     * every trigram of the input vector. The integers will start with 0 and
     * increment for every unique trigram where it occurs for the first time.
     *
     * Usage:
     *
     *     unsigned threads
     *     { 4 };
     *     // ...
     *     std::vector<TrigramImpl<...>> trigrams;
     *     rlxutil::parallel::portions portions
     *     { trigrams.begin(), trigrams.end(), threads };
     *     // ...
     *     typedef lexicographical_renaming lex;
     *
     *     auto result    = lex::apply(trigrams,portions);
     *     auto newstring = lex::move_newstring_from(result);
     *     if (lex::is<recursion::needed>(result))
     *       // perform recursion...
     *
     */
    template <typename InpVector> static
    result_type<InpVector> apply(InpVector &trigrams, rlxutil::parallel::portions &portions)
    {
      typedef typename std_dest_element<InpVector>::type pos_type;
      std::vector<pos_type> dest_vec(trigrams.size());
      lexicographical_renaming::recursion flag = apply(
          trigrams.begin(),trigrams.end(),portions,dest_vec.begin(),dest_vec.end());
      return { flag , std::move(dest_vec) };
    }

  };

  template <typename InpVector> lexicographical_renaming::result_type<InpVector>
  rename_lexicographically(
      InpVector &trigrams, rlxutil::parallel::portions &portions)
  {
    return lexicographical_renaming::apply(trigrams,portions);
  }

  template <typename InpVector> lexicographical_renaming::result_type<InpVector>
  rename_lexicographically(InpVector &trigrams)
  {
    rlxutil::parallel::portions portions
    { trigrams.begin(), trigrams.end(), 4 };
    return lexicographical_renaming::apply(trigrams,portions);
  }

}

#endif /* SUX_HPP_ */
