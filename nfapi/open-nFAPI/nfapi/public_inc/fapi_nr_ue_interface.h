/*Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _FAPI_NR_UE_INTERFACE_H_
#define _FAPI_NR_UE_INTERFACE_H_
#include <pthread.h>

#include "stddef.h"
#include "platform_types.h"
#include "fapi_nr_ue_constants.h"
#include "PHY/impl_defs_nr.h"

#define NFAPI_UE_MAX_NUM_CB 8
#define NFAPI_MAX_NUM_UL_PDU 8

/*
  typedef unsigned int	   uint32_t;
  typedef unsigned short	   uint16_t;
  typedef unsigned char	   uint8_t;
  typedef signed int		   int32_t;
  typedef signed short	   int16_t;
  typedef signed char		   int8_t;
*/


typedef struct {
  uint8_t uci_format;
  uint8_t uci_channel;
  uint8_t harq_ack_bits;
  uint32_t harq_ack;
  uint8_t csi_bits;
  uint32_t csi;
  uint8_t sr_bits;
  uint32_t sr;
} fapi_nr_uci_pdu_rel15_t;

typedef struct {
  uint32_t rsrp;
  int rsrp_dBm;
  uint8_t rank_indicator;
  uint8_t i1;
  uint8_t i2;
  uint8_t cqi;
} fapi_nr_csirs_measurements_t;

typedef struct {
  /// frequency_domain_resource;
  uint8_t frequency_domain_resource[6];
  uint8_t StartSymbolIndex;
  uint8_t duration;
  uint8_t CceRegMappingType; //  interleaved or noninterleaved
  uint8_t RegBundleSize;     //  valid if CCE to REG mapping type is interleaved type
  uint8_t InterleaverSize;   //  valid if CCE to REG mapping type is interleaved type
  uint8_t ShiftIndex;        //  valid if CCE to REG mapping type is interleaved type
  uint8_t CoreSetType;
  uint8_t precoder_granularity;
  uint16_t pdcch_dmrs_scrambling_id;
  uint16_t scrambling_rnti;
  uint8_t tci_state_pdcch;
  uint8_t tci_present_in_dci;
} fapi_nr_coreset_t;

//
// Top level FAPI messages
//



//
// P7
//

typedef struct {
  uint16_t rnti;
  uint8_t dci_format;
  // n_CCE index of first CCE for PDCCH reception
  int n_CCE;
  // N_CCE is L, or number of CCEs for DCI
  int N_CCE;
  uint8_t payloadSize;
  uint8_t payloadBits[16] __attribute__((aligned(16))); // will be cast as uint64
  //fapi_nr_dci_pdu_rel15_t dci;
} fapi_nr_dci_indication_pdu_t;


///
typedef struct {
  uint16_t SFN;
  uint8_t slot;
  uint16_t number_of_dcis;
  fapi_nr_dci_indication_pdu_t dci_list[10];
} fapi_nr_dci_indication_t;


typedef struct {
  uint8_t harq_pid;
  uint8_t ack_nack;
  uint32_t pdu_length;
  uint8_t* pdu;
} fapi_nr_pdsch_pdu_t;

typedef struct {
  uint8_t* pdu;   //  3bytes
  uint8_t additional_bits;
  uint8_t ssb_index;
  uint8_t ssb_length;
  uint16_t cell_id;
  uint16_t ssb_start_subcarrier;
  short rsrp_dBm;
} fapi_nr_ssb_pdu_t;

typedef struct {
  uint32_t pdu_length;
  uint8_t* pdu;
  uint32_t sibs_mask;
} fapi_nr_sib_pdu_t;

typedef struct {
  uint8_t pdu_type;
  union {
    fapi_nr_pdsch_pdu_t pdsch_pdu;
    fapi_nr_ssb_pdu_t ssb_pdu;
    fapi_nr_sib_pdu_t sib_pdu;
    fapi_nr_csirs_measurements_t csirs_measurements;
  };
} fapi_nr_rx_indication_body_t;

///
#define NFAPI_RX_IND_MAX_PDU 100
typedef struct {
  uint16_t sfn;
  uint16_t slot;
  uint16_t number_pdus;
  fapi_nr_rx_indication_body_t rx_indication_body[NFAPI_RX_IND_MAX_PDU];
} fapi_nr_rx_indication_t;

typedef struct {
  uint8_t ul_cqi;
  uint16_t timing_advance;
  uint16_t rnti;
} fapi_nr_tx_config_t;

typedef struct {
  uint16_t pdu_length;
  uint16_t pdu_index;
  uint8_t* pdu;
} fapi_nr_tx_request_body_t;

///
typedef struct {
  uint16_t sfn;
  uint16_t slot;
  fapi_nr_tx_config_t tx_config;
  uint16_t number_of_pdus;
  fapi_nr_tx_request_body_t tx_request_body[NFAPI_MAX_NUM_UL_PDU];
} fapi_nr_tx_request_t;

/// This struct replaces:
/// PRACH-ConfigInfo from 38.331 RRC spec
/// PRACH-ConfigSIB or PRACH-Config
typedef struct {
  /// PHY cell ID
  uint16_t phys_cell_id;
  /// Num PRACH occasions
  uint8_t  num_prach_ocas;
  /// PRACH format
  uint8_t  prach_format;
  /// Num RA
  uint8_t  num_ra;
  uint8_t  prach_slot;
  uint8_t  prach_start_symbol;
  /// 38.211 (NCS 38.211 6.3.3.1).
  uint16_t num_cs;
  /// Parameter: prach-rootSequenceIndex, see TS 38.211 (6.3.3.2).
  uint16_t root_seq_id;
  /// Parameter: High-speed-flag, see TS 38.211 (6.3.3.1). 1 corresponds to Restricted set and 0 to Unrestricted set.
  uint8_t  restricted_set;
  /// see TS 38.211 (6.3.3.2).
  uint16_t freq_msg1;
  // When multiple SSBs per RO is configured, this indicates which one is selected in this RO -> this is used to properly compute the PRACH preamble
  uint8_t ssb_nb_in_ro;
  // nfapi_nr_ul_beamforming_t beamforming;
} fapi_nr_ul_config_prach_pdu;

typedef struct {
  uint16_t rnti;
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t format_type;
  uint8_t start_symbol_index;
  uint8_t nr_of_symbols;
  uint16_t prb_start;
  uint16_t prb_size;
  uint32_t hopping_id;
  uint8_t freq_hop_flag;
  uint8_t group_hop_flag;
  uint8_t sequence_hop_flag;
  uint16_t second_hop_prb;
  uint16_t initial_cyclic_shift;
  uint8_t time_domain_occ_idx;
  uint8_t add_dmrs_flag;
  uint16_t dmrs_scrambling_id;
  uint16_t data_scrambling_id;
  uint8_t dmrs_cyclic_shift;
  uint8_t pi_2bpsk;
  uint8_t mcs;
  uint8_t pre_dft_occ_idx;
  uint8_t pre_dft_occ_len;
  int16_t pucch_tx_power;
  uint32_t n_bit;
  uint64_t payload;
} fapi_nr_ul_config_pucch_pdu;

