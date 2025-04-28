#define _CRT_SECURE_NO_WARNINGS
/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * - Useful language functions
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TinyJS_Functions.h"
#include "define.h"
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
 //#include "dregex.h"
using namespace std;
// ----------------------------------------------- Actual Functions
void js_print(CScriptVar* v, void* userdata) {
	IGNORE_PARAMETER(userdata);
	//headerCheck(js->socket, &(js->printed), js->headerBuf,1);
	wString str = v->getParameter("text")->getString();
	//int num = send( js->socket, str.c_str(), str.length(), 0);
	//if( num <0 ){
	 //   debug_log_output( "Script Write Error at js_print" );
	//}
	//debug_log_output( "print %s %d", str.c_str(), num );
	printf("%s", v->getParameter("text")->getString().c_str());
}

void scTrace(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(c);
	IGNORE_PARAMETER(userdata);
	CTinyJS* js = static_cast<CTinyJS*>(userdata);
	js->root->trace();
}

void scObjectDump(CScriptVar* c, void* userdata) {
	c->getParameter("this")->trace("> ");
}

void scObjectClone(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	CScriptVar* obj = c->getParameter("this");
	c->getReturnVar()->copyValue(obj);
}
void scKeys(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString list = c->getParameter("obj")->trace2();
	CScriptVar* result = c->getReturnVar();
	result->setArray();
	int length = 0;
	int count = list.getLines();
	for (int i = 0; i < count; i++) {
		result->setArrayIndex(length++, new CScriptVar(list.get_list_string(i)));
	}
}


void scMathRand(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	c->getReturnVar()->setDouble((double)rand() / RAND_MAX);
}

void scMathRandInt(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	int min = c->getParameter("min")->getInt();
	int max = c->getParameter("max")->getInt();
	int val = min + (int)(rand() % (1 + max - min));
	c->getReturnVar()->setInt(val);
}
/////////////////////////////////////////////////////////////////////////

void scPrint(CScriptVar* c, void*) {
	wString val = c->getParameter("val")->getString();
	printf("%s", val.c_str());
	c->getReturnVar()->setString(val);
}
/////////////////////////////////////////////////////////////////////////
void scTrim(CScriptVar* c, void*) {
	wString val = c->getParameter("this")->getString();
	c->getReturnVar()->setString(val.Trim());
}
//
void scRTrim(CScriptVar* c, void*) {
	wString val = c->getParameter("this")->getString();
	c->getReturnVar()->setString(val.rtrim());
}
//
void scLTrim(CScriptVar* c, void*) {
	wString val = c->getParameter("this")->getString();
	c->getReturnVar()->setString(val.LTrim());
}
/////////////////////////////////////////////////////////////////////////
void scCharToInt(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("ch")->getString();
	int val = 0;
	if (str.length() > 0) {
		val = (int)str.c_str()[0];
	}
	c->getReturnVar()->setInt(val);
}

void scStringIndexOf(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	wString search = c->getParameter("search")->getString();
	int  p = str.find(search);
	int val = (p == wString::npos) ? -1 : p;
	c->getReturnVar()->setInt(val);
}
//Substring
void scStringSubstring(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	int lo = c->getParameter("lo")->getInt();
	int hi = c->getParameter("hi")->getInt();

	int lex = hi - lo;
	if (lex > 0 && lo >= 0 && lo + lex <= (int)str.length())
		c->getReturnVar()->setString(str.substr(lo, lex));
	else
		c->getReturnVar()->setString("");
}
//SubStr
void scStringSubstr(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	int lo = c->getParameter("lo")->getInt();
	int hi = c->getParameter("hi")->getInt();

	if (hi > 0 && lo >= 0 && lo + hi <= (int)str.length())
		c->getReturnVar()->setString(str.substr(lo, hi));
	else
		c->getReturnVar()->setString("");
}
//AT

/// <summary>
/// function String.charAt(pos)
/// pos�ʒu�̕������擾�ibyte�P�ʁj
/// </summary>
/// <param name="c">�����n���f�[�^</param>
/// <param name="userdata"></param>
void scStringCharAt(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	int p = c->getParameter("pos")->getInt();
	if (p >= 0 && p < (int)str.length()) {
		c->getReturnVar()->setString(str.substr(p, 1));
	}
	else {
		c->getReturnVar()->setString("");
	}
}

void scStringCharCodeAt(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	int p = c->getParameter("pos")->getInt();
	if (p >= 0 && p < (int)str.length())
		c->getReturnVar()->setInt(str.at(p));
	else
		c->getReturnVar()->setInt(0);
}

