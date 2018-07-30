#include "Struct.h"
#include "../Utils.h"
#include "types/ArrayType.h"
#include "types/PointerType.h"
#include "types/PrimitiveType.h"
#include <sstream>

Field::Field(std::string name, std::shared_ptr<const Type> type)
    : TypeAndName(std::move(name), std::move(type)) {}

Field::Field(std::string name, std::shared_ptr<const Type> type,
             uint64_t offsetInBits)
    : TypeAndName(std::move(name), std::move(type)),
      offsetInBits(offsetInBits) {}

uint64_t Field::getOffsetInBits() const { return offsetInBits; }

StructOrUnion::StructOrUnion(std::string name,
                             std::vector<std::shared_ptr<Field>> fields,
                             std::shared_ptr<Location> location)
    : LocatableType(std::move(location)), name(std::move(name)),
      fields(std::move(fields)) {}

std::string StructOrUnion::getName() const { return name; }

bool StructOrUnion::hasHelperMethods() const { return !fields.empty(); }

Struct::Struct(std::string name, std::vector<std::shared_ptr<Field>> fields,
               uint64_t typeSize, std::shared_ptr<Location> location,
               bool isPacked, bool isBitField)
    : StructOrUnion(std::move(name), std::move(fields), std::move(location)),
      typeSize(typeSize), isPacked(isPacked), hasBitField(isBitField) {}

std::shared_ptr<TypeDef> Struct::generateTypeDef() {
    if (isRepresentedAsStruct()) {
        return std::make_shared<TypeDef>(getTypeAlias(), shared_from_this(),
                                         nullptr);
    } else {
        // There is no easy way to represent it as a struct in scala native,
        // have to represent it as an array and then Add helpers to help with
        // its manipulation
        return std::make_shared<TypeDef>(
            getTypeAlias(),
            std::make_shared<ArrayType>(std::make_shared<PrimitiveType>("Byte"),
                                        typeSize),
            location);
    }
}

std::string Struct::generateHelperClass() const {
    assert(hasHelperMethods());
    std::stringstream s;
    std::string type = getTypeAlias();
    s << "  implicit class " << type << "_ops(val p: native.Ptr[" << type
      << "])"
      << " extends AnyVal {\n";
    if (isRepresentedAsStruct()) {
        s << generateHelperClassMethodsForStructRepresentation();
    } else {
        s << generateHelperClassMethodsForArrayRepresentation();
    }
    s << "  }\n\n";

    /* makes struct instantiation easier */
    s << "  def "
      << type + "()(implicit z: native.Zone): native.Ptr[" + type + "]"
      << " = native.alloc[" + type + "]\n";

    return s.str();
}

bool Struct::hasHelperMethods() const {
    if (hasBitField) {
        return false;
    }
    if (!isRepresentedAsStruct()) {
        return !fields.empty();
    }
    return !isPacked && !fields.empty();
}

std::string Struct::generateHelperClassMethodsForStructRepresentation() const {
    std::stringstream s;
    for (unsigned fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++) {
        if (!fields[fieldIndex]->getName().empty()) {
            s << generateGetterForStructRepresentation(fieldIndex);
            s << generateSetterForStructRepresentation(fieldIndex);
        }
    }
    return s.str();
}

std::string Struct::generateHelperClassMethodsForArrayRepresentation() const {
    std::stringstream s;
    for (unsigned fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++) {
        if (!fields[fieldIndex]->getName().empty()) {
            s << generateGetterForArrayRepresentation(fieldIndex);
            s << generateSetterForArrayRepresentation(fieldIndex);
        }
    }
    return s.str();
}

std::string Struct::getTypeAlias() const { return "struct_" + name; }

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

bool Struct::usesType(const std::shared_ptr<const Type> &type,
                      bool stopOnTypeDefs) const {
    for (const auto &field : fields) {
        if (*field->getType() == *type ||
            field->getType()->usesType(type, stopOnTypeDefs)) {
            return true;
        }
    }
    return false;
}

bool Struct::operator==(const Type &other) const {
    if (this == &other) {
        return true;
    }
    auto *s = dynamic_cast<const Struct *>(&other);
    if (s) {
        /* structs have unique names */
        return name == s->name;
    }
    return false;
}

std::string
Struct::generateSetterForStructRepresentation(unsigned fieldIndex) const {
    std::shared_ptr<Field> field = fields[fieldIndex];
    std::string setter = handleReservedWords(field->getName(), "_=");
    std::string parameterType = field->getType()->str();
    std::string value = "value";
    if (isAliasForType<ArrayType>(field->getType().get()) ||
        isAliasForType<Struct>(field->getType().get())) {
        parameterType = "native.Ptr[" + parameterType + "]";
        value = "!" + value;
    }
    std::stringstream s;
    s << "    def " << setter << "(value: " + parameterType + "): Unit = !p._"
      << std::to_string(fieldIndex + 1) << " = " << value << "\n";
    return s.str();
}

