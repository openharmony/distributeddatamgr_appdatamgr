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

#include "napi_rdb_store.h"

#include <cinttypes>

#include "js_logger.h"
#include "js_utils.h"
#include "napi_async_proxy.h"
#include "napi_rdb_predicates.h"
#include "napi_result_set.h"
#include "rdb_errno.h"
#include "securec.h"

using namespace OHOS::NativeRdb;
using namespace OHOS::AppDataMgrJsKit;
using OHOS::DistributedRdb::SubscribeMode;
using OHOS::DistributedRdb::SubscribeOption;
using OHOS::DistributedRdb::SyncResult;
using OHOS::DistributedRdb::SyncOption;

namespace OHOS {
namespace RdbJsKit {
class RdbStoreContext : public NapiAsyncProxy<RdbStoreContext>::AysncContext {
public:
    RdbStoreContext()
        : AysncContext(),
          tableName(""),
          whereClause(""),
          sql(""),
          predicatesProxy(nullptr),
          valuesBucket(nullptr),
          rowId(0)
    {
        valuesBucket = new ValuesBucket();
    }

    virtual ~RdbStoreContext()
    {
        auto *obj = reinterpret_cast<RdbStoreProxy *>(boundObj);
        if (obj != nullptr) {
            obj->Release(env);
        }
        delete valuesBucket;
    }

