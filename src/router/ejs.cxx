
#include <fstream>
#include <sstream>
#include <vector>
#include <variant>
#include <regex>
#include <iostream>
#include <filesystem>
#include <print>

#include <boost/regex.hpp>

#include "ejs.h"
#include "config.h"

using ios = std::ios;
using JS = Script;

namespace fs = std::filesystem;

// std::regex if_else_block(R"(<% if \((.*)\) %>([\s\S]*?)<% else %>([\s\S]*?)<% endif %>)");
// std::regex if_block(R"(<% if \((.*)\) %>([\s\S]*?)<% endif %>)");
std::regex re(R"(<% (if|else|endif|for|endfor) (\((.*)\) )?%>)");
std::regex rm(R"(>[\t\n ]+<)");
boost::regex var(R"(<% (.*?) %>)");

template <typename T>
auto trim_any(T& str, std::string_view chars = " \t\n\r") {
	size_t start = str.find_first_not_of(chars);
	if (start == std::string_view::npos) return T(str.data(), 0); // All spaces

	size_t end = str.find_last_not_of(chars);
	// std::cout << end << ' ' << str[end] << std::endl;

	return str.substr(start, end - start + 1);
}

using svmatch = std::match_results<std::string_view::const_iterator>;
using svsub_match = std::sub_match<std::string_view::const_iterator>;

inline std::string_view get_sv(const svsub_match& m) {
	return std::string_view(m.first, m.length());
}

inline bool regex_match(std::string_view sv,
				  svmatch& m,
				  const std::regex& e,
				  std::regex_constants::match_flag_type flags = 
					  std::regex_constants::match_default) {
	return std::regex_match(sv.begin(), sv.end(), m, e, flags);
}

inline bool regex_match(std::string_view sv,
				  const std::regex& e,
				  std::regex_constants::match_flag_type flags = 
					  std::regex_constants::match_default) {
	return std::regex_match(sv.begin(), sv.end(), e, flags);
}

void boost_regex_replace_sv(
	const std::string_view&	 input,
	const boost::regex& re,
	std::ostream& out,
	const std::function<wString(const boost::match_results<std::string_view::const_iterator>&)>& replacer
) {
	using sv_iter = std::string_view::const_iterator;
	boost::match_results<sv_iter> match;

	sv_iter begin = input.begin();
	sv_iter end = input.end();

	while (boost::regex_search(begin, end, match, re)) {
		out << std::string_view(begin, match[0].first);  // text before match
		out << replacer(match);                     // replacement from lambda
		begin = match[0].second;                    // advance past match
	}

	out << std::string(begin, end); // remainder after last match
}

struct plain;
struct if_block;
struct for_block;

typedef std::variant<plain, if_block, for_block> block;
typedef std::vector<block> sequence;

void evaluate(const sequence& s, std::ostream& os, JS& js);

struct plain : std::string_view {

	using std::string_view::string_view;

	void eval(std::ostream& os, JS& js) const;
};

struct if_block {

	wString cond;
	sequence _true, _false;

	if_block(const std::string_view& c) : cond(c.data(), c.size()) {}
	// using plain::plain;

	void eval(std::ostream& os, JS& js) const;
};

struct for_block {

	wString cond;
	sequence _loop;

	for_block(const std::string_view& c) : cond(c.data(), c.size()) {}

	void eval(std::ostream& os, JS& js) const;
};

void plain::eval(std::ostream& os, JS& js) const {

	// std::cout << "EVAL: " << *this << std::endl;

	boost_regex_replace_sv(*this, var, os, [&js](const auto& match) -> wString {
		wString code(match[1].first, match[1].second - match[1].first);
		return js.evaluate(code);
	});

}

void if_block::eval(std::ostream& os, JS& js) const {
	//if (_true.size() > 0) {
		//evaluate(_true, os);
	//}


	auto r = js.evaluate(cond);

	if (r[0] == '0') {
		if (_false.size() > 0)
			evaluate(_false, os, js);
	}
	else {
		if (_true.size() > 0)
			evaluate(_true, os, js);
	}

	std::cout << "IF COND: " << r.c_str() << std::endl;
}

void for_block::eval(std::ostream& os, JS& js) const {
	//for (int i = 0; i < 3; ++i)
		//evaluate(b._loop, os);

	auto a = js.root->getParameter(cond);
	if (!a) {
		std::cerr << "Unknown var: " << cond << std::endl;
		return;
	}

	if (!a->isArray()) {
		std::cerr << "Not array: " << cond << std::endl;
		return;
	}

	auto len = a->getArrayLength();

	//auto ji = js.root->getParameter("i");
	//auto jv = js.root->getParameter("v");


	for (int i = 0; i < len; ++i) {

		auto v = a->getArrayIndex(i);

		js.root->addChildNoDup("i", new CScriptVar(i));
		js.root->addChildNoDup("v", v);

		evaluate(_loop, os, js);
	}

}

void dump(const sequence& s) {

	struct visitor {

		void operator()(const plain& b) {
			// todo: varibles

			//std::println("PLAIN: {0}", b);
			std::cout << "PLAIN: " << b << std::endl;
		}

		void operator()(const if_block& b) {
			std::println("IF");

			if (b._true.size() > 0) {
				std::println("TRUE");
				dump(b._true);
			}

			if (b._false.size() > 0) {
				std::println("FALSE");
				dump(b._false);
			}

			std::println("ENDIF");
		}

		void operator()(const for_block& b) {
			std::println("FOR");
			dump(b._loop);
			std::println("ENDFOR");

		}
	};

	visitor v;

	for (auto&& i : s) {
		std::visit(v, i);
	}

}

