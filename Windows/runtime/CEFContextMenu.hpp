#pragma once

#include "include/cef_context_menu_handler.h"

namespace CEF {

class CEFContextMenuHandler : public CefContextMenuHandler {
   public:
    CEFContextMenuHandler(){};

   public:
    void Continue(int command_id, cef_event_flags_t event_flags) {};

    void Cancel() {};

   private:
    IMPLEMENT_REFCOUNTING(CEFContextMenuHandler);
};

}  // namespace CEF