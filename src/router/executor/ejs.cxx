#include "ejs.h"
#include "utils.h"
#include "db.h"

#include <duktape.h>
#include <soci/soci.h>

#include <regex>
#include <string_view>
#include <sstream>
#include <fstream>
#include <print>
#include <filesystem>

#include <boost/regex.hpp>

#define eval_string(ctx,src)  \
	(duk_eval_raw((ctx), src.data(), src.size(), 0 /*args*/ | DUK_COMPILE_EVAL | DUK_COMPILE_SAFE | DUK_COMPILE_NOSOURCE | DUK_COMPILE_NOFILENAME))

#define push_string(ctx,s) \
	duk_push_lstring(ctx, s.data(), s.size());

#define duk_handle(p) ((JS)p)

typedef duk_context* JS;

const std::unordered_map<std::string, soci::data_type> DataType = {
	{ "TEXT", soci::dt_string },
	{ "SERIAL", soci::dt_unsigned_long_long },
	{ "INTEGER", soci::dt_integer },
	{ "DATETIME", soci::dt_date },
	// { "TIME", soci::dt_date },
};

namespace {

 duk_ret_t js_format_date(duk_context *ctx) {
	const char *input_date = duk_require_string(ctx, 0);  // e.g., "2025-05-19T10:20:30"
	const char *format = duk_require_string(ctx, 1);       // e.g., "%Y/%m/%d %H:%M:%S"

	struct tm tm = {0};
	char output[128];

	// Parse input string, adjust the format if needed
	if (strptime(input_date, "%Y-%m-%dT%H:%M:%S", &tm) == NULL) {
		return duk_error(ctx, DUK_ERR_TYPE_ERROR, "Invalid date format");
	}

	// Format the parsed time
	if (strftime(output, sizeof(output), format, &tm) == 0) {
		return duk_error(ctx, DUK_ERR_ERROR, "Formatting failed");
	}

	duk_push_string(ctx, output);
	return 1; // return one value to JS
}

duk_ret_t js_format_short_date(duk_context *ctx) {
	const char *input_date = duk_require_string(ctx, 0); // "2025-05-15 10:54:37"
	struct tm tm = {0};
	char output[64];

	// Parse input date (space-separated instead of ISO T)
	if (strptime(input_date, "%Y-%m-%d %H:%M:%S", &tm) == NULL) {
		return duk_error(ctx, DUK_ERR_TYPE_ERROR, "Invalid date format");
	}

	// Format to "15 May, 10:54"
	if (strftime(output, sizeof(output), "%d %b, %H:%M", &tm) == 0) {
		return duk_error(ctx, DUK_ERR_ERROR, "Formatting failed");
	}

	duk_push_string(ctx, output);
	return 1; // Return the formatted string
}

JS create_context() {
	auto ctx = duk_create_heap_default();

	duk_push_c_function(ctx, js_format_date, 2); // 2 arguments: date string, format
	duk_put_global_string(ctx, "formatDate");

	duk_push_c_function(ctx, js_format_short_date, 1);
	duk_put_global_string(ctx, "shortDate");

	return ctx;
}

} // namespace

std::string eval(JS ctx, std::string_view code);
std::string evaluate(JS ctx, const sequence& s);

// DB
void add_array(JS, const std::string& name, const rowset&);
void add_object(JS, const std::string& name, const row&);
void add_object(JS, const row&);

int  add_schema_sqlite(JS, const std::string& name, const soci::rowset<soci::row>&);

Ejs::Ejs() : ctx(nullptr) {

}

Ejs::Ejs(Ejs&& other) noexcept
	: ctx(other.ctx),
	  content(std::move(other.content)),
	  blocks(std::move(other.blocks)) {

	other.ctx = nullptr;  // Transfer ownership of the raw pointer
}


Ejs::~Ejs() {
	if (ctx)
		duk_destroy_heap(duk_handle(ctx));
}

Ejs::Env Ejs::env(const char* id) {
	if (id == nullptr) {

		if (!ctx)
			ctx = create_context();

		return { *this, ctx, false };
	}

	return { *this, create_context(), true };
}

