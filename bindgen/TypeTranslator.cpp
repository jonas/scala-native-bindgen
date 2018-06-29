#include "TypeTranslator.h"
#include "Utils.h"
#include "ir/types/FunctionPointerType.h"
#include "ir/types/PointerType.h"

TypeTranslator::TypeTranslator(clang::ASTContext *ctx_, IR &ir)
    : ctx(ctx_), ir(ir), typeMap() {
    // Native Types
    typeMap["void"] = PrimitiveType::of("Unit");
    typeMap["bool"] = PrimitiveType::of("native.CBool");
    typeMap["_Bool"] = PrimitiveType::of("native.CBool");
    typeMap["char"] = PrimitiveType::of("native.CChar");
    typeMap["signed char"] = PrimitiveType::of("native.CSignedChar");
    typeMap["unsigned char"] = PrimitiveType::of("native.CUnsignedChar");
    typeMap["short"] = PrimitiveType::of("native.CShort");
    typeMap["unsigned short"] = PrimitiveType::of("native.CUnsignedShort");
    typeMap["int"] = PrimitiveType::of("native.CInt");
    typeMap["long int"] = PrimitiveType::of("native.CLongInt");
    typeMap["unsigned int"] = PrimitiveType::UNSIGNED_INT;
    typeMap["unsigned long int"] = PrimitiveType::of("native.CUnsignedLongInt");
    typeMap["long"] = PrimitiveType::LONG;
    typeMap["unsigned long"] = PrimitiveType::UNSIGNED_LONG;
    typeMap["long long"] = PrimitiveType::of("native.CLongLong");
    typeMap["unsigned long long"] = PrimitiveType::of("native.CUnsignedLongLong");
    typeMap["size_t"] = PrimitiveType::of("native.CSize");
    typeMap["ptrdiff_t"] = PrimitiveType::of("native.CPtrDiff");
    typeMap["wchar_t"] = PrimitiveType::of("native.CWideChar");
    typeMap["char16_t"] = PrimitiveType::of("native.CChar16");
    typeMap["char32_t"] = PrimitiveType::of("native.CChar32");
    typeMap["float"] = PrimitiveType::of("native.CFloat");
    typeMap["double"] = PrimitiveType::of("native.CDouble");
}

std::shared_ptr<Type>
TypeTranslator::translateFunctionPointer(const clang::QualType &qtpe,
                                         const std::string *avoid) {
    const auto *ptr = qtpe.getTypePtr()->getAs<clang::PointerType>();
    const clang::QualType &inner = ptr->getPointeeType();

    if (inner->isFunctionProtoType()) {
        const auto *fc = inner->getAs<clang::FunctionProtoType>();
        std::shared_ptr<Type> returnType =
            translate(fc->getReturnType(), avoid);
        std::vector<std::shared_ptr<Type>> parametersTypes;

        for (const clang::QualType &param : fc->param_types()) {
            parametersTypes.push_back(translate(param, avoid));
        }

        return std::make_shared<FunctionPointerType>(
            returnType, parametersTypes, fc->isVariadic());

    } else {
        llvm::errs() << "Unsupported function pointer type: "
                     << qtpe.getAsString() << "\n";
        llvm::errs().flush();
        exit(-1);
    }
}

std::shared_ptr<Type>
TypeTranslator::translatePointer(const clang::QualType &pte,
                                 const std::string *avoid) {

    if (pte->isBuiltinType()) {
        const clang::BuiltinType *as = pte->getAs<clang::BuiltinType>();

        // Take care of void*
        if (as->getKind() == clang::BuiltinType::Void) {
            return std::make_shared<PointerType>(PrimitiveType::BYTE);
        }

        // Take care of char*
        if (as->getKind() == clang::BuiltinType::Char_S ||
            as->getKind() == clang::BuiltinType::SChar) {
            // TODO: new PointerType(new PrimitiveType("native.CChar"))
            return PrimitiveType::of("native.CString");
        }
    }

    return std::make_shared<PointerType>(translate(pte, avoid));
}

std::shared_ptr<Type>
TypeTranslator::translateStructOrUnionOrEnum(const clang::QualType &qtpe) {
    std::string name = qtpe.getUnqualifiedType().getAsString();

    auto it = aliasesMap.find(name);
    if (it != aliasesMap.end()) {
        /* name contains space: struct <name>.
         * Use type alias instead struct type */
        return (*it).second;
    }
    /* type has typedef alias */
    return ir.getTypeDefWithName(name);
}

std::shared_ptr<Type>
TypeTranslator::translateStructOrUnion(const clang::QualType &qtpe) {
    if (qtpe->hasUnnamedOrLocalType()) {
        // TODO: Verify that the local part is not a problem
        uint64_t sizeInBits = ctx->getTypeSize(qtpe);
        assert(sizeInBits % 8 == 0);
        return ArrayType::of(PrimitiveType::BYTE, sizeInBits / 8);
    }

    return translateStructOrUnionOrEnum(qtpe);
}

std::shared_ptr<Type>
TypeTranslator::translateConstantArray(const clang::ConstantArrayType *ar,
                                       const std::string *avoid) {
    const uint64_t size = ar->getSize().getZExtValue();
    std::shared_ptr<Type> elementType = translate(ar->getElementType(), avoid);
    if (elementType == nullptr) {
        llvm::errs() << "Failed to translate array type "
                     << ar->getElementType().getAsString() << "\n";
        elementType = PrimitiveType::BYTE;
    }

    return ArrayType::of(elementType, size);
}

std::shared_ptr<Type> TypeTranslator::translate(const clang::QualType &qtpe,
                                                const std::string *avoid) {

    const clang::Type *tpe = qtpe.getTypePtr();

    if (typeEquals(tpe, avoid)) {
        // This is a type that we want to avoid the usage.
        // Êxample: A struct that has a pointer to itself
        uint64_t sizeInBits = ctx->getTypeSize(tpe);
        return ArrayType::of(PrimitiveType::BYTE, sizeInBits / 8);
    }

    if (tpe->isFunctionPointerType()) {
        return translateFunctionPointer(qtpe, avoid);

    } else if (tpe->isPointerType()) {
        return translatePointer(
            tpe->getAs<clang::PointerType>()->getPointeeType(), avoid);

    } else if (qtpe->isStructureType()) {
        return translateStructOrUnion(qtpe);

    } else if (qtpe->isUnionType()) {
        return translateStructOrUnion(qtpe);

    } else if (qtpe->isEnumeralType()) {
        return translateStructOrUnionOrEnum(qtpe);

    } else if (qtpe->isConstantArrayType()) {
        return translateConstantArray(ctx->getAsConstantArrayType(qtpe), avoid);
    } else if (qtpe->isArrayType()) {
        return translatePointer(ctx->getAsArrayType(qtpe)->getElementType(),
                                avoid);
    } else {

        auto found = typeMap.find(qtpe.getUnqualifiedType().getAsString());
        if (found != typeMap.end()) {
            return found->second;
        } else {
            return ir.getTypeDefWithName(
                qtpe.getUnqualifiedType().getAsString());
        }
    }
}

void TypeTranslator::addAlias(std::string cName, std::shared_ptr<Type> type) {
    aliasesMap[cName] = type;
}

std::shared_ptr<Type> TypeTranslator::getTypeFromTypeMap(std::string cType) {
    auto it = typeMap.find(cType);
    if (it != typeMap.end()) {
        return (*it).second;
    }
    return nullptr;
}