void scStringSplit(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	wString sep = c->getParameter("separator")->getString();
	CScriptVar* result = c->getReturnVar();
	result->setArray();
	int length = 0;

	//consider sepatator length;
	int inc = sep.length();
	int pos = str.find(sep);
	while (pos != wString::npos) {
		result->setArrayIndex(length++, new CScriptVar(str.substr(0, pos)));
		str = str.substr(pos + inc);
		pos = str.find(sep);
	}

	if (str.size() > 0) {
		result->setArrayIndex(length++, new CScriptVar(str));
	}
}
//Replace
void scStringReplace(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	wString before = c->getParameter("before")->getString();
	wString after = c->getParameter("after")->getString();
	//str�̒���before��T��
	int pos = str.find(before);
	while (pos != wString::npos) {
		str = str.substr(0, pos) + after + str.substr(pos + before.length());
		pos = str.find(before, pos);
	}
	c->getReturnVar()->setString(str);
}
//PregReplace
void scPregStringReplace(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString result;
	wString str = c->getParameter("this")->getString();
	CScriptVar* arrp = c->getParameter("pattern");
	vector<wString> patterns;
	vector<wString> replaces;
	int pn = arrp->getArrayLength();
	if (pn) {
		CScriptVar* arrr = c->getParameter("replace");
		auto rn = arrr->getArrayLength();
		if (pn == rn) {
			for (int i = 0; i < pn; i++) {
				patterns.push_back(arrp->getArrayIndex(i)->getString());
				replaces.push_back(arrr->getArrayIndex(i)->getString());
			}
		}
	}
	else {
		wString pattern = c->getParameter("pattern")->getString();
		wString replace = c->getParameter("replace")->getString();
		patterns.push_back(pattern);
		replaces.push_back(replace);
	}
	//dregex::replace(&result, str, patterns, replaces);
	c->getReturnVar()->setString(result);
}

//getLocalAddress
void scGetLocalAddress(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	//c->getReturnVar()->setString(wString::get_local_address());
}
void scStringFromCharCode(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	char str[2] = {};
	str[0] = (char)c->getParameter("char")->getInt();
	c->getReturnVar()->setString(str);
}

void scIntegerParseInt(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("str")->getString();
	int val = strtol(str.c_str(), 0, 0);
	c->getReturnVar()->setInt(val);
}

void scIntegerValueOf(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("str")->getString();

	int val = 0;
	if (str.length() == 1)
		val = str[0];
	c->getReturnVar()->setInt(val);
}
//
void scIntegerToDateString(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	//wString times = c->getParameter("this")->getString();
	//wString format = c->getParameter("format")->getString();
	//char s[128] = {0};
	//long time = atol(times.c_str());
	//struct tm *timeptr;
	//timeptr = localtime(&time);
	//strftime(s, 128, format.c_str(), timeptr);
	//c->getReturnVar()->setString(s);
}
void scStringDate(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	auto t = time(NULL);
	char s[128];
#ifdef linux
    sprintf(s, "%ld", t);
#else
	sprintf(s, "%lld", t);
#endif
	c->getReturnVar()->setString(s);
}
void scNKFConv(CScriptVar* c, void* userdata) {
#ifdef WEB
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	wString format = c->getParameter("format")->getString();
	wString temp = str.nkfcnv(format);
	c->getReturnVar()->setString(temp);
#endif
}

void scDBConnect(CScriptVar* c, void* userdata) {
#ifdef DB
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	str = _DBConnect(str);
	if (str.Length()) {
		c->getParameter("this")->setString(str);
		c->getReturnVar()->setString(str);
	}
	else {
		c->getReturnVar()->setString("");
	}
#endif
}

void scDBDisConnect(CScriptVar* c, void* userdata) {
#ifdef DB
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	int ret = _DBDisConnect(str);
	c->getReturnVar()->setInt(ret);
#endif
}

void scDBSQL(CScriptVar* c, void* userdata) {
#ifdef DB
	IGNORE_PARAMETER(userdata);
	wString str = c->getParameter("this")->getString();
	wString sql = c->getParameter("sqltext")->getString();
	wString ret = _DBSQL(str, sql);
	c->getReturnVar()->setString(ret);
#endif
}


void scJSONStringify(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString result;
	c->getParameter("obj")->getJSON(result);
	c->getReturnVar()->setString(result.c_str());
}

void scExec(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	CTinyJS* tinyJS = static_cast<CTinyJS*>(userdata);
	wString str = c->getParameter("jsCode")->getString();
	tinyJS->execute(str);
}

void scEval(CScriptVar* c, void* userdata) {

	CTinyJS* tinyJS = static_cast<CTinyJS*>(userdata);
	c->setReturnVar(tinyJS->evaluateComplex(c->getParameter("jsCode")->getString()).var);
}

void scArraySize(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	auto self = c->getParameter("this");

	c->getReturnVar()->setInt(self->getArrayLength());
}

void scArrayContains(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	CScriptVar* obj = c->getParameter("obj");
	CScriptVarLink* v = c->getParameter("this")->firstChild;

	bool contains = false;
	while (v) {
		if (v->var->equals(obj)) {
			contains = true;
			break;
		}
		v = v->nextSibling;
	}

	c->getReturnVar()->setInt(contains);
}

