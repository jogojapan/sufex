/*
 * parallelization.hpp
 *
 *  Created on: 2012/12/30
 *      Author: gollerjo
 */

#ifndef PARALLELIZATION_HPP_
#define PARALLELIZATION_HPP_

#include <cstddef>
#include <cassert>
#include <vector>
#include <utility>
#include <functional>
#include <stdexcept>
#include <string>
#include <iterator>

#include "more_type_traits.hpp"
#include "tupletools.hpp"

namespace rlxutil {

  namespace parallel {

    namespace tools {
      template <typename Fun>
      Fun &&arg_generator(Fun&& fun)
      {
        static_assert(function_traits<Fun>::arity == 1,
            "For generator(fun) to work, fun needs to be a function that takes exactly one integer argument.");
        typedef typename function_traits<Fun>::template arg<0>::type first_arg_type;
        static_assert(std::is_integral<first_arg_type>::value,
            "For generator(fun) to work, fun needs to be a function that takes exactly one integer argument.");
        typedef typename function_traits<Fun>::result_type result_type;
        static_assert(is_instance_of<std::tuple,result_type>::value,
            "For generator(fun) to work, fun needs to be a function that returns tuples.");
        return std::forward<Fun>(fun);
      }

      template <typename Fun>
      struct is_arg_generator
      {
        static constexpr bool value =
            (function_traits<Fun>::arity == 1
                && std::is_integral<typename function_traits<Fun>::template arg<0>::type>::value
                && is_instance_of<std::tuple,typename function_traits<Fun>::result_type>::value
            );
      };

      /**
       * Given a vector of futures, wait for all results to become available.
       */
      template <typename... Args>
      static void wait_for(const std::vector<std::future<Args...>> &futs)
      {
        for (auto &fut : futs)
          fut.wait();
      }

    }

    /**
     * Exception thrown when a `portions` object is applied to
     * a range it does not fit, either because it is undefined
     * or because the total number of items it covers is different
     * from the size of the range it is applied to.
     */
    class portion_error : public std::exception
    {
    public:
      explicit portion_error(const char *message) :
        _msg(message)
      { }
      explicit portion_error(const std::string &message) :
        _msg(message)
      { }

      const char *what() const noexcept
      { return _msg.c_str(); }
    private:
      std::string _msg;
    };

    class portions
    {
    public:
      typedef ptrdiff_t diff_t;

      /**
       * Return values for boundary adjustment functions.
       *
       * Boundary adjustment functions are used to modify the boundary
       * between two adjacent threads.
       * Given arguments `(beg,it,end)`, the function should return
       * adjustment::needed if the offset represented by the iterator
       * `it` is not a valid thread boundary. It will then adjusted
       * rightward in steps of 1 until the function returns
       * adjustment::unneeded. `beg` and `end` are constant iterators
       * to the beginning and end of the internal vector, which
       * the function may use for checking purposes (e.g. adjustment
       * may be needed when `(it+1)` has certain properties, but
       * to be ablet o check this, the function must first check
       * that `(it+1)` is not equal to `end`. Note that the function
       * needs **not** check whether `it` itself is equal to `end`.
       * It can suggest adjustment even if it is; the adjustment will
       * then not be carried out, however (there is an automatic
       * boundary check built into the boundary adjustment routine). */
      enum class adjustment : bool
      { unneeded , needed };

      portions(unsigned min_portion_size = 10000) :
        _min_portion_size(static_cast<diff_t>(min_portion_size)),
        _offsets(),
        _total_range()
      { }

      template <typename It>
      portions(It start, It end, std::size_t num_portions, diff_t min_portion_size = 10000) :
        _min_portion_size(static_cast<diff_t>(min_portion_size)),
        _offsets(),
        _total_range(std::distance(start,end))
      { assign(start,end,num_portions); }

      // This might cause conflicts with the previous non-template
      // declaration.
//      template <typename It, typename BoundAdjust>
//      portions(It start, It end, diff_t num_portions,
//          BoundAdjust &&boundary_adjuster, diff_t min_portion_size = 10000) :
//        _min_portion_size(min_portion_size),
//        _offsets(),
//        _total_range(std::distance(start,end))
//      { assign(start,end,num_portions,std::forward<BoundAdjust>(boundary_adjuster)); }

