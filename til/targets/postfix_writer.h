#ifndef __SIMPLE_TARGETS_POSTFIX_WRITER_H__
#define __SIMPLE_TARGETS_POSTFIX_WRITER_H__

#include "targets/basic_ast_visitor.h"

#include <sstream>
#include <set>
#include <stack>
#include <optional>
#include <cdk/types/basic_type.h>
#include <cdk/emitters/basic_postfix_emitter.h>

namespace til {

  //!
  //! Traverse syntax tree and generate the corresponding assembly code.
  //!
  class postfix_writer: public basic_ast_visitor {
    cdk::symbol_table<til::symbol> &_symtab;
    std::set<std::string> _external_func_to_declare;
    std::optional<std::string> _external_func_name;

    // semantic analysis
    bool _errors , _inFunctionArgs;
    std::stack<bool> _globals; // for deciding whether a variable is global or not
    int _offset; // current framepointer offset (0 means no vars defined)
    cdk::typename_type _lvalueType;

    // remember function name for resolving '@'
    std::stack<std::string> _function_labels; // for keeping track of functions
    std::string _current_func_ret_label; // where to jump when a return occurs of an exclusive section ends

    // code generation
    cdk::basic_postfix_emitter &_pf;
    int _lbl;

    std::vector<std::pair<std::string, std::string>> *_cur_func_loop_labels;
    bool _outside_func;
    bool _loop_ended; 

  public:
    postfix_writer(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<til::symbol> &symtab, cdk::basic_postfix_emitter &pf) :
        basic_ast_visitor(compiler), _symtab(symtab), _errors(false), _inFunctionArgs(false),_offset(0), _lvalueType(cdk::TYPE_VOID), 
        _current_func_ret_label(""), _pf(pf), _lbl(0), _outside_func(false), _loop_ended(false) {
    }
  public:
    ~postfix_writer() {
      os().flush();
    }

  protected:
    void prepareIDBinaryExpression(cdk::binary_operation_node * const node, int lvl);
    void prepareIDBinaryComparisonExpression(cdk::binary_operation_node * const node, int lvl);
    void accept_covariant_node(std::shared_ptr<cdk::basic_type> const node_type, cdk::expression_node * const node, int lvl);
    template<size_t P, typename T> void loop_controller(T * const node);


  private:
    /** Method used to generate sequential labels. */
    inline std::string mklbl(int lbl) {
      std::ostringstream oss;
      if (lbl < 0)
        oss << ".L" << -lbl;
      else
        oss << "_L" << lbl;
      return oss.str();
    }

    template<class T>
    inline bool isInstanceOf(cdk::basic_node * const node) {
      return dynamic_cast<T*>(node) != nullptr;
    }
    template<class T, class... Rest, typename std::enable_if<sizeof...(Rest) != 0, int>::type = 0>
    inline bool isInstanceOf(cdk::basic_node * const node) {
      return dynamic_cast<T*>(node) != nullptr || isInstanceOf<Rest...>(node);
    }

  public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end

  };


#define THROW_ERROR_FOR_NODE(subject, msg) { \
  std::cerr << subject->lineno() << ": " << msg << std::endl; \
  return; \
}
#define THROW_ERROR(msg) THROW_ERROR_FOR_NODE(node, msg)
} // til

#endif
