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

#ifndef NATIVE_RDB_RDB_STORE_H
#define NATIVE_RDB_RDB_STORE_H

#include <memory>
#include <string>
#include <vector>

#include "abs_rdb_predicates.h"
#include "abs_shared_result_set.h"
#include "result_set.h"
#include "value_object.h"
#include "values_bucket.h"

namespace OHOS {
namespace NativeRdb {

enum class ConflictResolution {
    ON_CONFLICT_NONE = 0,
    ON_CONFLICT_ROLLBACK,
    ON_CONFLICT_ABORT,
    ON_CONFLICT_FAIL,
    ON_CONFLICT_IGNORE,
    ON_CONFLICT_REPLACE,
};

class RdbStoreObserver {
    virtual void OnChange(std::vector<std::string>& devices) = 0; // networkid
};

class RdbStore {
public:
    enum SyncMode {
        PUSH,
        PULL,
    };
    
    struct SyncOption {
        SyncMode mode;
        bool isBlock;
    };
    
    using SyncResult = std::map<std::string, int>; // networkId
    using SyncCallback = std::function<void(SyncResult&)>;
    
    enum SubscribeMode {
        LOCAL,
        REMOTE,
        LOCAL_AND_REMOTE,
    };
    
    struct SubscribeOption {
        SubscribeMode mode;
    };
    
    struct DropOption {
    };
    
    virtual ~RdbStore(){};
    virtual int Insert(int64_t &outRowId, const std::string &table, const ValuesBucket &initialValues) = 0;
    virtual int Replace(int64_t &outRowId, const std::string &table, const ValuesBucket &initialValues) = 0;
    virtual int InsertWithConflictResolution(int64_t &outRowId, const std::string &table,
        const ValuesBucket &initialValues,
        ConflictResolution conflictResolution = ConflictResolution::ON_CONFLICT_NONE) = 0;
    virtual int Update(int &changedRows, const std::string &table, const ValuesBucket &values,
        const std::string &whereClause = "",
        const std::vector<std::string> &whereArgs = std::vector<std::string>()) = 0;
    virtual int UpdateWithConflictResolution(int &changedRows, const std::string &table, const ValuesBucket &values,
        const std::string &whereClause = "", const std::vector<std::string> &whereArgs = std::vector<std::string>(),
        ConflictResolution conflictResolution = ConflictResolution::ON_CONFLICT_NONE) = 0;
    virtual int Delete(int &deletedRows, const std::string &table, const std::string &whereClause = "",
        const std::vector<std::string> &whereArgs = std::vector<std::string>()) = 0;
    virtual std::unique_ptr<AbsSharedResultSet> Query(int &errCode, bool distinct, const std::string &table,
        const std::vector<std::string> &columns, const std::string &selection = "",
        const std::vector<std::string> &selectionArgs = std::vector<std::string>(), const std::string &groupBy = "",
        const std::string &having = "", const std::string &orderBy = "", const std::string &limit = "") = 0;
    virtual std::unique_ptr<AbsSharedResultSet> QuerySql(
        const std::string &sql, const std::vector<std::string> &selectionArgs = std::vector<std::string>()) = 0;
    virtual std::unique_ptr<ResultSet> QueryByStep(
        const std::string &sql, const std::vector<std::string> &selectionArgs = std::vector<std::string>()) = 0;
    virtual int ExecuteSql(
        const std::string &sql, const std::vector<ValueObject> &bindArgs = std::vector<ValueObject>()) = 0;
    virtual int ExecuteAndGetLong(int64_t &outValue, const std::string &sql,
        const std::vector<ValueObject> &bindArgs = std::vector<ValueObject>()) = 0;
    virtual int ExecuteAndGetString(std::string &outValue, const std::string &sql,
        const std::vector<ValueObject> &bindArgs = std::vector<ValueObject>()) = 0;
    virtual int ExecuteForLastInsertedRowId(int64_t &outValue, const std::string &sql,
        const std::vector<ValueObject> &bindArgs = std::vector<ValueObject>()) = 0;
    virtual int ExecuteForChangedRowCount(int64_t &outValue, const std::string &sql,
        const std::vector<ValueObject> &bindArgs = std::vector<ValueObject>()) = 0;
    virtual int Backup(const std::string databasePath, const std::vector<uint8_t> destEncryptKey) = 0;
    virtual int Attach(
        const std::string &alias, const std::string &pathName, const std::vector<uint8_t> destEncryptKey) = 0;

    virtual int Count(int64_t &outValue, const AbsRdbPredicates &predicates) = 0;
    virtual std::unique_ptr<AbsSharedResultSet> Query(
        const AbsRdbPredicates &predicates, const std::vector<std::string> columns) = 0;
    virtual int Update(int &changedRows, const ValuesBucket &values, const AbsRdbPredicates &predicates) = 0;
    virtual int Delete(int &deletedRows, const AbsRdbPredicates &predicates) = 0;

    virtual int GetVersion(int &version) = 0;
    virtual int SetVersion(int version) = 0;
    virtual int BeginTransaction() = 0;
    virtual int RollBack() = 0;
    virtual int Commit() = 0;
    virtual int MarkAsCommit() = 0;
    virtual int EndTransaction() = 0;
    virtual bool IsInTransaction() = 0;
    virtual int ChangeEncryptKey(const std::vector<uint8_t> &newKey) = 0;
    virtual std::string GetPath() = 0;
    virtual bool IsHoldingConnection() = 0;
    virtual bool IsOpen() const = 0;
    virtual bool IsReadOnly() const = 0;
    virtual bool IsMemoryRdb() const = 0;
    virtual int ChangeDbFileForRestore(const std::string newPath, const std::string backupPath,
        const std::vector<uint8_t> &newKey) = 0;
    
    virtual bool SetDistributedTables(const std::vector<std::string>& tables) = 0;
    
    virtual bool Sync(SyncOption& option, AbsRdbPredicates& predicate, SyncCallback& callback) = 0;
    
    virtual bool Subscribe(SubscribeOption& option, RdbStoreObserver& observer) = 0;
    
    virtual bool UnSubscribe(SubscribeOption& option) = 0;
    
    // user must use UDID
    virtual bool DropDeviceData(std::vector<std::string>& devices, DropOption& option) = 0;
};

} // namespace NativeRdb
} // namespace OHOS
#endif
