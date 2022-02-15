/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "napi_rdb_predicates.h"

#include "js_logger.h"
#include "js_utils.h"

using namespace OHOS::NativeRdb;
using namespace OHOS::AppDataMgrJsKit;

namespace OHOS {
namespace RdbJsKit {
static __thread napi_ref constructor_ = nullptr;

void RdbPredicatesProxy::Init(napi_env env, napi_value exports)
{
    LOG_INFO("Init RdbPredicatesProxy");
    napi_property_descriptor descriptors[] = {
        DECLARE_NAPI_FUNCTION("equalTo", EqualTo),
        DECLARE_NAPI_FUNCTION("notEqualTo", NotEqualTo),
        DECLARE_NAPI_FUNCTION("beginWrap", BeginWrap),
        DECLARE_NAPI_FUNCTION("endWrap", EndWrap),
        DECLARE_NAPI_FUNCTION("or", Or),
        DECLARE_NAPI_FUNCTION("and", And),
        DECLARE_NAPI_FUNCTION("contains", Contains),
        DECLARE_NAPI_FUNCTION("beginsWith", BeginsWith),
        DECLARE_NAPI_FUNCTION("endsWith", EndsWith),
        DECLARE_NAPI_FUNCTION("isNull", IsNull),
        DECLARE_NAPI_FUNCTION("isNotNull", IsNotNull),
        DECLARE_NAPI_FUNCTION("like", Like),
        DECLARE_NAPI_FUNCTION("glob", Glob),
        DECLARE_NAPI_FUNCTION("between", Between),
        DECLARE_NAPI_FUNCTION("notBetween", NotBetween),
        DECLARE_NAPI_FUNCTION("greaterThan", GreaterThan),
        DECLARE_NAPI_FUNCTION("lessThan", LessThan),
        DECLARE_NAPI_FUNCTION("greaterThanOrEqualTo", GreaterThanOrEqualTo),
        DECLARE_NAPI_FUNCTION("lessThanOrEqualTo", LessThanOrEqualTo),
        DECLARE_NAPI_FUNCTION("orderByAsc", OrderByAsc),
        DECLARE_NAPI_FUNCTION("orderByDesc", OrderByDesc),
        DECLARE_NAPI_FUNCTION("distinct", Distinct),
        DECLARE_NAPI_FUNCTION("limitAs", Limit),
        DECLARE_NAPI_FUNCTION("offsetAs", Offset),
        DECLARE_NAPI_FUNCTION("groupBy", GroupBy),
        DECLARE_NAPI_FUNCTION("indexedBy", IndexedBy),
        DECLARE_NAPI_FUNCTION("in", In),
        DECLARE_NAPI_FUNCTION("notIn", NotIn),
        DECLARE_NAPI_FUNCTION("using", Using),
        DECLARE_NAPI_FUNCTION("leftOuterJoin", LeftOuterJoin),
        DECLARE_NAPI_FUNCTION("innerJoin", InnerJoin),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("clear", Clear),
        DECLARE_NAPI_FUNCTION("crossJoin", CrossJoin),
        DECLARE_NAPI_GETTER_SETTER("joinCount", GetJoinCount, SetJoinCount),
        DECLARE_NAPI_GETTER_SETTER("joinConditions", GetJoinConditions, SetJoinConditions),
        DECLARE_NAPI_GETTER_SETTER("joinNames", GetJoinTableNames, SetJoinTableNames),
        DECLARE_NAPI_GETTER_SETTER("joinTypes", GetJoinTypes, SetJoinTypes),
        DECLARE_NAPI_FUNCTION("inDevices", InDevices),
        DECLARE_NAPI_FUNCTION("inAllDevices", InAllDevices),
    };

    napi_value cons;
    NAPI_CALL_RETURN_VOID(env, napi_define_class(env, "RdbPredicates", NAPI_AUTO_LENGTH, New, nullptr,
                                   sizeof(descriptors) / sizeof(napi_property_descriptor), descriptors, &cons));

    NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, cons, 1, &constructor_));

    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, exports, "RdbPredicates", cons));
    LOG_DEBUG("Init RdbPredicatesProxy end");
}