      /**
       * Calculate the start and end offsets for `num` portions of a range of
       * values. If no boundary adjustment is enabled (final argument nullptr),
       * the portions will be equally sized (except the last one, which may be
       * slightly larger than the others).
       * @param start The starting point of the range of values
       * @param end The ending point of the range of values
       * @param num The maximum number of portions. If boundary adjustment
       *    has been enabled, this actual number you get may be smaller.
       * @param boundary_adjuster `nullptr` or a function that takes three
       *     `It` iterators as arguments and returns `adjustment::needed`
       *     or `adjustment::unneeded`.
       */
      template <typename It, typename BoundAdjust = std::nullptr_t>
      void assign(It range_from, It range_to, const std::size_t num_threads,
          BoundAdjust &&boundary_adjuster = nullptr)
      {
        using std::forward;
        using std::distance;
        using std::pair;

        /* The number of threads becomes a signed integer here.
         * From here on, all integer operations related to the
         * number and size of portions are signed. */
        const diff_t num = static_cast<diff_t>(num_threads);

        /* Sanity. */
        if (num < 1)
          assign(range_from,range_to,1,forward<BoundAdjust>(boundary_adjuster));
        /* Make sure we don't create too many threads. The
         * initialization below involves casting from unsigned
         * to signed. */
        const diff_t total
        { distance(range_from,range_to) };
        if ((num > 1) && (total/num < _min_portion_size))
          assign(
              range_from,range_to,
              ((total / _min_portion_size == 0) ? 1 : (total / _min_portion_size)),
              forward<BoundAdjust>(boundary_adjuster));

        /* Calculate boundaries, using the adjustment function if any
         * was given by the caller. */
        if (boundary_adjuster == nullptr)
          calculate_boundaries(range_from,range_to,num,nullptr);
        else
          calculate_boundaries(range_from,range_to,num,forward<BoundAdjust>(boundary_adjuster));

        /* Update total-range value. */
        _total_range = std::accumulate(_offsets.begin(),_offsets.end(),0,
            [](diff_t t, const pair<diff_t,diff_t> &p) { return t + (p.second - p.first); });

        /* Paranoia. */
        assert(distance(range_from,range_to) == _total_range);
      }

      /**
       * Return the number of portions currently maintained.
       */
      std::size_t num() const
      { return _offsets.size(); }

      /**
       * Return the minimum number of entries per portion configured for
       * this portions object.
       */
      std::size_t min_portion_size() const
      { return static_cast<std::size_t>(_min_portion_size); }

      /**
       * Apply a function to all elements of the vector, running multiple
       * threads in parallel.
       * @parameter from The starting point of the range to process
       * @parameter to The end point of the range to process. Note that
       *   std::distance(from,to) must match the _total_range of the
       *   `portions` object, otherwise an exception will be thrown.
       * @parameter fun The function to be applied. The function must take
       *   at least two `const_iterator` arguments, specifying the range
       *   of elements to which it will be applied. It can optionally take
       *   further arguments of any type.
       * @parameter args... Additional arguments to the function
       * @return A vector of futures. The threads may still be running when
       *   the vector is returned. Use `get()` to access each result
       *   (which may imply waiting for the corresponding thread to finish).
       */
      template <typename It, typename Fun, typename... Args>
      std::vector<typename std::future<typename std::result_of<Fun(It,It,Args...)>::type>>
      apply(It from, It to, Fun&& fun, Args&&... args) const
      {
        using std::pair;
        using std::vector;
        using std::future;
        using std::async;
        using std::forward;

        typedef typename std::result_of<Fun(It,It,Args...)>::type result_type;

        if (_offsets.empty())
          throw portion_error("Attempt to apply empty portions object to a range");
        if (_total_range != distance(from,to))
          throw portion_error("Portions object does not fit the range it is applied to");

        DLOG(INFO) << "Running parallel_perform without generator, using "
            << _offsets.size() << " threads";

        vector<future<result_type>> results
        { };

        for (const pair<diff_t,diff_t> &os : _offsets)
          {
            future<result_type> fut
            { async(std::launch::async,
                forward<Fun>(fun),(from + os.first),(from + os.second),forward<Args>(args)...) };
            results.push_back(std::move(fut));
          }

        return results;
      }

      /**
       * Apply a function to all elements of the vector, running multiple
       * threads in parallel.
       * @parameter from The starting point of the range to process
       * @parameter to The end point of the range to process. Note that
       *   std::distance(from,to) must match the _total_range of the
       *   `portions` object, otherwise an exception will be thrown.
       * @parameter fun The function to be applied. The function must take
       *   at least two `const_iterator` arguments, specifying the range
       *   of elements to which it will be applied. It can optionally take
       *   further arguments of any type.
       * @parameter gen A lambda function used to generate arguments for
       *   the calls to fun.
       * @return A vector of futures. The threads may still be running when
       *   the vector is returned. Use `get()` to access each result
       *   (which may imply waiting for the corresponding thread to finish).
       */
      template <typename It, typename Fun, typename Generator>
      typename std::enable_if<tools::is_arg_generator<Generator>::value,
         std::vector<typename std::future<typename function_traits<Fun>::result_type>>>::type
      apply_dynargs(It from, It to, Fun&& fun, Generator gen) const
      {
        using std::pair;
        using std::vector;
        using std::future;
        using std::async;
        using std::forward;
        using rlxutil::call_on_tuple;
        using std::make_tuple;
        using std::tuple_cat;

        typedef typename function_traits<Fun>::result_type result_type;

        if (_offsets.empty())
          throw portion_error("Attempt to apply empty portions object to a range");
        if (_total_range != distance(from,to))
          throw portion_error("Portions object does not fit the range it is applied to");

        DLOG(INFO) << "Running parallel_perform with generator, using "
            << _offsets.size() << " threads";

        vector<future<result_type>> results
        { };

        std::size_t counter = 0;
        for (const pair<diff_t,diff_t> &os : _offsets)
          {
            future<result_type> fut
            {
              async(std::launch::async,
                  [from,os,counter,&fun,&gen]()
                  {
                call_on_tuple(fun,tuple_cat(make_tuple((from + os.first),(from + os.second)),gen(counter)));
                  })
            };
            results.push_back(std::move(fut));
            ++counter;
          }

        return results;
      }

