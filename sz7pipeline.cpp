
#include "targetver.h"

#include <memory>
#include <vector>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define STRICT_CONST
#define STRICT_TYPED_ITEMIDS
#define NOOPENFILE
#define NOMINMAX

#include <Windows.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <strsafe.h>
#include <Unknwn.h>
#include <comdef.h>
#include <Propkey.h>

_COM_SMARTPTR_TYPEDEF(IFolderView2, __uuidof(IFolderView2));
_COM_SMARTPTR_TYPEDEF(IShellItemArray, __uuidof(IShellItemArray));
_COM_SMARTPTR_TYPEDEF(IShellItem, __uuidof(IShellItem));

#include <assert.h>
#include <tchar.h>

#pragma comment (lib, "shlwapi.lib")
#pragma comment( lib, "Rpcrt4.lib" ) // RpcStringFree


#define CPPLINQ_INLINEMETHOD __forceinline
#include "cpplinq.hpp"

#ifdef _DEBUG
#define SZ7PP_TEST 1
#endif

inline std::wstring guidToString(const GUID *pGuid)
{
	std::wstring wrk;
	RPC_WSTR waString = nullptr;

	if (RPC_S_OK == ::UuidToString(pGuid, &waString))
	{
		wrk = reinterpret_cast<const WCHAR*>(waString);
		if (RPC_S_OK != ::RpcStringFree(&waString))
		{
			assert(0);
		}
	}
	return std::move(wrk);
}

class CComHeapDeleter
{
public:
	void operator()(void* p)
	{
		::CoTaskMemFree(p);
	}
};

class ComHeapString : public std::unique_ptr<WCHAR[], CComHeapDeleter>
{
	typedef std::unique_ptr<WCHAR[], CComHeapDeleter> base;
public:
	constexpr ComHeapString() noexcept
		: base()
	{
	}
	explicit ComHeapString(WCHAR *ptr) noexcept
		: base(ptr)
	{
	}

	operator const WCHAR *() const
	{
		return get();
	}
};

//typedef std::unique_ptr<WCHAR[], CComHeapDeleter> ComHeapString;

class CCoInitialize {
public:
	CCoInitialize() : m_hr(::CoInitialize(NULL))
	{
		if (FAILED(m_hr)) _com_raise_error(m_hr);
	}
	~CCoInitialize() { if (SUCCEEDED(m_hr)) ::CoUninitialize(); }
	operator HRESULT() const { return m_hr; }
	HRESULT m_hr;
};

inline void check(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_issue_error(hr);
	}
}

inline void checkWin32(BOOL success)
{
	if (!success)
	{
		_com_issue_error(HRESULT_FROM_WIN32(::GetLastError()));
	}
}

inline VARIANT ToVariant(int iVal)
{
	VARIANT v;
	v.vt = VT_I4;
	v.lVal = iVal;

	return v;
}

IShellBrowserPtr getForegroundShellBrowser()
{
	auto hWndFg = ::GetAncestor(::GetForegroundWindow(), GA_ROOT);
	if (hWndFg == nullptr)
		return nullptr;

	IShellWindowsPtr pShellWindows;
	check(::CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pShellWindows)));

	long count;
	check(pShellWindows->get_Count(&count));

	for (int i = 0; i < count; i++)
	{
		IDispatchPtr pDispatch;

		check(pShellWindows->Item(ToVariant(i), &pDispatch));
		{
			IShellBrowserPtr pShellBrowser;
			check(IUnknown_QueryService(pDispatch, SID_STopLevelBrowser, IID_PPV_ARGS(&pShellBrowser)));

			HWND hwnd;
			check(IUnknown_GetWindow(pShellBrowser, &hwnd));
#if SZ7PP_TEST
			return pShellBrowser;
#else
			if (::GetAncestor(hwnd, GA_ROOT) == hWndFg)
			{
				return pShellBrowser;
			}
#endif
		}
	}
	return nullptr;
}


#ifdef NDEBUG
#define debugPrint __noop
#else
void debugPrint(const TCHAR *fmt, ...)
{
	TCHAR buf[512];
	va_list args;
	va_start(args, fmt);
	size_t size = _vsnwprintf_s(buf, _countof(buf), fmt, args);

	assert(size != -1);

	::OutputDebugString(buf);
}
#endif

bool canConvert(LPCWSTR str)
{
	BOOL UsedDefaultChar = FALSE;
	checkWin32(::WideCharToMultiByte(CP_THREAD_ACP, WC_NO_BEST_FIT_CHARS, str, -1, NULL, 0, NULL, &UsedDefaultChar));

	return !UsedDefaultChar;
}

class Config
{
	static std::wstring getString(LPCWSTR iniFile, LPCWSTR appName, LPCWSTR key, LPCWSTR default = L"")
	{
		DWORD cap = 256;
		auto str = std::make_unique<wchar_t[]>(cap);
		do
		{
			auto ret = ::GetPrivateProfileString(appName, key, default, str.get(), cap, iniFile);
			if (ret == cap - 1)
			{
				cap += 256;
				str = std::make_unique<wchar_t[]>(cap);
			}
			else
			{
				break;
			}
		} while (true);

		return str.get();
	}
public:
	const std::wstring m_pattern;
	const std::wstring m_workDir;
	const std::wstring m_filename;
	const bool useShortPath = true;
public:
	explicit Config(LPCWSTR iniFile, LPCWSTR sectionName = L"sz7pipeline")
		: m_pattern(std::move(getString(iniFile, sectionName, L"PATTERN", L"*")))
		, m_workDir(std::move(getString(iniFile, sectionName, L"PATH", L"%TEMP%")))
		, m_filename(std::move(getString(iniFile, sectionName, L"FILENAME", L"")))
	{

	}
};


