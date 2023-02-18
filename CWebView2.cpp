#include "pch.h"
#include "CWebView2.h"

//#pragma warning(disable : 4786)

#include <windows.h>

#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>
#include "JSON/json.hpp"
//using namespace nlohmann::json;
// for convenience
using json = nlohmann::json;

// Needed for Callback<...>.Get()
using namespace Microsoft::WRL;
#include "WebView2.h"

#include <initguid.h>
using std::wstring;
using std::string;

#include <vector>
using std::vector;

#include "Knownfolders.h"
#include "shlobj.h"

#if !defined(VERIFY)
# if !defined(QT_NO_DEBUG)
#  define VERIFY Q_ASSERT
# else
#  define VERIFY(expr)  \
    do                    \
    {                     \
        (void) (expr);    \
    } while (0)
# endif
#endif

#define CHECK_FAILURE_STRINGIFY(arg)         #arg
#define CHECK_FAILURE_FILE_LINE(file, line)  ([](HRESULT hr){ CheckFailure(hr, L"Failure at " CHECK_FAILURE_STRINGIFY(file) L"(" CHECK_FAILURE_STRINGIFY(line) L")"); })
#define CHECK_FAILURE                        CHECK_FAILURE_FILE_LINE(__FILE__, __LINE__)
#define CHECK_FAILURE_BOOL(value)            CHECK_FAILURE((value) ? S_OK : E_UNEXPECTED)

void ShowFailure(HRESULT hr, std::wstring const& message)
{
	std::wstring text;
	//text.Format(L"%s (0x%08X)", (LPCTSTR)message, hr);
	//  Was variable text before!
	::MessageBox(nullptr, static_cast<LPCTSTR>(message.c_str()), L"Failure", MB_OK);
}

void CheckFailure(HRESULT hr, std::wstring const& message)
{
	if (FAILED(hr))
	{
		std::wstring text;
		//text.Format(L"%s : 0x%08X", (LPCTSTR)message, hr);

		// TODO: log text
		std::exit(hr);
	}
}

TCHAR CWebView2::pbClassname[] = L"ux_webview2";

class CallbackInfo {
public:
	CWebView2 *pbWebView2;
	pblonglong callerID;
	boolean done;
	HRESULT result;
	std::wstring scriptResult;
	std::wstring scriptError;
	

public:
	CallbackInfo(CWebView2* pbWebView2, pblonglong callerID) {
		this->pbWebView2 = pbWebView2;
		this->callerID = callerID;
		this->done = false;
		this->result = S_OK;
	}
};

LPCTSTR CWebView2::GetWindowClassName()
{
	return pbClassname;
}

HWND CWebView2::CreateControl
(
	DWORD dwExStyle,      // Extended Window Style
	LPCTSTR lpWindowName, // Window Name
	DWORD dwStyle,        // Window Style
	int x,                // X-Position of Window
	int y,                // Y-Position of Window
	int nWidth,           // Width of Window 
	int nHeight,          // Height of Wwindow 
	HWND hWndParent,      // Handle to Parent Window
	HINSTANCE hInstance   // Handle to Application Instance
)
{
	d_hwnd = CreateWindowEx(dwExStyle, pbClassname, lpWindowName, dwStyle,
		x, y, nWidth, nHeight, hWndParent, NULL, hInstance, NULL);

	::SetWindowLongPtr(d_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	// create_webview();
	// We have to wait for is_ready status!

	return d_hwnd;
}

PBXRESULT CWebView2::Invoke
(
	IPB_Session* session,
	pbobject	obj,
	pbmethodID	mid,
	PBCallInfo* ci
)
{
	//PBXRESULT ret;

	switch (mid)
	{
	case mid_getBrowserVersionString:
		return getBrowserVersionString(ci);
	case mid_CanGoBack:
		return CanGoBack(ci);
	case mid_CanGoForward:
		return CanGoForward(ci);
	case mid_CreateWebView:
		return CreateWebview(ci);
	case mid_DocumentTitle:
		return DocumentTitle(ci);
	case mid_Navigate:
		return Navigate(ci);
	case mid_NavigateToString:
		return NavigateToString(ci);
	case mid_Reload:
		return Reload(ci);
	case mid_Stop:
		return Stop(ci);
	case mid_GoBack:
		return GoBack(ci);
	case mid_GoForward:
		return GoForward(ci);
	case mid_ExecuteScript:
		return ExecuteScript(ci);
	case mid_ExecuteScript2:
		return ExecuteScript2(ci);
	case mid_ExecuteScriptSync:
		return ExecuteScriptSync(ci);
	case mid_setdefaultURL:
		return PBX_OK;
	case mid_getdefaultURL:
		return PBX_OK;
	// Notification-Events
	case mid_OnClick:
		return PBX_OK;
	case mid_OnDoubleClick:
		return PBX_OK;
	case mid_DocumentTitleChanged:
		return PBX_OK;
	case mid_ExecuteScriptResult:
		return PBX_OK;
	case mid_ExecuteScript2Result:
		return PBX_OK;
	case mid_NavigationStarting:
		return PBX_OK;
	case mid_NavigationCompleted:
		return PBX_OK;
	case mid_SourceChanged:
		return PBX_OK;
	case mid_WebMessageReceived:
		return PBX_OK;
	default:
		return PBX_E_INVOKE_METHOD_AMBIGUOUS;
	}

	return PBX_OK;
}

void CWebView2::TriggerDocumentTitleChanged(LPCTSTR documentTitle)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"DocumentTitleChanged", PBRT_EVENT, L"IS");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	ci.pArgs->GetAt(0)->SetString(documentTitle);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}


