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

/*! \file PHY/NR_TRANSPORT/defs.h
* \brief data structures for PDSCH/DLSCH/PUSCH/ULSCH physical and transport channel descriptors (TX/RX)
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
* \note
* \warning
*/
#ifndef __NR_TRANSPORT_UE__H__
#define __NR_TRANSPORT_UE__H__
#include <limits.h>
#include "PHY/impl_defs_top.h"
#include "nfapi_nr_interface_scf.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"

//#include "PHY/defs_nr_UE.h"
#include "../NR_TRANSPORT/nr_transport_common_proto.h"
/*#ifndef STANDALONE_COMPILE
#include "UTIL/LISTS/list.h"
#endif
*/
#include "openair2/NR_UE_PHY_INTERFACE/NR_IF_Module.h"


// structures below implement 36-211 and 36-212

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */

typedef enum {
 NEW_TRANSMISSION_HARQ,
 RETRANSMISSION_HARQ
} harq_result_t;

typedef struct {
  /// NDAPI struct for UE
  nfapi_nr_ue_pusch_pdu_t pusch_pdu;
  nfapi_nr_ue_pssch_pdu_t pssch_pdu;
  /// Indicator of first transmission
  uint8_t first_tx;
  /// HARQ tx status
  harq_result_t tx_status;
  /// Status Flag indicating for this ULSCH (idle,active,disabled)
  SCH_status_t status;
  /// Subframe scheduling indicator (i.e. Transmission opportunity indicator)
  uint8_t subframe_scheduling_flag;
  /// Subframe cba scheduling indicator (i.e. Transmission opportunity indicator)
  uint8_t subframe_cba_scheduling_flag;
  /// Last TPC command
  uint8_t TPC;
  /// The payload + CRC size in bits, "B" from 36-212
  uint32_t B;
  /// Length of ACK information (bits)
  uint8_t O_ACK;
  /// Index of current HARQ round for this ULSCH
  uint8_t round;
  /// Last Ndi for this harq process
  uint8_t ndi;
  /// pointer to pdu from MAC interface (TS 36.212 V15.4.0, Sec 5.1 p. 8)
  unsigned char *a;
  /// Pointer to the payload + CRC 
  uint8_t *b;
  /// Pointers to transport block segments
  uint8_t **c;
  /// LDPC-code outputs
  uint8_t **d;
  /// LDPC-code outputs (TS 36.212 V15.4.0, Sec 5.3.2 p. 17)
  uint8_t *e;
  /// Rate matching (Interleaving) outputs (TS 36.212 V15.4.0, Sec 5.4.2.2 p. 30)
  uint8_t *f;
  /// Number of code segments
  uint32_t C;
  /// Number of bits in code segments
  uint32_t K;
  /// Total number of bits across all segments
  uint32_t sumKr;
  /// Number of "Filler" bits
  uint32_t F;
  /// n_DMRS  for cyclic shift of DMRS
  uint8_t n_DMRS;
  /// n_DMRS2 for cyclic shift of DMRS
  uint8_t n_DMRS2;
  /// Flag to indicate that this is a control only ULSCH (i.e. no MAC SDU)
  uint8_t control_only;
  /// Flag to indicate that this is a calibration ULSCH (i.e. no MAC SDU and filled with TDD calibration information)
  //  int calibration_flag;
  /// Number of soft channel bits
  uint32_t G;
  // Number of modulated symbols carrying data
  uint32_t num_of_mod_symbols;
  // decode phich
  uint8_t decode_phich;
  // Encoder BG
  uint8_t BG;
  // LDPC lifting size
  uint32_t Z;
  //pointer to SCI2 payload from MAC interface
  uint64_t *a_sci2;
  //pointer to sci2 after crc + polar encoding and rate matching stored in bytes
  uint8_t *b_sci2;
  /// The sci2's payload + CRC + channel coder + rate matching size in bits, "B" from 36-212
  uint32_t B_sci2;
  // pointer ot output of polar encoder stored in bits
  uint8_t *f_sci2;
  // pointer ot output of data-control multiplexer in bits
  uint8_t *f_multiplexed;
  // size of multiplexed output in bits
  uint32_t B_multiplexed;
  /////////////////////// slsch decoding ///////////////////////
  /// Frame where current HARQ round was sent
  uint32_t frame;
  /// Slot where current HARQ round was sent
  uint32_t slot;
  /// Flag to indicate that the UL configuration has been handled. Used to remove a stale ULSCH when frame wraps around
  uint8_t handled;

  /// Transport block size (This is A from 38.212 V16.10.0 section 5.1)
  uint32_t TBS;
  /// Number of bits in each code block after rate matching for LDPC code (38.212 V16.10.0 section 5.4.2.1)
  uint32_t E;
  /// Number of segments processed so far
  uint32_t processedSegments;
  bool new_rx;
  //////////////////////////////////////////////////////////////
} NR_UL_UE_HARQ_t;

