#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>

#include <string.h>
#include <assert.h>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include "BytecodeInterpreter.h"

// ================================================================================

namespace mathvm {
  class VarNum {
    public:
    int16_t add(const AstVar* var) {
      _vars.push_back(var);
      _numMap.insert(make_pair(var, _vars.size() - 1));
      return _vars.size() - 1;
    }

    bool exists(const AstVar* var) {
      return _numMap.find(var) != _numMap.end();
    }
      
    unsigned int getId(const AstVar* var) {
      return _numMap.find(var)->second;
    }

    unsigned int size() const {
      return _numMap.size();
    }

    unsigned int next() const {
      return _numMap.size();
    }

    unsigned int current() const {
      return _numMap.size() - 1;
    }

  private:
    std::vector<const AstVar*> _vars;
    std::map<const AstVar*, unsigned int> _numMap;
  };


#define VISIT(type)                             \
  void visit##type(type* node)


  class ScopeWalker : public AstVisitor {
    typedef std::map<AstVar*, uint16_t> Locals;

    VarNum* _varMap;
    size_t _size;


  public:
    ScopeWalker() { 
      _size = 0;
    }

    void collectLocals(VarNum* varMap, AstNode* function) {
      _varMap = varMap;
      function->visitChildren(this);
    }

    size_t size() const {
      return _size;
    }

    VISIT(BinaryOpNode) {
      return;
    }

    VISIT(UnaryOpNode) {
      return;
    }

    VISIT(StringLiteralNode) {
      return;
    }

    VISIT(DoubleLiteralNode) {
      return;
    }
    
    VISIT(IntLiteralNode) {
      return;
    }
    
    VISIT(LoadNode) {
      return;
    }
   
    VISIT(StoreNode) {
      return;
    }

    VISIT(ForNode) {
      node->visitChildren(this);
    }

    VISIT(WhileNode) {
      node->visitChildren(this);
    }
    
    VISIT(IfNode) {
      node->visitChildren(this);
    }
          
    VISIT(BlockNode) {
      Scope::VarIterator vi(node->scope());
      AstVar *v;

      while (vi.hasNext()) {
        v = vi.next();
        _varMap->add(v);
        _size++;
      }
      
      node->visitChildren(this);
    }
    
    VISIT(FunctionNode) {
      node->visitChildren(this);
    }
     
    VISIT(ReturnNode) {
      return;
    }
     
    VISIT(CallNode) {
      return;
    }

    VISIT(PrintNode) {
      return;
    }
  };

  //--------------------------------------------------------------------------------


  class BytecodeGenerator : public AstVisitor {
  private:
    Bytecode* code;
    BlockNode* _root;
    StringPool *strings;

    VarNum _num;
    std::map<AstNode*, VarType> node_type;
    BytecodeInterpreter *_interpreter;

    std::map<std::string, uint16_t> string_const;
    std::map<AstFunction*, uint16_t> _functions;

    BlockNode* _curBlock;

    FunArgs* _args;
    size_t _argNum;

    // --------------------------------------------------------------------------------

    void put(const AstVar* v) {
      assert(_num.exists(v));
      uint16_t vid = (uint16_t)_num.getId(v);
      assert(vid < _num.size());
      code->addTyped(vid);
        /*} else {
        // This is a function argument

        
        for (size_t i = 0; i < _argNum; ++i) {
          if (v->name() == *((*_args)[i].name)) {
            (*_args)[i].varId = (uint8_t)_num.add(v);
            code->add((*_args)[i].varId);
            break;
          }
        } 
        */
    }
  

    void put(unsigned char* buf, size_t size) {
      for(size_t i = 0; i < size; ++i) {
        code->add(buf[i]);
      }
    }