std::string Ejs::execute(const std::string& path) {

	loadFile(path);

	auto e = env();

	return e.execute();
}

Ejs::Env::~Env() {
	if (own)
		duk_destroy_heap(duk_handle(ctx));
}

void Ejs::Env::var(const std::string& name, int i) const {
	duk_push_int(duk_handle(ctx), i);
	duk_put_global_string(duk_handle(ctx), name.c_str());
}

void Ejs::Env::var(const std::string& name, double n) const {
	duk_push_number(duk_handle(ctx), n);
	duk_put_global_string(duk_handle(ctx), name.c_str());
}

void Ejs::Env::var(const std::string& name, bool b) const {
	duk_push_boolean(duk_handle(ctx), b ? 1 : 0);
	duk_put_global_string(duk_handle(ctx), name.c_str());
}

void Ejs::Env::var(const std::string& name, const std::string& s) const {
	duk_push_lstring(duk_handle(ctx), s.data(), s.size());
	duk_put_global_string(duk_handle(ctx), name.c_str());
}

void Ejs::Env::var(const std::string& name, const char* s) const {
	duk_push_string(duk_handle(ctx), s);
	duk_put_global_string(duk_handle(ctx), name.c_str());
}

void Ejs::Env::var(const std::string& name, const rowset& rs) const {
	add_array(duk_handle(ctx), name, rs);
}

void Ejs::Env::var(const std::string& name, const row& r) const {
	add_object(duk_handle(ctx), name, r);
}

void Ejs::Env::vars(const row& r) const {
	add_object(duk_handle(ctx), r);
}

Ejs::Env::Object::Object(void* ctx, const std::string& name) :
	ctx(ctx), name(name)
{
	duk_push_object(duk_handle(ctx)); // new JS object
}

Ejs::Env::Object::~Object() {
	duk_put_global_string(duk_handle(ctx), name.c_str());
}

Ejs::Env::Object Ejs::Env::object(const std::string& name) const {
	
	duk_push_object(duk_handle(ctx)); // new JS object
	
	// Set global JS variable "users"
	duk_put_global_string(duk_handle(ctx), name.c_str());

	return { ctx, name };
}

Ejs::Env::Object const& Ejs::Env::Object::var(const std::string& name, const std::string& s) const {
	duk_push_lstring(duk_handle(ctx), s.data(), s.size());
	duk_put_prop_string(duk_handle(ctx), -2, name.c_str());

	return *this;
}

int Ejs::Env::length(const std::string& name) const {

	duk_get_global_string(duk_handle(ctx), name.c_str());
	duk_size_t len = duk_get_length(duk_handle(ctx), -1);
	duk_pop(duk_handle(ctx)); // pop array

	return len;
}

void Ejs::Env::schemaSqlite(const std::string& name, const rowset& rs) const {
	add_schema_sqlite(duk_handle(ctx), name, rs);
}

std::string Ejs::Env::execute() const {
	return evaluate(duk_handle(ctx), ejs.getFlow());
}

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