typedef struct
{
  uint8_t  rv_index;
  uint8_t  harq_process_id;
  uint8_t  new_data_indicator;
  uint32_t tb_size;
  uint8_t sci2_size;
  uint16_t num_cb;
  uint8_t cb_present_and_position[(NFAPI_UE_MAX_NUM_CB+7) / 8];

} nfapi_nr_ue_pusch_data_t;

typedef struct
{
  uint16_t harq_ack_bit_length;
  uint16_t csi_part1_bit_length;
  uint16_t csi_part2_bit_length;
  uint8_t  alpha_scaling;
  uint8_t  beta_offset_harq_ack;
  uint8_t  beta_offset_csi1;
  uint8_t  beta_offset_csi2;

} nfapi_nr_ue_pusch_uci_t;

typedef struct
{
  uint16_t ptrs_port_index;//PT-RS antenna ports [TS38.214, sec6.2.3.1 and 38.212, section 7.3.1.1.2] Bitmap occupying the 12 LSBs with: bit 0: antenna port 0 bit 11: antenna port 11 and for each bit 0: PTRS port not used 1: PTRS port used
  uint8_t  ptrs_dmrs_port;//DMRS port corresponding to PTRS.
  uint8_t  ptrs_re_offset;//PT-RS resource element offset value taken from 0~11
} nfapi_nr_ue_ptrs_ports_t;

typedef struct
{
  uint8_t  num_ptrs_ports;
  nfapi_nr_ue_ptrs_ports_t* ptrs_ports_list;
  uint8_t  ptrs_time_density;
  uint8_t  ptrs_freq_density;
  uint8_t  ul_ptrs_power;

}nfapi_nr_ue_pusch_ptrs_t;

typedef struct
{
  uint8_t  low_papr_group_number;//Group number for Low PAPR sequence generation.
  uint16_t low_papr_sequence_number;//[TS38.211, sec 5.2.2] For DFT-S-OFDM.
  uint8_t  ul_ptrs_sample_density;//Number of PTRS groups [But I suppose this sentence is misplaced, so as the next one. --Chenyu]
  uint8_t  ul_ptrs_time_density_transform_precoding;//Number of samples per PTRS group

} nfapi_nr_ue_dfts_ofdm_t;

typedef struct
{
  uint16_t beam_idx;//Index of the digital beam weight vector pre-stored at cell configuration. The vector maps this input port to output TXRUs. Value: 0->65535

}nfapi_nr_ue_dig_bf_interface_t;

typedef struct
{
  nfapi_nr_ue_dig_bf_interface_t* dig_bf_interface_list;

} nfapi_nr_ue_ul_beamforming_number_of_prgs_t;

typedef struct
{
  uint16_t num_prgs;
  uint16_t prg_size;
  //watchout: dig_bf_interface here, in table 3-43 it's dig_bf_interfaces
  uint8_t  dig_bf_interface;
  nfapi_nr_ue_ul_beamforming_number_of_prgs_t* prgs_list;//

} nfapi_nr_ue_ul_beamforming_t;

typedef struct
{
  uint16_t pdu_bit_map;//Bitmap indicating presence of optional PDUs (see above)
  uint16_t rnti;
  uint32_t handle;//An opaque handling returned in the RxData.indication and/or UCI.indication message
  //BWP
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t  subcarrier_spacing;
  uint8_t  cyclic_prefix;
  //pusch information always include
  uint16_t target_code_rate;
  uint8_t  qam_mod_order;
  uint8_t  mcs_index;
  uint8_t  mcs_table;
  uint8_t  transform_precoding;
  uint16_t data_scrambling_id;
  uint8_t  nrOfLayers;
  uint8_t  Tpmi;
  //DMRS
  uint16_t  ul_dmrs_symb_pos;
  uint8_t  dmrs_config_type;
  uint16_t ul_dmrs_scrambling_id;
  uint8_t  scid;
  uint8_t  num_dmrs_cdm_grps_no_data;
  uint16_t dmrs_ports;//DMRS ports. [TS38.212 7.3.1.1.2] provides description between DCI 0-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used
  //Pusch Allocation in frequency domain [TS38.214, sec 6.1.2.2]
  uint8_t  resource_alloc;
  uint8_t  rb_bitmap[36];//
  uint16_t rb_start;
  uint16_t rb_size;
  uint8_t  vrb_to_prb_mapping;
  uint8_t  frequency_hopping;
  uint16_t tx_direct_current_location;//The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299, which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300, which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
  uint8_t  uplink_frequency_shift_7p5khz;
  //Resource Allocation in time domain
  uint8_t  start_symbol_index;
  uint8_t  nr_of_symbols;
  uint32_t tbslbrm;
  //Optional Data only included if indicated in pduBitmap
  nfapi_nr_ue_pusch_data_t pusch_data;
  nfapi_nr_ue_pusch_uci_t  pusch_uci;
  nfapi_nr_ue_pusch_ptrs_t pusch_ptrs;
  nfapi_nr_ue_dfts_ofdm_t dfts_ofdm;
  //beamforming
  nfapi_nr_ue_ul_beamforming_t beamforming;
  //OAI specific
  int8_t absolute_delta_PUSCH;
} nfapi_nr_ue_pusch_pdu_t;

typedef struct {
  uint8_t priority;
  uint8_t freq_res;
  uint8_t time_res;
  uint8_t period;
  uint16_t dmrs_pattern;
  uint8_t mcs;
  uint8_t beta_offset;
  uint8_t dmrs_port;
} SCI_1_A;

typedef struct {
  uint8_t harq_pid;
  bool new_data;
  uint8_t red_version;
  uint8_t src_id;
  uint16_t dst_id;
  bool harq_enb;
  uint8_t cast_type;
  bool csi_req;
} SCI_2_A;

typedef struct
{
  uint16_t pdu_bit_map;//Bitmap indicating presence of optional PDUs (see above)
  uint16_t rnti;
  uint32_t handle;//An opaque handling returned in the RxData.indication and/or UCI.indication message
  //BWP
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t  subcarrier_spacing;
  uint8_t  cyclic_prefix;
  //pusch information always include
  uint16_t target_code_rate;
  uint8_t  qam_mod_order;
  uint8_t  mcs_index;
  uint8_t  mcs_table;
  uint8_t  transform_precoding;
  uint16_t data_scrambling_id;
  uint8_t  nrOfLayers;
  uint8_t  Tpmi;
  //DMRS
  uint16_t  sl_dmrs_symb_pos;
  uint8_t  dmrs_config_type;
  uint16_t ul_dmrs_scrambling_id;
  uint8_t  scid;
  uint8_t  num_dmrs_cdm_grps_no_data;
  uint16_t dmrs_ports;//DMRS ports. [TS38.212 7.3.1.1.2] provides description between DCI 0-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used
  //Pusch Allocation in frequency domain [TS38.214, sec 6.1.2.2]
  uint8_t  resource_alloc;
  uint8_t  rb_bitmap[36];//
  uint16_t rb_start;
  uint16_t rb_size;
  uint8_t  vrb_to_prb_mapping;
  uint8_t  frequency_hopping;
  uint16_t tx_direct_current_location;//The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299, which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300, which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
  uint8_t  uplink_frequency_shift_7p5khz;
  //Resource Allocation in time domain
  uint8_t  start_symbol_index;
  uint8_t  nr_of_symbols;
  uint32_t tbslbrm;
  //Optional Data only included if indicated in pduBitmap
  nfapi_nr_ue_pusch_data_t pssch_data;
  nfapi_nr_ue_pusch_ptrs_t pssch_ptrs;
  nfapi_nr_ue_dfts_ofdm_t dfts_ofdm;
  //beamforming
  nfapi_nr_ue_ul_beamforming_t beamforming;
  //OAI specific
  int8_t absolute_delta_PUSCH;
  // SCI1 for PSSCH
  SCI_1_A sci1;
  // SCI2 for PSSCH
  SCI_2_A sci2;
} nfapi_nr_ue_pssch_pdu_t;