typedef struct {
  /// SRS active flag
  uint8_t srs_active; 
  // Pointers to HARQ processes for the ULSCH
  NR_UL_UE_HARQ_t *harq_processes[NR_MAX_ULSCH_HARQ_PROCESSES];
  int harq_process_id[NR_MAX_SLOTS_PER_FRAME];
  // UL number of harq processes
  uint8_t number_harq_processes_for_pusch;
  /// Minimum number of CQI bits for PUSCH (36-212 r8.6, Sec 5.2.4.1 p. 37)
  /// HARQ process mask, indicates which processes are currently active
  uint16_t harq_mask;
  uint8_t O_CQI_MIN;
  /// ACK/NAK Bundling flag
  uint8_t bundling;
  /// beta_offset_cqi times 8
  uint16_t beta_offset_cqi_times8;
  /// beta_offset_ri times 8
  uint16_t beta_offset_ri_times8;
  /// beta_offset_harqack times 8
  uint16_t beta_offset_harqack_times8;
  /// power_offset
  uint8_t power_offset;
  // for cooperative communication
  uint8_t cooperation_flag;
  /// Allocated RNTI for this SLSCH
  uint16_t rnti;
  /// RNTI type
  uint8_t rnti_type;
  /// Cell ID
  int     Nid_cell;
  // Nidx = it is used for SLSCH scrambler 
  uint16_t Nidx;
  /// f_PUSCH parameter for PUSCH power control
  int16_t f_pusch;
  /// Po_PUSCH - target output power for PUSCH
  int16_t Po_PUSCH;
  /// PHR - current power headroom (based on last PUSCH transmission)
  int16_t PHR;
  /// Po_SRS - target output power for SRS
  int16_t Po_SRS;
  /// num active cba group
  uint8_t num_active_cba_groups;
  /// bit mask of PT-RS ofdm symbol indicies
  uint16_t ptrs_symbols;
  /// num dci found for cba
  //uint8_t num_cba_dci[10];
  /// allocated CBA RNTI
  //uint16_t cba_rnti[4];//NUM_MAX_CBA_GROUP];
  /// Maximum number of LDPC iterations in sidelink decoding
  uint8_t max_ldpc_iterations;
  /// number of iterations used in last LDPC decoding in sidelink decoding
  uint8_t last_iteration_cnt;
} NR_UE_ULSCH_t;

typedef struct {
  /// Indicator of first reception
  uint8_t first_rx;
  /// Last Ndi received for this process on DCI (used for C-RNTI only)
  uint8_t Ndi;
  /// DLSCH status flag indicating
  SCH_status_t status;
  /// Transport block size
  uint32_t TBS;
  /// The payload + CRC size in bits
  uint32_t B;
  /// The sci2's payload + CRC + channel coder + rate matching size in bits, "B" from 36-212
  uint32_t B_sci2;
  /// Pointer to the payload
  uint8_t *b;
  /// Pointer to the sci2 payload
  uint64_t *b_sci2;
  /// Pointers to transport block segments
  uint8_t **c;
  /// soft bits for each received segment ("d"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
  /// Accumulates the soft bits for each round to increase decoding success (HARQ)
  int16_t **d;
  /// Index of current HARQ round for this DLSCH
  uint8_t DLround;
  /// MCS table for this DLSCH
  uint8_t mcs_table;
  /// MCS format for this DLSCH
  uint8_t mcs;
  /// Qm (modulation order) for this DLSCH
  uint8_t Qm;
  /// target code rate R x 1024
  uint16_t R;
  /// Redundancy-version of the current sub-frame
  uint8_t rvidx;
  /// MIMO mode for this DLSCH
  MIMO_nrmode_t mimo_mode;
  /// Number of code segments 
  uint32_t C;
  /// Number of bits in code segments
  uint32_t K;
  /// Number of "Filler" bits 
  uint32_t F;
  /// LDPC lifting factor
  uint32_t Z;
  /// Number of MIMO layers (streams) 
  uint8_t Nl;
  /// current delta_pucch
  int8_t delta_PUCCH;
  /// Number of soft channel bits
  uint32_t G;
  /// Start PRB of BWP
  uint16_t BWPStart;
  /// Number of PRBs in BWP
  uint16_t BWPSize;
  /// Current Number of RBs
  uint16_t nb_rb;
  /// Starting RB number
  uint16_t start_rb;
  /// Number of Symbols
  uint16_t nb_symbols;
  /// DMRS symbol positions
  uint16_t dlDmrsSymbPos;
  /// DMRS Configuration Type
  uint8_t dmrsConfigType;
  // Number of DMRS CDM groups with no data
  uint8_t n_dmrs_cdm_groups;
  /// DMRS ports bitmap
  uint16_t dmrs_ports;
  /// Starting Symbol number
  uint16_t start_symbol;
  /// Current subband PMI allocation
  uint16_t pmi_alloc;
  /// Current RB allocation (even slots)
  uint32_t rb_alloc_even[4];
  /// Current RB allocation (odd slots)
  uint32_t rb_alloc_odd[4];
  /// distributed/localized flag
  vrb_t vrb_type;
  /// downlink power offset field
  uint8_t dl_power_off;
  /// codeword this transport block is mapped to
  uint8_t codeword;
  /// HARQ-ACKs
  uint8_t ack;
  /// PTRS Frequency Density
  uint8_t PTRSFreqDensity;
  /// PTRS Time Density
  uint8_t PTRSTimeDensity;
  uint8_t PTRSPortIndex ;
  uint8_t nEpreRatioOfPDSCHToPTRS;
  uint8_t PTRSReOffset;
  /// bit mask of PT-RS ofdm symbol indicies
  uint16_t ptrs_symbols;
  // PTRS symbol index, to be updated every PTRS symbol within a slot.
  uint8_t ptrs_symbol_index;
  uint32_t tbslbrm;
  uint8_t nscid;
  uint16_t dlDmrsScramblingId;
  /// PDU BITMAP 
  uint16_t pduBitmap;

  nfapi_nr_pssch_pdu_t pssch_pdu;
} NR_DL_UE_HARQ_t;

