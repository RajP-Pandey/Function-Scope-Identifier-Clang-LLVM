#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include <fstream>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> {
public:
    explicit FunctionVisitor(ASTContext *Context)
        : Context(Context) {}

    bool VisitFunctionDecl(FunctionDecl *Func) {
        if (Func->hasBody()) {
            std::string functionName = Func->getNameInfo().getName().getAsString();
            SourceManager &SM = Context->getSourceManager();
            SourceLocation StartLoc = Func->getBeginLoc();
            SourceLocation EndLoc = Func->getEndLoc();
            unsigned startLine = SM.getSpellingLineNumber(StartLoc);
            unsigned endLine = SM.getSpellingLineNumber(EndLoc);

            std::ofstream outFile;
            outFile.open("function_scopes.txt", std::ios_base::app); // Open in append mode
            outFile << "Function: " << functionName 
                    << "\nStart Line: " << startLine 
                    << "\nEnd Line: " << endLine << "\n\n";
            outFile.close();
        }
        return true;
    }

private:
    ASTContext *Context;
};

class FunctionASTConsumer : public ASTConsumer {
public:
    explicit FunctionASTConsumer(ASTContext *Context)
        : Visitor(Context) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    FunctionVisitor Visitor;
};

class FunctionFrontendAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        return std::make_unique<FunctionASTConsumer>(&CI.getASTContext());
    }
};

static llvm::cl::OptionCategory ToolingSampleCategory("tooling-sample");

int main(int argc, const char **argv) {
    // Parse command-line options
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolingSampleCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = *ExpectedParser;

    // Create the Clang Tool
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    // Run the tool with the specified frontend action
    return Tool.run(newFrontendActionFactory<FunctionFrontendAction>().get());
}

