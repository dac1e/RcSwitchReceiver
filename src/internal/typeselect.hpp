/*
 * typesort.hpp
 *
 *  Created on: 21.11.2024
 *      Author: Wolfgang
 */

#ifndef _INTERNAL_TYPESELECT_HPP_
#define _INTERNAL_TYPESELECT_HPP_


#include <type_traits>
#include <tuple>

namespace typeselect {

namespace impl {

// split
template<typename L, typename ...Ts> struct
split {
	using head = L;
	using tail = std::tuple<Ts...>;
};

// select declaration
template<template<typename, typename> class COMPARE, typename L, typename P, typename T> struct
select;

// compare
template<template<typename, typename> class COMPARE, typename P, typename T> struct
compare;
template< template<typename, typename> class COMPARE, typename P, typename ...Ts > struct
compare<COMPARE, P, std::tuple<Ts...>> {
	using head = typename split<Ts...>::head;
	using tail = typename split<Ts...>::tail;
	using T = typename select<COMPARE,  std::tuple<>, head, tail >::selected;
	static constexpr bool value = COMPARE<P, T>::value;
};
template<template<typename, typename> class COMPARE, typename P, typename T> struct
compare<COMPARE, P, std::tuple<T>> {
	static constexpr bool value = COMPARE<P, T>::value;
};
template<template<typename, typename> class COMPARE, typename P> struct
compare<COMPARE, P, std::tuple<>> {
	static constexpr bool value = true;
};

// select
template<template<typename, typename> class COMPARE, typename ...Ls, typename P, typename ...Ts> struct
select<COMPARE, std::tuple<Ls...>, P, std::tuple<Ts...> > {
private:
	static constexpr bool COMPARE_RESULT = compare<COMPARE, P, std::tuple<Ts...>>::value;
	using head = typename split<Ts...>::head;
	using tail = typename split<Ts...>::tail;

	using selected_false = typename select<COMPARE, std::tuple<Ls..., P>, head, tail>::selected;
	using rest_false = typename select<COMPARE, std::tuple<Ls..., P>, head, tail>::rest;

	using selected_true = P;
	using rest_true = std::tuple<Ls..., Ts...>;
public:
	using selected = typename std::conditional<COMPARE_RESULT, selected_true, selected_false>::type;
	using rest = typename std::conditional<COMPARE_RESULT, rest_true, rest_false>::type;
};
template<template<typename, typename> class COMPARE, typename ...Ls, typename P> struct
select<COMPARE, std::tuple<Ls...>, P, std::tuple<> > {
	using selected = P;
	using rest = std::tuple<Ls...>;
};

} // namespace impl

// select
template< template<typename, typename> class COMPARE, typename ...Ts > struct
select {
	using selected = typename select<COMPARE, std::tuple<Ts...>>::selected;
	using rest = typename select<COMPARE, std::tuple<Ts...>>::rest;
};

template<template<typename, typename> class COMPARE, typename ...Ts> struct
select<COMPARE, std::tuple<Ts...>> {
private:
	using head = typename impl::split<Ts...>::head;
	using tail = typename impl::split<Ts...>::tail;
public:
	using selected = typename impl::select<COMPARE, std::tuple<>, head, tail>::selected;
	using rest = typename impl::select<COMPARE, std::tuple<>, head, tail>::rest;
};

} // namespace typeselect


#endif /* _INTERNAL_TYPESELECT_HPP_ */
