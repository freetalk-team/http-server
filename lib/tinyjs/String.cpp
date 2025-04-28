
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
//#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdarg>

#include "String.h"

namespace fs = std::filesystem;


#ifndef strrstr

char* strrstr(const char* string, const char* pattern)
{

	const char* last = NULL;
	for (const char* p = string; ; p++) {
		if (0 == *p) {
			return (char*)last;
		}
		p = strstr(p, pattern);
		if (p == NULL) {
			break;
		}
		last = p;
	}
	return (char*)last;
}//strrstr
#endif
//---------------------------------------------------------------------------
//source
//---------------------------------------------------------------------------

void wString::operator=(int num) {
	sprintf("%d", num);
}

void wString::operator=(double num) {
	sprintf("%f", num);
}


wString& wString::SetLength(const unsigned int num)
{
	resize(num);
	return *this;
}

wString wString::substr(int start, int mylen) const {
	if (start < 0 || static_cast<size_t>(start) > this->size()) {
		return wString(); // Empty
	}

	if (mylen == -1 || static_cast<size_t>(start + mylen) > this->size()) {
		return wString(std::string::substr(start));
	} else {
		return wString(std::string::substr(start, mylen));
	}
}

wString  wString::SubString(int start, int mylen) const {
	return substr(start, mylen);
	
}

void wString::Add(const char* str)
{
	*this += str;
	*this += "\r\n";
}

void wString::Add(const wString& str)
{
	*this += str;
	*this += "\r\n";
}

// Non-const method: Convert the string to lowercase (modifies the object)
void wString::to_lower_case() {
	std::transform(this->begin(), this->end(), this->begin(), [](unsigned char c) {
		return std::tolower(c);  // Convert character to lowercase
	});
}

// Non-const method: Convert the string to uppercase (modifies the object)
void wString::to_upper_case() {
	std::transform(this->begin(), this->end(), this->begin(), [](unsigned char c) {
		return std::toupper(c);  // Convert character to uppercase
	});
}

// Const method: Convert to lowercase and return a new object
wString wString::to_lower_case() const {
	wString result = *this;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
		return std::tolower(c);  // Convert character to lowercase
	});
	return result;
}

// Const method: Convert to uppercase and return a new object
wString wString::to_upper_case() const {
	wString result = *this;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
		return std::toupper(c);  // Convert character to uppercase
	});
	return result;
}

//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
//int wString::Pos(const char* pattern)
//{
//    char* ptr = strstr(String,pattern);
//    if( ptr == NULL ){
//        return npos;
//    }else{
//        return (int)(ptr-String);
//    }
//}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------


int wString::Pos(const char* pattern, int pos) const {
	auto ptr = strstr(data() + pos, pattern);
	if (ptr == NULL) {
		return npos;
	}
	else {
		return (int)(ptr - data());
	}
}

int wString::Pos(const wString& pattern, int pos) const
{
	return Pos(pattern.c_str(), pos);
}

void wString::load_from_file(const wString& str)
{
	load_from_file(str.c_str());
}

void wString::load_from_file(const char* FileName) {

	std::ifstream in(FileName);

	if (!in) 
		throw std::runtime_error("Failed to open file: " + std::string(FileName));

	std::ostringstream ss;
	ss << in.rdbuf();
	
	assign(ss.str());
}


int isNumber(char* str)
{
	for (int i = static_cast<int>(strlen(str)) - 1; i >= 0; i--) {
		if ((!isdigit(str[i])) && str[i] != '.') {
			return 0;
		}
	}
	return 1;
}


int wString::save_to_file(const wString& str)
{
	return save_to_file(str.c_str());
}

int wString::save_to_file(const char* FileName) {

	std::ofstream os(FileName);

	os << *this;

	os.close();

	return 1;
}

// Trim both ends
wString wString::Trim() const {
	return LTrim().rtrim();
}

// Trim right
wString wString::rtrim() const {
	wString result = *this;
	result.erase(
		std::find_if(result.rbegin(), result.rend(), [](wchar_t ch) {
			return !std::iswspace(ch);
		}).base(),
		result.end()
	);
	return result;
}

// Trim left
wString wString::LTrim() const {
	wString result = *this;
	result.erase(
		result.begin(),
		std::find_if(result.begin(), result.end(), [](wchar_t ch) {
			return !std::iswspace(ch);
		})
	);
	return result;
}