typedef struct {
} fapi_nr_ul_srs_parms_v4;

typedef struct {
  uint16_t rnti;                      // UE RNTI, Value: 1->65535
  uint32_t handle;                    // An opaque handling returned in the SRS.indication
  uint16_t bwp_size;                  // Bandwidth part size [3GPP TS 38.213, sec 12]. Number of contiguous PRBs allocated to the BWP, Value: 1->275
  uint16_t bwp_start;                 // Bandwidth part start RB index from reference CRB [3GPP TS 38.213, sec 12], Value: 0->274
  uint8_t subcarrier_spacing;         // subcarrierSpacing [3GPP TS 38.211, sec 4.2], Value:0->4
  uint8_t cyclic_prefix;              // Cyclic prefix type [3GPP TS 38.211, sec 4.2], 0: Normal; 1: Extended
  uint8_t num_ant_ports;              // Number of antenna ports N_SRS_ap [3GPP TS 38.211, Sec 6.4.1.4.1], Value: 0 = 1 port, 1 = 2 ports, 2 = 4 ports
  uint8_t num_symbols;                // Number of symbols N_SRS_symb [3GPP TS 38.211, Sec 6.4.1.4.1], Value: 0 = 1 symbol, 1 = 2 symbols, 2 = 4 symbols
  uint8_t num_repetitions;            // Repetition factor R [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 = 1, 1 = 2, 2 = 4
  uint8_t time_start_position;        // Starting position in the time domain l_0 [3GPP TS 38.211, Sec 6.4.1.4.1], Note: the MAC undertakes the translation from startPosition to l_0, Value: 0 --> 13
  uint8_t config_index;               // SRS bandwidth config index C_SRS [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 --> 63
  uint16_t sequence_id;               // SRS sequence ID n_SRS_ID [3GPP TS 38.211, Sec 6.4.1.4.2], Value: 0 --> 1023
  uint8_t bandwidth_index;            // SRS bandwidth index B_SRS [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 --> 3
  uint8_t comb_size;                  // Transmission comb size K_TC [3GPP TS 38.211, Sec 6.4.1.4.2], Value: 0 = comb size 2, 1 = comb size 4, 2 = comb size 8 (Rel16)
  uint8_t comb_offset;                // Transmission comb offset K'_TC[3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 --> 1 (combSize = 0), Value: 0 --> 3 (combSize = 1), Value: 0 --> 7 (combSize = 2)
  uint8_t cyclic_shift;               // Cyclic shift n_CS_SRS [3GPP TS 38.211, Sec 6.4.1.4.2], Value: 0 --> 7 (combSize = 0), Value: 0 --> 11 (combSize = 1), Value: 0 --> 5 (combSize = 2)
  uint8_t frequency_position;         // Frequency domain position n_RRC [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 --> 67
  uint16_t frequency_shift;           // Frequency domain shift n_shift [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 --> 268
  uint8_t frequency_hopping;          // Frequency hopping b_hop [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0 --> 3
  uint8_t group_or_sequence_hopping;  // Group or sequence hopping configuration (RRC parameter groupOrSequenceHopping in SRSResource IE), Value: 0 = No hopping, 1 = Group hopping groupOrSequenceHopping, 2 = Sequence hopping
  uint8_t resource_type;              // Type of SRS resource allocation [3GPP TS 38.211, Sec 6.4.1.4.3], Value: 0: aperiodic, 1: semi-persistent, 2: periodic
  uint16_t t_srs;                     // SRS-Periodicity in slots [3GPP TS 38.211, Sec 6.4.1.4.4], Value: 1,2,3,4,5,8,10,16,20,32,40,64,80,160,320,640,1280,2560
  uint16_t t_offset;                  // Slot offset value [3GPP TS 38.211, Sec 6.4.1.4.3], Value:0->2559
  nfapi_nr_ue_ul_beamforming_t beamforming;
} fapi_nr_ul_config_srs_pdu;

typedef struct {
  uint8_t pdu_type;
  union {
    fapi_nr_ul_config_prach_pdu prach_config_pdu;
    fapi_nr_ul_config_pucch_pdu pucch_config_pdu;
    nfapi_nr_ue_pusch_pdu_t     pusch_config_pdu;
    fapi_nr_ul_config_srs_pdu   srs_config_pdu;
  };
} fapi_nr_ul_config_request_pdu_t;

typedef struct {
  //uint16_t sfn;
  //uint16_t slot;
  uint16_t sfn;
  uint16_t slot;
  uint8_t number_pdus;
  fapi_nr_ul_config_request_pdu_t ul_config_list[FAPI_NR_UL_CONFIG_LIST_NUM];
  pthread_mutex_t mutex_ul_config;
} fapi_nr_ul_config_request_t;


typedef struct {
  uint16_t rnti;
  uint16_t BWPSize;
  uint16_t BWPStart;
  uint8_t SubcarrierSpacing;
  fapi_nr_coreset_t coreset;
  uint8_t number_of_candidates;
  uint16_t CCE[64];
  uint8_t L[64];
  // 3GPP TS 38.212 Sec. 7.3.1.0, 3GPP TS 138.131 sec. 6.3.2 (SearchSpace)
  // The maximum number of DCI lengths allowed by the spec are 4, with max 3 for C-RNTI.
  // But a given search space may only support a maximum of 2 DCI formats at a time
  // depending on its search space type configured by RRC. Hence for blind decoding, UE
  // needs to monitor only upto 2 DCI lengths for a given search space.
  uint8_t num_dci_options;  // Num DCIs the UE actually needs to decode (1 or 2)
  uint8_t dci_length_options[2];
  uint8_t dci_format_options[2];
} fapi_nr_dl_config_dci_dl_pdu_rel15_t;

typedef struct {
  fapi_nr_dl_config_dci_dl_pdu_rel15_t dci_config_rel15;
} fapi_nr_dl_config_dci_pdu;
typedef struct{
  uint8_t aperiodicSRS_ResourceTrigger;
} fapi_nr_dl_srs_config_t;

