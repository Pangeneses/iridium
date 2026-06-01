#pragma once

#include "include/cef_request_handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_request.h"
#include "include/cef_auth_callback.h"
#include "include/cef_resource_request_handler.h"
#include "include/cef_ssl_info.h"
#include "include/cef_callback.h"

namespace CEF {

class CEFRequestHandler : public CefRequestHandler {
   public:
    bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) override {
        return false;
    }

    bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, WindowOpenDisposition target_disposition,
                          bool user_gesture) override {
        return false;
    }

    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                                                                   bool is_navigation, bool is_download, const CefString& request_initiator,
                                                                   bool& disable_default_handling) override {
        return nullptr;
    }

    bool GetAuthCredentials(CefRefPtr<CefBrowser> browser, const CefString& origin_url, bool isProxy, const CefString& host, int port, const CefString& realm,
                            const CefString& scheme, CefRefPtr<CefAuthCallback> callback) override {
        return false;
    }

    bool OnCertificateError(CefRefPtr<CefBrowser> browser, cef_errorcode_t cert_error, const CefString& request_url, CefRefPtr<CefSSLInfo> ssl_info,
                            CefRefPtr<CefCallback> callback) override {
        return false;
    }

    bool OnSelectClientCertificate(CefRefPtr<CefBrowser> browser, bool isProxy, const CefString& host, int port, const X509CertificateList& certificates,
                                   CefRefPtr<CefSelectClientCertificateCallback> callback) override {
        return false;
    }

    void OnRenderViewReady(CefRefPtr<CefBrowser> browser) override {}

    bool OnRenderProcessUnresponsive(CefRefPtr<CefBrowser> browser, CefRefPtr<CefUnresponsiveProcessCallback> callback) override { return false; }

    void OnRenderProcessResponsive(CefRefPtr<CefBrowser> browser) override {}

    void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status, int error_code, const CefString& error_string) override {}

    void OnDocumentAvailableInMainFrame(CefRefPtr<CefBrowser> browser) override {}

   private:
    CEFRequestHandler() = default;

    IMPLEMENT_REFCOUNTING(CEFRequestHandler);
};

}  // namespace CEF