void main(int argc, WCHAR *argv[])
{
#ifndef SZ7PP_TEST
	if (argc < 2)
	{
		::MessageBox(nullptr, L"Shell:Sendto にショートカットを登録してください。", argv[0], MB_OK | MB_ICONINFORMATION);
		return;
	}
#endif

	WCHAR iniFile[MAX_PATH];
	wcscpy_s(iniFile, argv[0]);
	::PathRenameExtension(iniFile, L".ini");

	const Config config(iniFile);
	using namespace std;
	auto pShellBrowser = getForegroundShellBrowser();
	if (!pShellBrowser)
	{
		::MessageBox(nullptr, L"エクスプローラーが見つかりませんでした。", argv[0], MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	IShellViewPtr pShellView;
	check(pShellBrowser->QueryActiveShellView(&pShellView));

	IFolderView2Ptr pFolderView = pShellView;
	//IFolderViewPtr pFolderView = pShellView;

	IShellFolderPtr pShellFolder;
	HRESULT hr = pFolderView->GetFolder(IID_PPV_ARGS(&pShellFolder));
	if (SUCCEEDED(hr))
	{
		WCHAR dir[MAX_PATH] = L"";
		checkWin32(::ExpandEnvironmentStrings(config.m_workDir.c_str(), dir, _countof(dir)));

		WCHAR fname[MAX_PATH];

		if (config.m_filename.length() == 0)
		{
			::PathCombine(fname, dir, L"XXXXXX");
			_wmktemp_s(fname, _countof(dir));
			wcscat_s(fname, L".sz7");
		}
		else
		{
			::PathCombine(fname, dir, config.m_filename.c_str());
		}

		int selectionCount;
		check(pFolderView->ItemCount(SVGIO_SELECTION, &selectionCount));

		UINT flag = (selectionCount > 1) ? SVGIO_SELECTION : SVGIO_ALLVIEW;

		IShellItemArrayPtr pShellItemArray;
		check(pFolderView->Items(flag | SVGIO_FLAG_VIEWORDER, IID_PPV_ARGS(&pShellItemArray)));

		DWORD shellItemCount;
		check(pShellItemArray->GetCount(&shellItemCount));


		std::vector<ComHeapString> list;
		list.reserve(shellItemCount);

		for (DWORD dwIdx = 0; dwIdx < shellItemCount; dwIdx++)
		{
			IShellItemPtr pShellItem;
			check(pShellItemArray->GetItemAt(dwIdx, &pShellItem));
			LPWSTR pszName = NULL;
			check(pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName));
			list.emplace_back(ComHeapString(pszName));
		}

		FILE* fp = nullptr;
		auto err = _wfopen_s(&fp, fname, L"w+");
		if (err)
		{
			throw std::system_error(err, std::system_category());
		}

		{
			using namespace cpplinq;
			from(list)
				>> where([&](const auto &i) { return ::PathMatchSpec(i.get(), config.m_pattern.c_str()); })
				>> for_each([&](const auto &i)
			{
				WCHAR shortPath[MAX_PATH];

				LPCWSTR path = i.get();
				assert(path);
				if (!canConvert(path) && config.useShortPath)
				{
					checkWin32(::GetShortPathName(path, shortPath, _countof(shortPath)));
					path = shortPath;
				}
				debugPrint(L"%s\n", path);
				fprintf_s(fp, "%S\n", path);
			});
		}

		if (fclose(fp))
		{
			throw std::system_error(errno, std::system_category());
		}

		SHELLEXECUTEINFO sei = { sizeof(sei), };
		sei.lpVerb = L"open";
		sei.lpFile = fname;
		sei.nShow = SW_SHOWNORMAL;
		::ShellExecuteEx(&sei);
	}
}

std::wstring str2wstr(LPCSTR source, size_t size = _TRUNCATE, bool throwIfError = false)
{
	WCHAR buf[256];
	size_t conv;
	auto result = mbstowcs_s(&conv, buf, source, size);
	if (throwIfError && result != 0)
	{
		throw std::system_error(result, std::system_category());
	}

	return std::wstring{ buf, conv };
}

int APIENTRY _tWinMain(HINSTANCE,
	HINSTANCE,
	LPTSTR /*lpCmdLine*/,
	int /*nCmdShow*/)
{
	std::locale::global(std::locale("", LC_CTYPE));
	_tsetlocale(LC_CTYPE, _T(""));

	LPWSTR *szArglist;
	int nArgs;
	szArglist = ::CommandLineToArgvW(::GetCommandLineW(), &nArgs);

	try
	{
		CCoInitialize init;
		main(nArgs, szArglist);
	}
	catch (const _com_error& err)
	{
		::MessageBox(nullptr, err.ErrorMessage(), szArglist[0], MB_OK | MB_ICONERROR);
	}
	catch (const std::system_error& err)
	{
		auto msg = str2wstr(err.what());
		::MessageBox(nullptr, msg.c_str(), szArglist[0], MB_OK | MB_ICONERROR);
	}

	return 0;
}
