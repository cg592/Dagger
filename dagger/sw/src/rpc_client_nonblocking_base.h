#ifndef _RPC_CLIENT_NBLOCK_BASE_H_
#define _RPC_CLIENT_NBLOCK_BASE_H_

#include "completion_queue.h"
#include "nic.h"
#include "rpc_header.h"
#include "tx_queue.h"

#include <vector>
#include <utility>

namespace frpc {


/// Non-blocking RPC client
/// Does not block the calling thread,
/// returns the result through an async CompletionQueue
///
class RpcClientNonBlock_Base {
public:
    RpcClientNonBlock_Base(const Nic* nic,
                      size_t nic_flow_id,
                      uint16_t client_id);
    virtual ~RpcClientNonBlock_Base();

    // Get bound completion queue
    CompletionQueue* get_completion_queue() const;

    // Forbid instantiation
    virtual void abstract_class() const =0;

#ifdef PROFILE_LATENCY
    void init_latency_profile(uint64_t* timestamp_send,
                              uint64_t* timestamp_recv);
#endif

protected:
    // client_id - a part of rpc_id
    uint16_t client_id_;

    // NIC
    const Nic* nic_;
    size_t nic_flow_id_;

    // Tx queue
    TxQueue tx_queue_;

    // rpc_id counter
    uint16_t rpc_id_cnt_;

#ifdef NIC_CCIP_DMA
    uint32_t current_batch_ptr;
    size_t batch_counter;
#endif

#ifdef PROFILE_LATENCY
    // Latency profiler
    uint64_t* lat_prof_timestamp;
#endif

private:
    // Completion queue
    std::unique_ptr<CompletionQueue> cq_;

};

}  // namespace frpc

#endif  // _RPC_CLIENT_NBLOCK_BASE_H_