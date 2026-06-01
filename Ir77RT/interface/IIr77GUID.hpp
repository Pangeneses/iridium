#pragma once

#include <string>

namespace NSIr77RT {

    typedef struct IIr77GUID
	{
		IIr77GUID() = default;

		virtual void operator = (std::string const& uuid) = 0;

		virtual void operator = (IIr77GUID const& uuid) = 0;

		virtual void operator = (unsigned __int128 const& uuid) = 0;

		virtual bool operator == (std::string const& uuid) const = 0;

		virtual bool operator == (IIr77GUID const& uuid) const = 0;

		virtual bool operator == (unsigned __int128 const& uuid) const = 0;

		virtual bool operator != (std::string const& uuid) const = 0;

		virtual bool operator != (IIr77GUID const& uuid) const = 0;

		virtual bool operator != (unsigned __int128 const& uuid) const = 0;

		virtual bool operator < (std::string const& uuid) const = 0;

		virtual bool operator < (IIr77GUID const& uuid) const = 0;

		virtual bool operator < (unsigned __int128 const& uuid) const = 0;

		virtual bool operator > (std::string const& uuid) const = 0;

		virtual bool operator > (IIr77GUID const& uuid) const = 0;

		virtual bool operator > (unsigned __int128 const& uuid) const = 0;

		virtual unsigned __int128 operator()() const = 0;

		virtual void Generate() = 0;

		virtual void ToString(unsigned __int128 const& uuid, std::string& str) const = 0;

		virtual void ToGUID(std::string const& str, unsigned __int128& uuid) const = 0;

		virtual ~IIr77GUID() = default;

	}* PIr77GUID;
}