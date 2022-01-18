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

#define LOG_TAG "RdbClient"

#include "rdb_client.h"

#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "system_ability_definition.h"

#include "logger.h"
#include "ikvstore_data_service.h"
#include "irdb_service.h"

using namespace OHOS::DistributedKv;
namespace OHOS::NativeRdb {
class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit ServiceDeathRecipient(RdbClient* owner) : owner_(owner) {}
    void OnRemoteDied(const wptr<IRemoteObject> &object) override
    {
        if (owner_ != nullptr) {}
        owner_->OnRemoteDied();
    }
private:
    RdbClient* owner_;
};

static sptr<IKvStoreDataService> GetDistributedDataManager()
{
    auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) {
        LOG_ERROR("get system ability manager failed");
        return nullptr;
    }
    LOG_INFO("get distributed data manager");
    auto remoteObject = manager->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    return iface_cast<IKvStoreDataService>(remoteObject);
}

static void LinkToDeath(const sptr<IRemoteObject>& remote)
{
    auto& rdbClient = RdbClient::GetInstance();
    sptr<ServiceDeathRecipient> deathRecipient = new(std::nothrow) ServiceDeathRecipient(&rdbClient);
    if (!remote->AddDeathRecipient(deathRecipient)) {
        LOG_ERROR("add death recipient failed");
    }
    LOG_INFO("success");
}

RdbClient::RdbClient()
{
    LOG_INFO("construct");
}

RdbClient::~RdbClient()
{
    LOG_INFO("deconstruct");
}

RdbClient& RdbClient::GetInstance()
{
    static RdbClient rdbClient;
    return rdbClient;
}

sptr<IRdbService> RdbClient::GetRdbService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (rdbService_ != nullptr) {
        return rdbService_;
    }
    
    if (distributedDataMgr_ == nullptr) {
        distributedDataMgr_ = GetDistributedDataManager();
    }
    if (distributedDataMgr_ == nullptr) {
        LOG_ERROR("get distributed data manager failed");
        return nullptr;
    }
    
    rdbService_ = distributedDataMgr_->GetRdbService();
    if (rdbService_ == nullptr) {
        LOG_ERROR("get rdb service failed");
        return nullptr;
    }
    LinkToDeath(rdbService_->AsObject());
    return rdbService_;
}

std::shared_ptr<IRdbStore> RdbClient::GetRdbStore(const RdbStoreParam& param)
{
    if (!param.IsValid()) {
        LOG_ERROR("param is invalid");
        return nullptr;
    }
    auto service = GetRdbService();
    if (service == nullptr) {
        return nullptr;
    }
    RegisterClientDeathRecipient(param.bundleName_);
    auto storeSptr = service->GetRdbStore(param);
    std::shared_ptr<IRdbStore> store(storeSptr.GetRefPtr(), [holder = storeSptr] (const auto* ptr) {});
    return store;
}

int RdbClient::RegisterRdbServiceDeathCallback(const std::string& storeName, RdbServiceDeathCallback& callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    serviceDeathCallbacks_.insert({storeName, callback});
    return 0;
}

int RdbClient::UnRegisterRdbServiceDeathCallback(const std::string& storeName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    serviceDeathCallbacks_.erase(storeName);
    return 0;
}

void RdbClient::OnRemoteDied()
{
    LOG_INFO("rdb service has dead!!");
    NotifyServiceDeath();
    ResetServiceHandle();
}

void RdbClient::ResetServiceHandle()
{
    LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    distributedDataMgr_ = nullptr;
    rdbService_ = nullptr;
    clientDeathObject_ = nullptr;
}

void RdbClient::NotifyServiceDeath()
{
    LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : serviceDeathCallbacks_) {
        if (entry.second != nullptr) {
            entry.second();
        }
    }
}

void RdbClient::RegisterClientDeathRecipient(const std::string& bundleName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (clientDeathObject_ != nullptr) {
        return;
    }
    if (rdbService_ != nullptr) {
        sptr<IRemoteObject> object = new(std::nothrow) RdbClientDeathRecipientStub();
        if (rdbService_->RegisterClientDeathRecipient(bundleName, object) != 0) {
            LOG_ERROR("register client death recipient failed");
        } else {
            clientDeathObject_ = object;
        }
    }
}
}
