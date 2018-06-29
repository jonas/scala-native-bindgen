#ifndef SCALA_NATIVE_BINDGEN_SIMPLETYPE_H
#define SCALA_NATIVE_BINDGEN_SIMPLETYPE_H

#include "Type.h"
#include <string>

/**
 * For example native.CInt
 */
class PrimitiveType : public Type {
  public:
    explicit PrimitiveType(std::string type);

    std::string getType() const;

    bool usesType(std::shared_ptr<Type> type) const override;

    std::string str() const override;

    static std::shared_ptr<Type> of(std::string type);

    static std::shared_ptr<Type> BYTE;
    static std::shared_ptr<Type> LONG;
    static std::shared_ptr<Type> UNSIGNED_LONG;
    static std::shared_ptr<Type> UNSIGNED_INT;

  private:
    std::string type;
};

#endif // SCALA_NATIVE_BINDGEN_SIMPLETYPE_H
