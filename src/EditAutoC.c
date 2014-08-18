// Edit AutoComplete

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x501
#endif
#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include "Edit.h"
#include "Styles.h"
#include "Helpers.h"
#include "SciCall.h"
#include "Resource.h"

__forceinline BOOL IsASpace(int ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}
__forceinline BOOL IsAAlpha(int ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}
__forceinline BOOL IsWordStart(int ch) {
	return ch != '.' && IsDocWordChar(ch);
}
__forceinline BOOL IsEscapeChar(int ch) {
	return ch == 't' || ch == 'n' || ch == 'r' || ch == 'a' || ch == 'b' || ch == 'v' || ch == 'f'
		|| ch == '$'; // PHP
	// x u U
}

__forceinline BOOL IsCppCommentStyle(int style) {
	return style == SCE_C_COMMENT || style == SCE_C_COMMENTLINE ||
	style == SCE_C_COMMENTDOC || style == SCE_C_COMMENTLINEDOC ||
	style == SCE_C_COMMENTDOC_TAG || style == SCE_C_COMMENTDOC_TAG_XML;
}
__forceinline BOOL IsCppStringStyle(int style) {
	return style == SCE_C_STRING || style == SCE_C_CHARACTER || style == SCE_C_STRINGEOL || style == SCE_C_STRINGRAW
		|| style == SCE_C_VERBATIM || style == SCE_C_DSTRINGX || style == SCE_C_DSTRINGQ || style == SCE_C_DSTRINGT;
}
__forceinline BOOL IsSpecialStartChar(int ch, int chPrev) {
	return (ch == '.')	// member
		|| (ch == '#')	// preprocessor
		|| (ch == '@') // Java/PHP/Doxygen Doc Tag
		// ObjC Keyword, Java Annotation, Python Decorator, Cobra Directive
		|| (ch == '<') // HTML/XML Tag, C# Doc Tag
		|| (ch == '\\')// Doxygen Doc Tag, LaTeX Command
		|| (chPrev == '<' && ch == '/')	// HTML/XML Close Tag
		|| (chPrev == '-' && ch == '>')	// member(C/C++/PHP)
		|| (chPrev == ':' && ch == ':');// namespace(C++), static member(C++/Java8/PHP)
}


#include "EditAutoC_Data0.c"
#include "EditAutoC_WordList.c"

//=============================================================================
//
// EditCompleteWord()
// Auto-complete words
//

// item count in AutoComplete List
extern int	iAutoCItemCount;
extern BOOL	bAutoCompleteWords;
extern int	iAutoCDefaultShowItemCount;
extern int	iAutoCMinWordLength;
extern int	iAutoCMinNumberLength;
extern BOOL bAutoCIncludeDocWord;


__inline BOOL IsWordStyleToIgnore(int style) {
	switch (pLexCurrent->iLexer) {
	case SCLEX_CPP:
		return style == SCE_C_WORD || style == SCE_C_WORD2 || style == SCE_C_PREPROCESSOR;
	case SCLEX_PYTHON:
		return style == SCE_PY_WORD || style == SCE_PY_WORD2 || style == SCE_PY_BUILDIN_CONST || style == SCE_PY_BUILDIN_FUNC || style == SCE_PY_ATTR || style == SCE_PY_OBJ_FUNC;
	case SCLEX_JSON:
		return style == SCE_C_WORD;
	case SCLEX_SQL:
		return style == SCE_SQL_WORD || style == SCE_SQL_WORD2 || style == SCE_SQL_USER1 || style == SCE_SQL_HEX || style == SCE_SQL_HEX2;
	case SCLEX_SMALI:
		return style == SCE_SMALI_WORD || style == SCE_SMALI_DIRECTIVE || style == SCE_SMALI_INSTRUCTION;
	}
	return FALSE;
}

