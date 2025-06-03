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

/*! \file PHY/NR_UE_TRANSPORT/nr_slsch.c
* \brief Top-level routines for transmission of the PUSCH TS 38.211 v 15.4.0
* \author Khalid Ahmed
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: khalid.ahmed@iis.fraunhofer.de
* \note
* \warning
*/
#include <time.h>
#include <stdint.h>
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/MODULATION/modulation_common.h"
#include "common/utils/assertions.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_UE_TRANSPORT/nr_sch_dmrs_sl.h"
#include "PHY/defs_nr_common.h"
#include "PHY/TOOLS/tools_defs.h"
#include "executables/nr-softmodem.h"
#include "executables/softmodem-common.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include <openair2/UTIL/OPT/opt.h>
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "executables/nr-uesoftmodem.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include <openair2/LAYER2/NR_MAC_COMMON/nr_mac_common.h>
//#define DEBUG_PSSCH_MAPPING
//#define DEBUG_MAC_PDU
//#define DEBUG_DFT_IDFT

//extern int32_t uplink_counter;
#define SCI2_LEN_SIZE 35
#define HNA_SIZE 16 * 68 * 384 // [hna] 16 segments, 68*Zc
#define MAX_RELAY_Q_SIZE 5

pthread_mutex_t relay_mem_lock;
uint8_t cirBufRelay[MAX_RELAY_Q_SIZE][HNA_SIZE];
uint64_t cirBufRelaySCI2[MAX_RELAY_Q_SIZE];
uint8_t cirBufReadInx;
uint8_t cirBufWriteInx;
uint32_t relay_data_to_send;

void init_mutex_of_relay_data()
{
  pthread_mutex_init(&relay_mem_lock, NULL);
}

uint32_t qsize_of_relay_data()
{
  pthread_mutex_lock(&relay_mem_lock);
  uint32_t temp = relay_data_to_send;
  pthread_mutex_unlock(&relay_mem_lock);

  return temp;
}

void get_relay_data_from_buffer(uint8_t *dest, uint64_t *sci2, uint32_t sz)
{
  pthread_mutex_lock(&relay_mem_lock);
  for (int i = 0; i < sz; i++) {
    dest[i] = cirBufRelay[cirBufReadInx][i];
  }

  *sci2 = cirBufRelaySCI2[cirBufReadInx];
  cirBufReadInx = (cirBufReadInx + 1) % MAX_RELAY_Q_SIZE;
  relay_data_to_send--;

  LOG_D(PHY, "Decreased the number of relay_data_to_send %u\n", relay_data_to_send);
  pthread_mutex_unlock(&relay_mem_lock);
}

void put_relay_data_to_buffer(uint8_t *src, uint64_t *sci2, uint32_t sz)
{
  pthread_mutex_lock(&relay_mem_lock);
  if(relay_data_to_send >= MAX_RELAY_Q_SIZE) {
    LOG_W(PHY, "Relay Buffer overflow: relay_data_to_send %u >= %u Capacity\n", relay_data_to_send, MAX_RELAY_Q_SIZE);
  }

  for (int i = 0; i < sz; i++) {
    cirBufRelay[cirBufWriteInx][i] = src[i];
  }

  cirBufRelaySCI2[cirBufWriteInx] = *sci2;
  cirBufWriteInx = (cirBufWriteInx + 1) % MAX_RELAY_Q_SIZE;
  relay_data_to_send++;

  LOG_D(PHY, "Increased the number of relay_data_to_send %u\n", relay_data_to_send);
  pthread_mutex_unlock(&relay_mem_lock);
}

void nr_pusch_codeword_scrambling_sl(uint8_t *in,
                                     uint32_t size,
                                     uint32_t SCI2_bits,
                                     uint16_t Nid,
                                     uint32_t* out)
{
  uint8_t reset = 1, c, j = 0;
  uint32_t x1, x2, s = 0;
  int m_ij = 0;
  x2 = (Nid << 15) + 1010; // 1010 is following the spec. 38.211, 8.3.1.1
  for (int i = 0; i < size; i++) {
    const uint8_t b_idx = i & 0x1f;
    if (b_idx == 0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
      if (i)
        out++;
    }
    if (in[i] == NR_PSSCH_x) {
      *out ^= ((*out >> (b_idx - 2)) & 1) << b_idx;
      j++;
    } else {
      m_ij =  (i < SCI2_bits) ? j : SCI2_bits;
      c = (uint8_t)((s >> ((i - m_ij) % 32)) & 1);
      *out ^= ((in[i] + c) & 1) << b_idx;
    }
    //LOG_I(NR_PHY, "i %d b_idx %d in %d s 0x%08x out 0x%08x\n", i, b_idx, in[i], s, *out);
  }
}

void nr_slsch_layer_demapping(int16_t *llr_cw,
                              uint8_t Nl,
                              uint8_t mod_order,
                              uint32_t length,
                              int16_t **llr_layers)
{

  switch (Nl) {
    case 1:
      memcpy((void*)llr_cw, (void*)llr_layers[0], (length) * sizeof(int16_t));
      break;
    case 2:
    case 3:
    case 4:
      for (int i = 0; i < (length / Nl / mod_order); i++) {
        for (int l = 0; l < Nl; l++) {
          for (int m = 0; m < mod_order; m++) {
            llr_cw[i * Nl * mod_order + l * mod_order + m] = llr_layers[l][i * mod_order + m];
          }
        }
      }
      break;
  default:
  AssertFatal(0, "Not supported number of layers %d\n", Nl);
  }
}

void nr_pssch_data_control_multiplexing(uint8_t *in_slssh,
                                        uint8_t *in_sci2,
                                        uint32_t slssh_bits,
                                        uint32_t SCI2_bits,
                                        uint8_t Nl,
                                        uint8_t Q_SCI2,
                                        uint8_t* out)
{
  if (Nl == 1) {
    memcpy(out, in_sci2, SCI2_bits);
    memcpy(out + SCI2_bits, in_slssh, slssh_bits);
  } else if (Nl == 2) {
    uint32_t  M = SCI2_bits / Q_SCI2;
    uint8_t m = 0;
    for (int i = 0; i < M; i++) {
      for (int v = 0; v < Nl; v++) {
        for (int q = 0; q < Q_SCI2; q++) {
          if(v == 0)
            out[m] = in_sci2[i * Q_SCI2 + q];
          else
            out[m] = NR_PSSCH_x;
          m = m + 1;
        }
      }
    }
    memcpy(out + SCI2_bits * Nl, in_slssh, slssh_bits);
  }
}

uint32_t get_B_multiplexed_value(NR_DL_FRAME_PARMS* fp, NR_DL_UE_HARQ_t *harq) {

  uint8_t Nl                = harq->Nl;
  unsigned int G_slsch_bits = harq->G;
  uint32_t B_mul = G_slsch_bits + harq->B_sci2 * Nl;
  return B_mul;
}

static int get_PRB(int nb_rb) {
  if (nb_rb > 25)
    return (nb_rb / 25) * 25;
  if (nb_rb > 9)
    return (nb_rb / 5) * 5;
  else
    AssertFatal(1 == 0, "Not supported PRB");
}