    void BindArgs(napi_env env, napi_value value);
    void JSNumber2NativeType(std::shared_ptr<OHOS::NativeRdb::RdbStore> &rdbStore);
    std::string device;
    std::string tableName;
    std::vector<std::string> tablesName;
    std::string whereClause;
    std::vector<std::string> whereArgs;
    std::vector<std::string> selectionArgs;
    std::string sql;
    RdbPredicatesProxy *predicatesProxy;
    std::vector<std::string> columns;
    ValuesBucket *valuesBucket;
    std::map<std::string, ValueObject> numberMaps;
    std::vector<ValueObject> bindArgs;
    uint64_t rowId;
    std::vector<uint8_t> newKey;
    std::unique_ptr<AbsSharedResultSet> resultSet;
    std::unique_ptr<ResultSet> resultSet_value;
    std::string aliasName;
    std::string pathName;
    std::string destName;
    std::string srcName;
    int32_t enumArg;
    DistributedRdb::SyncResult syncResult;
};

static __thread napi_ref constructor_ = nullptr;
void RdbStoreContext::BindArgs(napi_env env, napi_value arg)
{
    bindArgs.clear();
    uint32_t arrLen = 0;
    napi_get_array_length(env, arg, &arrLen);
    if (arrLen == 0) {
        return;
    }
    for (size_t i = 0; i < arrLen; ++i) {
        napi_value element;
        napi_get_element(env, arg, i, &element);
        napi_valuetype type;
        napi_typeof(env, element, &type);
        switch (type) {
            case napi_boolean: {
                    bool value;
                    napi_status status = napi_get_value_bool(env, element, &value);
                    if (status == napi_ok) {
                        bindArgs.push_back(ValueObject(value));
                    }
                } break;
            case napi_number: {
                    double value;
                    napi_status status = napi_get_value_double(env, element, &value);
                    if (status == napi_ok) {
                        bindArgs.push_back(ValueObject(value));
                    }
                } break;
            case napi_null:
                bindArgs.push_back(ValueObject());
                break;
            case napi_string:
                bindArgs.push_back(ValueObject(JSUtils::Convert2String(env, element, JSUtils::DEFAULT_BUF_SIZE)));
                break;
            case napi_object:
                bindArgs.push_back(ValueObject(JSUtils::Convert2U8Vector(env, element)));
                break;
            default:
                break;
        }
    }
}

void RdbStoreContext::JSNumber2NativeType(std::shared_ptr<OHOS::NativeRdb::RdbStore> &rdbStore)
{
    std::unique_ptr<ResultSet> result = rdbStore->QueryByStep(std::string("SELECT * FROM ") + tableName + " LIMIT 1");
    LOG_DEBUG("ValueBucket table:%{public}s", tableName.c_str());
    result->GoToFirstRow();
    for (std::map<std::string, ValueObject>::iterator it = numberMaps.begin(); it != numberMaps.end(); ++it) {
        int index = -1;
        result->GetColumnIndex(it->first, index);
        ColumnType columnType = ColumnType::TYPE_FLOAT;
        result->GetColumnType(index, columnType);
        double value;
        it->second.GetDouble(value);
        switch (columnType) {
            case ColumnType::TYPE_FLOAT:
                LOG_DEBUG("JSNumber2NativeType to key:%{public}s type:float", it->first.c_str());
                valuesBucket->PutDouble(it->first, value);
                break;
            case ColumnType::TYPE_INTEGER:
                LOG_DEBUG("JSNumber2NativeType to key:%{public}s type:integer", it->first.c_str());
                valuesBucket->PutLong(it->first, int64_t(value));
                break;
            default:
                LOG_DEBUG("JSNumber2NativeType to key:%{public}s type:%{public}d", it->first.c_str(), int(columnType));
                valuesBucket->PutDouble(it->first, value);
                break;
        }
    }
    result->Close();
    result = nullptr;
    numberMaps.clear();
}

RdbStoreProxy::RdbStoreProxy() {}

RdbStoreProxy::~RdbStoreProxy()
{
    LOG_DEBUG("RdbStoreProxy destructor");
}

void RdbStoreProxy::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        DECLARE_NAPI_FUNCTION("delete", Delete),
        DECLARE_NAPI_FUNCTION("update", Update),
        DECLARE_NAPI_FUNCTION("insert", Insert),
        DECLARE_NAPI_FUNCTION("querySql", QuerySql),
        DECLARE_NAPI_FUNCTION("query", Query),
        DECLARE_NAPI_FUNCTION("executeSql", ExecuteSql),
        DECLARE_NAPI_FUNCTION("replace", Replace),
        DECLARE_NAPI_FUNCTION("backup", Backup),
        DECLARE_NAPI_FUNCTION("count", Count),
        DECLARE_NAPI_FUNCTION("addAttach", Attach),
        DECLARE_NAPI_FUNCTION("beginTransaction", BeginTransaction),
        DECLARE_NAPI_FUNCTION("rollBack", RollBack),
        DECLARE_NAPI_FUNCTION("commit", Commit),
        DECLARE_NAPI_FUNCTION("queryByStep", QueryByStep),
        DECLARE_NAPI_GETTER_SETTER("version", GetVersion, SetVersion),
        DECLARE_NAPI_FUNCTION("markAsCommit", MarkAsCommit),
        DECLARE_NAPI_FUNCTION("endTransaction", EndTransaction),
        DECLARE_NAPI_FUNCTION("restore", ChangeDbFileForRestore),
        DECLARE_NAPI_FUNCTION("changeEncryptKey", ChangeEncryptKey),
        DECLARE_NAPI_GETTER("isInTransaction", IsInTransaction),
        DECLARE_NAPI_GETTER("isOpen", IsOpen),
        DECLARE_NAPI_GETTER("path", GetPath),
        DECLARE_NAPI_GETTER("isHoldingConnection", IsHoldingConnection),
        DECLARE_NAPI_GETTER("isReadOnly", IsReadOnly),
        DECLARE_NAPI_GETTER("isMemoryRdb", IsMemoryRdb),
        DECLARE_NAPI_FUNCTION("setDistributedTables", SetDistributedTables),
        DECLARE_NAPI_FUNCTION("obtainDistributedTableName", ObtainDistributedTableName),
        DECLARE_NAPI_FUNCTION("sync", Sync),
        DECLARE_NAPI_FUNCTION("on", OnEvent),
        DECLARE_NAPI_FUNCTION("off", OffEvent),
    };
    napi_value cons = nullptr;
    napi_define_class(env, "RdbStore", NAPI_AUTO_LENGTH, Initialize, nullptr,
        sizeof(descriptors) / sizeof(napi_property_descriptor), descriptors, &cons);

    napi_create_reference(env, cons, 1, &constructor_);
    LOG_DEBUG("Init RdbStoreProxy end");
}

napi_value RdbStoreProxy::Initialize(napi_env env, napi_callback_info info)
{
    napi_value self;
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &self, nullptr));
    auto finalize = [](napi_env env, void *data, void *hint) {
        RdbStoreProxy *proxy = reinterpret_cast<RdbStoreProxy *>(data);
        if (proxy->ref_ != nullptr) {
            napi_delete_reference(env, proxy->ref_);
            proxy->ref_ = nullptr;
        }
        delete proxy;
    };
    auto *proxy = new RdbStoreProxy();
    napi_status status = napi_wrap(env, self, proxy, finalize, nullptr, &proxy->ref_);
    if (status != napi_ok) {
        LOG_ERROR("RdbStoreProxy napi_wrap failed! code:%{public}d!", status);
        finalize(env, proxy, nullptr);
        return nullptr;
    }
    if (proxy->ref_ == nullptr) {
        napi_create_reference(env, self, 0, &proxy->ref_);
    }
    LOG_INFO("RdbStoreProxy constructor ref:%{public}p", proxy->ref_);
    return self;
}

