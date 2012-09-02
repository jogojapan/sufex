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
#include <array>
#include <vector>
#include <algorithm>
#include <iterator>
#include <map>
#include <future>

namespace sux {

  /** Identity function. Used as auxiliary. */
  template<typename T>
  inline T& id(T &t) { return t; }
  template <typename T>
  inline const T& cid(const T& t) { return t; }

  /** General implementation of trigrams. */
  template <typename Char, typename Pos>
  struct TrigramImpl : public std::tuple<Pos,Char,Char,Char> {
    typedef std::tuple<Pos,Char,Char,Char> base_type;
    typedef Char                           char_type;
    typedef std::vector<TrigramImpl>       vec_type;
  
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
  
  /** `char` implementation of trigrams. */
  template <typename Pos>
  struct TrigramImpl<char,Pos> : public std::tuple<Pos,std::array<unsigned char,3>>
  {
    typedef std::tuple<Pos,std::array<unsigned char,3>> base_type;
    typedef char                                        char_type;
    typedef std::vector<TrigramImpl>                    vec_type;

    TrigramImpl():base_type() {}

    TrigramImpl(
        const Pos pos,
        const char c1,
        const char c2,
        const char c3)
    :base_type(pos,{{c1,c2,c3}})
    {}

    char_type get1() const { return std::get<1>(*this)[0]; }
    char_type get2() const { return std::get<1>(*this)[1]; }
    char_type get3() const { return std::get<1>(*this)[2]; }
  };

  /** `unsigned char` implementation of trigrams. */
  template <typename Pos>
  struct TrigramImpl<unsigned char,Pos> : public std::tuple<Pos,std::array<unsigned char,3>>
  {
    typedef std::tuple<Pos,std::array<unsigned char,3>> base_type;
    typedef unsigned char                               char_type;
    typedef std::vector<TrigramImpl>                    vec_type;

    TrigramImpl():base_type() {}

    TrigramImpl(
        const Pos pos,
        const unsigned char c1,
        const unsigned char c2,
        const unsigned char c3)
    :base_type(pos,{{c1,c2,c3}})
    {}

    char_type get1() const { return std::get<1>(*this)[0]; }
    char_type get2() const { return std::get<1>(*this)[1]; }
    char_type get3() const { return std::get<1>(*this)[2]; }
  };

  template <typename Char, typename Pos>
  struct SuxBuilder
  {
    typedef std::pair<Char,Pos>  CharFrequency;
    typedef std::map<Char,Pos>   CharDistribution;

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
     * Version of <code>determine_chardistribution()</code> that uses const identity as
     * char-extractor, i.e. it assumes the input sequence is a sequence of characters.
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
      while (from != to)
      {
        to_vec[bucket_sizes[extractor(*from)]++] = *from;
        ++from;
      }
    }

    typedef          TrigramImpl<Char,Pos>           Trigram;
    typedef typename TrigramImpl<Char,Pos>::vec_type Trigrams;

    static Char triget1(const Trigram &tri) { return tri.get1(); }
    static Char triget2(const Trigram &tri) { return tri.get2(); }
    static Char triget3(const Trigram &tri) { return tri.get3(); }

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
      /* Extractor function for the third character of every trigram. */
      auto extractor3 = [](const Trigram &trigram) { return SuxBuilder::triget3(trigram); };
      /* Determine the alphabet and distribution of trigram-final characters. */
      CharDistribution bucket_sizes {
        accumulated_charcounts(
            std::begin(trigrams),std::end(trigrams),extractor3)
      };
      /* Radix sort, first pass. */
      std::vector<Elem> temp_vec {};
      bucket_sort(
          std::begin(trigrams),std::end(trigrams),extractor3,bucket_sizes,temp_vec);
      std::swap(trigrams,temp_vec);
      /* Fresh bucket size calculation and radix sort, second pass. */
      auto extractor2 = [](const Trigram &trigram) { return SuxBuilder::triget2(trigram); };
      bucket_sizes = accumulated_charcounts(
          std::begin(trigrams),std::end(trigrams),extractor2);
      bucket_sort(
          std::begin(trigrams),std::end(trigrams),extractor2,bucket_sizes,temp_vec);
      std::swap(trigrams,temp_vec);
      /* Fresh bucket size calculation and radix sort, second pass. */
      auto extractor1 = [](const Trigram &trigram) { return SuxBuilder::triget1(trigram); };
      bucket_sizes = accumulated_charcounts(
          std::begin(trigrams),std::end(trigrams),extractor1);
      bucket_sort(
          std::begin(trigrams),std::end(trigrams),extractor1,bucket_sizes,temp_vec);
      std::swap(trigrams,temp_vec);
    }

