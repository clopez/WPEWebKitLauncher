#include <WPE/WebKit.h>
#include <WPE/WebKit/WKCookieManagerSoup.h>
#include <WPE/WebKit/WKUserMediaPermissionCheck.h>

#include <cstdio>
#include <glib.h>
#include <initializer_list>

WKPageNavigationClientV0 s_navigationClient = {
    { 0, nullptr },
    // decidePolicyForNavigationAction
    [](WKPageRef, WKNavigationActionRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
        WKFramePolicyListenerUse(listener);
    },
    // decidePolicyForNavigationResponse
    [](WKPageRef, WKNavigationResponseRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
        WKFramePolicyListenerUse(listener);
    },
    nullptr, // decidePolicyForPluginLoad
    nullptr, // didStartProvisionalNavigation
    nullptr, // didReceiveServerRedirectForProvisionalNavigation
    nullptr, // didFailProvisionalNavigation
    nullptr, // didCommitNavigation
    nullptr, // didFinishNavigation
    nullptr, // didFailNavigation
    nullptr, // didFailProvisionalLoadInSubframe
    // didFinishDocumentLoad
    [](WKPageRef page, WKNavigationRef, WKTypeRef, const void*) {
        WKStringRef messageName = WKStringCreateWithUTF8CString("Hello");
        WKMutableArrayRef messageBody = WKMutableArrayCreate();

        for (auto& item : { "Test1", "Test2", "Test3" }) {
            WKStringRef itemString = WKStringCreateWithUTF8CString(item);
            WKArrayAppendItem(messageBody, itemString);
            WKRelease(itemString);
        }

        fprintf(stderr, "[WPELauncher] Hello InjectedBundle ...\n");
        WKPagePostMessageToInjectedBundle(page, messageName, messageBody);
        WKRelease(messageBody);
        WKRelease(messageName);
    },
    nullptr, // didSameDocumentNavigation
    nullptr, // renderingProgressDidChange
    nullptr, // canAuthenticateAgainstProtectionSpace
    nullptr, // didReceiveAuthenticationChallenge
    nullptr, // webProcessDidCrash
    nullptr, // copyWebCryptoMasterKey
    nullptr, // didBeginNavigationGesture
    nullptr, // willEndNavigationGesture
    nullptr, // didEndNavigationGesture
    nullptr, // didRemoveNavigationGestureSnapshot
};

WKViewClientV0 s_viewClient = {
    { 0, nullptr },
    // frameDisplayed
    [](WKViewRef, const void*) {
        static unsigned s_frameCount = 0;
        static gint64 lastDumpTime = g_get_monotonic_time();

        if (!g_getenv("WPE_DISPLAY_FPS"))
          return;

        ++s_frameCount;
        gint64 time = g_get_monotonic_time();
        if (time - lastDumpTime >= 5 * G_USEC_PER_SEC) {
            fprintf(stderr, "[WPELauncher] %.2f FPS\n",
                s_frameCount * G_USEC_PER_SEC * 1.0 / (time - lastDumpTime));
            s_frameCount = 0;
            lastDumpTime = time;
        }
    },
};

WKPageUIClientV8 s_pageUiClient = {
    { 8, nullptr },

    // Version 0.
    nullptr, // createNewPage_deprecatedForUseWithV0
    nullptr, // showPage
    nullptr, // close
    nullptr, // takeFocus
    nullptr, // focus
    nullptr, // unfocus
    nullptr, // runJavaScriptAlert_deprecatedForUseWithV0
    nullptr, // runJavaScriptConfirm_deprecatedForUseWithV0
    nullptr, // runJavaScriptPrompt_deprecatedForUseWithV0
    nullptr, // setStatusText
    nullptr, // mouseDidMoveOverElement_deprecatedForUseWithV0
    nullptr, // missingPluginButtonClicked_deprecatedForUseWithV0
    nullptr, // didNotHandleKeyEvent
    nullptr, // didNotHandleWheelEvent
    nullptr, // toolbarsAreVisible
    nullptr, // setToolbarsAreVisible
    nullptr, // menuBarIsVisible
    nullptr, // setMenuBarIsVisible
    nullptr, // statusBarIsVisible
    nullptr, // setStatusBarIsVisible
    nullptr, // isResizable
    nullptr, // setIsResizable
    nullptr, // getWindowFrame
    nullptr, // setWindowFrame
    nullptr, // runBeforeUnloadConfirmPanel_deprecatedForUseWithV6
    nullptr, // didDraw
    nullptr, // pageDidScroll
    nullptr, // exceededDatabaseQuota
    nullptr, // runOpenPanel
    nullptr, // decidePolicyForGeolocationPermissionRequest
    nullptr, // headerHeight
    nullptr, // footerHeight
    nullptr, // drawHeader
    nullptr, // drawFooter
    nullptr, // printFrame
    nullptr, // runModal
    nullptr, // unused1 // Used to be didCompleteRubberBandForMainFrame
    nullptr, // saveDataToFileInDownloadsFolder
    nullptr, // shouldInterruptJavaScript_unavailable

    // Version 1.
    nullptr, // createNewPage_deprecatedForUseWithV1
    nullptr, // mouseDidMoveOverElement
    nullptr, // decidePolicyForNotificationPermissionRequest
    nullptr, // unavailablePluginButtonClicked_deprecatedForUseWithV1

    // Version 2.
    nullptr, // showColorPicker
    nullptr, // hideColorPicker
    nullptr, // unavailablePluginButtonClicked

    // Version 3.
    nullptr, // pinnedStateDidChange

    // Version 4.
    nullptr, // unused2 // Used to be didBeginTrackingPotentialLongMousePress.
    nullptr, // unused3 // Used to be didRecognizeLongMousePress.
    nullptr, // unused4 // Used to be didCancelTrackingPotentialLongMousePress.
    nullptr, // isPlayingAudioDidChange

    // Version 5.
    nullptr, // decidePolicyForUserMediaPermissionRequest
    nullptr, // didClickAutoFillButton
    nullptr, // runJavaScriptAlert_deprecatedForUseWithV5
    nullptr, // runJavaScriptConfirm_deprecatedForUseWithV5
    nullptr, // runJavaScriptPrompt_deprecatedForUseWithV5
    nullptr, // mediaSessionMetadataDidChange

    // Version 6.
    nullptr, // createNewPage
    nullptr, // runJavaScriptAlert
    nullptr, // runJavaScriptConfirm
    nullptr, // runJavaScriptPrompt
    // checkUserMediaPermissionForOrigin
    [](WKPageRef, WKFrameRef, WKSecurityOriginRef, WKSecurityOriginRef, WKUserMediaPermissionCheckRef deviceRequest, const void*) {
        // TODO: Do not just accept all requests.
        auto hashSalt = WKStringCreateWithUTF8CString("dummySalt");
        WKUserMediaPermissionCheckSetUserMediaAccessInfo(hashSalt, true);
        WKRelease(hashSalt);
    },

    // Version 7.
    nullptr, // runBeforeUnloadConfirmPanel
    nullptr, // fullscreenMayReturnToInline

    // Version 8.
    nullptr, // willAddDetailedMessageToConsole
};

