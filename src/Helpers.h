/******************************************************************************
*
*
* Notepad2
*
* Helpers.h
*   Definitions for general helper functions and macros
*
* See Readme.txt for more information about this source code.
* Please send me your comments to this work.
*
* See License.txt for details about distribution and modification.
*
*                                              (c) Florian Balmer 1996-2011
*                                                  florian.balmer@gmail.com
*                                               http://www.flos-freeware.ch
*
*
******************************************************************************/

#ifndef _NOTEPAD2_HELPERS_H_
#define _NOTEPAD2_HELPERS_H_

#define COUNTOF(ar)		(sizeof(ar)/sizeof(ar[0]))
#define CSTRLEN(s)		(COUNTOF(s)-1)

#ifdef NDEBUG
#if defined(_MSC_VER) && _MSC_VER < 1600
static __inline void DLog(const char* fmt, ...) {fmt;}
#else
#define DLog(fmt, ...)
#endif
#else
void DLog(const char* fmt, ...);
#endif

extern HINSTANCE g_hInstance;
extern HANDLE g_hDefaultHeap;
extern UINT16 g_uWinVer;
extern WCHAR szIniFile[MAX_PATH];

#define IsWin2KAndAbove()	(g_uWinVer >= 0x0500)
#define IsWinXPAndAbove()	(g_uWinVer >= 0x0501)
#define IsVistaAndAbove()	(g_uWinVer >= 0x0600)
#define IsWin7AndAbove()	(g_uWinVer >= 0x0601)

#define NP2HeapAlloc(size)			HeapAlloc(g_hDefaultHeap, HEAP_ZERO_MEMORY, size)
#define NP2HeapFree(hMem)			HeapFree(g_hDefaultHeap, 0, hMem)
#define NP2HeapReAlloc(hMem, size)	HeapReAlloc(g_hDefaultHeap, HEAP_ZERO_MEMORY, hMem, size)

#define IniGetString(lpSection, lpName, lpDefault, lpReturnedStr, nSize) \
	GetPrivateProfileString(lpSection, lpName, lpDefault, lpReturnedStr, nSize, szIniFile)
#define IniGetInt(lpSection, lpName, nDefault) \
	GetPrivateProfileInt(lpSection, lpName, nDefault, szIniFile)
#define IniSetString(lpSection, lpName, lpString) \
	WritePrivateProfileString(lpSection, lpName, lpString, szIniFile)
#define IniDeleteSection(lpSection) \
	WritePrivateProfileSection(lpSection, NULL, szIniFile)
__inline BOOL IniSetInt(LPCWSTR lpSection, LPCWSTR lpName, int i)
{
	WCHAR tch[32];
	wsprintf(tch, L"%i", i);
	return IniSetString(lpSection, lpName, tch);
}
#define LoadIniSection(lpSection, lpBuf, cchBuf) \
	GetPrivateProfileSection(lpSection, lpBuf, cchBuf, szIniFile);
#define SaveIniSection(lpSection, lpBuf) \
	WritePrivateProfileSection(lpSection, lpBuf, szIniFile)
int		IniSectionGetString(LPCWSTR lpCachedIniSection, LPCWSTR lpName, LPCWSTR lpDefault,
							LPWSTR lpReturnedString, int cchReturnedString);
int		IniSectionGetInt(LPCWSTR lpCachedIniSection, LPCWSTR lpName, int iDefault);
BOOL	IniSectionGetBool(LPCWSTR lpCachedIniSection, LPCWSTR lpName, BOOL bDefault);
BOOL	IniSectionSetString(LPWSTR lpCachedIniSection, LPCWSTR lpName, LPCWSTR lpString);
__inline BOOL IniSectionSetInt(LPWSTR lpCachedIniSection, LPCWSTR lpName, int i)
{
	WCHAR tch[32];
	wsprintf(tch, L"%i", i);
	return IniSectionSetString(lpCachedIniSection, lpName, tch);
}
__inline BOOL IniSectionSetBool(LPWSTR lpCachedIniSection, LPCWSTR lpName, BOOL b)
{
	return IniSectionSetString(lpCachedIniSection, lpName, (b? L"1" : L"0"));
}

extern HWND hwndEdit;
__inline void BeginWaitCursor()
{
	SendMessage(hwndEdit, SCI_SETCURSOR, (WPARAM)SC_CURSORWAIT, 0);
}
__inline void EndWaitCursor()
{
	POINT pt;
	SendMessage(hwndEdit, SCI_SETCURSOR, (WPARAM)SC_CURSORNORMAL, 0);
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
}

