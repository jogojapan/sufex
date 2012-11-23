/*
 * proctime.hpp
 *
 * Utility classes used to measure process time (user, system and real).
 * This can only be used on Linux.
 *
 *  Created on: 2012/11/11
 *      Author: Johannes Goller
 */

#ifndef PROCTIME_HPP_
#define PROCTIME_HPP_

#include <sys/times.h>
#include <unistd.h>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <ratio>
#include <boost/assert.hpp>
#include <glog/logging.h>

namespace rlxutil {

class cpu_clock;

}

namespace std {
namespace chrono {

template <typename Duration>
class time_point<rlxutil::cpu_clock,Duration>
{
public:
  typedef rlxutil::cpu_clock         clock;
  typedef Duration                   duration;
  typedef typename Duration::rep     rep;
  typedef typename Duration::period  period;

  constexpr time_point() : _d(duration::zero())
  { }

  constexpr explicit time_point(const duration& _dur)
    : _d(_dur)
  { }

  // conversions
  template<typename Dur2>
  constexpr time_point(const time_point<clock, Dur2>& _t)
    : _d(_t.time_since_epoch())
  { }

  constexpr duration
  time_since_epoch() const
  { return _d; }

  time_point&
  operator+=(const duration& _dur)
  {
    _d += _dur;
    return *this;
  }

  time_point&
  operator-=(const duration& _dur)
  {
    _d -= _dur;
    return *this;
  }

  static constexpr time_point
  min()
  { return time_point(duration::min()); }

  static constexpr time_point
  max()
  { return time_point(duration::max()); }

private:
  duration _d;

};

} // namespace
} // namespace

namespace rlxutil {

static std::intmax_t nanosec_per_tick() {
  static std::intmax_t result = 0;
  if (result == 0)
  {
    result = ::sysconf(_SC_CLK_TCK);
    BOOST_ASSERT_MSG(result > 0,
        "Cannot determine number of clock ticks per second.");
    BOOST_ASSERT_MSG(result < std::nano::den,
        "System clock too fast: More than 1 tick per nanosecond.");
  }
  return 1000000000l / ::sysconf(_SC_CLK_TCK);
}

class cpu_clock
{
public:
  typedef std::clock_t                                rep;
  typedef std::nano                                   period;
  typedef std::chrono::duration<rep,period>           duration;
  typedef std::chrono::time_point<cpu_clock,duration> time_point;

  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    tms _internal;
    times(&_internal);
    return time_point(duration(_internal.tms_utime * nanosec_per_tick()));
  }
};

}

#endif /* PROCTIME_HPP_ */
