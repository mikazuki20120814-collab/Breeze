// Breeze - CEF based web browser
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>

#include <string>

#include "include/cef_app.h"
#include "include/views/cef_window.h"

#include "breeze_window.h"

class BreezeApp : public CefApp, public CefBrowserProcessHandler {
 public:
  BreezeApp() = default;

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnContextInitialized() override {
    CefWindow::CreateTopLevelWindow(new breeze::BreezeWindow());
  }

 private:
  IMPLEMENT_REFCOUNTING(BreezeApp);
  DISALLOW_COPY_AND_ASSIGN(BreezeApp);
};

static std::wstring UserDataDir() {
  std::wstring dir;
  PWSTR path = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
    dir = path;
    CoTaskMemFree(path);
  } else {
    dir = L".";
  }
  dir += L"\\Breeze\\User Data";
  return dir;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
  CefEnableHighDPISupport();

  CefMainArgs main_args(hInstance);
  CefRefPtr<BreezeApp> app(new BreezeApp);

  // サブプロセス(レンダラー/GPUなど)ならここで処理されて終了する
  int exit_code = CefExecuteProcess(main_args, app, nullptr);
  if (exit_code >= 0) return exit_code;

  CefSettings settings;
  settings.no_sandbox = true;
  settings.log_severity = LOGSEVERITY_WARNING;

  // プロファイル(Cookie・キャッシュ)の保存先
  const std::wstring data_dir = UserDataDir();
  CefString(&settings.root_cache_path) = data_dir;
  CefString(&settings.cache_path) = data_dir;
  CefString(&settings.accept_language_list) = "ja,en-US,en";

  if (!CefInitialize(main_args, settings, app, nullptr)) return 1;

  CefRunMessageLoop();
  CefShutdown();
  return 0;
}