void scArrayRemove(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	CScriptVar* obj = c->getParameter("obj");
	vector<int> removedIndices;
	CScriptVarLink* v;
	// remove
	v = c->getParameter("this")->firstChild;
	while (v) {
		if (v->var->equals(obj)) {
			removedIndices.push_back(v->getIntName());
		}
		v = v->nextSibling;
	}
	// renumber
	v = c->getParameter("this")->firstChild;
	while (v) {
		int n = v->getIntName();
		int newn = n;
		for (size_t i = 0; i < removedIndices.size(); i++)
			if (n >= removedIndices[i])
				newn--;
		if (newn != n)
			v->setIntName(newn);
		v = v->nextSibling;
	}
}

void scArrayJoin(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	wString sep = c->getParameter("separator")->getString();
	CScriptVar* arr = c->getParameter("this");

	wString sstr;
	int lex = arr->getArrayLength();
	for (int i = 0; i < lex; i++) {
		if (i > 0) {
			sstr += sep;
		}
		sstr += arr->getArrayIndex(i)->getString();
	}

	c->getReturnVar()->setString(sstr.c_str());
}

//toLowerCase
void scToLowerCase(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	auto& str = c->getParameter("this")->getString();
	
	c->getReturnVar()->setString(str.to_lower_case());
}
//toUpperCase
void scToUpperCase(CScriptVar* c, void* userdata) {
	IGNORE_PARAMETER(userdata);
	auto& str = c->getParameter("this")->getString();
	
	c->getReturnVar()->setString(str.to_upper_case());
}

// ----------------------------------------------- Register Functions
void registerFunctions(CTinyJS* tinyJS) {
	tinyJS->addNative("function exec(jsCode)", scExec, tinyJS); // execute the given code
	tinyJS->addNative("function eval(jsCode)", scEval, tinyJS); // execute the given wString (an expression) and return the result
	tinyJS->addNative("function trace()", scTrace, tinyJS);
	tinyJS->addNative("function Object.dump()", scObjectDump, tinyJS);
	tinyJS->addNative("function Object.clone()", scObjectClone, 0);
	tinyJS->addNative("function Object.keys(obj)", scKeys, 0);
	tinyJS->addNative("function charToInt(ch)", scCharToInt, 0); //  convert a character to an int - get its value

	tinyJS->addNative("function Math.rand()", scMathRand, 0);
	tinyJS->addNative("function Math.randInt(min, max)", scMathRandInt, 0);
	tinyJS->addNative("function Integer.parseInt(str)", scIntegerParseInt, 0); // wString to int
	tinyJS->addNative("function Integer.valueOf(str)", scIntegerValueOf, 0); // value of a single character

	tinyJS->addNative("function String.indexOf(search)", scStringIndexOf, 0); // find the position of a wString in a string, -1 if not
	tinyJS->addNative("function String.substring(lo,hi)", scStringSubstring, 0);
	tinyJS->addNative("function String.substr(lo,hi)", scStringSubstr, 0);
	tinyJS->addNative("function String.charAt(pos)", scStringCharAt, 0);
	tinyJS->addNative("function String.charCodeAt(pos)", scStringCharCodeAt, 0);
	tinyJS->addNative("function String.fromCharCode(char)", scStringFromCharCode, 0);
	tinyJS->addNative("function String.split(separator)", scStringSplit, 0);
	tinyJS->addNative("function String.replace(before,after)", scStringReplace, 0);
	tinyJS->addNative("function String.preg_replace(pattern,replace)", scPregStringReplace, 0);
	//tinyJS->addNative("function String.preg_match(pattern)",scPregStringMatch, 0 );

	tinyJS->addNative("function getLocalAddress()", scGetLocalAddress, 0);
	tinyJS->addNative("function String.toLowerCase()", scToLowerCase, 0);
	tinyJS->addNative("function String.toUpperCase()", scToUpperCase, 0);
	tinyJS->addNative("function String.toDateString(format)", scIntegerToDateString, 0); // time to strng format
	tinyJS->addNative("function Date()", scStringDate, 0); // time to strng
	tinyJS->addNative("function String.nkfconv(format)", scNKFConv, 0); // language code convert
	tinyJS->addNative("function String.Connect()", scDBConnect, 0); // Connect to DB
	tinyJS->addNative("function String.DisConnect()", scDBDisConnect, 0); // DisConnect to DB
	tinyJS->addNative("function String.SQL(sqltext)", scDBSQL, 0); // Execute SQL

//    tinyJS->addNative("function JSON.mp3id3tag(path)",                scMp3Id3Tag,           0 );
	tinyJS->addNative("function JSON.stringify(obj, replacer)", scJSONStringify, 0); // convert to JSON. replacer is ignored at the moment

	// JSON.parse is left out as you can (unsafely!) use eval instead
	tinyJS->addNative("function Array.contains(obj)", scArrayContains, 0);
	tinyJS->addNative("function Array.remove(obj)", scArrayRemove, 0);
	tinyJS->addNative("function Array.join(separator)", scArrayJoin, 0);
	// tinyJS->addNative("function Array.size()", scArraySize, 0);

	tinyJS->addNative("function String.trim()", scTrim, 0);
	tinyJS->addNative("function String.rtrim()", scRTrim, 0);
	tinyJS->addNative("function String.ltrim()", scLTrim, 0);
	tinyJS->addNative("function print(text)", js_print, tinyJS);
}
