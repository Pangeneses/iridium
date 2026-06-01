#pragma once

#include "include/cef_life_span_handler.h"

namespace CEF {

class CEFLifeSpanHandler : public CefLifeSpanHandler {
   public:
    CEFLifeSpanHandler(){};

   private:
    IMPLEMENT_REFCOUNTING(CEFLifeSpanHandler);
};

}  // namespace CEF