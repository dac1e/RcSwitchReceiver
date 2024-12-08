/*
  RcSwitchReceiver - Arduino libary for remote control receiver Copyright (c)
  2024 Wolfgang Schmieder.  All right reserved.

  Contributors:
  - Wolfgang Schmieder

  Project home: https://github.com/dac1e/RcSwitchReceiver/

  This library is free software; you can redistribute it and/or modify it
  the terms of the GNU Lesser General Public License as under published
  by the Free Software Foundation; either version 3.0 of the License,
  or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

#ifndef RCSWITCH_RECEIVER_INTERNAL_TYPESELECT_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_TYPESELECT_HPP_

/**
 * These are helper classes to sort the row entries of RxProtocolTable at compile time.
 */

namespace typeselect {

template<typename ...Ts> struct tuple;

namespace impl {

// conditional
template<bool B, class T, class F>
struct conditional { using type = T; };

template<class T, class F>
struct conditional<false, T, F> { using type = F; };

// split
template<typename L, typename ...Ts> struct
split {
	using head = L;
	using tail = tuple<Ts...>;
};

// select declaration
template<template<typename, typename> class COMPARE, typename L, typename P, typename T> struct
select;

// compare
template<template<typename, typename> class COMPARE, typename P, typename T> struct
compare;
template< template<typename, typename> class COMPARE, typename P, typename ...Ts > struct
compare<COMPARE, P, tuple<Ts...>> {
	using head = typename split<Ts...>::head;
	using tail = typename split<Ts...>::tail;
	using T = typename select<COMPARE,  tuple<>, head, tail >::selected;
	static constexpr bool value = COMPARE<P, T>::value;
};
template<template<typename, typename> class COMPARE, typename P, typename T> struct
compare<COMPARE, P, tuple<T>> {
	static constexpr bool value = COMPARE<P, T>::value;
};
template<template<typename, typename> class COMPARE, typename P> struct
compare<COMPARE, P, tuple<>> {
	static constexpr bool value = true;
};

// select
template<template<typename, typename> class COMPARE, typename ...Ls, typename P, typename ...Ts> struct
select<COMPARE, tuple<Ls...>, P, tuple<Ts...> > {
private:
	static constexpr bool COMPARE_RESULT = compare<COMPARE, P, tuple<Ts...>>::value;
	using head = typename split<Ts...>::head;
	using tail = typename split<Ts...>::tail;

	using selected_false = typename select<COMPARE, tuple<Ls..., P>, head, tail>::selected;
	using rest_false = typename select<COMPARE, tuple<Ls..., P>, head, tail>::rest;

	using selected_true = P;
	using rest_true = tuple<Ls..., Ts...>;
public:
	using selected = typename conditional<COMPARE_RESULT, selected_true, selected_false>::type;
	using rest = typename conditional<COMPARE_RESULT, rest_true, rest_false>::type;
};
template<template<typename, typename> class COMPARE, typename ...Ls, typename P> struct
select<COMPARE, tuple<Ls...>, P, tuple<> > {
	using selected = P;
	using rest = tuple<Ls...>;
};

} // namespace impl

// select
template< template<typename, typename> class COMPARE, typename ...Ts > struct
select {
	using selected = typename select<COMPARE, tuple<Ts...>>::selected;
	using rest = typename select<COMPARE, tuple<Ts...>>::rest;
};

template<template<typename, typename> class COMPARE, typename ...Ts> struct
select<COMPARE, tuple<Ts...>> {
private:
	using head = typename impl::split<Ts...>::head;
	using tail = typename impl::split<Ts...>::tail;
public:
	using selected = typename impl::select<COMPARE, tuple<>, head, tail>::selected;
	using rest = typename impl::select<COMPARE, tuple<>, head, tail>::rest;
};

} // namespace typeselect


#endif /* RCSWITCH_RECEIVER_INTERNAL_TYPESELECT_HPP_ */