void CWebView2::TriggerNavigationStarting()
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"NavigationStarting", PBRT_EVENT, L"I");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}

void CWebView2::TriggerNavigationCompleted(BOOL success, int webErrorStatus)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"NavigationCompleted", PBRT_EVENT, L"IBI");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);
	ci.pArgs->GetAt(0)->SetBool(success);
	ci.pArgs->GetAt(1)->SetInt(webErrorStatus);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}

void CWebView2::TriggerExecuteScriptResult(LPCTSTR scriptResult)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"ExecuteScriptResult", PBRT_EVENT, L"IS");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	ci.pArgs->GetAt(0)->SetString(scriptResult);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}

void CWebView2::TriggerExecuteScript2Result(pblonglong callerID, LPCTSTR scriptResult)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"ExecuteScript2Result", PBRT_EVENT, L"IKS");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	ci.pArgs->GetAt(0)->SetLongLong(callerID);
	ci.pArgs->GetAt(1)->SetString(scriptResult);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}


void CWebView2::TriggerSourceChanged(LPCTSTR uri)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"SourceChanged", PBRT_EVENT, L"IS");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	ci.pArgs->GetAt(0)->SetString(uri);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}

void CWebView2::TriggerWebMessageReceived(LPCTSTR msg, LPCTSTR data)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"WebMessageReceived", PBRT_EVENT, L"ISS");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	ci.pArgs->GetAt(0)->SetString(L"WebMessageReceived");
	ci.pArgs->GetAt(1)->SetString(data);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);

	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}
// Anonymous Event - Without any parms
void CWebView2::TriggerEvent(LPCTSTR eventName)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, eventName, PBRT_EVENT, L"I");

	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	//pbint cnt = ci.pArgs->GetCount();
	//	ci.returnValue->SetInt(0);
	///ci.pArgs->GetAt(0)->SetLong(18);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);

	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}

PBXRESULT CWebView2::CanGoBack(PBCallInfo* ci)
{
	BOOL canGoBack;
	HRESULT result = m_contentWebView->get_CanGoBack(&canGoBack);
	ci->returnValue->SetBool(result == S_OK && canGoBack);
	return PBX_OK;
}

PBXRESULT CWebView2::CanGoForward(PBCallInfo* ci)
{
	BOOL canGoForward;
	HRESULT result = m_contentWebView->get_CanGoForward(&canGoForward);
	ci->returnValue->SetBool(result == S_OK && canGoForward);
	return PBX_OK;
}

PBXRESULT CWebView2::Navigate(PBCallInfo* ci)
{
	IPB_Arguments* args = ci->pArgs;
	pbstring str = args->GetAt(0)->GetString();
	if (str != NULL) {
		//while ((this->is_ready == false) ) { Sleep(10); };

		if (this->is_ready == false) 
			ci->returnValue->SetInt(-8);
		else {
			if (m_contentWebView != nullptr) {
				HRESULT result = m_contentWebView->Navigate(d_session->GetString(str));
				ci->returnValue->SetInt(result == S_OK ? 1 : -1);
			}
			else {
				ci->returnValue->SetInt(-7);
			}
		}
	
	}
	else {
		ci->returnValue->SetInt(0);
	}
	return PBX_OK;
}

