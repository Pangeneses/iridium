#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../interface/IIr77GUID.hpp"

namespace NSIr77RT {
class Ir77GUID : public IIr77GUID {
   public:
    Ir77GUID() {}

    Ir77GUID(std::string const& uuid) { ToGUID(uuid, m_tag_uuid); }

    Ir77GUID(unsigned __int128 const& uuid) { m_tag_uuid = uuid; }

    Ir77GUID(Ir77GUID const& uuid) { m_tag_uuid = uuid.m_tag_uuid; }

   public:
    void operator=(std::string const& uuid) { ToGUID(uuid, m_tag_uuid); }

    void operator=(unsigned __int128 const& uuid) { m_tag_uuid = uuid; }

    void operator=(IIr77GUID const& uuid) { m_tag_uuid = static_cast<Ir77GUID const&>(uuid).m_tag_uuid; }

    bool operator==(std::string const& uuid) const {
        unsigned __int128 hvid{};
        ToGUID(uuid, hvid);
        return m_tag_uuid == hvid;
    }

    bool operator==(unsigned __int128 const& uuid) const { return m_tag_uuid == uuid; }

    bool operator==(IIr77GUID const& uuid) const { return m_tag_uuid == static_cast<Ir77GUID const&>(uuid).m_tag_uuid; }

    bool operator!=(std::string const& uuid) const {
        unsigned __int128 hvid{};
        ToGUID(uuid, hvid);
        return m_tag_uuid != hvid;
    }

    bool operator!=(unsigned __int128 const& uuid) const { return m_tag_uuid != uuid; }

    bool operator!=(IIr77GUID const& uuid) const { return m_tag_uuid != static_cast<Ir77GUID const&>(uuid).m_tag_uuid; }

    bool operator<(std::string const& uuid) const { return *this < Ir77GUID{uuid}; }

    bool operator<(unsigned __int128 const& uuid) const { return m_tag_uuid < uuid; }

    bool operator<(IIr77GUID const& uuid) const { return m_tag_uuid < static_cast<Ir77GUID const&>(uuid).m_tag_uuid; }

    bool operator>(std::string const& uuid) const { return *this > Ir77GUID{uuid}; }

    bool operator>(unsigned __int128 const& uuid) const { return m_tag_uuid > uuid; }

    bool operator>(IIr77GUID const& uuid) const { return m_tag_uuid > static_cast<Ir77GUID const&>(uuid).m_tag_uuid; }

    bool operator>(IIr77GUID const* uuid) const { return uuid != nullptr && m_tag_uuid > static_cast<Ir77GUID const*>(uuid)->m_tag_uuid; }

    unsigned __int128 operator()() const { return m_tag_uuid; }

   public:
    void Generate() {
        static std::mt19937_64 gen = []() {
            std::random_device rd;
            return std::mt19937_64{rd()};
        }();
        static std::uniform_int_distribution<std::uint64_t> dist;
        static std::mutex mtx;

        std::lock_guard<std::mutex> lock(mtx);
        std::uint64_t high = dist(gen);
        std::uint64_t low = dist(gen);
        m_tag_uuid = (static_cast<unsigned __int128>(high) << 64) | low;
    }

    void ToString(unsigned __int128 const& uuid, std::string& str) const {
        std::uint32_t d1 = static_cast<std::uint32_t>(uuid >> 96);
        std::uint16_t d2 = static_cast<std::uint16_t>(uuid >> 80);
        std::uint16_t d3 = static_cast<std::uint16_t>(uuid >> 64);
        std::uint16_t d4 = static_cast<std::uint16_t>(uuid >> 48);
        std::uint64_t d5 = static_cast<std::uint64_t>(uuid & 0x0000FFFFFFFFFFFF);

        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0') << '{' << std::setw(8) << d1 << '-' << std::setw(4) << d2 << '-' << std::setw(4) << d3 << '-'
            << std::setw(4) << d4 << '-' << std::setw(12) << d5 << '}';
        str = oss.str();
    }

    void ToGUID(std::string const& str, unsigned __int128& uuid) const {
        if (str.size() != 38) throw std::invalid_argument{"Invalid GUID."};
        if (str.front() != '{' || str.back() != '}') throw std::invalid_argument{"Invalid GUID."};
        if (str[9] != '-' || str[14] != '-' || str[19] != '-' || str[24] != '-') throw std::invalid_argument{"Invalid GUID."};

        std::string d1s = str.substr(1, 8);
        std::string d2s = str.substr(10, 4);
        std::string d3s = str.substr(15, 4);
        std::string d4s = str.substr(20, 4);
        std::string d5s = str.substr(25, 12);

        for (char c : d1s + d2s + d3s + d4s + d5s) {
            if (std::find(std::begin(Values), std::end(Values), c) == std::end(Values)) throw std::invalid_argument{"Invalid GUID."};
        }

        std::uint64_t d1 = std::stoull(d1s, nullptr, 16);
        std::uint64_t d2 = std::stoull(d2s, nullptr, 16);
        std::uint64_t d3 = std::stoull(d3s, nullptr, 16);
        std::uint64_t d4 = std::stoull(d4s, nullptr, 16);
        std::uint64_t d5 = std::stoull(d5s, nullptr, 16);

        uuid = (static_cast<unsigned __int128>(d1) << 96) | (static_cast<unsigned __int128>(d2) << 80) | (static_cast<unsigned __int128>(d3) << 64) |
               (static_cast<unsigned __int128>(d4) << 48) | static_cast<unsigned __int128>(d5);
    }

   private:
    unsigned __int128 m_tag_uuid{};

    static constexpr char Values[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'A', 'B', 'C', 'D', 'E', 'F'};
};

struct Ir77GUIDHash {
    std::size_t operator()(Ir77GUID const* g) const {
        auto v = g->operator()();
        std::size_t hi = static_cast<std::size_t>(v >> 64);
        std::size_t lo = static_cast<std::size_t>(v);
        return hi ^ (lo * 0x9e3779b97f4a7c15ULL);
    }
};

// by value, to match Ir77GUIDHash -- equal-valued GUIDs at different addresses are the same key
struct Ir77GUIDEqual {
    bool operator()(Ir77GUID const* a, Ir77GUID const* b) const { return a == b || (*a)() == (*b)(); }
};
}  // namespace NSIr77RT