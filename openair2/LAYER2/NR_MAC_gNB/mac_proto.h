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

/*! \file mac_proto.h
 * \brief MAC functions prototypes for gNB
 * \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 */

#ifndef __LAYER2_NR_MAC_PROTO_H__
#define __LAYER2_NR_MAC_PROTO_H__

#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_TAG-Id.h"

void set_cset_offset(uint16_t);

void mac_top_init_gNB(ngran_node_t node_type);

void config_common(int Mod_idP,
                   int pdsch_AntennaPorts,
                   int pusch_AntennaPorts,
                   NR_ServingCellConfigCommon_t *scc);

int nr_mac_enable_ue_rrc_processing_timer(module_id_t Mod_idP,
                                          rnti_t rnti,
                                          NR_SubcarrierSpacing_t subcarrierSpacing,
                                          uint32_t rrc_reconfiguration_delay);

int rrc_mac_config_req_gNB(module_id_t Mod_idP,
                           rrc_pdsch_AntennaPorts_t pdsch_AntennaPorts,
                           int pusch_AntennaPorts,
                           int sib1_tda,
                           int minRXTXTIMEpdsch,
                           NR_ServingCellConfigCommon_t *scc,
                           NR_BCCH_BCH_Message_t *mib,
                           NR_BCCH_DL_SCH_Message_t *sib1,
                           int add_ue,
                           uint32_t rnti,
                           NR_CellGroupConfig_t *CellGroup);

void clear_nr_nfapi_information(gNB_MAC_INST * gNB, 
                                int CC_idP,
                                frame_t frameP, 
                                sub_frame_t subframeP);

void nr_mac_update_timers(module_id_t module_id,
                          frame_t frame,
                          sub_frame_t slot);

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP,
			       frame_t frame_rxP, sub_frame_t slot_rxP);

void schedule_nr_bwp_switch(module_id_t module_id,
                            frame_t frame,
                            sub_frame_t slot);

/* \brief main DL scheduler function. Calls a preprocessor to decide on
 * resource allocation, then "post-processes" resource allocation (nFAPI
 * messages, statistics, HARQ handling, CEs, ... */
void nr_schedule_ue_spec(module_id_t module_id,
                         frame_t frame,
                         sub_frame_t slot);

uint32_t schedule_control_sib1(module_id_t module_id,
                               int CC_id,
                               NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config,
                               int time_domain_allocation,
                               NR_pdsch_dmrs_t *dmrs_parms,
                               NR_tda_info_t *tda_info,
                               uint8_t candidate_idx,
                               uint16_t num_total_bytes);

/* \brief default FR1 DL preprocessor init routine, returns preprocessor to call */
nr_pp_impl_dl nr_init_fr1_dlsch_preprocessor(module_id_t module_id, int CC_id);

void schedule_nr_sib1(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t slotP);

/* \brief main UL scheduler function. Calls a preprocessor to decide on
 * resource allocation, then "post-processes" resource allocation (nFAPI
 * messages, statistics, HARQ handling, ... */
void nr_schedule_ulsch(module_id_t module_id, frame_t frame, sub_frame_t slot);

/* \brief default FR1 UL preprocessor init routine, returns preprocessor to call */
nr_pp_impl_ul nr_init_fr1_ulsch_preprocessor(module_id_t module_id, int CC_id);

/////// Random Access MAC-PHY interface functions and primitives ///////

void nr_schedule_RA(module_id_t module_idP, frame_t frameP, sub_frame_t slotP);

/* \brief Function to indicate a received preamble on PRACH.  It initiates the RA procedure.
@param module_idP Instance ID of gNB
@param preamble_index index of the received RA request
@param slotP Slot number on which to act
@param timing_offset Offset in samples of the received PRACH w.r.t. eNB timing. This is used to
@param rnti RA rnti corresponding to this PRACH preamble
@param rach_resource type (0=non BL/CE,1 CE level 0,2 CE level 1, 3 CE level 2,4 CE level 3)
*/
void nr_initiate_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP,
                         uint16_t preamble_index, uint8_t freq_index, uint8_t symbol, int16_t timing_offset);

