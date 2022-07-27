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

#include "napi_datashare_helper.h"

#include "napi_common_util.h"
#include "datashare_helper.h"
#include "datashare_log.h"
#include "napi_base_context.h"
#include "napi_datashare_values_bucket.h"
#include "datashare_predicates_proxy.h"
#include "datashare_result_set_proxy.h"

using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace DataShare {
constexpr int MAX_ARGC = 6;

std::list<std::shared_ptr<DataShareHelper>> g_dataShareHelperList;

void UnwrapDataSharePredicates(DataSharePredicates &predicates, napi_env env, napi_value value)
{
    auto tempPredicates = DataSharePredicatesProxy::GetNativePredicates(env, value);
    if (tempPredicates == nullptr) {
        LOG_ERROR("UnwrapDataSharePredicates GetNativePredicates retval Marshalling failed.");
        return;
    }
    predicates = *tempPredicates;
}

bool UnwrapValuesBucketArrayFromJS(napi_env env, napi_value param, std::vector<DataShareValuesBucket> &value)
{
    LOG_INFO("UnwrapValuesBucketArrayFromJS in");
    uint32_t arraySize = 0;
    napi_value jsValue = nullptr;
    std::string strValue = "";

    if (!IsArrayForNapiValue(env, param, arraySize)) {
        LOG_INFO("IsArrayForNapiValue is false");
        return false;
    }

    value.clear();
    for (uint32_t i = 0; i < arraySize; i++) {
        jsValue = nullptr;
        if (napi_get_element(env, param, i, &jsValue) != napi_ok) {
            LOG_INFO("napi_get_element is false");
            return false;
        }

        DataShareValuesBucket valueBucket;
        valueBucket.Clear();
        GetValueBucketObject(valueBucket, env, jsValue);

        value.push_back(valueBucket);
    }
    LOG_INFO("UnwrapValuesBucketArrayFromJS out");
    return true;
}

std::vector<DataShareValuesBucket> GetValuesBucketArray(napi_env env, napi_value param)
{
    LOG_INFO("NapiValueObject in");
    std::vector<DataShareValuesBucket> result;
    UnwrapValuesBucketArrayFromJS(env, param, result);
    return result;
}

napi_value NapiDataShareHelper::Napi_CreateDataShareHelper(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_CreateDataShareHelper in");
    struct CreateContextInfo {
        napi_ref ref = nullptr;
    };
    auto ctxInfo = std::make_shared<CreateContextInfo>();
    auto input = [ctxInfo](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        LOG_DEBUG("CreateDataShareHelper parser to native params %{public}d", static_cast<int>(argc));
        NAPI_ASSERT_BASE(env, (argc > 1) && (argc < 4), " need 2 or 3 parameters!", napi_invalid_arg);
        napi_value helperProxy = nullptr;
        napi_status status = napi_new_instance(env, GetConstructor(env), argc, argv, &helperProxy);
        if ((helperProxy == nullptr) || (status != napi_ok)) {
            return napi_generic_failure;
        }
        napi_create_reference(env, helperProxy, 1, &(ctxInfo->ref));
        return napi_ok;
    };
    auto output = [ctxInfo](napi_env env, napi_value *result) -> napi_status {
        napi_status status = napi_get_reference_value(env, ctxInfo->ref, result);
        napi_delete_reference(env, ctxInfo->ref);
        return status;
    };
    auto context = std::make_shared<AsyncCall::Context>(input, output);
    AsyncCall asyncCall(env, info, context);
    return asyncCall.Call(env);
}

napi_value NapiDataShareHelper::GetConstructor(napi_env env)
{
    napi_value cons = nullptr;
    napi_property_descriptor clzDes[] = {
        DECLARE_NAPI_FUNCTION("openFile", Napi_OpenFile),
        DECLARE_NAPI_FUNCTION("on", Napi_On),
        DECLARE_NAPI_FUNCTION("off", Napi_Off),
        DECLARE_NAPI_FUNCTION("insert", Napi_Insert),
        DECLARE_NAPI_FUNCTION("delete", Napi_Delete),
        DECLARE_NAPI_FUNCTION("query", Napi_Query),
        DECLARE_NAPI_FUNCTION("update", Napi_Update),
        DECLARE_NAPI_FUNCTION("batchInsert", Napi_BatchInsert),
        DECLARE_NAPI_FUNCTION("getType", Napi_GetType),
        DECLARE_NAPI_FUNCTION("getFileTypes", Napi_GetFileTypes),
        DECLARE_NAPI_FUNCTION("normalizeUri", Napi_NormalizeUri),
        DECLARE_NAPI_FUNCTION("denormalizeUri", Napi_DenormalizeUri),
        DECLARE_NAPI_FUNCTION("notifyChange", Napi_NotifyChange),
    };
    NAPI_CALL(env, napi_define_class(env, "DataShareHelper", NAPI_AUTO_LENGTH, Initialize, nullptr,
        sizeof(clzDes) / sizeof(napi_property_descriptor), clzDes, &cons));
    g_dataShareHelperList.clear();
    return cons;
}

napi_value NapiDataShareHelper::Initialize(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Initialize in");
    napi_value self = nullptr;
    size_t argc = ARGS_MAX_COUNT;
    napi_value argv[ARGS_MAX_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &self, nullptr));
    NAPI_ASSERT(env, argc > 1, "Wrong number of arguments");

    auto *proxy = new NapiDataShareHelper();
    std::string strUri;
    bool isStageMode = false;
    napi_status status = AbilityRuntime::IsStageContext(env, argv[PARAM0], isStageMode);
    if (status != napi_ok || !isStageMode) {
        auto ability = OHOS::AbilityRuntime::GetCurrentAbility(env);
        strUri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
        NAPI_ASSERT(env, ability != nullptr, "DataShareHelperConstructor: failed to get native ability");
        LOG_INFO("FA Model: strUri = %{public}s", strUri.c_str());
        proxy->datashareHelper_ = DataShareHelper::Creator(ability->GetContext(), strUri);
    } else {
        auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[PARAM0]);
        strUri = DataShareJSUtils::Convert2String(env, argv[PARAM1]);
        NAPI_ASSERT(env, context != nullptr, "DataShareHelperConstructor: failed to get native context");
        LOG_INFO("Stage Model: strUri = %{public}s", strUri.c_str());
        proxy->datashareHelper_ = DataShareHelper::Creator(context, strUri);
    }
    NAPI_ASSERT(env, proxy->datashareHelper_ != nullptr, "proxy->datashareHelper_ is nullptr");
    g_dataShareHelperList.emplace_back(proxy->datashareHelper_);
    auto finalize = [](napi_env env, void * data, void * hint) {
        NapiDataShareHelper *proxy = reinterpret_cast<NapiDataShareHelper *>(data);
        if (proxy != nullptr) {
            proxy->ReleaseObserverMap();
            delete proxy;
        }
    };
    if (napi_wrap(env, self, proxy, finalize, nullptr, nullptr) != napi_ok) {
        finalize(env, proxy, nullptr);
        return nullptr;
    }
    return self;
}