void nr_ue_set_slsch_rx(PHY_VARS_NR_UE *ue, unsigned char harq_pid)
{
  int nb_rb = get_PRB(ue->frame_parms.N_RB_SL);
  uint16_t nb_symb_sch = 12;
  uint16_t nb_symb_cch = 3; // Assumption there are three SLCCH symbols
  uint8_t dmrsConfigType = 0;
  uint8_t Nl = 1; // number of layers
  uint8_t Imcs = ue->frame_parms.Imcs;
  uint16_t dmrsSymbPos = 16 + 1024; // symbol 4 and 10
  uint8_t length_dmrs = get_num_dmrs(dmrsSymbPos);
  uint16_t start_symbol = 1; // start from 0
  uint8_t  SCI2_mod_order = 2;

  uint8_t mod_order = nr_get_Qm_ul(Imcs, 0);
  uint16_t code_rate = nr_get_code_rate_ul(Imcs, 0);
  LOG_D(NR_PHY, "\ncode_rate %u mod_order %d\n", code_rate, mod_order);

  NR_UE_DLSCH_t *slsch_ue_rx = ue->slsch_rx[0][0][0];
  NR_DL_UE_HARQ_t *harq = slsch_ue_rx->harq_processes[harq_pid];
  harq->Nl = Nl;
  harq->Qm = mod_order;
  harq->nb_rb = nb_rb;
  harq->n_dmrs_cdm_groups = 1;
  harq->dlDmrsSymbPos = dmrsSymbPos;
  harq->mcs = Imcs;
  harq->dmrsConfigType = dmrsConfigType;
  harq->R = code_rate;
  harq->nb_symbols = nb_symb_sch;
  harq->codeword = 0;
  harq->start_symbol = start_symbol;

  uint8_t nb_re_dmrs = 6;
  if (harq->dmrsConfigType == NFAPI_NR_DMRS_TYPE1) {
    nb_re_dmrs = 6 * harq->n_dmrs_cdm_groups;
  } else {
    nb_re_dmrs = 4 * harq->n_dmrs_cdm_groups;
  }

  uint16_t nb_re_sci1 = nb_symb_cch * NB_RB_SCI1 * NR_NB_SC_PER_RB;
  uint8_t N_PRB_oh = 0;
  uint16_t nb_symb_psfch = 0;
  uint8_t nb_codewords = 1;
  uint16_t N_RE_prime = NR_NB_SC_PER_RB * (nb_symb_sch - nb_symb_psfch) - nb_re_dmrs - N_PRB_oh;
  uint16_t num_of_mod_symbols = N_RE_prime * nb_rb * nb_codewords - nb_re_sci1;
  harq->B_sci2 = polar_encoder_output_length(code_rate, num_of_mod_symbols);

  uint16_t nb_re_sci2 = harq->B_sci2 / SCI2_mod_order;
  unsigned int TBS = nr_compute_tbs_sl(mod_order, code_rate, nb_rb, nb_re_sci1, nb_re_sci2, nb_symb_sch, nb_re_dmrs * length_dmrs, 0, 0, Nl);
  harq->TBS = TBS >> 3;
  harq->G = nr_get_G_sl(nb_rb, nb_re_sci1, nb_re_sci2, harq->nb_symbols, nb_re_dmrs, length_dmrs, harq->Qm, harq->Nl);
  LOG_D(NR_PHY, "mcs %u polar_encoder_output_len %u, code_rate %d, TBS %d\n", Imcs, harq->B_sci2, code_rate, TBS);
  harq->status = ACTIVE;

  nfapi_nr_pssch_pdu_t *rel16_sl_rx = &harq->pssch_pdu;
  rel16_sl_rx->mcs_index            = Imcs;
  rel16_sl_rx->pssch_data.rv_index  = 0;
  rel16_sl_rx->target_code_rate     = code_rate;
  rel16_sl_rx->qam_mod_order        = mod_order;
  rel16_sl_rx->pssch_data.sci2_size = SCI2_LEN_SIZE >> 3;
  rel16_sl_rx->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS, code_rate);
  rel16_sl_rx->nr_of_symbols  = nb_symb_sch; // number of symbols per slot
  rel16_sl_rx->start_symbol_index = start_symbol;
  rel16_sl_rx->ul_dmrs_symb_pos = harq->dlDmrsSymbPos;
  rel16_sl_rx->nrOfLayers = harq->Nl;
  rel16_sl_rx->num_dmrs_cdm_grps_no_data = 1;
  rel16_sl_rx->rb_size = nb_rb;
  rel16_sl_rx->bwp_start = 0;
  rel16_sl_rx->rb_start = 0;
  rel16_sl_rx->dmrs_config_type = dmrsConfigType;
  rel16_sl_rx->pssch_data.tb_size = TBS >> 3;
}

