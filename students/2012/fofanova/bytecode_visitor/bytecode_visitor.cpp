#include "bytecode_visitor.h"
#include "ast.h"

using namespace std;
using namespace mathvm;

void ByteCodeVisitor::castTOSToInt() {
	if (typeStack.top() == VT_INT)
	{
		return;
	}
	else if (typeStack.top() == VT_DOUBLE)
	{
		bytecode->addInsn(BC_D2I);
	}
	else if (typeStack.top() == VT_STRING)
	{
		bytecode->addInsn(BC_S2I);
	}
	typeStack.pop();
	typeStack.push(VT_INT);
}

void ByteCodeVisitor::castTOSToDouble() {
	if (typeStack.top() == VT_DOUBLE)
	{
		return;
	}
	else if (typeStack.top() == VT_INT)
	{
		bytecode->addInsn(BC_I2D);
	}
	else if (typeStack.top() == VT_STRING)
	{
		bytecode->addInsn(BC_S2I);
		bytecode->addInsn(BC_I2D);
	}
	typeStack.pop();
	typeStack.push(VT_DOUBLE);
}

void ByteCodeVisitor::castTwoOperands(bool onlyInt) {
	VarType TOStype = typeStack.top();
	typeStack.pop();
	VarType UOStype = typeStack.top();
	typeStack.pop();
	if (!TOStype || !UOStype)
	{
		std::cout << "Can't process binary operation!";	
	}
	if ((UOStype == VT_INT && TOStype == VT_INT) || (!onlyInt && UOStype == VT_DOUBLE && TOStype == VT_DOUBLE))
	{
		typeStack.push(UOStype);
		typeStack.push(TOStype);
		return;
	}
	if (!onlyInt && (TOStype == VT_DOUBLE || UOStype == VT_DOUBLE))
	{
		VarType toChange = TOStype;
		if (TOStype == VT_DOUBLE)
		{
			bytecode->addInsn(BC_SWAP);
			toChange = UOStype;
		}
		if (toChange == VT_STRING)
		{
			bytecode->addInsn(BC_S2I);
		} 
		bytecode->addInsn(BC_I2D);
		if (TOStype == VT_DOUBLE)
		{
			bytecode->addInsn(BC_SWAP);
		}
		typeStack.push(VT_DOUBLE);
		typeStack.push(VT_DOUBLE);
		return;
	} 
	else
	{
		if (TOStype == VT_STRING)
		{
			bytecode->addInsn(BC_S2I);
		}
		if (TOStype == VT_DOUBLE)
		{
			bytecode->addInsn(BC_D2I);
		}
		if (UOStype != VT_INT)
		{
			bytecode->addInsn(BC_SWAP);
			if (UOStype == VT_STRING)
			{
				bytecode->addInsn(BC_S2I);
			}
			if (UOStype == VT_DOUBLE)
			{
				bytecode->addInsn(BC_D2I);
			}
			bytecode->addInsn(BC_SWAP);
		}
		typeStack.push(VT_INT);
		typeStack.push(VT_INT);
	}

}

void ByteCodeVisitor::visitBlockNode(BlockNode *node) {
	map<string, int> before;
    Scope::VarIterator var(node->scope());
    while(var.hasNext()) {
			AstVar* cur_var = var.next();
			if(vars.count(cur_var->name()) > 0) {
				before[cur_var->name()] = vars[cur_var->name()];
			}
			vars[cur_var->name()] = last_id++;
    }
    Scope::FunctionIterator func(node->scope());
    while(func.hasNext())
		{
			AstFunction* function= func.next();
      		BytecodeFunction* bytecodeFunction = new BytecodeFunction(function);
      		code->addFunction(bytecodeFunction);
		}
	Scope::FunctionIterator func2(node->scope());
    while(func2.hasNext())
		{
			AstFunction* function= func2.next();
      		BytecodeFunction* bytecodeFunction = (BytecodeFunction*)code->functionByName(function->name());
      		uint32_t i = bytecode->length();
			function->node()->visit(this);
			
			for(i+=3/*JA*/;i < bytecode->length(); ++i) {
				bytecodeFunction->bytecode()->add(bytecode->get(i));
			}
		}
    for (uint32_t i = 0; i < node->nodes(); ++i)
    {
    	node->nodeAt(i)->visit(this);
    }

    map<string, int>::iterator iter = before.begin();
    while(iter != before.end()) {
    	vars[iter->first] = iter->second;
    	iter++;
    }
}

void ByteCodeVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        AstNode *pNode = node->operandAt(i);
        pNode->visit(this);
				switch (typeStack.top())
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_IPRINT);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_DPRINT);
						break;
					}
					case VT_STRING:
					{
						bytecode->addInsn(BC_SPRINT);
						break;
					}
					default:
						std::cout << "Can't print properly!\n";
				}
				typeStack.pop();
    }
}

