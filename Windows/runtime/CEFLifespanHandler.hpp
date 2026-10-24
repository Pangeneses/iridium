#pragma once

#include "include/cef_life_span_handler.h"

namespace NSIr77CEF {

class CEFLifeSpanHandler : public CefLifeSpanHandler {
   public:
    CEFLifeSpanHandler() {};

   private:
    IMPLEMENT_REFCOUNTING(CEFLifeSpanHandler);
};

}  // namespace NSIr77CEF