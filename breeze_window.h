// Breeze browser window: tabs + toolbar built with the CEF Views framework.
#pragma once

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_parser.h"
#include "include/cef_task.h"
#include "include/views/cef_box_layout.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_browser_view_delegate.h"
#include "include/views/cef_button_delegate.h"
#include "include/views/cef_label_button.h"
#include "include/views/cef_panel.h"
#include "include/views/cef_textfield.h"
#include "include/views/cef_textfield_delegate.h"
#include "include/views/cef_window.h"
#include "include/views/cef_window_delegate.h"
#include "include/wrapper/cef_closure_task.h"

namespace breeze {

constexpr char kHomeUrl[] = "https://www.google.com/";
constexpr char kSearchUrl[] = "https://www.google.com/search?q=";

// ボタンID
enum ButtonId {
  kBtnBack = 1,
  kBtnForward,
  kBtnReload,
  kBtnHome,
  kBtnNewTab,
  kFirstTabId = 1000,     // タブのID: 1000, 1001, ...
  kCloseOffset = 100000,  // 閉じるボタン: タブID + kCloseOffset
};

// ショートカットID
enum Accel {
  kAccNewTab = 1,
  kAccCloseTab,
  kAccFocusAddress,
  kAccReload,
  kAccDevTools,
  kAccBack,
  kAccForward,
  kAccNextTab,
};

class BreezeWindow : public CefWindowDelegate,
                     public CefBrowserViewDelegate,
                     public CefButtonDelegate,
                     public CefTextfieldDelegate,
                     public CefClient,
                     public CefDisplayHandler,
                     public CefLoadHandler,
                     public CefLifeSpanHandler,
                     public CefDownloadHandler {
 public:
  BreezeWindow() = default;

  // ===================== CefWindowDelegate =====================
  void OnWindowCreated(CefRefPtr<CefWindow> window) override {
    window_ = window;
    window_->SetTitle("Breeze");

    // 縦: タブバー / ツールバー / コンテンツ
    CefBoxLayoutSettings col;
    col.horizontal = false;
    CefRefPtr<CefBoxLayout> layout = window_->SetToBoxLayout(col);

    CefBoxLayoutSettings row;
    row.horizontal = true;
    row.between_child_spacing = 4;
    row.inside_border_insets = CefInsets(4, 6, 4, 6);
    row.cross_axis_alignment = CEF_AXIS_ALIGNMENT_STRETCH;

    tabbar_ = CefPanel::CreatePanel(nullptr);
    tabbar_->SetToBoxLayout(row);

    toolbar_ = CefPanel::CreatePanel(nullptr);
    CefRefPtr<CefBoxLayout> tool_layout = toolbar_->SetToBoxLayout(row);

    content_ = CefPanel::CreatePanel(nullptr);
    content_->SetToFillLayout();

    back_ = MakeButton("<", kBtnBack);
    forward_ = MakeButton(">", kBtnForward);
    reload_ = MakeButton("Reload", kBtnReload);
    home_ = MakeButton("Home", kBtnHome);
    newtab_ = MakeButton("+", kBtnNewTab);

    address_ = CefTextfield::CreateTextfield(this);
    address_->SetPlaceholderText("Search or enter address");

    toolbar_->AddChildView(back_);
    toolbar_->AddChildView(forward_);
    toolbar_->AddChildView(reload_);
    toolbar_->AddChildView(home_);
    toolbar_->AddChildView(address_);
    tool_layout->SetFlexForView(address_, 1);

    tabbar_->AddChildView(newtab_);

    window_->AddChildView(tabbar_);
    window_->AddChildView(toolbar_);
    window_->AddChildView(content_);
    layout->SetFlexForView(content_, 1);

    // ショートカットキー
    window_->SetAccelerator(kAccNewTab, 'T', false, true, false);
    window_->SetAccelerator(kAccCloseTab, 'W', false, true, false);
    window_->SetAccelerator(kAccFocusAddress, 'L', false, true, false);
    window_->SetAccelerator(kAccReload, 'R', false, true, false);
    window_->SetAccelerator(kAccReload, VK_F5, false, false, false);
    window_->SetAccelerator(kAccDevTools, VK_F12, false, false, false);
    window_->SetAccelerator(kAccBack, VK_LEFT, false, false, true);
    window_->SetAccelerator(kAccForward, VK_RIGHT, false, false, true);
    window_->SetAccelerator(kAccNextTab, VK_TAB, false, true, false);

    window_->Show();
    AddTab(kHomeUrl);
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow>) override {
    tabs_.clear();
    tabbar_ = toolbar_ = content_ = nullptr;
    back_ = forward_ = reload_ = home_ = newtab_ = nullptr;
    address_ = nullptr;
    window_ = nullptr;
    CefQuitMessageLoop();
  }

  bool CanClose(CefRefPtr<CefWindow>) override {
    for (auto& t : tabs_) {
      CefRefPtr<CefBrowser> b = t.view->GetBrowser();
      if (b && !b->GetHost()->TryCloseBrowser()) return false;
    }
    return true;
  }

  CefRect GetInitialBounds(CefRefPtr<CefWindow>) override {
    return CefRect(100, 100, 1280, 800);
  }

  bool OnAccelerator(CefRefPtr<CefWindow>, int command_id) override {
    CefRefPtr<CefBrowser> b = ActiveBrowser();
    switch (command_id) {
      case kAccNewTab:
        AddTab(kHomeUrl);
        address_->RequestFocus();
        return true;
      case kAccCloseTab:
        if (active_ >= 0) CloseTab(tabs_[active_].id);
        return true;
      case kAccFocusAddress:
        address_->RequestFocus();
        address_->SelectAll(false);
        return true;
      case kAccReload:
        if (b) b->Reload();
        return true;
      case kAccDevTools:
        if (b) {
          b->GetHost()->ShowDevTools(CefWindowInfo(), nullptr,
                                     CefBrowserSettings(), CefPoint());
        }
        return true;
      case kAccBack:
        if (b && b->CanGoBack()) b->GoBack();
        return true;
      case kAccForward:
        if (b && b->CanGoForward()) b->GoForward();
        return true;
      case kAccNextTab:
        if (!tabs_.empty()) SelectTab((active_ + 1) % static_cast<int>(tabs_.size()));
        return true;
    }
    return false;
  }

  // ===================== CefBrowserViewDelegate =====================
  cef_runtime_style_t GetBrowserRuntimeStyle() override {
    // 自前のUI(タブ/ツールバー)を使うので、Chrome標準UIではなくAlloyスタイルを使う
    return CEF_RUNTIME_STYLE_ALLOY;
  }

  // ===================== CefButtonDelegate =====================
  void OnButtonPressed(CefRefPtr<CefButton> button) override {
    const int id = button->GetID();
    CefRefPtr<CefBrowser> b = ActiveBrowser();

    if (id >= kFirstTabId && id < kCloseOffset) {
      const int idx = IndexOfId(id);
      if (idx >= 0) SelectTab(idx);
    } else if (id >= kCloseOffset) {
      // ボタン自身の処理中に破棄しないよう、タスクに回す
      CefPostTask(TID_UI, base::BindOnce(&BreezeWindow::CloseTab, this,
                                         id - kCloseOffset));
    } else if (id == kBtnBack) {
      if (b) b->GoBack();
    } else if (id == kBtnForward) {
      if (b) b->GoForward();
    } else if (id == kBtnReload) {
      if (b) {
        if (active_ >= 0 && tabs_[active_].loading) b->StopLoad();
        else b->Reload();
      }
    } else if (id == kBtnHome) {
      if (b) b->GetMainFrame()->LoadURL(kHomeUrl);
    } else if (id == kBtnNewTab) {
      AddTab(kHomeUrl);
      address_->RequestFocus();
    }
  }

  // ===================== CefTextfieldDelegate =====================
  bool OnKeyEvent(CefRefPtr<CefTextfield>, const CefKeyEvent& event) override {
    if (event.type == KEYEVENT_RAWKEYDOWN && event.windows_key_code == VK_RETURN) {
      CefRefPtr<CefBrowser> b = ActiveBrowser();
      if (b) {
        b->GetMainFrame()->LoadURL(ToUrl(address_->GetText().ToString()));
        tabs_[active_].view->RequestFocus();
      }
      return true;
    }
    return false;
  }

  // ===================== CefClient =====================
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }

