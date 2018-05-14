#ifndef DANDELION_NODE_H
#define DANDELION_NODE_H
#include <stdint.h>
#include <list>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "iterator_range.h"

#define XLEN 32

using namespace llvm;

namespace dandelion {

class Node;
class SuperNode;
class InstructionNode;
class LoopNode;
class MemoryNode;
class PhiSelectNode;

enum PrintType { Scala = 0, Dot, Json };

// enum MemoryMode { Cache = 0, Reg };

struct DataPort {
    std::list<Node *> data_input_port;
    std::list<Node *> data_output_port;
};

struct ControlPort {
    std::list<Node *> control_input_port;
    std::list<Node *> control_output_port;
};

struct DependencyPort {
    std::list<Node *> depen_input_port;
    std::list<Node *> depen_output_port;
};

struct MemoryPort {
    std::list<Node *> memory_req_port;
    std::list<Node *> memory_resp_port;
};

struct NodeInfo {
    uint32_t ID;
    std::string Name;

    NodeInfo(uint32_t _id, std::string _n) : ID(_id), Name(_n){};
};

class Node {
   public:
    enum NodeType {
        SuperNodeTy = 0,
        LoopNodeTy,
        InstructionNodeTy,
        FunctionArgTy,
        GlobalValueTy,
        ConstIntTy,
        MemoryUnitTy,
        SplitCallTy,
        UnkonwTy

    };

   private:
    // Type of the Node
    NodeType node_type;
    // Node information
    NodeInfo info;

    // List of data ports
    DataPort port_data;

    // List of Control ports
    ControlPort port_control;

    // List of Dependency port
    DependencyPort port_depen;

   public:  // Public methods
    Node(NodeType _nt, NodeInfo _ni) : info(_ni), node_type(_nt) {}

    uint32_t returnDataInputPortIndex(Node *);
    uint32_t returnControlInputPortIndex(Node *);
    uint32_t returnMemoryInputPortIndex(Node *);

    uint32_t returnDataOutputPortIndex(Node *);
    uint32_t returnControlOutputPortIndex(Node *);
    uint32_t returnMemoryOutputPortIndex(Node *);

    void addDataInputPort(Node *);
    void addDataOutputPort(Node *);

    void addControlInputPort(Node *);
    void addControlOutputPort(Node *);

    uint32_t numDataInputPort() { return port_data.data_input_port.size(); }
    uint32_t numDataOutputPort() { return port_data.data_output_port.size(); }
    uint32_t numControlInputPort() {
        return port_control.control_input_port.size();
    }
    uint32_t numControlOutputPort() {
        return port_control.control_output_port.size();
    }

    std::list<Node *>::const_iterator findDataInputNode(Node *);
    std::list<Node *>::const_iterator findDataOutputNode(Node *);
    std::list<Node *>::const_iterator findControlInputNode(Node *);
    std::list<Node *>::const_iterator findControlOutputNode(Node *);

    void removeNodeDataInputNode(Node *);
    void removeNodeDataOutputNode(Node *);
    void removeNodeControlInputNode(Node *);
    void removeNodeControlOutputNode(Node *);

    // Iterator over input data edges
    auto inputDataport_begin() {
        return this->port_data.data_input_port.cbegin();
    }
    auto inputDataport_end() { return this->port_data.data_input_port.cend(); }

    auto input_data_range() {
        return helpers::make_range(inputDataport_begin(), inputDataport_end());
    }

    // Iterator over output data edges
    auto outputDataport_begin() {
        return this->port_data.data_output_port.cbegin();
    }
    auto outputDataport_end() {
        return this->port_data.data_output_port.cend();
    }
    auto output_data_range() {
        return helpers::make_range(outputDataport_begin(),
                                   outputDataport_end());
    }

    // Iterator over input control edges
    auto inputControl_begin() {
        return this->port_control.control_input_port.cbegin();
    }
    auto inputControl_end() {
        return this->port_control.control_input_port.cend();
    }
    auto input_control_range() {
        return helpers::make_range(inputControl_begin(), inputControl_end());
    }

    // Iterator over output control edges
    auto outputControl_begin() {
        return this->port_control.control_output_port.cbegin();
    }
    auto outputControl_end() {
        return this->port_control.control_output_port.cend();
    }
    auto output_control_range() {
        return helpers::make_range(outputControl_begin(), outputControl_end());
    }

    uint32_t getID() { return info.ID; }
    std::string getName() { return info.Name; }

    // TODO how to define virtual functions?
    // virtual void printInitilization() {}

    uint32_t getType() const { return node_type; }