void AutoC_AddDocWord(HWND hwnd, struct WordList *pWList, BOOL bIgnore)
{
	LPCSTR pRoot = pWList->pWordStart;
	int iRootLen = pWList->iStartLen;
	struct Sci_TextToFind ft = {{0, 0}, 0, {0, 0}};
	struct Sci_TextRange tr = { { 0, -1 }, NULL };
	int iCurrentPos = SciCall_GetCurrentPos() - iRootLen;
	int iDocLen = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	int findFlag = bIgnore? SCFIND_WORDSTART : (SCFIND_WORDSTART | SCFIND_MATCHCASE);
	int iPosFind;
	char wordBuf[NP2_AUTOC_MAX_WORD_LENGTH + 3 + 1];

	ft.lpstrText = pRoot;
	ft.chrg.cpMax = iDocLen;
	iPosFind = (int)SendMessage(hwnd, SCI_FINDTEXT, findFlag, (LPARAM)&ft);

	while (iPosFind >= 0 && iPosFind < iDocLen) {
		int wordEnd = iPosFind + iRootLen;
		int style = SciCall_GetStyleAt(wordEnd - 1);
		if (iPosFind != iCurrentPos && !IsWordStyleToIgnore(style)) {
#ifdef NDEBUG
			int chPrev, ch = *pRoot, chNext = SciCall_GetCharAt(wordEnd);
#else
			char chPrev, ch = *pRoot, chNext = SciCall_GetCharAt(wordEnd);
#endif
			int wordLength = -iPosFind;
			BOOL bSubWord = FALSE;
			while (wordEnd < iDocLen) {
				chPrev = ch;
				ch = chNext;
				chNext = SciCall_GetCharAt(wordEnd + 1);
				if (!IsDocWordChar(ch) || (ch == ':' && pLexCurrent->iLexer == SCLEX_CPP)) {
					if ((ch == ':' && chNext == ':' && IsDocWordChar(chPrev))
					|| (chPrev == ':' && ch == ':' && IsDocWordChar(chNext))
					|| (ch == '-' && chNext == '>' && IsDocWordChar(chPrev))
					|| (chPrev == '-' && ch == '>' && IsDocWordChar(chNext))
					|| (ch == '-' && !IsOperatorStyle(SciCall_GetStyleAt(wordEnd)))
					)
						bSubWord = TRUE;
					else
						break;
				}
				if (ch == '.' || ch == ':') bSubWord = TRUE;
				wordEnd++;
			}
			wordLength += wordEnd;

			if (wordLength >= iRootLen) {
				char* pWord = wordBuf;
				tr.lpstrText = pWord;
				tr.chrg.cpMin = iPosFind;
				tr.chrg.cpMax = wordEnd;
				if (wordLength > NP2_AUTOC_MAX_WORD_LENGTH) {
					tr.chrg.cpMax = iPosFind + NP2_AUTOC_MAX_WORD_LENGTH;
				}
				SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

				ch = SciCall_GetCharAt(iPosFind - 1);
				// word after escape char
				chPrev = SciCall_GetCharAt(iPosFind - 2);
				if (chPrev != '\\' && ch == '\\' && IsEscapeChar(*pWord)) {
					pWord++;
					--wordLength;
				}
				//if (pLexCurrent->rid == NP2LEX_PHP && wordLength >= 2 && *pWord == '$' && *(pWord+1) == '$') {
				//	pWord++;
				//	--wordLength;
				//}
				while (wordLength > 0 && (pWord[wordLength - 1] == '-' || pWord[wordLength - 1] == ':' || pWord[wordLength - 1] == '.')) {
					--wordLength;
					pWord[wordLength] = '\0';
				}
				if (wordLength > 0 && IsWordStart(*pWord) && !(*pWord == ':' && *(pWord + 1) != ':')) {
					if (!(pLexCurrent->iLexer == SCLEX_CPP && style == SCE_C_MACRO)) {
						while (IsASpace(SciCall_GetCharAt(wordEnd))) wordEnd++;
					}
					if (SciCall_GetCharAt(wordEnd) == '(') {
						pWord[wordLength++] = '(';
						pWord[wordLength++] = ')';
					//} else if (SciCall_GetCharAt(wordEnd) == '[') {
					//	pWord[wordLength++] = '[';
					//	pWord[wordLength++] = ']';
					}
					if (wordLength >= iRootLen) {
						if (bSubWord && !(*pWord >= '0' && *pWord <= '9')) {
							int i;
							ch = 0,  chNext = *pWord;
							for (i = 0; i < wordLength-1; i++) {
								chPrev = ch;
								ch = chNext;
								chNext = pWord[i + 1];
								if (i >= iRootLen && (ch == '.' || ch == ':' || ch == '-')
								&& !(chPrev == '.' || chPrev == ':' || chPrev == '-')
								&& !(ch == '-' && (chNext >= '0' && chNext <= '9'))) {
									pWord[i] = '\0';
									WordList_AddWord(pWList, pWord, i);
									pWord[i] = (char)ch;
								}
							}
						}
						pWord[wordLength] = '\0';
						WordList_AddWord(pWList, pWord, wordLength);
					}
				}
			}
		}
		ft.chrg.cpMin = wordEnd;
		iPosFind = (int)SendMessage(hwnd, SCI_FINDTEXT, findFlag, (LPARAM)&ft);
	}
}

void AutoC_AddKeyword(struct WordList *pWList, int iCurrentStyle)
{
	int i, count;

	switch (pLexCurrent->rid) {
	case NP2LEX_CPP:
	case NP2LEX_CLI:
		count = 11;
		break;	// CPP
	case NP2LEX_PHP:
	case NP2LEX_JS:
	case NP2LEX_PYTHON:
		count = NUMKEYWORD;
		break;
	case NP2LEX_ASM:
		count = 10;
		break;	// ASM
	case NP2LEX_CSHARP:
	case NP2LEX_JAVA:
	case NP2LEX_SMALI:
		count = 12;
		break;
	default:
		count = 9;
	}

	WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[0]);
	if (pLexCurrent->rid != NP2LEX_JS) { // Reserved Word
		WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[1]);
	}
	i = 2;
	if (pLexCurrent->iLexer == SCLEX_CPP || pLexCurrent->iLexer == SCLEX_PYTHON) {
		i = 4;
		if (pLexCurrent->rid == NP2LEX_CSHARP || pLexCurrent->rid == NP2LEX_PHP)
			i = 3;
	}
	if (pLexCurrent->iLexer == SCLEX_PYTHON) {
		WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[2]);
	}
	for ( ; i < count; i++) {
		const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[i];
		if (*pKeywords) {
			WordList_AddList(pWList, pKeywords);
		}
	}
	if (pLexCurrent->rid == NP2LEX_CPP) {
		i = 13;
		for ( ; i < NUMKEYWORD; i++) {
			const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[i];
			if (*pKeywords) {
				WordList_AddList(pWList, pKeywords);
			}
		}
	}

	// additional keywords
	if (np2_LexKeyword && !(pLexCurrent->iLexer == SCLEX_CPP && !IsCppCommentStyle(iCurrentStyle))) {
		WordList_AddList(pWList, (*np2_LexKeyword)[0]);
		WordList_AddList(pWList, (*np2_LexKeyword)[1]);
		WordList_AddList(pWList, (*np2_LexKeyword)[2]);
		WordList_AddList(pWList, (*np2_LexKeyword)[3]);
	}
	// keywords with paranthesis
	{
		const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[NUMKEYWORD - 1];
		if (*pKeywords) {
			WordList_AddList(pWList, pKeywords);
		}
	}
}

