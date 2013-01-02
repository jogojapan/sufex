/*
 * more_type_traits.hpp
 *
 *  Created on: 2012/12/25
 *      Author: gollerjo
 */

#ifndef MORE_TYPE_TRAITS_HPP_
#define MORE_TYPE_TRAITS_HPP_

namespace rlxtype {

  /**
   * Use this to check at compile-time whether Instance is a template-instantiation
   * of Base. Example: `is_instance_of<std::tuple,std::tuple<int,int>>::value`.
   */
  template <template <typename...> class Base, typename Instance, typename... Args>
  struct is_instance_of
  { static constexpr bool value = false; };

  template <template <typename...> class Base, typename... Args>
  struct is_instance_of<Base,Base<Args...>>
  { static constexpr bool value = true; };

  /**
   * Use this to compute, at compile time, the type of the result of
   * dereferencing an expression of type `T`. This works with pointers,
   * iterators and anything that has a unary `operator*`.
   */
  template <typename T>
  struct deref
  { typedef typename std::remove_reference<decltype(*(std::declval<T>()))>::type type; };

  /**
   * Use this to compute, at compile time, the element type of the given
   * container type T. This assumes that T implements a function begin(),
   * which returns an iterator.
   */
  template <typename T>
  struct elemtype
  { typedef typename std::remove_reference<decltype(*(std::declval<T>().begin()))>::type type; };

  /**
   * Use this to check, at compile time, whether an assignment `T1 = T2;`
   * is possible without loosing information. This will only work for
   * fundamental data types. It basically checks whether `T1` is identical
   * to `T2`. (`std::is_convertible` wouldn't be good for this because
   * it yields true even when the assignment of a value of `T2` to one of
   * `T1` would cause information loss.)
   */
  template <typename T1, typename T2>
  struct is_compatible
  {
    typedef typename std::decay<T1>::type t1;
    typedef typename std::decay<T2>::type t2;
    static constexpr bool value = std::is_same<t1,t2>::value;
  };

  /**
   * Provide traits for functions, including lambda expressions and
   * member function pointers. This specialization is for lambda expressions.
   * Note that lambda expressions have a member-operator(). We take the
   * address of this and delegate (through inheritance) the rest to
   * the specialization for member-pointers to const-functions.
   */
  template <typename T>
  struct function_traits
    : public function_traits<decltype(&T::operator())>
  { };

  template <typename Return, typename... Args>
  struct function_traits<Return(Args...)>
  {
    typedef std::size_t size_type;
    typedef Return      result_type;
    typedef result_type function_type(Args...);

    static constexpr size_type arity = sizeof...(Args);


    template <size_type index>
    struct arg
    {
      typedef typename std::tuple_element<index,std::tuple<Args...>>::type type;
    };

    static constexpr bool value = true;
  };

  template <typename Return, typename... Args>
  struct function_traits<Return(*)(Args...)>
    : function_traits<Return(Args...)>
  { };

  template <typename Return, typename... Args>
  struct function_traits<Return(&)(Args...)>
    : function_traits<Return(Args...)>
  { };

  template <typename Class, typename Return, typename... Args>
  struct function_traits<Return(Class::*)(Args...)>
    : function_traits<Return(Args...)>
  { };

  template <typename Class, typename Return, typename... Args>
  struct function_traits<Return(Class::*)(Args...) volatile>
    : function_traits<Return(Args...)>
  { };

  template <typename Class, typename Return, typename... Args>
  struct function_traits<Return(Class::*)(Args...) const>
    : function_traits<Return(Args...)>
  { };

  template <typename Class, typename Return, typename... Args>
  struct function_traits<Return(Class::*)(Args...) const volatile>
    : function_traits<Return(Args...)>
  { };

}


#endif /* MORE_TYPE_TRAITS_HPP_ */