int main(int argc, char* argv[])
{
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

    auto contextConfiguration = WKContextConfigurationCreate();
    auto injectedBundlePath = WKStringCreateWithUTF8CString("/usr/lib/libWPEInjectedBundle.so");
    WKContextConfigurationSetInjectedBundlePath(contextConfiguration, injectedBundlePath);

    gchar *wpeStoragePath = g_build_filename(g_get_user_cache_dir(), "wpe", "local-storage", nullptr);
    g_mkdir_with_parents(wpeStoragePath, 0700);
    auto storageDirectory = WKStringCreateWithUTF8CString(wpeStoragePath);
    g_free(wpeStoragePath);
    WKContextConfigurationSetLocalStorageDirectory(contextConfiguration, storageDirectory);

    gchar *wpeDiskCachePath = g_build_filename(g_get_user_cache_dir(), "wpe", "disk-cache", nullptr);
    g_mkdir_with_parents(wpeDiskCachePath, 0700);
    auto diskCacheDirectory = WKStringCreateWithUTF8CString(wpeDiskCachePath);
    g_free(wpeDiskCachePath);
    WKContextConfigurationSetDiskCacheDirectory(contextConfiguration, diskCacheDirectory);

    WKRelease(injectedBundlePath);

    WKContextRef context = WKContextCreateWithConfiguration(contextConfiguration);
    WKRelease(contextConfiguration);

    auto pageGroupIdentifier = WKStringCreateWithUTF8CString("WPEPageGroup");
    auto pageGroup = WKPageGroupCreateWithIdentifier(pageGroupIdentifier);
    WKRelease(pageGroupIdentifier);

    auto preferences = WKPreferencesCreate();
    // Allow mixed content.
    WKPreferencesSetAllowRunningOfInsecureContent(preferences, true);
    WKPreferencesSetAllowDisplayOfInsecureContent(preferences, true);

    // By default allow console log messages to system console reporting.
    if (!g_getenv("WPE_SHELL_DISABLE_CONSOLE_LOG"))
      WKPreferencesSetLogsPageMessagesToSystemConsoleEnabled(preferences, true);

    WKPageGroupSetPreferences(pageGroup, preferences);

    auto pageConfiguration  = WKPageConfigurationCreate();
    WKPageConfigurationSetContext(pageConfiguration, context);
    WKPageConfigurationSetPageGroup(pageConfiguration, pageGroup);
    WKPreferencesSetFullScreenEnabled(preferences, true);

    if (!!g_getenv("WPE_SHELL_COOKIE_STORAGE")) {
      gchar *cookieDatabasePath = g_build_filename(g_get_user_cache_dir(), "cookies.db", nullptr);
      auto path = WKStringCreateWithUTF8CString(cookieDatabasePath);
      g_free(cookieDatabasePath);
      auto cookieManager = WKContextGetCookieManager(context);
      WKCookieManagerSetCookiePersistentStorage(cookieManager, path, kWKCookieStorageTypeSQLite);
    }

    auto view = WKViewCreate(pageConfiguration);
    WKViewSetViewClient(view, &s_viewClient.base);

    auto page = WKViewGetPage(view);
    WKPageSetPageNavigationClient(page, &s_navigationClient.base);
    WKPageSetPageUIClient(page, &s_pageUiClient.base);

    const char* url = "http://youtube.com/tv";
    if (argc > 1)
        url = argv[1];

    auto shellURL = WKURLCreateWithUTF8CString(url);
    WKPageLoadURL(page, shellURL);
    WKRelease(shellURL);

    g_main_loop_run(loop);

    WKRelease(view);
    WKRelease(pageConfiguration);
    WKRelease(pageGroup);
    WKRelease(context);
    WKRelease(preferences);
    g_main_loop_unref(loop);
    return 0;
}