typedef enum{vrb_to_prb_mapping_non_interleaved = 0, vrb_to_prb_mapping_interleaved = 1} vrb_to_prb_mapping_t;
//typedef fapi_nr_dci_pdu_rel15_t fapi_nr_dl_config_dlsch_pdu_rel15_t;
typedef struct {
  uint16_t BWPSize;
  uint16_t BWPStart;
  uint8_t SubcarrierSpacing;  
  uint16_t number_rbs;
  uint16_t start_rb;
  uint16_t number_symbols;
  uint16_t start_symbol;
  uint16_t dlDmrsSymbPos;  
  uint8_t dmrsConfigType;
  uint8_t prb_bundling_size_ind;
  uint8_t rate_matching_ind;
  uint8_t zp_csi_rs_trigger;
  uint8_t mcs;
  uint8_t ndi;
  uint8_t rv;
  uint16_t targetCodeRate;
  uint8_t qamModOrder;
  uint32_t TBS;
  uint8_t tb2_mcs;
  uint8_t tb2_ndi;
  uint8_t tb2_rv;
  uint8_t harq_process_nbr;
  vrb_to_prb_mapping_t vrb_to_prb_mapping;
  uint8_t dai;
  double scaling_factor_S;
  int8_t accumulated_delta_PUCCH;
  uint8_t pucch_resource_id;
  uint8_t pdsch_to_harq_feedback_time_ind;
  uint8_t n_dmrs_cdm_groups;
  uint16_t dmrs_ports;
  uint8_t n_front_load_symb;
  uint8_t tci_state;
  fapi_nr_dl_srs_config_t srs_config;
  uint8_t cbgti;
  uint8_t codeBlockGroupFlushIndicator;
  //  to be check the fields needed to L1 with NR_DL_UE_HARQ_t and NR_UE_DLSCH_t
  // PTRS [TS38.214, sec 5.1.6.3]
  /// PT-RS antenna ports [TS38.214, sec 5.1.6.3] [TS38.211, table 7.4.1.2.2-1] Bitmap occupying the 6 LSBs with: bit 0: antenna port 1000 bit 5: antenna port 1005 and for each bit 0: PTRS port not used 1: PTRS port used
  uint8_t PTRSPortIndex ;
  /// PT-RS time density [TS38.214, table 5.1.6.3-1] 0: 1 1: 2 2: 4
  uint8_t PTRSTimeDensity;
  /// PT-RS frequency density [TS38.214, table 5.1.6.3-2] 0: 2 1: 4
  uint8_t PTRSFreqDensity;
  /// PT-RS resource element offset [TS38.211, table 7.4.1.2.2-1] Value: 0->3
  uint8_t PTRSReOffset;
  ///  PT-RS-to-PDSCH EPRE ratio [TS38.214, table 4.1-2] Value :0->3
  uint8_t nEpreRatioOfPDSCHToPTRS;
  /// MCS table for this DLSCH
  uint8_t mcs_table;
  uint32_t tbslbrm;
  uint8_t nscid;
  uint16_t dlDmrsScramblingId;
  uint16_t pduBitmap;
} fapi_nr_dl_config_dlsch_pdu_rel15_t;

typedef struct {
  uint16_t rnti;
  fapi_nr_dl_config_dlsch_pdu_rel15_t dlsch_config_rel15;
} fapi_nr_dl_config_dlsch_pdu;


typedef struct {
  uint8_t subcarrier_spacing;       // subcarrierSpacing [3GPP TS 38.211, sec 4.2], Value:0->4
  uint8_t cyclic_prefix;            // Cyclic prefix type [3GPP TS 38.211, sec 4.2], 0: Normal; 1: Extended
  uint16_t start_rb;                // PRB where this CSI resource starts related to common resource block #0 (CRB#0). Only multiples of 4 are allowed. [3GPP TS 38.331, sec 6.3.2 parameter CSIFrequencyOccupation], Value: 0 ->274
  uint16_t nr_of_rbs;               // Number of PRBs across which this CSI resource spans. Only multiples of 4 are allowed. [3GPP TS 38.331, sec 6.3.2 parameter CSI-FrequencyOccupation], Value: 24 -> 276
  uint8_t csi_type;                 // CSI Type [3GPP TS 38.211, sec 7.4.1.5], Value: 0:TRS; 1:CSI-RS NZP; 2:CSI-RS ZP
  uint8_t row;                      // Row entry into the CSI Resource location table. [3GPP TS 38.211, sec 7.4.1.5.3 and table 7.4.1.5.3-1], Value: 1-18
  uint16_t freq_domain;             // Bitmap defining the frequencyDomainAllocation [3GPP TS 38.211, sec 7.4.1.5.3] [3GPP TS 38.331 CSIResourceMapping], Value: Up to the 12 LSBs, actual size is determined by the Row parameter
  uint8_t symb_l0;                  // The time domain location l0 and firstOFDMSymbolInTimeDomain [3GPP TS 38.211, sec 7.4.1.5.3], Value: 0->13
  uint8_t symb_l1;                  // The time domain location l1 and firstOFDMSymbolInTimeDomain2 [3GPP TS 38.211, sec 7.4.1.5.3], Value: 2->12
  uint8_t cdm_type;                 // The cdm-Type field [3GPP TS 38.211, sec 7.4.1.5.3 and table 7.4.1.5.3-1], Value: 0: noCDM; 1: fd-CDM2; 2: cdm4-FD2-TD2; 3: cdm8-FD2-TD4
  uint8_t freq_density;             // The density field, p and comb offset (for dot5). [3GPP TS 38.211, sec 7.4.1.5.3 and table 7.4.1.5.3-1], Value: 0: dot5 (even RB); 1: dot5 (odd RB); 2: one; 3: three
  uint16_t scramb_id;               // ScramblingID of the CSI-RS [3GPP TS 38.214, sec 5.2.2.3.1], Value: 0->1023
  uint8_t power_control_offset;     // Ratio of PDSCH EPRE to NZP CSI-RSEPRE [3GPP TS 38.214, sec 5.2.2.3.1], Value: 0->23 representing -8 to 15 dB in 1dB steps; 255: L1 is configured with ProfileSSS
  uint8_t power_control_offset_ss;  // Ratio of NZP CSI-RS EPRE to SSB/PBCH block EPRE [3GPP TS 38.214, sec 5.2.2.3.1], Values: 0: -3dB; 1: 0dB; 2: 3dB; 3: 6dB; 255: L1 is configured with ProfileSSS
} fapi_nr_dl_config_csirs_pdu_rel15_t;


typedef struct {
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t  subcarrier_spacing;
  uint16_t start_rb;
  uint16_t nr_of_rbs;
  uint8_t k_csiim[4];
  uint8_t l_csiim[4];
} fapi_nr_dl_config_csiim_pdu_rel15_t;


typedef struct {
  fapi_nr_dl_config_csirs_pdu_rel15_t csirs_config_rel15;
} fapi_nr_dl_config_csirs_pdu;


typedef struct {
  fapi_nr_dl_config_csiim_pdu_rel15_t csiim_config_rel15;
} fapi_nr_dl_config_csiim_pdu;


