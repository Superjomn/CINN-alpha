#include "cinn/hlir/builder.h"
#include "cinn/backends/code_gen_c.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {

ir::Expr Builder::Build(Session *session, Network *net) {
  Program program = net->Compile();

  Graph graph;
  graph.Build(program, *session);

  for (Node &node : GraphTraits::TS(graph)) {
    if (node.is_op()) node.op->Compile();
  }

  LOG(INFO) << "DOT:\n" << graph.dot();

  auto fns = graph.PartitionFunctions();
  AutoFuseStages(&fns);

  auto main_expr = graph.CompileExpr(&fns);
  auto global_vars = DeclBuffersGlobal(session, *net);

  AddMainFnToProgram(&main_expr, CreateMainFn(main_expr));
  AddIOFnsToProgram(&main_expr, CreateLoadInputFns(*net, *session));
  AddIOFnsToProgram(&main_expr, CreateGetOutputFns(*net, *session));

  auto expr = ir::Module::make(global_vars, main_expr);
  return expr;
}

Expr Builder::CreateExprForWeightDeclaration(const Session &session, const Network &network) {
  std::vector<ir::Expr> exprs;

  exprs.push_back(ir::Mark::make("create weight buffers"));
  for (auto &x : network.weight_names()) {
    Tensor *tensor = session.GetTensor(x);
    CHECK(tensor);
    Target target;
    CHECK(tensor->ptype() != primitive_t::unk);
    Expr size(tensor->shape().num_bytes(tensor->ptype()));
    auto expr = ir::BufferOpr::make(target, size, ir::BufferOpr::Opr::kCreateAssign, tensor->ptype(), tensor->name());
    expr.As<ir::BufferOpr>()->assigned_data.set<std::vector<float>>(tensor->buffer()->data<std::vector<float>>());
    exprs.push_back(expr);
  }

  ir::Expr block = ir::Block::make(std::move(exprs));
  return block;
}

Expr Builder::CreateExprForInputOutputDeclaration(const Session &session, const Network &network) {
  std::vector<ir::Expr> exprs;

  auto create_tensor = [&](const std::string &name) {
    auto *tensor = session.GetTensor(name);
    CHECK(tensor);
    CHECK(tensor->ptype() != primitive_t::unk);
    Expr size(tensor->shape().num_bytes(tensor->ptype()));
    Target target;
    auto expr = ir::BufferOpr::make(target, size, ir::BufferOpr::Opr::kCreate, tensor->ptype(), tensor->name());
    exprs.emplace_back(expr);
  };

  exprs.push_back(ir::Mark::make("create input buffers"));
  for (auto &x : network.input_names()) {
    create_tensor(x);
  }
  exprs.push_back(ir::Mark::make("create output buffers"));
  for (auto &x : network.output_names()) {
    create_tensor(x);
  }
  exprs.push_back(ir::Mark::make("create temporary variable buffers"));
  for (auto &x : network.tmp_var_names()) {
    create_tensor(x);
  }

  return ir::Block::make(std::move(exprs));
}

void Builder::ToCSourceCode(ir::Expr expr, const std::string &prefix) {
  LOG(INFO) << "output header file to " << prefix + ".h";
  LOG(INFO) << "output source file to " << prefix + ".cc";
  backends::CompileAsC(expr, prefix + ".h", prefix + ".cc");
}

Expr Builder::DeclBuffersGlobal(Session *session, const Network &net) {
  std::vector<ir::Expr> exprs(
      {CreateExprForWeightDeclaration(*session, net), CreateExprForInputOutputDeclaration(*session, net)});
  return ir::Block::make(std::move(exprs));
}

Expr Builder::CreateLoadInputFns(const Network &net, const Session &session) {
  std::vector<Expr> exprs;
  exprs.emplace_back(ir::Mark::make("functions for loadding input data"));
  for (auto &x : net.input_names()) {
    std::string fn_name = StringFormat(load_fn_name_format, x.c_str());
    // cinn_copy(x_, x)
    auto ptype = session.GetTensor(x)->ptype();
    auto size = session.GetTensor(x)->shape().num_bytes(ptype);
    Expr arg(x + "_", ptype);
    std::vector<Expr> fn_body({ir::Call::make("cinn_copy",  //
                                              {arg /*source*/, Expr(x, ptype) /*target*/, Expr(size) /*bytes*/})});

    exprs.emplace_back(ir::Function::make(fn_name, {arg}, {}, ir::Block::make(std::move(fn_body))));
  }

  return ir::Block::make(std::move(exprs));
}

Expr Builder::CreateGetOutputFns(const Network &net, const Session &session) {
  std::vector<Expr> exprs;
  exprs.emplace_back(ir::Mark::make("functions for reading output data"));
  for (auto &x : net.output_names()) {
    std::string fn_name = StringFormat(read_fn_name_format, x.c_str());
    // cinn_copy(x_, x)
    auto ptype = session.GetTensor(x)->ptype();
    Expr arg(x + "_", ptype);
    auto size = session.GetTensor(x)->shape().num_bytes(ptype);
    std::vector<Expr> fn_body({ir::Call::make("cinn_copy",  //
                                              {Expr(x, ptype) /*source*/, arg /*target*/, Expr(size) /*bytes*/})});

    exprs.emplace_back(ir::Function::make(fn_name, {arg}, {}, ir::Block::make(std::move(fn_body))));
  }

  return ir::Block::make(std::move(exprs));
}

Expr CreateFnCall(const ir::Function *fn) {
  std::vector<Expr> args;
  args.insert(args.end(), fn->inputs.begin(), fn->inputs.end());
  args.insert(args.end(), fn->outputs.begin(), fn->outputs.end());
  return ir::Call::make(fn->name(), args);
}

Expr Builder::CreateMainFn(Expr expr) {
  std::vector<Expr> body;

  // get all the functions in the expression, and call them in order.
  auto *block = expr.As<ir::Block>();
  CHECK(block);
  for (auto &expr : block->body) {
    auto *fn = expr.As<ir::Function>();
    if (fn) {
      body.push_back(CreateFnCall(fn));
    }
  }

  return ir::Function::make(main_fn_name, {}, {}, ir::Block::make(std::move(body)));
}

void Builder::AddMainFnToProgram(Expr *program, Expr main_fn) {
  auto *block = program->As<ir::Block>();
  CHECK(block);
  block->body.push_back(main_fn);
}

void Builder::AddIOFnsToProgram(Expr *program, Expr io_fns) {
  auto *block = program->As<ir::Block>();
  CHECK(block);
  block->body.insert(std::begin(block->body), io_fns);
}

void Builder::AutoFuseStages(std::vector<Function> *fns) {
  LOG_INDENT(0);
  auto fuse_stages_in_a_fn = [](Function *fn) {
    std::map<std::string, Stage *> pre_stages;
    CHECK(!fn->end_definition());
    for (auto &stage : fn->stages()) {
      CINN_DEBUG(2) << "testing stage: " << stage.name() << " " << stage.expr();
      // check has dependency.
      for (auto &item : pre_stages) {
        CINN_DEBUG(2) << "testing target " << item.first;
        if (TwoStagesHasDependency(stage, *item.second)) {
          stage.FuseWith(*item.second);
          CINN_DEBUG(0) << "fuse " << stage.name() << " with " << item.first;
          // Don't continue to fuse more previous stages to avoid fusion mark explosion, that may lower the tile
          // performance.
          break;
        }
      }
      pre_stages.emplace(stage.name(), &stage);
    }
  };

  for (auto &fn : *fns) {
    fuse_stages_in_a_fn(&fn);
  }
}

}  // namespace hlir
}  // namespace cinn
