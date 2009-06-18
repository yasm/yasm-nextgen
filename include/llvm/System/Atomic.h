//===- llvm/System/Atomic.h - Atomic Operations -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the llvm::sys atomic operations.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SYSTEM_ATOMIC_H
#define LLVM_SYSTEM_ATOMIC_H

#include "llvm/Support/DataTypes.h"
#include "yasmx/Config/export.h"

namespace llvm {
  namespace sys {
    YASM_LIB_EXPORT
    void MemoryFence();

    typedef uint32_t cas_flag;
    YASM_LIB_EXPORT
    cas_flag CompareAndSwap(volatile cas_flag* ptr,
                            cas_flag new_value,
                            cas_flag old_value);
  }
}

#endif
