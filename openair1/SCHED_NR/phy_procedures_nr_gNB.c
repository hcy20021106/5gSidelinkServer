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

#include "PHY/defs_gNB.h"
#include "sched_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_interface.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface.h"
#include "fapi_nr_l1.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
#include "T.h"
#include "executables/nr-softmodem.h"
#include "executables/softmodem-common.h"

#include "assertions.h"

#include <time.h>

#include "intertask_interface.h"

//#define DEBUG_RXDATA
//#define SRS_IND_DEBUG

uint8_t SSB_Table[38]={0,2,4,6,8,10,12,14,254,254,16,18,20,22,24,26,28,30,254,254,32,34,36,38,40,42,44,46,254,254,48,50,52,54,56,58,60,62};

extern uint8_t nfapi_mode;

void nr_common_signal_procedures (PHY_VARS_gNB *gNB,int frame,int slot,nfapi_nr_dl_tti_ssb_pdu ssb_pdu) {

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  int **txdataF = gNB->common_vars.txdataF;
  uint8_t ssb_index, n_hf;
  uint16_t ssb_start_symbol;
  int txdataF_offset = slot*fp->samples_per_slot_wCP;
  uint16_t slots_per_hf = (fp->slots_per_frame)>>1;

  if (slot<slots_per_hf)
    n_hf=0;
  else
    n_hf=1;

  ssb_index = ssb_pdu.ssb_pdu_rel15.SsbBlockIndex;
  LOG_D(PHY,"common_signal_procedures: frame %d, slot %d ssb index %d\n",frame,slot,ssb_index);

  int ssb_start_symbol_abs = nr_get_ssb_start_symbol(fp,ssb_index); // computing the starting symbol for current ssb
  ssb_start_symbol = ssb_start_symbol_abs % fp->symbols_per_slot;  // start symbol wrt slot

  // setting the first subcarrier
  const int scs = cfg->ssb_config.scs_common.value;
  const int prb_offset = (fp->freq_range == nr_FR1) ? ssb_pdu.ssb_pdu_rel15.ssbOffsetPointA>>scs : ssb_pdu.ssb_pdu_rel15.ssbOffsetPointA>>(scs-2);
  const int sc_offset = (fp->freq_range == nr_FR1) ? ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset>>scs : ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset;
  fp->ssb_start_subcarrier = (12 * prb_offset + sc_offset);
  LOG_D(PHY, "SSB first subcarrier %d (%d,%d)\n", fp->ssb_start_subcarrier, prb_offset, sc_offset);

  LOG_D(PHY,"SS TX: frame %d, slot %d, start_symbol %d\n",frame,slot, ssb_start_symbol);
  nr_generate_pss(&txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);
  nr_generate_sss(&txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);

  if (fp->Lmax == 4)
    nr_generate_pbch_dmrs(gNB->nr_gold_pbch_dmrs[n_hf][ssb_index&7],&txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);
  else
    nr_generate_pbch_dmrs(gNB->nr_gold_pbch_dmrs[0][ssb_index&7],&txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);

  if (T_ACTIVE(T_GNB_PHY_MIB)) {
    unsigned char bch[3];
    bch[0] = ssb_pdu.ssb_pdu_rel15.bchPayload & 0xff;
    bch[1] = (ssb_pdu.ssb_pdu_rel15.bchPayload >> 8) & 0xff;
    bch[2] = (ssb_pdu.ssb_pdu_rel15.bchPayload >> 16) & 0xff;
    T(T_GNB_PHY_MIB, T_INT(0) /* module ID */, T_INT(frame), T_INT(slot), T_BUFFER(bch, 3));
  }

  // Beam_id is currently used only for FR2
  if (fp->freq_range==nr_FR2){
    LOG_D(PHY,"slot %d, ssb_index %d, beam %d\n",slot,ssb_index,cfg->ssb_table.ssb_beam_id_list[ssb_index].beam_id.value);
    for (int j=0;j<fp->symbols_per_slot;j++) 
      gNB->common_vars.beam_id[0][slot*fp->symbols_per_slot+j] = cfg->ssb_table.ssb_beam_id_list[ssb_index].beam_id.value;
  }

  nr_generate_pbch(&ssb_pdu,
                   gNB->nr_pbch_interleaver,
                   &txdataF[0][txdataF_offset],
                   AMP,
                   ssb_start_symbol,
                   n_hf, frame, cfg, fp);
}



void phy_procedures_gNB_TX(processingData_L1tx_t *msgTx,
                           int frame,
                           int slot,
                           int do_meas) {

  int aa;
  PHY_VARS_gNB *gNB = msgTx->gNB;
  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  int offset = gNB->CC_id;
  int txdataF_offset = slot*fp->samples_per_slot_wCP;

  if ((cfg->cell_config.frame_duplex_type.value == TDD) &&
      (nr_slot_select(cfg,frame,slot) == NR_UPLINK_SLOT)) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_gNB_TX+offset,1);

  // clear the transmit data array and beam index for the current slot
  for (aa=0; aa<cfg->carrier_config.num_tx_ant.value; aa++) {
    memset(&gNB->common_vars.txdataF[aa][txdataF_offset],0,fp->samples_per_slot_wCP*sizeof(int32_t));
    memset(&gNB->common_vars.beam_id[aa][slot*fp->symbols_per_slot],255,fp->symbols_per_slot*sizeof(uint8_t));
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_gNB_COMMON_TX,1);
  for (int i=0; i<fp->Lmax; i++) {
    if (msgTx->ssb[i].active) {
      nr_common_signal_procedures(gNB,frame,slot,msgTx->ssb[i].ssb_pdu);
      msgTx->ssb[i].active = false;
    }
  }
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_gNB_COMMON_TX,0);

  int num_pdcch_pdus = msgTx->num_ul_pdcch + msgTx->num_dl_pdcch;

  if (num_pdcch_pdus > 0) {
    LOG_D(PHY, "[gNB %d] Frame %d slot %d Calling nr_generate_dci_top (number of UL/DL PDCCH PDUs %d/%d)\n",
	  gNB->Mod_id, frame, slot, msgTx->num_ul_pdcch, msgTx->num_dl_pdcch);
  
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_PDCCH_TX,1);

    nr_generate_dci_top(msgTx, slot,
			&gNB->common_vars.txdataF[0][txdataF_offset],
			AMP, fp);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_PDCCH_TX,0);
  }
 
  if (msgTx->num_pdsch_slot > 0) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DLSCH,1);
    LOG_D(PHY, "PDSCH generation started (%d) in frame %d.%d\n", msgTx->num_pdsch_slot,frame,slot);
    nr_generate_pdsch(msgTx, frame, slot);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DLSCH,0);
  }

  for (int i=0;i<NUMBER_OF_NR_CSIRS_MAX;i++){
    NR_gNB_CSIRS_t *csirs = &msgTx->csirs_pdu[i];
    if (csirs->active == 1) {
      LOG_D(PHY, "CSI-RS generation started in frame %d.%d\n",frame,slot);
      nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *csi_params = &csirs->csirs_pdu.csi_rs_pdu_rel15;
      nr_generate_csi_rs(&gNB->frame_parms, gNB->common_vars.txdataF, AMP, gNB->nr_csi_info, csi_params, slot, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      csirs->active = 0;
    }
  }

//  if ((frame&127) == 0) dump_pdsch_stats(gNB);
  //apply the OFDM symbol rotation here
  for (aa=0; aa<cfg->carrier_config.num_tx_ant.value; aa++) {
    apply_nr_rotation(fp,(int16_t*) &gNB->common_vars.txdataF[aa][txdataF_offset],slot,0,fp->Ncp==EXTENDED?12:14, link_type_dl);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_gNB_TX+offset,0);
}