void nr_clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, NR_RA_t *ra);

int nr_allocate_CCEs(int module_idP, int CC_idP, frame_t frameP, sub_frame_t slotP, int test_only);

void nr_get_Msg3alloc(module_id_t module_id,
                      int CC_id,
                      NR_ServingCellConfigCommon_t *scc,
                      sub_frame_t current_subframe,
                      frame_t current_frame,
                      NR_RA_t *ra,
                      int16_t *tdd_beam_association);

void nr_generate_Msg3_retransmission(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra);

/* \brief Function in gNB to fill RAR pdu when requested by PHY.
@param ra Instance of RA resources of gNB
@param dlsch_buffer Pointer to RAR input buffer
@param N_RB_UL Number of UL resource blocks
*/
void nr_fill_rar(uint8_t Mod_idP,
                 NR_RA_t * ra,
                 uint8_t * dlsch_buffer,
                 nfapi_nr_pusch_pdu_t  *pusch_pdu);

void fill_msg3_pusch_pdu(nfapi_nr_pusch_pdu_t *pusch_pdu,
                         NR_ServingCellConfigCommon_t *scc,
                         int round,
                         int startSymbolAndLength,
                         rnti_t rnti, int scs,
                         int bwp_size, int bwp_start,
                         int mappingtype, int fh,
                         int msg3_first_rb, int msg3_nb_rb);


void schedule_nr_prach(module_id_t module_idP, frame_t frameP, sub_frame_t slotP);

uint16_t nr_mac_compute_RIV(uint16_t N_RB_DL, uint16_t RBstart, uint16_t Lcrbs);

/////// Phy test scheduler ///////

/* \brief preprocessor for phytest: schedules UE_id 0 with fixed MCS on all
 * freq resources */
void nr_preprocessor_phytest(module_id_t module_id,
                             frame_t frame,
                             sub_frame_t slot);
/* \brief UL preprocessor for phytest: schedules UE_id 0 with fixed MCS on a
 * fixed set of resources */
bool nr_ul_preprocessor_phytest(module_id_t module_id, frame_t frame, sub_frame_t slot);

void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   subframeP);

void handle_nr_uci_pucch_0_1(module_id_t mod_id,
                             frame_t frame,
                             sub_frame_t slot,
                             const nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_01);
void handle_nr_uci_pucch_2_3_4(module_id_t mod_id,
                               frame_t frame,
                               sub_frame_t slot,
                               const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_234);


void config_uldci(const NR_SIB1_t *sib1,
                  const NR_ServingCellConfigCommon_t *scc,
                  const nfapi_nr_pusch_pdu_t *pusch_pdu,
                  dci_pdu_rel15_t *dci_pdu_rel15,
                  int time_domain_assignment,
                  uint8_t tpc,
                  NR_UE_UL_BWP_t *ul_bwp);

void nr_schedule_pucch(gNB_MAC_INST *nrmac,
                       frame_t frameP,
                       sub_frame_t slotP);

void nr_schedule_srs(int module_id, frame_t frame);

void nr_csirs_scheduling(int Mod_idP,
                         frame_t frame,
                         sub_frame_t slot,
                         int n_slots_frame);

void nr_csi_meas_reporting(int Mod_idP,
                           frame_t frameP,
                           sub_frame_t slotP);

int nr_acknack_scheduling(int Mod_idP,
                          NR_UE_info_t *UE,
                          frame_t frameP,
                          sub_frame_t slotP,
                          int r_pucch,
                          int do_common);

void get_pdsch_to_harq_feedback(NR_PUCCH_Config_t *pucch_Config,
                                nr_dci_format_t dci_format,
                                int *max_fb_time,
                                uint8_t *pdsch_to_harq_feedback);
  
void nr_configure_css_dci_initial(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
                                  nr_scs_e scs_common,
                                  nr_scs_e pdcch_scs,
                                  frequency_range_t freq_range,
                                  uint8_t rmsi_pdcch_config,
                                  uint8_t ssb_idx,
                                  uint8_t k_ssb,
                                  uint16_t sfn_ssb,
                                  uint8_t n_ssb,
                                  uint16_t nb_slots_per_frame,
                                  uint16_t N_RB);