INT AutoC_AddSpecWord(struct WordList *pWList, int iCurrentStyle, int ch, int chPrev)
{
	if (pLexCurrent->iLexer == SCLEX_CPP && IsCppCommentStyle(iCurrentStyle) && np2_LexKeyword) {
		if ((ch == '@' && (*np2_LexKeyword == kwJavaDoc || *np2_LexKeyword == kwPHPDoc || *np2_LexKeyword == kwDoxyDoc))
			|| (ch == '\\' && *np2_LexKeyword == kwDoxyDoc)
			|| ((ch == '<' || chPrev == '<') && *np2_LexKeyword == kwNETDoc)) {
			WordList_AddList(pWList, (*np2_LexKeyword)[0]);
			WordList_AddList(pWList, (*np2_LexKeyword)[1]);
			WordList_AddList(pWList, (*np2_LexKeyword)[2]);
			WordList_AddList(pWList, (*np2_LexKeyword)[3]);
			return 0;
		} else if (ch == '#' && (*np2_LexKeyword == kwJavaDoc) && IsWordStart(*(pWList->pWordStart))) { // package.Class#member
			return 1;
		}
	}
	else if ((pLexCurrent->rid == NP2LEX_HTML || pLexCurrent->rid == NP2LEX_XML || pLexCurrent->rid == NP2LEX_CONF)
		&& ((ch == '<') || (chPrev == '<' && ch == '/'))) {
		if (pLexCurrent->rid == NP2LEX_HTML || pLexCurrent->rid == NP2LEX_CONF) { // HTML Tag
			WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[0]);// HTML Tag
		} else {
			WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[3]);// XML Tag
			if (np2_LexKeyword) { // XML Tag
				WordList_AddList(pWList, (*np2_LexKeyword)[0]);
			}
		}
		return 0;
	}
	else if ((pLexCurrent->iLexer == SCLEX_CPP && iCurrentStyle == SCE_C_DEFAULT)
		|| (pLexCurrent->iLexer == SCLEX_PYTHON && iCurrentStyle == SCE_PY_DEFAULT)
		|| (pLexCurrent->iLexer == SCLEX_SMALI && iCurrentStyle == SCE_C_DEFAULT)) {
		if (ch == '#' && pLexCurrent->iLexer == SCLEX_CPP) { // #preprocessor
			const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[2];
			if (pKeywords && *pKeywords) {
				WordList_AddList(pWList, pKeywords);
				return 0;
			}
		} else if (ch == '@') { // @directive, @annotation, @decorator
			if (pLexCurrent->rid == NP2LEX_CSHARP) { // verbatim identifier
				//WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[0]);
				//WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[1]);
				//return 0;
			} else {
				const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[3];
				if (pKeywords && *pKeywords) {
					WordList_AddList(pWList, pKeywords);
					return 0; // user defined annotation
				}
			}
		}
		else if (ch == '.' && pLexCurrent->iLexer == SCLEX_SMALI) {
			WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[9]);
			return 0;
		}
		//else if (chPrev == ':' && ch == ':') {
		//	WordList_AddList(pWList, "C++/namespace C++/Java8/PHP/static SendMessage()");
		//	return 0;
		//}
		//else if (chPrev == '-' && ch == '>') {
		//	WordList_AddList(pWList, "C/C++pointer PHP-variable");
		//	return 0;
		//}
	}
	return 1;
}

