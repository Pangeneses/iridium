#pragma once

#include "winrt/Ir77X.h"

#include "ApplicationPage.g.h"

namespace winrt::Machine::m_implementation {
struct ApplicationPage : ApplicationPageT<ApplicationPage> {
   public:
    ApplicationPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ApplicationPage : ApplicationPageT<ApplicationPage, m_implementation::ApplicationPage> {};

}  // namespace winrt::Machine::factory_implementation
