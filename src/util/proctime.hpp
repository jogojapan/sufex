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
#include <type_traits>
#include <glog/logging.h>
#include "./tupletools.hpp"

namespace rlxutil {

  /**
   * Pretty-printing of time units.
   */
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


  /**
   * Determines the number of milliseconds (or microseconds,
   * or nanoseconds, depending on the value of `Precision`)
   * that pass during one tick of the CPU clock.
   */
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
        LOG(INFO) << "Number of "
            << ratiostr<Period>::repr << " per clock tick: " << result;
      }
    }
    return result;
  }

  /**
   * The triple of user, system and real time used to represent
   * time-points measured by `combined_clock<Precision>`.
   */
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

  /**
   * A type of clock that measures CPU user time, CPU system time as
   * well as CPU-clock based real time. The `time_point` objects it
   * generates contain a triple. You can apply the ordinary `operator-`
   * to two time points to calculate durations and pretty-print them
   * using `<<`.
   *
   * The `Precision` parameter must be a `std::ratio`, typically
   * `std::nano` or `std::micro`. It indicates the precision applied
   * when clock ticks are measured. High precision, e.g. `std::nano`
   * means that integer overflows may occur when long time periods
   * are measured. **Note:** The precision is unrelated to the unit
   * used when pretty-printing the results. For example, a result
   * measured at nanosecond precision can be outputted as milliseconds
   * or vice versa.
   */
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

  /**
   * Pretty-print a `duration` object that caputures a duration measured
   * by a `combined_clock<Precision>`.
   */
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

    /**
     * Calculates the difference between two `combined_clock<Precision>::time_point`
     * objects. Both time points must be generated by combined-clocks operating
     * on the same precision level.
     */
    template <typename Pr, typename Per>
    inline constexpr duration<rlxutil::rep_combined_clock<Pr>,Per>
    operator-(
        const time_point<rlxutil::combined_clock<Pr>,
                         duration<rlxutil::rep_combined_clock<Pr>,Per>> &lhs,
        const time_point<rlxutil::combined_clock<Pr>,
                         duration<rlxutil::rep_combined_clock<Pr>,Per>> &rhs)
    {
      typedef duration<rlxutil::rep_combined_clock<Pr>,Per> dur;
      return dur(rlxutil::rep_combined_clock<Pr> { std::make_tuple(
          std::get<0>(lhs.time_since_epoch().count()._d) - std::get<0>(rhs.time_since_epoch().count()._d),
          std::get<1>(lhs.time_since_epoch().count()._d) - std::get<1>(rhs.time_since_epoch().count()._d),
          std::get<2>(lhs.time_since_epoch().count()._d) - std::get<2>(rhs.time_since_epoch().count()._d)) });
    }

    /**
     * This exists only to prevent users from calculating the difference between
     * two `combined_clock<Precision>` time-points that have different precision
     * levels. If both operands were generated by combined-clocks operating on
     * the same precision level, then the previous definition of `operator-`
     * will be used.
     */
    template <typename Pr1, typename Pr2, typename Per1, typename Per2>
    inline constexpr duration<rlxutil::rep_combined_clock<Pr1>,Per1>
    operator-(
        const time_point<rlxutil::combined_clock<Pr1>,
                         duration<rlxutil::rep_combined_clock<Pr1>,Per1>> &lhs,
        const time_point<rlxutil::combined_clock<Pr2>,
                         duration<rlxutil::rep_combined_clock<Pr2>,Per2>> &rhs)
    {
      static_assert(std::is_same<Pr1,Pr2>::value && std::is_same<Per1,Per2>::value,
          "Cannot apply operator- to combined_clock time points of different precision");
      typedef duration<rlxutil::rep_combined_clock<Pr1>,Per1> dur;
      return dur(rlxutil::rep_combined_clock<Pr1> { std::make_tuple(
          std::get<0>(lhs.time_since_epoch().count()._d) - std::get<0>(rhs.time_since_epoch().count()._d),
          std::get<1>(lhs.time_since_epoch().count()._d) - std::get<1>(rhs.time_since_epoch().count()._d),
          std::get<2>(lhs.time_since_epoch().count()._d) - std::get<2>(rhs.time_since_epoch().count()._d)) });
    }

    /**
     * Helper class that is used to cast one element of a time-point tuple
     * from one unit to another.
     */
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

    /**
     * Implementation of `std::chrono::duration_cast` for `duration` objects
     * generated from `combined_clock<Precision>::time_point` objects. This
     * does not change the precision level, but it does change the unit
     * (e.g. from microseconds to milliseconds) of the `duration` object. The
     * `duration` object contains a tuple for the various types of time
     * measure by `combined_block<Precision>`. The cast is applied to each
     * member of the tuple.
     *
     * Example:
     *
     *     // Note: The std::micro below indicates the precision
     *     //   of the clock, not the unit of the results. Use
     *     //   lower precision (std::milli) to measure long
     *     //   periods, and higher precision (std::nano) for short
     *     //   ones.
     *     auto tp1 = rlxutil::combined_clock<std::micro>::now();
     *     // ..perform actions.. //
     *     auto tp2 = rlxutil::combined_clock<std::micro>::now();
     *
     *     // Use duration_cast to convert into your unit of choice.
     *     std::cout << "Elapsed: "
     *       << std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1)
     *       << std::endl;
     *
     */
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