typedef struct {
  uint8_t pdu_type;
  union {
    fapi_nr_dl_config_dci_pdu dci_config_pdu;
    fapi_nr_dl_config_dlsch_pdu dlsch_config_pdu;
    fapi_nr_dl_config_csirs_pdu csirs_config_pdu;
    fapi_nr_dl_config_csiim_pdu csiim_config_pdu;
  };
} fapi_nr_dl_config_request_pdu_t;

typedef struct {
  uint16_t sfn;
  uint16_t slot;
  uint8_t number_pdus;
  fapi_nr_dl_config_request_pdu_t dl_config_list[FAPI_NR_DL_CONFIG_LIST_NUM];
} fapi_nr_dl_config_request_t;


//
// P5
//

    

typedef struct {
  fapi_nr_coreset_t coreset;

  uint8_t monitoring_slot_peridicity;
  uint8_t monitoring_slot_offset;
  uint16_t duration;
  uint16_t monitoring_symbols_within_slot;
  uint8_t number_of_candidates[5];            //  aggregation level 1, 2, 4, 8, 16

  uint8_t dci_2_0_number_of_candidates[5];    //  aggregation level 1, 2, 4, 8, 16
  uint8_t dci_2_3_monitorying_periodicity;
  uint8_t dci_2_3_number_of_candidates;
        
} fapi_nr_search_space_t;

typedef struct {
  fapi_nr_search_space_t search_space_sib1;
  fapi_nr_search_space_t search_space_others_sib;
  fapi_nr_search_space_t search_space_paging;
  //fapi_nr_coreset_t      coreset_ra;         //  common coreset
  fapi_nr_search_space_t search_space_ra;    
} fapi_nr_pdcch_config_common_t;

typedef struct {
  uint8_t k0;
  uint8_t mapping_type;
  uint8_t symbol_starting;
  uint8_t symbol_length;
} fapi_nr_pdsch_time_domain_resource_allocation_t;

typedef struct {
  fapi_nr_pdsch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_DL_ALLOCATIONS];
} fapi_nr_pdsch_config_common_t;

typedef struct {
  uint8_t prach_configuration_index;
  uint8_t msg1_fdm;
  uint8_t msg1_frequency_start;
  uint8_t zero_correlation_zone_config;
  uint8_t preamble_received_target_power;
  uint8_t preamble_transmission_max;
  uint8_t power_ramping_step;
  uint8_t ra_window_size;

  uint8_t total_number_of_preamble;
  uint8_t ssb_occasion_per_rach;
  uint8_t cb_preamble_per_ssb;

  uint8_t group_a_msg3_size;
  uint8_t group_a_number_of_preamble;
  uint8_t group_b_power_offset;
  uint8_t contention_resolution_timer;
  uint8_t rsrp_threshold_ssb;
  uint8_t rsrp_threshold_ssb_sul;
  uint8_t prach_length;   //  l839, l139
  uint8_t prach_root_sequence_index;  //  0 - 837 for l839, 0 - 137 for l139
  uint8_t msg1_subcarrier_spacing;
  uint8_t restrictedset_config;
  uint8_t msg3_transform_precoding;
} fapi_nr_rach_config_common_t;

typedef struct {
  uint8_t k2;
  uint8_t mapping_type;
  uint8_t symbol_starting;
  uint8_t symbol_length;
} fapi_nr_pusch_time_domain_resource_allocation_t;
      
typedef struct {
  uint8_t group_hopping_enabled_transform_precoding;
  fapi_nr_pusch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_UL_ALLOCATIONS];
  uint8_t msg3_delta_preamble;
  uint8_t p0_nominal_with_grant;
} fapi_nr_pusch_config_common_t;

typedef struct {
  uint8_t pucch_resource_common;
  uint8_t pucch_group_hopping;
  uint8_t hopping_id;
  uint8_t p0_nominal;
} fapi_nr_pucch_config_common_t;

typedef struct {
        
  fapi_nr_pdcch_config_common_t pdcch_config_common;
  fapi_nr_pdsch_config_common_t pdsch_config_common;
        
} fapi_nr_dl_bwp_common_config_t;



typedef struct {
  uint16_t int_rnti;
  uint8_t time_frequency_set;
  uint8_t dci_payload_size;
  uint8_t serving_cell_id[FAPI_NR_MAX_NUM_SERVING_CELLS];    //  interrupt configuration per serving cell
  uint8_t position_in_dci[FAPI_NR_MAX_NUM_SERVING_CELLS];    //  interrupt configuration per serving cell
} fapi_nr_downlink_preemption_t;

typedef struct {
  uint8_t tpc_index;
  uint8_t tpc_index_sul;
  uint8_t target_cell;
} fapi_nr_pusch_tpc_command_config_t;

typedef struct {
  uint8_t tpc_index_pcell;
  uint8_t tpc_index_pucch_scell;
} fapi_nr_pucch_tpc_command_config_t;

typedef struct {
  uint8_t starting_bit_of_format_2_3;
  uint8_t feild_type_format_2_3;
} fapi_nr_srs_tpc_command_config_t;

typedef struct {
  fapi_nr_downlink_preemption_t downlink_preemption;
  fapi_nr_pusch_tpc_command_config_t tpc_pusch;
  fapi_nr_pucch_tpc_command_config_t tpc_pucch;
  fapi_nr_srs_tpc_command_config_t tpc_srs;
} fapi_nr_pdcch_config_dedicated_t;

typedef struct {
  uint8_t dmrs_type;
  uint8_t dmrs_addition_position;
  uint8_t max_length;
  uint16_t scrambling_id0;
  uint16_t scrambling_id1;
  uint8_t ptrs_frequency_density[2];      //  phase tracking rs
  uint8_t ptrs_time_density[3];           //  phase tracking rs
  uint8_t ptrs_epre_ratio;                //  phase tracking rs
  uint8_t ptrs_resource_element_offset;   //  phase tracking rs
} fapi_nr_dmrs_downlink_config_t;

typedef struct {
  uint8_t bwp_or_cell_level;
  uint8_t pattern_type;
  uint32_t resource_blocks[9];        //  bitmaps type 275 bits
  uint8_t slot_type;                  //  bitmaps type one/two slot(s)
  uint32_t symbols_in_resouece_block; //  bitmaps type 14/28 bits
  uint8_t periodic;                   //  bitmaps type 
  uint32_t pattern[2];                //  bitmaps type 2/4/5/8/10/20/40 bits

  fapi_nr_coreset_t coreset;         //  coreset

  uint8_t subcarrier_spacing;
  uint8_t mode;
} fapi_nr_rate_matching_pattern_group_t;

typedef struct {
  //  resource mapping
  uint8_t row;    //  row1/row2/row4/other
  uint16_t frequency_domain_allocation; //    4/12/3/6 bits
  uint8_t number_of_ports;
  uint8_t first_ofdm_symbol_in_time_domain;
  uint8_t first_ofdm_symbol_in_time_domain2;
  uint8_t cdm_type;
  uint8_t density;            //  .5/1/3
  uint8_t density_dot5_type;  //  even/odd PRBs
        
  uint8_t frequency_band_starting_rb;     //  freqBand
  uint8_t frequency_band_number_of_rb;    //  freqBand

  //  periodicityAndOffset
  uint8_t periodicity;    //  slot4/5/8/10/16/20/32/40/64/80/160/320/640
  uint32_t offset;        //  0..639 bits
} fapi_nr_zp_csi_rs_resource_t;