void nr_ue_set_slsch(NR_DL_FRAME_PARMS *fp,
                     unsigned char harq_pid,
                     NR_UE_ULSCH_t *slsch,
                     uint32_t frame,
                     uint8_t slot) {
  NR_UL_UE_HARQ_t *harq = slsch->harq_processes[harq_pid];
  uint8_t nb_codewords = 1;
  uint16_t nb_symb_sch = 12;
  uint16_t nb_symb_cch = 3; // Assumption there are three SLCCH symbols
  int nb_rb = get_PRB(fp->N_RB_SL);
  uint8_t Imcs = fp->Imcs;
  uint8_t Nl = 1; // number of layers
  uint16_t start_symbol = 1; // start from 0
  SCI_1_A *sci1 = &harq->pssch_pdu.sci1;
  sci1->period = 0;
  sci1->dmrs_pattern = (1 << 4) + (1 << 10);
  sci1->beta_offset = 0;
  sci1->dmrs_port = 0;
  sci1->priority = 0;
  sci1->freq_res = 1;
  sci1->time_res = 1;
  sci1->mcs = Imcs;
  uint16_t dmrsSymbPos = sci1->dmrs_pattern; // symbol 4 and 10
  uint8_t dmrsConfigType = 0;
  uint8_t length_dmrs = get_num_dmrs(dmrsSymbPos);
  uint16_t code_rate = nr_get_code_rate_ul(Imcs, 0);
  uint8_t mod_order = nr_get_Qm_ul(Imcs, 0);
  harq->pssch_pdu.mcs_index = Imcs;
  harq->pssch_pdu.nrOfLayers = Nl;
  harq->pssch_pdu.rb_size = nb_rb;
  harq->pssch_pdu.nr_of_symbols = nb_symb_sch;
  harq->pssch_pdu.dmrs_config_type = dmrsConfigType;
  harq->num_of_mod_symbols = NR_NB_SC_PER_RB * nb_rb * nb_codewords / 2;//symbol 4 without drms res.
  harq->pssch_pdu.pssch_data.rv_index = 0;
  harq->pssch_pdu.pssch_data.sci2_size = SCI2_LEN_SIZE >> 3;
  harq->pssch_pdu.target_code_rate = code_rate;
  harq->pssch_pdu.qam_mod_order = mod_order;
  harq->pssch_pdu.sl_dmrs_symb_pos = dmrsSymbPos;
  harq->pssch_pdu.num_dmrs_cdm_grps_no_data = 1;
  harq->pssch_pdu.start_symbol_index = start_symbol;
  harq->pssch_pdu.transform_precoding = transformPrecoder_disabled;
  harq->first_tx = 1;

  uint8_t nb_re_dmrs = 6;
  if (harq->pssch_pdu.dmrs_config_type == NFAPI_NR_DMRS_TYPE1) {
    nb_re_dmrs = 6 * harq->pssch_pdu.num_dmrs_cdm_grps_no_data;
  } else {
    nb_re_dmrs = 4 * harq->pssch_pdu.num_dmrs_cdm_grps_no_data;
  }

  uint16_t nb_re_sci1 = nb_symb_cch * NB_RB_SCI1 * NR_NB_SC_PER_RB;
  uint16_t polar_encoder_output_len = polar_encoder_output_length(code_rate, harq->num_of_mod_symbols);
  uint8_t  SCI2_mod_order = 2;
  uint16_t nb_re_sci2 = polar_encoder_output_len / SCI2_mod_order;
  unsigned int TBS = nr_compute_tbs_sl(mod_order, code_rate, nb_rb, nb_re_sci1, nb_re_sci2, nb_symb_sch, nb_re_dmrs * length_dmrs, 0, 0, Nl);
  harq->G = nr_get_G_sl(nb_rb, nb_re_sci1, nb_re_sci2, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, Nl);

  LOG_D(NR_PHY, "mcs %u polar_encoder_output_len %u, code_rate %d, TBS %d\n", Imcs, polar_encoder_output_len, code_rate, TBS);
  harq->pssch_pdu.pssch_data.tb_size  = TBS >> 3;

  harq->status = ACTIVE;
  unsigned char *test_input = harq->a;
  uint64_t *sci_input = harq->a_sci2;
 
  char *sl_user_msg = get_softmodem_params()->sl_user_msg;
  
  struct timespec t;
  if (clock_gettime(CLOCK_REALTIME, &t) == -1) abort();

  char timestamp_str[32];
  snprintf(timestamp_str, sizeof(timestamp_str), "@%lu.%06lu ", t.tv_sec, t.tv_nsec / 1000);
  
  int recv_len = recvfrom(udp_socket, test_input, TBS / 8, MSG_DONTWAIT, NULL, NULL);

  if (recv_len > 0) {
	LOG_I(NR_PHY, "Received UDP payload to transmit: len=%d\n", recv_len);
	} 
  else {
	// fallback，如果没有接收到，就发默认数据
	for (int i = 0; i < min(32, TBS / 8); i++)
		test_input[i] = '0' + (unsigned char) (i + 3) % 10;
	LOG_I(NR_PHY, "No UDP payload, sending default message.\n");
}
  if(qsize_of_relay_data() == 0) {
     
    uint64_t u = 0;
    uint64_t dest_id = (0x2 + ((slot % 2) == 0 ? 1 : 0)) * get_softmodem_params()->node_number;
    dest_id = (0x2 + ((slot % 2) == 0 ? 1 : 0)) * get_softmodem_params()->node_number;
    if (get_softmodem_params()->dest_id > 0)
      dest_id = get_softmodem_params()->dest_id;
    u ^= (dest_id << 32);
    *sci_input = u;
  } else {
    get_relay_data_from_buffer(test_input, sci_input, TBS / 8);
    uint64_t dest = (*sci_input >> 32) & 0xFFFF;
    LOG_W(NR_PHY, "SLSCH_TX will forward with original slot index %u for dest %lu\n", test_input[0], dest);
  }
}

void nr_ue_slsch_tx_procedures(PHY_VARS_NR_UE *txUE,
                               unsigned char harq_pid,
                               uint32_t frame,
                               uint8_t slot) {

  LOG_D(NR_PHY, "nr_ue_slsch_tx_procedures hard_id %d %d.%d\n", harq_pid, frame, slot);
  LOG_I(NR_PHY, "frame: %d, slot: %d\n", frame, slot);

  NR_DL_FRAME_PARMS *frame_parms = &txUE->frame_parms;
  int32_t **txdataF = txUE->common_vars.txdataF;

  NR_UE_ULSCH_t *slsch_ue = txUE->slsch[0][0];
  NR_UL_UE_HARQ_t *harq_process_ul_ue = slsch_ue->harq_processes[harq_pid];
  nfapi_nr_ue_pssch_pdu_t *pssch_pdu = &harq_process_ul_ue->pssch_pdu;

  uint8_t number_of_symbols = pssch_pdu->nr_of_symbols;
  uint16_t nb_rb            = pssch_pdu->rb_size;
  uint8_t Nl                = pssch_pdu->nrOfLayers;
  uint8_t mod_order         = pssch_pdu->qam_mod_order;

  /////////////////////////SLSCH data and SCI2 encoding/////////////////////////
  unsigned int G_slsch_bits = harq_process_ul_ue->G;

  if (nr_slsch_encoding(txUE, slsch_ue, frame_parms, harq_pid, G_slsch_bits) == -1)
    return;
  unsigned int G_SCI2_bits = harq_process_ul_ue->B_sci2;

  //////////////////SLSCH data and control multiplexing//////////////
  uint32_t available_bits = 0;
  uint32_t M_SCI2_bits = G_SCI2_bits * Nl;
  uint32_t M_data_bits = G_slsch_bits;
  available_bits += M_SCI2_bits;
  available_bits += M_data_bits;
  uint8_t  SCI2_mod_order = 2;

  nr_pssch_data_control_multiplexing(harq_process_ul_ue->f,
                                     harq_process_ul_ue->f_sci2,
                                     G_slsch_bits,
                                     harq_process_ul_ue->B_sci2,
                                     Nl,
                                     SCI2_mod_order,
                                     harq_process_ul_ue->f_multiplexed);

  /////////////////////////SLSCH scrambling/////////////////////////
  uint16_t Nidx = slsch_ue->Nidx;
  // +1 because size can be not modulo 4
  uint32_t scrambled_output[harq_process_ul_ue->B_multiplexed  / (8 * sizeof(uint32_t)) + 1];
  memset(scrambled_output, 0, sizeof(scrambled_output));

  nr_pusch_codeword_scrambling_sl(harq_process_ul_ue->f_multiplexed,
                                  harq_process_ul_ue->B_multiplexed,
                                  M_SCI2_bits,
                                  Nidx,
                                  scrambled_output);
  #ifdef DEBUG_PSSCH_MAPPING
    char filename[40];
    sprintf(filename,"scramble_output.m");
    LOG_M(filename,"scramble_output",&scrambled_output,(harq_process_ul_ue->B_multiplexed >> 5) + 1, 1, 13);
  #endif
  /////////////////////////SLSCH modulation/////////////////////////

  int max_num_re = Nl * number_of_symbols * nb_rb * NR_NB_SC_PER_RB;
  int32_t d_mod[max_num_re] __attribute__ ((aligned(16)));

  // modulating for the 2nd-stage SCI bits
  nr_modulation(scrambled_output, // assume one codeword for the moment
                M_SCI2_bits,
                SCI2_mod_order,
                (int16_t *)d_mod);

  // modulating SL-SCH bits
  nr_modulation(scrambled_output + (M_SCI2_bits >> 5), // assume one codeword for the moment
                G_slsch_bits,
                mod_order,
                (int16_t *)(d_mod + M_SCI2_bits / SCI2_mod_order));

  /////////////////////////SLSCH layer mapping/////////////////////////

  int16_t **tx_layers = (int16_t **)malloc16_clear(Nl * sizeof(int16_t *));
  uint16_t num_sci2_symbs = (M_SCI2_bits << 1) / SCI2_mod_order;
  uint16_t num_data_symbs = (M_data_bits << 1) / mod_order;
  uint32_t num_sum_symbs = (num_sci2_symbs + num_data_symbs) >> 1;
  for (int nl = 0; nl < Nl; nl++)
    tx_layers[nl] = (int16_t *)malloc16_clear((num_sci2_symbs + num_data_symbs) * sizeof(int16_t));

  nr_ue_layer_mapping((int16_t *)d_mod, Nl, num_sum_symbs, tx_layers);

  /////////////////////////DMRS Modulation/////////////////////////

  nr_init_pssch_dmrs(txUE, Nidx);

  uint32_t **pssch_dmrs = txUE->nr_gold_pssch_dmrs[slot];


  ////////////////SLSCH Mapping to virtual resource blocks////////////////

  int16_t** tx_precoding = virtual_resource_mapping(frame_parms, pssch_pdu, G_SCI2_bits, SCI2_mod_order, tx_layers, pssch_dmrs);


  /////////SLSCH Mapping from virtual to physical resource blocks/////////

  physical_resource_mapping(frame_parms, pssch_pdu, tx_precoding, txdataF);

  NR_UL_UE_HARQ_t *harq_process_slsch = NULL;
  harq_process_slsch = slsch_ue->harq_processes[harq_pid];
  harq_process_slsch->status = SCH_IDLE;

  for (int nl = 0; nl < Nl; nl++) {
    free_and_zero(tx_layers[nl]);
    free_and_zero(tx_precoding[nl]);
  }
  free_and_zero(tx_layers);
  free_and_zero(tx_precoding);
  ////////////////////////OFDM modulation/////////////////////////////
  nr_ue_pssch_common_procedures(txUE, slot, &txUE->frame_parms, Nl, link_type_sl);
}

