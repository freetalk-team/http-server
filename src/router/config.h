
#include "TinyJS.h"

#include <vector>
#include <memory>

class Script : public CTinyJS {

public:

	Script();
	
	static void addRoot(const wString& name, CScriptVar* v);

private:

	using Variable = std::pair<wString, std::unique_ptr<CScriptVar>>;
	
	static std::vector<Variable> roots;
};
