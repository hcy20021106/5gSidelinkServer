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

#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include <openair2/UTIL/OPT/opt.h>
#include <inttypes.h>
#include "executables/softmodem-common.h"

//#define DEBUG_SLSCH_CODING
#define NR_POLAR_SCI2_MESSAGE_TYPE 4
#define NR_POLAR_SCI2_PAYLOAD_BITS 35
#define NR_POLAR_SCI2_AGGREGATION_LEVEL 0

void free_nr_ue_slsch(NR_UE_ULSCH_t **slschptr,
                      uint16_t N_RB_UL,
                      NR_DL_FRAME_PARMS* frame_parms) {

  NR_UE_ULSCH_t *slsch = *slschptr;

  int max_layers = (frame_parms->nb_antennas_tx < NR_MAX_NB_LAYERS_SL) ? frame_parms->nb_antennas_tx : NR_MAX_NB_LAYERS_SL;
  uint16_t a_segments = MAX_NUM_NR_SLSCH_SEGMENTS_PER_LAYER * max_layers;  //number of segments to be allocated
  if (N_RB_UL != MAX_NUM_NR_RB) {
    a_segments = a_segments * N_RB_UL;
    a_segments = a_segments / MAX_NUM_NR_RB + 1;
  }

  for (int i = 0; i < NR_MAX_SLSCH_HARQ_PROCESSES; i++) {
    if (slsch->harq_processes[i]) {
      if (slsch->harq_processes[i]->a) {
        free(slsch->harq_processes[i]->a);
        slsch->harq_processes[i]->a = NULL;
      }
      if (slsch->harq_processes[i]->b) {
        free(slsch->harq_processes[i]->b);
        slsch->harq_processes[i]->b = NULL;
      }
      for (int r=0; r < a_segments; r++) {
        if (slsch->harq_processes[i]->c[r]) {
          free(slsch->harq_processes[i]->c[r]);
          slsch->harq_processes[i]->c[r] = NULL;
        }
        if (slsch->harq_processes[i]->d[r]) {
          free(slsch->harq_processes[i]->d[r]);
          slsch->harq_processes[i]->d[r] = NULL;
        }
      }
      if (slsch->harq_processes[i]->c) {
        free(slsch->harq_processes[i]->c);
        slsch->harq_processes[i]->c = NULL;
      }
      if (slsch->harq_processes[i]->d) {
        free(slsch->harq_processes[i]->d);
        slsch->harq_processes[i]->d = NULL;
      }
      if (slsch->harq_processes[i]->e) {
        free(slsch->harq_processes[i]->e);
        slsch->harq_processes[i]->e = NULL;
      }
      if (slsch->harq_processes[i]->f) {
        free(slsch->harq_processes[i]->f);
        slsch->harq_processes[i]->f = NULL;
      }
      free(slsch->harq_processes[i]);
      slsch->harq_processes[i] = NULL;
    }
  }
  free16(slsch, sizeof(NR_UE_ULSCH_t));
  *slschptr = NULL;
}



