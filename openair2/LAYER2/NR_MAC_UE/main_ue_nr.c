/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/* \file main_ue_nr.c
 * \brief top init of Layer 2
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

//#include "defs.h"
#include "mac_proto.h"
#include "sdr/COMMON/common_lib.h"
//#undef MALLOC
#include "assertions.h"
#include "PHY/types.h"
#include "PHY/defs_UE.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_entity.h"
#include "executables/softmodem-common.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp.h"
#include <pthread.h>
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"

static NR_UE_MAC_INST_t *nr_ue_mac_inst; 

NR_UE_MAC_INST_t * nr_l2_init_ue(NR_UE_RRC_INST_t* rrc_inst) {

    //LOG_I(MAC, "[MAIN] MAC_INIT_GLOBAL_PARAM IN...\n");

    //LOG_I(MAC, "[MAIN] init UE MAC functions \n");
    
    //init mac here
    nr_ue_mac_inst = (NR_UE_MAC_INST_t *)calloc(sizeof(NR_UE_MAC_INST_t),NB_NR_UE_MAC_INST);

    for (int j=0;j<NB_NR_UE_MAC_INST;j++) {
	nr_ue_init_mac(j);
    }


    if (rrc_inst && rrc_inst->scell_group_config) {

      nr_rrc_mac_config_req_ue(0,0,0,NULL,NULL,NULL,rrc_inst->scell_group_config);
      AssertFatal(rlc_module_init(0) == 0, "%s: Could not initialize RLC layer\n", __FUNCTION__);
      if (IS_SOFTMODEM_NOS1){
        pdcp_layer_init();
        nr_DRB_preconfiguration(nr_ue_mac_inst->crnti);
      }
      // Allocate memory for ul_config_request in the mac instance. This is now a pointer and will
      // point to a list of structures (one for each UL slot) to store PUSCH scheduling parameters
      // received from UL DCI.
      if (nr_ue_mac_inst->scc) {
        NR_TDD_UL_DL_ConfigCommon_t *tdd = nr_ue_mac_inst->scc->tdd_UL_DL_ConfigurationCommon;
        int num_slots_ul = tdd ? tdd->pattern1.nrofUplinkSlots : nr_slots_per_frame[*nr_ue_mac_inst->scc->ssbSubcarrierSpacing];
        if (tdd && nr_ue_mac_inst->scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols > 0) {
          num_slots_ul++;
        }
        LOG_D(NR_MAC, "In %s: Initializing ul_config_request. num_slots_ul = %d\n", __FUNCTION__, num_slots_ul);
        nr_ue_mac_inst->ul_config_request = (fapi_nr_ul_config_request_t *)calloc(num_slots_ul, sizeof(fapi_nr_ul_config_request_t));
        for (int i=0; i<num_slots_ul; i++)
          pthread_mutex_init(&(nr_ue_mac_inst->ul_config_request[i].mutex_ul_config), NULL);
      }
    }
    else {
      LOG_I(MAC,"Running without CellGroupConfig\n");
      if (get_softmodem_params()->sl_mode == 0) {
        nr_rrc_mac_config_req_ue(0, 0, 0, NULL, NULL, NULL, NULL);
        if (get_softmodem_params()->sa == 1) {
          AssertFatal(rlc_module_init(0) == 0, "%s: Could not initialize RLC layer\n", __FUNCTION__);
        }
      } else if (get_softmodem_params()->sl_mode == 2) {
        module_id_t module_id = 0;
        int cc_idP = 0;
        uint32_t sourceL2Id, groupL2Id, destinationL2Id;
        sourceL2Id = get_softmodem_params()->node_number;
        destinationL2Id = 2; //UPDATE it after implementing the ProSeApp connection
        groupL2Id = 3; //UPDATE it after implementing the ProSeApp connection
        uint32_t directFrameNumber_r16 = 1025; // Set greater than the valid directFrameNumber range (0-1023)
        long slotIndex_r16 = 21; // Set greater than the valid slotIndex_r16 range
        nr_rrc_mac_config_req_ue_sl(module_id, cc_idP, &sourceL2Id, &destinationL2Id, &groupL2Id,
                                    rrc_inst->SL_Preconfiguration[0],
                                    directFrameNumber_r16, slotIndex_r16);
      } else {
        LOG_E(NR_MAC, "Need implementation.\n");
      }
    }

    return (nr_ue_mac_inst);
}

NR_UE_MAC_INST_t *get_mac_inst(module_id_t module_id){
    return &nr_ue_mac_inst[(int)module_id];
}
