#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>

class CScriptVar;

class Ejs {
	public:

	Ejs();
	~Ejs();

	bool loadFile(const std::string&);
	std::string execute();


	typedef std::unordered_map<std::string, std::string> Context;
	std::string execute(const Context&) const;

	void addVariable(const std::string& name, CScriptVar*);
	CScriptVar* getRoot();

	private:

	class Script;
	std::unique_ptr<Script> script;
};