NR_UE_ULSCH_t *new_nr_ue_slsch(uint16_t N_RB_UL, int number_of_harq_pids, NR_DL_FRAME_PARMS* frame_parms) {

  NR_UE_ULSCH_t *slsch = malloc16(sizeof(NR_UE_ULSCH_t));
  DevAssert(slsch);
  memset(slsch, 0, sizeof(*slsch));
  slsch->number_harq_processes_for_pusch = NR_MAX_SLSCH_HARQ_PROCESSES;

  int max_layers = (frame_parms->nb_antennas_tx < NR_MAX_NB_LAYERS_SL) ? frame_parms->nb_antennas_tx : NR_MAX_NB_LAYERS_SL;
  uint16_t a_segments = MAX_NUM_NR_SLSCH_SEGMENTS_PER_LAYER * max_layers;  //number of segments to be allocated
  if (N_RB_UL != MAX_NUM_NR_RB) {
    a_segments = a_segments * N_RB_UL;
    a_segments = a_segments / MAX_NUM_NR_RB + 1;
  }
  uint32_t slsch_bytes = a_segments * 1056;  // allocated bytes per segment

  for (int i = 0; i < number_of_harq_pids; i++) {
    slsch->harq_processes[i] = malloc16(sizeof(NR_UL_UE_HARQ_t));
    DevAssert(slsch->harq_processes[i]);
    memset(slsch->harq_processes[i], 0, sizeof(NR_UL_UE_HARQ_t));

    slsch->harq_processes[i]->a = malloc16(slsch_bytes);
    DevAssert(slsch->harq_processes[i]->a);
    bzero(slsch->harq_processes[i]->a, slsch_bytes);

    slsch->harq_processes[i]->a_sci2 = malloc16(slsch_bytes);
    DevAssert(slsch->harq_processes[i]->a_sci2);
    bzero(slsch->harq_processes[i]->a_sci2, slsch_bytes);

    slsch->harq_processes[i]->b = malloc16(slsch_bytes);
    DevAssert(slsch->harq_processes[i]->b);
    bzero(slsch->harq_processes[i]->b, slsch_bytes);

    slsch->harq_processes[i]->b_sci2 = malloc16(slsch_bytes);
    DevAssert(slsch->harq_processes[i]->b_sci2);
    bzero(slsch->harq_processes[i]->b_sci2, slsch_bytes);

    slsch->harq_processes[i]->c = malloc16(a_segments * sizeof(uint8_t *));
    slsch->harq_processes[i]->d = malloc16(a_segments * sizeof(uint16_t *));
    for (int r = 0; r < a_segments; r++) {
      // account for filler in first segment and CRCs for multiple segment case
      slsch->harq_processes[i]->c[r] = malloc16(8448);
      DevAssert(slsch->harq_processes[i]->c[r]);
      bzero(slsch->harq_processes[i]->c[r], 8448);

      slsch->harq_processes[i]->d[r] = malloc16(68 * 384); //max size for coded output
      DevAssert(slsch->harq_processes[i]->d[r]);
      bzero(slsch->harq_processes[i]->d[r], (68 * 384));
    }

    slsch->harq_processes[i]->e = malloc16(NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);
    DevAssert(slsch->harq_processes[i]->e);
    bzero(slsch->harq_processes[i]->e, NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);

    slsch->harq_processes[i]->f = malloc16(NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);
    DevAssert(slsch->harq_processes[i]->f);
    bzero(slsch->harq_processes[i]->f, NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);

    slsch->harq_processes[i]->f_sci2 = malloc16(NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);
    DevAssert(slsch->harq_processes[i]->f_sci2);
    bzero(slsch->harq_processes[i]->f_sci2, NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);

    slsch->harq_processes[i]->f_multiplexed = malloc16(NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);
    DevAssert(slsch->harq_processes[i]->f_multiplexed);
    bzero(slsch->harq_processes[i]->f_multiplexed, NR_SYMBOLS_PER_SLOT * N_RB_UL * 12 * 8);

    slsch->harq_processes[i]->subframe_scheduling_flag = 0;
    slsch->harq_processes[i]->first_tx = 1;
  }

  return(slsch);
}

uint16_t polar_encoder_output_length(uint16_t target_code_rate, uint32_t num_of_mod_symbols) {
  // calculating length of sequence comming out of rate matching for SCI2 based on 8.4.4 TS38212
  uint8_t Osci2 = NR_POLAR_SCI2_PAYLOAD_BITS;
  uint8_t Lsci2 = 24;
  uint8_t Qmsci2 = 2; //modulation order of SCI2
  double beta = 1.125; // TODO: harq_process->pssch_pdu.sci1.beta_offset;
  double alpha = 1; // hardcoded sl-Scaling-r16 for now among {f0p5, f0p65, f0p8, f1}
  float R = (float)target_code_rate / (1024 * 10);

  double tmp1 = (Osci2 + Lsci2) * beta / (Qmsci2 * R);
  /*
  // following lines should be uncommented if number of RB for PSCCH is not 0.
  uint8_t N_sh_sym = harq_process->pssch_pdu.nr_of_symbols;
  uint8_t N_psfch_symb = 0; //hardcoded
  uint8_t N_pssch_symb = N_sh_sym - N_psfch_symb;
  */

  double tmp2 = alpha * num_of_mod_symbols; // it is assumed that number of RB for PSCCH is NB_RB_SCI1.
  uint16_t Qprime = min(ceil(tmp1), ceil(tmp2));
  int spare = 16 - Qprime % 16;
  Qprime += spare;
  uint16_t Gsci2 = Qprime * Qmsci2;
  return Gsci2;
}

