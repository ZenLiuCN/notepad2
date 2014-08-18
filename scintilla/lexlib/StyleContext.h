// Scintilla source code edit control
/** @file StyleContext.h
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#ifndef STYLECONTEXT_H
#define STYLECONTEXT_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

// All languages handled so far can treat all characters >= 0x80 as one class
// which just continues the current token or starts an identifier if in default.
// DBCS treated specially as the second character can be < 0x80 and hence
// syntactically significant. UTF-8 avoids this as all trail bytes are >= 0x80
class StyleContext {
	LexAccessor &styler;
	IDocumentWithLineEnd *multiByteAccess;
	unsigned int endPos;
	unsigned int lengthDocument;

	// Used for optimizing GetRelativeCharacter
	unsigned int posRelative;
	unsigned int currentPosLastRelative;
	int offsetRelative;

	StyleContext &operator=(const StyleContext &);
	void GetNextChar();

public:
	unsigned int currentPos;
	int currentLine;
	int lineDocEnd;
	int lineStartNext;
	bool atLineStart;
	bool atLineEnd;
	int state;
	int chPrev;
	int ch;
	int chNext;
	int width;
	int widthNext;

	StyleContext(unsigned int startPos, unsigned int length,
				int initStyle, LexAccessor &styler_, unsigned char chMask='\377');
	void Complete();
	bool More() const {
		return currentPos < endPos;
	}
	void Forward();
	void Forward(int nb);
	void ForwardBytes(int nb);
	void ChangeState(int state_);
	void SetState(int state_);
	void ForwardSetState(int state_);
	int LengthCurrent() const {
		return currentPos - styler.GetStartSegment();
	}
	int GetRelative(int n) const {
		return static_cast<unsigned char>(styler.SafeGetCharAt(currentPos+n));
	}
	int GetRelativeCharacter(int n);
	bool Match(char ch0) const {
		return ch == static_cast<unsigned char>(ch0);
	}
	bool Match(char ch0, char ch1) const {
		return (ch == static_cast<unsigned char>(ch0)) && (chNext == static_cast<unsigned char>(ch1));
	}
	bool Match(const char *s) const {
        return LexMatch(currentPos, styler, s);
	}
	bool MatchIgnoreCase(const char *s) const {
		return LexMatchIgnoreCase(currentPos, styler, s);
	}
	// Non-inline
	int GetCurrent(char *s, unsigned int len) const {
		return LexGetRange(styler.GetStartSegment(), currentPos - 1, styler, s, len);
	}
	int GetCurrentLowered(char *s, unsigned int len) const {
		return LexGetRangeLowered(styler.GetStartSegment(), currentPos - 1, styler, s, len);
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif