/*
 * tupletools.hpp
 *
 *  Created on: 2012/11/24
 *      Author: gollerjo
 */

#ifndef TUPLETOOLS_HPP_
#define TUPLETOOLS_HPP_

#include <tuple>
#include <functional>
#include <type_traits>

namespace rlxutil {
  namespace tuple {

    template <unsigned... Is>
    struct indices
    { };

    /**
     * Use this if you need a variadic template argument consisting
     * of N unsigned integers `N-1,N-2,...0`. Example: Given a variadic
     * function template with unsigned integer parameters:
     *
     *     template <unsigned... Is>
     *     void FunTemplate(indices<Is...>)
     *     {
     *       // Now you can use Is... in variadic template arguments. //
     *     };
     *
     * You can call the template function using the `index_maker`:
     *
     *     // Call the template function above with <4,3,2,1,0>:
     *     FunTemplate(typename index_maker<5>::type());
     */
    template <unsigned N, unsigned... Is>
    struct index_maker : index_maker<N-1,N-1,Is...>
    { };

    template <unsigned... Is>
    struct index_maker<0,Is...>
    { typedef indices<Is...> type; };


    /**
     * Helper function for the tuple-map.
     */
    template <typename F, typename... Ts, unsigned... Is>
    std::tuple<Ts...> map_(
        F f,
        const std::tuple<Ts...> &tup,
        rlxutil::tuple::indices<Is...>)
    {
      return std::make_tuple(f(std::get<Is>(tup))...);
    }

    /**
     * Tuple-map, i.e. a template that applies a function to
     * all elements of a tuple and returns a new tuple of the
     * same length that contains the results.
     *
     * Example:
     *
     *     inline constexpr int half(int i) {
     *        return i / 2;
     *     }
     *
     *     std::tuple<int,int,int> tup { 2,4,8 };
     *     auto ntup = rlxutil::tuple::map(half,tup);
     *     rlxutil::tuple::map([](int i){std::cout << i; return 0;},ntup);
     *     std::cout << std::endl;
     *
     */
    template <typename F, typename... Ts>
    std::tuple<Ts...> map(
        F f,
        const std::tuple<Ts...> &tup)
    {
      return map_(f,tup,typename index_maker<sizeof...(Ts)>::type());
    }

    /**
     * Helper function for `template_map`.
     */
    template <template <class> class F, typename... Ts, unsigned... Is>
    std::tuple<Ts...> template_map_(
        const std::tuple<Ts...> &tup,
        rlxutil::tuple::indices<Is...>)
    {
      return std::make_tuple(F<Ts>::apply(std::get<Is>(tup))...);
    }

    /**
     * Tuple-map using a type-dependent function, i.e. a template
     * that applies a function template to all elements of a tuple
     * and returns a new tuple of the same length that contains
     * the results. The function template must be defined as a
     * static member function of a struct/class template. The
     * prototype of the function must be `apply(T t)`.
     *
     * Example:
     *
     *     template <typename T>
     *     struct template_half
     *     {
     *       static constexpr T apply(T t)
     *       { return t / 4; }
     *     };
     *
     *     std::tuple<float,int,int> tup { 2.0,4,8 };
     *     auto ntup = rlxutil::tuple::template_map<template_half>(tup);
     *     std::cout << std::get<0>(ntup) << ','
     *               << std::get<1>(ntup) << ','
     *               << std::get<2>(ntup) << std::endl;
     *
     */
    template <template <class> class F, typename... Ts>
    std::tuple<Ts...> template_map(
        const std::tuple<Ts...> &tup)
    {
      return template_map_<F>(tup,typename index_maker<sizeof...(Ts)>::type());
    }

  } // namespace tuple

  /**
   * Auxiliary function for call_on_tuple().
   */
  template <typename Fun, typename... Args, unsigned... Is>
  typename std::result_of<Fun(Args...)>::type
  call_on_indexed_tuple(Fun&& f, std::tuple<Args...>&& tup, tuple::indices<Is...>)
  {
    return f(std::get<Is>(tup)...);
  }

  /**
   * Call a function, using the elements of an rvalue-reference tuple as arguments to
   * the function call.
   * @param f The function to be called. The types of its arguments must correspond to
   *    the types of the elements of the tuple.
   * @param tup The tuple of function arguments
   * @return The return value of f(get<0>(tup),get<1>(tup),...);
   */
  template <typename Fun, typename... Args>
  typename std::result_of<Fun(Args...)>::type call_on_tuple(Fun&& f, std::tuple<Args...>&& tup)
  {
    using std::tuple;
    using std::forward;
    using tuple::index_maker;

    return call_on_indexed_tuple
        (forward<Fun>(f),forward<tuple<Args...>>(tup),typename index_maker<sizeof...(Args)>::type());
  }

  /**
   * Auxiliary function for call_on_tuple().
   */
  template <typename Fun, typename... Args, unsigned... Is>
  typename std::result_of<Fun(Args...)>::type
  call_on_indexed_tuple(Fun&& f, const std::tuple<Args...>& tup, tuple::indices<Is...>)
  {
    return f(std::get<Is>(tup)...);
  }

  /**
   * Call a function, using the elements of an lvalue-reference tuple as arguments to
   * the function call.
   * @param f The function to be called. The types of its arguments must correspond to
   *    the types of the elements of the tuple.
   * @param tup The tuple of function arguments
   * @return The return value of f(get<0>(tup),get<1>(tup),...);
   */
  template <typename Fun, typename... Args>
  typename std::result_of<Fun(Args...)>::type call_on_tuple(Fun&& f, const std::tuple<Args...>& tup)
  {
    using std::tuple;
    using std::forward;
    using tuple::index_maker;

    return call_on_indexed_tuple
        (forward<Fun>(f),forward<const tuple<Args...>>(tup),typename index_maker<sizeof...(Args)>::type());
  }

} // namespace rlxutil


#endif /* TUPLETOOLS_HPP_ */
