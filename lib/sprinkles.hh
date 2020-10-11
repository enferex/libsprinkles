#ifndef __SPRINKLES_HH
#define __SPRINKLES_HH
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>

#include <map>
#include <memory>
#include <vector>

namespace sprinkles {

// First: Instruction Address, Second: Instruction
using InstList = std::vector<std::pair<uint64_t, llvm::MCInst>>;

class Sprinkles final {
  std::string _inputFname;
  std::vector<std::pair<uint64_t, llvm::MCInst>> _instructions;
  std::vector<llvm::object::SymbolRef> _symbols;
  std::vector<llvm::object::RelocationRef> _relocations;
  std::vector<llvm::object::SectionRef> _sections;
  std::unique_ptr<llvm::object::ObjectFile> _objFile;
  std::unique_ptr<llvm::MemoryBuffer> _objBuffer;
  std::unique_ptr<llvm::MCInstPrinter> _printer;
  std::unique_ptr<const llvm::MCSubtargetInfo> _sti;
  std::unique_ptr<const llvm::MCRegisterInfo> _rinfo;
  std::unique_ptr<const llvm::MCAsmInfo> _ainfo;
  std::unique_ptr<const llvm::MCInstrInfo> _iinfo;

  // The key is the SectionRef 'getIndex()' value.
  std::map<uint64_t,
           std::pair<InstList::const_iterator, InstList::const_iterator>>
      _sectionToInsns;

  llvm::Error parseObject();

 public:
  Sprinkles(const char *fname);
  llvm::Error initialize();
  const llvm::object::ObjectFile *getObjectFile() const;
  const InstList &getInstructions() const;
  const InstList getInstructions(const llvm::object::SymbolRef &sr) const;
  const std::vector<llvm::object::SymbolRef> &getSymbols() const;
  const std::vector<llvm::object::RelocationRef> &getRelocs() const;
  const std::vector<llvm::object::SectionRef> &getSections() const;
  const llvm::MCInstrDesc &getDesc(const llvm::MCInst &mi) const;
  llvm::MCInstPrinter *getPrinter() const;
  void dump(const llvm::MCInst *mi) const;
};

}  // namespace sprinkles
#endif  // __SPRINKLES_HH