/*
int nr_is_dci_opportunity(nfapi_nr_search_space_t search_space,
                          nfapi_nr_coreset_t coreset,
                          uint16_t frame,
                          uint16_t slot,
                          nfapi_nr_config_request_scf_t cfg);
*/

int nr_get_pucch_resource(NR_ControlResourceSet_t *coreset,
                          NR_PUCCH_Config_t *pucch_Config,
                          int CCEIndex);

void nr_configure_pucch(nfapi_nr_pucch_pdu_t* pucch_pdu,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_UE_info_t* UE,
                        uint8_t pucch_resource,
                        uint16_t O_csi,
                        uint16_t O_ack,
                        uint8_t O_sr,
                        int r_pucch);

void find_search_space(int ss_type,
                       NR_BWP_Downlink_t *bwp,
                       NR_SearchSpace_t *ss);

void nr_configure_pdcch(nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu,
                        NR_ControlResourceSet_t *coreset,
                        bool is_sib1,
                        NR_sched_pdcch_t *pdcch);

NR_sched_pdcch_t set_pdcch_structure(gNB_MAC_INST *gNB_mac,
                                     NR_SearchSpace_t *ss,
                                     NR_ControlResourceSet_t *coreset,
                                     NR_ServingCellConfigCommon_t *scc,
                                     NR_BWP_t *bwp,
                                     NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config);

int find_pdcch_candidate(gNB_MAC_INST *mac,
                         int cc_id,
                         int aggregation,
                         int nr_of_candidates,
                         NR_sched_pdcch_t *pdcch,
                         NR_ControlResourceSet_t *coreset,
                         uint32_t Y);

void fill_pdcch_vrb_map(gNB_MAC_INST *mac,
                        int CC_id,
                        NR_sched_pdcch_t *pdcch,
                        int first_cce,
                        int aggregation);

void fill_dci_pdu_rel15(const NR_ServingCellConfigCommon_t *scc,
                        const NR_CellGroupConfig_t *CellGroup,
                        const NR_UE_DL_BWP_t *dl_bwp,
                        nfapi_nr_dl_dci_pdu_t *pdcch_dci_pdu,
                        dci_pdu_rel15_t *dci_pdu_rel15,
                        int dci_formats,
                        int rnti_types,
                        int N_RB,
                        int bwp_id,
                        NR_ControlResourceSet_t *coreset,
                        uint16_t cset0_bwp_size);

void prepare_dci(const NR_CellGroupConfig_t *CellGroup,
                 const NR_UE_DL_BWP_t *dl_bwp,
                 const NR_ControlResourceSet_t *coreset,
                 dci_pdu_rel15_t *dci_pdu_rel15,
                 nr_dci_format_t format);

void set_r_pucch_parms(int rsetindex,
                       int r_pucch,
                       int bwp_size,
                       int *prb_start,
                       int *second_hop_prb,
                       int *nr_of_symbols,
                       int *start_symbol_index);

/* find coreset within the search space */
NR_ControlResourceSet_t *get_coreset(gNB_MAC_INST *nrmac,
                                     NR_ServingCellConfigCommon_t *scc,
                                     void *bwp,
                                     NR_SearchSpace_t *ss,
                                     NR_SearchSpace__searchSpaceType_PR ss_type);

/* find a search space within a BWP */
NR_SearchSpace_t *get_searchspace(NR_ServingCellConfigCommon_t *scc,
                                  NR_BWP_DownlinkDedicated_t *bwp_Dedicated,
                                  NR_SearchSpace__searchSpaceType_PR target_ss);

long get_K2(NR_PUSCH_TimeDomainResourceAllocationList_t *tdaList,
            int time_domain_assignment,
            int mu);

NR_tda_info_t nr_get_pdsch_tda_info(const NR_UE_DL_BWP_t *dl_bwp,
                                    const int tda);

