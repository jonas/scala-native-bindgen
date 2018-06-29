#pragma once

#include "ir/IR.h"
#include <clang/Tooling/Tooling.h>

class TypeTranslator {
  public:
    TypeTranslator(clang::ASTContext *ctx, IR &ir);

    /**
     * @brief Translate the qualified type from c to a scala type
     * @param tpe The type to translate
     * @param avoid A type to avoid, useful to avoid cyclic definitions inside
     * structs, unions, ...
     * @return the type translated
     */
    std::shared_ptr<Type> translate(const clang::QualType &tpe,
                                    const std::string * = nullptr);

    void addAlias(std::string cName, std::shared_ptr<Type> type);

    std::shared_ptr<Type> getTypeFromTypeMap(std::string cType);

  private:
    clang::ASTContext *ctx;
    IR &ir;

    /**
     * Primitive types
     */
    std::map<std::string, std::shared_ptr<Type>> typeMap;

    /**
     * Maps C struct, union or enum name to Type alias
     */
    std::map<std::string, std::shared_ptr<Type>> aliasesMap;

    std::shared_ptr<Type>
    translateStructOrUnionOrEnum(const clang::QualType &qtpe);

    std::shared_ptr<Type> translateStructOrUnion(const clang::QualType &qtpe);

    std::shared_ptr<Type> translateFunctionPointer(const clang::QualType &qtpe,
                                                   const std::string *avoid);

    std::shared_ptr<Type> translatePointer(const clang::QualType &pointee,
                                           const std::string *avoid);

    std::shared_ptr<Type>
    translateConstantArray(const clang::ConstantArrayType *ar,
                           const std::string *avoid);
};
