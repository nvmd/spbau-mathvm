/*
 * interpreter_code_impl.h
 *
 *  Created on: Nov 20, 2012
 *      Author: alex
 */

#ifndef INTERPRETER_CODE_IMPL_H_
#define INTERPRETER_CODE_IMPL_H_

#include "mathvm.h"
#include <stack>

namespace mathvm {

union ContextVar {
    double _doubleValue;
    int64_t _intValue;
    uint16_t _stringId;
};

class Context {
	Context* _parent;
	uint32_t _ip;
	BytecodeFunction* _function;
	vector<ContextVar> _variables;
public:
	Context(Context* parent, BytecodeFunction* function)
			: _parent(parent), _ip(0), _function(function),
			  _variables(function->localsNumber() + function->parametersNumber()) {
	}

	~Context() {
	}

	uint32_t ip() const {
		return _ip;
	}

	void setIp(uint32_t ip) {
		_ip = ip;
	}

	Bytecode* bytecode() {
		return _function->bytecode();
	}

	Context* parent() {
		return _parent;
	}

	Context* getContext(uint16_t id) {
		if (id == _function->id()) {
			return this;
		}

		// TODO: remove, debug only.
		if (_parent == 0) {
			cout << "null context parent for " << id << endl;
			return 0;
		}

		return _parent->getContext(id);
	}

	ContextVar* getVar(uint16_t index) {
		assert(_variables.size() > index);
		return &_variables[index];
	}

	uint16_t variables() const {
		return _variables.size();
	}
};

class InterpreterCodeImpl: public mathvm::Code {
public:
	InterpreterCodeImpl(ostream& out);
	virtual ~InterpreterCodeImpl();
    BytecodeFunction* bytecodeFunctionById(uint16_t index) const;
    BytecodeFunction* bytecodeFunctionByName(const string& name) const;
    virtual Status* execute(vector<Var*>& vars);

private:
    uint16_t nextUInt16();
    int64_t nextInt();
    double nextDouble();

    int64_t popInt();
    double popDouble();
    uint16_t popStringId();
    void pushInt(int64_t value);
    void pushDouble(double value);
    void pushString(uint16_t id);

    void loadVar(uint16_t varId);
    void loadContextVar();
    void storeContextIntVar();
    void storeContextDoubleVar();
    void storeContextStringVar();
    void storeIntVar(uint16_t varId);
    void storeStringVar(uint16_t varId);
    void storeDoubleVar(uint16_t varId);
    void callFunction(uint16_t id);

    template<class Comparator>
    void jump(Comparator comparator) {
    	int left = popInt();
    	int right = popInt();
    	if (comparator(left, right)) {
    		_ip += _bp->getInt16(_ip);
    	} else {
    		_ip += 2;
    	}
    }

    void returnFromFunction();

private:

    std::stack<ContextVar> _stack;
	Bytecode* _bp;
	uint32_t _ip;
	Context* _context;
	ostream& _out;
};

} /* namespace mathvm */
#endif /* INTERPRETER_CODE_IMPL_H_ */