void EditCompleteWord(HWND hwnd, BOOL autoInsert)
{
	const int iCurrentPos = SciCall_GetCurrentPos();
	const int iCurrentStyle = SciCall_GetStyleAt(iCurrentPos);
	int iLine = SciCall_LineFromPosition(iCurrentPos);
	int iCurrentLinePos = iCurrentPos - SciCall_PositionFromLine(iLine);
	int iStartWordPos = iCurrentLinePos;

	char *pLine;
	char *pRoot = NULL;
	char *pSubRoot = NULL;
	int iRootLen;
	int iDocLen;
#ifdef NDEBUG
	int ch = 0, chPrev = 0;
#else
	char ch = 0, chPrev = 0;
#endif

	BOOL bIgnore = FALSE; // ignore number
	struct WordList *pWList = NULL;

	iAutoCItemCount = 0; // recreate list

	iDocLen = SciCall_GetLine(iLine, NULL);  // get length
	pLine = NP2HeapAlloc(iDocLen + 1);
	SciCall_GetLine(iLine, pLine);
	iRootLen = iAutoCMinWordLength;

	while (iStartWordPos > 0 && IsDocWordChar(pLine[iStartWordPos - 1])) {
		iStartWordPos--;
		if (iStartWordPos >= 0 && IsSpecialStartChar(pLine[iStartWordPos], '\0')) {
			iStartWordPos++;
			break;
		}
	}
	if (iStartWordPos >= 0 && pLine[iStartWordPos] == ':') {
		if (iStartWordPos < iDocLen && pLine[iStartWordPos + 1] != ':')
			iStartWordPos++;
	}
	if (iStartWordPos >= 0 && pLine[iStartWordPos] >= '0' && pLine[iStartWordPos] <= '9') {
		if (iAutoCMinNumberLength <= 0) {
			bIgnore = TRUE;
		} else {
			iRootLen = iAutoCMinNumberLength;
			if (iStartWordPos < iDocLen && !(pLine[iStartWordPos + 1] >= '0' && pLine[iStartWordPos + 1] <= '9'))
				iRootLen += 2;
		}
	}
	// word after escape char
	if (iStartWordPos > 1 && pLine[iStartWordPos - 1] == '\\' && IsEscapeChar(pLine[iStartWordPos])) {
		if (!(iStartWordPos > 2 && pLine[iStartWordPos - 2] == '\\'))
			++iStartWordPos;
	}

	if (iStartWordPos > 0) {
		iLine = iStartWordPos - 1;
		ch = pLine[iLine];
		if (iLine > 0)
			chPrev = pLine[iLine - 1];
		if (pLexCurrent->rid == NP2LEX_CPP && (ch == '#' || IsASpace(ch))) {
			while (iLine >= 0 && IsASpace(ch)) {
				ch = pLine[iLine--];
			}
			while (iLine >= 0 && IsASpace(pLine[iLine--]));
			if (iLine > 0) {
				ch = '\0';
				chPrev = '\0';
			}
		}
	}

	if (iStartWordPos == iCurrentLinePos || bIgnore || (iCurrentLinePos - iStartWordPos < iRootLen)) {
		if (!IsSpecialStartChar(ch, chPrev)) {
			NP2HeapFree(pLine);
			return;
		}
	}

	if (iRootLen) {
		pRoot = NP2HeapAlloc(iCurrentLinePos - iStartWordPos + iRootLen);
		lstrcpynA(pRoot, pLine + iStartWordPos, iCurrentLinePos - iStartWordPos + 1);
		iRootLen = lstrlenA(pRoot);
		if (!iRootLen) {
			NP2HeapFree(pRoot);
			NP2HeapFree(pLine);
			return;
		}
		// word started with number
		if (!autoInsert && !IsWordStart(*pRoot)) {
			if (!IsSpecialStartChar(ch, chPrev)) {
				NP2HeapFree(pRoot);
				NP2HeapFree(pLine);
				return;
			}
		}
		pSubRoot = pRoot;
	}

	bIgnore = (iStartWordPos >= 0 && pLine[iStartWordPos] >= '0' && pLine[iStartWordPos] <= '9');
	pWList = (struct WordList *)NP2HeapAlloc(sizeof(struct WordList));
	pWList->nWordCount = 0;
	pWList->pWordStart = pRoot;
	pWList->iStartLen = iRootLen;
	pWList->iMaxLength = iRootLen;
#if NP2_AUTOC_USE_BUF
	pWList->capacity = NP2_AUTOC_INIT_BUF_SIZE;
	pWList->buffer = NP2HeapAlloc(NP2_AUTOC_INIT_BUF_SIZE);
#endif
	if (bIgnore) {
		pWList->WL_StrCmpA = StrCmpIA;
		pWList->WL_StrCmpNA = StrCmpNIA;
		goto label_add_doc_word;
	} else {
		pWList->WL_StrCmpA = StrCmpA;
		pWList->WL_StrCmpNA = StrCmpNA;
	}
//#ifndef NDEBUG
//goto label_add_doc_word;
//#endif

	if (IsSpecialStartChar(ch, chPrev)) {
		if (!AutoC_AddSpecWord(pWList, iCurrentStyle, ch, chPrev) && pWList->nWordCount > 0) {
			goto label_show_word_list;
		}
		pSubRoot = StrPBrkA(pRoot, ":.#@<\\/->");
		if (pSubRoot) {
			pSubRoot = pRoot;
			goto label_show_word_list;
		}
	}

label_retry:
	// keywords
	AutoC_AddKeyword(pWList, iCurrentStyle);
label_add_doc_word:
	if (bAutoCIncludeDocWord) {
		AutoC_AddDocWord(hwnd, pWList, bIgnore);
	}
label_show_word_list:
	iAutoCItemCount = pWList->nWordCount;
	if (iAutoCItemCount == 0 && pSubRoot && *pSubRoot) {
		pSubRoot = StrPBrkA(pSubRoot, ":.#@<\\/->");
		if (pSubRoot) {
			while (*pSubRoot && StrChrA(":.#@<\\/->", *pSubRoot)) pSubRoot++;
		}
		if (pSubRoot && *pSubRoot) {
			pWList->pWordStart = pSubRoot;
			pWList->iStartLen = lstrlenA(pSubRoot);
			pWList->iMaxLength = pWList->iStartLen;
			goto label_retry;
		}
	}
	if (iAutoCItemCount > 0) {
		char *pList = NULL;
		int maxWordLength = pWList->iMaxLength;
		if (iAutoCItemCount == 1 && maxWordLength == iRootLen) {
			WordList_Free(pWList);
			goto end;
		}
		WordList_GetList(pWList, &pList);
		//DLog(pList);
		SendMessage(hwnd, SCI_AUTOCSETORDER, SC_ORDER_PRESORTED, 0); // pre-sorted
		SendMessage(hwnd, SCI_AUTOCSETIGNORECASE, 1, 0); // case insensitive
		SendMessage(hwnd, SCI_AUTOCSETSEPARATOR, '\n', 0);
		SendMessage(hwnd, SCI_AUTOCSETFILLUPS, 0, (LPARAM)" \t\n\r;,([])");
		SendMessage(hwnd, SCI_AUTOCSETCHOOSESINGLE, 0, 0);
		//SendMessage(hwnd, SCI_AUTOCSETDROPRESTOFWORD, 1, 0); // delete orginal text: pRoot
		maxWordLength <<= 1;
		SendMessage(hwnd, SCI_AUTOCSETMAXWIDTH, maxWordLength, 0); // width columns, default auto
		maxWordLength = iAutoCItemCount;
		if (maxWordLength > iAutoCDefaultShowItemCount)
			maxWordLength = iAutoCDefaultShowItemCount;
		SendMessage(hwnd, SCI_AUTOCSETMAXHEIGHT, maxWordLength, 0); // height rows, default 5
		SendMessage(hwnd, SCI_AUTOCSHOW, pWList->iStartLen, (LPARAM)(pList));
		NP2HeapFree(pList);
	}
end:
	NP2HeapFree(pLine);
	NP2HeapFree(pRoot);
	NP2HeapFree(pWList);
}