PBXRESULT CWebView2::NavigateToString(PBCallInfo* ci)
{
	IPB_Arguments* args = ci->pArgs;
	pbstring str = args->GetAt(0)->GetString();
	if (str != NULL) {
		HRESULT result = m_contentWebView->NavigateToString(d_session->GetString(str));
		ci->returnValue->SetInt(result == S_OK ? 1 : -1);
	} else {
		ci->returnValue->SetInt(0);
	}
	return PBX_OK;
}

PBXRESULT CWebView2::Reload(PBCallInfo* ci)
{
	HRESULT result = m_contentWebView->Reload();
	ci->returnValue->SetInt(result == S_OK ? 1 : -1);
	return PBX_OK;
}

PBXRESULT CWebView2::Stop(PBCallInfo* ci)
{
	HRESULT result = m_contentWebView->Stop();
	ci->returnValue->SetInt(result == S_OK ? 1 : -1);
	return PBX_OK;
}

PBXRESULT CWebView2::GoBack(PBCallInfo* ci)
{
	HRESULT result = m_contentWebView->GoBack();
	ci->returnValue->SetInt(result == S_OK ? 1 : -1);
	return PBX_OK;
}

PBXRESULT CWebView2::GoForward(PBCallInfo* ci)
{
	HRESULT result = m_contentWebView->GoForward();
	ci->returnValue->SetInt(result == S_OK ? 1 : -1);
	return PBX_OK;
}

PBXRESULT CWebView2::ExecuteScript(PBCallInfo* ci)
{
	pbstring str = ci->pArgs->GetAt(0)->GetString();

	if (str != NULL) {
		m_contentWebView->ExecuteScript(d_session->GetString(str),
			Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[this](HRESULT error, PCWSTR result) -> HRESULT
				{
					if (error == S_OK) {
						this->TriggerExecuteScriptResult(result);
					}
					else {
						this->TriggerExecuteScriptResult(L"ERR");
					}
					return S_OK;
				}).Get());
		ci->returnValue->SetInt(1);
	}
	else {
		ci->returnValue->SetInt(0);
	}
	return PBX_OK;
}

PBXRESULT CWebView2::ExecuteScript2(PBCallInfo* ci)
{
	pblonglong callerID = ci->pArgs->GetAt(0)->GetLongLong();
	pbstring str = ci->pArgs->GetAt(1)->GetString();

	CallbackInfo *callbackinfo = new CallbackInfo(this, callerID);


	if (str != NULL) {
		m_contentWebView->ExecuteScript(d_session->GetString(str),
			Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[callbackinfo](HRESULT error, PCWSTR result) -> HRESULT
				{
					pblonglong callerID = callbackinfo->callerID;

					if (error == S_OK) {
						callbackinfo->pbWebView2->TriggerExecuteScript2Result(callerID, result);
					}
					else {
						callbackinfo->pbWebView2->TriggerExecuteScript2Result(callerID,L"ERR");
					}
					return S_OK;
				}).Get());
		ci->returnValue->SetInt(1);
	}
	else {
		ci->returnValue->SetInt(0);
	}
	return PBX_OK;
}

PBXRESULT CWebView2::ExecuteScriptSync(PBCallInfo* ci)
{
	pblonglong callerID = ci->pArgs->GetAt(0)->GetLongLong();
	pbstring str = ci->pArgs->GetAt(1)->GetString();

	CallbackInfo* callbackinfo = new CallbackInfo(this, callerID);

	if (str != NULL) {
		m_contentWebView->ExecuteScript(d_session->GetString(str),
			Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[callbackinfo](HRESULT error, PCWSTR result) -> HRESULT
				{
					pblonglong callerID = callbackinfo->callerID;

					callbackinfo->result = error;
					if (error == S_OK) {
						callbackinfo->scriptResult = result;
					}
					else {
						callbackinfo->scriptResult = L"ERR";
					}
					callbackinfo->done = true;
					return S_OK;
				}).Get());
	}
	else {
		ci->returnValue->SetInt(0);
		return PBX_OK;
	}
	// The return string will be set by CExecuteScriptCompletedCallback::Invoke()
	MSG sMsg;
	do
	{
		GetMessage(&sMsg, nullptr, WM_USER + 1, WM_USER + 1);
		TranslateMessage(&sMsg);
		DispatchMessage(&sMsg);
	} while (!(callbackinfo->done));


	d_session->SetString(ci->pArgs->GetAt(2)->GetString(), callbackinfo->scriptResult.c_str());

	ci->returnValue->SetInt(1);

	return PBX_OK;
}