int16_t** virtual_resource_mapping(NR_DL_FRAME_PARMS *frame_parms,
                              nfapi_nr_ue_pssch_pdu_t *pssch_pdu,
                              unsigned int G_SCI2_bits,
                              uint8_t  SCI2_mod_order,
                              int16_t **tx_layers,
                              uint32_t **pssch_dmrs
                             ) {
  uint8_t mod_order = pssch_pdu->qam_mod_order;
  int start_symbol  = pssch_pdu->start_symbol_index;
  uint8_t Nl        = pssch_pdu->nrOfLayers;
  uint8_t number_of_symbols = pssch_pdu->nr_of_symbols;
  uint16_t start_rb         = pssch_pdu->rb_start;
  uint16_t bwp_start        = pssch_pdu->bwp_start;
  uint16_t sl_dmrs_symb_pos = pssch_pdu->sl_dmrs_symb_pos;
  uint16_t nb_rb            = pssch_pdu->rb_size;
  uint8_t dmrs_type         = pssch_pdu->dmrs_config_type;
  int16_t Wf[2], Wt[2];
  int sample_offsetF, l_prime[2], delta;
  uint16_t start_sc = frame_parms->first_carrier_offset + (start_rb + bwp_start) * NR_NB_SC_PER_RB;
  uint16_t n_dmrs = (bwp_start + start_rb + nb_rb) * ((dmrs_type == pusch_dmrs_type1) ? 6 : 4);
  int16_t mod_dmrs[n_dmrs << 1] __attribute((aligned(16)));
  if (start_sc >= frame_parms->ofdm_symbol_size)
    start_sc -= frame_parms->ofdm_symbol_size;

  l_prime[0] = 0; // single symbol ap 0
  pssch_pdu->dmrs_ports = (Nl == 2) ? 0b11 : 0b01;
  uint16_t M_SCI2_Layer = G_SCI2_bits / SCI2_mod_order;
  uint16_t nb_re_sci1 = NB_RB_SCI1 * NR_NB_SC_PER_RB; //NB_RB_SCI1 needs to be from parameter.

  int encoded_length = frame_parms->N_RB_SL * 14 * NR_NB_SC_PER_RB * mod_order * Nl;
  int16_t **tx_precoding = (int16_t **)malloc16_clear(Nl * sizeof(int16_t *));
  for (int nl = 0; nl < Nl; nl++)
    tx_precoding[nl] = (int16_t *)malloc16_clear((encoded_length << 1) * sizeof(int16_t));

  for (int nl = 0; nl < Nl; nl++) {
    uint8_t k_prime = 0;
    uint16_t m = M_SCI2_Layer;
    uint16_t m0 = 0;
    int dmrs_port = get_dmrs_port(nl, pssch_pdu->dmrs_ports);
    // DMRS params for this dmrs port
    get_Wt_sl(Wt, dmrs_port);
    get_Wf_sl(Wf, dmrs_port);
    delta = get_delta_sl(dmrs_port);

    for (int l = start_symbol; l < start_symbol + number_of_symbols; l++) {
      uint16_t k = (1 <= l && l <= 3) ? start_sc + nb_re_sci1 : start_sc;
      uint16_t n = 0;
      uint8_t is_dmrs_sym = 0;
      uint16_t dmrs_idx = 0;

      if ((sl_dmrs_symb_pos >> l) & 0x01) {
        is_dmrs_sym = 1;

        if (pssch_pdu->transform_precoding == transformPrecoder_disabled) {
            dmrs_idx = (bwp_start + start_rb) * 6;
          // TODO: performance improvement, we can skip the modulation of DMRS symbols outside the bandwidth part
          // Perform this on gold sequence, not required when SC FDMA operation is done,
          LOG_D(NR_PHY, "DMRS in symbol %d\n", l);
          nr_modulation(pssch_dmrs[l], n_dmrs * 2, DMRS_MOD_ORDER, mod_dmrs); // currently only codeword 0 is modulated. Qm = 2 as DMRS is QPSK modulated

          #ifdef DEBUG_PSSCH_MAPPING
          char filename[40];
          if (l == 4){
            sprintf(filename, "tx_dmrs_output_4.m");
            LOG_M(filename, "tx_dmrs_output", mod_dmrs, 1200, 1, 3);
          }
          #endif
        } else {
            dmrs_idx = 0;
        }
      }

      for (int i = k; i < nb_rb * NR_NB_SC_PER_RB; i++) {
        uint8_t is_dmrs = 0;

        sample_offsetF = l * frame_parms->ofdm_symbol_size + k;

        if (is_dmrs_sym) {
          if (k == ((start_sc + get_dmrs_freq_idx_ul(n, k_prime, delta, dmrs_type)) % frame_parms->ofdm_symbol_size))
            is_dmrs = 1;
        }

        if (is_dmrs == 1) {
          ((int16_t *)tx_precoding[nl])[(sample_offsetF) << 1] = (Wt[l_prime[0]] * Wf[k_prime] * AMP * mod_dmrs[dmrs_idx << 1]) >> 15;
          ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1] = (Wt[l_prime[0]] * Wf[k_prime] * AMP * mod_dmrs[(dmrs_idx << 1) + 1]) >> 15;

#ifdef DEBUG_PSSCH_MAPPING
          printf("DMRS: Layer: %d\t, dmrs_idx %d\t l %d \t k %d \t k_prime %d \t n %d \t dmrs: %d %d\n",
                 nl, dmrs_idx, l, k, k_prime, n, ((int16_t*)tx_precoding[nl])[(sample_offsetF) << 1],
                 ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1]);
#endif

          dmrs_idx++;
          k_prime++;
          k_prime &= 1;
          n += (k_prime) ? 0 : 1;

        } else if (!is_dmrs_sym) {
          ((int16_t *)tx_precoding[nl])[(sample_offsetF) << 1]       = ((int16_t *)tx_layers[nl])[m << 1];
          ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1] = ((int16_t *)tx_layers[nl])[(m << 1) + 1];

#ifdef DEBUG_PSSCH_MAPPING
          printf("DATA: layer %d\t m %d\t l %d \t k %d \t tx_precoding: %d %d\n",
                 nl, m, l, k, ((int16_t *)tx_precoding[nl])[(sample_offsetF) << 1],
                 ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1]);
#endif
          m++;
        } else if ((is_dmrs_sym) && (is_dmrs != 1)) {
          if (m0 < M_SCI2_Layer) {
            ((int16_t *)tx_precoding[nl])[(sample_offsetF) << 1]       = ((int16_t *)tx_layers[nl])[m0 << 1];
            ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1] = ((int16_t *)tx_layers[nl])[(m0 << 1) + 1];
            m0++;
          } else {
            ((int16_t *)tx_precoding[nl])[(sample_offsetF) << 1]       = ((int16_t *)tx_layers[nl])[m << 1];
            ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1] = ((int16_t *)tx_layers[nl])[(m << 1) + 1];
            m++;
          }
        } else {
          ((int16_t *)tx_precoding[nl])[(sample_offsetF) << 1]       = 0;
          ((int16_t *)tx_precoding[nl])[((sample_offsetF) << 1) + 1] = 0;
        }
        if (++k >= frame_parms->ofdm_symbol_size)
          k -= frame_parms->ofdm_symbol_size;
      } //for (i = 0; i < nb_rb * NR_NB_SC_PER_RB; i++)
    } //for (l = start_symbol; l < start_symbol + number_of_symbols; l++)
  } //for (nl = 0; nl < Nl; nl++)
  return tx_precoding;
}

