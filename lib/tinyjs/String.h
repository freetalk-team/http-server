#pragma once

#include <string>
#include <iostream>
#include <format>

#define DELIMITER "/"
#ifndef IGNNRE_PARAMETER
#define IGNORE_PARAMETER(n) ((void)n)
#endif
#define MAXSAC 100


class wString : public std::string
{
private:
	

	void replace_character_len(const char* sentence, int slen, const char* p, int klen, const char* rep);
	void replace_str(const char* sentence, unsigned int slen, const char* p, unsigned int klen, const char* rep);

public:
	
	using std::string::string;

	wString(const std::string& s) : std::string(s) {}
	wString(std::string&& s) : std::string(std::move(s)) {}

	void operator=(int);
	void operator=(double);

	void to_lower_case();
	wString to_lower_case() const;

	void to_upper_case();
	wString to_upper_case() const;

	wString get_nth_line(int n) const;
	
	wString SubString(int start, int mylen)   const;
	wString substr(int start, int mylen = -1) const;

	//    int             Pos(const char* pattern);
	int             Pos(const wString& pattern, int pos = 0) const;
	int             Pos(const char* pattern, int pos = 0) const;
	//    int             Pos(const wString& pattern,int pos);

	
	unsigned int    Total(void) const;
	wString& SetLength(const unsigned int num);

	wString         Trim() const;
	wString         rtrim() const;
	wString         LTrim() const;
	static void     Rtrimch(char* sentence, const char cut_char);
	void            sprintf(const char* format, ...);
	void            cat_sprintf(const char* format, ...);
	int             last_delimiter(const char* delim) const;
	wString         strsplit(const char* delimstr);

	template<typename... Args>
	void format(const char* fmt, Args&&... args) {
		this->assign(std::format(fmt, std::forward<Args>(args)...));
	}

	template<typename... Args>
	void cat_format(const char* fmt, Args&&... args) {
		this->append(std::format(fmt, std::forward<Args>(args)...));
	}
	
	wString get_list_string(const int pos) const;

	//    int             Count(void);
	void ResetLength(unsigned int num);
	void Add(const wString& str);
	void Add(const char* str);

	int getLines() const;
	
	void load_from_file(const char* FileName);
	void load_from_file(const wString& str);
	int  save_to_file(const char* FileName);
	int  save_to_file(const wString& str);

};
