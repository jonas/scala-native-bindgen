#include "ArrayType.h"
#include "../../Utils.h"

ArrayType::ArrayType(std::shared_ptr<Type> elementsType, uint64_t size)
    : size(size), elementsType(elementsType) {}

std::string ArrayType::str() const {
    return "native.CArray[" + elementsType->str() + ", " +
           uint64ToScalaNat(size) + "]";
}

bool ArrayType::usesType(std::shared_ptr<Type> type) const {
    return this == type.get() || elementsType == type;
}

ArrayType::~ArrayType() {}

std::shared_ptr<Type> ArrayType::of(std::shared_ptr<Type> type, uint64_t size) {
    return std::make_shared<ArrayType>(type, size);
}
