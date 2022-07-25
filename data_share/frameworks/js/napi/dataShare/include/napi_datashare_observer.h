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

#ifndef NAPI_DATASHARE_OBSERVER_H
#define NAPI_DATASHARE_OBSERVER_H

#include <uv.h>
#include "data_ability_observer_stub.h"
#include "napi/native_common.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace DataShare {
class NAPIDataShareObserver : public AAFwk::DataAbilityObserverStub {
public:
    NAPIDataShareObserver(napi_env env, napi_value callback);
    virtual ~NAPIDataShareObserver();
    void OnChange() override;
private:
    struct ObserverWorker {
        const NAPIDataShareObserver *observer_ = nullptr;
        napi_env env_ = nullptr;
        ObserverWorker(const NAPIDataShareObserver *observerIn) : observer_(observerIn) {}
    };

    napi_env env_ = nullptr;
    napi_ref ref_ = nullptr;
    uv_loop_s *loop_ = nullptr;
};
}  // namespace DataShare
}  // namespace OHOS
#endif /* DATASHARE_COMMON_H */