  public:
    BytecodeGenerator(AstFunction* root) { 
      ScopeWalker w;
      BCIFunction* top;

      w.collectLocals(&_num, root->node());
      
      //_root = root;
      _interpreter = new BytecodeInterpreter;
      
      _interpreter->createFunction(&top, root);
      top->setLocalsNumber(w.size());
 
      code = top->bytecode();
      strings = _interpreter->strings();

      root->node()->body()->visit(this);
      code->add(BC_STOP);

      _interpreter->setVarPoolSize(_num.size());
    }

    Code *getCode() {
      return _interpreter;
    }


    // Visitor interface implementation 

    VarType gen_num_conversions(BinaryOpNode* node) {
      VarType l = node_type[node->left()];
      VarType r = node_type[node->right()];

      if (l != r) {
        if (r == VT_INT) {
          code->add(BC_SWAP);
        }

        code->add(BC_I2D);

        return VT_DOUBLE;
      } else {
        return l;
      }
    }

    void gen_arith(BinaryOpNode* node) {
      VarType t = gen_num_conversions(node);

      switch (t) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tADD:
          code->add(BC_DADD);
          break;

        case tSUB:
          code->add(BC_DSUB);
          break;

        case tMUL:
          code->add(BC_DMUL);
          break;

        case tDIV:
          code->add(BC_DDIV);
          break;

        case tINCRSET:
          code->add(BC_DADD);
          code->add(BC_STOREDVAR);
          put(node->left()->asLoadNode()->var());
          break;

        case tDECRSET:
          code->add(BC_DSUB);
          code->add(BC_STOREDVAR);
          put(node->left()->asLoadNode()->var());
          break;

        default:
          break;
        }
        break;

      case VT_INT:
        switch (node->kind()) {
          // Arithmetics 
        case tADD:
          code->add(BC_IADD);
          break;

        case tSUB:
          code->add(BC_ISUB);
          break;

        case tMUL:
          code->add(BC_IMUL);
          break;

        case tDIV:
          code->add(BC_IDIV);
          break;
          
        case tINCRSET:
          code->add(BC_IADD);
          code->add(BC_STOREIVAR);
          put(node->left()->asLoadNode()->var());
          break;

        case tDECRSET:
          code->add(BC_ISUB);
          code->add(BC_STOREIVAR);
          put(node->left()->asLoadNode()->var());
          break;

        default:
          break;
        }
        break;

      default:
        break;
      }