NR_tda_info_t nr_get_pusch_tda_info(const NR_UE_UL_BWP_t *ul_bwp,
                                    const int tda);

NR_pusch_dmrs_t get_ul_dmrs_params(const NR_ServingCellConfigCommon_t *scc,
                                   const NR_UE_UL_BWP_t *ul_bwp,
                                   const NR_tda_info_t *tda_info,
                                   const int Layers);

uint8_t nr_get_tpc(int target, uint8_t cqi, int incr);

int get_spf(nfapi_nr_config_request_scf_t *cfg);

int to_absslot(nfapi_nr_config_request_scf_t *cfg,int frame,int slot);

int get_nrofHARQ_ProcessesForPDSCH(e_NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH n);

void nr_get_tbs_dl(nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
		   int x_overhead,
                   uint8_t numdmrscdmgroupnodata,
                   uint8_t tb_scaling);

int NRRIV2BW(int locationAndBandwidth,int N_RB);

int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB);

/* Functions to manage an NR_list_t */
void create_nr_list(NR_list_t *listP, int len);
void resize_nr_list(NR_list_t *list, int new_len);
void destroy_nr_list(NR_list_t *list);
void add_nr_list(NR_list_t *listP, int id);
void remove_nr_list(NR_list_t *listP, int id);
void add_tail_nr_list(NR_list_t *listP, int id);
void add_front_nr_list(NR_list_t *listP, int id);
void remove_front_nr_list(NR_list_t *listP);

NR_UE_info_t * find_nr_UE(NR_UEs_t* UEs, rnti_t rntiP);

int find_nr_RA_id(module_id_t mod_idP, int CC_idP, rnti_t rntiP);

void configure_UE_BWP(gNB_MAC_INST *nr_mac,
                      NR_ServingCellConfigCommon_t *scc,
                      NR_UE_sched_ctrl_t *sched_ctrl,
                      NR_RA_t *ra,
                      NR_UE_info_t *UE);

NR_UE_info_t* add_new_nr_ue(gNB_MAC_INST *nr_mac, rnti_t rntiP, NR_CellGroupConfig_t *CellGroup);

void mac_remove_nr_ue(gNB_MAC_INST *nr_mac, rnti_t rnti);

void nr_mac_remove_ra_rnti(module_id_t mod_id, rnti_t rnti);

int nr_get_default_pucch_res(int pucch_ResourceCommon);

int get_dlscs(nfapi_nr_config_request_t *cfg);

int get_ulscs(nfapi_nr_config_request_t *cfg);

int get_symbolsperslot(nfapi_nr_config_request_t *cfg);

int nr_write_ce_dlsch_pdu(module_id_t module_idP,
                          const NR_UE_sched_ctrl_t *ue_sched_ctl,
                          unsigned char *mac_pdu,
                          unsigned char drx_cmd,
                          unsigned char *ue_cont_res_id);

void nr_generate_Msg2(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra);

void nr_generate_Msg4(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra);

void nr_check_Msg4_Ack(module_id_t module_id, int CC_id, frame_t frame, sub_frame_t slot, NR_RA_t *ra);

int binomial(int n, int k);

bool is_xlsch_in_slot(uint64_t bitmap, sub_frame_t slot);

void fill_ssb_vrb_map (NR_COMMON_channels_t *cc, int rbStart, int ssb_subcarrier_offset, uint16_t symStart, int CC_id);


/* \brief Function to indicate a received SDU on ULSCH.
@param Mod_id Instance ID of gNB
@param CC_id Component carrier index
@param rnti RNTI of UE transmitting the SDU
@param sdu Pointer to received SDU
@param sdu_len Length of SDU
@param timing_advance timing advance adjustment after this pdu
@param ul_cqi Uplink CQI estimate after this pdu (SNR quantized to 8 bits, -64 ... 63.5 dB in .5dB steps)
*/
void nr_rx_sdu(const module_id_t gnb_mod_idP,
               const int CC_idP,
               const frame_t frameP,
               const sub_frame_t subframeP,
               const rnti_t rntiP,
               uint8_t * sduP,
               const uint16_t sdu_lenP,
               const uint16_t timing_advance,
               const uint8_t ul_cqi,
               const uint16_t rssi);

