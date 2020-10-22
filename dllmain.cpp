
// PowerBuilder WebView2 - PBNI Extension - Proof Of Concept
// Minumum Requuirements: 
//    Microsoft Egde Browser Release 85.0.564.40
//    PowerBuilder 10.5 (32-Bit Version)
// 
// Author: Arnd Schmidt

#include "pch.h"

//
#pragma warning(disable : 4786)

#include <windows.h>
#include "pbni\include\pbext.h"
#include <string>
using std::wstring;
#include <vector>
using std::vector;


#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"

#include <initguid.h>
#include "Knownfolders.h"
#include "shlobj.h"
using namespace Microsoft::WRL;

HMODULE g_dll_hModule = 0;

PBXEXPORT LPCTSTR PBXCALL PBX_GetDescription()
{
	static const TCHAR desc[] = {
		L"class ux_webview2 from userobject\n"
		L"event long onclick(long a)\n"
		L"event int ondoubleclick()\n"
		L"function int CreateWebView(string config)\n"
		L"function int Navigate(string url)\n"
		L"function int StopNavigation()\n"
		L"function string ExecuteScript(string executeScript)\n"
		L"event int navigationstarting()\n"
		L"event int navigationcompleted()\n"
		L"event int documenttitlechanged(string title)\n"
		L"event int webmessagereceived(string msg, string data)\n"
		L"end class\n"
	};

	return desc;
}

class CVisualExt : public IPBX_VisualObject
{
	enum
	{
		mid_OnClick = 0,
		mid_OnDoubleClick,
		mid_CreateWebView,
		mid_Navigate,
		mid_StopNavigation,
		mid_ExecuteScript,
		mid_Navigationstarting,
		mid_NavigationCompleted,
		mid_DocumentTitleChanged,
		mid_WebMessageReceived
	};

	static TCHAR s_className[];
	
	IPB_Session *d_session;
	pbobject	d_pbobj;
	HWND		d_hwnd;
	bool		is_ready;

	Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_contentController = nullptr;
	Microsoft::WRL::ComPtr<ICoreWebView2> m_contentWebView;
	Microsoft::WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> m_securityStateChangedReceiver;
	Microsoft::WRL::ComPtr<ICoreWebView2Settings> m_webSettings;


	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateWebViewControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);

	PWSTR getuserdatafolder();
	PWSTR getbrowserExecutableFolder();

public:
	CVisualExt(IPB_Session *session, pbobject pbobj) :
		d_session(session),
		d_pbobj(pbobj),
		d_hwnd(NULL),
		is_ready(false)
	{
	}

	~CVisualExt()
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

	
	static void RegisterClass();
	static void UnregisterClass();

	void create_webview();
	void close_webview();
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	PBXRESULT Navigate(PBCallInfo* ci);
	PBXRESULT StopNavigation(PBCallInfo* ci);
	PBXRESULT ExecuteScript(PBCallInfo* ci);

	void TriggerEvent(LPCTSTR eventName);
	void TriggerWebMessageReceived(LPCTSTR msg, LPCTSTR data);

};


BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  reasonForCall,
	LPVOID lpReserved
)
{
	g_dll_hModule = HMODULE(hModule);

	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		//CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		CVisualExt::RegisterClass();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		CVisualExt::UnregisterClass();
		break;
	}
	return TRUE;
}



TCHAR CVisualExt::s_className[] = L"ux_webview2";

LPCTSTR CVisualExt::GetWindowClassName()
{
	return s_className;
}

HWND CVisualExt::CreateControl
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
	d_hwnd = CreateWindowEx(dwExStyle, s_className, lpWindowName, dwStyle,
		x, y, nWidth, nHeight, hWndParent, NULL, hInstance, NULL);

	::SetWindowLong(d_hwnd, GWL_USERDATA, (LONG)this);

	create_webview();
	// We have to wait for is_ready status!

	return d_hwnd;
}

PBXRESULT CVisualExt::Invoke
(
	IPB_Session* session,
	pbobject	obj,
	pbmethodID	mid,
	PBCallInfo* ci
)
{
	PBXRESULT ret;

	switch (mid)
	{
	case mid_OnClick:
		return PBX_OK;

	case mid_OnDoubleClick:
		return PBX_OK;

	case mid_Navigate:
		return Navigate(ci);

	case mid_StopNavigation:
		return StopNavigation(ci);

	case mid_CreateWebView:
		create_webview();
		return PBX_OK;

	case mid_ExecuteScript:
		ret = ExecuteScript(ci);
		return ret;

	case mid_Navigationstarting:
		return PBX_OK;
	case mid_NavigationCompleted:
		return PBX_OK;
	case mid_DocumentTitleChanged:
		return PBX_OK;
	case mid_WebMessageReceived:
		return PBX_OK;


	default:
		// This is brutal!
		return PBX_FAIL;
	}

	return PBX_OK;
}


