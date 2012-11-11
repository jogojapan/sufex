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
#include <ctime>
#include <chrono>

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

class cpu_clock
{
public:
  typedef std::clock_t                                rep;
  typedef std::ratio<1,CLOCKS_PER_SEC>                period;
  typedef std::chrono::duration<rep,period>           duration;
  typedef std::chrono::time_point<cpu_clock,duration> time_point;

  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    tms _internal;
    times(&_internal);
    return time_point(duration(_internal.tms_utime));
  }
};

}

#endif /* PROCTIME_HPP_ */
