#ifndef SCALA_NATIVE_BINDGEN_ARRAYTYPE_H
#define SCALA_NATIVE_BINDGEN_ARRAYTYPE_H

#include "Type.h"

class ArrayType : public Type {
  public:
    ArrayType(std::shared_ptr<Type> elementsType, uint64_t size);

    ~ArrayType() override;

    bool usesType(std::shared_ptr<Type> type) const override;

    std::string str() const override;

    static std::shared_ptr<Type> of(std::shared_ptr<Type> type, uint64_t size);

  private:
    const uint64_t size;
    std::shared_ptr<Type> elementsType;
};

#endif // SCALA_NATIVE_BINDGEN_ARRAYTYPE_H
