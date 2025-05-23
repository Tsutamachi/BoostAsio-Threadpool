#include "VerifyGrpcClient.h"
#include "defines.h"
#include "ConfigMgr.h"

VerifyGrpcClient::VerifyGrpcClient() {
    // 获取配置
    auto& gCfgMgr = ConfigMgr::Inst();
    std::string host = gCfgMgr["VarifyServer"]["Host"];
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    pool_.reset(new RPConPool(5, host, port));
}
