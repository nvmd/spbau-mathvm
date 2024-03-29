#include "printer.h"

using namespace std;
using namespace mathvm;

namespace mathvm {

string escape(const string & str) {
	string escaped;

	for (string::const_iterator it = str.begin(); it != str.end(); ++it) {
		switch (*it) {
			case '\n': escaped += "\\n"; break;
			case '\t': escaped += "\\n"; break;
			case '\b': escaped += "\\n"; break;
			case '\r': escaped += "\\n"; break;
			case '\\': escaped += "\\n"; break;
			default:   escaped += *it;	
		}
	}
	return escaped;
}

void Printer::visitBinaryOpNode(BinaryOpNode* node) {
	out << "(";
	node->left()->visit(this);
	out << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	out << ")";
}

void Printer::visitUnaryOpNode(UnaryOpNode* node) {
    out << tokenOp(node->kind());
    node->operand()->visit(this);
}

void Printer::visitStringLiteralNode(StringLiteralNode* node) {
    out << "'" << escape(node->literal()) << "'";
}

void Printer::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    out << node->literal();
}

void Printer::visitIntLiteralNode(IntLiteralNode* node) {
    out << node->literal();
}

void Printer::visitLoadNode(LoadNode* node) {
    out << node->var()->name();
}

void Printer::visitStoreNode(StoreNode* node) {
    out << node->var()->name();
    out << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
}

void Printer::visitForNode(ForNode* node) {
    out << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    out << ") ";
    node->body()->visit(this);
}

void Printer::visitWhileNode(WhileNode* node) {
    out << "while (";
    node->whileExpr()->visit(this);
    out << ") ";
    node->loopBlock()->visit(this);
}

void Printer::visitIfNode(IfNode* node) {
    out << "if (";
    node->ifExpr()->visit(this);
    out << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        out << " else ";
        node->elseBlock()->visit(this);
    }
}

void Printer::visitBlockNode(BlockNode* node) {
    out << "{\n";
    printBlockContents(node);
    out << "}\n";
}

void Printer::printBlockContents(BlockNode* node) {
    // functions delcarations
    Scope::FunctionIterator funIt(node->scope());
    while (funIt.hasNext()) {
        funIt.next()->node()->visit(this);
        out << '\n';
    }

	// variables declarations
    Scope::VarIterator varIt(node->scope());
    while (varIt.hasNext()) {
        AstVar* var = varIt.next();
        out << typeToName(var->type()) 
        	<< " " << var->name() << ";\n";
    }

    // nodes
    for (size_t i = 0; i < node->nodes(); ++i) {
        AstNode* subNode = node->nodeAt(i);
        subNode->visit(this);
        if (!subNode->isIfNode() && 
        	!subNode->isWhileNode() && 
        	!subNode->isForNode()) {
            out << ';';
        }
        out << '\n';
    }
}

void Printer::visitFunctionNode(FunctionNode* node) {
    out << "function " << typeToName(node->returnType()) 
    	<< " " << node->name() << "(";
   	// parameters
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        out << typeToName(node->parameterType(i)) << " " 
            << node->parameterName(i);

        if (i != node->parametersNumber() - 1)
        	out << ", ";
    }
    out << ") ";

	// body
    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
        out << "\n";
    } else {
        node->body()->visit(this);
    }
}

void Printer::visitReturnNode(ReturnNode* node) {
    out << "return ";
    node->visitChildren(this);
}

void Printer::visitCallNode(CallNode* node) {
    out << node->name() << "(";
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);

        if (i != node->parametersNumber() - 1)
        	out << ", ";
    }
    out << ") ";
}

void Printer::visitNativeCallNode(NativeCallNode* node) {
    out << " native '" << node->nativeName() << "';";
}

void Printer::visitPrintNode(PrintNode* node) {
    out << "print (";
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);

        if (i != node->operands() - 1)
        	out << ", ";
    }
    out << ")";
}

} // namespace mathvm

