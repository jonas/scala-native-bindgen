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

std::shared_ptr<Type> PrimitiveType::LONG = std::make_shared<PrimitiveType>("native.CLong");
std::shared_ptr<Type> PrimitiveType::UNSIGNED_LONG = std::make_shared<PrimitiveType>("native.CUnsignedLong");
std::shared_ptr<Type> PrimitiveType::UNSIGNED_INT = std::make_shared<PrimitiveType>("native.CUnsignedInt");