    private:
      /** The minimal size of a portion. Since these portions are used
       * as input to processing threads, this defines the minimum number
       * of items processed by one thread. */
      diff_t                                 _min_portion_size;
      /** Start and end offsets for threads. */
      std::vector<std::pair<diff_t,diff_t>>  _offsets;
      /** The total number of items covered by the portions. */
      diff_t                                 _total_range;

      template <typename Fun, typename Arg>
      struct is_boundary_adjuster
      {
        static constexpr bool value =
            std::is_same<typename std::result_of<Fun(Arg,Arg,Arg)>::type,adjustment>::value;
      };

      template <typename Arg>
      struct is_boundary_adjuster<std::nullptr_t,Arg>
      { static constexpr bool value = true; };

      /**
       * Calculate portion boundaries and re-adjust them with the user-defined
       * adjustment function.
       */
      template <typename It, typename BoundAdjust>
      typename std::enable_if<is_boundary_adjuster<BoundAdjust,It>::value,void>::type
      calculate_boundaries(
          It range_from, It range_to, const diff_t num, BoundAdjust &&boundary_adjuster)
      {
        using std::make_pair;
        using std::distance;
        using std::pair;

        const diff_t total
        { distance(range_from,range_to) };
        const diff_t portion
        { total / num };

        _offsets.resize(num);

        It portion_end
        { range_from };

        std::generate(_offsets.begin(),_offsets.end(),
            [range_from,range_to,portion,&portion_end,&boundary_adjuster]()
        {
          It portion_start
          { portion_end };

          /* Calculate how many items are left in the vector. This may be
           * less than a full portion, due to boundary adjustments.
           * Note: The below involves a cast from signed (difference_type) to
           * unsigned. */
          diff_t remainder = distance(portion_start,range_to);
          if (remainder < portion)
            portion_end = range_to;
          else
            {
              portion_end += (portion - 1);
              while ((portion_end != range_to)
                  && (boundary_adjuster(range_from,portion_end,range_to) == adjustment::needed))
                ++portion_end;
              ++portion_end;
            }
          return make_pair(
              distance(range_from,portion_start),
              distance(range_from,portion_end));
        });

        /* As a result of boundary adjustments, their may be empty portions
         * at the end of the _offsets vector. We remove them. */
        auto last_non_empty =
            std::find_if(_offsets.rbegin(),_offsets.rend(),[](const pair<diff_t,diff_t> &p)
                { return (p.first != p.second); });
        _offsets.erase(last_non_empty.base(),_offsets.end());

        _offsets.back().second = distance(range_from,range_to);
      }

      /**
       * Calculate portion boundaries without user-defined adjustment function.
       * All portions will be equally sized (except the last one, which may be
       * 1 item larger than the rest due to possible non-zero remainders of
       * integer division).
       */
      template <typename It> void
      calculate_boundaries(
          It range_from, It range_to, const diff_t num, std::nullptr_t&&)
      {
        using std::make_pair;
        using std::distance;

        const diff_t total
        { distance(range_from,range_to) };
        const diff_t portion
        { total / num };

        _offsets.resize(num);

        It portion_end
        { range_from };

        std::generate(_offsets.begin(),_offsets.end(),[range_from,&portion_end,portion]()
        {
          It portion_start
          { portion_end };
          portion_end += portion;
          return make_pair(
              distance(range_from,portion_start),
              distance(range_from,portion_end));
        });

        _offsets.back().second = distance(range_from,range_to);
      }

    public:
      /**
       * Access to the threads-boundaries vector. This is used by unit
       * tests to inspect the inner state of the parallel vector.
       * There shouldn't usually be a need for ordinary users to
       * call this function.
       */
//      auto get_boundaries() const -> const decltype(this->_offsets) &
//      { return _offsets; }
      const std::vector<std::pair<diff_t,diff_t>> &get_boundaries() const
      { return _offsets; }
    };

  }

}


#endif /* PARALLELIZATION_HPP_ */