void wString::Rtrimch(char* sentence, char cut_char)
{
	if (sentence == NULL || *sentence == 0) return;
	auto lengthes = static_cast<unsigned int>(strlen(sentence));        // 文字列長Get
	auto source_p = sentence;
	source_p += lengthes;                  // ワークポインタを文字列の最後にセット。
	for (auto i = 0U; i < lengthes; i++) {  // 文字列の数だけ繰り返し。
		source_p--;                      // 一文字ずつ前へ。
		if (*source_p == cut_char) {     // 削除キャラ ヒットした場合削除
			*source_p = '\0';
		}
		else {                           // 違うキャラが出てきたところで終了。
			break;
		}
	}
	return;
}


//void wString::ResetLength(unsigned int num)
//{
//    assert(capa>(unsigned int)num);
//    String[num] = 0;
//    len = num;
//}

//---------------------------------------------------------------------------
unsigned int wString::Total(void) const {
	return capacity();
}
//---------------------------------------------------------------------------
int wString::last_delimiter(const char* delim) const {

	// Loop through each character in the delimiter string
	size_t last_pos = std::string::npos;
	for (const char* d = delim; *d != '\0'; ++d) {
		size_t pos = rfind(*d);
		if (pos != std::string::npos && (last_pos == std::string::npos || pos > last_pos)) {
			last_pos = pos;
		}
	}

	// If no delimiter was found, return -1
	return (last_pos == std::string::npos) ? -1 : static_cast<int>(last_pos);

}

void wString::replace_character_len(const char* sentence, int slen, const char* p, int klen, const char* rep)
{
	//char* str;
	auto rlen = strlen(rep);
	if (klen == rlen) {
		memcpy((void*)p, rep, rlen);
		//前詰め置換そのままコピーすればいい
	}
	else if (klen > rlen) {
		auto num = klen - rlen;
		strcpy(const_cast<char*>(p), p + num);
		memcpy((void*)p, rep, rlen);
		//置換文字が長いので後詰めする
	}
	else {
		auto num = rlen - klen;
		//pからrlen-klenだけのばす
		for (auto str = const_cast<char*>(sentence + slen + num); str > p + num; str--) {
			*str = *(str - num);
		}
		memcpy((void*)p, rep, rlen);
	}
	return;
}


void wString::sprintf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buffer[1024]; // Adjust buffer size as needed
	int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (n >= 0 && static_cast<size_t>(n) < sizeof(buffer)) {
		assign(buffer); // replace string content
	} else {
		// Fallback if result was too large
		std::vector<char> bigbuf(n + 1);
		va_start(args, fmt);
		std::vsnprintf(bigbuf.data(), bigbuf.size(), fmt, args);
		va_end(args);
		assign(bigbuf.data());
	}
}

void wString::cat_sprintf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	// Try small buffer first
	char buffer[1024];
	int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (n >= 0 && static_cast<size_t>(n) < sizeof(buffer)) {
		this->append(buffer);
	} else if (n > 0) {
		// Allocate a buffer large enough
		std::vector<char> bigbuf(n + 1);
		va_start(args, fmt); // Restart args
		std::vsnprintf(bigbuf.data(), bigbuf.size(), fmt, args);
		va_end(args);

		this->append(bigbuf.data());
	} else {
		// vsnprintf failed, optionally handle error
	}
	
}

wString wString::strsplit(const char* delimstr)
{

	wString tmp;
	auto delimlen = static_cast<unsigned int>(strlen(delimstr));
	auto pos = Pos(delimstr);
	if (pos != npos) {
		tmp = substr(pos + delimlen);
	}
	return tmp;
}


// **************************************************************************
// URIデコードを行います.
//  機能 : URIデコードを行う
//  引数 : dst 変換した文字の書き出し先.
//                dst_len 変換した文字の書き出し先の最大長.
//                src 変換元の文字.
//                src_len 変換元の文字の長さ.
// 返値 : デコードした文字の数(そのままも含む)
// **************************************************************************
unsigned char htoc(unsigned char x)
{
	if ('0' <= x && x <= '9') return (unsigned char)(x - '0');
	if ('a' <= x && x <= 'f') return (unsigned char)(x - 'a' + 10);
	if ('A' <= x && x <= 'F') return (unsigned char)(x - 'A' + 10);
	return 0;
}

int wString::getLines() const {

	if (empty()) return 0;

	int count = 1; // At least one line if not empty
	for (char ch : *this) {
		if (ch == '\n') ++count;
	}
	return count;
}

wString wString::get_nth_line(int n) const {
	std::istringstream stream(*this);
	std::string line;
	int line_number = 0;
	
	// Read lines from the stream
	while (std::getline(stream, line)) {
		if (line_number == n) {
			return std::move(line);  // Return the N-th line
		}
		++line_number;
	}
	
	return "";  // Return empty string if N-th line doesn't exist
}

wString wString::get_list_string(int pos) const {
	return get_nth_line(pos);
}
