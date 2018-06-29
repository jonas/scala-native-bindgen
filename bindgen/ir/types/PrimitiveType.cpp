#include "PrimitiveType.h"
#include "../../Utils.h"

PrimitiveType::PrimitiveType(std::string type) : type(std::move(type)) {}

std::string PrimitiveType::str() const { return handleReservedWords(type); }

std::string PrimitiveType::getType() const { return type; }

bool PrimitiveType::usesType(std::shared_ptr<Type> type) const {
    if (this == type.get()) {
        return true;
    }
    return str() == type->str();
}

std::shared_ptr<Type> PrimitiveType::of(std::string type) {
    return std::make_shared<PrimitiveType>(type);
}

std::shared_ptr<Type> PrimitiveType::BYTE = PrimitiveType::of("Byte");
std::shared_ptr<Type> PrimitiveType::LONG = PrimitiveType::of("native.CLong");
std::shared_ptr<Type> PrimitiveType::UNSIGNED_LONG = PrimitiveType::of("native.CUnsignedLong");
std::shared_ptr<Type> PrimitiveType::UNSIGNED_INT = PrimitiveType::of("native.CUnsignedInt");