void byte2bit(uint8_t *in_bytes, uint8_t *out_bits, uint16_t num_bytes) {

  for (int i=0 ; i<num_bytes ; i++) {
    for (int b=0 ; b<8 ; b++){
      out_bits[i*8 + b] = (in_bytes[i]>>b) & 1;
    }
  }
  return;
}

static void check_Nidx_value(uint32_t nidx, NR_DL_FRAME_PARMS* fp) {
    switch(fp->Imcs) {
    case 0:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 1:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 2:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 3:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 4:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 5:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 6:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 7:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 8:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
    case 9:
      AssertFatal(nidx == 45727, "Invalid nidx size %d for MCS %d!\n", nidx, fp->Imcs);
      break;
  }
}

int nr_slsch_encoding(PHY_VARS_NR_UE *ue,
                      NR_UE_ULSCH_t *slsch,
                      NR_DL_FRAME_PARMS* frame_parms,
                      uint8_t harq_pid,
                      unsigned int G) {

  start_meas(&ue->slsch_encoding_stats);
  NR_UL_UE_HARQ_t *harq_process = slsch->harq_processes[harq_pid];
  uint16_t nb_rb = harq_process->pssch_pdu.rb_size;
  uint32_t A = harq_process->pssch_pdu.pssch_data.tb_size << 3; // payload size in bits
  uint32_t *pz = &harq_process->Z;
  uint8_t mod_order = harq_process->pssch_pdu.qam_mod_order;
  uint16_t Kr = 0;
  uint32_t r_offset = 0;
  uint32_t F = 0;
  // target_code_rate is in 0.1 units
  float Coderate = (float) harq_process->pssch_pdu.target_code_rate / 10240.0f;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_UE_ULSCH_ENCODING, VCD_FUNCTION_IN);
  LOG_D(NR_PHY, "slsch coding nb_rb %d, Nl = %d\n", nb_rb, harq_process->pssch_pdu.nrOfLayers);
  LOG_D(NR_PHY, "slsch coding A %d G %d mod_order %d Coderate %f\n", A, G, mod_order, Coderate);
  LOG_D(NR_PHY, "harq_pid %d harq_process->ndi %d, pssch_data.new_data_indicator %d\n",
        harq_pid, harq_process->ndi, harq_process->pssch_pdu.pssch_data.new_data_indicator);

  if (harq_process->first_tx == 1 || harq_process->ndi != harq_process->pssch_pdu.pssch_data.new_data_indicator) {  // this is a new packet
    harq_process->first_tx = 0;
    ///////////////////////// a---->| add CRC |---->b /////////////////////////

#ifdef DEBUG_SLSCH_CODING
    LOG_D(NR_PHY, "slsch (tx): \n");
    for (int i = 0; i < (A >> 3); i++)
      LOG_D(NR_PHY, "%02x.", harq_process->a[i]);
    LOG_D(NR_PHY, "\n");
    LOG_D(NR_PHY, "encoding thinks this is a new packet \n");
#endif
    int max_payload_bytes = MAX_NUM_NR_SLSCH_SEGMENTS_PER_LAYER * harq_process->pssch_pdu.nrOfLayers * 1056;
    uint16_t polar_encoder_output_len = polar_encoder_output_length(harq_process->pssch_pdu.target_code_rate, harq_process->num_of_mod_symbols);
    polar_encoder_fast(harq_process->a_sci2, (void*)harq_process->b_sci2, 0, 0,
                       NR_POLAR_SCI2_MESSAGE_TYPE,
                       polar_encoder_output_len,
                       NR_POLAR_SCI2_AGGREGATION_LEVEL);
#if 0
    slsch->Nidx = get_Nidx_from_CRC(harq_process->a_sci2, 0, 0,
                                    NR_POLAR_SCI2_MESSAGE_TYPE,
                                    polar_encoder_output_len,
                                    NR_POLAR_SCI2_AGGREGATION_LEVEL);
    check_Nidx_value(slsch->Nidx, frame_parms);
#endif
    slsch->Nidx = 1;
    harq_process->B_sci2 = polar_encoder_output_len;
    byte2bit(harq_process->b_sci2, harq_process->f_sci2, polar_encoder_output_len>>3);
    /*
    for (int i=0 ; i<polar_encoder_output_len ; i++){
      printf("%d",harq_process->f_sci2[i]);
    }
    printf("\n");
    */

    nr_attach_crc_to_payload(harq_process->a, harq_process->b, max_payload_bytes, A, &harq_process->B);

    ///////////////////////// b---->| block segmentation |---->c /////////////////////////

    if ((A <= 292) || ((A <= 3824) && (Coderate <= 0.6667)) || Coderate <= 0.25) {
      harq_process->BG = 2;
    } else {
      harq_process->BG = 1;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_SEGMENTATION, VCD_FUNCTION_IN);
    start_meas(&ue->slsch_segmentation_stats);
    uint32_t  Kb = nr_segmentation(harq_process->b,
                                   harq_process->c,
                                   harq_process->B,
                                   &harq_process->C,
                                   &harq_process->K,
                                   pz,
                                   &harq_process->F,
                                   harq_process->BG);

    if (harq_process->C > MAX_NUM_NR_SLSCH_SEGMENTS_PER_LAYER * harq_process->pssch_pdu.nrOfLayers) {
      LOG_E(NR_PHY, "nr_segmentation.c: too many segments %d, B %d\n", harq_process->C, harq_process->B);
      return(-1);
    }
    stop_meas(&ue->slsch_segmentation_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_SEGMENTATION, VCD_FUNCTION_OUT);
    F = harq_process->F;
    Kr = harq_process->K;
#ifdef DEBUG_SLSCH_CODING
    uint16_t Kr_bytes;
    Kr_bytes = Kr >> 3;
#endif

    ///////////////////////// c---->| LDPC coding |---->d /////////////////////////

#ifdef DEBUG_SLSCH_CODING
    LOG_D(NR_PHY, "segment Z %d k %d Kr %d BG %d\n", *pz, harq_process->K, Kr, harq_process->BG);
    for (int r = 0; r < harq_process->C; r++) {
      //channel_input[r] = &harq_process->d[r][0];
      LOG_D(NR_PHY, "Encoder: B %d F %d \n", harq_process->B, harq_process->F);
      LOG_D(NR_PHY, "start ldpc encoder segment %d/%d\n", r, harq_process->C);
      LOG_D(NR_PHY, "input %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1],
              harq_process->c[r][2], harq_process->c[r][3], harq_process->c[r][4]);
      for (int cnt = 0 ; cnt < 22 * (*pz) / 8; cnt++){
        LOG_D(NR_PHY, "%d ", harq_process->c[r][cnt]);
      }
      LOG_D(NR_PHY, "\n");
      //ldpc_encoder_orig((unsigned char*)harq_process->c[r], harq_process->d[r], Kr, BG, 0);
      //ldpc_encoder_optim((unsigned char*)harq_process->c[r], (unsigned char*)&harq_process->d[r][0], Kr, BG, NULL, NULL, NULL, NULL);
    }
#endif

    encoder_implemparams_t impp = {
      .n_segments = harq_process->C,
      .macro_num = 0,
      .tinput  = NULL,
      .tprep   = NULL,
      .tparity = NULL,
      .toutput = NULL};

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LDPC_ENCODER_OPTIM, VCD_FUNCTION_IN);
    start_meas(&ue->slsch_ldpc_encoding_stats);
    for (int j = 0; j < (harq_process->C / 8 + 1); j++) {
      impp.macro_num = j;
      nrLDPC_encoder(harq_process->c, harq_process->d, *pz, Kb, Kr, harq_process->BG, &impp);
    }
    stop_meas(&ue->slsch_ldpc_encoding_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LDPC_ENCODER_OPTIM, VCD_FUNCTION_OUT);

