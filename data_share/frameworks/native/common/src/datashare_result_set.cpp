/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "datashare_result_set.h"

#include <securec.h>
#include <sstream>
#include "datashare_log.h"
#include "parcel.h"
#include "datashare_errno.h"
#include "shared_block.h"
#include "string_ex.h"

namespace OHOS {
namespace DataShare {
namespace {
    // The default position of the cursor
    static const int INITIAL_POS = -1;
    static const size_t DEFAULT_SHARE_BLOCK_SIZE = 2 * 1024 * 1024;
    // Equivalent to filling in setp + 1 rows each time
    static const int STEP_LEN = 2;
} // namespace
int DataShareResultSet::blockId_ = 0;
DataShareResultSet::DataShareResultSet()
{
}

DataShareResultSet::DataShareResultSet(std::shared_ptr<ResultSetBridge> &bridge)
    : bridge_(bridge)
{
    std::string name = "DataShare" + std::to_string(blockId_++);
    blockWriter_ = std::make_shared<DataShareBlockWriterImpl>(name, DEFAULT_SHARE_BLOCK_SIZE);
    if (blockWriter_ == nullptr) {
        return;
    }
    sharedBlock_ = blockWriter_->GetBlock();
    if (sharedBlock_ == nullptr) {
        return;
    }
}

DataShareResultSet::~DataShareResultSet()
{
    ClosedBlock();
}

int DataShareResultSet::GetAllColumnNames(std::vector<std::string> &columnNames)
{
    if (bridge_ == nullptr) {
        LOG_ERROR("bridge_ is null!");
        return E_ERROR;
    }
    return bridge_->GetAllColumnNames(columnNames);
}

int DataShareResultSet::GetRowCount(int &count)
{
    if (bridge_ == nullptr) {
        LOG_ERROR("bridge_ is null!");
        return E_ERROR;
    }
    return bridge_->GetRowCount(count);
}

bool DataShareResultSet::OnGo(int startRowIndex, int targetRowIndex)
{
    if (bridge_ == nullptr || blockWriter_ == nullptr || sharedBlock_ == nullptr) {
        LOG_ERROR("bridge_ or blockWriter_ or sharedBlock_ is null!");
        return E_ERROR;
    }
    std::vector<std::string> columnNames;
    GetAllColumnNames(columnNames);
    sharedBlock_->Clear();
    sharedBlock_->SetColumnNum(columnNames.size());
    return bridge_->OnGo(startRowIndex, targetRowIndex, *blockWriter_);
}

void DataShareResultSet::FillBlock(int startRowIndex, AppDataFwk::SharedBlock *block)
{
    return;
}

/**
 * Get current shared block
 */
AppDataFwk::SharedBlock *DataShareResultSet::GetBlock() const
{
    return sharedBlock_;
}

int DataShareResultSet::GetDataType(int columnIndex, DataType &dataType)
{
    AppDataFwk::SharedBlock::CellUnit *cellUnit =
        sharedBlock_->GetCellUnit((uint32_t)rowPos_ - startRowPos_, (uint32_t)columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::GetDataType cellUnit is null!");
        return E_ERROR;
    }
    dataType = (DataType)cellUnit->type;
    return E_OK;
}

int DataShareResultSet::GoToRow(int position)
{
    if (sharedBlock_ == nullptr) {
        LOG_ERROR("sharedBlock_ is null!");
        return E_ERROR;
    }
    int rowCnt = 0;
    GetRowCount(rowCnt);
    if (position >= rowCnt) {
        rowPos_ = rowCnt;
        return E_ERROR;
    }
    if (position < 0) {
        rowPos_ = INITIAL_POS;
        return E_ERROR;
    }
    if (position == rowPos_) {
        return E_OK;
    }
    bool result = true;
    if (position > endRowPos_ || position < startRowPos_) {
        result = OnGo(position, std::min(position + STEP_LEN, rowCnt - 1));
        if (result) {
            startRowPos_ = position;
            endRowPos_ = position + static_cast<int>(sharedBlock_->GetRowNum()) -1;
        }
    }

    if (!result) {
        rowPos_ = INITIAL_POS;
        startRowPos_ = INITIAL_POS;
        endRowPos_ = INITIAL_POS;
        return E_ERROR;
    } else {
        rowPos_ = position;
        return E_OK;
    }
}

int DataShareResultSet::GetBlob(int columnIndex, std::vector<uint8_t> &value)
{
    int errorCode = CheckState(columnIndex);
    if (errorCode != E_OK) {
        return errorCode;
    }

    AppDataFwk::SharedBlock::CellUnit *cellUnit = sharedBlock_->GetCellUnit(rowPos_ - startRowPos_, columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::GetBlob cellUnit is null!");
        return E_ERROR;
    }

    value.resize(0);
    int type = cellUnit->type;
    if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB
        || type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_STRING) {
        size_t size;
        const auto *blob = static_cast<const uint8_t *>(sharedBlock_->GetCellUnitValueBlob(cellUnit, &size));
        if (size == 0 || blob == nullptr) {
            LOG_WARN("blob data is empty!");
        } else {
            value.resize(size);
            value.assign(blob, blob + size);
        }
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_INTEGER) {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::CELL_UNIT_TYPE_INTEGER!");
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL) {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL!");
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_FLOAT) {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::CELL_UNIT_TYPE_FLOAT!");
        return E_OK;
    } else {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::nothing !");
        return E_INVALID_OBJECT_TYPE;
    }
}

int DataShareResultSet::GetString(int columnIndex, std::string &value)
{
    int errorCode = CheckState(columnIndex);
    if (errorCode != E_OK) {
        return errorCode;
    }
    AppDataFwk::SharedBlock::CellUnit *cellUnit = sharedBlock_->GetCellUnit(rowPos_ - startRowPos_, columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::GetString cellUnit is null!");
        return E_ERROR;
    }
    int type = cellUnit->type;
    if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_STRING) {
        size_t sizeIncludingNull;
        const char *tempValue = sharedBlock_->GetCellUnitValueString(cellUnit, &sizeIncludingNull);
        if ((sizeIncludingNull <= 1) || (tempValue == nullptr)) {
            value = "";
            return E_ERROR;
        }
        value = tempValue;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_INTEGER) {
        int64_t tempValue = cellUnit->cell.longValue;
        value = std::to_string(tempValue);
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_FLOAT) {
        double tempValue = cellUnit->cell.doubleValue;
        std::ostringstream os;
        if (os << tempValue)
            value = os.str();
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL) {
        LOG_ERROR("DataShareResultSet::AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL!");
        return E_ERROR;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB) {
        LOG_ERROR("DataShareResultSet::AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB!");
        return E_ERROR;
    } else {
        LOG_ERROR("DataShareResultSet::GetString is failed!");
        return E_ERROR;
    }
}

