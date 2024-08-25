#pragma once

#include <cstdint>
#include <cstddef>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using isize = std::ptrdiff_t;
using usize = std::size_t;

template<class... Ts>
struct multi_visitor : Ts... { using Ts::operator()...; };