    virtual std::string printDefinition(PrintType) {
        return this->info.Name + std::string("Not defined!");
    }
    virtual std::string printInputEnable(PrintType, uint32_t) {
        return std::string("Not defined!");
    }
    virtual std::string printInputEnable(PrintType) {
        return std::string("Not defined!");
    }
    virtual std::string printOutputEnable(PrintType) {
        return this->info.Name + std::string(" Not defined!");
    }
    virtual std::string printOutputEnable(PrintType, uint32_t) {
        return std::string("Not defined!");
    }
    virtual std::string printInputData(PrintType) {
        return std::string("Not defined!");
    }
    virtual std::string printInputData(PrintType, uint32_t) {
        return std::string("Not defined!");
    }
    virtual std::string printOutputData(PrintType) {
        return std::string("Not defined!");
    }
    virtual std::string printOutputData(PrintType, uint32_t) {
        return std::string("Not defined!");
    }
};

/**
 * Super node is actual implimetation of our basic blocks
 */
class SuperNode : public Node {
   public:
    // List of the instructions
    using PhiNodeList = std::list<PhiSelectNode *>;
    enum SuperNodeType { Mask, NoMask, LoopHead };

   private:
    Node *activate_input;

    llvm::BasicBlock *basic_block;

    std::list<InstructionNode *> instruction_list;
    PhiNodeList phi_list;

    SuperNodeType type;

   public:
    explicit SuperNode(NodeInfo _nf, llvm::BasicBlock *_bb = nullptr)
        : Node(Node::SuperNodeTy, _nf),
          activate_input(nullptr),
          basic_block(_bb),
          type(SuperNodeType::NoMask) {}

    // Define classof function so that we can use dyn_cast function
    static bool classof(const Node *T) {
        return T->getType() == Node::SuperNodeTy;
    }

    llvm::BasicBlock *getBasicBlock();
    void addInstruction(InstructionNode *);
    void addPhiInstruction(PhiSelectNode *);

    bool hasPhi() { return !phi_list.empty(); }
    uint32_t getNumPhi() const { return phi_list.size(); }

    auto phi_begin() { return this->phi_list.cbegin(); }
    auto phi_end() { return this->phi_list.cend(); }
    auto phis() { return helpers::make_range(phi_begin(), phi_end()); }

    auto ins_begin() const { return this->instruction_list.begin(); }
    auto ins_end() const { return this->instruction_list.end(); }
    auto instructions() { return helpers::make_range(ins_begin(), ins_end()); }

    const SuperNodeType getNodeType() { return type; }
    void setNodeType(SuperNodeType _t) { this->type = _t; }
    void setActivateInput(Node *_n) { this->activate_input = _n; }
    auto getActivateNode(){ return this->activate_input; }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType, uint32_t) override;
    virtual std::string printOutputEnable(PrintType, uint32_t) override;
    virtual std::string printMaskOutput(PrintType, uint32_t);
    std::string printActivateEnable(PrintType);
};

/**
 * Memory unit works as a local memory for each graph
 */
class MemoryNode : public Node {
   private:
    MemoryPort read_port_data;
    MemoryPort write_port_data;

   public:
    explicit MemoryNode(NodeInfo _nf) : Node(Node::MemoryUnitTy, _nf) {}

    // Restrict access to data input ports
    void addDataInputPort(Node *) = delete;
    void addDataOutputPort(Node *) = delete;
    uint32_t numDataInputPort() = delete;
    uint32_t numDataOutputPort() = delete;

    // Define classof function so that we can use dyn_cast function
    static bool classof(const Node *T) {
        return T->getType() == Node::MemoryUnitTy;
    }

    virtual std::string printDefinition(PrintType);

    void addReadMemoryReqPort(Node *);
    void addReadMemoryRespPort(Node *);
    void addWriteMemoryReqPort(Node *);
    void addWriteMemoryRespPort(Node *);
    uint32_t numReadDataInputPort() {
        return read_port_data.memory_req_port.size();
    }
    uint32_t numReadDataOutputPort() {
        return read_port_data.memory_resp_port.size();
    }
    uint32_t numWriteDataInputPort() {
        return write_port_data.memory_req_port.size();
    }
    uint32_t numWriteDataOutputPort() {
        return write_port_data.memory_resp_port.size();
    }
};

/**
 * LoopNode contains all the instructions and useful information about the loops
 */
class LoopNode : public Node {
   private:
    std::list<InstructionNode *> instruction_list;
    std::list<SuperNode *> basic_block_list;
    SuperNode *head_node;
    SuperNode *latch_node;