void physical_resource_mapping(NR_DL_FRAME_PARMS *frame_parms,
                               nfapi_nr_ue_pssch_pdu_t *pssch_pdu,
                               int16_t** tx_precoding,
                               int32_t **txdataF
                              ) {
  uint16_t nb_re_sci1 = NB_RB_SCI1 * NR_NB_SC_PER_RB; //NB_RB_SCI1 needs to be from parameter.
  uint16_t start_rb   = pssch_pdu->rb_start;
  uint16_t bwp_start  = pssch_pdu->bwp_start;
  uint16_t nb_rb      = pssch_pdu->rb_size;
  int start_symbol    = pssch_pdu->start_symbol_index;
  uint8_t number_of_symbols = pssch_pdu->nr_of_symbols;
  uint16_t start_sc = frame_parms->first_carrier_offset + (start_rb + bwp_start) * NR_NB_SC_PER_RB;
  for (int ap = 0; ap < frame_parms->nb_antennas_tx; ap++) {
    // Copying symbol 1 to symbol 0 (AGC)
    memcpy(&txdataF[ap][start_sc],
          &tx_precoding[ap][2 * (frame_parms->ofdm_symbol_size + start_sc)],
          NR_NB_SC_PER_RB * sizeof(int32_t));

    for (int l = start_symbol; l < start_symbol + number_of_symbols; l++) {
      uint16_t k;
      if (1 <= l && l <= 3) { // Assumption there are three SLCCH symbols
        k = start_sc + nb_re_sci1;
      } else {
        k = start_sc;
      }
      for (int rb = 0; rb < nb_rb; rb++) {
        //get pmi info
        uint8_t pmi = pssch_pdu->Tpmi;
        if (pmi == 0) {//unitary Precoding
          if (k + NR_NB_SC_PER_RB <= frame_parms->ofdm_symbol_size) { // RB does not cross DC
            if (ap < pssch_pdu->nrOfLayers) {
              if (1 <= l && l <= 3)
                memcpy(&txdataF[ap][l * frame_parms->ofdm_symbol_size  + k],
                      &tx_precoding[ap][2 * (l * frame_parms->ofdm_symbol_size + k)],
                      NR_NB_SC_PER_RB * sizeof(int32_t));
              else
                memcpy(&txdataF[ap][l * frame_parms->ofdm_symbol_size  + k],
                      &tx_precoding[ap][2 * (l * frame_parms->ofdm_symbol_size + k)],
                      NR_NB_SC_PER_RB * sizeof(int32_t));
            } else {
                memset(&txdataF[ap][l * frame_parms->ofdm_symbol_size + k],
                      0,
                      NR_NB_SC_PER_RB * sizeof(int32_t));
            }
          } else { // RB does cross DC
            int neg_length = frame_parms->ofdm_symbol_size - k;
            int pos_length = NR_NB_SC_PER_RB - neg_length;
            if (ap < pssch_pdu->nrOfLayers) {
              if (1 <= l && l <= 3)
                memcpy(&txdataF[ap][l * frame_parms->ofdm_symbol_size + k],
                      &tx_precoding[ap][2 * (l * frame_parms->ofdm_symbol_size + k)],
                      neg_length * sizeof(int32_t));
              else
                memcpy(&txdataF[ap][l * frame_parms->ofdm_symbol_size + k],
                      &tx_precoding[ap][2 * (l * frame_parms->ofdm_symbol_size + k)],
                      neg_length * sizeof(int32_t));
              memcpy(&txdataF[ap][l * frame_parms->ofdm_symbol_size],
                    &tx_precoding[ap][2 * (l * frame_parms->ofdm_symbol_size)],
                    pos_length * sizeof(int32_t));
            } else {
              memset(&txdataF[ap][l * frame_parms->ofdm_symbol_size + k],
                    0,
                    neg_length * sizeof(int32_t));
              memset(&txdataF[ap][l * frame_parms->ofdm_symbol_size],
                    0,
                    pos_length * sizeof(int32_t));
            }
          }
          k += NR_NB_SC_PER_RB;
          if (k >= frame_parms->ofdm_symbol_size) {
            k -= frame_parms->ofdm_symbol_size;
          }
        }
      } //RB loop
    } // symbol loop
  } // port loop
}

