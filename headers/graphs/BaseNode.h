#ifndef APOLLO_GRAPHS_BASENODE
#define APOLLO_GRAPHS_BASENODE

// Shared namespace within the project.
namespace apollo {

// All the types of visitors that are allowed to exist in our custom graph types.
// Necessary for the visit() and accept() functions when using the recursive
// Visitor pattern in node classes.
class Visitor;
class DataDependenceVisitor;
class ControlFlowVisitor;
class MemoryDependenceVisitor;
class ProgramDependenceVisitor;

// Base class for all nodes (including top-level graph nodes).
class BaseNode {
public:
  /* Destructor for all nodes.
   *
   * Default: Use C++'s default destruction process.
   */
  virtual ~BaseNode() { }

  /* [accept] records actions from the generic visitor [v].
   *   Returns nothing.
   *     [v]: A generic visitor that operates on all types of graphs.
   *
   * Default: Purely virtual.
   */
  virtual void accept(Visitor &v) = 0;

  /* [accept] records actions from the data-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on data-dependence graphs.
   *
   * Default: Purely virtual.
   */
  virtual void accept(DataDependenceVisitor &v) = 0;

  /* [accept] records actions from the control-flow visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on control-flow graphs.
   *
   * Default: Purely virtual.
   */
  virtual void accept(ControlFlowVisitor &v) = 0;

  /* [accept] records actions from the memory-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on memory-dependence graphs.
   *
   * Default: Purely virtual.
   */
  virtual void accept(MemoryDependenceVisitor &v) = 0;

  /* [accept] records actions from the program-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on program-dependence graphs.
   *
   * Default: Purely virtual.
   */
  virtual void accept(ProgramDependenceVisitor &v) = 0;

};

}

#endif
