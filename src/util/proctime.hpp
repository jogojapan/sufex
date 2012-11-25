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

  template <typename Ratio>
  struct ratiostr;

  template<> struct ratiostr<std::pico>
  { static constexpr const char *repr = "picosec"; };
  template<> struct ratiostr<std::nano>
  { static constexpr const char *repr = "nanosec"; };
  template<> struct ratiostr<std::micro>
  { static constexpr const char *repr = "microsec"; };
  template<> struct ratiostr<std::milli>
  { static constexpr const char *repr = "millisec"; };
  template<> struct ratiostr<std::centi>
  { static constexpr const char *repr = "centisec"; };
  template<> struct ratiostr<std::deci>
  { static constexpr const char *repr = "decisec"; };
  template<> struct ratiostr<std::ratio<1>>
  { static constexpr const char *repr = "sec"; };
  template<> struct ratiostr<std::ratio<60>>
  { static constexpr const char *repr = "min"; };
  template<> struct ratiostr<std::ratio<3600>>
  { static constexpr const char *repr = "hrs"; };


  template <typename Period>
  static std::intmax_t tickfactor() {
    static std::intmax_t result = 0;
    if (result == 0)
    {
      result = ::sysconf(_SC_CLK_TCK);
      if (result <= 0) {
        LOG(ERROR) << "Could not retrieve number of clock ticks per second (_SC_CLK_TCK).";
        result = -1;
      } else if (result > Period::den) {
        LOG(ERROR) << "Found more than 1 clock tick per "
            << ratiostr<Period>::repr
            << ". rlxutil::cpu_clock can't handle that.";
        result = -1;
      } else {
        result = Period::den / ::sysconf(_SC_CLK_TCK);
        LOG(INFO) << "Clock ticks per "
            << ratiostr<Period>::repr << ": " << result;
      }
    }
    return result;
  }

  template <typename Precision>
  struct rep_combined_clock
  {
    typedef Precision                             precision;
    typedef std::tuple<
        std::clock_t,
        std::clock_t,
        std::chrono::high_resolution_clock::rep>  rep;

    rep _d;
  };

  template <typename Precision>
  class combined_clock
  {
    static_assert(std::chrono::__is_ratio<Precision>::value,
        "Precision must be a std::ratio");
  public:
    typedef rep_combined_clock<Precision>                    rep;
    typedef Precision                                        period;
    typedef std::chrono::duration<rep,period>                duration;
    typedef std::chrono::time_point<combined_clock,duration> time_point;

    static constexpr bool is_steady = true;

    static time_point now() noexcept
    {
      typedef std::chrono::high_resolution_clock hires;
      tms _internal;
      ::times(&_internal);
      return time_point(duration(
          rep { std::make_tuple(
              (_internal.tms_utime + _internal.tms_cutime)
                * tickfactor<period>(),
              (_internal.tms_stime + _internal.tms_cstime)
                * tickfactor<period>(),
              std::chrono::duration_cast<std::chrono::duration<hires::rep,period>>(
                  hires::now().time_since_epoch()).count()
          )}
      ));
    }
  };

} // namespace rlxutil


namespace std
{

  template <typename Precision, typename Period>
  std::ostream &operator<<(
      std::ostream &strm,
      const chrono::duration<rlxutil::rep_combined_clock<Precision>,Period> &dur)
  {
    auto rep = dur.count();
    return (strm << "[user "
        << std::get<0>(rep._d) << ", system "
        << std::get<1>(rep._d) << ", real "
        << std::get<2>(rep._d) << ' ' << rlxutil::ratiostr<Period>::repr << ']');
  }

  namespace chrono
  {

    template <typename Precision, typename Period>
    inline constexpr duration<rlxutil::rep_combined_clock<Precision>,Period>
    operator-(
        const time_point<rlxutil::combined_clock<Precision>,
                         duration<rlxutil::rep_combined_clock<Precision>,Period>> &lhs,
        const time_point<rlxutil::combined_clock<Precision>,
                         duration<rlxutil::rep_combined_clock<Precision>,Period>> &rhs)
    {
      typedef duration<rlxutil::rep_combined_clock<Precision>,Period> dur;
      return dur(rlxutil::rep_combined_clock<Precision> { std::make_tuple(
          std::get<0>(lhs.time_since_epoch().count()._d) - std::get<0>(rhs.time_since_epoch().count()._d),
          std::get<1>(lhs.time_since_epoch().count()._d) - std::get<1>(rhs.time_since_epoch().count()._d),
          std::get<2>(lhs.time_since_epoch().count()._d) - std::get<2>(rhs.time_since_epoch().count()._d)) });
    }

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

    template <typename ToDur, typename Pr, typename Period> inline
    constexpr duration<rlxutil::rep_combined_clock<Pr>,typename ToDur::period>
    duration_cast(const duration<rlxutil::rep_combined_clock<Pr>,Period> &dur)
    {
      return duration<rlxutil::rep_combined_clock<Pr>,typename ToDur::period>(
          rlxutil::rep_combined_clock<Pr>
      {
        rlxutil::tuple::template_map<tuple_duration_element<Period,ToDur>::template caster>(
              dur.count()._d)
      });
    }

  }
}

#endif /* PROCTIME_HPP_ */