   public:
    explicit LoopNode(NodeInfo _nf, SuperNode *_hnode = nullptr,
                      SuperNode *_lnode = nullptr)
        : Node(Node::LoopNodeTy, _nf), head_node(_hnode), latch_node(_lnode) {}

    // Define classof function so that we can use dyn_cast function
    static bool classof(const Node *T) {
        return T->getType() == Node::LoopNodeTy;
    }

    // Iterator over instucrion list
    auto ins_begin() { return instruction_list.cbegin(); }
    auto ins_end() { return instruction_list.cend(); }
    auto instructions() { return helpers::make_range(ins_begin(), ins_end()); }

    // Iterator over basic block list
    auto bb_begin() { return basic_block_list.cbegin(); }
    auto bb_end() { return basic_block_list.cend(); }
    auto bblocks() { return helpers::make_range(bb_begin(), bb_end()); }

    // Iterator over input edges

    void setHeadNode(SuperNode *_n) { head_node = _n; }
    void setLatchNode(SuperNode *_n) { latch_node = _n; }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printOutputEnable(PrintType) override;
    virtual std::string printInputEnable(PrintType, uint32_t) override;
};

/**
 * This class is basic implementation of Instruction nodes
 * It inheretens from Node class and it has a pointer to original bitcode IR
 * the pointer can be NULL that means this is a new insturction type
 */
class InstructionNode : public Node {
   public:
    enum InstType {
        BinaryInstructionTy,
        IcmpInstructionTy,
        BranchInstructionTy,
        PhiInstructionTy,
        AllocaInstructionTy,
        GetElementPtrInstTy,
        LoadInstructionTy,
        StoreInstructionTy,
        SextInstructionTy,
        ZextInstructionTy,
        BitCastInstructionTy,
        TruncInstructionTy,
        SelectInstructionTy,
#ifdef TAPIR
        DetachInstructionTy,
        ReattachInstructionTy,
        SyncInstructionTy,
#endif
        ReturnInstrunctionTy,
        CallInstructionTy
    };

   private:
    InstType ins_type;
    llvm::Instruction *parent_instruction;

   public:
    InstructionNode(NodeInfo _ni, InstType _ins_t,
                    llvm::Instruction *_ins = nullptr)
        : Node(Node::InstructionNodeTy, _ni),
          ins_type(_ins_t),
          parent_instruction(_ins) {}

    llvm::Instruction *getInstruction();

    uint32_t getOpCode() const { return ins_type; }
    const std::string getOpCodeName() {
        return parent_instruction->getOpcodeName();
    }

    bool isBinaryOp() const { return ins_type == BinaryInstructionTy; }

    static bool classof(const Node *T) {
        return T->getType() == Node::InstructionNodeTy;
    }

    virtual std::string printDefinition(PrintType) override {
        return std::string("Not defined instructions\n");
    }
};

class BinaryOperatorNode : public InstructionNode {
   public:
    BinaryOperatorNode(NodeInfo _ni, llvm::BinaryOperator *_ins = nullptr)
        : InstructionNode(_ni, InstructionNode::BinaryInstructionTy, _ins) {}

    // Overloading isa<>, dyn_cast from llvm
    static bool classof(const InstructionNode *I) {
        return I->getOpCode() == InstType::BinaryInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
};

class IcmpNode : public InstructionNode {
   public:
    IcmpNode(NodeInfo _ni, llvm::ICmpInst *_ins = nullptr)
        : InstructionNode(_ni, InstructionNode::IcmpInstructionTy, _ins) {}

    static bool classof(const InstructionNode *I) {
        return I->getOpCode() == InstType::IcmpInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
};

class BranchNode : public InstructionNode {
   public:
    BranchNode(NodeInfo _ni, llvm::BranchInst *_ins = nullptr)
        : InstructionNode(_ni, InstType::BranchInstructionTy, _ins) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::BranchInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printOutputEnable(PrintType, uint32_t) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
};

class PhiSelectNode : public InstructionNode {
   private:
    SuperNode *mask_node;

   public:
    PhiSelectNode(NodeInfo _ni, llvm::PHINode *_ins = nullptr,
                  SuperNode *_parent = nullptr)
        : InstructionNode(_ni, InstType::PhiInstructionTy, _ins) {}