void EditAutoCloseBraceQuote(HWND hwnd, int ch)
{
	int iCurPos = SciCall_GetCurrentPos();
	int chPrev = SciCall_GetCharAt(iCurPos - 2);
	int chNext = SciCall_GetCharAt(iCurPos);
	int iCurrentStyle = SciCall_GetStyleAt(iCurPos - 2);
	int iNextStyle = SciCall_GetStyleAt(iCurPos);
	char tchIns[2] = "";
	if (pLexCurrent->iLexer == SCLEX_CPP) {
		// within char
		if (iCurrentStyle == SCE_C_CHARACTER && iNextStyle == SCE_C_CHARACTER && pLexCurrent->rid != NP2LEX_PHP) {
			return;
		}
		if (ch == '`' && !IsCppStringStyle(iCurrentStyle)) {
			return;
		}
	}
	switch (ch) {
	case '(':
		tchIns[0] = ')';
		break;
	case '[':
		if (!(pLexCurrent->rid == NP2LEX_SMALI)) { // Smali array type
			tchIns[0] = ']';
		}
		break;
	case '{':
		tchIns[0] = '}';
		break;
	case '<':
		if (pLexCurrent->rid == NP2LEX_CPP || pLexCurrent->rid == NP2LEX_CSHARP || pLexCurrent->rid == NP2LEX_JAVA) {
			// geriatric type, template
			if (iCurrentStyle == SCE_C_CLASS || iCurrentStyle == SCE_C_INTERFACE || iCurrentStyle ==  SCE_C_STRUCT)
				tchIns[0] = '>';
		}
		break;
	case '\"':
		if (chPrev != '\\') {
			tchIns[0] = '\"';
		}
		break;
	case '\'':
		if (chPrev != '\\' && !(pLexCurrent->iLexer == SCLEX_NULL	// someone's
			|| pLexCurrent->iLexer == SCLEX_HTML || pLexCurrent->iLexer == SCLEX_XML
			|| pLexCurrent->iLexer == SCLEX_VB			// line comment
			|| pLexCurrent->iLexer == SCLEX_VBSCRIPT	// line comment
			|| pLexCurrent->iLexer == SCLEX_VERILOG		// inside number
			|| pLexCurrent->iLexer == SCLEX_LISP 		// operator
			)) {
			tchIns[0] = '\'';
		}
		break;
	case '`':
		//if (pLexCurrent->iLexer == SCLEX_BASH
		//|| pLexCurrent->rid == NP2LEX_JULIA
		//|| pLexCurrent->iLexer == SCLEX_MAKEFILE
		//|| pLexCurrent->iLexer == SCLEX_SQL
		//) {
		//	tchIns[0] = '`';
		//} else if (0) {
		//	tchIns[0] = '\'';
		//}
		tchIns[0] = '`';
		break;
	case ',':
		if (!(chNext == ' ' || chNext == '\t' || (chPrev == '\'' && chNext == '\'') || (chPrev == '\"' && chNext == '\"'))) {
			tchIns[0] = ' ';
		}
		break;
	default:
		break;
	}
	if (tchIns[0]) {
		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)tchIns);
		if (ch == ',')
			iCurPos++;
		SendMessage(hwnd, SCI_SETSEL, iCurPos, iCurPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}
}