napi_value RdbStoreProxy::NewInstance(napi_env env, std::shared_ptr<OHOS::NativeRdb::RdbStore> value)
{
    napi_value cons;
    napi_status status = napi_get_reference_value(env, constructor_, &cons);
    if (status != napi_ok) {
        LOG_ERROR("RdbStoreProxy get constructor failed! code:%{public}d!", status);
        return nullptr;
    }

    napi_value instance = nullptr;
    status = napi_new_instance(env, cons, 0, nullptr, &instance);
    if (status != napi_ok) {
        LOG_ERROR("RdbStoreProxy napi_new_instance failed! code:%{public}d!", status);
        return nullptr;
    }

    RdbStoreProxy *proxy = nullptr;
    status = napi_unwrap(env, instance, reinterpret_cast<void **>(&proxy));
    if (proxy == nullptr) {
        LOG_ERROR("RdbStoreProxy native instance is nullptr! code:%{public}d!", status);
        return instance;
    }
    proxy->rdbStore_ = std::move(value);
    return instance;
}

RdbStoreProxy *RdbStoreProxy::GetNativeInstance(napi_env env, napi_value self)
{
    RdbStoreProxy *proxy = nullptr;
    napi_status status = napi_unwrap(env, self, reinterpret_cast<void **>(&proxy));
    if (proxy == nullptr) {
        LOG_ERROR("RdbStoreProxy::GetNativePredicates native instance is nullptr! code:%{public}d!", status);
        return nullptr;
    }
    uint32_t count = 0;
    {
        std::lock_guard<std::mutex> lock(proxy->mutex_);
        status = napi_reference_ref(env, proxy->ref_, &count);
    }
    if (status != napi_ok) {
        LOG_ERROR("RdbStoreProxy::GetNativePredicates napi_reference_ref(%{public}p) failed! code:%{public}d!, "
            "count:%{public}u",
            proxy->ref_, status, count);
        return proxy;
    }
    return proxy;
}

void RdbStoreProxy::Release(napi_env env)
{
    uint32_t count = 0;
    napi_status status = napi_ok;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status = napi_reference_unref(env, ref_, &count);
    }

    if (status != napi_ok) {
        LOG_ERROR("RdbStoreProxy::Release napi_reference_unref(%{public}p) failed! code:%{public}d!, count:%{public}u",
            ref_, status, count);
    }
}

void ParseThis(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->boundObj = RdbStoreProxy::GetNativeInstance(env, arg);
    LOG_DEBUG("ParseThis is : %{public}p", asyncContext->boundObj);
}