typedef struct {
  /// RNTI
  uint16_t rnti;
  /// RNTI type
  uint8_t rnti_type;
  /// Active flag for DLSCH demodulation
  uint8_t active;
  /// accumulated tx power adjustment for PUCCH
  int8_t g_pucch;
  /// Transmission mode
  uint8_t mode1_flag;
  /// amplitude of PDSCH (compared to RS) in symbols without pilots
  int16_t sqrt_rho_a;
  /// amplitude of PDSCH (compared to RS) in symbols containing pilots
  int16_t sqrt_rho_b;
  /// Current HARQ process id threadRx Odd and threadRx Even
  uint8_t current_harq_pid;
  /// Current subband antenna selection
  uint32_t antenna_alloc;
  /// Current subband RI allocation
  uint32_t ri_alloc;
  /// Current subband CQI1 allocation
  uint32_t cqi_alloc1;
  /// Current subband CQI2 allocation
  uint32_t cqi_alloc2;
  /// saved subband PMI allocation from last PUSCH/PUCCH report
  uint16_t pmi_alloc;
  /// Pointers to up to HARQ processes
  NR_DL_UE_HARQ_t *harq_processes[NR_MAX_DLSCH_HARQ_PROCESSES];
  // DL number of harq processes
  uint8_t number_harq_processes_for_pdsch;
  /* higher layer parameter for reception of two transport blocks TS 38.213 9.1.3.1 Type-2 HARQ-ACK codebook dtermination */
  uint8_t Number_MCS_HARQ_DL_DCI;
  /* spatial bundling of PUCCH */
  uint8_t HARQ_ACK_spatial_bundling_PUCCH;
  /// Maximum number of HARQ processes(for definition see 36-212 V8.6 2009-03, p.17
  uint8_t Mdlharq;
  /// MIMO transmission mode indicator for this sub-frame (for definition see 36-212 V8.6 2009-03, p.17)
  uint8_t Kmimo;
  /// Nsoft parameter related to UE Category
  uint32_t Nsoft;
  /// Maximum number of LDPC iterations
  uint8_t max_ldpc_iterations;
  /// number of iterations used in last turbo decoding
  uint8_t last_iteration_cnt;
} NR_UE_DLSCH_t;

typedef enum {format0_0,
              format0_1,
              format1_0,
              format1_1,
              format2_0,
              format2_1,
              format2_2,
              format2_3
             } NR_DCI_format_t;


typedef enum {nr_pucch_format0=0,
              nr_pucch_format1,
              nr_pucch_format2,
              nr_pucch_format3,
              nr_pucch_format4
             } NR_PUCCH_FMT_t;

typedef struct {
  /// Length of DCI in bits
  uint8_t dci_length;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  bool ra_flag;
  /// rnti
  rnti_t rnti;
  /// rnti type
  //crc_scrambled_t rnti_type;
  /// Format
  NR_DCI_format_t format;
  /// search space
  dci_space_t search_space;
  /// DCI pdu
  uint64_t dci_pdu[2];
//#if defined(UPGRADE_RAT_NR)
#if 1
  /// harq information
  uint8_t harq_pid_pusch;
  /// delay between current slot and slot to transmit
  uint8_t number_slots_rx_to_tx;
#endif
} NR_DCI_ALLOC_t;

typedef struct {
  long  sl_numssb_withinperiod_r16;
  long  sl_timeoffsetssb_r16;
  long  sl_timeinterval_r16;
  long  sl_numssb_withinperiod_r16_copy;
  long  sl_timeinterval_r16_copy;
  long  sl_timeoffsetssb_r16_copy;
  uint16_t slss_id;
  uint8_t sl_mib_length;
  uint8_t sl_mib[5];
} NR_SLSS_t;

/**@}*/
#endif