BOOL PrivateIsAppThemed();
HRESULT PrivateSetCurrentProcessExplicitAppUserModelID(PCWSTR AppID);
BOOL IsElevated();
//BOOL SetExplorerTheme(HWND);


BOOL BitmapMergeAlpha(HBITMAP hbmp, COLORREF crDest);
BOOL BitmapAlphaBlend(HBITMAP hbmp, COLORREF crDest, BYTE alpha);
BOOL BitmapGrayScale(HBITMAP hbmp);
BOOL VerifyContrast(COLORREF cr1, COLORREF cr2);
BOOL IsFontAvailable(LPCWSTR lpszFontName);

void SetClipDataW(HWND hwnd, WCHAR *pszData);
BOOL SetWindowTitle(HWND hwnd, UINT uIDAppName, BOOL bIsElevated, UINT uIDUntitled,
					LPCWSTR lpszFile, int iFormat, BOOL bModified,
					UINT uIDReadOnly, BOOL bReadOnly, LPCWSTR lpszExcerpt);
void SetWindowTransparentMode(HWND hwnd, BOOL bTransparentMode);

void CenterDlgInParent(HWND hDlg);
void GetDlgPos(HWND hDlg, LPINT xDlg, LPINT yDlg);
void SetDlgPos(HWND hDlg, int xDlg, int yDlg);
void ResizeDlg_Init(HWND hwnd, int cxFrame, int cyFrame, int nIdGrip);
void ResizeDlg_Destroy(HWND hwnd, int *cxFrame, int *cyFrame);
void ResizeDlg_Size(HWND hwnd, LPARAM lParam, int *cx, int *cy);
void ResizeDlg_GetMinMaxInfo(HWND hwnd, LPARAM lParam);
HDWP DeferCtlPos(HDWP hdwp, HWND hwndDlg, int nCtlId, int dx, int dy, UINT uFlags);
void MakeBitmapButton(HWND hwnd, int nCtlId, HINSTANCE hInstance, UINT uBmpId);
void MakeColorPickButton(HWND hwnd, int nCtlId, HINSTANCE hInstance, COLORREF crColor);
void DeleteBitmapButton(HWND hwnd, int nCtlId);


#define StatusSetSimple(hwnd, b) SendMessage(hwnd, SB_SIMPLE, (WPARAM)b, 0)
BOOL StatusSetText(HWND hwnd, UINT nPart, LPCWSTR lpszText);
BOOL StatusSetTextID(HWND hwnd, UINT nPart, UINT uID);
int  StatusCalcPaneWidth(HWND hwnd, LPCWSTR lpsz);

int Toolbar_GetButtons(HWND hwnd, int cmdBase, LPWSTR lpszButtons, int cchButtons);
int Toolbar_SetButtons(HWND hwnd, int cmdBase, LPCWSTR lpszButtons, LPCTBBUTTON ptbb, int ctbb);

LRESULT SendWMSize(HWND hwnd);

#define EnableCmd(hmenu, id, b)	EnableMenuItem(hmenu, id, (b)? (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND | MF_GRAYED))
#define CheckCmd(hmenu, id, b)	CheckMenuItem(hmenu, id, (b)? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED))

BOOL IsCmdEnabled(HWND hwnd, UINT uId);


#define GetString(id, pb, cb) LoadString(g_hInstance, id, pb, cb)

#define StrEnd(pStart) (pStart + lstrlen(pStart))

int FormatString(LPWSTR lpOutput, int nOutput, UINT uIdFormat, ...);


void PathRelativeToApp(LPWSTR lpszSrc, LPWSTR lpszDest, int cchDest,
					   BOOL bSrcIsFile, BOOL bUnexpandEnv, BOOL bUnexpandMyDocs);
void PathAbsoluteFromApp(LPWSTR lpszSrc, LPWSTR lpszDest, int cchDest, BOOL bExpandEnv);


BOOL PathIsLnkFile(LPCWSTR pszPath);
BOOL PathGetLnkPath(LPCWSTR pszLnkFile, LPWSTR pszResPath, int cchResPath);
BOOL PathIsLnkToDirectory(LPCWSTR pszPath, LPWSTR pszResPath, int cchResPath);
BOOL PathCreateDeskLnk(LPCWSTR pszDocument);
BOOL PathCreateFavLnk(LPCWSTR pszName, LPCWSTR pszTarget, LPCWSTR pszDir);