void ParseTableName(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->tableName = JSUtils::Convert2String(env, arg, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("ParseTableName is : %{public}s", asyncContext->tableName.c_str());
}

void ParseDevice(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->device = JSUtils::Convert2String(env, arg, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("ParseDevice: %{public}s", asyncContext->device.c_str());
}

void ParseTablesName(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    uint32_t arrLen = 0;
    napi_get_array_length(env, arg, &arrLen);
    if (arrLen == 0) {
        return;
    }
    for (uint32_t i = 0; i < arrLen; ++i) {
        napi_value element;
        napi_get_element(env, arg, i, &element);
        napi_valuetype type;
        napi_typeof(env, element, &type);
        if (type == napi_string) {
            std::string table = JSUtils::Convert2String(env, element, JSUtils::DEFAULT_BUF_SIZE);
            LOG_INFO("ParseTablesName: %{public}s", table.c_str());
            asyncContext->tablesName.push_back(table);
        }
    }
}

void ParseEnumArg(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    napi_get_value_int32(env, arg, &asyncContext->enumArg);
}

void ParsePredicates(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    napi_unwrap(env, arg, reinterpret_cast<void **>(&asyncContext->predicatesProxy));
    asyncContext->tableName = asyncContext->predicatesProxy->GetPredicates()->GetTableName();
}

void ParseNewKey(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->newKey = JSUtils::Convert2U8Vector(env, arg);
    LOG_DEBUG("ParseNewKey is end");
}

void ParseDestName(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->destName = JSUtils::Convert2String(env, arg, E_EMPTY_FILE_NAME);
    LOG_DEBUG("DestName is : %{public}s", asyncContext->destName.c_str());
}

void ParseSrcName(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->srcName = JSUtils::Convert2String(env, arg, E_EMPTY_TABLE_NAME);
    LOG_DEBUG("ParseSrcName is : %{public}s", asyncContext->srcName.c_str());
}

void ParseColumns(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->columns = JSUtils::Convert2StrVector(env, arg, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("ParseColumns columns :%{public}zu", asyncContext->columns.size());
}

void ParseWhereClause(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->whereClause = JSUtils::Convert2String(env, arg, E_HAVING_CLAUSE_NOT_IN_GROUP_BY);
    LOG_DEBUG("ParseWhereClause is : %{public}s", asyncContext->whereClause.c_str());
}

void ParseAlias(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->aliasName = JSUtils::Convert2String(env, arg, E_EMPTY_TABLE_NAME);
    LOG_DEBUG("ParseAlias is : %{public}s", asyncContext->aliasName.c_str());
}

void ParsePath(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->pathName = JSUtils::Convert2String(env, arg, E_EMPTY_TABLE_NAME);
    LOG_DEBUG("ParsePath is : %{public}s", asyncContext->pathName.data());
}

void ParseWhereArgs(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->whereArgs = JSUtils::Convert2StrVector(env, arg, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("ParseWhereArgs is : %{public}zu", asyncContext->whereArgs.size());
}

void ParseSelectionArgs(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->selectionArgs = JSUtils::Convert2StrVector(env, arg, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("ParseSelectionArgs is : %{public}zu", asyncContext->selectionArgs.size());
}

void ParseSql(const napi_env &env, const napi_value &arg, RdbStoreContext *asyncContext)
{
    asyncContext->sql = JSUtils::Convert2String(env, arg, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("ParseSql is : %{public}s", asyncContext->sql.c_str());
}

void ParseValuesBucket(const napi_env &env, const napi_value &arg, RdbStoreContext *context)
{
    napi_value keys = 0;
    napi_get_property_names(env, arg, &keys);
    uint32_t arrLen = 0;
    napi_status status = napi_get_array_length(env, keys, &arrLen);
    if (status != napi_ok) {
        LOG_DEBUG("ValuesBucket errr");
        return;
    }
    LOG_DEBUG("ValuesBucket num:%{public}d ", arrLen);
    for (size_t i = 0; i < arrLen; ++i) {
        napi_value key;
        status = napi_get_element(env, keys, i, &key);
        std::string keyStr = JSUtils::Convert2String(env, key, JSUtils::DEFAULT_BUF_SIZE);
        napi_value value;
        napi_get_property(env, arg, key, &value);
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, value, &valueType);
        if (valueType == napi_string) {
            std::string valueString = JSUtils::Convert2String(env, value, JSUtils::DEFAULT_BUF_SIZE);
            context->valuesBucket->PutString(keyStr, valueString);
            LOG_DEBUG("ValueObject type:%{public}d, key:%{public}s, value:%{public}s", valueType, keyStr.c_str(),
                valueString.c_str());
        } else if (valueType == napi_number) {
            double valueNumber;
            napi_get_value_double(env, value, &valueNumber);
            context->numberMaps.insert(std::make_pair(keyStr, ValueObject(valueNumber)));
            LOG_DEBUG("ValueObject type:%{public}d, key:%{public}s, value:%{public}lf", valueType, keyStr.c_str(),
                valueNumber);
        } else if (valueType == napi_boolean) {
            bool valueBool = false;
            napi_get_value_bool(env, value, &valueBool);
            context->valuesBucket->PutBool(keyStr, valueBool);
            LOG_DEBUG("ValueObject type:%{public}d, key:%{public}s, value:%{public}d", valueType, keyStr.c_str(),
                valueBool);
        } else if (valueType == napi_null) {
            context->valuesBucket->PutNull(keyStr);
            LOG_DEBUG("ValueObject type:%{public}d, key:%{public}s, value:null", valueType, keyStr.c_str());
        } else if (valueType == napi_object) {
            context->valuesBucket->PutBlob(keyStr, JSUtils::Convert2U8Vector(env, value));
            LOG_DEBUG("ValueObject type:%{public}d, key:%{public}s, value:Uint8Array", valueType, keyStr.c_str());
        } else {
            LOG_WARN("valuesBucket error");
        }
    }
}

napi_value RdbStoreProxy::Insert(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseTableName);
    parsers.push_back(ParseValuesBucket);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Insert",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int64_t rowId = 0;
            LOG_DEBUG("Insert tableName :%{public}s", context->tableName.c_str());
            context->JSNumber2NativeType(obj->rdbStore_);
            int errCode = obj->rdbStore_->Insert(rowId, context->tableName, *(context->valuesBucket));
            context->rowId = rowId;
            LOG_DEBUG("Insert rowId :%{public}" PRIu64, context->rowId);
            return errCode;
        },
        [](RdbStoreContext *context, napi_value &output) {
            LOG_DEBUG("Insert rowId :%{public}" PRIu64, context->rowId);
            napi_status status = napi_create_int64(context->env, context->rowId, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Delete(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParsePredicates);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Delete",
        [](RdbStoreContext *context) {
            LOG_DEBUG("napi Delete predicates:%{public}s",
                context->predicatesProxy->GetPredicates()->ToString().c_str());
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int temp = 0;
            int errCode = obj->rdbStore_->Delete(temp, *(context->predicatesProxy->GetPredicates()));
            context->rowId = temp;
            LOG_DEBUG("napi Delete");
            return errCode;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_create_int64(context->env, context->rowId, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Update(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseValuesBucket);
    parsers.push_back(ParsePredicates);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Update",
        [](RdbStoreContext *context) {
            LOG_DEBUG("napi Update predicates:%{public}s",
                context->predicatesProxy->GetPredicates()->ToString().c_str());
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int temp = 0;
            context->JSNumber2NativeType(obj->rdbStore_);
            int errCode =
                obj->rdbStore_->Update(temp, *(context->valuesBucket), *(context->predicatesProxy->GetPredicates()));
            context->rowId = temp;
            return errCode;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_create_int64(context->env, context->rowId, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Query(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParsePredicates);
    parsers.push_back(ParseColumns);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Query",
        [](RdbStoreContext *context) {
            LOG_DEBUG("napi Query predicates:%{public}s",
                context->predicatesProxy->GetPredicates()->ToString().c_str());
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            context->resultSet = obj->rdbStore_->Query(*(context->predicatesProxy->GetPredicates()), context->columns);
            LOG_DEBUG("Query result is nullptr ? %{public}d", (context->resultSet == nullptr));
            return (context->resultSet != nullptr) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            output = ResultSetProxy::NewInstance(context->env,
                                                 std::shared_ptr<AbsSharedResultSet>(context->resultSet.release()));
            return (output != nullptr) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::QuerySql(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseSql);
    parsers.push_back(ParseSelectionArgs);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "QuerySql",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            context->resultSet = obj->rdbStore_->QuerySql(context->sql, context->selectionArgs);
            LOG_DEBUG("Queried Sql: %{public}s, result == null ? %{public}d", context->sql.c_str(),
                (context->resultSet != nullptr));
            return (context->resultSet != nullptr) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            output = ResultSetProxy::NewInstance(context->env,
                                                 std::shared_ptr<AbsSharedResultSet>(context->resultSet.release()));
            return (output != nullptr) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::ExecuteSql(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseSql);
    parsers.push_back(
        [](const napi_env &env, const napi_value &arg, RdbStoreContext *ctx) { ctx->BindArgs(env, arg); });
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "ExecuteSql",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = obj->rdbStore_->ExecuteSql(context->sql, context->bindArgs);
            LOG_DEBUG("Executed Sql:%{public}s", context->sql.c_str());
            return errCode;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Count(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParsePredicates);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Count",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            std::int64_t temp = 0;
            int errCode = obj->rdbStore_->Count(temp, *(context->predicatesProxy->GetPredicates()));
            context->rowId = temp;
            return errCode;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_create_int64(context->env, context->rowId, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Replace(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseTableName);
    parsers.push_back(ParseValuesBucket);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Replace",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int64_t rowId = 0;
            LOG_DEBUG("Replace tableName:%{public}s", context->tableName.c_str());
            context->JSNumber2NativeType(obj->rdbStore_);
            int errCode = obj->rdbStore_->Replace(rowId, context->tableName, *(context->valuesBucket));
            context->rowId = rowId;
            LOG_DEBUG("Replace rowId:%{public}" PRIu64, context->rowId);
            return errCode;
        },
        [](RdbStoreContext *context, napi_value &output) {
            LOG_DEBUG("Replace rowId :%{public}" PRIu64, context->rowId);
            napi_status status = napi_create_int64(context->env, context->rowId, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Backup(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseTableName);
    parsers.push_back(ParseNewKey);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Backup",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = obj->rdbStore_->Backup(context->tableName, context->newKey);
            LOG_DEBUG("RdbStoreProxy::Backup errCode is:%{public}d", errCode);
            return (errCode == E_OK) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Attach(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseAlias);
    parsers.push_back(ParsePath);
    parsers.push_back(ParseNewKey);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Attach",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = obj->rdbStore_->Attach(context->aliasName, context->pathName, context->newKey);
            LOG_ERROR("RdbStoreProxy::Attach errCode:%{public}d ", errCode);
            return (errCode != E_OK) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::IsHoldingConnection(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    bool out = rdbStoreProxy->rdbStore_->IsHoldingConnection();
    LOG_DEBUG("RdbStoreProxy::IsHoldingConnection out is : %{public}d", out);
    return JSUtils::Convert2JSValue(env, out);
}

napi_value RdbStoreProxy::IsReadOnly(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    bool out = rdbStoreProxy->rdbStore_->IsReadOnly();
    LOG_DEBUG("RdbStoreProxy::IsReadOnly out is : %{public}d", out);
    return JSUtils::Convert2JSValue(env, out);
}

napi_value RdbStoreProxy::IsMemoryRdb(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    bool out = rdbStoreProxy->rdbStore_->IsMemoryRdb();
    LOG_DEBUG("RdbStoreProxy::IsMemoryRdb out is : %{public}d", out);
    return JSUtils::Convert2JSValue(env, out);
}

napi_value RdbStoreProxy::GetPath(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    std::string path = rdbStoreProxy->rdbStore_->GetPath();
    LOG_DEBUG("RdbStoreProxy::GetPath path is : %{public}s", path.c_str());
    return JSUtils::Convert2JSValue(env, path);
}

napi_value RdbStoreProxy::BeginTransaction(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    proxy.ParseInputs(parsers, ParseThis);

    return proxy.DoAsyncWork(
        "BeginTransaction",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int out = obj->rdbStore_->BeginTransaction();
            return out;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::RollBack(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("RdbStoreProxy::Rollback on called.");
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Rollback",
        [](RdbStoreContext *context) {
                RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
                int errCode = obj->rdbStore_->RollBack();
                LOG_DEBUG("RdbStoreProxy::Rollback errCode is : %{public}d", errCode);
                return (errCode == E_OK) ? OK : ERR;
            },
        [](RdbStoreContext *context, napi_value &output) {
                napi_status status = napi_get_undefined(context->env, &output);
                return (status == napi_ok) ? OK : ERR;
            });
}

napi_value RdbStoreProxy::Commit(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("RdbStoreProxy::Commit on called.");
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Commit",
        [](RdbStoreContext *context) {
                RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
                int errCode = obj->rdbStore_->Commit();
                LOG_DEBUG("RdbStoreProxy::Commit errCode is : %{public}d", errCode);
                return (errCode == E_OK) ? OK : ERR;
            },
        [](RdbStoreContext *context, napi_value &output) {
                napi_status status = napi_get_undefined(context->env, &output);
                return (status == napi_ok) ? OK : ERR;
            });
}

napi_value RdbStoreProxy::QueryByStep(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseSql);
    parsers.push_back(ParseColumns);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "QueryByStep",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            context->resultSet_value = obj->rdbStore_->QueryByStep(context->sql, context->columns);
            return (context->resultSet_value != nullptr) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::IsInTransaction(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    bool out = rdbStoreProxy->rdbStore_->IsInTransaction();
    LOG_DEBUG("RdbStoreProxy::IsInTransaction out is : %{public}d", out);
    return JSUtils::Convert2JSValue(env, out);
}

napi_value RdbStoreProxy::IsOpen(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    bool out = rdbStoreProxy->rdbStore_->IsOpen();
    LOG_DEBUG("RdbStoreProxy::IsOpen out is : %{public}d", out);
    return JSUtils::Convert2JSValue(env, out);
}

napi_value RdbStoreProxy::GetVersion(napi_env env, napi_callback_info info)
{
    napi_value thisObj = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisObj, nullptr);
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thisObj);
    int32_t getVersion = 0;
    int out = rdbStoreProxy->rdbStore_->GetVersion(getVersion);
    LOG_DEBUG("RdbStoreProxy::GetVersion out is : %{public}d", out);
    return JSUtils::Convert2JSValue(env, out);
}

napi_value RdbStoreProxy::SetVersion(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbStoreProxy::SetVersion Invalid argvs!");
    RdbStoreProxy *rdbStoreProxy = GetNativeInstance(env, thiz);
    int32_t setVersion = 0;
    napi_get_value_int32(env, args[0], &setVersion);
    LOG_DEBUG("RdbStoreProxy::SetVersion setVersion is : %{public}d", setVersion);
    int out = rdbStoreProxy->rdbStore_->SetVersion(setVersion);
    LOG_DEBUG("RdbStoreProxy::SetVersion out is : %{public}d", out);
    return thiz;
}

napi_value RdbStoreProxy::MarkAsCommit(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "MarkAsCommit",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = obj->rdbStore_->MarkAsCommit();
            LOG_ERROR("RdbStoreProxy::MarkAsCommit errCode is: %{public}d", errCode);
            return (errCode == E_OK) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::EndTransaction(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "EndTranscation",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = obj->rdbStore_->EndTransaction();
            LOG_DEBUG("RdbStoreProxy::EndTransaction errCode is : %{public}d", errCode);
            return (errCode != E_OK) ? OK : ERR;
         },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::ChangeDbFileForRestore(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseDestName);
    parsers.push_back(ParseSrcName);
    parsers.push_back(ParseNewKey);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Restore",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = 0;
            errCode  = obj->rdbStore_->ChangeDbFileForRestore(context->destName, context->srcName, context->newKey);
            LOG_DEBUG("RdbStoreProxy::ChangeDbFileForRestore errCode is : %{public}d", errCode);
            return (errCode != E_OK) ? OK : ERR;
        },

        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::ChangeEncryptKey(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseNewKey);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "ChangeEncryptKey",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            int errCode = obj->rdbStore_->ChangeEncryptKey(context->newKey);
            LOG_DEBUG("RdbStoreProxy::ChangeEncryptKey errCode is : %{public}d", errCode);
            return (errCode == E_OK) ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::SetDistributedTables(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseTablesName);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "SetDistributedTables",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            bool res = obj->rdbStore_->SetDistributedTables(context->tablesName);
            LOG_DEBUG("RdbStoreProxy::SetDistributedTables is: %{public}d", res);
            return res ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_get_undefined(context->env, &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::ObtainDistributedTableName(napi_env env, napi_callback_info info)
{
    LOG_INFO("RdbStoreProxy::ObtainDistributedTableName");
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseDevice);
    parsers.push_back(ParseTableName);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "ObtainDistributedTableName",
        [](RdbStoreContext *context) {
            RdbStoreProxy *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            auto name = obj->rdbStore_->ObtainDistributedTableName(context->device, context->tableName);
            LOG_INFO("RdbStoreProxy::ObtainDistributedTableName: %{public}s", name.c_str());
            context->tableName = name;
            return name.empty() ? ERR : OK;
        },
        [](RdbStoreContext *context, napi_value &output) {
            napi_status status = napi_create_string_utf8(context->env, context->tableName.c_str(),
                                                         context->tableName.length(), &output);
            return (status == napi_ok) ? OK : ERR;
        });
}

napi_value RdbStoreProxy::Sync(napi_env env, napi_callback_info info)
{
    NapiAsyncProxy<RdbStoreContext> proxy;
    proxy.Init(env, info);
    std::vector<NapiAsyncProxy<RdbStoreContext>::InputParser> parsers;
    parsers.push_back(ParseEnumArg);
    parsers.push_back(ParsePredicates);
    proxy.ParseInputs(parsers, ParseThis);
    return proxy.DoAsyncWork(
        "Sync",
        [](RdbStoreContext *context) {
            auto *obj = reinterpret_cast<RdbStoreProxy *>(context->boundObj);
            SyncOption option;
            option.mode = static_cast<DistributedRdb::SyncMode>(context->enumArg);
            option.isBlock = true;
            bool res = obj->rdbStore_->Sync(option, *context->predicatesProxy->GetPredicates(),
                                            [context](const SyncResult& result) {
                                                context->syncResult = result;
                                            });
            LOG_INFO("RdbStoreProxy Sync: res=%{public}d", res);
            return res ? OK : ERR;
        },
        [](RdbStoreContext *context, napi_value &output) {
            output = JSUtils::Convert2JSValue(context->env, context->syncResult);
            return (output != nullptr) ? OK : ERR;
        });
}

void RdbStoreProxy::OnDataChangeEvent(napi_env env, size_t argc, napi_value* argv)
{
    napi_valuetype type;
    napi_typeof(env, argv[0], &type);
    if (type != napi_number) {
        LOG_ERROR("OnDataChangeEvent: first argument is not number");
        return;
    }
    int32_t mode = SubscribeMode::SUBSCRIBE_MODE_MAX;
    napi_get_value_int32(env, argv[0], &mode);
    if (mode < 0 || mode >= SubscribeMode::SUBSCRIBE_MODE_MAX) {
        LOG_ERROR("OnDataChangeEvent: first argument value is invalid");
        return;
    }
    LOG_INFO("OnDataChangeEvent: mode=%{public}d", mode);

    napi_typeof(env, argv[1], &type);
    if (type != napi_function) {
        LOG_ERROR("OnDataChangeEvent: second argument is not function");
        return;
    }

    std::lock_guard<std::mutex> lockGuard(mutex_);
    for (const auto& observer : observers_[mode]) {
        if (*observer == argv[1]) {
            LOG_ERROR("OnDataChangeEvent: duplicate subscribe");
            return;
        }
    }
    SubscribeOption option;
    option.mode = static_cast<SubscribeMode>(mode);
    auto observer = std::make_shared<NapiRdbStoreObserver>(env, argv[1]);
    if (!rdbStore_->Subscribe(option, observer.get())) {
        LOG_ERROR("OnDataChangeEvent: subscribe failed");
        return;
    }
    observers_[mode].push_back(observer);
    LOG_ERROR("OnDataChangeEvent: subscribe success");
}

void RdbStoreProxy::OffDataChangeEvent(napi_env env, size_t argc, napi_value* argv)
{
    napi_valuetype type;
    napi_typeof(env, argv[0], &type);
    if (type != napi_number) {
        LOG_ERROR("OffDataChangeEvent: first argument is not number");
        return;
    }
    int32_t mode = SubscribeMode::SUBSCRIBE_MODE_MAX;
    napi_get_value_int32(env, argv[0], &mode);
    if (mode < 0 || mode >= SubscribeMode::SUBSCRIBE_MODE_MAX) {
        LOG_ERROR("OffDataChangeEvent: first argument value is invalid");
        return;
    }
    LOG_INFO("OffDataChangeEvent: mode=%{public}d", mode);

    napi_typeof(env, argv[1], &type);
    if (type != napi_function) {
        LOG_ERROR("OffDataChangeEvent: second argument is not function");
        return;
    }

    SubscribeOption option;
    option.mode = static_cast<SubscribeMode>(mode);
    std::lock_guard<std::mutex> lockGuard(mutex_);
    for (auto it = observers_[mode].begin(); it != observers_[mode].end(); it++) {
        if (**it == argv[1]) {
            rdbStore_->UnSubscribe(option, it->get());
            observers_[mode].erase(it);
            LOG_INFO("OffDataChangeEvent: unsubscribe success");
            return;
        }
    }
    LOG_INFO("OffDataChangeEvent: not found");
}
napi_value RdbStoreProxy::OnEvent(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ON_EVENT_ARG_NUM;
    napi_value argv[MAX_ON_EVENT_ARG_NUM] {};
    napi_value self = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &self, nullptr) != napi_ok) {
        LOG_ERROR("RdbStoreProxy OnEvent: get args failed");
        return nullptr;
    }
    bool invalid_condition = argc < MIN_ON_EVENT_ARG_NUM || argc > MAX_ON_EVENT_ARG_NUM || self == nullptr;
    NAPI_ASSERT(env, !invalid_condition, "RdbStoreProxy OnEvent: invalid args");

    auto proxy = RdbStoreProxy::GetNativeInstance(env, self);
    NAPI_ASSERT(env, proxy != nullptr, "RdbStoreProxy OnEvent: invalid args");

    std::string event = JSUtils::Convert2String(env, argv[0]);
    if (event == "dataChange") {
        proxy->OnDataChangeEvent(env, argc - 1, argv + 1);
    }

    proxy->Release(env);
    return nullptr;
}

napi_value RdbStoreProxy::OffEvent(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ON_EVENT_ARG_NUM;
    napi_value argv[MAX_ON_EVENT_ARG_NUM] {};
    napi_value self = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &self, nullptr) != napi_ok) {
        LOG_ERROR("RdbStoreProxy OnEvent: get args failed");
        return nullptr;
    }
    bool invalid_condition = argc < MIN_ON_EVENT_ARG_NUM || argc > MAX_ON_EVENT_ARG_NUM || self == nullptr;
    NAPI_ASSERT(env, !invalid_condition, "RdbStoreProxy OffEvent: invalid args");

    auto proxy = RdbStoreProxy::GetNativeInstance(env, self);
    NAPI_ASSERT(env, proxy != nullptr, "RdbStoreProxy OffEvent: invalid args");

    std::string event = JSUtils::Convert2String(env, argv[0]);
    if (event == "dataChange") {
        proxy->OffDataChangeEvent(env, argc - 1, argv + 1);
    }

    proxy->Release(env);
    return nullptr;
}
} // namespace RdbJsKit
} // namespace OHOS