void nr_postDecode(PHY_VARS_gNB *gNB, notifiedFIFO_elt_t *req) {
  ldpcDecode_t *rdata = (ldpcDecode_t*) NotifiedFifoData(req);
  NR_UL_gNB_HARQ_t *ulsch_harq = rdata->ulsch_harq;
  NR_gNB_ULSCH_t *ulsch = rdata->ulsch;
  int r = rdata->segment_r;
  nfapi_nr_pusch_pdu_t *pusch_pdu = &gNB->ulsch[rdata->ulsch_id]->harq_processes[rdata->harq_pid]->ulsch_pdu;
  bool decodeSuccess = (rdata->decodeIterations <= rdata->decoderParms.numMaxIter);
  ulsch_harq->processedSegments++;
  LOG_D(PHY, "processing result of segment: %d, processed %d/%d\n",
	rdata->segment_r, ulsch_harq->processedSegments, rdata->nbSegments);
  gNB->nbDecode--;
  LOG_D(PHY,"remain to decoded in subframe: %d\n", gNB->nbDecode);
  if (decodeSuccess) {
    memcpy(ulsch_harq->b+rdata->offset,
           ulsch_harq->c[r],
           rdata->Kr_bytes - (ulsch_harq->F>>3) -((ulsch_harq->C>1)?3:0));

  } else {
    if ( rdata->nbSegments != ulsch_harq->processedSegments ) {
      int nb = abortTpoolJob(&gNB->threadPool, req->key);
      nb += abortNotifiedFIFOJob(&gNB->respDecode, req->key);
      gNB->nbDecode-=nb;
      LOG_D(PHY,"uplink segment error %d/%d, aborted %d segments\n",rdata->segment_r,rdata->nbSegments, nb);
      LOG_D(PHY, "ULSCH %d in error\n",rdata->ulsch_id);
      AssertFatal(ulsch_harq->processedSegments+nb == rdata->nbSegments,"processed: %d, aborted: %d, total %d\n",
		  ulsch_harq->processedSegments, nb, rdata->nbSegments);
      ulsch_harq->processedSegments=rdata->nbSegments;
    }
  }

  //int dumpsig=0;
  // if all segments are done
  if (rdata->nbSegments == ulsch_harq->processedSegments) {
    if (decodeSuccess && !gNB->pusch_vars[rdata->ulsch_id]->DTX) {
      LOG_D(PHY,"[gNB %d] ULSCH: Setting ACK for SFN/SF %d.%d (pid %d, ndi %d, status %d, round %d, TBS %d, Max interation (all seg) %d)\n",
            gNB->Mod_id,ulsch_harq->frame,ulsch_harq->slot,rdata->harq_pid,pusch_pdu->pusch_data.new_data_indicator,ulsch_harq->status,ulsch_harq->round,ulsch_harq->TBS,rdata->decodeIterations);
      ulsch_harq->status = SCH_IDLE;
      ulsch_harq->round  = 0;
      ulsch->harq_mask &= ~(1 << rdata->harq_pid);

      LOG_D(PHY, "ULSCH received ok \n");
      nr_fill_indication(gNB,ulsch_harq->frame, ulsch_harq->slot, rdata->ulsch_id, rdata->harq_pid, 0,0);
      //dumpsig=1;
    } else {
      LOG_D(PHY,"[gNB %d] ULSCH: Setting NAK for SFN/SF %d/%d (pid %d, ndi %d, status %d, round %d, RV %d, prb_start %d, prb_size %d, TBS %d) r %d\n",
            gNB->Mod_id, ulsch_harq->frame, ulsch_harq->slot,
            rdata->harq_pid, pusch_pdu->pusch_data.new_data_indicator, ulsch_harq->status,
	          ulsch_harq->round,
            ulsch_harq->ulsch_pdu.pusch_data.rv_index,
	          ulsch_harq->ulsch_pdu.rb_start,
	          ulsch_harq->ulsch_pdu.rb_size,
	          ulsch_harq->TBS,
	          r);
      ulsch_harq->handled  = 1;

      LOG_D(PHY, "ULSCH %d in error\n",rdata->ulsch_id);
      nr_fill_indication(gNB,ulsch_harq->frame, ulsch_harq->slot, rdata->ulsch_id, rdata->harq_pid, 1,0);
//      dumpsig=1;
    }
/*
    if (ulsch_harq->ulsch_pdu.mcs_index == 0 && dumpsig==1) {
#ifdef __AVX2__
      int off = ((ulsch_harq->ulsch_pdu.rb_size&1) == 1)? 4:0;
#else
      int off = 0;
#endif

      LOG_M("rxsigF0.m","rxsF0",&gNB->common_vars.rxdataF[0][(ulsch_harq->slot&3)*gNB->frame_parms.ofdm_symbol_size*gNB->frame_parms.symbols_per_slot],gNB->frame_parms.ofdm_symbol_size*gNB->frame_parms.symbols_per_slot,1,1);
      LOG_M("rxsigF0_ext.m","rxsF0_ext",
             &gNB->pusch_vars[0]->rxdataF_ext[0][ulsch_harq->ulsch_pdu.start_symbol_index*NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size],ulsch_harq->ulsch_pdu.nr_of_symbols*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size)),1,1);
      LOG_M("chestF0.m","chF0",
            &gNB->pusch_vars[0]->ul_ch_estimates[0][ulsch_harq->ulsch_pdu.start_symbol_index*gNB->frame_parms.ofdm_symbol_size],gNB->frame_parms.ofdm_symbol_size,1,1);
      LOG_M("chestF0_ext.m","chF0_ext",
            &gNB->pusch_vars[0]->ul_ch_estimates_ext[0][(ulsch_harq->ulsch_pdu.start_symbol_index+1)*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size))],
            (ulsch_harq->ulsch_pdu.nr_of_symbols-1)*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size)),1,1);
      LOG_M("rxsigF0_comp.m","rxsF0_comp",
            &gNB->pusch_vars[0]->rxdataF_comp[0][ulsch_harq->ulsch_pdu.start_symbol_index*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size))],ulsch_harq->ulsch_pdu.nr_of_symbols*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size)),1,1);
      LOG_M("rxsigF0_llr.m","rxsF0_llr",
            &gNB->pusch_vars[0]->llr[0],(ulsch_harq->ulsch_pdu.nr_of_symbols-1)*NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size * ulsch_harq->ulsch_pdu.qam_mod_order,1,0);
      if (gNB->frame_parms.nb_antennas_rx > 1) {

        LOG_M("rxsigF1_ext.m","rxsF0_ext",
               &gNB->pusch_vars[0]->rxdataF_ext[1][ulsch_harq->ulsch_pdu.start_symbol_index*NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size],ulsch_harq->ulsch_pdu.nr_of_symbols*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size)),1,1);
        LOG_M("chestF1.m","chF1",
              &gNB->pusch_vars[0]->ul_ch_estimates[1][ulsch_harq->ulsch_pdu.start_symbol_index*gNB->frame_parms.ofdm_symbol_size],gNB->frame_parms.ofdm_symbol_size,1,1);
        LOG_M("chestF1_ext.m","chF1_ext",
              &gNB->pusch_vars[0]->ul_ch_estimates_ext[1][(ulsch_harq->ulsch_pdu.start_symbol_index+1)*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size))],
              (ulsch_harq->ulsch_pdu.nr_of_symbols-1)*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size)),1,1);
        LOG_M("rxsigF1_comp.m","rxsF1_comp",
              &gNB->pusch_vars[0]->rxdataF_comp[1][ulsch_harq->ulsch_pdu.start_symbol_index*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size))],ulsch_harq->ulsch_pdu.nr_of_symbols*(off+(NR_NB_SC_PER_RB * ulsch_harq->ulsch_pdu.rb_size)),1,1);
      }
      exit(-1);

    } 
*/
    ulsch->last_iteration_cnt = rdata->decodeIterations;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING,0);
  }
}