  // ===================== CefDisplayHandler =====================
  void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override {
    const int idx = IndexOfBrowser(browser);
    if (idx < 0) return;
    std::wstring w = title.ToWString();
    if (w.empty()) w = L"New Tab";
    tabs_[idx].full_title = w;
    if (w.size() > 20) w = w.substr(0, 19) + L"...";
    tabs_[idx].title_btn->SetText(CefString(w));
    if (idx == active_) UpdateWindowTitle();
  }

  void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                       const CefString& url) override {
    if (!frame->IsMain()) return;
    const int idx = IndexOfBrowser(browser);
    if (idx < 0) return;
    tabs_[idx].url = url.ToString();
    if (idx == active_ && !address_->HasFocus()) address_->SetText(url);
  }

  // ===================== CefLoadHandler =====================
  void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool is_loading,
                            bool can_go_back, bool can_go_forward) override {
    const int idx = IndexOfBrowser(browser);
    if (idx < 0) return;
    tabs_[idx].loading = is_loading;
    tabs_[idx].can_back = can_go_back;
    tabs_[idx].can_forward = can_go_forward;
    if (idx == active_) UpdateToolbar();
  }

  // ===================== CefLifeSpanHandler =====================
  // window.open / target=_blank は新しいタブで開く
  bool OnBeforePopup(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, int,
                     const CefString& target_url, const CefString&,
                     cef_window_open_disposition_t, bool, const CefPopupFeatures&,
                     CefWindowInfo&, CefRefPtr<CefClient>&, CefBrowserSettings&,
                     CefRefPtr<CefDictionaryValue>&, bool*) override {
    CefPostTask(TID_UI, base::BindOnce(&BreezeWindow::AddTabTask, this,
                                       target_url.ToString()));
    return true;  // 既定のポップアップ生成はキャンセル
  }

  // ===================== CefDownloadHandler =====================
  void OnBeforeDownload(CefRefPtr<CefBrowser>, CefRefPtr<CefDownloadItem>,
                        const CefString&,
                        CefRefPtr<CefBeforeDownloadCallback> callback) override {
    callback->Continue(CefString(), true);  // 保存ダイアログを表示
  }

 private:
  struct Tab {
    int id = 0;
    CefRefPtr<CefBrowserView> view;
    CefRefPtr<CefLabelButton> title_btn;
    CefRefPtr<CefLabelButton> close_btn;
    std::string url;
    std::wstring full_title = L"New Tab";
    bool loading = false;
    bool can_back = false;
    bool can_forward = false;
  };

  CefRefPtr<CefLabelButton> MakeButton(const char* text, int id) {
    CefRefPtr<CefLabelButton> b = CefLabelButton::CreateLabelButton(this, text);
    b->SetID(id);
    return b;
  }

  CefRefPtr<CefBrowser> ActiveBrowser() {
    if (active_ < 0 || active_ >= static_cast<int>(tabs_.size())) return nullptr;
    return tabs_[active_].view->GetBrowser();
  }

  int IndexOfId(int id) const {
    for (size_t i = 0; i < tabs_.size(); ++i)
      if (tabs_[i].id == id) return static_cast<int>(i);
    return -1;
  }

  int IndexOfBrowser(CefRefPtr<CefBrowser> browser) const {
    for (size_t i = 0; i < tabs_.size(); ++i) {
      CefRefPtr<CefBrowser> b = tabs_[i].view->GetBrowser();
      if (b && b->IsSame(browser)) return static_cast<int>(i);
    }
    return -1;
  }

  // 入力欄の文字列 → URL(URLならそのまま、単語なら検索)
  static std::string ToUrl(std::string in) {
    const char* ws = " \t\r\n";
    size_t b = in.find_first_not_of(ws);
    if (b == std::string::npos) return kHomeUrl;
    size_t e = in.find_last_not_of(ws);
    in = in.substr(b, e - b + 1);

    auto starts = [&](const char* p) { return in.rfind(p, 0) == 0; };
    if (starts("http://") || starts("https://") || starts("file://") ||
        starts("about:") || starts("chrome://") || starts("data:"))
      return in;
    if (starts("localhost") || starts("127.0.0.1")) return "http://" + in;

    const bool has_space = in.find(' ') != std::string::npos;
    if (!has_space && in.find('.') != std::string::npos) return "https://" + in;

    return std::string(kSearchUrl) + CefURIEncode(in, true).ToString();
  }

  void AddTabTask(std::string url) { AddTab(url); }

  void AddTab(const std::string& url) {
    Tab t;
    t.id = next_id_++;
    t.url = url;

    CefBrowserSettings settings;
    t.view = CefBrowserView::CreateBrowserView(this, url, settings, nullptr,
                                               nullptr, this);

    t.title_btn = MakeButton("New Tab", t.id);
    t.close_btn = MakeButton("x", t.id + kCloseOffset);

    const size_t pos = tabs_.size() * 2;  // [+] ボタンの手前に挿入
    tabbar_->AddChildViewAt(t.title_btn, static_cast<int>(pos));
    tabbar_->AddChildViewAt(t.close_btn, static_cast<int>(pos + 1));

    t.view->SetVisible(false);
    content_->AddChildView(t.view);

    tabs_.push_back(t);
    SelectTab(static_cast<int>(tabs_.size()) - 1);
  }

  void SelectTab(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return;
    active_ = index;
    for (size_t i = 0; i < tabs_.size(); ++i) {
      const bool on = static_cast<int>(i) == index;
      tabs_[i].view->SetVisible(on);
      tabs_[i].title_btn->SetBackgroundColor(on ? CefColorSetARGB(255, 205, 225, 255)
                                                : CefColorSetARGB(255, 240, 240, 240));
      tabs_[i].close_btn->SetBackgroundColor(on ? CefColorSetARGB(255, 205, 225, 255)
                                                : CefColorSetARGB(255, 240, 240, 240));
    }
    address_->SetText(tabs_[index].url);
    UpdateToolbar();
    UpdateWindowTitle();
    tabs_[index].view->RequestFocus();
  }

  void CloseTab(int id) {
    const int idx = IndexOfId(id);
    if (idx < 0) return;
    Tab t = tabs_[idx];
    tabs_.erase(tabs_.begin() + idx);

    tabbar_->RemoveChildView(t.title_btn);
    tabbar_->RemoveChildView(t.close_btn);
    content_->RemoveChildView(t.view);
    if (CefRefPtr<CefBrowser> b = t.view->GetBrowser()) b->GetHost()->CloseBrowser(true);

    if (tabs_.empty()) {
      active_ = -1;
      if (window_) window_->Close();
      return;
    }
    SelectTab(std::min(idx, static_cast<int>(tabs_.size()) - 1));
  }

  void UpdateToolbar() {
    if (active_ < 0) return;
    const Tab& t = tabs_[active_];
    back_->SetEnabled(t.can_back);
    forward_->SetEnabled(t.can_forward);
    reload_->SetText(t.loading ? "Stop" : "Reload");
  }

  void UpdateWindowTitle() {
    if (!window_ || active_ < 0) return;
    window_->SetTitle(CefString(tabs_[active_].full_title + L" - Breeze"));
  }

  CefRefPtr<CefWindow> window_;
  CefRefPtr<CefPanel> tabbar_, toolbar_, content_;
  CefRefPtr<CefLabelButton> back_, forward_, reload_, home_, newtab_;
  CefRefPtr<CefTextfield> address_;
  std::vector<Tab> tabs_;
  int active_ = -1;
  int next_id_ = kFirstTabId;

  IMPLEMENT_REFCOUNTING(BreezeWindow);
  DISALLOW_COPY_AND_ASSIGN(BreezeWindow);
};

}  // namespace breeze
