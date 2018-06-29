#include "Struct.h"
#include "../Utils.h"
#include "types/ArrayType.h"
#include "types/PrimitiveType.h"
#include <sstream>
#include <utility>

Field::Field(std::string name, std::shared_ptr<Type> type)
    : TypeAndName(std::move(name), type) {}

StructOrUnion::StructOrUnion(std::string name, std::vector<Field *> fields)
    : name(std::move(name)), fields(std::move(fields)) {}

std::string StructOrUnion::getName() const { return name; }

StructOrUnion::~StructOrUnion() {
    for (const auto &field : fields) {
        delete field;
    }
}

Struct::Struct(std::string name, std::vector<Field *> fields, uint64_t typeSize)
    : StructOrUnion(std::move(name), std::move(fields)), typeSize(typeSize) {}

std::shared_ptr<TypeDef> Struct::generateTypeDef() {
    if (fields.size() < SCALA_NATIVE_MAX_STRUCT_FIELDS) {
        return std::make_shared<TypeDef>(getAliasType(), shared_from_this());
    } else {
        // There is no easy way to represent it as a struct in scala native,
        // have to represent it as an array and then Add helpers to help with
        // its manipulation
        return std::make_shared<TypeDef>(
            getAliasType(),
            ArrayType::of(PrimitiveType::BYTE, typeSize));
    }
}

std::string Struct::generateHelperClass() const {
    if (!hasHelperMethods()) {
        /* struct is empty or represented as an array */
        return "";
    }
    std::stringstream s;
    std::string type = getAliasType();
    s << "  implicit class " << type << "_ops(val p: native.Ptr[" << type
      << "])"
      << " extends AnyVal {\n";
    int fieldIndex = 0;
    for (const auto &field : fields) {
        if (!field->getName().empty()) {
            std::string getter = handleReservedWords(field->getName());
            std::string setter = handleReservedWords(field->getName(), "_=");
            std::shared_ptr<Type> ftype = field->getType();
            s << "    def " << getter << ": " << ftype->str() << " = !p._"
              << std::to_string(fieldIndex + 1) << "\n"
              << "    def " << setter
              << "(value: " + ftype->str() + "):Unit = !p._"
              << std::to_string(fieldIndex + 1) << " = value\n";
        }
        fieldIndex++;
    }
    s << "  }\n\n";

    /* makes struct instantiation easier */
    s << "  def "
      << type + "()(implicit z: native.Zone): native.Ptr[" + type + "]"
      << " = native.alloc[" + type + "]\n";

    return s.str();
}

bool Struct::hasHelperMethods() const {
    return !fields.empty() && fields.size() < SCALA_NATIVE_MAX_STRUCT_FIELDS;
}

std::string Struct::getAliasType() const { return "struct_" + name; }

std::string Struct::str() const {
    std::stringstream ss;
    ss << "native.CStruct" << std::to_string(fields.size()) << "[";

    std::string sep = "";
    for (const auto &field : fields) {
        ss << sep << field->getType()->str();
        sep = ", ";
    }

    ss << "]";
    return ss.str();
}

bool Struct::usesType(std::shared_ptr<Type> type) const {
    if (this == type.get()) {
        return true;
    }
    for (const auto &field : fields) {
        if (field->getType() == type) {
            return true;
        }
    }
    return false;
}

Union::Union(std::string name, std::vector<Field *> fields, uint64_t maxSize)
    : StructOrUnion(std::move(name), std::move(fields)),
      ArrayType(PrimitiveType::BYTE, maxSize) {}

std::shared_ptr<TypeDef> Union::generateTypeDef() {
    return std::make_shared<TypeDef>(getTypeAlias(), shared_from_this());
}

std::string Union::generateHelperClass() const {
    std::stringstream s;
    std::string type = getTypeAlias();
    s << "  implicit class " << type << "_pos"
      << "(val p: native.Ptr[" << type << "]) extends AnyVal {\n";
    for (const auto &field : fields) {
        if (!field->getName().empty()) {
            std::string getter = handleReservedWords(field->getName());
            std::string setter = handleReservedWords(field->getName(), "_=");
            std::shared_ptr<Type> ftype = field->getType();
            s << "    def " << getter << ": native.Ptr[" << ftype->str()
              << "] = p.cast[native.Ptr[" << ftype->str() << "]]\n";

            s << "    def " << setter << "(value: " << ftype->str()
              << "): Unit = !p.cast[native.Ptr[" << ftype->str()
              << "]] = value\n";
        }
    }
    s << "  }\n";
    return s.str();
}

std::string Union::getTypeAlias() const { return "union_" + name; }
