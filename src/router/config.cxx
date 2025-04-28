
#include "config.h"

std::vector<Script::Variable> Script::roots;

Script::Script() : CTinyJS() {

	for (auto&& [name, v] : roots)
		root->addChild(name, v.get());

}

void Script::addRoot(const wString& name, CScriptVar* v) {
	
	roots.emplace_back(name, v->setRef());
}