PBXRESULT CWebView2::getBrowserVersionString(PBCallInfo* ci) {
	std::wstring result;
	wil::unique_cotaskmem_string version_info;
	m_webViewEnvironment->get_BrowserVersionString(&version_info);
	result = version_info.get();
	ci->returnValue->SetString(result.c_str());
	return PBX_OK;
}

PBXRESULT CWebView2::DocumentTitle(PBCallInfo* ci) {
	std::wstring result;
	wil::unique_cotaskmem_string documentTitle;
	m_contentWebView->get_DocumentTitle(&documentTitle);
	result = documentTitle.get();
	ci->returnValue->SetString(result.c_str());
	return PBX_OK;
}

void CWebView2::RegisterClass(HMODULE hModuleInstance)
{
	WNDCLASS wndclass;

	wndclass.style = CS_GLOBALCLASS | CS_DBLCLKS;
	//wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hModuleInstance;
	wndclass.hIcon = nullptr;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = pbClassname;

	::RegisterClass(&wndclass);
}

void CWebView2::UnregisterClass(HMODULE hModuleInstance)
{
	::UnregisterClass(pbClassname, hModuleInstance);
}

LRESULT CALLBACK CWebView2::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWebView2* ext = (CWebView2*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_CREATE:

		//ext->create_webview();
		return 0;

	case WM_SIZE:
		// WM_SIZE can trigger before WebControl is ready!		
		if ((ext != nullptr) && (ext->is_ready)) {

			if (ext->m_contentController != nullptr) {
				RECT bounds;
				GetClientRect(hwnd, &bounds);
				ext->m_contentController->put_Bounds(bounds);
				return 0;
			};
		};
		PostMessageW(hwnd, uMsg, wParam, lParam);
		return 0;

	case WM_COMMAND:
		break;

	case WM_LBUTTONDBLCLK:
		ext->TriggerEvent(L"ondoubleclick");
		break;

	case WM_LBUTTONDOWN:
		ext->TriggerEvent(L"onclick");
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
/*
LRESULT CWebView2::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::PostMessage(d_hwnd, message, wParam, lParam);
}

void CWebView2::RunAsync(CallbackFunc callback)
{
	auto* task = new CallbackFunc(callback);
	PostMessage(MSG_RUN_ASYNC_CALLBACK, reinterpret_cast<WPARAM>(task), 0);
}
*/

HRESULT CWebView2::OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment)
{
	environment->QueryInterface(IID_PPV_ARGS(&m_webViewEnvironment));
	m_webViewEnvironment->CreateCoreWebView2Controller(
		d_hwnd,
		Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
			this,
			&CWebView2::OnCreateWebViewControllerCompleted).Get());

	return S_OK;
}

