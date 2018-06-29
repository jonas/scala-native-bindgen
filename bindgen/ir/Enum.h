#ifndef SCALA_NATIVE_BINDGEN_ENUM_H
#define SCALA_NATIVE_BINDGEN_ENUM_H

#include "TypeDef.h"
#include "types/PrimitiveType.h"
#include <vector>

class Enumerator {
  public:
    Enumerator(std::string name, int64_t value);

    std::string getName();

    int64_t getValue();

  private:
    std::string name;
    int64_t value;
};

class Enum : public Type, public std::enable_shared_from_this<Enum> {
  public:
    Enum(std::string name, std::shared_ptr<Type> type,
         std::vector<Enumerator> enumerators);

    bool isAnonymous() const;

    std::shared_ptr<TypeDef> generateTypeDef();

    std::string getName() const;
    std::string str() const override;

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &s, const Enum &e);

  private:
    std::string name; // might be empty
    std::shared_ptr<Type> type;
    std::vector<Enumerator> enumerators;
};

#endif // SCALA_NATIVE_BINDGEN_ENUM_H