uint8_t nr_ue_pssch_common_procedures(PHY_VARS_NR_UE *UE,
                                      uint8_t slot,
                                      NR_DL_FRAME_PARMS *frame_parms,
                                      uint8_t n_antenna_ports,
                                      int link_type) {

  int32_t **txdata = UE->common_vars.txdata;
  int32_t **txdataF = UE->common_vars.txdataF;
  int tx_offset = frame_parms->get_samples_slot_timestamp(slot, frame_parms, 0);
  int symb_offset = (slot % frame_parms->slots_per_subframe) * frame_parms->symbols_per_slot;

  for(int ap = 0; ap < n_antenna_ports; ap++) {
    for (int s = 0; s < NR_NUMBER_OF_SYMBOLS_PER_SLOT; s++){
      c16_t *this_symbol = (c16_t *)&txdataF[ap][frame_parms->ofdm_symbol_size * s];
      c16_t rot = frame_parms->symbol_rotation[link_type][s + symb_offset];
      LOG_D(NR_PHY, "offset is %d rotating txdataF symbol %d (%d) => (%d.%d)\n", tx_offset, s, s + symb_offset, rot.r, rot.i);
      if (frame_parms->N_RB_SL & 1) {
        rotate_cpx_vector(this_symbol, &rot, this_symbol,
                          (frame_parms->N_RB_SL + 1) * 6, 15);
        rotate_cpx_vector(this_symbol + frame_parms->first_carrier_offset - 6,
                          &rot,
                          this_symbol + frame_parms->first_carrier_offset - 6,
                          (frame_parms->N_RB_SL + 1) * 6, 15);
      } else {
        rotate_cpx_vector(this_symbol, &rot, this_symbol,
                          frame_parms->N_RB_SL * 6, 15);
        rotate_cpx_vector(this_symbol + frame_parms->first_carrier_offset,
                          &rot,
                          this_symbol + frame_parms->first_carrier_offset,
                          frame_parms->N_RB_SL * 6, 15);
      }
    }
  }

  for (int ap = 0; ap < n_antenna_ports; ap++) {
    if (frame_parms->Ncp == 1) { // extended cyclic prefix
      PHY_ofdm_mod(txdataF[ap],
                   &txdata[ap][tx_offset],
                   frame_parms->ofdm_symbol_size,
                   NR_NUMBER_OF_SYMBOLS_PER_SLOT,
                   frame_parms->nb_prefix_samples,
                   CYCLIC_PREFIX);
    } else { // normal cyclic prefix
      nr_normal_prefix_mod(txdataF[ap],
                           &txdata[ap][tx_offset],
                           NR_NUMBER_OF_SYMBOLS_PER_SLOT,
                           frame_parms,
                           slot);
    }
  }

  return 0;
}