void nr_ulsch_procedures(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx, int ULSCH_id, uint8_t harq_pid)
{
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_pusch_pdu_t *pusch_pdu = &gNB->ulsch[ULSCH_id]->harq_processes[harq_pid]->ulsch_pdu;
  
  uint8_t l, number_dmrs_symbols = 0;
  uint32_t G;
  uint16_t start_symbol, number_symbols, nb_re_dmrs;
  uint8_t enable_ldpc_offload = gNB->ldpc_offload_flag;
  start_symbol = pusch_pdu->start_symbol_index;
  number_symbols = pusch_pdu->nr_of_symbols;

  for (l = start_symbol; l < start_symbol + number_symbols; l++)
    number_dmrs_symbols += ((pusch_pdu->ul_dmrs_symb_pos)>>l)&0x01;

  if (pusch_pdu->dmrs_config_type==pusch_dmrs_type1)
    nb_re_dmrs = 6*pusch_pdu->num_dmrs_cdm_grps_no_data;
  else
    nb_re_dmrs = 4*pusch_pdu->num_dmrs_cdm_grps_no_data;

  G = nr_get_G(pusch_pdu->rb_size,
               number_symbols,
               nb_re_dmrs,
               number_dmrs_symbols, // number of dmrs symbols irrespective of single or double symbol dmrs
               pusch_pdu->qam_mod_order,
               pusch_pdu->nrOfLayers);
  
  AssertFatal(G>0,"G is 0 : rb_size %u, number_symbols %d, nb_re_dmrs %d, number_dmrs_symbols %d, qam_mod_order %u, nrOfLayer %u\n",
	      pusch_pdu->rb_size,
	      number_symbols,
	      nb_re_dmrs,
	      number_dmrs_symbols, // number of dmrs symbols irrespective of single or double symbol dmrs
	      pusch_pdu->qam_mod_order,
	      pusch_pdu->nrOfLayers);
  LOG_D(PHY,"rb_size %d, number_symbols %d, nb_re_dmrs %d, dmrs symbol positions %d, number_dmrs_symbols %d, qam_mod_order %d, nrOfLayer %d\n",
	pusch_pdu->rb_size,
	number_symbols,
	nb_re_dmrs,
        pusch_pdu->ul_dmrs_symb_pos,
	number_dmrs_symbols, // number of dmrs symbols irrespective of single or double symbol dmrs
	pusch_pdu->qam_mod_order,
	pusch_pdu->nrOfLayers);


  nr_ulsch_layer_demapping(gNB->pusch_vars[ULSCH_id]->llr,
                           pusch_pdu->nrOfLayers,
                           pusch_pdu->qam_mod_order,
                           G,
                           gNB->pusch_vars[ULSCH_id]->llr_layers);
             
  //----------------------------------------------------------
  //------------------- ULSCH unscrambling -------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_unscrambling_stats);
  nr_ulsch_unscrambling(gNB->pusch_vars[ULSCH_id]->llr,
                        G,
                        pusch_pdu->data_scrambling_id,
                        pusch_pdu->rnti);
  stop_meas(&gNB->ulsch_unscrambling_stats);
  //----------------------------------------------------------
  //--------------------- ULSCH decoding ---------------------
  //----------------------------------------------------------

  start_meas(&gNB->ulsch_decoding_stats);
  nr_ulsch_decoding(gNB,
                    ULSCH_id,
                    gNB->pusch_vars[ULSCH_id]->llr,
                    frame_parms,
                    pusch_pdu,
                    frame_rx,
                    slot_rx,
                    harq_pid,
                    G);
  if (enable_ldpc_offload ==0) {
    while (gNB->nbDecode > 0) {
      notifiedFIFO_elt_t *req = pullTpool(&gNB->respDecode, &gNB->threadPool);
      if (req == NULL)
	break; // Tpool has been stopped
      nr_postDecode(gNB, req);
      delNotifiedFIFO_elt(req);
    }
  } 
  stop_meas(&gNB->ulsch_decoding_stats);
}