void ByteCodeVisitor::visitLoadNode(LoadNode *node) {
		switch (node->var()->type())
		{
			case VT_INT:
			{
				typeStack.push(VT_INT);
				bytecode->addInsn(BC_LOADIVAR);
				bytecode->addUInt16(vars[node->var()->name()]);
				break;
			}
			case VT_DOUBLE:
			{
				typeStack.push(VT_DOUBLE);
				bytecode->addInsn(BC_LOADDVAR);
				bytecode->addUInt16(vars[node->var()->name()]);
				break;
			}
			case VT_STRING:
			{
				typeStack.push(VT_STRING);
				bytecode->addInsn(BC_LOADSVAR);
				bytecode->addUInt16(vars[node->var()->name()]);
				break;
			}
			default:
			{
				std::cout << "Can't load variable!\n";
			}
		}
    node->var()->name();
}

void ByteCodeVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
	Label else_label(bytecode);
	Label end(bytecode);
	bytecode->addInsn(BC_ILOAD1);
	bytecode->addBranch(BC_IFICMPNE, else_label);
    node->thenBlock()->visit(this);
	bytecode->addBranch(BC_JA,end);
	bytecode->bind(else_label);
	if (node->elseBlock())
	{	
    	node->elseBlock()->visit(this);
    }
	bytecode->bind(end);
}

void ByteCodeVisitor::visitCallNode(CallNode *node) {
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);	
        		typeStack.pop();
				switch (code->functionByName(node->name())->parameterType(i))
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[code->functionByName(node->name())->parameterName(i)]);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[code->functionByName(node->name())->parameterName(i)]);
						break;
					}
					case VT_STRING:
					{
						bytecode->addInsn(BC_STORESVAR);
						bytecode->addUInt16(vars[code->functionByName(node->name())->parameterName(i)]);
						break;
					}
					default:
					{
						std::cout << "Can't call function!\n";
					}
				}	
    }
    typeStack.push(code->functionByName(node->name())->returnType());
	bytecode->addInsn(BC_CALL);
	bytecode->addUInt16(code->functionByName(node->name())->id());
}

void ByteCodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	bytecode->addInsn(BC_DLOAD);
	bytecode->addDouble(node->literal());
    typeStack.push(VT_DOUBLE);
}