uint32_t nr_ue_slsch_rx_procedures(PHY_VARS_NR_UE *rxUE,
                            unsigned char harq_pid,
                            uint32_t frame,
                            uint8_t slot,
                            int32_t **rxdata,
                            uint32_t multiplex_input_len,
                            uint32_t Nidx,
                            UE_nr_rxtx_proc_t *proc) {
  int UE_id = 0;
  int16_t **ulsch_llr = rxUE->pssch_vars[UE_id]->llr;
  int16_t **ulsch_llr_layers = rxUE->pssch_vars[UE_id]->llr_layers;
  int16_t **ulsch_llr_layers_adj = rxUE->pssch_vars[UE_id]->llr_layers_adj;
  NR_UE_DLSCH_t *slsch_ue_rx = rxUE->slsch_rx[0][0][0];
  NR_DL_UE_HARQ_t *slsch_ue_rx_harq = slsch_ue_rx->harq_processes[harq_pid];
  uint16_t nb_rb          = slsch_ue_rx_harq->nb_rb ;
  uint16_t bwp_start      = slsch_ue_rx_harq->BWPStart;
  uint16_t pssch_start_rb = slsch_ue_rx_harq->start_rb;
  uint16_t start_sym      = slsch_ue_rx_harq->start_symbol;
  uint8_t nb_symb_sch     = slsch_ue_rx_harq->nb_symbols;
  uint8_t mod_order       = slsch_ue_rx_harq->pssch_pdu.qam_mod_order;
  uint16_t dmrs_pos       = slsch_ue_rx_harq->dlDmrsSymbPos;
  uint8_t dmrs_config     = slsch_ue_rx_harq->dmrsConfigType;
  uint8_t SCI2_mod_order  = 2;
  uint8_t Nl              = slsch_ue_rx_harq->Nl;
  // TODO: has to be checked if rx has access to these info.
  int nb_re_SCI2 = slsch_ue_rx->harq_processes[0]->B_sci2/SCI2_mod_order;
  uint8_t nb_re_dmrs = 6 * slsch_ue_rx_harq->n_dmrs_cdm_groups;
  uint32_t dmrs_data_re = 12 - nb_re_dmrs;
  unsigned int G = slsch_ue_rx_harq->G;
  uint16_t num_data_symbs = (G << 1) / mod_order;
  uint32_t M_SCI2_bits = slsch_ue_rx->harq_processes[0]->B_sci2 * Nl;
  uint16_t num_sci2_symbs = (M_SCI2_bits << 1) / SCI2_mod_order;

  int avgs = 0;
  int avg[16];
  int32_t median[16];
  uint32_t rxdataF_ext_offset = 0;
  uint32_t sci2_offset = 0;
  uint32_t data_offset = num_sci2_symbs;
  uint32_t diff_re_comp;

  /////////////// Channel Estimation ///////////////////////

  unsigned short port = 0;
  unsigned char nscid = 0; // it is not used for SL, so should be zero
  unsigned short Nid = Nidx%(1<<16);
  for (int sym = start_sym ; sym < (start_sym+nb_symb_sch) ; sym++){
    if (dmrs_pos & (1 << sym)){
      for (uint8_t aatx=0; aatx<Nl; aatx++) {
        port = get_dmrs_port(aatx,slsch_ue_rx_harq->dmrs_ports);//get_dmrs_port(1,slsch_ue_rx_harq->dmrs_ports);
        if (nr_pdsch_channel_estimation(rxUE, proc, 0, 0, slot, port,
                                        sym,nscid, Nid, bwp_start,dmrs_config,
                                        rxUE->frame_parms.first_carrier_offset + (bwp_start + pssch_start_rb) * 12,
                                        nb_rb) == -1)
          return 1; // Non-zero return represents a failure in nr_ue_slsch_rx_procedures
      }
    }
  }

  if (rxUE->chest_time == 1) { // averaging time domain channel estimates
    nr_chest_time_domain_avg(&rxUE->frame_parms,
                              rxUE->pssch_vars[UE_id]->sl_ch_estimates,
                              nb_symb_sch,
                              start_sym,
                              dmrs_pos,
                              nb_rb);
  }

  nr_ue_sl_pssch_rsrp_measurements(rxUE, harq_pid, 0, proc);

  //----------------------------------------------------------
  //--------------------- RBs extraction ---------------------
  //----------------------------------------------------------

  int first_symbol_flag = 0;
  uint16_t first_symbol_with_data = start_sym;
  while ((dmrs_data_re == 0) && (dmrs_pos & (1 << first_symbol_with_data))) {
    first_symbol_with_data++;
  }

  for (int sym = start_sym ; sym < start_sym + nb_symb_sch ; sym++){
    uint8_t pilots = (dmrs_pos >> sym) & 1;
    uint16_t nb_re_sci1 = 0;
    if (1 <= sym && sym <= 3) {
      nb_re_sci1 = NR_NB_SC_PER_RB * NB_RB_SCI1;
    }
    uint32_t allocatable_sci2_re = min(nb_re_SCI2, (NR_NB_SC_PER_RB * nb_rb - nb_re_sci1) / 2);

    if (sym==first_symbol_with_data)
      first_symbol_flag = 1;
    else
      first_symbol_flag = 0;

    start_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);
    nr_slsch_extract_rbs(rxdata,
                        rxUE->pssch_vars[UE_id],
                        slot,
                        sym,
                        pilots,
                        &slsch_ue_rx_harq->pssch_pdu,
                        &rxUE->frame_parms,
                        slsch_ue_rx_harq,
                        rxUE->chest_time);

#if 0
      char buffer1[rxUE->frame_parms.ofdm_symbol_size * 4];
      bzero(buffer1, sizeof(buffer1));
      printf("Slot %u, RXUE Symbol[%d:%d]: rxdataF_ext %s\n",
            slot, sym, sym * nb_rb * NR_NB_SC_PER_RB,
            hexdump((int16_t *)&rxUE->pssch_vars[UE_id]->rxdataF_ext[0][sym * nb_rb * NR_NB_SC_PER_RB],
                    rxUE->frame_parms.ofdm_symbol_size * 4, buffer1, sizeof(buffer1)));
#endif

    stop_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);
  //----------------------------------------------------------
  //--------------------- Channel Scaling --------------------
  //----------------------------------------------------------
    // Todo: this line should be double check
    #if 1
    int32_t nb_re_pssch = (pilots==1)? (nb_rb*dmrs_data_re) : (nb_rb*12);
    start_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);
    nr_dlsch_scale_channel(rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                          &rxUE->frame_parms,
                          Nl,
                          rxUE->frame_parms.nb_antennas_rx,
                          &slsch_ue_rx,
                          sym,
                          pilots,
                          nb_re_pssch,
                          nb_rb);
    stop_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);

    //----------------------------------------------------------
    //--------------------- Channel Level Calc. ----------------
    //----------------------------------------------------------
    start_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);
    if (first_symbol_flag==1) {
      nr_dlsch_channel_level(rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                            &rxUE->frame_parms,
                            Nl,
                            avg,
                            sym,
                            nb_re_pssch,
                            nb_rb);
      avgs = 0;
      for (int aatx=0;aatx<Nl;aatx++)
        for (int aarx=0;aarx<rxUE->frame_parms.nb_antennas_rx;aarx++) {
          //LOG_I(PHY, "nb_rb %d len %d avg_%d_%d Power per SC is %d\n",nb_rb, len,aarx, aatx,avg[aatx*rxUE->frame_parms.nb_antennas_rx+aarx]);
          avgs = cmax(avgs,avg[(aatx*rxUE->frame_parms.nb_antennas_rx)+aarx]);
          //LOG_I(PHY, "avgs Power per SC is %d\n", avgs);
          median[(aatx*rxUE->frame_parms.nb_antennas_rx)+aarx] = avg[(aatx*rxUE->frame_parms.nb_antennas_rx)+aarx];
        }
      if (slsch_ue_rx_harq->Nl > 1) {
        nr_dlsch_channel_level_median(rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                                      median,
                                      Nl,
                                      rxUE->frame_parms.nb_antennas_rx,
                                      nb_re_pssch,
                                      sym*nb_rb*12);
        for (int aatx = 0; aatx < Nl; aatx++) {
          for (int aarx = 0; aarx < rxUE->frame_parms.nb_antennas_rx; aarx++) {
            avgs = cmax(avgs, median[aatx*rxUE->frame_parms.nb_antennas_rx + aarx]);
          }
        }
      }
      rxUE->pssch_vars[UE_id]->log2_maxh = (log2_approx(avgs)/2) + 1;
      //LOG_I(PHY, "avgs Power per SC is %d lg2_maxh %d\n", avgs,  rxUE->pssch_vars[UE_id]->log2_maxh);
      LOG_D(PHY,"[SLSCH] AbsSubframe %d.%d log2_maxh = %d [log2_maxh0 %d log2_maxh1 %d] (%d,%d)\n",
            frame%1024,
            slot,
            rxUE->pssch_vars[UE_id]->log2_maxh,
            rxUE->pssch_vars[UE_id]->log2_maxh0,
            rxUE->pssch_vars[UE_id]->log2_maxh1,
            avg[0],
            avgs);
    }
    stop_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);

  /////////////////////////////////////////////////////////
  ////////////// Channel Compensation /////////////////////
    start_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);

    if (pilots==0){
      nr_dlsch_channel_compensation(rxUE->pssch_vars[UE_id]->rxdataF_ext,
                                    rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                                    rxUE->pssch_vars[UE_id]->sl_ch_mag0,
                                    rxUE->pssch_vars[UE_id]->sl_ch_magb0,
                                    rxUE->pssch_vars[UE_id]->sl_ch_magr0,
                                    rxUE->pssch_vars[UE_id]->rxdataF_comp,
                                    NULL,//NULL:disable meas. rxUE->pssch_vars[UE_id]->rho:enable meas.
                                    &rxUE->frame_parms,
                                    Nl,
                                    sym,
                                    nb_re_pssch,
                                    first_symbol_flag,
                                    slsch_ue_rx_harq->Qm,
                                    nb_rb,
                                    rxUE->pssch_vars[UE_id]->log2_maxh,
                                    &rxUE->measurements,
                                    0); // log2_maxh+I0_shift

    } else { // DMRS symbol
        if (allocatable_sci2_re > 0) {
          // for SCI2
          nr_dlsch_channel_compensation(rxUE->pssch_vars[UE_id]->rxdataF_ext,
                                        rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                                        rxUE->pssch_vars[UE_id]->sl_ch_mag0,
                                        rxUE->pssch_vars[UE_id]->sl_ch_magb0,
                                        rxUE->pssch_vars[UE_id]->sl_ch_magr0,
                                        rxUE->pssch_vars[UE_id]->rxdataF_comp,
                                        NULL,//NULL:disable meas. rxUE->pssch_vars[UE_id]->rho:enable meas.
                                        &rxUE->frame_parms,
                                        Nl,
                                        sym,
                                        allocatable_sci2_re,
                                        first_symbol_flag,
                                        SCI2_mod_order,
                                        nb_rb,
                                        rxUE->pssch_vars[UE_id]->log2_maxh,
                                        &rxUE->measurements,
                                        0);
          diff_re_comp = NR_NB_SC_PER_RB * slsch_ue_rx_harq->nb_rb / 2 - nb_re_sci1 - allocatable_sci2_re;
        } else {
          diff_re_comp = nb_re_pssch;
        }
        nr_dlsch_channel_compensation(rxUE->pssch_vars[UE_id]->rxdataF_ext,
                                      rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                                      rxUE->pssch_vars[UE_id]->sl_ch_mag0,
                                      rxUE->pssch_vars[UE_id]->sl_ch_magb0,
                                      rxUE->pssch_vars[UE_id]->sl_ch_magr0,
                                      rxUE->pssch_vars[UE_id]->rxdataF_comp,
                                      NULL,//NULL:disable meas. rxUE->pssch_vars[UE_id]->rho:enable meas.
                                      &rxUE->frame_parms,
                                      Nl,
                                      sym,
                                      diff_re_comp,
                                      first_symbol_flag,
                                      slsch_ue_rx_harq->Qm,
                                      nb_rb,
                                      rxUE->pssch_vars[UE_id]->log2_maxh,
                                      &rxUE->measurements,
                                      allocatable_sci2_re);
    }

    stop_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);

    start_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);

    if (rxUE->frame_parms.nb_antennas_rx > 1) {
      nr_dlsch_detection_mrc(rxUE->pssch_vars[UE_id]->rxdataF_comp,
                            (Nl>1)? rxUE->pssch_vars[UE_id]->rho : NULL,
                            rxUE->pssch_vars[UE_id]->sl_ch_mag0,
                            rxUE->pssch_vars[UE_id]->sl_ch_magb0,
                            rxUE->pssch_vars[UE_id]->sl_ch_magr0,
                            Nl,
                            rxUE->frame_parms.nb_antennas_rx,
                            sym,
                            nb_rb,
                            nb_re_pssch);
      if (Nl >= 2)//Apply zero forcing for 2, 3, and 4 Tx layers
        nr_zero_forcing_rx(rxUE->pssch_vars[UE_id]->rxdataF_comp,
                          rxUE->pssch_vars[UE_id]->sl_ch_mag0,
                          rxUE->pssch_vars[UE_id]->sl_ch_magb0,
                          rxUE->pssch_vars[UE_id]->sl_ch_magr0,
                          rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext,
                          nb_rb,
                          rxUE->frame_parms.nb_antennas_rx,
                          Nl,
                          slsch_ue_rx_harq->Qm,
                          rxUE->pssch_vars[UE_id]->log2_maxh,
                          sym,
                          nb_re_pssch);
    }
    stop_meas(&rxUE->generic_stat_bis[proc->thread_id][slot]);