napi_value RdbPredicatesProxy::New(napi_env env, napi_callback_info info)
{
    napi_value new_target;
    NAPI_CALL(env, napi_get_new_target(env, info, &new_target));
    bool is_constructor = (new_target != nullptr);

    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thiz;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thiz, nullptr));

    if (is_constructor) {
        napi_valuetype valueType;
        NAPI_CALL(env, napi_typeof(env, args[0], &valueType));
        NAPI_ASSERT(env, valueType == napi_string, "Table name should be a string.");
        std::string tableName = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
        auto *proxy = new RdbPredicatesProxy(tableName);
        proxy->env_ = env;
        NAPI_CALL(env, napi_wrap(env, thiz, proxy, RdbPredicatesProxy::Destructor, nullptr, &proxy->wrapper_));
        LOG_INFO("RdbPredicatesProxy::New constructor ref:%{public}p", proxy->wrapper_);
        return thiz;
    }

    argc = 1;
    napi_value argv[1] = { args[0] };

    napi_value cons;
    NAPI_CALL(env, napi_get_reference_value(env, constructor_, &cons));

    napi_value output;
    NAPI_CALL(env, napi_new_instance(env, cons, argc, argv, &output));

    return output;
}

napi_value RdbPredicatesProxy::NewInstance(napi_env env, std::shared_ptr<NativeRdb::RdbPredicates> value)
{
    napi_value cons;
    napi_status status = napi_get_reference_value(env, constructor_, &cons);
    if (status != napi_ok) {
        LOG_ERROR("RdbPredicatesProxy get constructor failed! napi_status:%{public}d!", status);
        return nullptr;
    }

    size_t argc = 1;
    napi_value args[1] = { JSUtils::Convert2JSValue(env, value->GetTableName()) };
    napi_value instance = nullptr;
    status = napi_new_instance(env, cons, argc, args, &instance);
    if (status != napi_ok) {
        LOG_ERROR("RdbPredicatesProxy napi_new_instance failed! napi_status:%{public}d!", status);
        return nullptr;
    }

    RdbPredicatesProxy *proxy = nullptr;
    status = napi_unwrap(env, instance, reinterpret_cast<void **>(&proxy));
    if (status != napi_ok) {
        LOG_ERROR("RdbPredicatesProxy native instance is nullptr! napi_status:%{public}d!", status);
        return instance;
    }
    proxy->predicates_ = std::move(value);
    return instance;
}

void RdbPredicatesProxy::Destructor(napi_env env, void *nativeObject, void *)
{
    RdbPredicatesProxy *proxy = static_cast<RdbPredicatesProxy *>(nativeObject);
    delete proxy;
}

RdbPredicatesProxy::~RdbPredicatesProxy()
{
    napi_delete_reference(env_, wrapper_);
}

RdbPredicatesProxy::RdbPredicatesProxy(std::string &tableName)
    : predicates_(std::make_shared<NativeRdb::RdbPredicates> (tableName)), env_(nullptr), wrapper_(nullptr)
{}

std::shared_ptr<NativeRdb::RdbPredicates> RdbPredicatesProxy::GetNativePredicates(
    napi_env env, napi_callback_info info)
{
    RdbPredicatesProxy *predicatesProxy = nullptr;
    napi_value thiz;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    napi_unwrap(env, thiz, reinterpret_cast<void **>(&predicatesProxy));
    return predicatesProxy->predicates_;
}