void CVisualExt::TriggerWebMessageReceived(LPCTSTR msg, LPCTSTR data)
{
	d_session->PushLocalFrame();
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"webmessagereceived", PBRT_EVENT, L"ISS");
	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	//pbint cnt = ci.pArgs->GetCount();
	
	ci.pArgs->GetAt(0)->SetString(msg);
	ci.pArgs->GetAt(1)->SetString(data);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);

	d_session->FreeCallInfo(&ci);
	d_session->PopLocalFrame();
}


void CVisualExt::TriggerEvent(LPCTSTR eventName)
{
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, L"onclick", PBRT_EVENT, L"LL");

	PBCallInfo ci;
	PBXRESULT resinit = d_session->InitCallInfo(clz, mid, &ci);

	pbint cnt = ci.pArgs->GetCount();
	//	ci.returnValue->SetInt(0);
	ci.pArgs->GetAt(0)->SetLong(18);
	PBXRESULT res = d_session->TriggerEvent(d_pbobj, mid, &ci);

	d_session->FreeCallInfo(&ci);

}

PBXRESULT CVisualExt::Navigate(PBCallInfo* ci)
{
	IPB_Arguments* args = ci->pArgs;
	LPCWSTR uri;
	pbstring str = args->GetAt(0)->GetString();
	if (str != NULL) {
		uri = d_session->GetString(str);
		m_contentWebView->Navigate(uri);
	}
	return PBX_OK;
}


PBXRESULT CVisualExt::StopNavigation(PBCallInfo* ci)
{
	IPB_Arguments* args = ci->pArgs;

	return PBX_OK;
}

PBXRESULT CVisualExt::ExecuteScript(PBCallInfo* ci)
{
	ULONG g_ScriptCall; // global, accessible to all threads
	//ULONG CapturedValue = 1;
	//ULONG WaitFree = 0;
	//ULONG WaitBlocked = 1;
	//g_ScriptCall = 1;

	//HANDLE ghMutex;
	//std::mutex _listManagerLock;
	//IPB_Arguments* args = ci->pArgs;
	//LPCWSTR executeScript;
	//LPWSTR execReturn;
	//pbstring ret_val;
	//bool scriptExecuted = false;
	pbstring str = ci->pArgs->GetAt(0)->GetString();
	//ci->returnValue->SetString(L"FAILED");
	if (str != NULL) {
//		WaitForSingleObject(ghMutex, INFINITE);
		//executeScript = d_session->GetString(str);
//		MessageBox(nullptr, executeScript, L"ExecuteScript Script", MB_OK);
		//m_contentWebView->ExecuteScript(executeScript)
		 m_contentWebView->ExecuteScript(d_session->GetString(str),
			Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[this](HRESULT error, PCWSTR result) -> HRESULT
				{
					if (error == S_OK) {
						//int bufferlen = wcslen(result);
						//execReturn = (LPWSTR)malloc((bufferlen + 1) * sizeof(WCHAR));
						//wcscpy_s(execReturn, bufferlen + 1, result);
						this->TriggerWebMessageReceived(L"executeScript", result);
						//free(execReturn);

					}
					else {
						this->TriggerWebMessageReceived(L"executeScript", L"ERR");
					//	MessageBox(nullptr, L"ExecuteScript failed", nullptr, MB_OK);
			//			MessageBox(nullptr, result, L"ExecuteScript Result", MB_OK);
					}
					
					//scriptExecuted = true;
					//g_ScriptCall = 0;
					//ghMutex = CreateMutex(
					//	NULL,              // default security attributes
					//	FALSE,             // initially not owned
					//	NULL);
					//ReleaseMutex(ghMutex);
					return S_OK;
				}).Get());
		
		// while (!(scriptExecuted))
		//	 d_session->ProcessPBMessage();
		//	Yield();
		//	Sleep(2);
		
		//while (CapturedValue == WaitBlocked) {
		//	Yield();
		//	Sleep(200);
		//	//WaitOnAddress(&g_ScriptCall, &WaitFree, sizeof(ULONG), INFINITE);
		//	CapturedValue = g_ScriptCall;
		//}

		//ret_val = d_session->NewString(execReturn);
	//	MessageBox(nullptr, L"ExecuteScript After Call", nullptr, MB_OK);
	//	if ( scriptExecuted ) 
	//		MessageBox(nullptr, L"ExecuteScript Script Executed", nullptr, MB_OK);

		ci->returnValue->SetString(L"OK");
		//if (!(ghMutex ==NULL)) {
		//	CloseHandle(ghMutex);
		//}
		//WaitForSingleObject(ghMutex, INFINITE);

		
	}
	else {
		ci->returnValue->SetString(L"NOP");
	}
	// How to wait for executeScript?
	//ret_val = d_session->NewString(L"Hello From C++");
	//ci->returnValue->SetString(L"Hello From C++");
	//ci->returnValue->SetString(execReturn);
	//d_session->SetString(ret_val, d_session->GetString(ci->returnValue->GetString()));
	//d_session->FreeCallInfo(ci);
	//D
	return PBX_OK;

}