void EditAutoCloseXMLTag(HWND hwnd)
{
	char tchBuf[512];
	int	 iCurPos = SciCall_GetCurrentPos();
	int	 iHelper = iCurPos - (COUNTOF(tchBuf) - 1);
	int	 iStartPos = max(0, iHelper);
	int	 iSize = iCurPos - iStartPos;
	BOOL autoClosed = FALSE;

	if (pLexCurrent->iLexer == SCLEX_CPP) {
		int iCurrentStyle = SciCall_GetStyleAt(iCurPos);
		if (iCurrentStyle == SCE_C_OPERATOR || iCurrentStyle == SCE_C_DEFAULT) {
			iHelper = FALSE;
		} else {
			int iLine = SciCall_LineFromPosition(iCurPos);
			int iCurrentLinePos = SciCall_PositionFromLine(iLine);
			while (iCurrentLinePos < iCurPos && IsASpace(SciCall_GetCharAt(iCurrentLinePos)))
				iCurrentLinePos++;
			iCurrentStyle = SciCall_GetStyleAt(iCurrentLinePos);
			if (SciCall_GetCharAt(iCurrentLinePos) == '#' && iCurrentStyle == SCE_C_PREPROCESSOR) {
				iHelper = FALSE;
			}
		}
	}

	if (iSize >= 3 && iHelper) {
		struct Sci_TextRange tr;
		tr.chrg.cpMin = iStartPos;
		tr.chrg.cpMax = iCurPos;
		tr.lpstrText = tchBuf;
		SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

		if (tchBuf[iSize - 2] != '/') {
			char tchIns[516] = "</";
			int	 cchIns = 2;
			const char *pBegin = &tchBuf[0];
			const char *pCur = &tchBuf[iSize - 2];

			while (pCur > pBegin && *pCur != '<' && *pCur != '>') {
				--pCur;
			}

			if (*pCur == '<') {
				pCur++;
				while (StrChrA(":_-.", *pCur) || IsCharAlphaNumericA(*pCur)) {
					tchIns[cchIns++] = *pCur;
					pCur++;
				}
			}

			tchIns[cchIns++] = '>';
			tchIns[cchIns] = '\0';

			iHelper = cchIns > 3;
			if (iHelper && pLexCurrent->iLexer == SCLEX_HTML) {
				iHelper =  lstrcmpiA(tchIns, "</base>")
						&& lstrcmpiA(tchIns, "</bgsound>")
						&& lstrcmpiA(tchIns, "</br>")
						&& lstrcmpiA(tchIns, "</embed>")
						&& lstrcmpiA(tchIns, "</hr>")
						&& lstrcmpiA(tchIns, "</img>")
						&& lstrcmpiA(tchIns, "</input>")
						&& lstrcmpiA(tchIns, "</link>")
						&& lstrcmpiA(tchIns, "</meta>")
						;
			}
			if (iHelper) {
				autoClosed = TRUE;
				SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
				SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)tchIns);
				SendMessage(hwnd, SCI_SETSEL, iCurPos, iCurPos);
				SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
			}
		}
	}
	if (!autoClosed && bAutoCompleteWords) {
		int iCurPos = SciCall_GetCurrentPos();
		if (SciCall_GetCharAt(iCurPos - 2) == '-') {
			EditCompleteWord(hwnd, FALSE); // obj->field, obj->method
		}
	}
}


BOOL IsIndentKeywordStyle(int style) {
	switch (pLexCurrent->iLexer) {
	case SCLEX_MATLAB:
		return style == SCE_MAT_KEYWORD;
	case SCLEX_LUA:
		return style == SCE_LUA_WORD;
	}
	return FALSE;
}
char* EditKeywordIndent(const char* head, int *indent) {
	char word[64] = "";
	int length = 0;
	char *endPart = NULL;
	*indent = 0;
	word[length++] = *head++;
	while (*head && length < 63 && IsAAlpha(*head)) {
		word[length++] = *head++;
	}
	switch (pLexCurrent->iLexer) {
	case SCLEX_CPP:
	//case SCLEX_VB:
	//case SCLEX_VBSCRIPT:
	case SCLEX_RUBY:
		break;
	case SCLEX_MATLAB:
		if (!strcmp(word, "function")) {
			*indent = 1;
		} else if (!strcmp(word, "if") || !strcmp(word, "for") || !strcmp(word, "while") || !strcmp(word, "switch") || !strcmp(word, "try")) {
			*indent = 2;
			endPart = "end";
		}
		break;
	case SCLEX_LUA:
		if (!strcmp(word, "function") || !strcmp(word, "if") || !strcmp(word, "do")) {
			*indent = 2;
			endPart = "end";
		}
		break;
	//case SCLEX_BASH:
	//case SCLEX_CMAKE:
	//case SCLEX_VHDL:
	//case SCLEX_VERILOG:
	//case SCLEX_PASCAL:
	//case SCLEX_INNOSETUP:
	//case SCLEX_NSIS:
	}
	return endPart;
}

