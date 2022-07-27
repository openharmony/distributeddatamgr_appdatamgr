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

#ifndef NAPI_DATASHARE_HELPER_H
#define NAPI_DATASHARE_HELPER_H

#include "async_call.h"
#include "datashare_helper.h"
#include "data_share_common.h"
#include "napi_datashare_observer.h"

namespace OHOS {
namespace DataShare {
class NapiDataShareHelper {
public:
    static napi_value Napi_CreateDataShareHelper(napi_env env, napi_callback_info info);

    static napi_value Napi_OpenFile(napi_env env, napi_callback_info info);
    static napi_value Napi_On(napi_env env, napi_callback_info info);
    static napi_value Napi_Off(napi_env env, napi_callback_info info);
    static napi_value Napi_Insert(napi_env env, napi_callback_info info);
    static napi_value Napi_Delete(napi_env env, napi_callback_info info);
    static napi_value Napi_Query(napi_env env, napi_callback_info info);
    static napi_value Napi_Update(napi_env env, napi_callback_info info);
    static napi_value Napi_BatchInsert(napi_env env, napi_callback_info info);
    static napi_value Napi_GetType(napi_env env, napi_callback_info info);
    static napi_value Napi_GetFileTypes(napi_env env, napi_callback_info info);
    static napi_value Napi_NormalizeUri(napi_env env, napi_callback_info info);
    static napi_value Napi_DenormalizeUri(napi_env env, napi_callback_info info);
    static napi_value Napi_NotifyChange(napi_env env, napi_callback_info info);

    void ReleaseObserverMap();
private:
    static napi_value GetConstructor(napi_env env);
    static napi_value Initialize(napi_env env, napi_callback_info info);

    std::shared_ptr<DataShareHelper> datashareHelper_ = nullptr;
    std::map<std::string, sptr<NAPIDataShareObserver>> observerMap_;

    struct ContextInfo : public AsyncCall::Context {
        NapiDataShareHelper *proxy = nullptr;
        napi_status status = napi_generic_failure;
        int resultNumber = 0;
        std::shared_ptr<DataShareResultSet> resultObject = nullptr;
        std::string resultString = "";
        std::vector<std::string> resultStrArr;

        std::string uri;
        std::string mode;
        DataShareValuesBucket valueBucket;
        DataSharePredicates predicates;
        std::vector<std::string> columns;
        std::vector<DataShareValuesBucket> values;
        std::string mimeTypeFilter;

        ContextInfo() : Context(nullptr, nullptr) {};
        ContextInfo(InputAction input, OutputAction output) : Context(std::move(input), std::move(output)) {};
        virtual ~ContextInfo() {};

        napi_status operator()(napi_env env, size_t argc, napi_value *argv, napi_value self) override
        {
            NAPI_ASSERT_BASE(env, self != nullptr, "self is nullptr", napi_invalid_arg);
            NAPI_CALL_BASE(env, napi_unwrap(env, self, reinterpret_cast<void **>(&proxy)), napi_invalid_arg);
            NAPI_ASSERT_BASE(env, proxy != nullptr, "there is no native upload task", napi_invalid_arg);
            return Context::operator()(env, argc, argv, self);
        }
        napi_status operator()(napi_env env, napi_value *result) override
        {
            if (status != napi_ok) {
                return status;
            }
            return Context::operator()(env, result);
        }
    };
};
}  // namespace DataShare
}  // namespace OHOS
#endif /* NAPI_DATASHARE_HELPER_H */
