#include "../proctime.hpp"

#include <chrono>
#include <boost/chrono.hpp>
#include <thread>
#include <iostream>
#include <glog/logging.h>
#include <ratio>

int main(int /*argc*/, char *argv[])
{
  google::InitGoogleLogging(argv[0]);

  typedef boost::chrono::process_user_cpu_clock         CPUClock;

  auto boost_tp1    = CPUClock::now();
  auto combined_tp1 = rlxutil::combined_clock<std::micro>::now();

  unsigned long step1 = 1;
  unsigned long step2 = 1;
  for (int i = 0 ; i < 10000000 ; ++i) {
    unsigned long step3 = step1 + step2;
    std::swap(step1,step2);
    std::swap(step2,step3);
  }
  std::cout << "Arithmetic result: " << step2 << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));

  auto boost_tp2    = CPUClock::now();
  auto combined_tp2 = rlxutil::combined_clock<std::micro>::now();

  std::cout << "CPU time (Boost)  "
      << boost::chrono::duration_cast<boost::chrono::milliseconds>(boost_tp2 - boost_tp1).count()
      << std::endl;

  std::cout << "CPU time (combined clock) "
      << std::chrono::duration_cast<std::chrono::milliseconds>(combined_tp2 - combined_tp1)
      << std::endl;

  return 0;
}
