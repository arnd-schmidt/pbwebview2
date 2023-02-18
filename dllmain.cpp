
// PowerBuilder WebView2 - PBNI Extension - Proof Of Concept
// Minumum Requuirements: 
//    Microsoft Egde Browser Release 85.0.564.40
//    PowerBuilder 10.5 (32-Bit Version)
// 
// Author: Arnd Schmidt

#include "pch.h"
//#include "pbni\include\pbext.h"
#include "CWebView2.h"

#pragma warning(disable : 4786)

PBXEXPORT LPCTSTR PBXCALL PBX_GetDescription()
{
	static const TCHAR desc[] = {
		L"class ux_webview2 from userobject\n"
	//	L"INDIRECT string defaulURL{setdefaultuUrl(*value),getDefaultUrl()}\n"
		L"subroutine setdefaultuUrl(string defaultURL)\n"
		L"function string getDefaultUrl()\n"
		L"event int onclick()\n"
		L"event int ondoubleclick()\n"
		L"function string getBrowserVersionString()\n"
		L"function boolean CanGoBack()\n"
		L"function boolean CanGoForward()\n"
		L"function int CreateWebView(string config_JSON)\n"
		L"function string DocumentTitle()\n"
		L"function int Navigate(string uri)\n"
		L"function int NavigateEx(string uri, string method, string headers, string postdata)\n"
		L"function int NavigateToString(string htmlContent)\n"
		L"function int Reload()\n"
		L"function int Stop()\n"
		L"function int GoBack()\n"
		L"function int GoForward()\n"
		L"function int ExecuteScript(string executeScript)\n"
		L"function int ExecuteScript2(longlong callerID, string executeScript)\n"
		L"function int ExecuteScriptSync(longlong callerID, string executeScript, ref string result)\n"
		L"event int DocumentTitleChanged(string title)\n"
		L"event int ExecuteScriptResult(string scriptresult)\n"
		L"event int ExecuteScript2Result(longlong callerID, string scriptResult)\n"
		L"event int NavigationStarting()\n"
		L"event int NavigationCompleted(boolean success, int webErrorStatus)\n"
		L"event int SourceChanged(string uri)\n"
		L"event int WebMessageReceived(string msg, string data)\n"
		L"end class\n"
	};

	return desc;
}


BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  reasonForCall,
	LPVOID lpReserved
)
{
	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		CWebView2::RegisterClass(HMODULE(hModule));
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		CWebView2::UnregisterClass(HMODULE(hModule));
		break;
	}
	return TRUE;
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

	if ( wcscmp(className, L"ux_webview2") == 0 )
	{
		*obj = new CWebView2(pbsession, pbobj);
	}
	else
	{
		*obj = NULL;
		result = PBX_FAIL;
	}

	return result;
};

