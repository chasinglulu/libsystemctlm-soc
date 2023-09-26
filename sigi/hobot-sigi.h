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

#include "systemc.h"

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

#include "remote-port-tlm.h"
#include "remote-port-tlm-memory-master.h"
#include "remote-port-tlm-memory-slave.h"
#include "remote-port-tlm-wires.h"
#include "tlm-modules/wire-splitter.h"

class hobot_sigi
: public remoteport_tlm
{
private:

	remoteport_tlm_memory_master rp_axi_mmp0;
	remoteport_tlm_memory_master rp_axi_mmp1;
	remoteport_tlm_memory_master rp_axi_mmp2;
	remoteport_tlm_memory_master rp_axi_reserved;

	remoteport_tlm_memory_slave rp_axi_msp0;
	remoteport_tlm_memory_slave rp_axi_msp1;

	remoteport_tlm_wires rp_wires_in;
	remoteport_tlm_wires rp_wires_out;
	remoteport_tlm_wires rp_irq_out;

	sc_vector<remoteport_tlm_memory_master > rp_user_master;
	sc_vector<remoteport_tlm_memory_slave > rp_user_slave;

	/*
	 * In order to get Master-IDs right, we need to proxy all
	 * transactions and inject generic attributes with Master IDs.
	 */
	sc_vector<tlm_utils::simple_target_socket_tagged<hobot_sigi> > proxy_in;
	sc_vector<tlm_utils::simple_initiator_socket_tagged<hobot_sigi> > proxy_out;

	/*
	 * Proxies for friendly named resets.
	 */
	wire_splitter *resetn_splitter[4];

	virtual void b_transport(int id,
				 tlm::tlm_generic_payload& trans,
				 sc_time& delay);
	virtual unsigned int transport_dbg(int id,
					   tlm::tlm_generic_payload& trans);
public:

	tlm_utils::simple_initiator_socket<remoteport_tlm_memory_master> *s_axi_mmp[3];
	tlm_utils::simple_initiator_socket<remoteport_tlm_memory_master> *s_axi_reserved;

	tlm_utils::simple_target_socket_tagged<hobot_sigi> *s_axi_msp[2];


	sc_vector<sc_signal<bool> > h2d_irq;
	sc_vector<sc_signal<bool> > d2h_irq;

	sc_vector<sc_signal<bool> > resetn;

	/*
	 * User-defined ports.
	 */
	tlm_utils::simple_initiator_socket<remoteport_tlm_memory_master> *user_master[10];
	tlm_utils::simple_target_socket<remoteport_tlm_memory_slave> *user_slave[10];

	hobot_sigi(sc_core::sc_module_name name, const char *sk_descr,
			Iremoteport_tlm_sync *sync = NULL,
			bool blocking_socket = true);
	~hobot_sigi(void);
	void tie_off(void);
};