typedef struct {
  uint16_t data_scrambling_id_pdsch;
  fapi_nr_dmrs_downlink_config_t dmrs_dl_for_pdsch_mapping_type_a;
  fapi_nr_dmrs_downlink_config_t dmrs_dl_for_pdsch_mapping_type_b; 
  uint8_t vrb_to_prb_interleaver;
  uint8_t resource_allocation;
  fapi_nr_pdsch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_DL_ALLOCATIONS];
  uint8_t pdsch_aggregation_factor;
  fapi_nr_rate_matching_pattern_group_t rate_matching_pattern_group1;
  fapi_nr_rate_matching_pattern_group_t rate_matching_pattern_group2;
  uint8_t rbg_size;
  uint8_t mcs_table;
  uint8_t max_num_of_code_word_scheduled_by_dci;
  uint8_t bundle_size;        //  prb_bundling static
  uint8_t bundle_size_set1;   //  prb_bundling dynamic 
  uint8_t bundle_size_set2;   //  prb_bundling dynamic
  fapi_nr_zp_csi_rs_resource_t periodically_zp_csi_rs_resource_set[FAPI_NR_MAX_NUM_ZP_CSI_RS_RESOURCE_PER_SET];
} fapi_nr_pdsch_config_dedicated_t;

typedef struct {
  uint16_t starting_prb;
  uint8_t intra_slot_frequency_hopping;
  uint16_t second_hop_prb;
  uint8_t format;                 //  pucch format 0..4
  uint8_t initial_cyclic_shift;
  uint8_t number_of_symbols;
  uint8_t starting_symbol_index;
  uint8_t time_domain_occ;
  uint8_t number_of_prbs;
  uint8_t occ_length;
  uint8_t occ_index;
} fapi_nr_pucch_resource_t;

typedef struct {
  uint8_t periodicity;
  uint8_t number_of_harq_process;
  fapi_nr_pucch_resource_t n1_pucch_an;
} fapi_nr_sps_config_t;

typedef struct {
  uint8_t beam_failure_instance_max_count;
  uint8_t beam_failure_detection_timer;
} fapi_nr_radio_link_monitoring_config_t;

typedef struct {
  fapi_nr_pdcch_config_dedicated_t pdcch_config_dedicated;
  fapi_nr_pdsch_config_dedicated_t pdsch_config_dedicated;
  fapi_nr_sps_config_t sps_config;
  fapi_nr_radio_link_monitoring_config_t radio_link_monitoring_config;

} fapi_nr_dl_bwp_dedicated_config_t;

typedef struct {
  fapi_nr_rach_config_common_t  rach_config_common;
  fapi_nr_pusch_config_common_t pusch_config_common;
  fapi_nr_pucch_config_common_t pucch_config_common;

} fapi_nr_ul_bwp_common_config_t;
        
typedef struct {
  uint8_t inter_slot_frequency_hopping;
  uint8_t additional_dmrs;
  uint8_t max_code_rate;
  uint8_t number_of_slots;
  uint8_t pi2bpsk;
  uint8_t simultaneous_harq_ack_csi;
} fapi_nr_pucch_format_config_t;

typedef struct {
  fapi_nr_pucch_format_config_t format1;
  fapi_nr_pucch_format_config_t format2;
  fapi_nr_pucch_format_config_t format3;
  fapi_nr_pucch_format_config_t format4;
  fapi_nr_pucch_resource_t multi_csi_pucch_resources[2];
  uint8_t dl_data_to_ul_ack[8];
  //  pucch power control
  uint8_t deltaF_pucch_f0;
  uint8_t deltaF_pucch_f1;
  uint8_t deltaF_pucch_f2;
  uint8_t deltaF_pucch_f3;
  uint8_t deltaF_pucch_f4;
  uint8_t two_pucch_pc_adjusment_states;
} fapi_nr_pucch_config_dedicated_t;

typedef struct {
  uint8_t dmrs_type;
  uint8_t dmrs_addition_position;
  uint8_t ptrs_uplink_config; // to indicate if PTRS Uplink is configured of not
  uint8_t ptrs_type;  //cp-OFDM, dft-S-OFDM
  uint16_t ptrs_frequency_density[2];
  uint8_t ptrs_time_density[3];
  uint8_t ptrs_max_number_of_ports;
  uint8_t ptrs_resource_element_offset;
  uint8_t ptrs_power;
  uint16_t ptrs_sample_density[5];
  uint8_t ptrs_time_density_transform_precoding;

  uint8_t max_length;
  uint16_t scrambling_id0;
  uint16_t scrambling_id1;
  uint8_t npusch_identity;
  uint8_t disable_sequence_group_hopping;
  uint8_t sequence_hopping_enable;
} fapi_nr_dmrs_uplink_config_t;

typedef struct {
  uint8_t tpc_accmulation;
  uint8_t msg3_alpha;
  uint8_t p0_nominal_with_grant;
  uint8_t two_pusch_pc_adjustments_states;
  uint8_t delta_mcs;
} fapi_nr_pusch_power_control_t;

typedef enum {tx_config_codebook = 1, tx_config_nonCodebook = 2} tx_config_t;
typedef enum {transform_precoder_enabled = 0, transform_precoder_disabled = 1} transform_precoder_t;
typedef enum {
  codebook_subset_fullyAndPartialAndNonCoherent = 1,
  codebook_subset_partialAndNonCoherent = 2,
  codebook_subset_nonCoherent = 3} codebook_subset_t;
typedef struct {
  uint16_t data_scrambling_identity;
  tx_config_t tx_config;
  fapi_nr_dmrs_uplink_config_t dmrs_ul_for_pusch_mapping_type_a;
  fapi_nr_dmrs_uplink_config_t dmrs_ul_for_pusch_mapping_type_b;
  fapi_nr_pusch_power_control_t pusch_power_control;
  uint8_t frequency_hopping;
  uint16_t frequency_hopping_offset_lists[4];
  uint8_t resource_allocation;
  fapi_nr_pusch_time_domain_resource_allocation_t allocation_list[FAPI_NR_MAX_NUM_UL_ALLOCATIONS];
  uint8_t pusch_aggregation_factor;
  uint8_t mcs_table;
  uint8_t mcs_table_transform_precoder;
  transform_precoder_t transform_precoder;
  codebook_subset_t codebook_subset;
  uint8_t max_rank;
  uint8_t rbg_size;

  //uci-OnPUSCH
  uint8_t uci_on_pusch_type;  //dynamic, semi-static
  uint8_t beta_offset_ack_index1[4];
  uint8_t beta_offset_ack_index2[4];
  uint8_t beta_offset_ack_index3[4];
  uint8_t beta_offset_csi_part1_index1[4];
  uint8_t beta_offset_csi_part1_index2[4];
  uint8_t beta_offset_csi_part2_index1[4];
  uint8_t beta_offset_csi_part2_index2[4];

  uint8_t tp_pi2BPSK;
} fapi_nr_pusch_config_dedicated_t;

