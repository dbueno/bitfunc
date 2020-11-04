//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#include <stdlib.h>

#define DEBUG_TYPE "bfll_bitcode"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include "llvm/Constants.h"
// #if LLVM_VERSION_CODE < LLVM_VERSION(2, 7)
// #include "llvm/ModuleProvider.h"
// #endif
#include "llvm/Type.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TargetSelect.h" // needs llvm 3.0, apparently
#include "llvm/Support/Signals.h"
#include "llvm/Support/system_error.h"
#include <bitfunc.h>
#include "bfll_bitcode.h"

#include <iostream>
#include <fstream>
#include <cerrno>




using namespace llvm;

Module *ll_load_bitcode(bfman *BF, const char *bitcode_path)
{
  // some of this code is basically from klee.
  OwningPtr<MemoryBuffer> BufferPtr;
  error_code ec=MemoryBuffer::getFileOrSTDIN(bitcode_path, BufferPtr);
  if (ec) {
    fprintf(stderr, "error loading program '%s': %s", bitcode_path,
            ec.message().c_str());
    exit(1);
  }
  std::string ErrorMsg;
  Module *mainModule = 0;
  mainModule = getLazyBitcodeModule(BufferPtr.take(), getGlobalContext(), &ErrorMsg);

  if (mainModule) {
    if (mainModule->MaterializeAllPermanently(&ErrorMsg)) {
      delete mainModule;
      mainModule = 0;
    }
  } else {
    fprintf(stderr, "error loading program '%s': %s", bitcode_path,
            ErrorMsg.c_str());
    exit(1);
  }

  return mainModule;

#if 0
  Function *mainFn = mainModule->getFunction("main");
  if (!mainFn) {
    fprintf(stderr, "'main' function not found in module.\n");
    return NULL;
  }
#endif
}

