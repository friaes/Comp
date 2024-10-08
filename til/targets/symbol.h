#ifndef __TIL_TARGETS_SYMBOL_H__
#define __TIL_TARGETS_SYMBOL_H__

#include <string>
#include <memory>
#include <cdk/types/basic_type.h>

namespace til {

  class symbol {
    std::shared_ptr<cdk::basic_type> _type;
    std::string _name;
    long _qualifier;
    int _offset = 0;
    bool _is_main = false; 

  public:
    symbol(std::shared_ptr<cdk::basic_type> type, const std::string &name, long qualifier) :
        _type(type), _name(name), _qualifier(qualifier) {
    }

    virtual ~symbol() {
      // EMPTY
    }

    std::shared_ptr<cdk::basic_type> type() const {
      return _type;
    }
    bool is_typed(cdk::typename_type name) const {
      return _type->name() == name;
    }
    const std::string &name() const {
      return _name;
    }
    long qualifier() const {
      return _qualifier;
    }
    long qualifier(long v) {
      return _qualifier = v;
    }
    int offset() const {
      return _offset;
    }
    int offset(int o) {
      return _offset = o;
    }
    bool global() const {
      return _offset == 0;
    }
    bool is_main() const {
      return _is_main;
    }
    bool is_main(bool b) {
      return _is_main = b;
    }
  };
  
  inline auto make_symbol(const std::string &name, std::shared_ptr<cdk::basic_type> type, int qualifier = 0) {
    return std::make_shared<symbol>(type, name, qualifier);
  }
  
} // til

#endif
