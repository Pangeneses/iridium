#pragma once

#include "winrt/Ir77X.h"

#include "PromptPage.g.h"

namespace winrt::Machine::m_implementation {
struct PromptPage : PromptPageT<PromptPage> {
   public:
    PromptPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct PromptPage : PromptPageT<PromptPage, m_implementation::PromptPage> {};
}  // namespace winrt::Machine::factory_implementation
