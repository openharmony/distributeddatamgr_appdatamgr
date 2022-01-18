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

#include "js_ability.h"

#include "hilog/log.h"

#ifdef STANDARD_SYSTEM_ENABLE
#include "ability.h"
#else
#include "js_utils.h"
#endif

namespace OHOS {
namespace JsKit {
static const OHOS::HiviewDFX::HiLogLabel PREFIX_LABEL = { LOG_CORE, 0xD001650, "JOHOS_JsKit_Ability" };

#define LOG_DEBUG(...) ((void)OHOS::HiviewDFX::HiLog::Debug(PREFIX_LABEL, __VA_ARGS__))
#define LOG_INFO(...) ((void)OHOS::HiviewDFX::HiLog::Info(PREFIX_LABEL, __VA_ARGS__))
#define LOG_WARN(...) ((void)OHOS::HiviewDFX::HiLog::Warn(PREFIX_LABEL, __VA_ARGS__))
#define LOG_ERROR(...) ((void)OHOS::HiviewDFX::HiLog::Error(PREFIX_LABEL, __VA_ARGS__))

#ifdef STANDARD_SYSTEM_ENABLE
static AppExecFwk::Ability* GetAbility(napi_env env)
{
    napi_value global = nullptr;
    napi_status status = napi_get_global(env, &global);
    if (status != napi_ok || global == nullptr) {
        LOG_ERROR("Cannot get global instance for %{public}d", status);
        return nullptr;
    }
    
    napi_value abilityContext = nullptr;
    status = napi_get_named_property(env, global, "ability", &abilityContext);
    if (status != napi_ok || abilityContext == nullptr) {
        LOG_ERROR("Cannot get ability context for %{public}d", status);
        return nullptr;
    }
    
    AppExecFwk::Ability *ability = nullptr;
    status = napi_get_value_external(env, abilityContext, (void **)&ability);
    if (status != napi_ok || ability == nullptr) {
        LOG_ERROR("Get ability form property failed for %{public}d", status);
        return nullptr;
    }
    return ability;
}

std::string JSAbility::GetDatabaseDir(napi_env env)
{
    AppExecFwk::Ability* ability = GetAbility(env);
    if (ability == nullptr) {
        return std::string();
    }
    return ability->GetDatabaseDir();
}

std::string JSAbility::GetBundleName(napi_env env)
{
    AppExecFwk::Ability* ability = GetAbility(env);
    if (ability == nullptr) {
        return std::string();
    }
    return ability->GetBundleName();
}
#else
std::string JSAbility::GetDatabaseDir(napi_env env)
{
    napi_value global = nullptr;
    napi_status status = napi_get_global(env, &global);
    NAPI_ASSERT(env, status == napi_ok, "napi get global failed!");

    napi_value ohosPlugin = nullptr;
    status = napi_get_named_property(env, global, "ohosplugin", &ohosPlugin);
    NAPI_ASSERT(env, status == napi_ok, "napi get ohosplugin failed!");

    napi_value app = nullptr;
    status = napi_get_named_property(env, ohosPlugin, "app", &app);
    NAPI_ASSERT(env, status == napi_ok, "napi get app failed!");

    napi_value context = nullptr;
    status = napi_get_named_property(env, app, "context", &context);
    NAPI_ASSERT(env, status == napi_ok, "napi get context failed!");

    napi_value getDatabaseDirSync = nullptr;
    status = napi_get_named_property(env, context, "getDatabaseDirSync", &getDatabaseDirSync);
    NAPI_ASSERT(env, status == napi_ok, "napi get getDatabaseDirSync failed!");

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, getDatabaseDirSync, &valueType);
    NAPI_ASSERT(env, valueType == napi_function, "getDatabaseDirSync is not napi_function!");

    napi_value callbackResult = nullptr;
    status = napi_call_function(env, context, getDatabaseDirSync, 0, nullptr, &callbackResult);
    NAPI_ASSERT(env, status == napi_ok, "napi call getDatabaseDirSync failed!");

    std::string databaseDir = JSUtils::Convert2String(env, callbackResult, JSUtils::DEFAULT_BUF_SIZE);
    LOG_DEBUG("getDatabaseDirSync is %{public}s!", databaseDir.c_str());
    return databaseDir;
}
#endif
} // namespace JsKit
} // namespace OHOS