void CVisualExt::RegisterClass()
{
	WNDCLASS wndclass;

	wndclass.style = CS_GLOBALCLASS | CS_DBLCLKS;
	//wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = g_dll_hModule;
	wndclass.hIcon = nullptr;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = s_className;

	::RegisterClass(&wndclass);
}

void CVisualExt::UnregisterClass()
{
	::UnregisterClass(s_className, g_dll_hModule);
}

LRESULT CALLBACK CVisualExt::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CVisualExt* ext = (CVisualExt*)::GetWindowLong(hwnd, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_CREATE:

		//ext->create_webview();
		return 0;

	case WM_SIZE:
		// WM_SIZE might trigger before WebControl is ready!		
		if ((ext != nullptr) && (ext->is_ready)) {

			if (ext->m_contentController != nullptr) {
				RECT bounds;
				GetClientRect(ext->d_hwnd, &bounds);
				ext->m_contentController->put_Bounds(bounds);
			};
		};
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

PBXEXPORT PBXRESULT PBXCALL PBX_CreateVisualObject
(
	IPB_Session* pbsession,
	pbobject	pbobj,
	LPCTSTR		className,
	IPBX_VisualObject** obj
)
{
	PBXRESULT result = PBX_OK;

	wstring cn(className);
	if (cn.compare(L"ux_webview2") == 0)
	{
		*obj = new CVisualExt(pbsession, pbobj);
	}
	else
	{
		*obj = NULL;
		result = PBX_FAIL;
	}

	return PBX_OK;
};


HRESULT CVisualExt::OnCreateEnvironmentCompleted(
	HRESULT result,
	ICoreWebView2Environment* environment)
{
	environment->QueryInterface(IID_PPV_ARGS(&m_webViewEnvironment));
	m_webViewEnvironment->CreateCoreWebView2Controller(
		d_hwnd,
		Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
			this,
			&CVisualExt::OnCreateWebViewControllerCompleted).Get());

	return S_OK;
}

HRESULT CVisualExt::OnCreateWebViewControllerCompleted(
	HRESULT result,
	ICoreWebView2Controller* controller)
{
	if (result == S_OK)
	{
		if (controller != nullptr)
		{
			m_contentController = controller;
			m_contentController->get_CoreWebView2(&m_contentWebView);
		}
		m_contentWebView->get_Settings(&m_webSettings);
		this->is_ready = true;

		// Quick and Dirty resize
		if (m_contentController != nullptr) {
			RECT bounds;
			GetClientRect(d_hwnd, &bounds);
				m_contentController->put_Bounds(bounds);
		};
		
		// Navigate to an inital URL
	//	LPCWSTR uri = L"about:blank";
	//	m_contentWebView->Navigate(uri);

	}
	else
	{
		// Error Handling..
		MessageBox(nullptr, L"Cannot create webview environment.", nullptr, MB_OK);
	}

	return S_OK;
}
PWSTR CVisualExt::getbrowserExecutableFolder() {
	PWSTR browserExecutableFolder;
	size_t buflen;
	//LPCWSTR addToPath = L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\85.0.564.68";
	//LPCWSTR addToPath = L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\86.0.622.48";
	LPCWSTR addToPath = L"D:\\asc\\PBNI\\Microsoft.WebView2.FixedVersionRuntime.87.0.664.8.x86";
	buflen = lstrlen(addToPath) + 1;
	browserExecutableFolder = new TCHAR[buflen];
	wcscpy_s(browserExecutableFolder, buflen, addToPath);
	return browserExecutableFolder;
};

PWSTR CVisualExt::getuserdatafolder() {
	PWSTR userDataFolder;
	LPCWSTR addToPath = L"\\pbwebview2";
	HRESULT hres;
	// FOLDERID_UserProgramFiles
	hres = SHGetKnownFolderPath(FOLDERID_UserProgramFiles, 0, nullptr, &userDataFolder);
	if (SUCCEEDED(hres)) {
		//	wprintf(L"%ls\n", path);
		//MessageBox(nullptr, userDataFolder, nullptr, MB_OK);
		size_t newlen = wcslen(userDataFolder) + wcslen(addToPath) + 1; //  sizeof(WCHAR);
		size_t newbufsize = newlen * sizeof(WCHAR);

		auto newPtr = CoTaskMemRealloc(userDataFolder, newbufsize);
		if (!newPtr) {
			CoTaskMemFree(userDataFolder);
			// Error Handling..
		}
		userDataFolder = reinterpret_cast<PWSTR>(newPtr);
		wcscat_s(userDataFolder, newlen, addToPath);
	}
	else {
		// Error Handling
	};
	return userDataFolder;
};



void CVisualExt::create_webview() {
	PWSTR userDataFolder = getuserdatafolder();
	PWSTR browserExecutableFolder = getbrowserExecutableFolder();
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		browserExecutableFolder,
		userDataFolder,
		nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			this,
			&CVisualExt::OnCreateEnvironmentCompleted).Get());
	return;
}

void CVisualExt::close_webview()
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