#ifdef DEBUG_SLSCH_CODING
    LOG_D(NR_PHY, "end ldpc encoder -- output\n");
    write_output("slsch_enc_input0.m", "enc_in0", &harq_process->c[0][0], Kr_bytes, 1, 4);
    write_output("slsch_enc_output0.m", "enc0", &harq_process->d[0][0], (3 * 8 * Kr_bytes) + 12, 1, 4);
#endif

    LOG_D(NR_PHY, "setting ndi to %d from pssch_data\n", harq_process->pssch_pdu.pssch_data.new_data_indicator);
    harq_process->ndi = harq_process->pssch_pdu.pssch_data.new_data_indicator;
  }

  F = harq_process->F;
  Kr = harq_process->K;
  for (int r = 0; r < harq_process->C; r++) { // looping over C segments
    if (harq_process->F > 0) {
      for (int k = (Kr - F - 2 * (*pz)); k < Kr - 2 * (*pz); k++) {
        harq_process->d[r][k] = NR_NULL;
      }
    }

    LOG_D(NR_PHY, "Rate Matching, Code segment %d (coded bits (G) %u, unpunctured/repeated bits per code segment %d, mod_order %d, nb_rb %d, rvidx %d)...\n",
          r, G, Kr*3, mod_order, nb_rb, harq_process->pssch_pdu.pssch_data.rv_index);

///////////////////////// d---->| Rate matching bit selection |---->e /////////////////////////

    uint32_t E = nr_get_E(G, harq_process->C, mod_order, harq_process->pssch_pdu.nrOfLayers, r);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RATE_MATCHING_LDPC, VCD_FUNCTION_IN);
    start_meas(&ue->slsch_rate_matching_stats);
    if (nr_rate_matching_ldpc(harq_process->pssch_pdu.tbslbrm,
                              harq_process->BG,
                              *pz,
                              harq_process->d[r],
                              harq_process->e+r_offset,
                              harq_process->C,
                              F,
                              Kr-F-2*(*pz),
                              harq_process->pssch_pdu.pssch_data.rv_index,
                              E,
                              true) == -1)
      return -1;
    stop_meas(&ue->slsch_rate_matching_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RATE_MATCHING_LDPC, VCD_FUNCTION_OUT);

