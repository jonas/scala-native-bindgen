#ifndef SCALA_NATIVE_BINDGEN_INTERMEDIATEREPRESENTATION_H
#define SCALA_NATIVE_BINDGEN_INTERMEDIATEREPRESENTATION_H

#include "Enum.h"
#include "Function.h"
#include "LiteralDefine.h"
#include "PossibleVarDefine.h"
#include "Struct.h"
#include "TypeDef.h"
#include "VarDefine.h"

/**
 * Intermediate representation
 */
class IR {
  public:
    IR(std::string libName, std::string linkName, std::string objectName,
       std::string packageName);

    ~IR();

    void addFunction(std::string name, std::vector<Parameter *> parameters,
                     std::shared_ptr<Type> retType, bool isVariadic);

    void addTypeDef(std::string name, std::shared_ptr<Type> type);

    /**
     * @return type alias for the enum
     */
    std::shared_ptr<Type> addEnum(std::string name, std::shared_ptr<Type> type,
                                  std::vector<Enumerator> enumerators);

    /**
     * @return type alias for the struct
     */
    std::shared_ptr<Type>
    addStruct(std::string name, std::vector<Field *> fields, uint64_t typeSize);

    /**
     * @return type alias for the union
     */
    std::shared_ptr<Type>
    addUnion(std::string name, std::vector<Field *> fields, uint64_t maxSize);

    void addLiteralDefine(std::string name, std::string literal,
                          std::shared_ptr<Type> type);

    void addPossibleVarDefine(const std::string &macroName,
                              const std::string &varName);

    void addVarDefine(std::string name, std::shared_ptr<Variable> variable);

    std::shared_ptr<Variable> addVariable(const std::string &name,
                                          std::shared_ptr<Type> type);

    /**
     * @return true if there are no functions, types,
     *         structs and unions
     */
    bool libObjEmpty() const;

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &s, const IR &ir);

    void generate(const std::string &excludePrefix);

    void removeDefine(const std::string &name);

    /**
     * @return macro name if there is a macro for the variable.
     *         otherwise return empty string
     */
    std::string getDefineForVar(const std::string &varName) const;

    std::shared_ptr<TypeDef> getTypeDefWithName(const std::string &name);

  private:
    /**
     * @return true if helper methods will be generated for this library
     */
    bool hasHelperMethods() const;

    /**
     * Remove functions that start with given prefix.
     *
     * Replace typedef by underlying type if it starts with given prefix
     * and it is used only in other typedefs.
     *
     * Example:
     * @code
     * type __int32_t = native.CInt
     * type __darwin_pid_t = __int32_t
     * type pid_t = __darwin_pid_t
     * @endcode
     *
     * Becomes:
     * @code
     * type pid_t = native.CInt
     * @endcode
     *
     */
    void filterDeclarations(const std::string &excludePrefix);

    /**
     * Remove all typedefs that start with given prefix.
     */
    void filterTypeDefs(const std::string &excludePrefix);

    /**
     * Find all typedefs that use oldType and replace it with newType.
     */
    void replaceTypeInTypeDefs(std::shared_ptr<Type> oldType,
                               std::shared_ptr<Type> newType);

    /**
     * @return true if given type is used only in typedefs.
     */
    bool typeIsUsedOnlyInTypeDefs(std::shared_ptr<Type> type);

    /**
     * @return true if type is used in one of given declarations.
     */
    template <typename T>
    bool isTypeUsed(const std::vector<T> &declarations,
                    std::shared_ptr<Type> type);

    void setScalaNames();

    bool existsFunctionWithName(std::string functionName);

    template <typename T>
    void filterByPrefix(std::vector<T> &declarations,
                        const std::string &excludePrefix);

    template <typename T>
    void filterByName(std::vector<T> &declarations, const std::string &name);

    template <typename T>
    T getDeclarationWithName(std::vector<T> &declarations,
                             const std::string &name);

    template <typename T> void clearVector(std::vector<T> v);

    std::string libName;    // name of the library
    std::string linkName;   // name of the library to link with
    std::string objectName; // name of Scala object
    std::vector<std::shared_ptr<Function>> functions;
    std::vector<std::shared_ptr<TypeDef>> typeDefs;
    std::vector<std::shared_ptr<Struct>> structs;
    std::vector<std::shared_ptr<Union>> unions;
    std::vector<std::shared_ptr<Enum>> enums;
    std::vector<std::shared_ptr<LiteralDefine>> literalDefines;
    std::vector<std::shared_ptr<PossibleVarDefine>> possibleVarDefines;
    std::vector<std::shared_ptr<VarDefine>> varDefines;
    std::vector<std::shared_ptr<Variable>> variables;
    bool generated = false; // generate type defs only once
    std::string packageName;
};

#endif // SCALA_NATIVE_BINDGEN_INTERMEDIATEREPRESENTATION_H