void ByteCodeVisitor::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
		switch (node->op())
		{
			case tINCRSET:
			{	
				switch (node->var()->type())
				{
					case VT_INT:
					{
						castTOSToInt();
						bytecode->addInsn(BC_LOADIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_IADD);
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_DOUBLE:
					{
						castTOSToDouble();
						bytecode->addInsn(BC_LOADDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_DADD);
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					default:
					{
						std::cout << "Can't store variable!\n";
					}
				}
				break;
			}
			case tDECRSET:
			{				
				switch (node->var()->type())
				{
					case VT_INT:
					{
						castTOSToInt();
						bytecode->addInsn(BC_LOADIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_ISUB);
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_DOUBLE:
					{
						castTOSToDouble();
						bytecode->addInsn(BC_LOADDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_DSUB);
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					default:
					{
						std::cout << "Can't store variable!\n";
					}
				}
				break;
			}
			case tASSIGN:
			{	
				switch (node->var()->type())
				{
					case VT_INT:
					{
						castTOSToInt();
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_DOUBLE:
					{
						castTOSToDouble();
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_STRING:
					{
						if (typeStack.top() != VT_STRING)
						{
							std::cout << "Undefined behaviour! Can't cast to string!" << endl;
						}
						bytecode->addInsn(BC_STORESVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					default:
					{
						std::cout << "Can't store variable!\n";
					}
				}
				break;
			}
			default:
			{
				std::cout << "Can't store variable!\n";
			}
		}
}

void ByteCodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
	bytecode->addInsn(BC_SLOAD);
	bytecode->addInt16(code->makeStringConstant(node->literal()));
    typeStack.push(VT_STRING);
}

void ByteCodeVisitor::visitWhileNode(WhileNode *node) {
	Label while_label(bytecode);
	Label end(bytecode);
	bytecode->bind(while_label);
   	node->whileExpr()->visit(this);
	bytecode->addInsn(BC_ILOAD1);
	bytecode->addBranch(BC_IFICMPNE, end);
   	node->loopBlock()->visit(this);
	bytecode->addBranch(BC_JA, while_label);
	bytecode->bind(end);
}

void ByteCodeVisitor::visitForNode(ForNode *node) {
	int indexId = vars[node->var()->name()];

	BinaryOpNode *range = (BinaryOpNode *) node->inExpr();
	range->left()->visit(this);
	bytecode->addInsn(BC_STOREIVAR);
	bytecode->addUInt16(indexId);

	Label for_label(bytecode);
	Label end(bytecode);
	bytecode->bind(for_label);

   	range->right()->visit(this);
	bytecode->addInsn(BC_LOADIVAR);
	bytecode->addUInt16(indexId);
	bytecode->addBranch(BC_IFICMPG, end);
   	node->body()->visit(this);

   	//inc
	bytecode->addInsn(BC_LOADIVAR);
	bytecode->addUInt16(indexId);
	bytecode->addInsn(BC_ILOAD1);
	bytecode->addInsn(BC_IADD);
	bytecode->addInsn(BC_STOREIVAR);
	bytecode->addUInt16(indexId);

	bytecode->addBranch(BC_JA, for_label);
	bytecode->bind(end);
}

void ByteCodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
	bytecode->addInsn(BC_ILOAD);
	bytecode->addInt64(node->literal());
    typeStack.push(VT_INT);
}

void ByteCodeVisitor::visitFunctionNode(FunctionNode *node) {
    if(node->name() == "<top>")
    {
		node->body()->visit(this);
		return;
    }
	Label end(bytecode);
	bytecode->addBranch(BC_JA, end);
    for (uint32_t j = 0; j < node->parametersNumber(); j++) {
		vars[node->parameterName(j)] = last_id++;
    }
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode())
    {
		node->body()->nodeAt(0)->visit(this);
    }
    else
    {
    	node->body()->visit(this);
    }
    //UOStype = TOStype;
	//TOStype = node->returnType();
	bytecode->bind(end);
}

void ByteCodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    node->right()->visit(this);
		switch(node->kind())
		{
			case tAND:
			{
				castTwoOperands(1);
				Label right(bytecode);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, right);
				bytecode->addInsn(BC_POP);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->bind(right);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				break;
			}
			case tOR:
			{
				castTwoOperands(1);
				Label true_end_pop(bytecode);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end_pop);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end_pop);
				bytecode->addInsn(BC_POP);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				break;
			}
			case tEQ:
			{
				castTwoOperands(0);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				break;
			}
			case tNEQ:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				break;
			}
			case tGT:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPG, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				break;
			}
			case tGE:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPGE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				break;
			}
			case tLT:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPL, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				break;
			}
			case tLE:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPLE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				break;
			}

			case tADD:
			{
				castTwoOperands(0);
				if (typeStack.top() == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DADD);
				}
				else if (typeStack.top() == VT_INT)
				{
					bytecode->addInsn(BC_IADD);
				}
				else
				{
					std::cerr << "Impossible adding!\n";
				}
				typeStack.pop();
				break;
			}			
			case tSUB:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				if (typeStack.top() == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DSUB);
				}
				else if (typeStack.top() == VT_INT)
				{
					bytecode->addInsn(BC_ISUB);
				}
				else
				{
					std::cerr << "Impossible subtraction!\n";
				}
				typeStack.pop();
				break;
			}
			case tMUL:
			{
				castTwoOperands(0);
				if (typeStack.top() == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DMUL);
				}
				else if (typeStack.top() == VT_INT)
				{
					bytecode->addInsn(BC_IMUL);
				}
				else
				{
					std::cerr << "Impossible multiplication!\n";
				}
				typeStack.pop();
				break;
			}			
			case tDIV:
			{
				castTwoOperands(0);
				bytecode->add(BC_SWAP);
				if (typeStack.top() == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DDIV);
				}
				else if (typeStack.top() == VT_INT)
				{
					bytecode->addInsn(BC_IDIV);
				}
				else
				{
					std::cerr << "Impossible dividing!\n";
				}
				typeStack.pop();
				break;
			}			
			case tMOD:
			{
				castTwoOperands(1);
				bytecode->add(BC_SWAP);
				if (typeStack.top() == VT_INT)
				{
					bytecode->addInsn(BC_IMOD);
				}
				else
				{
					std::cerr << "Impossible dividing by mod!\n";
				}
				typeStack.pop();
				break;
			}
			default:
			{
					std::cerr << "Impossible binary operation!\n";
			}
		}
}

void ByteCodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
		switch(node->kind())
		{
			case tNOT:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->bind(end);
				break;
			}
			case tSUB:
			{
				if (typeStack.top() == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DNEG);
				}
				else if (typeStack.top() == VT_INT)
				{
					bytecode->addInsn(BC_INEG);
				}
				else
				{
					std::cerr << "Impossible negating!\n";
				}
				break;
			}
			default:
			{
					std::cerr << "Impossible unary operation!\n";
			}
		}
}

void ByteCodeVisitor::visitReturnNode(ReturnNode *node) {
	if(node->returnExpr()) {
		node->returnExpr()->visit(this);		
	}
    bytecode->addInsn(BC_RETURN);
}

void ByteCodeVisitor::visitNativeCallNode(NativeCallNode *node) {
    bytecode->addInsn(BC_CALLNATIVE);
    bytecode->addUInt16(code->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0));
}
