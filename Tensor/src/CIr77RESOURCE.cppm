#pragma once

#include <type_traits>

#include <stdexcept>

export module CIr77TENSOR:CIr77RESOURCE;

import :CIr77FORWARD;

export namespace CIr77TENSOR {
template <typename T>
concept assert_char = std::is_same<T, char>::value || std::is_same<T, unsigned char>::value;

template <typename T>
concept assert_char_array = std::is_same<T, char* const>::value || std::is_same<T, unsigned char* const>::value;

template <typename T>
concept assert_signed_int =
    std::is_same<T, std::int8_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value;

template <typename T>
concept assert_unsigned_int =
    std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value;

template <typename T>
concept assert_integer =
    std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value ||
    std::is_same<T, std::int8_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value;

template <typename T>
concept assert_size_t = std::is_same<T, std::size_t>::value;

template <typename T>
concept assert_precision = std::is_same<T, float>::value || std::is_same<T, double>::value || std::is_same<T, long double>::value;

}  // namespace CIr77TENSOR