int DataShareResultSet::GetInt(int columnIndex, int &value)
{
    AppDataFwk::SharedBlock::CellUnit *cellUnit = sharedBlock_->GetCellUnit(rowPos_ - startRowPos_, columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::GetInt cellUnit is null!");
        return E_ERROR;
    }
    value = (int)cellUnit->cell.longValue;
    return E_OK;
}

int DataShareResultSet::GetLong(int columnIndex, int64_t &value)
{
    int errorCode = CheckState(columnIndex);
    if (errorCode != E_OK) {
        return errorCode;
    }
    AppDataFwk::SharedBlock::CellUnit *cellUnit = sharedBlock_->GetCellUnit(rowPos_ - startRowPos_, columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::GetLong cellUnit is null!");
        return E_ERROR;
    }

    int type = cellUnit->type;

    if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_INTEGER) {
        value = cellUnit->cell.longValue;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_STRING) {
        size_t sizeIncludingNull;
        const char *tempValue = sharedBlock_->GetCellUnitValueString(cellUnit, &sizeIncludingNull);
        value = ((sizeIncludingNull > 1) && (tempValue != nullptr)) ? long(strtoll(tempValue, nullptr, 0)) : 0L;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_FLOAT) {
        value = (int64_t)cellUnit->cell.doubleValue;
        LOG_ERROR("DataShareResultSet::GetLong AppDataFwk::SharedBlock::CELL_UNIT_TYPE_FLOAT !");
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL) {
        LOG_ERROR("DataShareResultSet::GetLong AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL !");
        value = 0L;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB) {
        LOG_ERROR("DataShareResultSet::GetLong AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB !");
        value = 0L;
        return E_OK;
    } else {
        LOG_ERROR("DataShareResultSet::GetLong Nothing !");
        return E_INVALID_OBJECT_TYPE;
    }
}