void  CWebView2::RegisterEventCallbacks() {

	CHECK_FAILURE(m_contentWebView->add_NavigationCompleted(
		Callback<ICoreWebView2NavigationCompletedEventHandler>(
			[this](
				ICoreWebView2*,
				ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
			{
				this->m_isNavigating = false;
				COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
				
				BOOL success;
				CHECK_FAILURE(args->get_IsSuccess(&success));

				if (!success)
				{
					//COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus{};
					CHECK_FAILURE(args->get_WebErrorStatus(&webErrorStatus));
					if (webErrorStatus == COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED)
					{
						// Do something here if you want to handle a specific error case.
						// In most cases this isn't necessary, because the WebView will
						// display its own error page automatically.
					}
				}
				/*wil::unique_cotaskmem_string uri;
				this->m_contentWebView->get_Source(&uri);

				if (wcscmp(uri.get(), L"about:blank") == 0)
				{
					uri = wil::make_cotaskmem_string(L"");
				}*/

				this->TriggerNavigationCompleted(success, webErrorStatus);
				/*
				auto callback = m_callbacks[CallbackType::NavigationCompleted];
				if (callback != nullptr)
					RunAsync(callback);
				*/
				return S_OK;
			}).Get(), &m_navigationCompletedToken));

	CHECK_FAILURE(m_contentWebView->add_NavigationStarting(
		Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](
				ICoreWebView2*,
				ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
			{
				wil::unique_cotaskmem_string uri;
				CHECK_FAILURE(args->get_Uri(&uri));

				this->m_isNavigating = true;
				this->TriggerNavigationStarting();

				return S_OK;
			}).Get(), &m_navigationStartingToken));

	CHECK_FAILURE(m_contentWebView->add_SourceChanged(
		Callback<ICoreWebView2SourceChangedEventHandler>(
			[this](
				ICoreWebView2*,
				ICoreWebView2SourceChangedEventArgs* args) -> HRESULT
			{
				BOOL isNewDocument;
				std::wstring result;
				wil::unique_cotaskmem_string uri;
				CHECK_FAILURE(args->get_IsNewDocument(&isNewDocument));
				
				this->m_contentWebView->get_Source(&uri);
				result = uri.get();
				this->TriggerSourceChanged(result.c_str());

				return S_OK;
			}).Get(), &m_sourceChangedToken));

	CHECK_FAILURE(m_contentWebView->add_WebMessageReceived(
		Callback<ICoreWebView2WebMessageReceivedEventHandler>(
			[this](
				ICoreWebView2*,
				ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT 
			{
				PWSTR message;
				std::wstring messagewstr;
				
				CHECK_FAILURE(args->TryGetWebMessageAsString(&message));
				messagewstr = message;
				this->TriggerWebMessageReceived(L"'webMessageReceived", messagewstr.c_str());
				CoTaskMemFree(message);
				return S_OK;
			}).Get(), &m_webMessageReceivedToken));

}

HRESULT CWebView2::OnCreateWebViewControllerCompleted( HRESULT result, ICoreWebView2Controller* controller)
{
	if (result == S_OK)
	{
		if (controller != nullptr)
		{
			m_contentController = controller;
			m_contentController->get_CoreWebView2(&m_contentWebView);
		}
		else {
			// Error Handling..
			MessageBox(nullptr, L"Cannot get CoreWebView2", nullptr, MB_OK);
		}
		m_contentWebView->get_Settings(&m_webSettings);
		m_webSettings->put_IsWebMessageEnabled(true);		
		
		RegisterEventCallbacks();
		
		// Quick and Dirty resize
		if (m_contentController != nullptr) {
			RECT bounds;
			GetClientRect(this->d_hwnd, &bounds);
			m_contentController->put_Bounds(bounds);
			this->is_ready = true;
		};
		
	}
	else
	{
		// Error Handling..
		MessageBox(nullptr, L"Cannot create webview environment.", nullptr, MB_OK);
	}

	return S_OK;
}
PWSTR CWebView2::getbrowserExecutableFolder() {
	// Only for internal use!

	PWSTR browserExecutableFolder = NULL;
	//size_t buflen;
	//LPCWSTR addToPath = L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\85.0.564.68";
	//LPCWSTR addToPath = L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\86.0.622.48";
	//LPCWSTR addToPath = L"D:\\asc\\PBNI\\Microsoft.WebView2.FixedVersionRuntime.87.0.664.8.x86";
	//buflen = lstrlen(addToPath) + 1;
	//browserExecutableFolder = new TCHAR[buflen];
	//wcscpy_s(browserExecutableFolder, buflen, addToPath);
	return browserExecutableFolder;
};

PWSTR CWebView2::getuserdatafolder() {
	PWSTR userDataFolder = NULL;
	LPCWSTR addToPath = L"\\pbwebview2";
	HRESULT hres;

	// FOLDERID_UserProgramFiles
	hres = SHGetKnownFolderPath(FOLDERID_UserProgramFiles, 0, nullptr, &userDataFolder);
	if (SUCCEEDED(hres) && userDataFolder != NULL) {

		size_t newlen = wcslen(userDataFolder) + wcslen(addToPath) + 1; //  sizeof(WCHAR);
		size_t newbufsize = newlen * sizeof(WCHAR);

		auto newPtr = CoTaskMemRealloc(userDataFolder, newbufsize);
		if (!newPtr ) {
			CoTaskMemFree(userDataFolder);
			// Error Handling?!
		}
		userDataFolder = reinterpret_cast<PWSTR>(newPtr);
		wcscat_s(userDataFolder, newlen, addToPath);
	}
	else {
		// Error Handling?
	};
	return userDataFolder;
};

// Old internal Function
void CWebView2::create_webview() {

	PWSTR userDataFolder = getuserdatafolder();
	PWSTR browserExecutableFolder = getbrowserExecutableFolder();
	this->is_ready = false;
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		browserExecutableFolder,
		userDataFolder,
		nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			this,
			&CWebView2::OnCreateEnvironmentCompleted).Get());

	return;
}

