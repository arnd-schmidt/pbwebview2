
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
		L"event int navigationstarting()\n"
		L"event int navigationcompleted()\n"
		L"event int documenttitlechanged()\n"
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
		mid_StopNavigation
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

	void TriggerEvent(LPCTSTR eventName);

	static void RegisterClass();
	static void UnregisterClass();

	void create_webview();
	void close_webview();
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	PBXRESULT Navigate(PBCallInfo* ci);
	PBXRESULT StopNavigation(PBCallInfo* ci);
};

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

	default:
		// This is brutal!
		return PBX_FAIL;
	}

	return PBX_OK;
}

void CVisualExt::TriggerEvent(LPCTSTR eventName)
{
	pbclass clz = d_session->GetClass(d_pbobj);
	//pbmethodID mid = d_session->GetMethodID(clz, eventName, PBRT_EVENT, _T("I");
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


void CVisualExt::RegisterClass()
{
	WNDCLASS wndclass;

	wndclass.style = CS_GLOBALCLASS | CS_DBLCLKS;
	wndclass.lpfnWndProc = WindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = g_dll_hModule;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = NULL;
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
		// WM_SIZE might trigger bevor Control is ready!		
		if ((ext != nullptr) && (ext->is_ready)) {

			if (ext->m_contentController != nullptr) {
				RECT bounds;
				GetClientRect(ext->d_hwnd, &bounds);
				ext->m_contentController->put_Bounds(bounds);
			};
		};
		return 0;

	case WM_COMMAND:
		return 0;

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
	pbobject				pbobj,
	LPCTSTR					className,
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

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  reasonForCall,
	LPVOID lpReserved
)
{
	g_dll_hModule = HMODULE(hModule);

	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
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
		is_ready = true;

		// Quick and Dirty resize
		if (m_contentController != nullptr) {
			RECT bounds;
			GetClientRect(d_hwnd, &bounds);
				m_contentController->put_Bounds(bounds);
		};
		
		// Navigate to an inital URL
		LPCWSTR uri = L"about:blank";
		m_contentWebView->Navigate(uri);

	}
	else
	{
		// Error Handling..
		MessageBox(nullptr, L"Cannot create webview environment.", nullptr, MB_OK);
	}

	return S_OK;
}
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
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		nullptr,
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