    SuperNode *getMaskNode() const { return mask_node; }

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::PhiInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    void setParentNode(SuperNode *_parent) { this->mask_node = _parent; }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
    virtual std::string printMaskInput(PrintType);
};

class AllocaNode : public InstructionNode {
   public:
    AllocaNode(NodeInfo _ni, llvm::AllocaInst *_ins = nullptr)
        : InstructionNode(_ni, InstructionNode::AllocaInstructionTy, _ins) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::AllocaInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }
};

class GEPNode : public InstructionNode {
   public:
    GEPNode(NodeInfo _ni, llvm::GetElementPtrInst *_ins = nullptr)
        : InstructionNode(_ni, InstructionNode::GetElementPtrInstTy, _ins) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::GetElementPtrInstTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printInputEnable(PrintType, uint32_t) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
};

class LoadNode : public InstructionNode {
   private:
    MemoryPort read_port_data;
    MemoryNode *mem_unit;

   public:
    LoadNode(NodeInfo _ni, llvm::LoadInst *_ins = nullptr, MemoryNode *_node = nullptr)
        : InstructionNode(_ni, InstructionNode::LoadInstructionTy, _ins), mem_unit(_node) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::LoadInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    void setMemoryUnit(MemoryNode *_node) {mem_unit = _node;}

    void addReadMemoryReqPort(Node *);
    void addReadMemoryRespPort(Node *);

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printInputEnable(PrintType, uint32_t) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
};

class StoreNode : public InstructionNode {
   private:
    MemoryPort write_port_data;
    MemoryNode *mem_node;

   public:
    StoreNode(NodeInfo _ni, llvm::StoreInst *_ins = nullptr, MemoryNode *_mem = nullptr)
              //NodeType _nd = UnkonwTy)
        : InstructionNode(_ni, InstructionNode::StoreInstructionTy, _ins), mem_node(_mem) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::StoreInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    void addWriteMemoryReqPort(Node *);
    void addWriteMemoryRespPort(Node *);

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printInputEnable(PrintType, uint32_t) override;
    virtual std::string printInputData(PrintType) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
};

class ReturnNode : public InstructionNode {
   public:
    ReturnNode(NodeInfo _ni, llvm::ReturnInst *_ins = nullptr,
               NodeType _nd = UnkonwTy)
        : InstructionNode(_ni, InstructionNode::ReturnInstrunctionTy, _ins) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::ReturnInstrunctionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputEnable(PrintType) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
};

class CallNode : public InstructionNode {
   public:
    CallNode(NodeInfo _ni, llvm::CallInst *_ins = nullptr,
             NodeType _nd = UnkonwTy)
        : InstructionNode(_ni, InstructionNode::CallInstructionTy, _ins) {}

    static bool classof(const InstructionNode *T) {
        return T->getOpCode() == InstructionNode::CallInstructionTy;
    }
    static bool classof(const Node *T) {
        return isa<InstructionNode>(T) && classof(cast<InstructionNode>(T));
    }
};

class ArgumentNode : public Node {
   private:
    llvm::Argument *parent_argument;

   public:
    ArgumentNode(NodeInfo _ni, llvm::Argument *_arg = nullptr)
        : Node(Node::FunctionArgTy, _ni), parent_argument(_arg) {}

    const llvm::Argument *getArgumentValue() { return parent_argument; }

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printInputData(PrintType, uint32_t) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
};

class GlobalValueNode : public Node {
   private:
    llvm::GlobalValue *parent_glob;

   public:
    GlobalValueNode(NodeInfo _ni, llvm::GlobalValue *_glb = nullptr)
        : Node(Node::GlobalValueTy, _ni), parent_glob(_glb) {}

    llvm::GlobalValue *getGlobalValue();
};

class ConstIntNode : public Node {
   private:
    llvm::ConstantInt *parent_const_int;

   public:
    ConstIntNode(NodeInfo _ni, llvm::ConstantInt *_cint = nullptr)
        : Node(Node::ConstIntTy, _ni), parent_const_int(_cint) {}

    llvm::ConstantInt *getConstantParent();
    virtual std::string printOutputData(PrintType, uint32_t);
};

/**
 * SplitCall node
 */
class SplitCallNode : public Node {
   public:
    using FunctionArgumentList = std::list<std::unique_ptr<ArgumentNode>>;

   private:
    FunctionArgumentList fun_arg_list;

   public:
    explicit SplitCallNode(NodeInfo _nf) : Node(Node::SplitCallTy, _nf) {}

    // Define classof function so that we can use dyn_cast function
    static bool classof(const Node *T) {
        return T->getType() == Node::SuperNodeTy;
    }

    ArgumentNode *insertArgument(llvm::Argument &);

    virtual std::string printDefinition(PrintType) override;
    virtual std::string printOutputEnable(PrintType, uint32_t) override;
    virtual std::string printOutputData(PrintType, uint32_t) override;
};

}  // namespace dandelion

#endif  // end of DANDDELION_NODE_H
