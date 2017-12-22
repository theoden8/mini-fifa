#pragma once

#include <type_traits>
#include <utility>
#include <tuple>
#include <functional>

namespace Tuple {
template <
    size_t Index = 0, // start iteration at 0 index
    typename TTuple,  // the tuple type
    size_t Size = std::tuple_size<std::remove_reference_t<TTuple>>::value,
    typename TCallable, // the callable to bo invoked for each tuple item
    typename... TArgs   // other arguments to be passed to the callable
>
void for_each(TTuple&& tpl, TCallable&& callable, TArgs&&... args) {
  if constexpr(Index < Size) {
    std::invoke(callable, args..., std::get<Index>(tpl));
    if constexpr(Index + 1 < Size) {
      for_each<Index + 1>(
        std::forward<TTuple>(tpl),
        std::forward<TCallable>(callable),
        std::forward<TArgs>(args)...
      );
    }
  }
}
}
