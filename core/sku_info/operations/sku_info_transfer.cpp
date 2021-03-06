/*
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "core/sku_info/operations/sku_info_transfer.h"

namespace NEO {
void SkuInfoTransfer::transferWaTableForGmm(_WA_TABLE *dstWaTable, const NEO::WorkaroundTable *srcWaTable) {
    transferWaTableForGmmBase(dstWaTable, srcWaTable);
}

void SkuInfoTransfer::transferFtrTableForGmm(_SKU_FEATURE_TABLE *dstFtrTable, const NEO::FeatureTable *srcFtrTable) {
    transferFtrTableForGmmBase(dstFtrTable, srcFtrTable);
}
} // namespace NEO
