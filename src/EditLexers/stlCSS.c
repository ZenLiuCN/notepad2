#include "EditLexer.h"
#include "EditStyle.h"

// https://en.wikipedia.org/wiki/Cascading_Style_Sheets
// https://www.w3.org/Style/CSS/

static KEYWORDLIST Keywords_CSS = {{
"^-moz- ^-ms- ^-o- ^-webkit- alignment-adjust alignment-baseline animation animation-delay "
"animation-direction animation-duration animation-iteration-count animation-name "
"animation-play-state animation-timing-function appearance ascent azimuth backface-visibility "
"background background-attachment background-break background-clip background-color "
"background-image background-origin background-position background-repeat background-size "
"baseline baseline-shift bbox binding bleed bookmark-label bookmark-level bookmark-state "
"bookmark-target border border-bottom border-bottom-color border-bottom-left-radius "
"border-bottom-right-radius border-bottom-style border-bottom-width border-collapse border-color "
"border-image border-image-outset border-image-repeat border-image-slice border-image-source "
"border-image-width border-left border-left-color border-left-style border-left-width "
"border-length border-radius border-right border-right-color border-right-style "
"border-right-width border-spacing border-style border-top border-top-color "
"border-top-left-radius border-top-right-radius border-top-style border-top-width border-width "
"bottom box-align box-decoration-break box-direction box-flex box-flex-group box-lines "
"box-ordinal-group box-orient box-pack box-shadow box-sizing break-after break-before "
"break-inside cap-height caption-side centerline change-bar change-bar-class change-bar-offset "
"change-bar-side clear clip color color-profile column-count column-fill column-gap column-rule "
"column-rule-color column-rule-style column-rule-width columns column-span column-width content "
"counter-increment counter-reset crop cue cue-after cue-before cursor definition-src descent "
"direction display dominant-baseline drop-initial-after-adjust drop-initial-after-align "
"drop-initial-before-adjust drop-initial-before-align drop-initial-size drop-initial-value "
"elevation empty-cells fit fit-position float float-offset font font-family font-size "
"font-size-adjust font-stretch font-style font-variant font-weight grid-columns grid-rows "
"hanging-punctuation height hyphenate-after hyphenate-before hyphenate-character hyphenate-lines "
"hyphenate-resource hyphens icon image-orientation image-resolution inline-box-align "
"insert-position left letter-spacing line-height line-stacking line-stacking-ruby "
"line-stacking-shift line-stacking-strategy list-style list-style-image list-style-position "
"list-style-type make-element margin margin-bottom margin-left margin-right margin-top mark "
"mark-after mark-before marker-offset marks marquee-direction marquee-play-count marquee-speed "
"marquee-style mathline max-height max-width min-height min-width move-to nav-down nav-index "
"nav-left nav-right nav-up opacity orphans outline outline-color outline-offset outline-style "
"outline-width overflow overflow-style overflow-x overflow-y padding padding-bottom padding-left "
"padding-right padding-top page page-break-after page-break-before page-break-inside page-policy "
"panose-1 pause pause-after pause-before perspective perspective-origin phonemes pitch "
"pitch-range play-during position presentation-level prototype prototype-insert-policy "
"prototype-insert-position punctuation-trim quotes rendering-intent resize rest rest-after "
"rest-before richness right rotation rotation-point ruby-align ruby-overhang ruby-position "
"ruby-span size slope speak speak-header speak-numeral speak-punctuation speech-rate src stemh "
"stemv stress string-set table-layout target target-name target-new target-position text-align "
"text-align-last text-decoration text-emphasis text-height text-indent text-justify text-outline "
"text-replace text-shadow text-transform text-wrap top topline transform transform-origin "
"transform-style transition transition-delay transition-duration transition-property "
"transition-timing-function unicode-bidi unicode-range units-per-em vertical-align visibility "
"voice-balance voice-duration voice-family voice-pitch voice-pitch-range voice-rate voice-stress "
"voice-volume volume white-space white-space-collapse widows width widths word-break word-spacing "
"word-wrap x-height z-index",
"active after before checked default disabled empty enabled first first-child first-letter "
"first-line first-of-type focus hover indeterminate invalid lang last-child last-of-type left "
"link not nth-child nth-last-child nth-last-of-type nth-of-type only-child only-of-type optional "
"required right root target valid visited",
"", "", "", "", "", "", ""

#if NUMKEYWORD == 16
, "", "", "", "", "", "", ""
#endif
}};

EDITLEXER lexCSS = { SCLEX_CSS, NP2LEX_CSS, L"CSS Style Sheet", L"css", L"", &Keywords_CSS,
{
	{ STYLE_DEFAULT, NP2STYLE_Default, L"Default", L"", L"" },
	//{ SCE_CSS_DEFAULT, L"Default", L"", L"" },
	{ SCE_CSS_COMMENT, NP2STYLE_Comment, L"Comment", L"fore:#008000", L"" },
	{ SCE_CSS_TAG, 63161, L"HTML Tag", L"bold; fore:#0A246A", L"" },
	{ SCE_CSS_CLASS, 63301, L"Tag-Class", L"fore:#648000", L"" },
	{ SCE_CSS_ID, 63302, L"Tag-Id", L"fore:#648000", L"" },
	{ SCE_CSS_ATTRIBUTE, 63303, L"Tag-Attribute", L"italic; fore:#648000", L"" },
	{ MULTI_STYLE(SCE_CSS_PSEUDOCLASS, SCE_CSS_EXTENDED_PSEUDOCLASS, SCE_CSS_PSEUDOELEMENT, SCE_CSS_EXTENDED_PSEUDOELEMENT), 63304, L"Pseudo-class/element", L"fore:#B000B0", L"" },
	{ MULTI_STYLE(SCE_CSS_IDENTIFIER, SCE_CSS_IDENTIFIER2, SCE_CSS_IDENTIFIER3, SCE_CSS_EXTENDED_IDENTIFIER), 63306, L"CSS Property", L"fore:#FF4000", L"" },
	{ MULTI_STYLE(SCE_CSS_DOUBLESTRING, SCE_CSS_SINGLESTRING, 0, 0), NP2STYLE_String, L"String", L"fore:#008000", L"" },
	{ SCE_CSS_VALUE, 63309, L"Value", L"fore:#3A6EA5", L"" },
	{ SCE_CSS_OPERATOR, NP2STYLE_Operator, L"Operator", L"fore:#B000B0", L"" },
	{ SCE_CSS_IMPORTANT, 63310, L"Important", L"bold; fore:#C80000", L"" },
	{ SCE_CSS_DIRECTIVE, NP2STYLE_Directive, L"Directive", L"bold; fore:#000000; back:#FFF1A8", L"" },
	{ SCE_CSS_MEDIA, 63382, L"Media", L"bold; fore:#0A246A", L"" },
	{ SCE_CSS_VARIABLE, NP2STYLE_Variable, L"Variable", L"bold; fore:#FF4000", L"" },
	{ SCE_CSS_UNKNOWN_PSEUDOCLASS, 63305, L"Unknown Pseudo-class", L"fore:#C80000; back:#FFFF80", L"" },
	{ SCE_CSS_UNKNOWN_IDENTIFIER, 63307, L"Unknown Property", L"fore:#C80000; back:#FFFF80", L"" },
	{ -1, 00000, L"", L"", L"" }
}
};