void evaluate(const sequence& s, std::ostream& os, JS& js) {

	struct visitor {

		std::ostream& os;
		JS& js;

		visitor(std::ostream& s, JS& js) : os(s), js(js) {}

		void operator()(const plain& b) {
			b.eval(os, js);
		}

		void operator()(const if_block& b) {
			b.eval(os, js);
		}

		void operator()(const for_block& b) {
			b.eval(os, js);
		}
	};

	visitor v(os, js);

	for (auto&& i : s) {
		std::visit(v, i);
	}

}

std::string evaluate(const sequence& s, JS& js) {
	std::ostringstream os;

	evaluate(s, os, js);

	return std::regex_replace(os.str(), rm, "><");
}

std::string evaluate(const sequence& s) {
	JS js;

	return  evaluate(s, js);
}


// struct Block : std::variant<block> {

// 	enum {
// 		PLAIN,
// 		FOR,
// 		IF
// 	};
// };

std::string load_file(const std::string& path) {
	std::ifstream in(path);
	if (!in) throw std::runtime_error("Failed to open file: " + path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

std::string remove_html_comments(const std::string& content) {
	// Regex matches <!-- ... --> including multiline comments
	static const std::regex html_comment(R"(<!--[\s\S]*?-->)", std::regex::optimize);
	return std::regex_replace(content, html_comment, "");
}


std::string render_template(const std::string& content, const std::filesystem::path& base_path) {
	
	std::string cleaned = remove_html_comments(content);
	
	std::regex include_regex(R"(<%\s*include\s*\(\s*['"]([^'"]+)['"]\s*\)\s*%>)");
	std::string result;
	std::sregex_iterator iter(cleaned.begin(), cleaned.end(), include_regex);
	std::sregex_iterator end;

	size_t last_pos = 0;
	for (; iter != end; ++iter) {
		auto match = *iter;
		result.append(content, last_pos, match.position() - last_pos); // add text before match

		std::filesystem::path included_file = base_path / match[1].str();
		std::string included_content = load_file(included_file.string());

		if (included_file.extension() == ".ejs")
			included_content = render_template(included_content, included_file.parent_path()); // recursive!

		result += included_content;
		last_pos = match.position() + match.length();
	}

	result.append(content, last_pos, content.length() - last_pos); // add remaining text
	return result;
}

std::string render_template_from_file(const std::filesystem::path& path) {
	std::string content = load_file(path.string());
	return render_template(content, path.parent_path());
}

class Ejs::Script {
	public:

	std::string content;
	sequence blocks;
	JS js;

	bool loadFile(const std::string& file) {

		content = render_template_from_file(file);

		std::println("File loaded: {0}\n{1}", file, content);

		return parse();
	}


	bool parse() {

		std::smatch match;
		int pos = 0, i;

		//for (std::sregex_iterator it(content.begin(), content.end(), re), end; it != end; ++it) {

		auto start = content.cbegin(),
			end = content.cend();

		std::vector<sequence*> stack;

		auto current = &blocks;

		stack.push_back(current);

		auto data = content.data(), s = data, t = s;

		while (std::regex_search(start, end, match, re)) {

			auto keyword = match[1].str();

			std::println("EXP: {0}", keyword);

			start = match.suffix().first;

			//std::advance(iterator, 5);

			auto p = match.position(0), l = match.length(0);
			std::string_view text(s, p);

			text = trim_any(text);

			if (text.size() > 0)
				current->emplace_back(plain(text.data(), text.size()));

			t = s;
			s = s + p + l;

			if (keyword == "if") {
				// auto cond = match[3].str();
				std::string_view cond(t + match.position(3), match.length(3));

				std::println("  {0}", cond);

				stack.push_back(current);

				current->emplace_back(if_block{cond});

				auto& last = std::get<if_block>(current->back());

				current = &last._true;
			}

			else if (keyword == "else") {
				auto& last = std::get<if_block>(stack.back()->back());

				current = &last._false;
			}

			else if (keyword == "endif") {

				stack.pop_back();

				current = stack.back();
			}

			else if (keyword == "for") {
				std::string_view cond(t + match.position(3), match.length(3));

				stack.push_back(current);

				current->emplace_back(for_block{cond});

				auto& last = std::get<for_block>(current->back());

				current = &last._loop;
			}

			else if (keyword == "endfor") {
				stack.pop_back();

				current = stack.back();
			}

			//i = match.position(0)

			start = match.suffix().first;
		}

		std::string_view text(s, content.length() - (s - data));

		text = trim_any(text);

		if (text.size() > 0)
			blocks.emplace_back(plain(text.data(), text.size()));

		dump(blocks);

		// auto html = evaluate(blocks);
		// std::println("HTML:\n{0}", html);

		return true;
	}

	void addVariable(const wString& name, CScriptVar* v) {
		js.root->addChildNoDup(name, v);
	}

	std::string execute() {
		return evaluate(blocks, js);
	}

	CScriptVar* getRoot() {
		return js.root;
	}
};

Ejs::Ejs() : 
	script(new Ejs::Script()) 
{}

Ejs::~Ejs() {}

bool Ejs::loadFile(const std::string& path) {
	return script->loadFile(path);
}

void Ejs::addVariable(const std::string& name, CScriptVar* v) {
	script->addVariable(name.c_str(), v);
}

CScriptVar* Ejs::getRoot() {
	return script->getRoot();
}

std::string Ejs::execute() {
	return script->execute();
}