#ifdef DEBUG_SLSCH_CODING
    for (int i = 0; i < 16; i++)
      LOG_D(NR_PHY, "output ratematching e[%d] = %d r_offset %d\n", i, harq_process->e[i+r_offset], r_offset);
#endif

///////////////////////// e---->| Rate matching bit interleaving |---->f /////////////////////////

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_INTERLEAVING_LDPC, VCD_FUNCTION_IN);
    start_meas(&ue->slsch_interleaving_stats);
    nr_interleaving_ldpc(E,
                         mod_order,
                         harq_process->e+r_offset,
                         harq_process->f+r_offset);
    stop_meas(&ue->slsch_interleaving_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_INTERLEAVING_LDPC, VCD_FUNCTION_OUT);
    r_offset += E;

#ifdef DEBUG_SLSCH_CODING
    for (int i = 0; i < 16; i++)
      LOG_D(NR_PHY, "output interleaving f[%d] = %d r_offset %d\n", i, harq_process->f[i+r_offset], r_offset);
    if (r == harq_process->C - 1)
      write_output("enc_output.m","enc", harq_process->f, G, 1, 4);
#endif
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_UE_ULSCH_ENCODING, VCD_FUNCTION_OUT);
  stop_meas(&ue->slsch_encoding_stats);
  harq_process->B_multiplexed = G + harq_process->B_sci2 * harq_process->pssch_pdu.nrOfLayers;
  // dummy multiplexer
  // harq_process->f_multiplexed = harq_process->f_sci2;
  // int ind = harq_process->B_sci2;
  // for (int i=0 ; i<G ; i++){
  //   harq_process->f_multiplexed[ind] = harq_process->f[i];
  //   ind++;
  // }
#if DEBUG_SLSCH_CODING
  int j = 0;
  for (int i = harq_process->B_sci2 - 10 +1 ; i < harq_process->B_sci2 + 10 ; i++){
    printf("f_mul: %u ",harq_process->f_multiplexed[i]);
    if (i < harq_process->B_sci2){
      printf("f_sci: %u -- %d\n",harq_process->f_sci2[i],i);
    }else{
      printf("f_data: %u-- %d\n",harq_process->f[j],i);
      j++;
    }
  }
  printf("\n");
#endif
  return(0);
}
