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
#include "./tupletools.hpp"

namespace rlxutil {

class cpu_user_clock;
class combined_clock;

}

namespace std {
namespace chrono {

//template <typename Duration>
//class time_point<rlxutil::cpu_user_clock,Duration>
//{
//public:
//  typedef rlxutil::cpu_user_clock    clock;
//  typedef Duration                   duration;
//  typedef typename Duration::rep     rep;
//  typedef typename Duration::period  period;
//
//  constexpr time_point() : _d(duration::zero())
//  { }
//
//  constexpr explicit time_point(const duration& _dur)
//    : _d(_dur)
//  { }
//
//  // conversions
//  template<typename Dur2>
//  constexpr time_point(const time_point<clock, Dur2>& _t)
//    : _d(_t.time_since_epoch())
//  { }
//
//  constexpr duration
//  time_since_epoch() const
//  { return _d; }
//
//  time_point&
//  operator+=(const duration& _dur)
//  {
//    _d += _dur;
//    return *this;
//  }
//
//  time_point&
//  operator-=(const duration& _dur)
//  {
//    _d -= _dur;
//    return *this;
//  }
//
//  static constexpr time_point
//  min()
//  { return time_point(duration::min()); }
//
//  static constexpr time_point
//  max()
//  { return time_point(duration::max()); }
//
//private:
//  duration _d;
//
//};
//
//template <typename Duration>
//class time_point<rlxutil::combined_clock,Duration>
//{
//public:
//  typedef rlxutil::combined_clock    clock;
//  typedef Duration                   duration;
//  typedef typename Duration::rep     rep;
//  typedef typename Duration::period  period;
//
//  constexpr time_point() : _d(duration::zero())
//  { }
//
//  constexpr explicit time_point(const duration& _dur)
//    : _d(_dur)
//  { }
//
//  // conversions
//  template<typename Dur2>
//  constexpr time_point(const time_point<clock, Dur2>& _t)
//    : _d(_t.time_since_epoch())
//  { }
//
//  constexpr duration
//  time_since_epoch() const
//  { return _d; }
//
//  time_point&
//  operator+=(const duration& _dur)
//  {
//    _d += _dur;
//    return *this;
//  }
//
//  time_point&
//  operator-=(const duration& _dur)
//  {
//    _d -= _dur;
//    return *this;
//  }
//
//  static constexpr time_point
//  min()
//  { return time_point(duration::min()); }
//
//  static constexpr time_point
//  max()
//  { return time_point(duration::max()); }
//
//private:
//  duration _d;
//
//};

} // namespace
} // namespace

namespace rlxutil {

static std::intmax_t nanosec_per_tick() {
  static std::intmax_t result = 0;
  if (result == 0)
  {
    result = ::sysconf(_SC_CLK_TCK);
    if (result <= 0) {
      LOG(ERROR) << "Could not retrieve number of clock ticks per second (_SC_CLK_TCK).";
      result = -1;
    } else if (result > std::nano::den) {
      LOG(ERROR) << "Found more than 1 clock tick per nanosecond. "
          << "rlxutil::cpu_clock can't handle that.";
      result = -1;
    } else {
      result = std::nano::den / ::sysconf(_SC_CLK_TCK);
      LOG(INFO) << "Clock ticks per nanosecond: " << result;
    }
  }
  return result;
}

class cpu_user_clock
{
public:
  typedef std::clock_t                                rep;
  typedef std::nano                                   period;
  typedef std::chrono::duration<rep,period>           duration;
  typedef std::chrono::time_point<cpu_user_clock,duration> time_point;

  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    tms _internal;
    times(&_internal);
    return time_point(duration((_internal.tms_utime + _internal.tms_cutime)
        * nanosec_per_tick()));
  }
};

class combined_clock
{
public:
  typedef std::tuple<
      std::clock_t,
      std::clock_t,
      std::chrono::high_resolution_clock::rep>             rep;
  typedef std::nano                                        period;
  typedef std::chrono::duration<rep,period>                duration;
  typedef std::chrono::time_point<combined_clock,duration> time_point;

  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    tms _internal;
    times(&_internal);
    return time_point(duration(std::make_tuple(
        (_internal.tms_utime + _internal.tms_cutime)
            * nanosec_per_tick(),
        (_internal.tms_stime + _internal.tms_cstime)
            * nanosec_per_tick(),
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
        )));
  }
};

}

namespace std
{

  template <typename Period>
  std::ostream &operator<<(
      std::ostream &strm,
      const chrono::duration<rlxutil::combined_clock::rep,Period> &dur)
  {
    auto rep = dur.count();
    return (strm << '(' << std::get<0>(rep) << ','
        << std::get<1>(rep) << ','
        << std::get<2>(rep) << ')');
  }

  inline constexpr rlxutil::combined_clock::duration
  operator-(
      const rlxutil::combined_clock::duration &lhs,
      const rlxutil::combined_clock::duration &rhs)
  {
    typedef rlxutil::combined_clock::duration duration;
    return duration(std::make_tuple(
        std::get<0>(lhs.count()) - std::get<0>(rhs.count()),
        std::get<1>(lhs.count()) - std::get<1>(rhs.count()),
        std::get<2>(lhs.count()) - std::get<2>(rhs.count())));
  }

  namespace chrono
  {

    template <typename FromPeriod, typename ToDur>
    struct tuple_duration_element
    {
      template <typename FromT>
      struct caster
      {
        static constexpr typename ToDur::rep apply(FromT t)
        { return duration_cast<ToDur>(duration<FromT,FromPeriod>(t)).count(); }
      };
    };

    template <typename ToDur, typename Period> inline
    constexpr duration<rlxutil::combined_clock::rep,typename ToDur::period>
    duration_cast(const duration<rlxutil::combined_clock::rep,Period> &dur)
    {
      return duration<rlxutil::combined_clock::rep,typename ToDur::period>(
          rlxutil::tuple::template_map<tuple_duration_element<Period,ToDur>::template caster>(
              dur.count())
      );
    }

  }
}

#endif /* PROCTIME_HPP_ */
