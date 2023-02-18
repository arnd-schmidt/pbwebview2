#pragma once
#include "pbext.h"
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"

#include <EventToken.h>
#include <functional>
#include <map>

static constexpr UINT MSG_NAVIGATE = WM_APP + 123;
static constexpr UINT MSG_RUN_ASYNC_CALLBACK = WM_APP + 124;

class CWebView2 :    public IPBX_VisualObject
{
	static TCHAR pbClassname[];

	enum PowerBuilder_Method_Identifier
	{

		mid_setdefaultURL = 0,
		mid_getdefaultURL,
		mid_OnClick, 
		mid_OnDoubleClick,
		mid_getBrowserVersionString,
		mid_CanGoBack,
		mid_CanGoForward,
		mid_CreateWebView,
		mid_DocumentTitle,
		mid_Navigate,
		mid_NavigateEx,
		mid_NavigateToString,
		mid_Reload,
		mid_Stop,
		mid_GoBack,
		mid_GoForward,
		mid_ExecuteScript,
		mid_ExecuteScript2,
		mid_ExecuteScriptSync,
		mid_DocumentTitleChanged,
		mid_ExecuteScriptResult, 
		mid_ExecuteScript2Result,
		mid_NavigationStarting,
		mid_NavigationCompleted,
		mid_SourceChanged,
		mid_WebMessageReceived
	};

	
private:

	IPB_Session* d_session;
	pbobject	d_pbobj;
	HWND		d_hwnd;

	bool		is_ready;
	bool m_isNavigating;

	// Special Member Variables for WebView2
	Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_contentController = nullptr;
	Microsoft::WRL::ComPtr<ICoreWebView2> m_contentWebView;
	Microsoft::WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> m_securityStateChangedReceiver;
	Microsoft::WRL::ComPtr<ICoreWebView2Settings> m_webSettings;

	enum class CallbackType
	{
		CreationCompleted = 0,
		NavigationCompleted,
		TitleChanged,
	};

	using CallbackFunc = std::function<void()>;


public:

	CWebView2(IPB_Session* session, pbobject pbobj) :
		d_session(session),
		d_pbobj(pbobj),
		d_hwnd(NULL),
		is_ready(false)
	{
		m_callbacks[CallbackType::CreationCompleted] = nullptr;
		m_callbacks[CallbackType::NavigationCompleted] = nullptr;
	}

	~CWebView2()
	{
	}

	LPCTSTR GetWindowClassName();

	HWND CreateControl
	(
		DWORD dwExStyle,      // extended window style
		LPCTSTR lpWindowName, // window name
		DWORD dwStyle,        // window style
		int x,                // horizontal position of window
		int y,                // vertical position of window
		int nWidth,           // window width
		int nHeight,          // window height
		HWND hWndParent,      // handle to parent or owner window
		HINSTANCE hInstance   // handle to application instance
	);

	PBXRESULT Invoke
	(
		IPB_Session* session,
		pbobject	obj,
		pbmethodID	mid,
		PBCallInfo* ci
	);

	void Destroy()
	{
		delete this;
	}

	static void RegisterClass(HMODULE hModuleInstance);
	static void UnregisterClass(HMODULE hModuleInstance);


	// Internal Callback Notififications
	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateWebViewControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	// Helper Functions
	PWSTR getuserdatafolder();
	PWSTR getbrowserExecutableFolder();

	// Create and Destroy
	void create_webview();
	void close_webview();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	std::map<CallbackType, CallbackFunc> m_callbacks;
	void RunAsync(CallbackFunc callback);
private:
	EventRegistrationToken m_navigationCompletedToken = {};
	EventRegistrationToken m_navigationStartingToken = {};
	EventRegistrationToken m_documentTitleChangedToken = {};
	EventRegistrationToken m_sourceChangedToken = {};
	EventRegistrationToken m_webMessageReceivedToken = {};

	PBXRESULT getBrowserVersionString(PBCallInfo* ci);
	PBXRESULT CanGoBack(PBCallInfo* ci);
	PBXRESULT CanGoForward(PBCallInfo* ci);
	PBXRESULT DocumentTitle(PBCallInfo* ci);
	PBXRESULT Navigate(PBCallInfo* ci);
	PBXRESULT NavigateToString(PBCallInfo* ci);
	PBXRESULT Reload(PBCallInfo* ci);
	PBXRESULT Stop(PBCallInfo* ci);
	PBXRESULT GoBack(PBCallInfo* ci);
	PBXRESULT GoForward(PBCallInfo* ci);
	PBXRESULT ExecuteScript(PBCallInfo* ci);
	PBXRESULT ExecuteScript2(PBCallInfo* ci);
	PBXRESULT ExecuteScriptSync(PBCallInfo* ci);
	PBXRESULT CreateWebview(PBCallInfo* ci);

	void TriggerEvent(LPCTSTR eventName);
	void TriggerWebMessageReceived(LPCTSTR msg, LPCTSTR data);
	void TriggerExecuteScriptResult(LPCTSTR scriptResult);
	void TriggerExecuteScript2Result(pblonglong callerID, LPCTSTR scriptResult);
	void TriggerDocumentTitleChanged(LPCTSTR documentTitle);
	void TriggerNavigationStarting();
	void TriggerNavigationCompleted(BOOL success, int webErrorStatus);
	void TriggerSourceChanged(LPCTSTR uri);
	void RegisterEventCallbacks();

	//LRESULT PostMessage(UINT message, WPARAM wParam, LPARAM lParam);
};