typedef struct {
  uint8_t frequency_hopping;
  fapi_nr_dmrs_uplink_config_t cg_dmrs_configuration;
  uint8_t mcs_table;
  uint8_t mcs_table_transform_precoder;

  //uci-OnPUSCH
  uint8_t uci_on_pusch_type;  //dynamic, semi-static
  uint8_t beta_offset_ack_index1[4];
  uint8_t beta_offset_ack_index2[4];
  uint8_t beta_offset_ack_index3[4];
  uint8_t beta_offset_csi_part1_index1[4];
  uint8_t beta_offset_csi_part1_index2[4];
  uint8_t beta_offset_csi_part2_index1[4];
  uint8_t beta_offset_csi_part2_index2[4];

  uint8_t resource_allocation;
  //  rgb-Size structure missing in spec.
  uint8_t power_control_loop_to_use;
  //  p0-PUSCH-Alpha
  uint8_t p0;
  uint8_t alpha;

  uint8_t transform_precoder;
  uint8_t number_of_harq_process;
  uint8_t rep_k;
  uint8_t rep_k_rv;
  uint8_t periodicity;
  uint8_t configured_grant_timer;
  //  rrc-ConfiguredUplinkGrant
  uint16_t time_domain_offset;
  uint8_t time_domain_allocation;
  uint32_t frequency_domain_allocation;
  uint8_t antenna_ports;
  uint8_t dmrs_seq_initialization;
  uint8_t precoding_and_number_of_layers;
  uint8_t srs_resource_indicator;
  uint8_t mcs_and_tbs;
  uint8_t frequency_hopping_offset;
  uint8_t path_loss_reference_index;

} fapi_nr_configured_grant_config_t;

typedef struct {
  uint8_t qcl_type1_serving_cell_index;
  uint8_t qcl_type1_bwp_id;
  uint8_t qcl_type1_rs_type;  //  csi-rs or ssb
  uint8_t qcl_type1_nzp_csi_rs_resource_id;
  uint8_t qcl_type1_ssb_index;
  uint8_t qcl_type1_type;
        
  uint8_t qcl_type2_serving_cell_index;
  uint8_t qcl_type2_bwp_id;
  uint8_t qcl_type2_rs_type;  //  csi-rs or ssb
  uint8_t qcl_type2_nzp_csi_rs_resource_id;
  uint8_t qcl_type2_ssb_index;
  uint8_t qcl_type2_type;

} fapi_nr_tci_state_t;

