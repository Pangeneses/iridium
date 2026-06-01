#pragma once

#include "winrt/Ir77X.h"

#include "AccountPage.g.h"

namespace winrt::Machine::m_implementation {
struct AccountPage : AccountPageT<AccountPage> {
   public:
    AccountPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct AccountPage : AccountPageT<AccountPage, m_implementation::AccountPage> {};
}  // namespace winrt::Machine::factory_implementation
