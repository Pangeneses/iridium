#pragma once

#include "winrt/Ir77X.h"

#include "ProjectIndexPage.g.h"

namespace winrt::Machine::m_implementation {
struct ProjectIndexPage : ProjectIndexPageT<ProjectIndexPage> {
   public:
    ProjectIndexPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ProjectIndexPage : ProjectIndexPageT<ProjectIndexPage, m_implementation::ProjectIndexPage> {};
}  // namespace winrt::Machine::factory_implementation
