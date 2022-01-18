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
#ifndef NATIVE_RDB_RDB_CLIENT_H
#define NATIVE_RDB_RDB_CLIENT_H

#include <mutex>
#include <map>

#include <refbase.h>
#include <iremote_object.h>
#include "irdb_service.h"
#include "rdb_store_config.h"
#include "rdb_parcel.h"

namespace OHOS::DistributedKv {
class IKvStoreDataService;
class IRdbService;
}

namespace OHOS::NativeRdb {
class ServiceDeathRecipient;
class RdbClient {
public:
    using RdbServiceDeathCallback = std::function<void()>;

    static RdbClient &GetInstance();
    
    std::shared_ptr<OHOS::DistributedKv::IRdbStore> GetRdbStore(const OHOS::DistributedKv::RdbStoreParam& param);
    
    int RegisterRdbServiceDeathCallback(const std::string& storeName, RdbServiceDeathCallback& callback);

    int UnRegisterRdbServiceDeathCallback(const std::string& storeName);

    void OnRemoteDied();
    
private:
    RdbClient();

    ~RdbClient();
    
    sptr<OHOS::DistributedKv::IRdbService> GetRdbService();
    
    void ResetServiceHandle();
    
    void NotifyServiceDeath();
    
    void RegisterClientDeathRecipient(const std::string& bundleName);
    
    std::mutex mutex_;
    sptr<OHOS::DistributedKv::IKvStoreDataService> distributedDataMgr_;
    sptr<OHOS::DistributedKv::IRdbService> rdbService_;
    sptr<IRemoteObject> clientDeathObject_;

    std::map<std::string, RdbServiceDeathCallback> serviceDeathCallbacks_;
};
}
#endif