void nr_fill_indication(PHY_VARS_gNB *gNB, int frame, int slot_rx, int ULSCH_id, uint8_t harq_pid, uint8_t crc_flag, int dtx_flag) {

  pthread_mutex_lock(&gNB->UL_INFO_mutex);

  NR_gNB_ULSCH_t                       *ulsch                 = gNB->ulsch[ULSCH_id];
  NR_UL_gNB_HARQ_t                     *harq_process          = ulsch->harq_processes[harq_pid];
  NR_gNB_SCH_STATS_t *stats=get_ulsch_stats(gNB,ulsch);

  nfapi_nr_pusch_pdu_t *pusch_pdu = &harq_process->ulsch_pdu;

  //  pdu->data                              = gNB->ulsch[ULSCH_id+1]->harq_processes[harq_pid]->b;
  int sync_pos = nr_est_timing_advance_pusch(gNB, ULSCH_id); // estimate timing advance for MAC

  // scale the 16 factor in N_TA calculation in 38.213 section 4.2 according to the used FFT size
  uint16_t bw_scaling = 16 * gNB->frame_parms.ofdm_symbol_size / 2048;

  // do some integer rounding to improve TA accuracy
  int sync_pos_rounded;
  if (sync_pos > 0)
    sync_pos_rounded = sync_pos + (bw_scaling / 2) - 1;
  else
    sync_pos_rounded = sync_pos - (bw_scaling / 2) + 1;
  if (stats) stats->sync_pos = sync_pos;

  int timing_advance_update = sync_pos_rounded / bw_scaling;

  // put timing advance command in 0..63 range
  timing_advance_update += 31;

  if (timing_advance_update < 0)  timing_advance_update = 0;
  if (timing_advance_update > 63) timing_advance_update = 63;

  if (crc_flag == 0) LOG_D(PHY, "%d.%d : Received PUSCH : Estimated timing advance PUSCH is  = %d, timing_advance_update is %d \n", frame,slot_rx,sync_pos,timing_advance_update);

  // estimate UL_CQI for MAC
  int SNRtimes10 = dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_power_tot) -
                   dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot);

  LOG_D(PHY, "%d.%d: Estimated SNR for PUSCH is = %f dB (ulsch_power %f, noise %f) delay %d\n", frame, slot_rx, SNRtimes10/10.0,dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_power_tot)/10.0,dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot)/10.0,sync_pos);

  int cqi;
  if      (SNRtimes10 < -640) cqi=0;
  else if (SNRtimes10 >  635) cqi=255;
  else                        cqi=(640+SNRtimes10)/5;


  if (0/*pusch_pdu->mcs_index == 9*/) {
      __attribute__((unused))
#ifdef __AVX2__
      int off = ((pusch_pdu->rb_size&1) == 1)? 4:0;
#else
      int off = 0;
#endif
      LOG_M("rxsigF0.m","rxsF0",&gNB->common_vars.rxdataF[0][(slot_rx&3)*gNB->frame_parms.ofdm_symbol_size*gNB->frame_parms.symbols_per_slot],gNB->frame_parms.ofdm_symbol_size*gNB->frame_parms.symbols_per_slot,1,1);
      LOG_M("rxsigF0_ext.m","rxsF0_ext",
             &gNB->pusch_vars[0]->rxdataF_ext[0][pusch_pdu->start_symbol_index*NR_NB_SC_PER_RB * pusch_pdu->rb_size],pusch_pdu->nr_of_symbols*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_M("chestF0.m","chF0",
            &gNB->pusch_vars[0]->ul_ch_estimates[0][pusch_pdu->start_symbol_index*gNB->frame_parms.ofdm_symbol_size],gNB->frame_parms.ofdm_symbol_size,1,1);
      LOG_M("chestF0_ext.m","chF0_ext",
            &gNB->pusch_vars[0]->ul_ch_estimates_ext[0][(pusch_pdu->start_symbol_index+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
            (pusch_pdu->nr_of_symbols-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_M("rxsigF0_comp.m","rxsF0_comp",
            &gNB->pusch_vars[0]->rxdataF_comp[0][pusch_pdu->start_symbol_index*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],pusch_pdu->nr_of_symbols*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_M("rxsigF0_llr.m","rxsF0_llr",
            &gNB->pusch_vars[0]->llr[0],(pusch_pdu->nr_of_symbols-1)*NR_NB_SC_PER_RB *pusch_pdu->rb_size * pusch_pdu->qam_mod_order,1,0);
      if (gNB->frame_parms.nb_antennas_rx > 1) {
        LOG_M("rxsigF1.m","rxsF1",&gNB->common_vars.rxdataF[1][(slot_rx&3)*gNB->frame_parms.ofdm_symbol_size*gNB->frame_parms.symbols_per_slot],gNB->frame_parms.ofdm_symbol_size*gNB->frame_parms.symbols_per_slot,1,1);
        LOG_M("rxsigF1_ext.m","rxsF1_ext",
               &gNB->pusch_vars[0]->rxdataF_ext[1][pusch_pdu->start_symbol_index*NR_NB_SC_PER_RB * pusch_pdu->rb_size],pusch_pdu->nr_of_symbols*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
        LOG_M("chestF1.m","chF1",
              &gNB->pusch_vars[0]->ul_ch_estimates[1][pusch_pdu->start_symbol_index*gNB->frame_parms.ofdm_symbol_size],gNB->frame_parms.ofdm_symbol_size,1,1);
        LOG_M("chestF1_ext.m","chF1_ext",
              &gNB->pusch_vars[0]->ul_ch_estimates_ext[1][(pusch_pdu->start_symbol_index+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
              (pusch_pdu->nr_of_symbols-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
        LOG_M("rxsigF1_comp.m","rxsF1_comp",
              &gNB->pusch_vars[0]->rxdataF_comp[1][pusch_pdu->start_symbol_index*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],pusch_pdu->nr_of_symbols*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      }
      exit(-1);

    }

  // crc indication
  uint16_t num_crc = gNB->UL_INFO.crc_ind.number_crcs;
  gNB->UL_INFO.crc_ind.crc_list = &gNB->crc_pdu_list[0];
  gNB->UL_INFO.crc_ind.sfn = frame;
  gNB->UL_INFO.crc_ind.slot = slot_rx;

  gNB->crc_pdu_list[num_crc].handle = pusch_pdu->handle;
  gNB->crc_pdu_list[num_crc].rnti = pusch_pdu->rnti;
  gNB->crc_pdu_list[num_crc].harq_id = harq_pid;
  gNB->crc_pdu_list[num_crc].tb_crc_status = crc_flag;
  gNB->crc_pdu_list[num_crc].num_cb = pusch_pdu->pusch_data.num_cb;
  gNB->crc_pdu_list[num_crc].ul_cqi = cqi;
  gNB->crc_pdu_list[num_crc].timing_advance = timing_advance_update;
  // in terms of dBFS range -128 to 0 with 0.1 step
  gNB->crc_pdu_list[num_crc].rssi = (dtx_flag==0) ? 1280 - (10*dB_fixed(32767*32767)-dB_fixed_times10(gNB->pusch_vars[ULSCH_id]->ulsch_power[0])) : 0;

  gNB->UL_INFO.crc_ind.number_crcs++;

  // rx indication
  uint16_t num_rx = gNB->UL_INFO.rx_ind.number_of_pdus;
  gNB->UL_INFO.rx_ind.pdu_list = &gNB->rx_pdu_list[0];
  gNB->UL_INFO.rx_ind.sfn = frame;
  gNB->UL_INFO.rx_ind.slot = slot_rx;
  gNB->rx_pdu_list[num_rx].handle = pusch_pdu->handle;
  gNB->rx_pdu_list[num_rx].rnti = pusch_pdu->rnti;
  gNB->rx_pdu_list[num_rx].harq_id = harq_pid;
  gNB->rx_pdu_list[num_rx].ul_cqi = cqi;
  gNB->rx_pdu_list[num_rx].timing_advance = timing_advance_update;
  gNB->rx_pdu_list[num_rx].rssi = gNB->crc_pdu_list[num_crc].rssi;
  if (crc_flag)
    gNB->rx_pdu_list[num_rx].pdu_length = 0;
  else {
    gNB->rx_pdu_list[num_rx].pdu_length = harq_process->TBS;
    gNB->rx_pdu_list[num_rx].pdu = harq_process->b;
  }

  gNB->UL_INFO.rx_ind.number_of_pdus++;

  pthread_mutex_unlock(&gNB->UL_INFO_mutex);

}

// Function to fill UL RB mask to be used for N0 measurements
void fill_ul_rb_mask(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx) {

  int rb = 0;
  int rb2 = 0;
  int prbpos = 0;

  for (int symbol=0;symbol<14;symbol++) {
    for (int m=0;m<9;m++) {
      gNB->rb_mask_ul[symbol][m] = 0;
      for (int i=0;i<32;i++) {
        prbpos = (m*32)+i;
        if (prbpos>gNB->frame_parms.N_RB_UL) break;
        gNB->rb_mask_ul[symbol][m] |= (gNB->ulprbbl[prbpos]>0 ? 1 : 0)<<i;
      }
    }
  }

  for (int i=0;i<NUMBER_OF_NR_PUCCH_MAX;i++){
    NR_gNB_PUCCH_t *pucch = gNB->pucch[i];
    if (pucch) {
      if ((pucch->active == 1) &&
          (pucch->frame == frame_rx) &&
          (pucch->slot == slot_rx) ) {
        nfapi_nr_pucch_pdu_t  *pucch_pdu = &pucch->pucch_pdu;
        LOG_D(PHY,"%d.%d pucch %d : start_symbol %d, nb_symbols %d, prb_size %d\n",frame_rx,slot_rx,i,pucch_pdu->start_symbol_index,pucch_pdu->nr_of_symbols,pucch_pdu->prb_size);
        for (int symbol=pucch_pdu->start_symbol_index ; symbol<(pucch_pdu->start_symbol_index+pucch_pdu->nr_of_symbols);symbol++) {
          if(gNB->frame_parms.frame_type == FDD ||
              (gNB->frame_parms.frame_type == TDD && gNB->gNB_config.tdd_table.max_tdd_periodicity_list[slot_rx].max_num_of_symbol_per_slot_list[symbol].slot_config.value==1)) {
            for (rb=0; rb<pucch_pdu->prb_size; rb++) {
              rb2 = rb + pucch_pdu->bwp_start +
                    ((symbol < pucch_pdu->start_symbol_index+(pucch_pdu->nr_of_symbols>>1)) || (pucch_pdu->freq_hop_flag == 0) ?
                     pucch_pdu->prb_start : pucch_pdu->second_hop_prb);
              gNB->rb_mask_ul[symbol][rb2>>5] |= (1<<(rb2&31));
            }
          }
        }
      }
    }
  }

  for (int ULSCH_id=0;ULSCH_id<gNB->number_of_nr_ulsch_max;ULSCH_id++) {
    NR_gNB_ULSCH_t *ulsch = gNB->ulsch[ULSCH_id];
    int harq_pid;
    NR_UL_gNB_HARQ_t *ulsch_harq;
    if ((ulsch) &&
        (ulsch->rnti > 0)) {
      for (harq_pid=0;harq_pid<NR_MAX_ULSCH_HARQ_PROCESSES;harq_pid++) {
        ulsch_harq = ulsch->harq_processes[harq_pid];
        AssertFatal(ulsch_harq!=NULL,"harq_pid %d is not allocated\n",harq_pid);
        if ((ulsch_harq->status == NR_ACTIVE) &&
            (ulsch_harq->frame == frame_rx) &&
            (ulsch_harq->slot == slot_rx) &&
            (ulsch_harq->handled == 0)){
          uint8_t symbol_start = ulsch_harq->ulsch_pdu.start_symbol_index;
          uint8_t symbol_end = symbol_start + ulsch_harq->ulsch_pdu.nr_of_symbols;
          for (int symbol=symbol_start ; symbol<symbol_end ; symbol++) {
            if(gNB->frame_parms.frame_type == FDD ||
                (gNB->frame_parms.frame_type == TDD && gNB->gNB_config.tdd_table.max_tdd_periodicity_list[slot_rx].max_num_of_symbol_per_slot_list[symbol].slot_config.value==1)) {
              LOG_D(PHY,"symbol %d Filling rb_mask_ul rb_size %d\n",symbol,ulsch_harq->ulsch_pdu.rb_size);
              for (rb=0; rb<ulsch_harq->ulsch_pdu.rb_size; rb++) {
                rb2 = rb+ulsch_harq->ulsch_pdu.rb_start+ulsch_harq->ulsch_pdu.bwp_start;
                gNB->rb_mask_ul[symbol][rb2 >> 5] |= 1U << (rb2 & 31);
              }
            }
          }
        }
      }
    }
  }

  for (int i=0;i<NUMBER_OF_NR_SRS_MAX;i++) {
    NR_gNB_SRS_t *srs = gNB->srs[i];
    if (srs) {
      if ((srs->active == 1) && (srs->frame == frame_rx) && (srs->slot == slot_rx)) {
        nfapi_nr_srs_pdu_t *srs_pdu = &srs->srs_pdu;
        for(int symbol = 0; symbol<(1<<srs_pdu->num_symbols); symbol++) {
          for(rb = srs_pdu->bwp_start; rb < (srs_pdu->bwp_start+srs_pdu->bwp_size); rb++) {
            gNB->rb_mask_ul[gNB->frame_parms.symbols_per_slot-srs_pdu->time_start_position-1+symbol][rb>>5] |= 1<<(rb&31);
          }
        }
      }
    }
  }

}

int fill_srs_reported_symbol_list(nfapi_nr_srs_indication_reported_symbol_t *reported_symbol_list,
                                  const nfapi_nr_srs_pdu_t *srs_pdu,
                                  const int N_RB_UL,
                                  const int8_t *snr_per_rb,
                                  const int srs_est) {

  reported_symbol_list->num_rbs = srs_bandwidth_config[srs_pdu->config_index][srs_pdu->bandwidth_index][0];

  if (!reported_symbol_list->rb_list) {
    reported_symbol_list->rb_list = (nfapi_nr_srs_indication_reported_symbol_resource_block_t*) calloc(1, N_RB_UL*sizeof(nfapi_nr_srs_indication_reported_symbol_resource_block_t));
  }

  for(int rb = 0; rb < reported_symbol_list->num_rbs; rb++) {
    if (srs_est<0) {
      reported_symbol_list->rb_list[rb].rb_snr = 0xFF;
    } else if (snr_per_rb[rb] < -64) {
      reported_symbol_list->rb_list[rb].rb_snr = 0;
    } else if (snr_per_rb[rb] > 63) {
      reported_symbol_list->rb_list[rb].rb_snr = 0xFE;
    } else {
      reported_symbol_list->rb_list[rb].rb_snr = (snr_per_rb[rb] + 64)<<1;
    }
  }

  return 0;
}

int phy_procedures_gNB_uespec_RX(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx) {
  /* those variables to log T_GNB_PHY_PUCCH_PUSCH_IQ only when we try to decode */
  int pucch_decode_done = 0;
  int pusch_decode_done = 0;
  int pusch_DTX = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_gNB_UESPEC_RX,1);
  LOG_D(PHY,"phy_procedures_gNB_uespec_RX frame %d, slot %d\n",frame_rx,slot_rx);

  fill_ul_rb_mask(gNB, frame_rx, slot_rx);

  int first_symb=0,num_symb=0;
  if (gNB->frame_parms.frame_type == TDD)
    for(int symbol_count=0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
      if (gNB->gNB_config.tdd_table.max_tdd_periodicity_list[slot_rx].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value==1) {
	      if (num_symb==0) first_symb=symbol_count;
	      num_symb++;
      }
    }
  else num_symb=NR_NUMBER_OF_SYMBOLS_PER_SLOT;
  gNB_I0_measurements(gNB,slot_rx,first_symb,num_symb);

  int offset = 10*gNB->frame_parms.ofdm_symbol_size + gNB->frame_parms.first_carrier_offset;
  int power_rxF = signal_energy_nodc(&gNB->common_vars.rxdataF[0][offset+(47*12)],12*18);
  LOG_D(PHY,"frame %d, slot %d: UL signal energy %d\n",frame_rx,slot_rx,power_rxF);

  start_meas(&gNB->phy_proc_rx);

  for (int i=0;i<NUMBER_OF_NR_PUCCH_MAX;i++){
    NR_gNB_PUCCH_t *pucch = gNB->pucch[i];
    if (pucch) {
      if (NFAPI_MODE == NFAPI_MODE_PNF)
        pucch->frame = frame_rx;
      if ((pucch->active == 1) &&
          (pucch->frame == frame_rx) &&
          (pucch->slot == slot_rx) ) {

        pucch_decode_done = 1;

        nfapi_nr_pucch_pdu_t  *pucch_pdu = &pucch->pucch_pdu;
        uint16_t num_ucis;
        switch (pucch_pdu->format_type) {
        case 0:
          num_ucis = gNB->UL_INFO.uci_ind.num_ucis;
          gNB->UL_INFO.uci_ind.uci_list = &gNB->uci_pdu_list[0];
          gNB->UL_INFO.uci_ind.sfn = frame_rx;
          gNB->UL_INFO.uci_ind.slot = slot_rx;
          gNB->uci_pdu_list[num_ucis].pdu_type = NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE;
          gNB->uci_pdu_list[num_ucis].pdu_size = sizeof(nfapi_nr_uci_pucch_pdu_format_0_1_t);
          nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_pdu_format0 = &gNB->uci_pdu_list[num_ucis].pucch_pdu_format_0_1;

          offset = pucch_pdu->start_symbol_index*gNB->frame_parms.ofdm_symbol_size + (gNB->frame_parms.first_carrier_offset+pucch_pdu->prb_start*12);
          power_rxF = signal_energy_nodc(&gNB->common_vars.rxdataF[0][offset],12);
          LOG_D(PHY,"frame %d, slot %d: PUCCH signal energy %d\n",frame_rx,slot_rx,power_rxF);

          nr_decode_pucch0(gNB,
                           frame_rx,
                           slot_rx,
                           uci_pdu_format0,
                           pucch_pdu);

          gNB->UL_INFO.uci_ind.num_ucis += 1;
          pucch->active = 0;
	        break;
        case 2:
          num_ucis = gNB->UL_INFO.uci_ind.num_ucis;
          gNB->UL_INFO.uci_ind.uci_list = &gNB->uci_pdu_list[0];
          gNB->UL_INFO.uci_ind.sfn = frame_rx;
          gNB->UL_INFO.uci_ind.slot = slot_rx;
          gNB->uci_pdu_list[num_ucis].pdu_type = NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE;
          gNB->uci_pdu_list[num_ucis].pdu_size = sizeof(nfapi_nr_uci_pucch_pdu_format_2_3_4_t);
          nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_pdu_format2 = &gNB->uci_pdu_list[num_ucis].pucch_pdu_format_2_3_4;

          LOG_D(PHY,"%d.%d Calling nr_decode_pucch2\n",frame_rx,slot_rx);
          nr_decode_pucch2(gNB,
                           frame_rx,
                           slot_rx,
                           uci_pdu_format2,
                           pucch_pdu);

          gNB->UL_INFO.uci_ind.num_ucis += 1;
          pucch->active = 0;
          break;
        default:
	        AssertFatal(1==0,"Only PUCCH formats 0 and 2 are currently supported\n");
        }
      }
    }
  }

  for (int ULSCH_id=0;ULSCH_id<gNB->number_of_nr_ulsch_max;ULSCH_id++) {
    NR_gNB_ULSCH_t *ulsch = gNB->ulsch[ULSCH_id];
    NR_UL_gNB_HARQ_t *ulsch_harq;

    if ((ulsch) &&
        (ulsch->rnti > 0)) {
      // for for an active HARQ process
      for (int harq_pid=0;harq_pid<NR_MAX_ULSCH_HARQ_PROCESSES;harq_pid++) {
        ulsch_harq = ulsch->harq_processes[harq_pid];
        AssertFatal(ulsch_harq!=NULL,"harq_pid %d is not allocated\n",harq_pid);
        if ((ulsch_harq->status == NR_ACTIVE) &&
            (ulsch_harq->frame == frame_rx) &&
            (ulsch_harq->slot == slot_rx) &&
            (ulsch_harq->handled == 0)){

          LOG_D(PHY, "PUSCH detection started in frame %d slot %d\n",
                frame_rx,slot_rx);
          int num_dmrs=0;
          for (int s=0;s<NR_NUMBER_OF_SYMBOLS_PER_SLOT; s++)
             num_dmrs+=(ulsch_harq->ulsch_pdu.ul_dmrs_symb_pos>>s)&1;

#ifdef DEBUG_RXDATA
          NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
          RU_t *ru = gNB->RU_list[0];
          int slot_offset = frame_parms->get_samples_slot_timestamp(slot_rx,frame_parms,0);
          slot_offset -= ru->N_TA_offset;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[0]=(int16_t)ulsch->rnti;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[1]=(int16_t)ulsch_harq->ulsch_pdu.rb_size;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[2]=(int16_t)ulsch_harq->ulsch_pdu.rb_start;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[3]=(int16_t)ulsch_harq->ulsch_pdu.nr_of_symbols;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[4]=(int16_t)ulsch_harq->ulsch_pdu.start_symbol_index;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[5]=(int16_t)ulsch_harq->ulsch_pdu.mcs_index;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[6]=(int16_t)ulsch_harq->ulsch_pdu.pusch_data.rv_index;
          ((int16_t*)&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset])[7]=(int16_t)harq_pid;
          memcpy(&gNB->common_vars.debugBuff[gNB->common_vars.debugBuff_sample_offset+4],&ru->common.rxdata[0][slot_offset],frame_parms->get_samples_per_slot(slot_rx,frame_parms)*sizeof(int32_t));
          gNB->common_vars.debugBuff_sample_offset+=(frame_parms->get_samples_per_slot(slot_rx,frame_parms)+1000+4);
          if(gNB->common_vars.debugBuff_sample_offset>((frame_parms->get_samples_per_slot(slot_rx,frame_parms)+1000+2)*20)) {
            FILE *f;
            f = fopen("rxdata_buff.raw", "w"); if (f == NULL) exit(1);
            fwrite((int16_t*)gNB->common_vars.debugBuff,2,(frame_parms->get_samples_per_slot(slot_rx,frame_parms)+1000+4)*20*2, f);
            fclose(f);
            exit(-1);
          }
#endif

          pusch_decode_done = 1;

          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RX_PUSCH,1);
	        start_meas(&gNB->rx_pusch_stats);
          nr_rx_pusch(gNB, ULSCH_id, frame_rx, slot_rx, harq_pid);
          gNB->pusch_vars[ULSCH_id]->ulsch_power_tot=0;
          gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot=0;
          for (int aarx=0;aarx<gNB->frame_parms.nb_antennas_rx;aarx++) {
             gNB->pusch_vars[ULSCH_id]->ulsch_power[aarx]/=num_dmrs;
             gNB->pusch_vars[ULSCH_id]->ulsch_power_tot += gNB->pusch_vars[ULSCH_id]->ulsch_power[aarx];
             gNB->pusch_vars[ULSCH_id]->ulsch_noise_power[aarx]/=num_dmrs;
             gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot += gNB->pusch_vars[ULSCH_id]->ulsch_noise_power[aarx];
          }
          if (dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_power_tot) <
              dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot) + gNB->pusch_thres) {
             NR_gNB_SCH_STATS_t *stats=get_ulsch_stats(gNB,ulsch);

             LOG_D(PHY, "PUSCH not detected in %d.%d (%d,%d,%d)\n",frame_rx,slot_rx,
                   dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_power_tot),
                   dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot),gNB->pusch_thres);
             gNB->pusch_vars[ULSCH_id]->ulsch_power_tot = gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot;
             gNB->pusch_vars[ULSCH_id]->DTX=1;
             if (stats) stats->DTX++;
             if (!get_softmodem_params()->phy_test) {
               /* in case of phy_test mode, we still want to decode to measure execution time. 
                  Therefore, we don't yet call nr_fill_indication, it will be called later */
               nr_fill_indication(gNB,frame_rx, slot_rx, ULSCH_id, harq_pid, 1,1);
               pusch_DTX++;
               continue;
             }
          } else {
            LOG_D(PHY, "PUSCH detected in %d.%d (%d,%d,%d)\n",frame_rx,slot_rx,
                  dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_power_tot),
                  dB_fixed_x10(gNB->pusch_vars[ULSCH_id]->ulsch_noise_power_tot),gNB->pusch_thres);

            gNB->pusch_vars[ULSCH_id]->DTX=0;
          }
          stop_meas(&gNB->rx_pusch_stats);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RX_PUSCH,0);
          //LOG_M("rxdataF_comp.m","rxF_comp",gNB->pusch_vars[0]->rxdataF_comp[0],6900,1,1);
          //LOG_M("rxdataF_ext.m","rxF_ext",gNB->pusch_vars[0]->rxdataF_ext[0],6900,1,1);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_ULSCH_PROCEDURES_RX,1);
          nr_ulsch_procedures(gNB, frame_rx, slot_rx, ULSCH_id, harq_pid);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_ULSCH_PROCEDURES_RX,0);
          break;
        }
      }
    }
  }

  for (int i=0;i<NUMBER_OF_NR_SRS_MAX;i++) {
    NR_gNB_SRS_t *srs = gNB->srs[i];
    if (srs) {
      if ((srs->active == 1) && (srs->frame == frame_rx) && (srs->slot == slot_rx)) {

        LOG_D(NR_PHY, "(%d.%d) gNB is waiting for SRS, id = %i\n", frame_rx, slot_rx, i);

        NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
        nfapi_nr_srs_pdu_t *srs_pdu = &srs->srs_pdu;
        uint8_t N_symb_SRS = 1<<srs_pdu->num_symbols;
        int32_t srs_received_signal[frame_parms->nb_antennas_rx][frame_parms->ofdm_symbol_size*N_symb_SRS];
        int32_t srs_ls_estimated_channel[frame_parms->nb_antennas_rx][1<<srs_pdu->num_ant_ports][frame_parms->ofdm_symbol_size*N_symb_SRS];
        int32_t srs_estimated_channel_freq[frame_parms->nb_antennas_rx][1<<srs_pdu->num_ant_ports][frame_parms->ofdm_symbol_size*N_symb_SRS] __attribute__ ((aligned(32)));
        int32_t srs_estimated_channel_time[frame_parms->nb_antennas_rx][1<<srs_pdu->num_ant_ports][frame_parms->ofdm_symbol_size] __attribute__ ((aligned(32)));
        int32_t srs_estimated_channel_time_shifted[frame_parms->nb_antennas_rx][1<<srs_pdu->num_ant_ports][frame_parms->ofdm_symbol_size];
        uint32_t noise_power_per_rb[srs_pdu->bwp_size];
        int8_t snr_per_rb[srs_pdu->bwp_size];
        uint32_t signal_power;
        uint32_t noise_power;
        int8_t snr;

        // At least currently, the configuration is constant, so it is enough to generate the sequence just once.
        if(gNB->nr_srs_info[i]->srs_generated_signal_bits == 0) {
          generate_srs_nr(srs_pdu, frame_parms, gNB->nr_srs_info[i]->srs_generated_signal, 0, gNB->nr_srs_info[i], AMP, frame_rx, slot_rx);
        }

        int srs_est = nr_get_srs_signal(gNB, frame_rx, slot_rx, srs_pdu, gNB->nr_srs_info[i], srs_received_signal);

        if (srs_est >= 0) {
          nr_srs_channel_estimation(gNB,
                                    frame_rx,
                                    slot_rx,
                                    srs_pdu,
                                    gNB->nr_srs_info[i],
                                    (const int32_t **) gNB->nr_srs_info[i]->srs_generated_signal,
                                    srs_received_signal,
                                    srs_ls_estimated_channel,
                                    srs_estimated_channel_freq,
                                    srs_estimated_channel_time,
                                    srs_estimated_channel_time_shifted,
                                    &signal_power,
                                    noise_power_per_rb,
                                    &noise_power,
                                    snr_per_rb,
                                    &snr);
        }

        if ((snr * 10) < gNB->srs_thres) {
          srs_est = -1;
        }

        T(T_GNB_PHY_UL_FREQ_CHANNEL_ESTIMATE,
          T_INT(0),
          T_INT(srs_pdu->rnti),
          T_INT(frame_rx),
          T_INT(0),
          T_INT(0),
          T_BUFFER(srs_estimated_channel_freq[0][0], frame_parms->ofdm_symbol_size * sizeof(int32_t)));

        T(T_GNB_PHY_UL_TIME_CHANNEL_ESTIMATE,
          T_INT(0),
          T_INT(srs_pdu->rnti),
          T_INT(frame_rx),
          T_INT(0),
          T_INT(0),
          T_BUFFER(srs_estimated_channel_time_shifted[0][0], frame_parms->ofdm_symbol_size * sizeof(int32_t)));

        gNB->UL_INFO.srs_ind.pdu_list = &gNB->srs_pdu_list[0];
        gNB->UL_INFO.srs_ind.sfn = frame_rx;
        gNB->UL_INFO.srs_ind.slot = slot_rx;

        nfapi_nr_srs_indication_pdu_t *srs_indication = &gNB->srs_pdu_list[gNB->UL_INFO.srs_ind.number_of_pdus];
        srs_indication->handle = srs_pdu->handle;
        srs_indication->rnti = srs_pdu->rnti;
        srs_indication->timing_advance = srs_est >= 0 ? nr_est_timing_advance_srs(frame_parms, srs_estimated_channel_time[0]) : 0xFFFF;
        srs_indication->num_symbols = 1 << srs_pdu->num_symbols;
        srs_indication->wide_band_snr = srs_est >= 0 ? (snr + 64) << 1 : 0xFF; // 0xFF will be set if this field is invalid
        srs_indication->num_reported_symbols = 1 << srs_pdu->num_symbols;
        if (!srs_indication->reported_symbol_list) {
          srs_indication->reported_symbol_list = (nfapi_nr_srs_indication_reported_symbol_t *)calloc(1, srs_indication->num_reported_symbols * sizeof(nfapi_nr_srs_indication_reported_symbol_t));
        }
        fill_srs_reported_symbol_list(&srs_indication->reported_symbol_list[0], srs_pdu, frame_parms->N_RB_UL, snr_per_rb, srs_est);

        gNB->UL_INFO.srs_ind.number_of_pdus += 1;

#ifdef SRS_IND_DEBUG
        LOG_I(NR_PHY, "gNB->UL_INFO.srs_ind.sfn = %i\n", gNB->UL_INFO.srs_ind.sfn);
        LOG_I(NR_PHY, "gNB->UL_INFO.srs_ind.slot = %i\n", gNB->UL_INFO.srs_ind.slot);
        LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].rnti = 0x%04x\n", num_srs, gNB->srs_pdu_list[num_srs].rnti);
        LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].timing_advance = %i\n", num_srs, gNB->srs_pdu_list[num_srs].timing_advance);
        LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].num_symbols = %i\n", num_srs, gNB->srs_pdu_list[num_srs].num_symbols);
        LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].wide_band_snr = %i\n", num_srs, gNB->srs_pdu_list[num_srs].wide_band_snr);
        LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].num_reported_symbols = %i\n", num_srs, gNB->srs_pdu_list[num_srs].num_reported_symbols);
        LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].reported_symbol_list[0].num_rbs = %i\n", num_srs, gNB->srs_pdu_list[num_srs].reported_symbol_list[0].num_rbs);
        for (int rb = 0; rb < gNB->srs_pdu_list[num_srs].reported_symbol_list[0].num_rbs; rb++) {
          LOG_I(NR_PHY, "gNB->srs_pdu_list[%i].reported_symbol_list[0].rb_list[%3i].rb_snr = %i\n", num_srs, rb, gNB->srs_pdu_list[num_srs].reported_symbol_list[0].rb_list[rb].rb_snr);
        }
#endif

        srs->active = 0;
      }
    }
  }

  stop_meas(&gNB->phy_proc_rx);

  if (pucch_decode_done || pusch_decode_done) {
    T(T_GNB_PHY_PUCCH_PUSCH_IQ, T_INT(frame_rx), T_INT(slot_rx), T_BUFFER(&gNB->common_vars.rxdataF[0][0], gNB->frame_parms.symbols_per_slot * gNB->frame_parms.ofdm_symbol_size * 4));
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_gNB_UESPEC_RX,0);
  return pusch_DTX;
}
