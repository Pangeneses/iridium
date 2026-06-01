#pragma once

#include "winrt/Ir77X.h"

#include "PostProcessPage.g.h"

namespace winrt::Machine::m_implementation {
struct PostProcessPage : PostProcessPageT<PostProcessPage> {
   public:
    PostProcessPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct PostProcessPage : PostProcessPageT<PostProcessPage, m_implementation::PostProcessPage> {};

}  // namespace winrt::Machine::factory_implementation