void NapiDataShareHelper::ReleaseObserverMap()
{
    auto it = this->observerMap_.begin();
    while (it != this->observerMap_.end()) {
        it->second->DeleteReference();
    }
    this->observerMap_.clear();
}

napi_value NapiDataShareHelper::Napi_OpenFile(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_OpenFile in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 2 || argc == 3, " should 2 or 3 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        napi_typeof(env, argv[PARAM1], &valuetype);
        if (valuetype == napi_string) {
            context->mode = DataShareJSUtils::Convert2String(env, argv[PARAM1]);
            LOG_INFO("mode : %{public}s", context->mode.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_int32(env, context->resultNumber, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultNumber = context->proxy->datashareHelper_->OpenFile(uri, context->mode);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_Insert(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_Insert in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 2 || argc == 3, " should 2 or 3 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        context->valueBucket.Clear();
        GetValueBucketObject(context->valueBucket, env, argv[PARAM1]);
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_int32(env, context->resultNumber, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultNumber = context->proxy->datashareHelper_->Insert(uri, context->valueBucket);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_Delete(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_Delete in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 2 || argc == 3, " should 2 or 3 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        UnwrapDataSharePredicates(context->predicates, env, argv[PARAM1]);
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_int32(env, context->resultNumber, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultNumber = context->proxy->datashareHelper_->Delete(uri, context->predicates);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_Query(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_Query in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 3 || argc == 4, " should 3 or 4 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        UnwrapDataSharePredicates(context->predicates, env, argv[PARAM1]);

        context->columns = DataShareJSUtils::Convert2StrVector(env, argv[PARAM2], DataShareJSUtils::DEFAULT_BUF_SIZE);
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        *result = DataShareResultSetProxy::NewInstance(env, context->resultObject);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultObject =
                    context->proxy->datashareHelper_->Query(uri, context->predicates, context->columns);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_Update(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_Update in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 3 || argc == 4, " should 3 or 4 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        UnwrapDataSharePredicates(context->predicates, env, argv[PARAM1]);

        context->valueBucket.Clear();
        GetValueBucketObject(context->valueBucket, env, argv[PARAM2]);
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_int32(env, context->resultNumber, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultNumber =
                    context->proxy->datashareHelper_->Update(uri, context->predicates, context->valueBucket);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_BatchInsert(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_BatchInsert in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 2 || argc == 3, " should 2 or 3 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        context->values = GetValuesBucketArray(env, argv[PARAM1]);
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_int32(env, context->resultNumber, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultNumber = context->proxy->datashareHelper_->BatchInsert(uri, context->values);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_GetType(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_GetType in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 1 || argc == 2, " should 1 or 2 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_string_utf8(env, context->resultString.c_str(), NAPI_AUTO_LENGTH, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultString = context->proxy->datashareHelper_->GetType(uri);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_GetFileTypes(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_GetFileTypes in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 2 || argc == 3, " should 2 or 3 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }

        napi_typeof(env, argv[PARAM1], &valuetype);
        if (valuetype == napi_string) {
            context->mimeTypeFilter = DataShareJSUtils::Convert2String(env, argv[PARAM1]);
            LOG_INFO("mimeTypeFilter : %{public}s", context->mimeTypeFilter.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        *result = DataShareJSUtils::Convert2JSValue(env, context->resultStrArr);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->resultStrArr = context->proxy->datashareHelper_->GetFileTypes(uri, context->mimeTypeFilter);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_NormalizeUri(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_NormalizeUri in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 1 || argc == 2, " should 1 or 2 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_string_utf8(env, context->resultString.c_str(), NAPI_AUTO_LENGTH, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                Uri uriValue = context->proxy->datashareHelper_->NormalizeUri(uri);
                context->resultString = uriValue.ToString();
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_DenormalizeUri(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_DenormalizeUri in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 1 || argc == 2, " should 1 or 2 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_create_string_utf8(env, context->resultString.c_str(), NAPI_AUTO_LENGTH, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                Uri uriValue = context->proxy->datashareHelper_->DenormalizeUri(uri);
                context->resultString = uriValue.ToString();
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_NotifyChange(napi_env env, napi_callback_info info)
{
    LOG_DEBUG("Napi_NotifyChange in");
    auto context = std::make_shared<ContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        NAPI_ASSERT_BASE(env, argc == 1 || argc == 2, " should 1 or 2 parameters!", napi_invalid_arg);
        LOG_DEBUG("argc : %{public}d", static_cast<int>(argc));

        napi_valuetype valuetype = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valuetype);
        if (valuetype == napi_string) {
            context->uri = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
            LOG_INFO("uri : %{public}s", context->uri.c_str());
        } else {
            LOG_INFO("wrong type, should be napi_string");
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_get_null(env, result);
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        if (context->proxy->datashareHelper_ != nullptr) {
            if (!context->uri.empty()) {
                OHOS::Uri uri(context->uri);
                context->proxy->datashareHelper_->NotifyChange(uri);
                context->status = napi_ok;
            } else {
                LOG_ERROR("context->uri is empty");
            }
        } else {
            LOG_ERROR("dataShareHelper_ == nullptr");
        }
    };
    context->SetAction(std::move(input), std::move(output));
    AsyncCall asyncCall(env, info, std::dynamic_pointer_cast<AsyncCall::Context>(context));
    return asyncCall.Call(env, exec);
}

napi_value NapiDataShareHelper::Napi_On(napi_env env, napi_callback_info info)
{
    LOG_INFO("Napi_On in");
    napi_value self = nullptr;
    size_t argc = MAX_ARGC;
    napi_value argv[MAX_ARGC] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &self, nullptr));
    NAPI_ASSERT(env, argc == ARGS_THREE, "wrong count of args");

    NapiDataShareHelper *proxy = nullptr;
    NAPI_CALL_BASE(env, napi_unwrap(env, self, reinterpret_cast<void **>(&proxy)), nullptr);
    NAPI_ASSERT_BASE(env, proxy != nullptr, "there is no NapiDataShareHelper instance", nullptr);
    NAPI_ASSERT_BASE(env, proxy->datashareHelper_ != nullptr, "there is no DataShareHelper instance", nullptr);

    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[PARAM0], &valueType));
    if (valueType != napi_string) {
        LOG_ERROR("type is not string");
        return nullptr;
    }
    std::string type = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
    if (type != "dataChange") {
        LOG_ERROR("wrong register type : %{public}s", type.c_str());
        return nullptr;
    }

    NAPI_CALL(env, napi_typeof(env, argv[PARAM1], &valueType));
    NAPI_ASSERT_BASE(env, valueType == napi_string, "uri is not string", nullptr);
    std::string uri = DataShareJSUtils::Convert2String(env, argv[PARAM1]);

    NAPI_CALL(env, napi_typeof(env, argv[PARAM2], &valueType));
    NAPI_ASSERT_BASE(env, valueType == napi_function, "callback is not a function", nullptr);
    sptr<NAPIDataShareObserver> observer(new (std::nothrow) NAPIDataShareObserver(env, argv[PARAM2]));

    auto obs = proxy->observerMap_.find(uri);
    if (obs != proxy->observerMap_.end()) {
        proxy->datashareHelper_->UnregisterObserver(Uri(uri), obs->second);
        obs->second->DeleteReference();
        proxy->observerMap_.erase(uri);
    }
    proxy->datashareHelper_->RegisterObserver(Uri(uri), observer);
    proxy->observerMap_.emplace(uri, observer);

    return nullptr;
}

napi_value NapiDataShareHelper::Napi_Off(napi_env env, napi_callback_info info)
{
    LOG_INFO("Napi_Off in");
    napi_value self = nullptr;
    size_t argc = MAX_ARGC;
    napi_value argv[MAX_ARGC] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &self, nullptr));
    NAPI_ASSERT(env, argc == ARGS_TWO || argc == ARGS_THREE, "wrong count of args");

    NapiDataShareHelper *proxy = nullptr;
    NAPI_CALL_BASE(env, napi_unwrap(env, self, reinterpret_cast<void **>(&proxy)), nullptr);
    NAPI_ASSERT_BASE(env, proxy != nullptr, "there is no NapiDataShareHelper instance", nullptr);
    NAPI_ASSERT_BASE(env, proxy->datashareHelper_ != nullptr, "there is no DataShareHelper instance", nullptr);

    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[PARAM0], &valueType));
    if (valueType != napi_string) {
        LOG_ERROR("type is not string");
        return nullptr;
    }
    std::string type = DataShareJSUtils::Convert2String(env, argv[PARAM0]);
    if (type != "dataChange") {
        LOG_ERROR("wrong register type : %{public}s", type.c_str());
        return nullptr;
    }

    NAPI_CALL(env, napi_typeof(env, argv[PARAM1], &valueType));
    NAPI_ASSERT_BASE(env, valueType == napi_string, "uri is not string", nullptr);
    std::string uri = DataShareJSUtils::Convert2String(env, argv[PARAM1]);

    if (argc == ARGS_THREE) {
        NAPI_CALL(env, napi_typeof(env, argv[PARAM2], &valueType));
        NAPI_ASSERT_BASE(env, valueType == napi_function, "callback is not a function", nullptr);
    }

    auto obs = proxy->observerMap_.find(uri);
    if (obs != proxy->observerMap_.end()) {
        proxy->datashareHelper_->UnregisterObserver(Uri(uri), obs->second);
        obs->second->DeleteReference();
        proxy->observerMap_.erase(uri);
    } else {
        LOG_DEBUG("this uri hasn't been registered");
    }

    return nullptr;
}
}  // namespace DataShare
}  // namespace OHOS
