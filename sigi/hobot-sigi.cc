/* SPDX-License-Identifier: GPL-2.0
 *
 * Hobot SystemC/TLM-2.0 Sigi Wrapper.
 *
 * Copyright (C) 2023, Charleye<wangkart@aliyun.com>
 * All rights reserved.
 *
 * Borrowed xilinx-zynqmp code
 *
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <inttypes.h>

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

using namespace sc_core;
using namespace std;

extern "C" {
#include "safeio.h"
#include "remote-port-proto.h"
#include "remote-port-sk.h"
};
#include "hobot-sigi.h"
#include "tlm-extensions/genattr.h"
#include <sys/types.h>

hobot_sigi::hobot_sigi(sc_module_name name, const char *sk_descr,
				Iremoteport_tlm_sync *sync,
				bool blocking_socket)
	: remoteport_tlm(name, -1, sk_descr, sync, blocking_socket),
	  rp_axi_mmp0("rp_axi_mmp0"),
	  rp_axi_mmp1("rp_axi_mmp1"),
	  rp_axi_mmp2("rp_axi_mmp2"),
	  rp_axi_reserved("rp_axi_reserved"),
	  rp_axi_msp0("rp_axi_msp0"),
	  rp_axi_msp1("rp_axi_msp1"),
	  rp_wires_in("wires_in", 16, 0),
	  rp_wires_out("wires_out", 0, 4),
	  rp_irq_out("irq_out", 0, 164),
	  rp_user_master("rp_net_master", 10),
	  rp_user_slave("rp_net_slave", 10),
	  proxy_in("proxy_in", 9),
	  proxy_out("proxy_out", proxy_in.size()),
	  h2d_irq("h2d_irq", rp_wires_in.wires_in.size()),
	  d2h_irq("d2h_irq", rp_irq_out.wires_out.size()),
	  resetn("resetn", 4)
{
	unsigned int i;

	// Register with Remote-Port.
	register_dev(0, &rp_axi_mmp0);
	register_dev(1, &rp_axi_mmp1);
	register_dev(2, &rp_axi_mmp2);
	register_dev(3, &rp_axi_reserved);

	register_dev(9, &rp_axi_msp0);
	register_dev(10, &rp_axi_msp1);

	register_dev(12, &rp_wires_in);
	register_dev(13, &rp_wires_out);
	register_dev(14, &rp_irq_out);

	for (i = 0; i < rp_user_master.size(); i++) {
		register_dev(256 + i, &rp_user_master[i]);
		register_dev(256 + 10 + i, &rp_user_slave[i]);
	}
}

void hobot_sigi::tie_off(void)
{
	tlm_utils::simple_initiator_socket<hobot_sigi> *tieoff_sk;
	unsigned int i;

	remoteport_tlm::tie_off();

	for (i = 0; i < proxy_in.size(); i++) {
		if (proxy_in[i].size())
			continue;
		tieoff_sk = new tlm_utils::simple_initiator_socket<hobot_sigi>();
		tieoff_sk->bind(proxy_in[i]);
	}
}

hobot_sigi::~hobot_sigi(void)
{
}

// Modify the Master ID and pass through transactions.
void hobot_sigi::b_transport(int id,
				tlm::tlm_generic_payload& trans,
				sc_time &delay)
{
	// The lower 6 bits of the Master ID are controlled by PL logic.
	// Upper 7 bits are dictated by the PS.
	//
	// Bits [9:6] are the port index + 8.
	// Bits [12:10] are the TBU index.
#define MASTER_ID(tbu, id_9_6) ((tbu) << 10 | (id_9_6) << 6)
	static const uint32_t master_id[9] = {
		MASTER_ID(0, 8),
		MASTER_ID(0, 9),
		MASTER_ID(3, 10),
		MASTER_ID(4, 11),
		MASTER_ID(4, 12),
		MASTER_ID(5, 13),
		MASTER_ID(2, 14),
		MASTER_ID(0, 2), /* ACP. No TBU. AXI IDs? */
		MASTER_ID(0, 15), /* ACE. No TBU.  */
	};
	uint64_t mid;
	genattr_extension *genattr;

	trans.get_extension(genattr);
	if (!genattr) {
		genattr = new genattr_extension();
		trans.set_extension(genattr);
	}

	mid = genattr->get_master_id();
	/* PL Logic cannot control upper bits.  */
	mid &= (1ULL << 6) - 1;
	mid |= master_id[id];
	genattr->set_master_id(mid);

	proxy_out[id]->b_transport(trans, delay);
}

// Passthrough.
unsigned int hobot_sigi::transport_dbg(int id, tlm::tlm_generic_payload& trans) {
	return proxy_out[id]->transport_dbg(trans);
}