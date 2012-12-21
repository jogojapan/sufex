//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ratio>
#include <glog/logging.h>

#include "../trigram.hpp"
#include "../../util/random.hpp"
#include "../../util/proctime.hpp"

using namespace std;

typedef unsigned char             Char;
typedef unsigned short            Pos;
typedef unsigned long             LPos;

typedef sux::TrigramSorter<Char,Pos>  SSorter;
typedef sux::TrigramSorter<Char,LPos> LSorter;

typedef sux::TrigramMaker<sux::TGImpl::arraytuple,Char,Pos> SAMaker;
typedef SAMaker::Trigram             SATrigram;
typedef SAMaker::Trigrams            SATrigrams;
typedef SSorter::CharFrequency       SCharFrequency;
typedef SSorter::CharDistribution    SCharDistribution;

typedef sux::TrigramMaker<sux::TGImpl::arraytuple,Char,LPos> LAMaker;
typedef LAMaker::Trigram            LATrigram;
typedef LAMaker::Trigrams           LATrigrams;
typedef LSorter::CharFrequency      LACharFrequency;
typedef LSorter::CharDistribution   LACharDistribution;

typedef sux::TrigramMaker<sux::TGImpl::pointer,Char,LPos> LPMaker;
typedef LPMaker::Trigram          LPTrigram;
typedef LPMaker::Trigrams         LPTrigrams;

typedef sux::TrigramMaker<sux::TGImpl::tuple,Char,LPos> LTMaker;
typedef LTMaker::Trigram          LTTrigram;
typedef LTMaker::Trigrams         LTTrigrams;


BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_3)
{
  const std::basic_string<Char> input { (const Char *)"abc" };
  SAMaker::Trigrams expected {
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_4)
{
  const std::basic_string<Char> input { (const Char *)"abcd" };
  SAMaker::Trigrams expected {
    SATrigram { 1,'b','c','d' },
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_5)
{
  const std::basic_string<Char> input { (const Char *)"abcde" };
  SAMaker::Trigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' }
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_6)
{
  const std::basic_string<Char> input { (const Char *)"abcdef" };
  SAMaker::Trigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' }
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_7)
{
  const std::basic_string<Char> input { (const Char *)"abcdefg" };
  SAMaker::Trigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' },
    SATrigram { 4,'e','f','g' }
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_8)
{
  const std::basic_string<Char> input { (const Char *)"abcdefgh" };
  SAMaker::Trigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' },
    SATrigram { 4,'e','f','g' },
    SATrigram { 5,'f','g','h' }
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_chardistribution_test)
{
  const std::basic_string<Char> input { (const Char *)"abcabbbbcc" };
  vector<SCharFrequency> expected {
    SCharFrequency { 'a',0 },
    SCharFrequency { 'b',2 },
    SCharFrequency { 'c',7 }
  };

  SCharDistribution actual { SSorter::accumulated_charcounts(begin(input),end(input)) };
  BOOST_CHECK(actual.size() == 3);
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected),
      [](const SCharFrequency &f1, const SCharFrequency &f2){
    return ((f1.first == f2.first) && (f1.second == f2.second));
  }));
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test1)
{
  const std::basic_string<Char> input { (const Char *)"aecabfgc" };
  SAMaker::Trigrams expected {
    SATrigram { 4,'b','f','g' },
    SATrigram { 2,'c','a','b' },
    SATrigram { 1,'e','c','a' },
    SATrigram { 5,'f','g','c' }
  };

  SAMaker::Trigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  SSorter::sort_23trigrams(actual);
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

/**
 * Auxiliary function that performs trigram creation and sorting
 * for a relatively large string and measures time. It also
 * checks whether the result of trigram sorting is correct.
 */
template <sux::TGImpl tgimpl>
void perform_multi_threaded_trigram_sorting()
{
  LOG(INFO) << "Performing sorting test for the "
      << sux::repr<tgimpl>::str
      << " trigram implementation";

  /* Prepare for precise time measurements. */
  typedef std::chrono::duration<double,std::milli> MS;

  /* Generate a random string of 40m characters. */
  constexpr std::size_t N = 40 * 1024 * 1024;
  std::basic_string<Char> input;
  input.resize(N);
  std::generate_n(begin(input),N,
      rlxutil::RandomSequenceGeneratorUniform<Char>(' ','z'));

  /* Generate trigrams. */
  typedef sux::TrigramMaker<tgimpl,Char,LPos> TrigramMaker;
  typedef typename TrigramMaker::Trigram      Trigram;
  typedef typename TrigramMaker::Trigrams     Trigrams;

  Trigrams actual = TrigramMaker::make_23trigrams(begin(input),end(input));
  /* Make a copy of the trigrams, which will later be sorted them separately. */
  Trigrams expected { actual };

  /* Trigam sort. */
  auto tp1 = rlxutil::combined_clock<std::micro>::now();
  LSorter::sort_23trigrams(actual,4);
  auto tp2 = rlxutil::combined_clock<std::micro>::now();
  /* Alternative trigram sort, as reference. */
  auto tp3 = rlxutil::combined_clock<std::micro>::now();
  std::stable_sort(std::begin(expected),std::end(expected),
      [](const Trigram &tri1, const Trigram &tri2) {
          return ((sux::triget1(tri1) < sux::triget1(tri2))
                  || ((sux::triget1(tri1) == sux::triget1(tri2))
                      && (sux::triget2(tri1) < sux::triget2(tri2)))
                  || ((sux::triget1(tri1) == sux::triget1(tri2))
                      && (sux::triget2(tri1) == sux::triget2(tri2))
                      && (sux::triget3(tri1) < sux::triget3(tri2))));
    });
  auto tp4 = rlxutil::combined_clock<std::micro>::now();

  auto duration_radix  = tp2 - tp1;
  auto duration_stable = tp4 - tp3;
  /* Print time measurements. */
  cout << setw(18) << "Total trigrams:" << setw(10)
      << distance(begin(expected),end(expected)) << '\n'
      << setw(18) << "Radix sort:" << setw(10)
      << chrono::duration_cast<MS>(duration_radix) << '\n'
      << setw(18) << "Stable sort:" << setw(10)
      << chrono::duration_cast<MS>(duration_stable) << endl;

  /* Check for equality of the two results. */
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected)));
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test_tuple)
{
  perform_multi_threaded_trigram_sorting<sux::TGImpl::tuple>();
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test3)
{
  perform_multi_threaded_trigram_sorting<sux::TGImpl::arraytuple>();
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test4)
{
  perform_multi_threaded_trigram_sorting<sux::TGImpl::pointer>();
}