BOOL StrLTrim(LPWSTR pszSource, LPCWSTR pszTrimChars);
BOOL TrimString(LPWSTR lpString);
BOOL ExtractFirstArgument(LPCWSTR lpArgs, LPWSTR lpArg1, LPWSTR lpArg2);

void PrepareFilterStr(LPWSTR lpFilter);

void	StrTab2Space(LPWSTR lpsz);
void	PathFixBackslashes(LPWSTR lpsz);


void	ExpandEnvironmentStringsEx(LPWSTR lpSrc, DWORD dwSrc);
void	PathCanonicalizeEx(LPWSTR lpSrc);
DWORD	GetLongPathNameEx(LPWSTR lpszPath, DWORD cchBuffer);
DWORD_PTR SHGetFileInfo2(LPCWSTR pszPath, DWORD dwFileAttributes,
						SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags);


int		FormatNumberStr(LPWSTR lpNumberStr);
BOOL	SetDlgItemIntEx(HWND hwnd, int nIdItem, UINT uValue);


#define MBCSToWChar(c, a, w, i) MultiByteToWideChar(c, 0, a, -1, w, i)
#define WCharToMBCS(c, w, a, i) WideCharToMultiByte(c, 0, w, -1, a, i, NULL, NULL)

UINT	GetDlgItemTextA2W(UINT uCP, HWND hDlg, int nIDDlgItem, LPSTR lpString, int nMaxCount);
UINT	SetDlgItemTextA2W(UINT uCP, HWND hDlg, int nIDDlgItem, LPSTR lpString);
LRESULT ComboBox_AddStringA2W(UINT uCP, HWND hwnd, LPCSTR lpString);


UINT CodePageFromCharSet(UINT uCharSet);


//==== MRU Functions ==========================================================
#define MRU_MAXITEMS	24
#define MRU_NOCASE		1
#define MRU_UTF8		2

typedef struct _mrulist {
	WCHAR	szRegKey[256];
	int		iFlags;
	int		iSize;
	LPWSTR pszItems[MRU_MAXITEMS];
} MRULIST,  *PMRULIST,  *LPMRULIST;

LPMRULIST MRU_Create(LPCWSTR pszRegKey, int iFlags, int iSize);
BOOL	MRU_Destroy(LPMRULIST pmru);
BOOL	MRU_Add(LPMRULIST pmru, LPCWSTR pszNew);
BOOL	MRU_AddFile(LPMRULIST pmru, LPCWSTR pszFile, BOOL bRelativePath, BOOL bUnexpandMyDocs);
BOOL	MRU_Delete(LPMRULIST pmru, int iIndex);
BOOL	MRU_DeleteFileFromStore(LPMRULIST pmru, LPCWSTR pszFile);
BOOL	MRU_Empty(LPMRULIST pmru);
int 	MRU_Enum(LPMRULIST pmru, int iIndex, LPWSTR pszItem, int cchItem);
BOOL	MRU_Load(LPMRULIST pmru);
BOOL	MRU_Save(LPMRULIST pmru);
BOOL	MRU_MergeSave(LPMRULIST pmru, BOOL bAddFiles, BOOL bRelativePath, BOOL bUnexpandMyDocs);

//==== Themed Dialogs =========================================================
#ifndef DLGTEMPLATEEX
#pragma pack(push,  1)
typedef struct {
	WORD	dlgVer;
	WORD	signature;
	DWORD	helpID;
	DWORD	exStyle;
	DWORD	style;
	WORD	cDlgItems;
	short	x;
	short	y;
	short	cx;
	short	cy;
} DLGTEMPLATEEX;
#pragma pack(pop)
#endif

BOOL	GetThemedDialogFont(LPWSTR lpFaceName, WORD *wSize);
DLGTEMPLATE *LoadThemedDialogTemplate(LPCTSTR lpDialogTemplateID, HINSTANCE hInstance);
#define ThemedDialogBox(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
	ThemedDialogBoxParam(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0)
INT_PTR ThemedDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent,
							 DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND	CreateThemedDialogParam(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent,
								DLGPROC lpDialogFunc, LPARAM dwInitParam);

//==== UnSlash Functions ======================================================
void TransformBackslashes(char *pszInput, BOOL bRegEx, UINT cpEdit);
BOOL AddBackslash(char *pszOut, const char *pszInput);

//==== MinimizeToTray Functions - see comments in Helpers.c ===================
BOOL GetDoAnimateMinimize(VOID);
VOID MinimizeWndToTray(HWND hWnd);
VOID RestoreWndFromTray(HWND hWnd);

#endif // _NOTEPAD2_HELPERS_H_

// End of Helpers.h