    /**
     * Multithreaded version.
     */
    // template <typename Elem>
    // static void sort_23trigrams(
    //     std::vector<Elem> &trigrams,
    //     const unsigned     threads)
    // {
    //   /* Sanity. */
    //   if (threads < 1)
    //     sort_23trigrams(trigrams,1);
    //   /* Make sure we don't create too many threads. */
    //   const std::size_t total { trigrams.size() };
    //   if ((threads > 1) && (total/threads < 1000))
    //     sort_23trigrams(trigrams,(total/1000==0?1:total/1000));
    // 
    //   /* Determine start and end positions of every thread. */
    //   typedef std::vector<Elem>::iterator It;
    //   const std::size_t portion { trigrams.size() / threads };
    //   std::vector<std::pair<It,It>> offsets { threads };
    //   It endpos { std::begin(trigrams) };
    //   std::generate(std::begin(offsets,std::end(offsets),
    //       [&endpos]() {
    //     It startpos { endpos };
    //     endpos += portion;
    //     return std::make_pair(startpos,endpos);
    //   }));
    // 
    //   /* Extractor function for the third character of every trigram. */
    //   auto extractor3 = [](const Trigram &trigram) { return std::get<3>(trigram); };
    //   /* Determine the alphabet and distribution of trigram-final characters. */
    //   std::vector<std::future<CharDistribution>> buckets_future_vec {};
    //   for (const std::pair<It,It> &offset : offsets)
    //   {
    //     /* CHECK THIS. We shouldn't compute accumulated buckets here,
    //      just total frequency per character. */
    //     buckets_future_vec.emplace_back([&trigrams,&offset]() {
    //       accumulated_charcounts(offset.first,offset.second,extractor3)
    //     });
    //   }
    //   CharDistribution cumul_buckets {};
    //   std::vector<CharDistribution> buckets_vec {};
    //   for (auto &local_buckets : buckets_future_vec) {
    //     /* Push copy of accumulated buckets to the end of the vector. */
    //     buckets_vec.push_back(cumul_buckets);
    //     auto &current { buckets_vec.back() };
    //     /* Move the new buckets out of the future. */
    //     CharDistribution buckets { local_buckets.get() };
    //     /* Update the cumulated buckets. */
    //     for (CharFrequency &entry : buckets)
    //       cumul_buckets[entry.first] += entry.second;
    //   }
    //   /* CONTINUE HERE. */
    //   CharDistribution bucket_sizes { accumulated_charcounts(
    //         std::begin(trigrams),std::end(trigrams),extractor3)
    //   };
    //   /* Radix sort, first pass. */
    //   std::vector<Elem> temp_vec {};
    //   bucket_sort(
    //       std::begin(trigrams),std::end(trigrams),extractor3,bucket_sizes,temp_vec);
    //   std::swap(trigrams,temp_vec);
    //   /* Fresh bucket size calculation and radix sort, second pass. */
    //   auto extractor2 = [](const Trigram &trigram) { return std::get<2>(trigram); };
    //   bucket_sizes = accumulated_charcounts(
    //       std::begin(trigrams),std::end(trigrams),extractor2);
    //   bucket_sort(
    //       std::begin(trigrams),std::end(trigrams),extractor2,bucket_sizes,temp_vec);
    //   std::swap(trigrams,temp_vec);
    //   /* Fresh bucket size calculation and radix sort, second pass. */
    //   auto extractor1 = [](const Trigram &trigram) { return std::get<1>(trigram); };
    //   bucket_sizes = accumulated_charcounts(
    //       std::begin(trigrams),std::end(trigrams),extractor1);
    //   bucket_sort(
    //       std::begin(trigrams),std::end(trigrams),extractor1,bucket_sizes,temp_vec);
    //   std::swap(trigrams,temp_vec);
    // }

  };

}


#endif /* SUX_HPP_ */
