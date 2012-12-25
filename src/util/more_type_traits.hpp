/*
 * more_type_traits.hpp
 *
 *  Created on: 2012/12/25
 *      Author: gollerjo
 */

#ifndef MORE_TYPE_TRAITS_HPP_
#define MORE_TYPE_TRAITS_HPP_

namespace rlxutil {

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