typedef struct {
  uint8_t root_sequence_index;
  //  rach genertic
  uint8_t prach_configuration_index;
  uint8_t msg1_fdm;
  uint8_t msg1_frequency_start;
  uint8_t zero_correlation_zone_config;
  uint8_t preamble_received_target_power;
  uint8_t preamble_transmission_max;
  uint8_t power_ramping_step;
  uint8_t ra_window_size;

  uint8_t rsrp_threshold_ssb;
  //  PRACH-ResourceDedicatedBFR
  uint8_t bfr_ssb_index[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint8_t bfr_ssb_ra_preamble_index[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  // NZP-CSI-RS-Resource
  uint8_t bfr_csi_rs_nzp_resource_mapping[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint8_t bfr_csi_rs_power_control_offset[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint8_t bfr_csi_rs_power_control_offset_ss[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint16_t bfr_csi_rs_scrambling_id[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint8_t bfr_csi_rs_resource_periodicity[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint16_t bfr_csi_rs_resource_offset[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  fapi_nr_tci_state_t qcl_infomation_periodic_csi_rs[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];

  uint8_t bfr_csirs_ra_occasions[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS];
  uint8_t bfr_csirs_ra_preamble_index[FAPI_NR_MAX_NUM_CANDIDATE_BEAMS][FAPI_NR_MAX_RA_OCCASION_PER_CSIRS];

  uint8_t ssb_per_rach_occasion;
  uint8_t ra_ssb_occasion_mask_index;
  fapi_nr_search_space_t recovery_search_space;
  //  RA-Prioritization
  uint8_t power_ramping_step_high_priority;
  uint8_t scaling_factor_bi;
  uint8_t beam_failure_recovery_timer;
} fapi_nr_beam_failure_recovery_config_t;

typedef struct {
  fapi_nr_pucch_config_dedicated_t pucch_config_dedicated;
  fapi_nr_pusch_config_dedicated_t pusch_config_dedicated;
  fapi_nr_configured_grant_config_t configured_grant_config;
  //  SRS-Config
  uint8_t srs_tpc_accumulation;
  fapi_nr_beam_failure_recovery_config_t beam_failure_recovery_config;
        
} fapi_nr_ul_bwp_dedicated_config_t;

#define FAPI_NR_CONFIG_REQUEST_MASK_PBCH                0x01
#define FAPI_NR_CONFIG_REQUEST_MASK_DL_BWP_COMMON       0x02
#define FAPI_NR_CONFIG_REQUEST_MASK_UL_BWP_COMMON       0x04
#define FAPI_NR_CONFIG_REQUEST_MASK_DL_BWP_DEDICATED    0x08
#define FAPI_NR_CONFIG_REQUEST_MASK_UL_BWP_DEDICATED    0x10

typedef struct 
{
  uint16_t dl_bandwidth;//Carrier bandwidth for DL in MHz [38.104, sec 5.3.2] Values: 5, 10, 15, 20, 25, 30, 40,50, 60, 70, 80,90,100,200,400
  uint16_t sl_bandwidth; //Carrier bandwidth for SL in MHz [38.101, sec 5.3.5] Values: 10, 20, 30, and 40
  uint32_t dl_frequency; //Absolute frequency of DL point A in KHz [38.104, sec5.2 and 38.211 sec 4.4.4.2] Value: 450000 -> 52600000
  uint32_t sl_frequency; //Absolute frequency of SL point A in KHz [38.331, sec6.3.5 and 38.211 sec 8.2.7]
  uint16_t dl_k0[5];//𝑘_{0}^{𝜇} for each of the numerologies [38.211, sec 5.3.1] Value: 0 ->23699
  uint16_t dl_grid_size[5];//Grid size 𝑁_{𝑔𝑟𝑖𝑑}^{𝑠𝑖𝑧𝑒,𝜇} for each of the numerologies [38.211, sec 4.4.2] Value: 0->275 0 = this numerology not used
  uint16_t num_tx_ant;//Number of Tx antennas
  uint16_t uplink_bandwidth;//Carrier bandwidth for UL in MHz. [38.104, sec 5.3.2] Values: 5, 10, 15, 20, 25, 30, 40,50, 60, 70, 80,90,100,200,400
  uint32_t uplink_frequency;//Absolute frequency of UL point A in KHz [38.104, sec5.2 and 38.211 sec 4.4.4.2] Value: 450000 -> 52600000
  uint16_t ul_k0[5];//𝑘0 𝜇 for each of the numerologies [38.211, sec 5.3.1] Value: : 0 ->23699
  uint16_t ul_grid_size[5];//Grid size 𝑁𝑔𝑟𝑖𝑑 𝑠𝑖𝑧𝑒,𝜇 for each of the numerologies [38.211, sec 4.4.2]. Value: 0->275 0 = this numerology not used
  uint16_t sl_grid_size[5];
  uint16_t num_rx_ant;//
  uint8_t  frequency_shift_7p5khz;//Indicates presence of 7.5KHz frequency shift. Value: 0 = false 1 = true

} fapi_nr_ue_carrier_config_t; 

typedef struct 
{
  uint8_t phy_cell_id;//Physical Cell ID, 𝑁_{𝐼𝐷}^{𝑐𝑒𝑙𝑙} [38.211, sec 7.4.2.1] Value: 0 ->1007
  uint8_t frame_duplex_type;//Frame duplex type Value: 0 = FDD 1 = TDD

} fapi_nr_cell_config_t;

typedef struct 
{
  int ss_pbch_power;//SSB Block Power Value: TBD (-60..50 dBm)
  uint8_t  bch_payload;//Defines option selected for generation of BCH payload, see Table 3-13 (v0.0.011 Value: 0: MAC generates the full PBCH payload 1: PHY generates the timing PBCH bits 2: PHY generates the full PBCH payload
  uint8_t  scs_common;//subcarrierSpacing for common, used for initial access and broadcast message. [38.211 sec 4.2] Value:0->3

} fapi_nr_ssb_config_t;

typedef struct 
{
  uint32_t ssb_mask;//Bitmap for actually transmitted SSB. MSB->LSB of first 32 bit number corresponds to SSB 0 to SSB 31 MSB->LSB of second 32 bit number corresponds to SSB 32 to SSB 63 Value for each bit: 0: not transmitted 1: transmitted

} fapi_nr_ssb_mask_size_2_t;

typedef struct 
{
  int8_t beam_id[64];//BeamID for each SSB in SsbMask. For example, if SSB mask bit 26 is set to 1, then BeamId[26] will be used to indicate beam ID of SSB 26. Value: from 0 to 63

} fapi_nr_ssb_mask_size_64_t;

typedef struct 
{
  uint16_t ssb_offset_point_a;//Offset of lowest subcarrier of lowest resource block used for SS/PBCH block. Given in PRB [38.211, section 4.4.4.2] Value: 0->2199
  uint8_t  beta_pss;//PSS EPRE to SSS EPRE in a SS/PBCH block [38.213, sec 4.1] Values: 0 = 0dB
  uint8_t  ssb_period;//SSB periodicity in msec Value: 0: ms5 1: ms10 2: ms20 3: ms40 4: ms80 5: ms160
  uint8_t  ssb_subcarrier_offset;//ssbSubcarrierOffset or 𝑘𝑆𝑆𝐵 (38.211, section 7.4.3.1) Value: 0->31
  uint32_t MIB;//MIB payload, where the 24 MSB are used and represent the MIB in [38.331 MIB IE] and represent 0 1 2 3 1 , , , ,..., A− a a a a a [38.212, sec 7.1.1]
  fapi_nr_ssb_mask_size_2_t ssb_mask_list[2];
  fapi_nr_ssb_mask_size_64_t* ssb_beam_id_list;//64
  uint8_t  ss_pbch_multiple_carriers_in_a_band;//0 = disabled 1 = enabled
  uint8_t  multiple_cells_ss_pbch_in_a_carrier;//Indicates that multiple cells will be supported in a single carrier 0 = disabled 1 = enabled

} fapi_nr_ssb_table_t;

typedef struct 
{
  uint8_t slot_config;//For each symbol in each slot a uint8_t value is provided indicating: 0: DL slot 1: UL slot 2: Guard slot

} fapi_nr_max_num_of_symbol_per_slot_t;

typedef struct 
{
  fapi_nr_max_num_of_symbol_per_slot_t* max_num_of_symbol_per_slot_list;

} fapi_nr_max_tdd_periodicity_t;

typedef struct 
{
  uint8_t tdd_period;//DL UL Transmission Periodicity. Value:0: ms0p5 1: ms0p625 2: ms1 3: ms1p25 4: ms2 5: ms2p5 6: ms5 7: ms10 8: ms3 9: ms4
  uint8_t tdd_period_in_slots;
  fapi_nr_max_tdd_periodicity_t* max_tdd_periodicity_list;

} fapi_nr_tdd_table_t;

typedef struct 
{
  uint8_t  num_prach_fd_occasions;
  uint16_t prach_root_sequence_index;//Starting logical root sequence index, 𝑖, equivalent to higher layer parameter prach-RootSequenceIndex [38.211, sec 6.3.3.1] Value: 0 -> 837
  uint8_t  num_root_sequences;//Number of root sequences for a particular FD occasion that are required to generate the necessary number of preambles
  uint16_t k1;//Frequency offset (from UL bandwidth part) for each FD. [38.211, sec 6.3.3.2] Value: from 0 to 272
  uint8_t  prach_zero_corr_conf;//PRACH Zero CorrelationZone Config which is used to dervive 𝑁𝑐𝑠 [38.211, sec 6.3.3.1] Value: from 0 to 15
  uint8_t  num_unused_root_sequences;//Number of unused sequences available for noise estimation per FD occasion. At least one unused root sequence is required per FD occasion.
  uint8_t* unused_root_sequences_list;//Unused root sequence or sequences per FD occasion. Required for noise estimation.

} fapi_nr_num_prach_fd_occasions_t;

typedef struct 
{
  uint8_t prach_sequence_length;//RACH sequence length. Only short sequence length is supported for FR2. [38.211, sec 6.3.3.1] Value: 0 = Long sequence 1 = Short sequence
  uint8_t prach_sub_c_spacing;//Subcarrier spacing of PRACH. [38.211 sec 4.2] Value:0->4
  uint8_t restricted_set_config;//PRACH restricted set config Value: 0: unrestricted 1: restricted set type A 2: restricted set type B
  uint8_t num_prach_fd_occasions;//Corresponds to the parameter 𝑀 in [38.211, sec 6.3.3.2] which equals the higher layer parameter msg1FDM Value: 1,2,4,8
  fapi_nr_num_prach_fd_occasions_t* num_prach_fd_occasions_list;
  uint8_t ssb_per_rach;//SSB-per-RACH-occasion Value: 0: 1/8 1:1/4, 2:1/2 3:1 4:2 5:4, 6:8 7:16
  uint8_t prach_multiple_carriers_in_a_band;//0 = disabled 1 = enabled

} fapi_nr_prach_config_t;

typedef struct {
  uint32_t config_mask;

  fapi_nr_ue_carrier_config_t carrier_config;
  fapi_nr_cell_config_t cell_config;
  fapi_nr_ssb_config_t ssb_config;
  fapi_nr_ssb_table_t ssb_table;
  fapi_nr_tdd_table_t tdd_table;
  fapi_nr_prach_config_t prach_config;

} fapi_nr_config_request_t;

#endif