napi_value RdbPredicatesProxy::EqualTo(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::EqualTo Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);
    LOG_DEBUG("RdbPredicatesProxy::EqualTo {field=%{public}s, value=%{public}s}.", field.c_str(), value.c_str());

    GetNativePredicates(env, info)->EqualTo(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::NotEqualTo(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::NotEqualTo Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);
    LOG_DEBUG("RdbPredicatesProxy::NotEqualTo {field=%{public}s, value=%{public}s}.", field.c_str(), value.c_str());

    GetNativePredicates(env, info)->NotEqualTo(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::BeginWrap(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->BeginWrap();
    return thiz;
}

napi_value RdbPredicatesProxy::EndWrap(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->EndWrap();
    return thiz;
}

napi_value RdbPredicatesProxy::Or(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->Or();
    return thiz;
}

napi_value RdbPredicatesProxy::And(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->And();
    return thiz;
}

napi_value RdbPredicatesProxy::Contains(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::Contains Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->Contains(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::BeginsWith(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::BeginsWith Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->BeginsWith(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::EndsWith(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::EndsWith Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->EndsWith(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::IsNull(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::IsNull Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->IsNull(field);
    return thiz;
}

napi_value RdbPredicatesProxy::IsNotNull(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::IsNotNull Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->IsNotNull(field);
    return thiz;
}

napi_value RdbPredicatesProxy::Like(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::Like Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->Like(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::Glob(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::Glob Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->Glob(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::Between(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::Between Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string low = JSUtils::ConvertAny2String(env, args[1]);
    std::string high = JSUtils::ConvertAny2String(env, args[2]);
    LOG_DEBUG("RdbPredicatesProxy::Between {field=%{public}s, low=%{public}s, high=%{public}s}.", field.c_str(),
        low.c_str(), high.c_str());

    GetNativePredicates(env, info)->Between(field, low, high);
    return thiz;
}

napi_value RdbPredicatesProxy::NotBetween(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::NotBetween Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string low = JSUtils::ConvertAny2String(env, args[1]);
    std::string high = JSUtils::ConvertAny2String(env, args[2]);

    GetNativePredicates(env, info)->NotBetween(field, low, high);
    return thiz;
}

napi_value RdbPredicatesProxy::GreaterThan(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::GreaterThan Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->GreaterThan(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::LessThan(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::LessThan Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->LessThan(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::GreaterThanOrEqualTo(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::GreaterThanOrEqualTo Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->GreaterThanOrEqualTo(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::LessThanOrEqualTo(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::LessThanOrEqualTo Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::string value = JSUtils::ConvertAny2String(env, args[1]);

    GetNativePredicates(env, info)->LessThanOrEqualTo(field, value);
    return thiz;
}

napi_value RdbPredicatesProxy::OrderByAsc(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::OrderByAsc Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->OrderByAsc(field);
    return thiz;
}

napi_value RdbPredicatesProxy::OrderByDesc(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::OrderByDesc Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->OrderByDesc(field);
    return thiz;
}

napi_value RdbPredicatesProxy::Distinct(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->Distinct();
    return thiz;
}

napi_value RdbPredicatesProxy::Limit(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::Limit Invalid argvs!");
    int32_t limit = 0;
    napi_get_value_int32(env, args[0], &limit);
    GetNativePredicates(env, info)->Limit(limit);
    return thiz;
}

napi_value RdbPredicatesProxy::Offset(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::Offset Invalid argvs!");
    int32_t offset = 0;
    napi_get_value_int32(env, args[0], &offset);
    GetNativePredicates(env, info)->Offset(offset);
    return thiz;
}

napi_value RdbPredicatesProxy::GroupBy(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::GroupBy Invalid argvs!");
    std::vector<std::string> fields = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->GroupBy(fields);
    return thiz;
}

napi_value RdbPredicatesProxy::IndexedBy(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::IndexedBy Invalid argvs!");
    std::string indexName = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->IndexedBy(indexName);
    return thiz;
}

napi_value RdbPredicatesProxy::In(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::In Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::vector<std::string> values = JSUtils::Convert2StrVector(env, args[1], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->In(field, values);
    return thiz;
}

napi_value RdbPredicatesProxy::NotIn(napi_env env, napi_callback_info info)
{
    napi_value thiz;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc > 0, "RdbPredicatesProxy::NotIn Invalid argvs!");
    std::string field = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    std::vector<std::string> values = JSUtils::Convert2StrVector(env, args[1], JSUtils::DEFAULT_BUF_SIZE);

    GetNativePredicates(env, info)->NotIn(field, values);
    return thiz;
}

napi_value RdbPredicatesProxy::Using(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::Using Invalid argvs!");
    auto fields = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    GetNativePredicates(env, info)->Using(fields);
    return thiz;
}

napi_value RdbPredicatesProxy::LeftOuterJoin(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::LeftOuterJoin Invalid argvs!");
    std::string tablename = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("RdbPredicatesProxy::LeftOuterJoin tablename:%{public}s", tablename.c_str());
    GetNativePredicates(env, info)->LeftOuterJoin(tablename);
    return thiz;
}

napi_value RdbPredicatesProxy::InnerJoin(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::InnerJoin Invalid argvs!");
    std::string tablename = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("RdbPredicatesProxy::InnerJoin tablename:%{public}s", tablename.c_str());
    GetNativePredicates(env, info)->InnerJoin(tablename);
    return thiz;
}

napi_value RdbPredicatesProxy::On(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::On Invalid argvs!");
    auto clauses = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    GetNativePredicates(env, info)->On(clauses);
    return thiz;
}

napi_value RdbPredicatesProxy::Clear(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->Clear();
    return thiz;
}

napi_value RdbPredicatesProxy::CrossJoin(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::CrossJoin Invalid argvs!");
    std::string tablename = JSUtils::Convert2String(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("RdbPredicatesProxy::CrossJoin tablename:%{public}s", tablename.c_str());
    GetNativePredicates(env, info)->CrossJoin(tablename);
    return thiz;
}

napi_value RdbPredicatesProxy::GetJoinCount(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    int errCode = GetNativePredicates(env, info)->GetJoinCount();
    return JSUtils::Convert2JSValue(env, errCode);
}

napi_value RdbPredicatesProxy::SetJoinCount(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::SetJoinCount Invalid argvs!");
    int32_t joinCount = 0;
    napi_get_value_int32(env, args[0], &joinCount);
    GetNativePredicates(env, info)->SetJoinCount(joinCount);
    return thiz;
}

napi_value RdbPredicatesProxy::GetJoinTypes(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    auto joinTypes = GetNativePredicates(env, info)->GetJoinTypes();
    return JSUtils::Convert2JSValue(env, joinTypes);
}

napi_value RdbPredicatesProxy::GetJoinTableNames(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    auto joinTableNames = GetNativePredicates(env, info)->GetJoinTableNames();
    return JSUtils::Convert2JSValue(env, joinTableNames);
}

napi_value RdbPredicatesProxy::GetJoinConditions(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    auto joinConditions = GetNativePredicates(env, info)->GetJoinConditions();
    return JSUtils::Convert2JSValue(env, joinConditions);
}

napi_value RdbPredicatesProxy::SetJoinConditions(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::SetJoinConditions Invalid argvs!");
    auto joinConditions = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    GetNativePredicates(env, info)->SetJoinConditions(joinConditions);
    return thiz;
}

napi_value RdbPredicatesProxy::SetJoinTableNames(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::SetJoinTableNames Invalid argvs!");
    auto joinNames = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    GetNativePredicates(env, info)->SetJoinTableNames(joinNames);
    return thiz;
}

napi_value RdbPredicatesProxy::SetJoinTypes(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::SetJoinTypes Invalid argvs!");
    auto joinTypes = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    GetNativePredicates(env, info)->SetJoinTypes(joinTypes);
    return thiz;
}

std::shared_ptr<NativeRdb::RdbPredicates> RdbPredicatesProxy::GetPredicates() const
{
    return this->predicates_;
}

napi_value RdbPredicatesProxy::InDevices(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    size_t argc = 1;
    napi_value args[1] = {nullptr };
    napi_get_cb_info(env, info, &argc, args, &thiz, nullptr);
    NAPI_ASSERT(env, argc == 1, "RdbPredicatesProxy::InDevices Invalid args!");
    auto devices = JSUtils::Convert2StrVector(env, args[0], JSUtils::DEFAULT_BUF_SIZE);
    GetNativePredicates(env, info)->InDevices(devices);
    return thiz;
}

napi_value RdbPredicatesProxy::InAllDevices(napi_env env, napi_callback_info info)
{
    napi_value thiz = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thiz, nullptr);
    GetNativePredicates(env, info)->InAllDevices();
    return thiz;
}
} // namespace RdbJsKit
} // namespace OHOS