void create_dl_harq_list(NR_UE_sched_ctrl_t *sched_ctrl,
                         const NR_PDSCH_ServingCellConfig_t *pdsch);

void reset_dl_harq_list(NR_UE_sched_ctrl_t *sched_ctrl);

void reset_ul_harq_list(NR_UE_sched_ctrl_t *sched_ctrl);

void handle_nr_ul_harq(const int CC_idP,
                       module_id_t mod_id,
                       frame_t frame,
                       sub_frame_t slot,
                       const nfapi_nr_crc_t *crc_pdu);

void handle_nr_srs_measurements(const module_id_t module_id,
                                const frame_t frame,
                                const sub_frame_t slot,
                                const rnti_t rnti,
                                const uint16_t timing_advance,
                                const uint8_t num_symbols,
                                const uint8_t wide_band_snr,
                                const uint8_t num_reported_symbols,
                                nfapi_nr_srs_indication_reported_symbol_t* reported_symbol_list);

int16_t ssb_index_from_prach(module_id_t module_idP,
                             frame_t frameP,
                             sub_frame_t slotP,
                             uint16_t preamble_index,
                             uint8_t freq_index,
                             uint8_t symbol);

void find_SSB_and_RO_available(module_id_t module_idP);

NR_pdsch_dmrs_t get_dl_dmrs_params(const NR_ServingCellConfigCommon_t *scc,
                                   const NR_UE_DL_BWP_t *BWP,
                                   const NR_tda_info_t *tda_info,
                                   const int Layers);

uint16_t get_pm_index(const NR_UE_info_t *UE,
                      int layers,
                      int xp_pdsch_antenna_ports);

uint8_t get_mcs_from_cqi(int mcs_table, int cqi_table, int cqi_idx);

uint8_t get_dl_nrOfLayers(const NR_UE_sched_ctrl_t *sched_ctrl, const nr_dci_format_t dci_format);

const int get_dl_tda(const gNB_MAC_INST *nrmac, const NR_ServingCellConfigCommon_t *scc, int slot);
const int get_ul_tda(const gNB_MAC_INST *nrmac, const NR_ServingCellConfigCommon_t *scc, int slot);

bool find_free_CCE(sub_frame_t slot, NR_UE_info_t *UE);

bool nr_find_nb_rb(uint16_t Qm,
                   uint16_t R,
                   uint8_t nrOfLayers,
                   uint16_t nb_symb_sch,
                   uint16_t nb_dmrs_prb,
                   uint32_t bytes,
                   uint16_t nb_rb_min,
                   uint16_t nb_rb_max,
                   uint32_t *tbs,
                   uint16_t *nb_rb);

int get_mcs_from_bler(const NR_bler_options_t *bler_options,
                      const NR_mac_dir_stats_t *stats,
                      NR_bler_stats_t *bler_stats,
                      int max_mcs,
                      frame_t frame);

void UL_tti_req_ahead_initialization(gNB_MAC_INST * gNB, NR_ServingCellConfigCommon_t *scc, int n, int CCid);

void nr_sr_reporting(gNB_MAC_INST *nrmac, frame_t frameP, sub_frame_t slotP);

size_t dump_mac_stats(gNB_MAC_INST *gNB, char *output, size_t strlen, bool reset_rsrp);

void process_CellGroup(NR_CellGroupConfig_t *CellGroup, NR_UE_sched_ctrl_t *sched_ctrl);

void send_initial_ul_rrc_message(module_id_t        module_id,
                                 int                CC_id,
                                 const NR_UE_info_t *UE,
                                 rb_id_t            srb_id,
                                 const uint8_t      *sdu,
                                 sdu_size_t         sdu_len);

void abort_nr_dl_harq(NR_UE_info_t* UE, int8_t harq_pid);

#endif /*__LAYER2_NR_MAC_PROTO_H__*/
