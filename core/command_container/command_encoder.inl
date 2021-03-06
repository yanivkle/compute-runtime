/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "core/command_container/command_encoder.h"
#include "core/command_stream/linear_stream.h"
#include "core/helpers/hw_helper.h"
#include "core/helpers/preamble.h"
#include "core/helpers/register_offsets.h"
#include "core/helpers/simd_helper.h"
#include "core/helpers/string.h"
#include "core/kernel/dispatch_kernel_encoder_interface.h"
#include "runtime/device/device.h"
#include "runtime/execution_environment/execution_environment.h"
#include "runtime/helpers/hardware_commands_helper.h"

#include <algorithm>

namespace NEO {

template <typename Family>
uint32_t EncodeStates<Family>::copySamplerState(IndirectHeap *dsh,
                                                uint32_t samplerStateOffset,
                                                uint32_t samplerCount,
                                                uint32_t borderColorOffset,
                                                const void *fnDynamicStateHeap) {
    auto sizeSamplerState = sizeof(SAMPLER_STATE) * samplerCount;
    auto borderColorSize = samplerStateOffset - borderColorOffset;

    dsh->align(alignIndirectStatePointer);
    auto borderColorOffsetInDsh = static_cast<uint32_t>(dsh->getUsed());

    auto borderColor = dsh->getSpace(borderColorSize);

    memcpy_s(borderColor, borderColorSize, ptrOffset(fnDynamicStateHeap, borderColorOffset),
             borderColorSize);

    dsh->align(INTERFACE_DESCRIPTOR_DATA::SAMPLERSTATEPOINTER_ALIGN_SIZE);
    auto samplerStateOffsetInDsh = static_cast<uint32_t>(dsh->getUsed());

    auto samplerState = dsh->getSpace(sizeSamplerState);

    memcpy_s(samplerState, sizeSamplerState, ptrOffset(fnDynamicStateHeap, samplerStateOffset),
             sizeSamplerState);

    auto pSmplr = reinterpret_cast<SAMPLER_STATE *>(samplerState);
    for (uint32_t i = 0; i < samplerCount; i++) {
        pSmplr[i].setIndirectStatePointer((uint32_t)borderColorOffsetInDsh);
    }

    return samplerStateOffsetInDsh;
}
template <typename Family>
void EncodeMathMMIO<Family>::encodeMulRegVal(CommandContainer &container, uint32_t offset, uint32_t val, uint64_t dstAddress) {
    int logLws = 0;
    int addsCount = 0;
    int i = val;
    while (val >> logLws) {
        if (val & (1 << logLws)) {
            addsCount++;
        }
        logLws++;
        addsCount++;
    }

    EncodeSetMMIO<Family>::encodeREG(container, CS_GPR_R0, offset);
    EncodeSetMMIO<Family>::encodeIMM(container, CS_GPR_R1, 0);

    uint32_t length = NUM_ALU_INST_FOR_READ_MODIFY_WRITE * addsCount;

    auto cmd2 = reinterpret_cast<uint32_t *>(container.getCommandStream()->getSpace(sizeof(MI_MATH) + sizeof(MI_MATH_ALU_INST_INLINE) * length));
    reinterpret_cast<MI_MATH *>(cmd2)->DW0.Value = 0x0;
    reinterpret_cast<MI_MATH *>(cmd2)->DW0.BitField.InstructionType = MI_MATH::COMMAND_TYPE_MI_COMMAND;
    reinterpret_cast<MI_MATH *>(cmd2)->DW0.BitField.InstructionOpcode = MI_MATH::MI_COMMAND_OPCODE_MI_MATH;
    reinterpret_cast<MI_MATH *>(cmd2)->DW0.BitField.DwordLength = length - 1;
    cmd2++;

    MI_MATH_ALU_INST_INLINE *pAluParam = reinterpret_cast<MI_MATH_ALU_INST_INLINE *>(cmd2);
    i = 0;
    while (i < logLws) {
        if (val & (1 << i)) {
            encodeAluAdd(pAluParam, ALU_REGISTER_R_1, ALU_REGISTER_R_0);
            pAluParam += NUM_ALU_INST_FOR_READ_MODIFY_WRITE;
        }
        encodeAluAdd(pAluParam, ALU_REGISTER_R_0, ALU_REGISTER_R_0);
        pAluParam += NUM_ALU_INST_FOR_READ_MODIFY_WRITE;
        i++;
    }
    EncodeStoreMMIO<Family>::encode(container, CS_GPR_R1, dstAddress);
}

template <typename Family>
void EncodeMathMMIO<Family>::encodeGreaterThanPredicate(CommandContainer &container, uint64_t lhsVal, uint32_t rhsVal) {
    EncodeSetMMIO<Family>::encodeMEM(container, CS_GPR_R0, lhsVal);
    EncodeSetMMIO<Family>::encodeIMM(container, CS_GPR_R1, rhsVal);

    size_t size = sizeof(MI_MATH) + sizeof(MI_MATH_ALU_INST_INLINE) * NUM_ALU_INST_FOR_READ_MODIFY_WRITE;

    auto cmd = reinterpret_cast<uint32_t *>(container.getCommandStream()->getSpace(size));
    reinterpret_cast<MI_MATH *>(cmd)->DW0.Value = 0x0;
    reinterpret_cast<MI_MATH *>(cmd)->DW0.BitField.InstructionType = MI_MATH::COMMAND_TYPE_MI_COMMAND;
    reinterpret_cast<MI_MATH *>(cmd)->DW0.BitField.InstructionOpcode = MI_MATH::MI_COMMAND_OPCODE_MI_MATH;
    reinterpret_cast<MI_MATH *>(cmd)->DW0.BitField.DwordLength = NUM_ALU_INST_FOR_READ_MODIFY_WRITE - 1;
    cmd++;

    encodeAluSubStoreCarry(reinterpret_cast<MI_MATH_ALU_INST_INLINE *>(cmd), ALU_REGISTER_R_0, ALU_REGISTER_R_1);

    EncodeSetMMIO<Family>::encodeREG(container, CS_PREDICATE_RESULT, CS_GPR_R0);
}

template <typename Family>
void EncodeMathMMIO<Family>::encodeAlu(MI_MATH_ALU_INST_INLINE *pAluParam, uint32_t srcA, uint32_t srcB, uint32_t op, uint32_t dest, uint32_t result) {
    pAluParam->DW0.BitField.ALUOpcode = ALU_OPCODE_LOAD;
    pAluParam->DW0.BitField.Operand1 = ALU_REGISTER_R_SRCA;
    pAluParam->DW0.BitField.Operand2 = srcA;
    pAluParam++;

    pAluParam->DW0.BitField.ALUOpcode = ALU_OPCODE_LOAD;
    pAluParam->DW0.BitField.Operand1 = ALU_REGISTER_R_SRCB;
    pAluParam->DW0.BitField.Operand2 = srcB;
    pAluParam++;

    pAluParam->DW0.BitField.ALUOpcode = op;
    pAluParam->DW0.BitField.Operand1 = 0;
    pAluParam->DW0.BitField.Operand2 = 0;
    pAluParam++;

    pAluParam->DW0.BitField.ALUOpcode = ALU_OPCODE_STORE;
    pAluParam->DW0.BitField.Operand1 = dest;
    pAluParam->DW0.BitField.Operand2 = result;
    pAluParam++;
}

template <typename Family>
void EncodeMathMMIO<Family>::encodeAluSubStoreCarry(MI_MATH_ALU_INST_INLINE *pAluParam, uint32_t regA, uint32_t regB) {
    encodeAlu(pAluParam, regA, regB, ALU_OPCODE_SUB, ALU_REGISTER_R_CF, regA);
}

template <typename Family>
void EncodeMathMMIO<Family>::encodeAluAdd(MI_MATH_ALU_INST_INLINE *pAluParam, uint32_t regA, uint32_t regB) {
    encodeAlu(pAluParam, regA, regB, ALU_OPCODE_ADD, ALU_REGISTER_R_ACCU, regA);
}

template <typename Family>
void EncodeIndirectParams<Family>::setGroupCountIndirect(CommandContainer &container, uint32_t offsets[3], void *crossThreadAddress) {
    EncodeStoreMMIO<Family>::encode(container, GPUGPU_DISPATCHDIMX, ptrOffset(reinterpret_cast<uint64_t>(crossThreadAddress), offsets[0]));
    EncodeStoreMMIO<Family>::encode(container, GPUGPU_DISPATCHDIMY, ptrOffset(reinterpret_cast<uint64_t>(crossThreadAddress), offsets[1]));
    EncodeStoreMMIO<Family>::encode(container, GPUGPU_DISPATCHDIMZ, ptrOffset(reinterpret_cast<uint64_t>(crossThreadAddress), offsets[2]));
}

template <typename Family>
void EncodeIndirectParams<Family>::setGroupSizeIndirect(CommandContainer &container, uint32_t offsets[3], void *crossThreadAddress, uint32_t lws[3]) {
    EncodeMathMMIO<Family>::encodeMulRegVal(container, GPUGPU_DISPATCHDIMX, lws[0], ptrOffset(reinterpret_cast<uint64_t>(crossThreadAddress), offsets[0]));
    EncodeMathMMIO<Family>::encodeMulRegVal(container, GPUGPU_DISPATCHDIMY, lws[1], ptrOffset(reinterpret_cast<uint64_t>(crossThreadAddress), offsets[1]));
    EncodeMathMMIO<Family>::encodeMulRegVal(container, GPUGPU_DISPATCHDIMZ, lws[2], ptrOffset(reinterpret_cast<uint64_t>(crossThreadAddress), offsets[2]));
}

template <typename Family>
void EncodeFlush<Family>::encode(CommandContainer &container) {
    PIPE_CONTROL cmd = Family::cmdInitPipeControl;
    cmd.setCommandStreamerStallEnable(true);
    cmd.setDcFlushEnable(true);
    auto buffer = container.getCommandStream()->getSpace(sizeof(cmd));
    *(PIPE_CONTROL *)buffer = cmd;
}

template <typename Family>
void EncodeFlush<Family>::encodeWithQwordWrite(CommandContainer &container, uint64_t gpuAddress,
                                               uint64_t value, bool dcFlushEnable) {
    PIPE_CONTROL cmd = Family::cmdInitPipeControl;
    cmd.setPostSyncOperation(POST_SYNC_OPERATION::POST_SYNC_OPERATION_WRITE_IMMEDIATE_DATA);
    cmd.setImmediateData(value);
    cmd.setCommandStreamerStallEnable(true);
    if (dcFlushEnable) {
        cmd.setDcFlushEnable(true);
    }
    cmd.setAddressHigh(gpuAddress >> 32u);
    cmd.setAddress(uint32_t(gpuAddress));

    auto buffer = container.getCommandStream()->getSpace(sizeof(cmd));
    *(PIPE_CONTROL *)buffer = cmd;
}

template <typename Family>
void EncodeSetMMIO<Family>::encodeIMM(CommandContainer &container, uint32_t offset, uint32_t data) {
    MI_LOAD_REGISTER_IMM cmd = Family::cmdInitLoadRegisterImm;
    cmd.setRegisterOffset(offset);
    cmd.setDataDword(data);
    auto buffer = container.getCommandStream()->getSpace(sizeof(cmd));
    *(MI_LOAD_REGISTER_IMM *)buffer = cmd;
}

template <typename Family>
void EncodeSetMMIO<Family>::encodeMEM(CommandContainer &container, uint32_t offset, uint64_t address) {
    MI_LOAD_REGISTER_MEM cmd = Family::cmdInitLoadRegisterMem;
    cmd.setRegisterAddress(offset);
    cmd.setMemoryAddress(address);
    auto buffer = container.getCommandStream()->getSpace(sizeof(cmd));
    *(MI_LOAD_REGISTER_MEM *)buffer = cmd;
}

template <typename Family>
void EncodeSetMMIO<Family>::encodeREG(CommandContainer &container, uint32_t dstOffset, uint32_t srcOffset) {
    MI_LOAD_REGISTER_REG cmd = Family::cmdInitLoadRegisterReg;
    cmd.setSourceRegisterAddress(srcOffset);
    cmd.setDestinationRegisterAddress(dstOffset);
    auto buffer = container.getCommandStream()->getSpace(sizeof(cmd));
    *(MI_LOAD_REGISTER_REG *)buffer = cmd;
}

template <typename Family>
void EncodeStoreMMIO<Family>::encode(CommandContainer &container, uint32_t offset, uint64_t address) {
    MI_STORE_REGISTER_MEM cmd = Family::cmdInitStoreRegisterMem;
    cmd.setRegisterAddress(offset);
    cmd.setMemoryAddress(address);
    auto buffer = container.getCommandStream()->getSpace(sizeof(cmd));
    *(MI_STORE_REGISTER_MEM *)buffer = cmd;
}

template <typename Family>
void EncodeSurfaceState<Family>::encodeBuffer(void *dst, void *address, size_t size, uint32_t mocs,
                                              bool cpuCoherent) {
    auto ss = reinterpret_cast<R_SURFACE_STATE *>(dst);
    UNRECOVERABLE_IF(!isAligned<getSurfaceBaseAddressAlignment()>(size));

    SURFACE_STATE_BUFFER_LENGTH Length = {0};
    Length.Length = static_cast<uint32_t>(size - 1);

    ss->setWidth(Length.SurfaceState.Width + 1);
    ss->setHeight(Length.SurfaceState.Height + 1);
    ss->setDepth(Length.SurfaceState.Depth + 1);

    ss->setSurfaceType((address != nullptr) ? R_SURFACE_STATE::SURFACE_TYPE_SURFTYPE_BUFFER
                                            : R_SURFACE_STATE::SURFACE_TYPE_SURFTYPE_NULL);
    ss->setSurfaceFormat(SURFACE_FORMAT::SURFACE_FORMAT_RAW);
    ss->setSurfaceVerticalAlignment(R_SURFACE_STATE::SURFACE_VERTICAL_ALIGNMENT_VALIGN_4);
    ss->setSurfaceHorizontalAlignment(R_SURFACE_STATE::SURFACE_HORIZONTAL_ALIGNMENT_HALIGN_4);

    ss->setTileMode(R_SURFACE_STATE::TILE_MODE_LINEAR);
    ss->setVerticalLineStride(0);
    ss->setVerticalLineStrideOffset(0);
    ss->setMemoryObjectControlState(mocs);
    ss->setSurfaceBaseAddress(reinterpret_cast<uintptr_t>(address));

    ss->setCoherencyType(cpuCoherent ? R_SURFACE_STATE::COHERENCY_TYPE_IA_COHERENT
                                     : R_SURFACE_STATE::COHERENCY_TYPE_GPU_COHERENT);
    ss->setAuxiliarySurfaceMode(AUXILIARY_SURFACE_MODE::AUXILIARY_SURFACE_MODE_AUX_NONE);
}

template <typename Family>
void EncodeStates<Family>::adjustStateComputeMode(CommandContainer &container) {
}

template <typename Family>
void *EncodeDispatchKernel<Family>::getInterfaceDescriptor(CommandContainer &container, uint32_t &iddOffset) {

    if (container.nextIddInBlock == container.numIddsPerBlock) {
        container.getIndirectHeap(HeapType::DYNAMIC_STATE)->align(HardwareCommandsHelper<Family>::alignInterfaceDescriptorData);
        container.setIddBlock(container.getHeapSpaceAllowGrow(HeapType::DYNAMIC_STATE,
                                                              sizeof(INTERFACE_DESCRIPTOR_DATA) * container.numIddsPerBlock));
        container.nextIddInBlock = 0;

        EncodeMediaInterfaceDescriptorLoad<Family>::encode(container);
    }

    iddOffset = container.nextIddInBlock;
    auto interfaceDescriptorData = static_cast<INTERFACE_DESCRIPTOR_DATA *>(container.getIddBlock());
    return &interfaceDescriptorData[container.nextIddInBlock++];
}
template <typename Family>
size_t EncodeStates<Family>::getAdjustStateComputeModeSize() {
    return 0;
}

template <typename Family>
size_t EncodeIndirectParams<Family>::getCmdsSizeForIndirectParams() {
    return 3 * sizeof(typename Family::MI_LOAD_REGISTER_MEM);
}

template <typename Family>
size_t EncodeIndirectParams<Family>::getCmdsSizeForSetGroupCountIndirect() {
    return 3 * (sizeof(MI_STORE_REGISTER_MEM));
}

template <typename Family>
size_t EncodeIndirectParams<Family>::getCmdsSizeForSetGroupSizeIndirect() {
    return 3 * (sizeof(MI_LOAD_REGISTER_REG) + sizeof(MI_LOAD_REGISTER_IMM) + sizeof(MI_MATH) + sizeof(MI_MATH_ALU_INST_INLINE) + sizeof(MI_STORE_REGISTER_MEM));
}

} // namespace NEO