namespace cfgJSON {
	const string cs_userDataFolder = std::string("userDataFolder");
	const string cs_browserExecutableFolder = std::string("browserExecutableFolder");
	const string cs_environmentOptions = std::string("environmentOptions");

	struct webviewconfig
	{
		std::wstring userDataFolder;
		std::wstring browserExecutableFolder;
		std::wstring environmentOptions;
	};
	void to_json(json& j, const webviewconfig& w) {
		j = json{
			{cs_userDataFolder, w.userDataFolder},
			{cs_browserExecutableFolder, w.browserExecutableFolder},
			{cs_environmentOptions, w.environmentOptions} };
	}
	void from_json(const json& j, webviewconfig& w) {
	std::string tmp;
		if (j.contains(cs_userDataFolder)) {
			if (!(j.at(cs_userDataFolder).is_null())) {
				j.at(cs_userDataFolder).get_to(tmp);
				w.userDataFolder = std::wstring(tmp.begin(), tmp.end());
				tmp = "";
			}
		}
		if (j.contains(cs_browserExecutableFolder)) {
			if (!(j.at(cs_browserExecutableFolder).is_null())) {
				j.at(cs_browserExecutableFolder).get_to(tmp);
				w.browserExecutableFolder = std::wstring(tmp.begin(), tmp.end());
				tmp = "";
			}
		}
		j.at(cs_environmentOptions).get_to(tmp);
		w.environmentOptions = std::wstring(tmp.begin(), tmp.end());
		tmp = "";
	}
}

PBXRESULT CWebView2::CreateWebview(PBCallInfo* ci) {
	using namespace cfgJSON;
	IPB_Arguments* args = ci->pArgs;
	this->is_ready = false;
	// Get JSON String from Argument
	std::wstring webViewConfig_strJSON = d_session->GetString(args->GetAt(0)->GetString());

	// Get webViewConfig from JSON 
	webviewconfig webViewConfig;
	try {
		auto j = json::parse(webViewConfig_strJSON);
		cfgJSON::from_json(j, webViewConfig);
	}
	catch (json::exception& e) {
		// Convert std:string => std:wstring
		std::string tmp = e.what();
		std::wstring msgw = std::wstring(tmp.begin(), tmp.end());
		MessageBox(nullptr, msgw.c_str(), nullptr, MB_OK);
	}

	LPCWSTR	userdatafolder = (webViewConfig.userDataFolder.length() > 0) ? webViewConfig.userDataFolder.c_str() : getuserdatafolder();
	LPCWSTR browserexecutablefolder = (webViewConfig.browserExecutableFolder.length() > 0) ? webViewConfig.browserExecutableFolder.c_str() : getbrowserExecutableFolder();

	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		browserexecutablefolder,
		userdatafolder,
		nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			this,
			&CWebView2::OnCreateEnvironmentCompleted).Get());

	//MSG sMsg;
	if (this->is_ready == false) {
		ci->returnValue->SetInt(-8);
	}
	else {
		ci->returnValue->SetInt(1);
	}

	// The return string will be set by CExecuteScriptCompletedCallback::Invoke()
	//MSG sMsg;
	//while(GetMessage(&sMsg, nullptr, WM_USER + 1, WM_USER + 1)) {
	//	TranslateMessage(&sMsg);
	//	DispatchMessage(&sMsg);
	//} ;


	//{
	//	GetMessage(&sMsg, nullptr, WM_USER + 1, WM_USER + 1);
//		TranslateMessage(&sMsg);
		//DispatchMessage(&sMsg);
	//} 
	/*
	auto hwnd = FindWindowEx(HWND_MESSAGE, this->d_hwnd, L"Chrome_MessageWindow", NULL); // This Window name is undocumented
	MSG msg;
	while ((this->is_ready == false) && GetMessage(&msg, hwnd, WM_USER + 1, WM_USER + 1))
	{
		// From observation, the Edge hidden window uses WM_USER + 1. Since there's no API access to the window, there's no
		// way to be sure that only its messages are retrieved.
	//	ASSERT(msg.message == WM_USER + 1);
		TranslateMessage(&msg); // Probably not necessary for this scenario
		DispatchMessage(&msg);
	}
	*/
	
	return PBX_OK;
}

void CWebView2::close_webview()
{
	if (m_contentWebView)
	{
		m_contentController->Close();

		m_contentController = nullptr;
		m_contentWebView = nullptr;
		m_webSettings = nullptr;
	}

	m_webViewEnvironment = nullptr;
}
