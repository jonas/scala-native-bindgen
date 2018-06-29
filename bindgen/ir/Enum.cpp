#include "Enum.h"

Enumerator::Enumerator(std::string name, int64_t value)
    : name(std::move(name)), value(value) {}

std::string Enumerator::getName() { return name; }

int64_t Enumerator::getValue() { return value; }

Enum::Enum(std::string name, std::shared_ptr<Type> type,
           std::vector<Enumerator> enumerators)
    : name(std::move(name)), type(type),
      enumerators(std::move(enumerators)) {}

bool Enum::isAnonymous() const { return name.empty(); }

std::shared_ptr<TypeDef> Enum::generateTypeDef() {
    assert(!isAnonymous());
    return std::make_shared<TypeDef>("enum_" + name, shared_from_this());
}

std::string Enum::str() const {
    return type->str();
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &s, const Enum &e) {
    for (auto enumerator : e.enumerators) {
        std::string enumeratorName;
        if (!e.name.empty()) {
            enumeratorName = "enum_" + e.name + "_" + enumerator.getName();
        } else {
            enumeratorName = "enum_" + enumerator.getName();
        }
        s << "  final val " << enumeratorName;
        std::string type;
        if (e.isAnonymous()) {
            type = e.type->str();
        } else {
            type = "enum_" + e.name;
        }
        s << ": " << type << " = " << std::to_string(enumerator.getValue());

        if (e.type == PrimitiveType::LONG) {
            s << "L";
        } else if (e.type == PrimitiveType::UNSIGNED_INT) {
            s << ".toUInt";
        } else if (e.type == PrimitiveType::UNSIGNED_LONG) {
            s << "L.toULong";
        }
        s << "\n";
    }
    return s;
}

std::string Enum::getName() const { return name; }
