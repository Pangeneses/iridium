#include "Ir77PVLifetime.hpp"

#include "../dictionary/IDIr77PVContext.hpp"

namespace NSIr77PeregrineV {

std::shared_ptr<IIr77Return const> Ir77PVLifetime::Initialize(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> Ir77PVLifetime::BuildContext() {

}

}  // namespace NSIr77PeregrineV