extern int	iEOLMode;
extern BOOL	bTabsAsSpaces;
extern BOOL	bTabIndents;
extern int	iTabWidth;
extern int	iIndentWidth;
void EditAutoIndent(HWND hwnd)
{
	char *pLineBuf;
	char *pPos;

	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	//int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	int iCurLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, (WPARAM)iCurPos, 0);
	//int iLineLength = (int)SendMessage(hwnd, SCI_LINELENGTH, iCurLine, 0);
	//int iIndentBefore = (int)SendMessage(hwnd, SCI_GETLINEINDENTATION, (WPARAM)iCurLine-1, 0);

#ifdef BOOKMARK_EDITION
	// Move bookmark along with line if inserting lines (pressing return at beginning of line) because Scintilla does not do this for us
	if (iCurLine > 0) {
		int iPrevLineLength =   (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iCurLine - 1, 0) -
								(int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine - 1, 0);
		if (iPrevLineLength == 0) {
			int bitmask = (int)SendMessage(hwnd , SCI_MARKERGET , iCurLine - 1 , 0);
			if (bitmask & 1) {
				SendMessage(hwnd , SCI_MARKERDELETE , iCurLine - 1 , 0);
				SendMessage(hwnd , SCI_MARKERADD , iCurLine , 0);
			}
		}
	}
#endif

	if (iCurLine > 0/* && iLineLength <= 2*/) {
		int iPrevLineLength = (int)SendMessage(hwnd, SCI_LINELENGTH, iCurLine - 1, 0);
		if (iPrevLineLength < 2) {
			return;
		}
		pLineBuf = GlobalAlloc(GPTR, 2*iPrevLineLength + 1 + iIndentWidth*2 + 2 + 64);
		if (pLineBuf) {
			int indent = 0;
			int	iIndentLen = 0;
			int iIndentPos = iCurPos;
			char ch;
			char *endPart = NULL;
			SciCall_GetLine(iCurLine - 1, pLineBuf);
			*(pLineBuf + iPrevLineLength) = '\0';
			ch = pLineBuf[iPrevLineLength - 2];
			if (ch == '\r') {
				ch = pLineBuf[iPrevLineLength - 3];
			}
			if (ch == '{' || ch == '[' || ch == '(') {
				indent = 2;
			} else if (ch == ':') { // case label/Python
				indent = 1;
			}
			ch = SciCall_GetCharAt(SciCall_PositionFromLine(iCurLine));
			if (indent == 2 && !(ch == '}' || ch == ']' || ch == ')')) {
				indent = 1;
			} else if (!indent && (ch == '}' || ch == ']' || ch == ')')) {
				indent = 1;
			}
			for (pPos = pLineBuf; *pPos; pPos++) {
				if (*pPos != ' ' && *pPos != '\t') {
					if (!indent && IsWordStart(*pPos)) { // indent on keywords
						int style = SciCall_GetStyleAt(SciCall_PositionFromLine(iCurLine - 1) + iIndentLen);
						if (IsIndentKeywordStyle(style)) {
							endPart = EditKeywordIndent(pPos, &indent);
						}
					}
					if (indent) {
						memset(pPos, 0, iPrevLineLength - iIndentLen);
					}
					*pPos = '\0';
					break;
				}
				iIndentLen += 1;
			}
			if (indent) {
				int pad = iIndentWidth;
				iIndentPos += iIndentLen;
				ch = ' ';
				if (bTabIndents) {
					if (bTabsAsSpaces) {
						pad = iTabWidth;
						ch = ' ';
					} else {
						pad = 1;
						ch = '\t';
					}
				}
				iIndentPos += pad;
				while (pad-- > 0) {
					*pPos++ = ch;
				}
				if (iEOLMode == SC_EOL_CRLF || iEOLMode == SC_EOL_CR) {
					*pPos++ = '\r';
				}
				if (iEOLMode == SC_EOL_CRLF || iEOLMode == SC_EOL_LF) {
					*pPos++ = '\n';
				}
				if (indent == 2) {
					lstrcpynA(pPos, pLineBuf, iIndentLen + 1);
					pPos += iIndentLen;
					if (endPart) {
						iIndentLen = lstrlenA(endPart);
						lstrcpynA(pPos, endPart, iIndentLen + 1);
						pPos += iIndentLen;
					}
				}
				*pPos = '\0';
			}
			if (*pLineBuf) {
				//int iPrevLineStartPos;
				//int iPrevLineEndPos;
				//int iPrevLineIndentPos;

				SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
				SendMessage(hwnd, SCI_ADDTEXT, lstrlenA(pLineBuf), (LPARAM)pLineBuf);
				if (indent) {
					if (indent == 1) {// remove new line
						iCurPos = iIndentPos + ((iEOLMode == SC_EOL_CRLF)? 2 : 1);
						SendMessage(hwnd, SCI_SETSEL, iIndentPos, iCurPos);
						SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)"");
					}
					SendMessage(hwndEdit, SCI_SETSEL, iIndentPos, iIndentPos);
				}
				SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

				//iPrevLineStartPos	 = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, (WPARAM)iCurLine-1, 0);
				//iPrevLineEndPos	 = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, (WPARAM)iCurLine-1, 0);
				//iPrevLineIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, (WPARAM)iCurLine-1, 0);

				//if (iPrevLineEndPos == iPrevLineIndentPos) {
				//	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
				//	SendMessage(hwnd, SCI_SETTARGETSTART, (WPARAM)iPrevLineStartPos, 0);
				//	SendMessage(hwnd, SCI_SETTARGETEND, (WPARAM)iPrevLineEndPos, 0);
				//	SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
				//	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
				//}
			}
			GlobalFree(pLineBuf);
			//int iIndent = (int)SendMessage(hwnd, SCI_GETLINEINDENTATION, (WPARAM)iCurLine, 0);
			//SendMessage(hwnd, SCI_SETLINEINDENTATION, (WPARAM)iCurLine, (LPARAM)iIndentBefore);
			//iIndentLen = /*- iIndent +*/ SendMessage(hwnd, SCI_GETLINEINDENTATION, (WPARAM)iCurLine, 0);
			//if (iIndentLen > 0)
			//	SendMessage(hwnd, SCI_SETSEL, (WPARAM)iAnchorPos+iIndentLen, (LPARAM)iCurPos+iIndentLen);
		}
	}
}