      node_type[node] = t;
    }

    void gen_comp(BinaryOpNode* node) {
      VarType t = gen_num_conversions(node);

      if (t == VT_DOUBLE) {
        code->add(BC_DCMP);

        code->add(BC_SWAP);    // Remove arguments
        code->add(BC_POP);
        code->add(BC_SWAP);
        code->add(BC_POP);

        code->add(BC_ILOAD0);  // convert to the integer case :)
        //code->add(BC_SWAP);
      }

      switch (node->kind()) {
      case tEQ:
        code->add(BC_IFICMPE);
        break;
          
      case tNEQ:
        code->add(BC_IFICMPNE);
        break;

      case tGT:
        code->add(BC_IFICMPG);
        break;

      case tGE:
        code->add(BC_IFICMPGE);
        break;
          
      case tLT:
        code->add(BC_IFICMPL);
        break;

      case tLE:
        code->add(BC_IFICMPLE);
        break;

      default:
        break;
      }
      code->addInt16(4);

      code->add(BC_ILOAD0); // If the condition doesn't hold
      
      code->add(BC_JA);
      code->addInt16(1);
      
      code->add(BC_ILOAD1); // If the condition holds
      
      node_type[node] = VT_INT;       
    }

    void gen_logic(BinaryOpNode* node) {
      switch(node->kind()) {
      case tOR:
        code->add(BC_IADD);
        code->add(BC_ILOAD0);

        code->add(BC_IFICMPG);
        code->addInt16(4);
        
        code->add(BC_ILOAD0);

        code->add(BC_JA);
        code->add(1);

        code->add(BC_ILOAD1);        
        break;

      case tAND:
        code->add(BC_IMUL);
        break;

      default:
        break;
      }

      node_type[node] = VT_INT;
    }

    VISIT(BinaryOpNode) {
      node->visitChildren(this);

      switch(node->kind()) {
      case tADD:
      case tSUB:
      case tMUL:
      case tDIV:
      case tINCRSET:
      case tDECRSET:
        gen_arith(node);
        break;

      case tEQ:
      case tNEQ:
      case tGT:
      case tLT:
      case tLE:          
        gen_comp(node);
        break;

      case tOR:
      case tAND:
        gen_logic(node);
        break;
        
      default:
        break;
      }

      if (node_type.find(node) == node_type.end()) {
        node_type[node] = node_type[node->left()];
      }
    }

    VISIT(UnaryOpNode) {
      node->operand()->visit(this); 

      switch (node_type[node->operand()]) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tSUB:
          code->add(BC_DNEG);
          break;

        default:
          break;
        }
        break;

      case VT_INT:
        switch (node->kind()) {
        case tSUB:
          code->add(BC_INEG);
          break;

        case tNOT:
          code->add(BC_ILOAD1);
          code->add(BC_SWAP);
          code->add(BC_ISUB);
          break;

        default:
          break;
        }
        break;

      default:
        break;
      }

      node_type[node] = node_type[node->operand()];
    }

    VISIT(DoubleLiteralNode) {
      code->add(BC_DLOAD);
      double val = node->literal();
      code->addDouble(val);

      node_type[node] = VT_DOUBLE;
    }

    VISIT(IntLiteralNode) {
      code->add(BC_ILOAD);
      int64_t val = node->literal();
      code->addInt64(val);

      node_type[node] = VT_INT;
    }


    VISIT(StringLiteralNode) {
      std::map<std::string, uint16_t>::const_iterator it;
      uint16_t id;

      it = string_const.find(node->literal());
      if (it == string_const.end()) {
        strings->push_back(node->literal());
        id = string_const[node->literal()] = strings->size() - 1;        
      } else {
        id = it->second;
      }
      
      code->add(BC_SLOAD);
      code->addUInt16(id);

      node_type[node] = VT_STRING;
    }
 

    VISIT(LoadNode) {
      switch (node->var()->type()) {
      case VT_DOUBLE:
        code->add(BC_LOADDVAR);
        break;

      case VT_INT:
        code->add(BC_LOADIVAR);
        break;

      case VT_STRING:
        code->add(BC_LOADSVAR);
        break;

      default:
        // Should never happen
        break;
      }

      node_type[node] = node->var()->type();
      put(node->var());
    }

    VISIT(StoreNode) {
      node->visitChildren(this);

      switch (node->var()->type()) {
      case VT_DOUBLE:
        if (node->op() == tINCRSET) {
          code->add(BC_LOADDVAR);
          put(node->var());
          code->add(BC_DADD);
        } else if (node->op() == tDECRSET) {
          code->add(BC_LOADDVAR);
          put(node->var());
          code->add(BC_SWAP);
          code->add(BC_DSUB);          
        }
        code->add(BC_STOREDVAR);
        break;

      case VT_INT:
        if (node->op() == tINCRSET) {
          code->add(BC_LOADIVAR);
          put(node->var());
          code->add(BC_IADD);
        } else if (node->op() == tDECRSET) {
          code->add(BC_LOADIVAR);
          put(node->var());
          code->add(BC_SWAP);
          code->add(BC_ISUB);          
        }
        code->add(BC_STOREIVAR);
        break;

      case VT_STRING:
        code->add(BC_STORESVAR);
        break;

      default:
        break;
      }

      node_type[node] = VT_INVALID;
      put(node->var());
    }
    
    VISIT(ForNode) {
      uint32_t jmp_pos;

      _num.add(node->var());
      
      node->inExpr()->asBinaryOpNode()->left()->visit(this);

      code->add(BC_STOREIVAR);
      put(node->var());

      jmp_pos = code->current();
      
      node->body()->visit(this);

      code->add(BC_LOADIVAR);
      put(node->var());

      code->add(BC_ILOAD1);
      code->add(BC_IADD);
      code->add(BC_STOREIVAR);
      put(node->var());

      code->add(BC_LOADIVAR);
      put(node->var());
      node->inExpr()->asBinaryOpNode()->right()->visit(this);      

      code->add(BC_IFICMPLE);
      code->addInt16((int16_t)jmp_pos - code->current() - 2);
    }
  
    VISIT(IfNode) {
      uint32_t jmp_pos;

      node->ifExpr()->visit(this);

      code->add(BC_ILOAD1);
      code->add(BC_IFICMPNE);
      code->addInt16(0);      
      jmp_pos = code->current();

      node->thenBlock()->visit(this);
      
      if (node->elseBlock()) {
        code->add(BC_JA);
        code->addInt16(0);
        code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));

        jmp_pos = code->current();

        node->elseBlock()->visit(this);
        
        code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));
      } else {
        code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));
      }             
    }

    VISIT(WhileNode) {
      uint32_t jmp_pos, cond_pos;

      cond_pos = code->current();
      node->whileExpr()->visit(this);
      code->add(BC_ILOAD1);
      code->add(BC_IFICMPNE);
      code->addInt16(0);
      jmp_pos = code->current();

      node->loopBlock()->visit(this);
      code->add(BC_JA);
      code->addInt16((int16_t)cond_pos - code->current() - 2);

      code->setTyped(jmp_pos - 2, (int16_t)((int32_t)code->current() - jmp_pos));
    }
    
    VISIT(BlockNode) {
      Scope::VarIterator vi(node->scope());
      Scope::FunctionIterator fi(node->scope());

      while (fi.hasNext()) {
        Bytecode* old_code = code;
        AstFunction* af = fi.next();

        /*
        if (af->name() == "<top>") {
          continue;
        }
        */

        Scope::VarIterator argsIt(af->scope());
        ScopeWalker w;
        BCIFunction* cf;
        

        //cf->setArgsNum(af->parametersNumber());
        _interpreter->createFunction(&cf, af);
        cf->setFirstArg(_num.next());

        while (argsIt.hasNext()) {
          AstVar* v = argsIt.next();
          _num.add(v);
        }

        cf->setFirstLocal(_num.next());
        w.collectLocals(&_num, af->node());
        cf->setLocalsNumber(w.size());

        uint16_t funId = _functions.size();
        _functions[af] = funId;

        code = cf->bytecode();
        af->node()->visit(this);

        code = old_code;
      }


      BlockNode* oldBlock = _curBlock;
      _curBlock = node;
      node->visitChildren(this);
      _curBlock = oldBlock;
    }


    VISIT(PrintNode) {
      for (size_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        
        code->add(BC_DUMP);
        code->add(BC_POP);
      }
    }

    VISIT(CallNode) {
      node->visitChildren(this);
      
      code->add(BC_CALL);
      AstFunction *af = _curBlock->scope()->lookupFunction(node->name());
      uint16_t fid = _functions[af] + 1;
      //assert(fid != 0 && af != NULL);
      code->addUInt16(fid);

      node_type[node] = af->returnType();
    }

    VISIT(ReturnNode) {
      node->visitChildren(this);
      code->add(BC_RETURN);
    }

    VISIT(FunctionNode) {
      node->visitChildren(this);
    }

#undef VISIT

  };

  // --------------------------------------------------------------------------------  

  class BytecodeTranslator : public Translator {
  public:
    Status* translate(const string& program, Code** code) {
      Parser parser;

      Status *status = parser.parseProgram(program);
      if (!status) {
        BytecodeGenerator bg(parser.top());

        *code = bg.getCode();
      }

      return status;
    }
  };

  // --------------------------------------------------------------------------------

  Translator* Translator::create(const string& impl) {
    return new BytecodeTranslator;
  }  
}