#endif
  ////////////////////////////////////////////////////////
  /////////////// LLR calculation ////////////////////////

    for (int aatx = 0; aatx < Nl; aatx++) {
      if (pilots == 0) {
        nr_slsch_compute_llr(&rxUE->pssch_vars[UE_id]->rxdataF_comp[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB],
                             &rxUE->pssch_vars[UE_id]->sl_ch_mag0[aatx*rxUE->frame_parms.nb_antennas_rx][0],
                             &rxUE->pssch_vars[UE_id]->sl_ch_magb0[aatx*rxUE->frame_parms.nb_antennas_rx][0],
                             &ulsch_llr_layers[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB],
                             nb_rb,
                             nb_rb * NR_NB_SC_PER_RB - nb_re_sci1,
                             sym,
                             mod_order);

        memcpy(&ulsch_llr_layers_adj[aatx*rxUE->frame_parms.nb_antennas_rx][data_offset],
                &ulsch_llr_layers[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB],
                sizeof(uint32_t) * (nb_rb * NR_NB_SC_PER_RB - nb_re_sci1) * mod_order / 2);

        rxdataF_ext_offset += nb_rb * NR_NB_SC_PER_RB - nb_re_sci1;
        data_offset += (nb_rb * NR_NB_SC_PER_RB - nb_re_sci1) * mod_order;
      } else {
        if (allocatable_sci2_re > 0) {

          nr_slsch_compute_llr(&rxUE->pssch_vars[UE_id]->rxdataF_comp[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB],
                               &rxUE->pssch_vars[UE_id]->sl_ch_mag0[aatx*rxUE->frame_parms.nb_antennas_rx][0],
                               &rxUE->pssch_vars[UE_id]->sl_ch_magb0[aatx*rxUE->frame_parms.nb_antennas_rx][0],
                               &ulsch_llr_layers[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB],
                               nb_rb,
                               allocatable_sci2_re,
                               sym,
                               SCI2_mod_order);

          memcpy(&ulsch_llr_layers_adj[aatx*rxUE->frame_parms.nb_antennas_rx][sci2_offset * 2],
                  &ulsch_llr_layers[aatx*rxUE->frame_parms.nb_antennas_rx][sym * slsch_ue_rx_harq->nb_rb * NR_NB_SC_PER_RB],
                  sizeof(uint32_t) * allocatable_sci2_re);

          sci2_offset += allocatable_sci2_re;
        }
        uint32_t diff_re = (NR_NB_SC_PER_RB * nb_rb - nb_re_sci1)/ 2  - allocatable_sci2_re;
        if (diff_re > 0) {
          uint32_t offset = allocatable_sci2_re;

          nr_slsch_compute_llr(&rxUE->pssch_vars[UE_id]->rxdataF_comp[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB + offset],
                               &rxUE->pssch_vars[UE_id]->sl_ch_mag0[aatx*rxUE->frame_parms.nb_antennas_rx][0],
                               &rxUE->pssch_vars[UE_id]->sl_ch_magb0[aatx*rxUE->frame_parms.nb_antennas_rx][0],
                               &ulsch_llr_layers[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB + offset],
                               nb_rb,
                               diff_re,
                               sym,
                               mod_order);

          memcpy(&ulsch_llr_layers_adj[aatx*rxUE->frame_parms.nb_antennas_rx][data_offset],
                &ulsch_llr_layers[aatx*rxUE->frame_parms.nb_antennas_rx][sym * nb_rb * NR_NB_SC_PER_RB + offset],
                sizeof(uint32_t) * diff_re * mod_order / 2);

          data_offset += diff_re * mod_order;
        }
        rxdataF_ext_offset += nb_rb * NR_NB_SC_PER_RB - nb_re_sci1;
        if (allocatable_sci2_re > 0) {
          nb_re_SCI2 -= allocatable_sci2_re;
        }
      }
    }
  }//symbol

  #ifdef DEBUG_PSSCH_MAPPING
  sprintf(filename,"ch_est_ext_output_%d.m", slot);
  LOG_M(filename,"ch_est_ext_output",rxUE->pssch_vars[UE_id]->sl_ch_estimates_ext[0],5*(rxUE->frame_parms.ofdm_symbol_size), 1, 13);
  sprintf(filename,"rxdata_ext_%d.m", slot);
  LOG_M(filename,"rxdata_ext",rxUE->pssch_vars[UE_id]->rxdataF_ext[0],5*(rxUE->frame_parms.ofdm_symbol_size), 1, 13);
  sprintf(filename,"rxdata_comp_%d.m", slot);
  LOG_M(filename,"rxdata_comp",rxUE->pssch_vars[UE_id]->rxdataF_comp[0],5*(rxUE->frame_parms.ofdm_symbol_size), 1, 13);
  sprintf(filename,"ulsch_llr_layers_adj.m");
  LOG_M(filename,"ulsch_llr_layers_adj",ulsch_llr_layers_adj[0],5*(rxUE->frame_parms.ofdm_symbol_size), 1, 13);
  #endif
  /////////////// Layer demapping ////////////////////////
  // For SCI2
  nr_dlsch_layer_demapping(ulsch_llr,
                          Nl,
                          SCI2_mod_order,
                          num_sci2_symbs,
                          slsch_ue_rx_harq->codeword,
                          -1,
                          ulsch_llr_layers_adj);

  int16_t *dst_data = ulsch_llr[0] + num_sci2_symbs * slsch_ue_rx_harq->Nl;
  int16_t *src_data = ulsch_llr_layers_adj[0] + num_sci2_symbs;
  // For Data
  nr_dlsch_layer_demapping(&dst_data,
                          Nl,
                          mod_order,
                          num_data_symbs,
                          slsch_ue_rx_harq->codeword,
                          -1,
                          &src_data);
  ////////////////////////////////////////////////////////
  /////////////// Unscrambling ///////////////////////////
  nr_codeword_unscrambling_sl(ulsch_llr[0], multiplex_input_len,
                              slsch_ue_rx->harq_processes[0]->B_sci2,
                              Nidx, Nl);
  ///////////////////////////////////////////////////////
  #ifdef DEBUG_PSSCH_MAPPING
  sprintf(filename,"llr_decoding.m");
  LOG_M(filename,"llr_decoding",ulsch_llr[0],5*(rxUE->frame_parms.ofdm_symbol_size), 1, 13);
  #endif
  /////////////// Decoding SLSCH and SCI2 //////////////
  uint32_t ret = nr_slsch_decoding(rxUE, proc, ulsch_llr[0],
                            &rxUE->frame_parms, slsch_ue_rx,
                            slsch_ue_rx->harq_processes[0], frame,
                            nb_symb_sch, slot, harq_pid);
  ///////////////////////////////////////////////////////
  return ret;

}
