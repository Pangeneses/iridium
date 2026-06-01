#pragma once

#include "winrt/Ir77X.h"

#include "EnterprisePage.g.h"

namespace winrt::Machine::m_implementation {
struct EnterprisePage : EnterprisePageT<EnterprisePage> {
   public:
    EnterprisePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct EnterprisePage : EnterprisePageT<EnterprisePage, m_implementation::EnterprisePage> {};

}  // namespace winrt::Machine::factory_implementation
