#include <llvm/MC/MCInst.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

#include "sprinkles.hh"
using namespace llvm;

static cl::OptionCategory OCat("Vorhees Options");
static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<object file>"),
                                          cl::cat(OCat));

static cl::opt<bool> Dump("dump", cl::desc("Dump disassembly (not JSON)."),
                          cl::cat(OCat));

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "vorhees: Disassembly to JSON.");
  InitLLVM(argc, argv);

  const object::SymbolRef emptySym;

  sprinkles::Sprinkles sdis(InputFilename.c_str());
  if (auto err = sdis.initialize()) {
    llvm::errs() << err;
    return 0;
  }
  auto printer = sdis.getPrinter();
  if (Dump)
    for (const auto Pr : sdis.getInstructions()) {
      sdis.dump(&Pr.second);
      llvm::outs() << '\n';
    }
  else {
    auto printer = sdis.getPrinter();
    llvm::json::Array insns;
    std::string sdata;
    llvm::raw_string_ostream sos(sdata);
    llvm::json::OStream j(llvm::outs());
    j.object([&] {
      j.attribute("FileName", InputFilename);
      j.attributeArray("Sections", [&] {
        for (const object::SectionRef &s : sdis.getSections()) {
          j.object([&] {
            if (auto name = s.getName()) j.attribute("Name", *name);
            j.attribute("Address", s.getAddress());
          });
        }
      });
      j.attributeArray("Symbols", [&] {
        for (const object::SymbolRef &sr : sdis.getSymbols()) {
          j.object([&] {
            if (auto name = sr.getName()) j.attribute("Name", *name);
            if (auto addr = sr.getAddress()) j.attribute("Address", *addr);
            if (auto val = sr.getValue()) j.attribute("Value", *val);
          });
        }
      });
      j.attributeArray("Relocations", [&] {
        for (const object::RelocationRef &rr : sdis.getRelocs()) {
          j.object([&] {
            auto s = rr.getSymbol();
            if (s != emptySym) {
              if (auto n = s->getName()) j.attribute("Symbol", *n);
              j.attribute("Offset", rr.getOffset());
              j.attribute("Type", rr.getType());
            }
          });
        }
      });
      j.attributeArray("OpCodes", [&] {
        for (const auto &pr : sdis.getInstructions()) {
          j.object([&] {
            const MCInst &i = pr.second;
            j.attribute("Name", printer->getOpcodeName(i.getOpcode()));
            j.attribute("OpCode", i.getOpcode());
            j.attribute("Address", pr.first);
            j.attributeArray("Operands", [&] {
              for (const auto &opnd : i) {
                sdata.clear();
                if (opnd.isReg() && opnd.getReg() != 0) {
                  printer->printRegName(sos, opnd.getReg());
                  sos.flush();
                  j.object([&] { j.attribute("Register", sdata); });
                } else if (opnd.isImm())
                  j.object([&] { j.attribute("Immediate", opnd.getImm()); });
                else if (opnd.isFPImm())
                  j.object([&] { j.attribute("Immediate", opnd.getFPImm()); });
                else {
                  opnd.print(sos);
                  sos.flush();
                  j.object([&] { j.attribute("Other", sdata); });
                }
              }
            });
          });
        }
      });
    });
  }
  return 0;
}