void EditToggleCommentLine(HWND hwnd)
{
	switch (pLexCurrent->iLexer) {
	case SCLEX_CPP:
		BeginWaitCursor();
		switch (pLexCurrent->rid) {
		case NP2LEX_AWK:
		case NP2LEX_JAM:
			EditToggleLineComments(hwnd, L"#", TRUE);
			break;
		default:
			EditToggleLineComments(hwnd, L"//", FALSE);
			break;
		}
		EndWaitCursor();
		break;
	case SCLEX_CSS:
	case SCLEX_PASCAL:
	case SCLEX_VERILOG:
	case SCLEX_FSHARP:
	case SCLEX_GRAPHVIZ:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"//", FALSE);
		EndWaitCursor();
		break;
	case SCLEX_VBSCRIPT:
	case SCLEX_VB:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"'", TRUE);
		EndWaitCursor();
		break;
	case SCLEX_PYTHON:
	case SCLEX_RUBY:
	case SCLEX_SMALI:
	case SCLEX_MAKEFILE:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"#", FALSE);
		EndWaitCursor();
		break;
	case SCLEX_PERL:
	case SCLEX_CONF:
	case SCLEX_BASH:
	case SCLEX_TCL:
	case SCLEX_POWERSHELL:
	case SCLEX_CMAKE:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"#", TRUE);
		EndWaitCursor();
		break;
	case SCLEX_ASM:
	case SCLEX_PROPERTIES:
	case SCLEX_AU3:
	case SCLEX_INNOSETUP:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L";", TRUE);
		EndWaitCursor();
		break;
	case SCLEX_SQL:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"-- ", TRUE); // extra space
		EndWaitCursor();
		break;
	case SCLEX_LUA:
	case SCLEX_VHDL:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"--", TRUE);
		EndWaitCursor();
		break;
	case SCLEX_BATCH:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"rem ", TRUE);
		EndWaitCursor();
		break;
	case SCLEX_LATEX:
	case SCLEX_MATLAB:
		BeginWaitCursor();
		if (pLexCurrent->rid == NP2LEX_JULIA)
			EditToggleLineComments(hwnd, L"#", FALSE);
		else
			EditToggleLineComments(hwnd, L"%", FALSE);
		EndWaitCursor();
		break;
	case SCLEX_LISP:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L";", FALSE);
		EndWaitCursor();
		break;
	case SCLEX_VIM:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"\" ", FALSE);
		EndWaitCursor();
		break;
	case SCLEX_TEXINFO:
		BeginWaitCursor();
		EditToggleLineComments(hwnd, L"@c ", FALSE);
		EndWaitCursor();
		break;
	default:
		break;
	}
}


void EditToggleCommentBlock(HWND hwnd)
{
	switch (pLexCurrent->iLexer) {
	case SCLEX_CPP:
		switch (pLexCurrent->rid) {
		case NP2LEX_AWK:
		case NP2LEX_JAM:
			break;
		default:
			EditEncloseSelection(hwnd, L"/*", L"*/");
			break;
		}
		break;
	case SCLEX_XML:
		EditEncloseSelection(hwnd, L"<!--", L"-->");
		break;
	case SCLEX_HTML:
		EditEncloseSelection(hwnd, L"<!--", L"-->");
		break;
	case SCLEX_CSS:
	case SCLEX_ASM:
	case SCLEX_VERILOG:
	case SCLEX_GRAPHVIZ:
		EditEncloseSelection(hwnd, L"/*", L"*/");
		break;
	case SCLEX_PASCAL:
	case SCLEX_INNOSETUP:
		EditEncloseSelection(hwnd, L"{", L"}");
		break;
	case SCLEX_LUA:
		EditEncloseSelection(hwnd, L"--[[", L"]]");
		break;
	case SCLEX_FSHARP:
		EditEncloseSelection(hwnd, L"(*", L"*)");
		break;
	case SCLEX_LATEX:
		EditEncloseSelection(hwnd, L"\\begin{comment}", L"\\end{comment}");
		break;
	default:
		break;
	}
}

void EditShowCallTips(HWND hwnd, int position) {
	char *text = NULL;
	char *pLine = NULL;
	int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, (WPARAM)position, 0);
	int iDocLen = SciCall_GetLine(iLine, NULL); // get length
	pLine = NP2HeapAlloc(iDocLen + 1);
	SciCall_GetLine(iLine, pLine);
	text = NP2HeapAlloc(iDocLen + 1 + 128);
	wsprintfA(text, "ShowCallTips(%d, %d, %d)\n%s",iLine+1, position, iDocLen, pLine);
	SendMessage(hwnd, SCI_CALLTIPSHOW, position, (LPARAM)text);
	NP2HeapFree(pLine);
	NP2HeapFree(text);
}

// End of EditAutoC.c