std::string
Struct::generateGetterForStructRepresentation(unsigned fieldIndex) const {
    std::shared_ptr<Field> field = fields[fieldIndex];
    std::string getter = handleReservedWords(field->getName());
    std::string returnType = field->getType()->str();
    std::string methodBody;
    if (isAliasForType<ArrayType>(field->getType().get()) ||
        isAliasForType<Struct>(field->getType().get())) {
        returnType = "native.Ptr[" + returnType + "]";
        methodBody = "p._" + std::to_string(fieldIndex + 1);
    } else {
        methodBody = "!p._" + std::to_string(fieldIndex + 1);
    }
    std::stringstream s;
    s << "    def " << getter << ": " << returnType << " = " << methodBody
      << "\n";
    return s.str();
}

bool Struct::isRepresentedAsStruct() const {
    return fields.size() <= SCALA_NATIVE_MAX_STRUCT_FIELDS && !hasBitField;
}

std::string
Struct::generateSetterForArrayRepresentation(unsigned int fieldIndex) const {
    std::shared_ptr<Field> field = fields[fieldIndex];
    std::string setter = handleReservedWords(field->getName(), "_=");
    std::string parameterType;
    std::string value = "value";
    std::string castedField = "p._1";

    PointerType pointerToFieldType = PointerType(field->getType());
    uint64_t offsetInBytes = field->getOffsetInBits() / 8;
    if (offsetInBytes > 0) {
        castedField =
            "(" + castedField + " + " + std::to_string(offsetInBytes) + ")";
    }
    castedField = "!" + castedField + ".cast[" + pointerToFieldType.str() + "]";
    if (isAliasForType<ArrayType>(field->getType().get()) ||
        isAliasForType<Struct>(field->getType().get())) {
        parameterType = pointerToFieldType.str();
        value = "!" + value;
    } else {
        parameterType = field->getType()->str();
    }
    std::stringstream s;
    s << "    def " << setter
      << "(value: " + parameterType + "): Unit = " << castedField << " = "
      << value << "\n";
    return s.str();
}

std::string
Struct::generateGetterForArrayRepresentation(unsigned fieldIndex) const {
    std::shared_ptr<Field> field = fields[fieldIndex];
    std::string getter = handleReservedWords(field->getName());
    std::string returnType;
    std::string methodBody;

    PointerType pointerToFieldType = PointerType(field->getType());
    uint64_t offsetInBytes = field->getOffsetInBits() / 8;
    if (offsetInBytes > 0) {
        methodBody = "(p._1 + " + std::to_string(offsetInBytes) + ")";
    } else {
        methodBody = "p._1";
    }
    methodBody = methodBody + ".cast[" + pointerToFieldType.str() + "]";

    if (isAliasForType<ArrayType>(field->getType().get()) ||
        isAliasForType<Struct>(field->getType().get())) {
        returnType = pointerToFieldType.str();
    } else {
        methodBody = "!" + methodBody;
        returnType = field->getType()->str();
    }
    std::stringstream s;
    s << "    def " << getter << ": " << returnType << " = " << methodBody
      << "\n";
    return s.str();
}

Union::Union(std::string name, std::vector<std::shared_ptr<Field>> fields,
             uint64_t maxSize, std::shared_ptr<Location> location)
    : StructOrUnion(std::move(name), std::move(fields), std::move(location)),
      ArrayType(std::make_shared<PrimitiveType>("Byte"), maxSize) {}

std::shared_ptr<TypeDef> Union::generateTypeDef() {
    return std::make_shared<TypeDef>(getTypeAlias(), shared_from_this(),
                                     nullptr);
}

std::string Union::generateHelperClass() const {
    assert(hasHelperMethods());
    std::stringstream s;
    std::string type = getTypeAlias();
    s << "  implicit class " << type << "_pos"
      << "(val p: native.Ptr[" << type << "]) extends AnyVal {\n";
    for (const auto &field : fields) {
        if (!field->getName().empty()) {
            s << generateGetter(field);
            s << generateSetter(field);
        }
    }
    s << "  }\n";
    return s.str();
}

std::string Union::getTypeAlias() const { return "union_" + name; }

bool Union::operator==(const Type &other) const {
    if (this == &other) {
        return true;
    }
    auto *u = dynamic_cast<const Union *>(&other);
    if (u) {
        /* unions have unique names */
        return name == u->name;
    }
    return false;
}

std::string Union::generateGetter(const std::shared_ptr<Field> &field) const {
    std::string getter = handleReservedWords(field->getName());
    std::string ftype = field->getType()->str();
    return "    def " + getter + ": native.Ptr[" + ftype +
           "] = p.cast[native.Ptr[" + ftype + "]]\n";
}

std::string Union::generateSetter(const std::shared_ptr<Field> &field) const {
    std::string setter = handleReservedWords(field->getName(), "_=");
    std::string ftype = field->getType()->str();
    if (isAliasForType<ArrayType>(field->getType().get()) ||
        isAliasForType<Struct>(field->getType().get())) {
        return "    def " + setter + "(value: native.Ptr[" + ftype +
               "]): Unit = !p.cast[native.Ptr[" + ftype + "]] = !value\n";
    }
    return "    def " + setter + "(value: " + ftype +
           "): Unit = !p.cast[native.Ptr[" + ftype + "]] = value\n";
}