void boost_regex_replace_sv(
	const std::string_view&	 input,
	const boost::regex& re,
	std::ostream& out,
	const std::function<std::string(const boost::match_results<std::string_view::const_iterator>&)>& replacer
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

void evaluate(const sequence& s, std::ostream& os, JS js) {

	duk_push_undefined(js);
	duk_put_global_string(js, "_a");

	struct visitor {

		std::ostream& os;
		JS ctx;

		void operator()(const plain& b) {

			// std::println("Plain replace:\n{0}", b);

			auto replacer = [this](const auto& match) -> std::string {
				auto p = match[1].first;
				size_t len = match[1].second - match[1].first;

				// std::println("R: {0}  => {1}", len, p);
				
				return eval(ctx, { p, len });
			};

			boost_regex_replace_sv(b, var, os, replacer);

		}

		void operator()(const if_block& b) {
			auto r = eval(ctx, b.cond);
			auto f = r.length() == 0 || (r.length() == 1 && r[0] == '0') || r == "false";

			if (f) {
				if (b._false.size() > 0)
					evaluate(b._false, os, ctx);
			}
			else {
				if (b._true.size() > 0)
					evaluate(b._true, os, ctx);
			}

		}

		void operator()(const for_block& b) {
			// Step 1: Push array onto stack
			// duk_get_global_string(ctx, b.cond.c_str());

			duk_eval_string(ctx,  b.cond.c_str());
			duk_put_global_string(ctx, "_a");

			duk_get_global_string(ctx, "_a");
			if (duk_is_array(ctx, -1)) {
				duk_size_t len = duk_get_length(ctx, -1);
				duk_idx_t idx = duk_get_top_index(ctx); // fixed index for array

				for (duk_uarridx_t i = 0; i < len; ++i) {
					// Get object at index
					duk_get_prop_index(ctx, idx, i); // stack: [... array, obj]

					// Set global 'v' = object
					duk_dup(ctx, -1);
					duk_put_global_string(ctx, "v");

					// Set global 'i' = index
					duk_push_uint(ctx, i);
					duk_put_global_string(ctx, "i");

					evaluate(b._loop, os, ctx);

					duk_pop(ctx); // pop array element
					
				}
			}

			duk_pop(ctx); // pop array
			
		}
	};

	visitor v{os, js};

	for (auto&& i : s) {
		std::visit(v, i);
	}

}

std::string evaluate(JS ctx, const sequence& s) {

	std::ostringstream os;

	evaluate(s, os, ctx);

	auto res = os.str();

	// std::println("EVALUATE:\n{0}", res);

	return std::regex_replace(res, rm, "><");
}

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

	// std::println("AFTER COMMENTS REMOVE\n{0}", cleaned);
	
	std::regex include_regex(R"(<%\s*include\s*\(\s*['"]([^'"]+)['"]\s*\)\s*%>)");
	std::string result;
	std::sregex_iterator iter(cleaned.begin(), cleaned.end(), include_regex);
	std::sregex_iterator end;

	size_t last_pos = 0;
	for (; iter != end; ++iter) {
		auto match = *iter;
		result.append(cleaned, last_pos, match.position() - last_pos); // add text before match

		std::filesystem::path included_file = base_path / match[1].str();
		std::string included_content = load_file(included_file.string());

		if (included_file.extension() == ".ejs")
			included_content = render_template(included_content, included_file.parent_path()); // recursive!

		result += included_content;
		last_pos = match.position() + match.length();
	}

	result.append(cleaned, last_pos, cleaned.length() - last_pos); // add remaining text

	// std::println("AFTER INCLUDE\n{0}", result);

	return result;
}

std::string render_template_from_file(const std::filesystem::path& path) {
	std::string content = load_file(path.string());
	return render_template(content, path.parent_path());
}

sequence parse_ejs(const std::string& content) {

	sequence blocks;

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

		// std::println("EXP: {0}", keyword);

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

			// std::println("  {0}", cond);

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

	// std::println("FOOTER:\n", text);

	if (text.size() > 0)
		blocks.emplace_back(plain(text.data(), text.size()));

	// dump(blocks);

	// auto html = evaluate(blocks);
	// std::println("HTML:\n{0}", html);

	return std::move(blocks);
}

bool Ejs::loadFile(const std::string& file) {

	content = render_template_from_file(file);

	// std::println("File loaded: {0}\n{1}", file, content);

	blocks = parse_ejs(content);

	return true;
}

void add_object_prop(JS ctx, const row& r) {


	for (std::size_t i = 0; i < r.size(); ++i) {
		const soci::column_properties& props = r.get_properties(i);
		const std::string& colName = props.get_name();

		if (r.get_indicator(i) == soci::i_null) {
			duk_push_null(ctx);
		} else {
			switch (props.get_data_type()) {
				case soci::dt_string:
					duk_push_string(ctx, r.get<std::string>(i).c_str());
					break;
				case soci::dt_double:
					duk_push_number(ctx, r.get<double>(i));
					break;
				case soci::dt_integer:
					duk_push_int(ctx, r.get<int>(i));
					break;
				case soci::dt_long_long:
					duk_push_number(ctx, static_cast<double>(r.get<long long>(i)));
					break;
				case soci::dt_unsigned_long_long:
					duk_push_number(ctx, static_cast<double>(r.get<unsigned long long>(i)));
					break;
				case soci::dt_date: {
					std::tm tm = r.get<std::tm>(i);
					char buf[64];
					std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
					duk_push_string(ctx, buf);
					break;
				}
				default:
					duk_push_undefined(ctx);
			}
		}

		duk_put_prop_string(ctx, -2, colName.c_str()); // obj[colName] = value
	}

	
}


void add_object(JS ctx, const row& row) {
	for (std::size_t i = 0; i < row.size(); ++i) {
		const soci::column_properties& props = row.get_properties(i);
		const std::string& colName = props.get_name();

		if (row.get_indicator(i) == soci::i_null) {
			duk_push_null(ctx);
		} else {
			switch (props.get_data_type()) {
				case soci::dt_string:
					duk_push_string(ctx, row.get<std::string>(i).c_str());
					break;
				case soci::dt_double:
					duk_push_number(ctx, row.get<double>(i));
					break;
				case soci::dt_integer:
					duk_push_int(ctx, row.get<int>(i));
					break;
				case soci::dt_long_long:
					duk_push_number(ctx, static_cast<double>(row.get<long long>(i)));
					break;
				case soci::dt_unsigned_long_long:
					duk_push_number(ctx, static_cast<double>(row.get<unsigned long long>(i)));
					break;
				case soci::dt_date: {
					std::tm tm = row.get<std::tm>(i);
					char buf[64];
					std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
					duk_push_string(ctx, buf);
					break;
				}
				case soci::dt_blob: {
					// detect by column name
					if (colName == "md") {
						auto content = db::blob(row, i);
						auto md = gzip_decompress(content);

						auto html = markdown_to_html(md);

						duk_push_string(ctx, html.get());
					}
					else if (colName == "json") {

					}
				}
				break;

				default:
					duk_push_undefined(ctx);
			}
		}

		// Set as global JS variable with the column name
		duk_put_global_string(ctx, colName.c_str());
	}
}

void add_array(JS ctx, const std::string& name, const rowset& rs) {
	// Create JS array
	duk_push_array(ctx);
	int index = 0;

	for (auto& r : rs) {

		duk_push_object(ctx); // new JS object

		add_object_prop(ctx, r);

		duk_put_prop_index(ctx, -2, index++); // array[index] = obj
	}

	// Set global JS variable "users"
	duk_put_global_string(ctx, name.c_str());
}

void add_object(JS ctx, const std::string& name, const row& r) {

	duk_push_object(ctx); // new JS object
	add_object_prop(ctx, r);
	
	// Set global JS variable "users"
	duk_put_global_string(ctx, name.c_str());
}

int add_schema_sqlite(JS ctx, const std::string& name, const soci::rowset<soci::row>& rs) {
	// Create JS array
	duk_push_array(ctx);
	int index = 0;

	for (auto& r : rs) {
		duk_push_object(ctx); // new JS object

		for (std::size_t i = 0; i < r.size(); ++i) {


			auto sqltype = r.get<std::string>(2);
			auto dt = DataType.find(sqltype)->second;

			auto name = r.get<std::string>(1);
			auto type = "text";

			switch (dt) {

				case soci::dt_integer:
				case soci::dt_long_long:
				case soci::dt_unsigned_long_long:
				case soci::dt_double:
				type = "number";
			}
			
			push_string(ctx, name);
			duk_put_prop_string(ctx, -2, "name");

			duk_push_string(ctx, type);
			duk_put_prop_string(ctx, -2, "type");
		}

		duk_put_prop_index(ctx, -2, index++); // array[index] = obj
	}

	// Set global JS variable "users"
	duk_put_global_string(ctx, name.c_str());

	return index;
}

std::string eval(JS ctx, std::string_view code) {

	// std::println("EVAL: {0}", code);

	if (eval_string(ctx, code) != 0) {
		std::string err = duk_safe_to_string(ctx, -1);
		
		throw std::runtime_error(err);
	}

	std::string res = duk_safe_to_string(ctx, -1);

	return res == "undefined" ? "" : std::move(res);
}

