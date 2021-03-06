/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "core/command_container/cmdcontainer.h"
#include "core/command_stream/linear_stream.h"
#include "core/helpers/simd_helper.h"
#include "core/kernel/dispatch_kernel_encoder_interface.h"
#include "runtime/execution_environment/execution_environment.h"

#include <algorithm>

namespace NEO {

template <typename GfxFamily>
struct EncodeDispatchKernel {
    using WALKER_TYPE = typename GfxFamily::WALKER_TYPE;
    using INTERFACE_DESCRIPTOR_DATA = typename GfxFamily::INTERFACE_DESCRIPTOR_DATA;
    using BINDING_TABLE_STATE = typename GfxFamily::BINDING_TABLE_STATE;

    static void encode(CommandContainer &container,
                       const void *pThreadGroupDimensions, bool isIndirect, bool isPredicate, DispatchKernelEncoderI *dispatchInterface,
                       GraphicsAllocation *eventAllocation, Device *device, PreemptionMode preemptionMode);

    static void *getInterfaceDescriptor(CommandContainer &container, uint32_t &iddOffset);

    static size_t estimateEncodeDispatchKernelCmdsSize(Device *device);
};

template <typename GfxFamily>
struct EncodeStates {
    using BINDING_TABLE_STATE = typename GfxFamily::BINDING_TABLE_STATE;
    using INTERFACE_DESCRIPTOR_DATA = typename GfxFamily::INTERFACE_DESCRIPTOR_DATA;
    using SAMPLER_STATE = typename GfxFamily::SAMPLER_STATE;

    static const uint32_t alignIndirectStatePointer = MemoryConstants::cacheLineSize;

    static uint32_t copySamplerState(IndirectHeap *dsh,
                                     uint32_t samplerStateOffset,
                                     uint32_t samplerCount,
                                     uint32_t borderColorOffset,
                                     const void *fnDynamicStateHeap);

    static void adjustStateComputeMode(CommandContainer &container);

    static size_t getAdjustStateComputeModeSize();
};

template <typename GfxFamily>
struct EncodeMathMMIO {
    using MI_STORE_REGISTER_MEM = typename GfxFamily::MI_STORE_REGISTER_MEM;
    using MI_MATH_ALU_INST_INLINE = typename GfxFamily::MI_MATH_ALU_INST_INLINE;
    using MI_MATH = typename GfxFamily::MI_MATH;

    static const size_t size = sizeof(MI_STORE_REGISTER_MEM);

    static void encodeMulRegVal(CommandContainer &container, uint32_t offset, uint32_t val, uint64_t dstAddress);

    static void encodeGreaterThanPredicate(CommandContainer &container, uint64_t lhsVal, uint32_t rhsVal);

    static void encodeAlu(MI_MATH_ALU_INST_INLINE *pAluParam, uint32_t srcA, uint32_t srcB, uint32_t op, uint32_t dest, uint32_t result);

    static void encodeAluSubStoreCarry(MI_MATH_ALU_INST_INLINE *pAluParam, uint32_t regA, uint32_t regB);

    static void encodeAluAdd(MI_MATH_ALU_INST_INLINE *pAluParam, uint32_t regA, uint32_t regB);
};

template <typename GfxFamily>
struct EncodeIndirectParams {
    using MI_LOAD_REGISTER_IMM = typename GfxFamily::MI_LOAD_REGISTER_IMM;
    using MI_LOAD_REGISTER_MEM = typename GfxFamily::MI_LOAD_REGISTER_MEM;
    using MI_LOAD_REGISTER_REG = typename GfxFamily::MI_LOAD_REGISTER_REG;
    using MI_STORE_REGISTER_MEM = typename GfxFamily::MI_STORE_REGISTER_MEM;
    using MI_MATH = typename GfxFamily::MI_MATH;
    using MI_MATH_ALU_INST_INLINE = typename GfxFamily::MI_MATH_ALU_INST_INLINE;
    static void setGroupCountIndirect(CommandContainer &container, uint32_t offsets[3], void *crossThreadAddress);

    static void setGroupSizeIndirect(CommandContainer &container, uint32_t offsets[3], void *crossThreadAddress, uint32_t lws[3]);

    static size_t getCmdsSizeForIndirectParams();
    static size_t getCmdsSizeForSetGroupSizeIndirect();
    static size_t getCmdsSizeForSetGroupCountIndirect();
};

template <typename GfxFamily>
struct EncodeFlush {
    using PIPE_CONTROL = typename GfxFamily::PIPE_CONTROL;
    using POST_SYNC_OPERATION = typename PIPE_CONTROL::POST_SYNC_OPERATION;
    static void encode(CommandContainer &container);

    static void encodeWithQwordWrite(CommandContainer &container, uint64_t gpuAddress,
                                     uint64_t value, bool dcFlushEnable);
};

template <typename GfxFamily>
struct EncodeSetMMIO {
    using MI_LOAD_REGISTER_IMM = typename GfxFamily::MI_LOAD_REGISTER_IMM;
    using MI_LOAD_REGISTER_MEM = typename GfxFamily::MI_LOAD_REGISTER_MEM;
    using MI_LOAD_REGISTER_REG = typename GfxFamily::MI_LOAD_REGISTER_REG;

    static const size_t sizeIMM = sizeof(MI_LOAD_REGISTER_IMM);
    static const size_t sizeMEM = sizeof(MI_LOAD_REGISTER_MEM);
    static const size_t sizeREG = sizeof(MI_LOAD_REGISTER_REG);

    static void encodeIMM(CommandContainer &container, uint32_t offset, uint32_t data);

    static void encodeMEM(CommandContainer &container, uint32_t offset, uint64_t address);

    static void encodeREG(CommandContainer &container, uint32_t dstOffset, uint32_t srcOffset);
};

template <typename GfxFamily>
struct EncodeL3State {
    static void encode(CommandContainer &container, bool enableSLM);
};

template <typename GfxFamily>
struct EncodeMediaInterfaceDescriptorLoad {
    using INTERFACE_DESCRIPTOR_DATA = typename GfxFamily::INTERFACE_DESCRIPTOR_DATA;

    static void encode(CommandContainer &container);
};

template <typename GfxFamily>
struct EncodeStateBaseAddress {
    static void encode(CommandContainer &container);
};

template <typename GfxFamily>
struct EncodeStoreMMIO {
    using MI_STORE_REGISTER_MEM = typename GfxFamily::MI_STORE_REGISTER_MEM;

    static const size_t size = sizeof(MI_STORE_REGISTER_MEM);
    static void encode(CommandContainer &container, uint32_t offset, uint64_t address);
};

template <typename GfxFamily>
struct EncodeSurfaceState {
    using R_SURFACE_STATE = typename GfxFamily::RENDER_SURFACE_STATE;
    using SURFACE_FORMAT = typename R_SURFACE_STATE::SURFACE_FORMAT;
    using AUXILIARY_SURFACE_MODE = typename R_SURFACE_STATE::AUXILIARY_SURFACE_MODE;

    static void encodeBuffer(void *dst, void *address, size_t size, uint32_t mocs,
                             bool cpuCoherent);

    static constexpr uintptr_t getSurfaceBaseAddressAlignmentMask() {
        return ~(getSurfaceBaseAddressAlignment() - 1);
    }

    static constexpr uintptr_t getSurfaceBaseAddressAlignment() { return 4; }
};

template <typename GfxFamily>
struct EncodeComputeMode {
    static void adjustComputeMode(CommandContainer &container, uint32_t numGrfRequired);
};
} // namespace NEO