int DataShareResultSet::GetDouble(int columnIndex, double &value)
{
    int errorCode = CheckState(columnIndex);
    if (errorCode != E_OK) {
        return errorCode;
    }
    AppDataFwk::SharedBlock::CellUnit *cellUnit = sharedBlock_->GetCellUnit(rowPos_ - startRowPos_, columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::GetDouble cellUnit is null!");
        return E_ERROR;
    }
    int type = cellUnit->type;
    if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_FLOAT) {
        value = cellUnit->cell.doubleValue;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_STRING) {
        size_t sizeIncludingNull;
        const char *tempValue = sharedBlock_->GetCellUnitValueString(cellUnit, &sizeIncludingNull);
        value = ((sizeIncludingNull > 1) && (tempValue != nullptr)) ? strtod(tempValue, nullptr) : 0.0;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_INTEGER) {
        value = cellUnit->cell.longValue;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL) {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL!");
        value = 0.0;
        return E_OK;
    } else if (type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB) {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::CELL_UNIT_TYPE_BLOB!");
        value = 0.0;
        return E_OK;
    } else {
        LOG_ERROR("DataShareResultSet::GetDouble AppDataFwk::SharedBlock::nothing !");
        value = 0.0;
        return E_INVALID_OBJECT_TYPE;
    }
}

int DataShareResultSet::IsColumnNull(int columnIndex, bool &isNull)
{
    int errorCode = CheckState(columnIndex);
    if (errorCode != E_OK) {
        return errorCode;
    }
    AppDataFwk::SharedBlock::CellUnit *cellUnit = sharedBlock_->GetCellUnit(rowPos_ - startRowPos_, columnIndex);
    if (!cellUnit) {
        LOG_ERROR("DataShareResultSet::IsColumnNull cellUnit is null!");
        return E_ERROR;
    }
    if (cellUnit->type == AppDataFwk::SharedBlock::CELL_UNIT_TYPE_NULL) {
        isNull = true;
        return E_OK;
    }
    isNull = false;
    return E_OK;
}

int DataShareResultSet::Close()
{
    DataShareAbsResultSet::Close();
    ClosedBlock();
    return E_OK;
}

/**
 * Allocates a new shared block to an {@link DataShareResultSet}
 */
void DataShareResultSet::SetBlock(AppDataFwk::SharedBlock *block)
{
    if (sharedBlock_ != block) {
        ClosedBlock();
        sharedBlock_ = block;
    }
}

/**
 * Checks whether an {@code DataShareResultSet} object contains shared blocks
 */
bool DataShareResultSet::HasBlock() const
{
    return sharedBlock_ != nullptr;
}

/**
 * Closes a shared block that is not empty in this {@code DataShareResultSet} object
 */
void DataShareResultSet::ClosedBlock()
{
    delete sharedBlock_;
    sharedBlock_ = nullptr;
}

void DataShareResultSet::ClearBlock()
{
    if (sharedBlock_ != nullptr) {
        sharedBlock_->Clear();
    }
}

void DataShareResultSet::Finalize()
{
    Close();
}

/**
 * Check current status
 */
int DataShareResultSet::CheckState(int columnIndex)
{
    if (sharedBlock_ == nullptr) {
        LOG_ERROR("DataShareResultSet::CheckState sharedBlock is null!");
        return E_ERROR;
    }
    int cnt = 0;
    GetColumnCount(cnt);
    if (columnIndex >= cnt || columnIndex < 0) {
        return E_INVALID_COLUMN_INDEX;
    }
    int rowCnt = 0;
    GetRowCount(rowCnt);
    if (rowPos_ < 0 || rowPos_ >= rowCnt) {
        return E_INVALID_STATEMENT;
    }
    return E_OK;
}

bool DataShareResultSet::Marshalling(MessageParcel &parcel)
{
    if (sharedBlock_ == nullptr) {
        LOG_ERROR("DataShareResultSet::Marshalling sharedBlock is null.");
        return false;
    }
    LOG_DEBUG("DataShareResultSet::Marshalling sharedBlock.");
    return sharedBlock_->WriteMessageParcel(parcel);
}

bool DataShareResultSet::Unmarshalling(MessageParcel &parcel)
{
    if (sharedBlock_ != nullptr) {
        return false;
    }
    int result = AppDataFwk::SharedBlock::ReadMessageParcel(parcel, sharedBlock_);
    if (result < 0) {
        LOG_ERROR("DataShareResultSet: create from parcel error is %{public}d.", result);
    }
    return true;
}
} // namespace DataShare
} // namespace OHOS
