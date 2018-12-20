/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2018 (git@dga.me.uk)
 */
#include "Common.h"
#include "net/Rpc.h"
#include "net/NetRole.h"
#include "net/CNetData.h"

namespace dw {
RpcSender::RpcSender() : entity_(nullptr), net_data_(nullptr), logger_(nullptr), rpc_id_(0) {
}

void RpcSender::onAddToEntity(Entity& entity, RpcId rpc_id) {
    entity_ = &entity;
    net_data_ = entity.component<CNetData>();
    logger_ = &entity.log();
    rpc_id_ = rpc_id;
}

bool RpcSender::shouldShortCircuit(RpcType type) const {
    // TODO: Check this.
    assert(net_data_);
    if (type == RpcType::Server && net_data_->role() == NetRole::Authority) {
        return true;
    }
    if (type == RpcType::Client && net_data_->role() == NetRole::AuthoritativeProxy) {
        return true;
    }
    return false;
}

void RpcSender::sendServerRpcPayload(const OutputBitStream& payload) {
    assert(net_data_);
    net_data_->sendRpc(rpc_id_, RpcType::Server, payload.vec_data());
}

void RpcSender::sendClientRpcPayload(const OutputBitStream& payload) {
    assert(net_data_);
    if (net_data_->role() != NetRole::AuthoritativeProxy) {
        logger_->warn("Trying to send a client RPC from a non-authoritative proxy.");
    } else {
        net_data_->sendRpc(rpc_id_, RpcType::Client, payload.vec_data());
    }
}
}  // namespace dw
