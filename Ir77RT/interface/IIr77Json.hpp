#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "../types/Ir77Types.hpp"

namespace NSIr77RT {

    typedef struct IIr77Json
	{
		IIr77Json() = default;

		virtual IIr77Json& operator [] (std::string_view const& key) = 0;

		virtual IIr77Json const* Find(std::string_view const& key) const = 0;

		virtual bool Has(std::string_view const& key) const = 0;

		virtual void Remove(std::string_view const& key) = 0;

		virtual IIr77Json& Push(IIr77Json const& value) = 0;

		virtual void Merge(IIr77Json const& other) = 0;

		virtual Ir77JsonKind Kind() const = 0;

		virtual bool IsNull() const = 0;

		virtual std::size_t Size() const = 0;

		virtual void Write(std::string& out) const = 0;

		virtual std::string Write() const = 0;

		virtual ~IIr77Json() = default;

	}* PIr77Json;
}
