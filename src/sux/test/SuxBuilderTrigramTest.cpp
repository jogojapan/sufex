//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include "../sux.hpp"
#include "../../util/random.hpp"
#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <boost/chrono.hpp>

using namespace std;

typedef unsigned char             Char;
typedef unsigned short            Pos;
typedef unsigned long             LPos;

typedef sux::SuxBuilder<Char,Pos> Builder;
typedef Builder::Trigram          Trigram;
typedef Builder::Trigrams         Trigrams;
typedef Builder::CharFrequency    CharFrequency;
typedef Builder::CharDistribution CharDistribution;

typedef sux::SuxBuilder<Char,LPos> LBuilder;
typedef LBuilder::Trigram          LTrigram;
typedef LBuilder::Trigrams         LTrigrams;
typedef LBuilder::CharFrequency    LCharFrequency;
typedef LBuilder::CharDistribution LCharDistribution;

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_3)
{
  const std::basic_string<Char> input { (const Char *)"abc" };
  Builder::Trigrams expected {
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_4)
{
  const std::basic_string<Char> input { (const Char *)"abcd" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_5)
{
  const std::basic_string<Char> input { (const Char *)"abcde" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_6)
{
  const std::basic_string<Char> input { (const Char *)"abcdef" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_7)
{
  const std::basic_string<Char> input { (const Char *)"abcdefg" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' },
    Trigram { 4,'e','f','g' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_8)
{
  const std::basic_string<Char> input { (const Char *)"abcdefgh" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' },
    Trigram { 4,'e','f','g' },
    Trigram { 5,'f','g','h' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_chardistribution_test)
{
  const std::basic_string<Char> input { (const Char *)"abcabbbbcc" };
  vector<CharFrequency> expected {
    CharFrequency { 'a',0 },
    CharFrequency { 'b',2 },
    CharFrequency { 'c',7 }
  };

  CharDistribution actual { Builder::accumulated_charcounts(begin(input),end(input)) };
  BOOST_CHECK(actual.size() == 3);
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected),
      [](const CharFrequency &f1, const CharFrequency &f2){
    return ((f1.first == f2.first) && (f1.second == f2.second));
  }));
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test1)
{
  const std::basic_string<Char> input { (const Char *)"aecabfgc" };
  Builder::Trigrams expected {
    Trigram { 4,'b','f','g' },
    Trigram { 2,'c','a','b' },
    Trigram { 1,'e','c','a' },
    Trigram { 5,'f','g','c' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  Builder::sort_23trigrams(actual);
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

//BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test2)
//{
//  /* Prepare for precise time measurements. */
//  typedef chrono::high_resolution_clock       Clock;
//  typedef chrono::duration<double,std::milli> MS;
//
//  /* Generate a random string of 40m characters. */
//  constexpr std::size_t N = 40 * 1024 * 1024;
//  std::basic_string<Char> input;
//  input.resize(N);
//  std::generate_n(begin(input),N,rlxutil::RandomSequenceGeneratorUniform<Char>(' ','z'));
//
//  /* Generate trigrams. */
//  LTrigrams actual = LBuilder::make_23trigrams(begin(input),end(input));
//  /* Make a copy of the trigrams, which will later be sorted them separately. */
//  LTrigrams expected { actual };
//
//  /* Trigam sort. */
//  auto tp1 = Clock::now();
//  LBuilder::sort_23trigrams(actual);
//  auto tp2 = Clock::now();
//  /* Alternative trigram sort, as reference. */
//  auto tp3 = Clock::now();
//  std::stable_sort(std::begin(expected),std::end(expected),
//      [](const LTrigram &tri1, const LTrigram &tri2) {
//          return ((LBuilder::triget1(tri1) < LBuilder::triget1(tri2))
//                  || ((LBuilder::triget1(tri1) == LBuilder::triget1(tri2))
//                      && (LBuilder::triget2(tri1) < LBuilder::triget2(tri2)))
//                  || ((LBuilder::triget1(tri1) == LBuilder::triget1(tri2))
//                      && (LBuilder::triget2(tri1) == LBuilder::triget2(tri2))
//                      && (LBuilder::triget3(tri1) < LBuilder::triget3(tri2))));
//    });
//  auto tp4 = Clock::now();
//
//  /* Print time measurements. */
//  cout << setw(18) << "Total trigrams:" << setw(10)
//      << distance(begin(expected),end(expected)) << '\n'
//      << fixed << setprecision(3)
//      << setw(18) << "Radix sort:" << setw(10)
//      << chrono::duration_cast<MS>(tp2-tp1).count()
//      << " ms\n"
//      << setw(18) << "Stable sort:" << setw(10)
//      << chrono::duration_cast<MS>(tp4-tp3).count()
//      << " ms" << endl;
//
//  /* Check for equality of the two results. */
//  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected)));
//}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test3)
{
  /* Prepare for precise time measurements. */
  typedef std::chrono::high_resolution_clock       Clock;
  typedef boost::chrono::process_cpu_clock         CPUClock;
  typedef std::chrono::duration<double,std::milli> MS;

  /* Generate a random string of 40m characters. */
  constexpr std::size_t N = 40 * 1024 * 1024;
  std::basic_string<Char> input;
  input.resize(N);
  std::generate_n(begin(input),N,
      rlxutil::RandomSequenceGeneratorUniform<Char>(' ','z'));

  /* Generate trigrams. */
  LTrigrams actual = LBuilder::make_23trigrams(begin(input),end(input));
  /* Make a copy of the trigrams, which will later be sorted them separately. */
  LTrigrams expected { actual };

  /* Trigam sort. */
  auto real_tp1 = Clock::now();
  auto cpu_tp1  = CPUClock::now();
  LBuilder::sort_23trigrams(actual,4);
  auto real_tp2 = Clock::now();
  auto cpu_tp2  = CPUClock::now();
  /* Alternative trigram sort, as reference. */
  auto real_tp3 = Clock::now();
  auto cpu_tp3  = CPUClock::now();
  std::stable_sort(std::begin(expected),std::end(expected),
      [](const LTrigram &tri1, const LTrigram &tri2) {
          return ((LBuilder::triget1(tri1) < LBuilder::triget1(tri2))
                  || ((LBuilder::triget1(tri1) == LBuilder::triget1(tri2))
                      && (LBuilder::triget2(tri1) < LBuilder::triget2(tri2)))
                  || ((LBuilder::triget1(tri1) == LBuilder::triget1(tri2))
                      && (LBuilder::triget2(tri1) == LBuilder::triget2(tri2))
                      && (LBuilder::triget3(tri1) < LBuilder::triget3(tri2))));
    });
  auto real_tp4 = Clock::now();
  auto cpu_tp4  = CPUClock::now();

  auto cpu_duration1 = boost::chrono::process_cpu_clock_times(CPUClock::duration(cpu_tp2 - cpu_tp1).count());
  auto cpu_duration2 = boost::chrono::process_cpu_clock_times(CPUClock::duration(cpu_tp4 - cpu_tp3).count());
  /* Print time measurements. */
  cout << setw(18) << "Total trigrams:" << setw(10)
      << distance(begin(expected),end(expected)) << '\n'
      << fixed << setprecision(3)
      << setw(18) << "Radix sort:" << setw(10)
      << chrono::duration_cast<MS>(real_tp2 - real_tp1).count()
      << " ms (CPU/user "
      << boost::chrono::duration_cast<boost::chrono::milliseconds>(static_cast<boost::chrono::nanoseconds>(cpu_duration1.user))
      << " [" << cpu_duration1.user << "] "
      << ", CPU/system "
      << boost::chrono::duration_cast<boost::chrono::milliseconds>(static_cast<boost::chrono::nanoseconds>(cpu_duration1.system))
      << " [" << cpu_duration1.system << "] "
      << ")\n"
      << setw(18) << "Stable sort:" << setw(10)
      << chrono::duration_cast<MS>(real_tp4 - real_tp3).count()
      << " ms (CPU/user "
      << boost::chrono::duration_cast<boost::chrono::milliseconds>(static_cast<boost::chrono::nanoseconds>(cpu_duration2.user))
      << " [" << cpu_duration2.user << "] "
      << ", CPU/system "
      << boost::chrono::duration_cast<boost::chrono::milliseconds>(static_cast<boost::chrono::nanoseconds>(cpu_duration2.system))
      << " [" << cpu_duration2.system << "] "
      << ")"
      << endl;

  /* Check for equality of the two results. */
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected)));
}
