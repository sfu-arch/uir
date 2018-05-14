#ifndef DANDELION_EDGE_H
#define DANDELION_EDGE_H
#include <stdint.h>
#include <list>

#include "llvm/IR/Argument.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "Dandelion/Node.h"

using namespace llvm;

namespace dandelion {

class Edge {
   public:
    enum EdgeType {
        DataTypeEdge = 0,
        ControlTypeEdge,
        DataToControlTypeEdge,
        ControlToDataTypeEdge,
        MaskTypeEdge,
        MemoryReadTypeEdge,
        MemoryWriteTypeEdge,
        MemoryToDataTypeEdge,
        MemoryToControlTypeEdge,
        UknownType
    };

   private:
    EdgeType edge_type;
    Node *src;
    Node *tar;

   public:
    explicit Edge(EdgeType _ty) : edge_type(_ty) {}
    explicit Edge(Node *_src = nullptr, Node *_tar = nullptr) : src(_src), tar(_tar) {}
    explicit Edge(EdgeType _ty = UknownType, Node *_src = nullptr, Node *_tar = nullptr)
        : edge_type(_ty), src(_src), tar(_tar) {}

    uint32_t getType() const { return edge_type; }

    Node *getSrc() { return src; }
    Node *getTar() { return tar; }
};
}

#endif  // end of DANDDELION_EDGE_H
