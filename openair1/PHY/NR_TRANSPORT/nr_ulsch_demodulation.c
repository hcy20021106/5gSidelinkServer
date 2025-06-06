#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nr_transport_proto.h"
#include "PHY/impl_defs_top.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "PHY/defs_nr_common.h"
#include "common/utils/nr/nr_common.h"
#include "openair1/PHY/NR_TRANSPORT/nr_transport_common_proto.h"


//#define DEBUG_CH_COMP
//#define DEBUG_RB_EXT
//#define DEBUG_CH_MAG

#define INVALID_VALUE 255

void nr_ulsch_extract_rbs(int32_t **rxdataF,
                          NR_gNB_PUSCH *pusch_vars,
                          int slot,
                          unsigned char symbol,
                          uint8_t is_dmrs_symbol,
                          nfapi_nr_pusch_pdu_t *pusch_pdu,
                          NR_DL_FRAME_PARMS *frame_parms) {

  unsigned short start_re, re, nb_re_pusch;
  unsigned char aarx, aatx;
  uint32_t rxF_ext_index = 0;
  uint32_t ul_ch0_ext_index = 0;
  uint32_t ul_ch0_index = 0;
  int16_t *rxF,*rxF_ext;
  int *ul_ch0,*ul_ch0_ext;
  int soffset = (slot&3)*frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size;

#ifdef DEBUG_RB_EXT
  printf("--------------------symbol = %d-----------------------\n", symbol);
  printf("--------------------ch_ext_index = %d-----------------------\n", symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size);
#endif

  uint8_t is_data_re;
  start_re = (frame_parms->first_carrier_offset + (pusch_pdu->rb_start + pusch_pdu->bwp_start) * NR_NB_SC_PER_RB)%frame_parms->ofdm_symbol_size;
  nb_re_pusch = NR_NB_SC_PER_RB * pusch_pdu->rb_size;

#ifdef __AVX2__
  int nb_re_pusch2 = nb_re_pusch + (nb_re_pusch&7);
#else
  int nb_re_pusch2 = nb_re_pusch;
#endif

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {

    rxF = (int16_t *)&rxdataF[aarx][soffset+(symbol * frame_parms->ofdm_symbol_size)];
    rxF_ext = (int16_t *)&pusch_vars->rxdataF_ext[aarx][symbol * nb_re_pusch2]; // [hna] rxdataF_ext isn't contiguous in order to solve an alignment problem ib llr computation in case of mod_order = 4, 6
    
    if (is_dmrs_symbol == 0) {
      if (start_re + nb_re_pusch <= frame_parms->ofdm_symbol_size) {
        memcpy1((void*)rxF_ext, (void*)&rxF[start_re*2], nb_re_pusch*sizeof(int32_t));
      } else {
        int neg_length = frame_parms->ofdm_symbol_size-start_re;
        int pos_length = nb_re_pusch-neg_length;
        memcpy1((void*)rxF_ext,(void*)&rxF[start_re*2],neg_length*sizeof(int32_t));
        memcpy1((void*)&rxF_ext[2*neg_length],(void*)rxF,pos_length*sizeof(int32_t));
      }

      for (aatx = 0; aatx < pusch_pdu->nrOfLayers; aatx++) {
        ul_ch0 = &pusch_vars->ul_ch_estimates[aatx*frame_parms->nb_antennas_rx+aarx][pusch_vars->dmrs_symbol*frame_parms->ofdm_symbol_size]; // update channel estimates if new dmrs symbol are available
        ul_ch0_ext = &pusch_vars->ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*nb_re_pusch2];
        memcpy1((void*)ul_ch0_ext,(void*)ul_ch0,nb_re_pusch*sizeof(int32_t));
      }

    } else {

      for (aatx = 0; aatx < pusch_pdu->nrOfLayers; aatx++) {
        ul_ch0 = &pusch_vars->ul_ch_estimates[aatx*frame_parms->nb_antennas_rx+aarx][pusch_vars->dmrs_symbol*frame_parms->ofdm_symbol_size]; // update channel estimates if new dmrs symbol are available
        ul_ch0_ext = &pusch_vars->ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*nb_re_pusch2];

        rxF_ext_index = 0;
        ul_ch0_ext_index = 0;
        ul_ch0_index = 0;
        for (re = 0; re < nb_re_pusch; re++) {
          uint16_t k = start_re + re;
          is_data_re = allowed_xlsch_re_in_dmrs_symbol(k, start_re, frame_parms->ofdm_symbol_size, pusch_pdu->num_dmrs_cdm_grps_no_data, pusch_pdu->dmrs_config_type);
          if (++k >= frame_parms->ofdm_symbol_size) {
            k -= frame_parms->ofdm_symbol_size;
          }

          #ifdef DEBUG_RB_EXT
          printf("re = %d, is_dmrs_symbol = %d, symbol = %d\n", re, is_dmrs_symbol, symbol);
          #endif

          // save only data and respective channel estimates
          if (is_data_re == 1) {
            if (aatx == 0) {
              rxF_ext[rxF_ext_index]     = (rxF[ ((start_re + re)*2)      % (frame_parms->ofdm_symbol_size*2)]);
              rxF_ext[rxF_ext_index + 1] = (rxF[(((start_re + re)*2) + 1) % (frame_parms->ofdm_symbol_size*2)]);
              rxF_ext_index +=2;
            }
          
            ul_ch0_ext[ul_ch0_ext_index] = ul_ch0[ul_ch0_index];
            ul_ch0_ext_index++;

            #ifdef DEBUG_RB_EXT
            printf("dmrs symb %d: rxF_ext[%d] = (%d,%d), ul_ch0_ext[%d] = (%d,%d)\n",
                 is_dmrs_symbol,rxF_ext_index>>1, rxF_ext[rxF_ext_index],rxF_ext[rxF_ext_index+1],
                 ul_ch0_ext_index,  ((int16_t*)&ul_ch0_ext[ul_ch0_ext_index])[0],  ((int16_t*)&ul_ch0_ext[ul_ch0_ext_index])[1]);
            #endif          
          } 
          ul_ch0_index++;
        }
      }
    }
  }
}

void nr_ulsch_scale_channel(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            NR_gNB_ULSCH_t *ulsch_gNB,
                            uint8_t symbol,
                            uint8_t is_dmrs_symbol,
                            uint32_t len,
                            uint8_t nrOfLayers,
                            unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb, ch_amp;
  unsigned char aarx,aatx;
  __m128i *ul_ch128, ch_amp128;

  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);

  // Determine scaling amplitude based the symbol

  ch_amp = 1024*8; //((pilots) ? (ulsch_gNB->sqrt_rho_b) : (ulsch_gNB->sqrt_rho_a));

  LOG_D(PHY,"Scaling PUSCH Chest in OFDM symbol %d by %d, pilots %d nb_rb %d NCP %d symbol %d\n", symbol, ch_amp, is_dmrs_symbol, nb_rb, frame_parms->Ncp, symbol);
   // printf("Scaling PUSCH Chest in OFDM symbol %d by %d\n",symbol_mod,ch_amp);

  ch_amp128 = _mm_set1_epi16(ch_amp); // Q3.13

#ifdef __AVX2__
  int off = ((nb_rb&1) == 1)? 4:0;
#else
  int off = 0;
#endif

  for (aatx = 0; aatx < nrOfLayers; aatx++) {
    for (aarx=0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      ul_ch128 = (__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*NR_NB_SC_PER_RB))];
      for (rb=0;rb < nb_rb_0;rb++) {
        ul_ch128[0] = _mm_mulhi_epi16(ul_ch128[0], ch_amp128);
        ul_ch128[0] = _mm_slli_epi16(ul_ch128[0], 3);

        ul_ch128[1] = _mm_mulhi_epi16(ul_ch128[1], ch_amp128);
        ul_ch128[1] = _mm_slli_epi16(ul_ch128[1], 3);

        ul_ch128[2] = _mm_mulhi_epi16(ul_ch128[2], ch_amp128);
        ul_ch128[2] = _mm_slli_epi16(ul_ch128[2], 3);
        ul_ch128+=3;
      }
    }
  }
#endif
}

//compute average channel_level on each (TX,RX) antenna pair
void nr_ulsch_channel_level(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            int32_t *avg,
                            uint8_t symbol,
                            uint32_t len,
                            uint8_t nrOfLayers,
                            unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aatx, aarx;
  __m128i *ul_ch128, avg128U;

  int16_t x = factor2(len);
  int16_t y = (len)>>x;
  
  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);

#ifdef __AVX2__
  int off = ((nb_rb&1) == 1)? 4:0;
#else
  int off = 0;
#endif

  for (aatx = 0; aatx < nrOfLayers; aatx++) {
    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128U = _mm_setzero_si128();

      ul_ch128=(__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];

      for (rb = 0; rb < nb_rb_0; rb++) {
        avg128U = _mm_add_epi32(avg128U, _mm_srai_epi32(_mm_madd_epi16(ul_ch128[0], ul_ch128[0]), x));
        avg128U = _mm_add_epi32(avg128U, _mm_srai_epi32(_mm_madd_epi16(ul_ch128[1], ul_ch128[1]), x));
        avg128U = _mm_add_epi32(avg128U, _mm_srai_epi32(_mm_madd_epi16(ul_ch128[2], ul_ch128[2]), x));
        ul_ch128+=3;
      }

      avg[aatx*frame_parms->nb_antennas_rx+aarx] = (((int32_t*)&avg128U)[0] +
                                                    ((int32_t*)&avg128U)[1] +
                                                    ((int32_t*)&avg128U)[2] +
                                                    ((int32_t*)&avg128U)[3]) / y;
    }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  short rb;
  unsigned char aatx, aarx, nre = 12, symbol_mod;
  int32x4_t avg128U;
  int16x4_t *ul_ch128;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);
  for (aatx=0; aatx<nrOfLayers; aatx++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128U = vdupq_n_s32(0);
      // 5 is always a symbol with no pilots for both normal and extended prefix

      ul_ch128 = (int16x4_t *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*frame_parms->N_RB_UL*12];

      for (rb = 0; rb < nb_rb_0; rb++) {
        //  printf("rb %d : ",rb);
        //  print_shorts("ch",&ul_ch128[0]);
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[0], ul_ch128[0]));
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[1], ul_ch128[1]));
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[2], ul_ch128[2]));
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[3], ul_ch128[3]));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(nrOfLayers!=1)) {
          ul_ch128+=4;
        } else {
          avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[4], ul_ch128[4]));
          avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[5], ul_ch128[5]));
          ul_ch128+=6;
        }

        /*
          if (rb==0) {
          print_shorts("ul_ch128",&ul_ch128[0]);
          print_shorts("ul_ch128",&ul_ch128[1]);
          print_shorts("ul_ch128",&ul_ch128[2]);
          }
        */
      }

      if (symbol==2) //assume start symbol 2
          nre=6;
      else
          nre=12;

      avg[aatx*frame_parms->nb_antennas_rx+aarx] = (((int32_t*)&avg128U)[0] +
                                                    ((int32_t*)&avg128U)[1] +
                                                    ((int32_t*)&avg128U)[2] +
                                                    ((int32_t*)&avg128U)[3]) / (nb_rb*nre);
    }
  }
#endif
}



//==============================================================================================
// Pre-processing for LLR computation
//==============================================================================================
void nr_ulsch_channel_compensation(int **rxdataF_ext,
                                   int **ul_ch_estimates_ext,
                                   int **ul_ch_mag,
                                   int **ul_ch_magb,
                                   int **rxdataF_comp,
                                   int ***rho,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   int length,
                                   uint8_t is_dmrs_symbol,
                                   unsigned char mod_order,
                                   uint8_t  nrOfLayers,
                                   unsigned short nb_rb,
                                   unsigned char output_shift) {

#ifdef __AVX2__
  int off = ((nb_rb&1) == 1)? 4:0;
#else
  int off = 0;
#endif

#ifdef DEBUG_CH_COMP
  int16_t *rxF, *ul_ch;
  int prnt_idx;
  for (int nl=0; nl<nrOfLayers; nl++) {
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      rxF = (int16_t *) &rxdataF_ext[aarx][symbol * (off + (nb_rb * 12))];
      ul_ch = (int16_t *) &ul_ch_estimates_ext[nl * frame_parms->nb_antennas_rx + aarx][symbol * (off + (nb_rb * 12))];

      printf("--------symbol = %d, mod_order = %d, output_shift = %d, layer %i, antenna rx = %d -----------\n",
             symbol, mod_order, output_shift, nl, aarx);
      printf("----------------Before compensation------------------\n");

      for (prnt_idx = 0; prnt_idx < 12 * 5 * 2; prnt_idx += 2) {
        printf("rxF[%d] = (%d,%d)\n", prnt_idx >> 1, rxF[prnt_idx], rxF[prnt_idx + 1]);
        printf("ul_ch[%d] = (%d,%d)\n", prnt_idx >> 1, ul_ch[prnt_idx], ul_ch[prnt_idx + 1]);
      }
    }
  }
#endif

#ifdef DEBUG_CH_MAG
  int16_t *ch_mag;
  int print_idx;


  for (int ant=0; ant<frame_parms->nb_antennas_rx; ant++) {
    ch_mag   = (int16_t *)&ul_ch_mag[ant][symbol*(off+(nb_rb*12))];

    printf("--------------------symbol = %d, mod_order = %d-----------------------\n", symbol, mod_order);
    printf("----------------Before computation------------------\n");

    for (print_idx=0;print_idx<5;print_idx++){

      printf("ch_mag[%d] = %d\n", print_idx, ch_mag[print_idx]);

    }
  }

#endif

#if defined(__i386) || defined(__x86_64__)

  unsigned short rb;
  unsigned char aatx,aarx;
  __m128i *ul_ch128,*ul_ch128_2,*ul_ch_mag128,*ul_ch_mag128b,*rxdataF128,*rxdataF_comp128,*rho128;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0},QAM_amp128b={0};
  QAM_amp128b = _mm_setzero_si128();

  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);
  for (aatx=0; aatx<nrOfLayers; aatx++) {
    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
    } 
    else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = _mm_set1_epi16(QAM64_n2);
    }

    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)  {
      ul_ch128          = (__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      ul_ch_mag128      = (__m128i *)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      ul_ch_mag128b     = (__m128i *)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*(off+(nb_rb*12))];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];


      for (rb=0; rb<nb_rb_0; rb++) {
        if (mod_order>2) {
          // get channel amplitude if not QPSK

          //print_shorts("ch:",(int16_t*)&ul_ch128[0]);

          mmtmpD0 = _mm_madd_epi16(ul_ch128[0],ul_ch128[0]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = _mm_madd_epi16(ul_ch128[1],ul_ch128[1]);
          mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

          mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1);

          // store channel magnitude here in a new field of ulsch

          ul_ch_mag128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
          ul_ch_mag128b[0] = ul_ch_mag128[0];
          ul_ch_mag128[0] = _mm_mulhi_epi16(ul_ch_mag128[0],QAM_amp128);
          ul_ch_mag128[0] = _mm_slli_epi16(ul_ch_mag128[0],1);

          ul_ch_mag128b[0] = _mm_mulhi_epi16(ul_ch_mag128b[0],QAM_amp128b);
          ul_ch_mag128b[0] = _mm_slli_epi16(ul_ch_mag128b[0],1);
          // print_ints("ch: = ",(int32_t*)&mmtmpD0);
          // print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
          // print_shorts("mag:",(int16_t*)&ul_ch_mag128[0]);

          ul_ch_mag128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
          ul_ch_mag128b[1] = ul_ch_mag128[1];
          ul_ch_mag128[1] = _mm_mulhi_epi16(ul_ch_mag128[1],QAM_amp128);
          ul_ch_mag128[1] = _mm_slli_epi16(ul_ch_mag128[1],1);
          
          ul_ch_mag128b[1] = _mm_mulhi_epi16(ul_ch_mag128b[1],QAM_amp128b);
          ul_ch_mag128b[1] = _mm_slli_epi16(ul_ch_mag128b[1],1);

          mmtmpD0 = _mm_madd_epi16(ul_ch128[2],ul_ch128[2]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);

          ul_ch_mag128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          ul_ch_mag128b[2] = ul_ch_mag128[2];

          ul_ch_mag128[2] = _mm_mulhi_epi16(ul_ch_mag128[2],QAM_amp128);
          ul_ch_mag128[2] = _mm_slli_epi16(ul_ch_mag128[2],1);


          ul_ch_mag128b[2] = _mm_mulhi_epi16(ul_ch_mag128b[2],QAM_amp128b);
          ul_ch_mag128b[2] = _mm_slli_epi16(ul_ch_mag128b[2],1);

        }

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[0],rxdataF128[0]);
        //  print_ints("re",&mmtmpD0);

        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rxdataF_comp128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128);
        //  print_shorts("ch:",ul_ch128);
        //  print_shorts("pack:",rxdataF_comp128);

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[1],rxdataF128[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rxdataF_comp128[1] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+1);
        //  print_shorts("ch:",ul_ch128+1);
        //  print_shorts("pack:",rxdataF_comp128+1);

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rxdataF_comp128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",rxdataF128+2);
        //print_shorts("ch:",ul_ch128+2);
        //print_shorts("pack:",rxdataF_comp128+2);

        ul_ch128+=3;
        ul_ch_mag128+=3;
        ul_ch_mag128b+=3;
        rxdataF128+=3;
        rxdataF_comp128+=3;
      }
    }
  }

  if (rho) {
    //we compute the Tx correlation matrix for each Rx antenna
    //As an example the 2x2 MIMO case requires
    //rho[aarx][nb_aatx*nb_aatx] = [cov(H_aarx_0,H_aarx_0) cov(H_aarx_0,H_aarx_1)
    //                              cov(H_aarx_1,H_aarx_0) cov(H_aarx_1,H_aarx_1)], aarx=0,...,nb_antennas_rx-1

    int avg_rho_re[frame_parms->nb_antennas_rx][nrOfLayers*nrOfLayers];
    int avg_rho_im[frame_parms->nb_antennas_rx][nrOfLayers*nrOfLayers];

    for (aarx=0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      for (aatx=0; aatx < nrOfLayers; aatx++) {
        for (int atx=0; atx< nrOfLayers; atx++) {

          avg_rho_re[aarx][aatx*nrOfLayers+atx] = 0;
          avg_rho_im[aarx][aatx*nrOfLayers+atx] = 0;
          rho128        = (__m128i *)&rho[aarx][aatx*nrOfLayers+atx][symbol*(off+(nb_rb*12))];
          ul_ch128      = (__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
          ul_ch128_2    = (__m128i *)&ul_ch_estimates_ext[atx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];

          for (rb=0; rb<nb_rb_0; rb++) {
            // multiply by conjugated channel
            mmtmpD0 = _mm_madd_epi16(ul_ch128[0],ul_ch128_2[0]);
            //  print_ints("re",&mmtmpD0);

            // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
            mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[0],_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
            //  print_ints("im",&mmtmpD1);
            mmtmpD1 = _mm_madd_epi16(mmtmpD1,ul_ch128_2[0]);
            // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
            mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
            //  print_ints("re(shift)",&mmtmpD0);
            mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
            //  print_ints("im(shift)",&mmtmpD1);
            mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
            mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
            //        print_ints("c0",&mmtmpD2);
            //  print_ints("c1",&mmtmpD3);
            rho128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

            //print_shorts("rx:",ul_ch128_2);
            //print_shorts("ch:",ul_ch128);
            //print_shorts("pack:",rho128);

            avg_rho_re[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[0])[0]+
              ((int16_t*)&rho128[0])[2] +
              ((int16_t*)&rho128[0])[4] +
              ((int16_t*)&rho128[0])[6])/16;//

            avg_rho_im[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[0])[1]+
              ((int16_t*)&rho128[0])[3] +
              ((int16_t*)&rho128[0])[5] +
              ((int16_t*)&rho128[0])[7])/16;//
            // multiply by conjugated channel
            mmtmpD0 = _mm_madd_epi16(ul_ch128[1],ul_ch128_2[1]);
            // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
            mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[1],_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
            mmtmpD1 = _mm_madd_epi16(mmtmpD1,ul_ch128_2[1]);
            // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
            mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
            mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
            mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
            rho128[1] =_mm_packs_epi32(mmtmpD2,mmtmpD3);
            //print_shorts("rx:",ul_ch128_2+1);
            //print_shorts("ch:",ul_ch128+1);
            //print_shorts("pack:",rho128+1);

            // multiply by conjugated channel
            avg_rho_re[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[1])[0]+
              ((int16_t*)&rho128[1])[2] +
              ((int16_t*)&rho128[1])[4] +
              ((int16_t*)&rho128[1])[6])/16;

            avg_rho_im[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[1])[1]+
              ((int16_t*)&rho128[1])[3] +
              ((int16_t*)&rho128[1])[5] +
              ((int16_t*)&rho128[1])[7])/16;

            mmtmpD0 = _mm_madd_epi16(ul_ch128[2],ul_ch128_2[2]);
            // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
            mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[2],_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
            mmtmpD1 = _mm_madd_epi16(mmtmpD1,ul_ch128_2[2]);
            // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
            mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
            mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
            mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

            rho128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
            //print_shorts("rx:",ul_ch128_2+2);
            //print_shorts("ch:",ul_ch128+2);
            //print_shorts("pack:",rho128+2);
            avg_rho_re[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[2])[0]+
              ((int16_t*)&rho128[2])[2] +
              ((int16_t*)&rho128[2])[4] +
              ((int16_t*)&rho128[2])[6])/16;

            avg_rho_im[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[2])[1]+
              ((int16_t*)&rho128[2])[3] +
              ((int16_t*)&rho128[2])[5] +
              ((int16_t*)&rho128[2])[7])/16;

            ul_ch128+=3;
            ul_ch128_2+=3;
            rho128+=3;
          }
          if (is_dmrs_symbol==1) {
            //measurements->rx_correlation[0][0][aarx] = signal_energy(&rho[aarx][aatx*nb_aatx+atx][symbol*nb_rb*12],rb*12);
            avg_rho_re[aarx][aatx*nrOfLayers+atx] = 16*avg_rho_re[aarx][aatx*nrOfLayers+atx]/(nb_rb*12);
            avg_rho_im[aarx][aatx*nrOfLayers+atx] = 16*avg_rho_im[aarx][aatx*nrOfLayers+atx]/(nb_rb*12);
            //printf("rho[rx]%d tx%d tx%d = Re: %d Im: %d\n",aarx, aatx,atx, avg_rho_re[aarx][aatx*nb_aatx+atx], avg_rho_im[aarx][aatx*nb_aatx+atx]);
          }
        }
      }
    }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  unsigned short rb;
  unsigned char aatx,aarx,symbol_mod,is_dmrs_symbol=0;

  int16x4_t *ul_ch128,*ul_ch128_2,*rxdataF128;
  int32x4_t mmtmpD0,mmtmpD1,mmtmpD0b,mmtmpD1b;
  int16x8_t *ul_ch_mag128,*ul_ch_mag128b,mmtmpD2,mmtmpD3,mmtmpD4;
  int16x8_t QAM_amp128,QAM_amp128b;
  int16x4x2_t *rxdataF_comp128,*rho128;

  int16_t conj[4]__attribute__((aligned(16))) = {1,-1,1,-1};
  int32x4_t output_shift128 = vmovq_n_s32(-(int32_t)output_shift);

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (nrOfLayers==1) { // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    }
    else {
      is_dmrs_symbol=1;
    }
  }

  for (aatx=0; aatx<nrOfLayers; aatx++) {
    if (mod_order == 4) {
      QAM_amp128  = vmovq_n_s16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = vmovq_n_s16(0);
    } else if (mod_order == 6) {
      QAM_amp128  = vmovq_n_s16(QAM64_n1); //
      QAM_amp128b = vmovq_n_s16(QAM64_n2);
    }
    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      ul_ch128          = (int16x4_t*)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch_mag128      = (int16x8_t*)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx+aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch_mag128b     = (int16x8_t*)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx+aarx][symbol*frame_parms->N_RB_UL*12];
      rxdataF128        = (int16x4_t*)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_UL*12];
      rxdataF_comp128   = (int16x4x2_t*)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx+aarx][symbol*frame_parms->N_RB_UL*12];

      for (rb=0; rb<nb_rb; rb++) {
  if (mod_order>2) {
    // get channel amplitude if not QPSK
    mmtmpD0 = vmull_s16(ul_ch128[0], ul_ch128[0]);
    // mmtmpD0 = [ch0*ch0,ch1*ch1,ch2*ch2,ch3*ch3];
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    // mmtmpD0 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3]>>output_shift128 on 32-bits
    mmtmpD1 = vmull_s16(ul_ch128[1], ul_ch128[1]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD2 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    // mmtmpD2 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3,ch4*ch4 + ch5*ch5,ch4*ch4 + ch5*ch5,ch6*ch6 + ch7*ch7,ch6*ch6 + ch7*ch7]>>output_shift128 on 16-bits
    mmtmpD0 = vmull_s16(ul_ch128[2], ul_ch128[2]);
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    mmtmpD1 = vmull_s16(ul_ch128[3], ul_ch128[3]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD3 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    if (is_dmrs_symbol==0) {
      mmtmpD0 = vmull_s16(ul_ch128[4], ul_ch128[4]);
      mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
      mmtmpD1 = vmull_s16(ul_ch128[5], ul_ch128[5]);
      mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
      mmtmpD4 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    }

    ul_ch_mag128b[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128b);
    ul_ch_mag128b[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128b);
    ul_ch_mag128[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128);
    ul_ch_mag128[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128);

    if (is_dmrs_symbol==0) {
      ul_ch_mag128b[2] = vqdmulhq_s16(mmtmpD4,QAM_amp128b);
      ul_ch_mag128[2]  = vqdmulhq_s16(mmtmpD4,QAM_amp128);
    }
  }

  mmtmpD0 = vmull_s16(ul_ch128[0], rxdataF128[0]);
  //mmtmpD0 = [Re(ch[0])Re(rx[0]) Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1]) Im(ch[1])Im(ch[1])]
  mmtmpD1 = vmull_s16(ul_ch128[1], rxdataF128[1]);
  //mmtmpD1 = [Re(ch[2])Re(rx[2]) Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3]) Im(ch[3])Im(ch[3])]
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  //mmtmpD0 = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])]

  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[0],*(int16x4_t*)conj)), rxdataF128[0]);
  //mmtmpD0 = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[1],*(int16x4_t*)conj)), rxdataF128[1]);
  //mmtmpD0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
  //mmtmpD1 = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rxdataF_comp128[0] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
  mmtmpD0 = vmull_s16(ul_ch128[2], rxdataF128[2]);
  mmtmpD1 = vmull_s16(ul_ch128[3], rxdataF128[3]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[2],*(int16x4_t*)conj)), rxdataF128[2]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[3],*(int16x4_t*)conj)), rxdataF128[3]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rxdataF_comp128[1] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  if (is_dmrs_symbol==0) {
    mmtmpD0 = vmull_s16(ul_ch128[4], rxdataF128[4]);
    mmtmpD1 = vmull_s16(ul_ch128[5], rxdataF128[5]);
    mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
         vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));

    mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[4],*(int16x4_t*)conj)), rxdataF128[4]);
    mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[5],*(int16x4_t*)conj)), rxdataF128[5]);
    mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
         vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));


    mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
    mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
    rxdataF_comp128[2] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));


    ul_ch128+=6;
    ul_ch_mag128+=3;
    ul_ch_mag128b+=3;
    rxdataF128+=6;
    rxdataF_comp128+=3;

  } else { // we have a smaller PUSCH in symbols with pilots so skip last group of 4 REs and increment less
    ul_ch128+=4;
    ul_ch_mag128+=2;
    ul_ch_mag128b+=2;
    rxdataF128+=4;
    rxdataF_comp128+=2;
  }
      }
    }
  }

  if (rho) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      rho128        = (int16x4x2_t*)&rho[aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch128      = (int16x4_t*)&ul_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch128_2    = (int16x4_t*)&ul_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_UL*12];
      for (rb=0; rb<nb_rb; rb++) {
  mmtmpD0 = vmull_s16(ul_ch128[0], ul_ch128_2[0]);
  mmtmpD1 = vmull_s16(ul_ch128[1], ul_ch128_2[1]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[0],*(int16x4_t*)conj)), ul_ch128_2[0]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[1],*(int16x4_t*)conj)), ul_ch128_2[1]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[0] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  mmtmpD0 = vmull_s16(ul_ch128[2], ul_ch128_2[2]);
  mmtmpD1 = vmull_s16(ul_ch128[3], ul_ch128_2[3]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[2],*(int16x4_t*)conj)), ul_ch128_2[2]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[3],*(int16x4_t*)conj)), ul_ch128_2[3]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[1] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  mmtmpD0 = vmull_s16(ul_ch128[0], ul_ch128_2[0]);
  mmtmpD1 = vmull_s16(ul_ch128[1], ul_ch128_2[1]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[4],*(int16x4_t*)conj)), ul_ch128_2[4]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[5],*(int16x4_t*)conj)), ul_ch128_2[5]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[2] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));


  ul_ch128+=6;
  ul_ch128_2+=6;
  rho128+=3;
      }
    }
  }
#endif


#ifdef DEBUG_CH_COMP
  for (int nl2=0; nl2<nrOfLayers; nl2++) {
    for (int aarx2=0; aarx2<frame_parms->nb_antennas_rx; aarx2++) {
      rxF   = (int16_t *)&rxdataF_comp[nl2*frame_parms->nb_antennas_rx+aarx2][(symbol*(off+(nb_rb*12)))];

      printf("--------After compansation, layer %i, antenna rx %i----------\n", nl2, aarx2);

      for (prnt_idx=0;prnt_idx<12*5*2;prnt_idx+=2){
        printf("rxF[%d] = (%d,%d)\n", prnt_idx>>1, rxF[prnt_idx],rxF[prnt_idx+1]);
      }
    }
  }
#endif

#ifdef DEBUG_CH_MAG


  for (int ant=0; ant<frame_parms->nb_antennas_rx; ant++) {
    ch_mag   = (int16_t *)&ul_ch_mag[ant][(symbol*(off+(nb_rb*12)))];

    printf("----------------After computation------------------\n");

    for (print_idx=0;print_idx<12*5*2;print_idx+=2){

      printf("ch_mag[%d] = (%d,%d)\n", print_idx>>1, ch_mag[print_idx],ch_mag[print_idx+1]);

    }
  }

#endif

}

void nr_ulsch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                int32_t **rxdataF_comp,
                int32_t **ul_ch_mag,
                int32_t **ul_ch_magb,
                int32_t ***rho,                
                uint8_t  nrOfLayers,
                uint8_t symbol,
                uint16_t nb_rb,
                int length) {
  int n_rx = frame_parms->nb_antennas_rx;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxdataF_comp128[2],*ul_ch_mag128[2],*ul_ch_mag128b[2];
#elif defined(__arm__)
  int16x8_t *rxdataF_comp128_0,*ul_ch_mag128_0,*ul_ch_mag128_0b;
  int16x8_t *rxdataF_comp128_1,*ul_ch_mag128_1,*ul_ch_mag128_1b;
#endif
  int32_t i;
  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

#ifdef __AVX2__
  int off = ((nb_rb&1) == 1)? 4:0;
#else
  int off = 0;
#endif

  if (n_rx > 1) {
    #if defined(__x86_64__) || defined(__i386__)
    for (int aatx=0; aatx<nrOfLayers; aatx++) {
      int nb_re = nb_rb*12;

      rxdataF_comp128[0]   = (__m128i *)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];
      ul_ch_mag128[0]      = (__m128i *)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];
      ul_ch_mag128b[0]     = (__m128i *)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];

      for (int aa=1;aa < n_rx;aa++) {
        rxdataF_comp128[1]   = (__m128i *)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];
        ul_ch_mag128[1]      = (__m128i *)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];
        ul_ch_mag128b[1]     = (__m128i *)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];
      
        // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
        for (i=0; i<nb_rb_0*3; i++) {
            rxdataF_comp128[0][i] = _mm_adds_epi16(rxdataF_comp128[0][i],rxdataF_comp128[1][i]);
            ul_ch_mag128[0][i]    = _mm_adds_epi16(ul_ch_mag128[0][i],ul_ch_mag128[1][i]);
            ul_ch_mag128b[0][i]   = _mm_adds_epi16(ul_ch_mag128b[0][i],ul_ch_mag128b[1][i]);
            //rxdataF_comp128[0][i] = _mm_add_epi16(rxdataF_comp128_0[i],(*(__m128i *)&jitterc[0]));
        }
      }
    }
    #elif defined(__arm__)
    rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[0][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[1][symbol*frame_parms->N_RB_DL*12];
    ul_ch_mag128_0      = (int16x8_t *)&ul_ch_mag[0][symbol*frame_parms->N_RB_DL*12];
    ul_ch_mag128_1      = (int16x8_t *)&ul_ch_mag[1][symbol*frame_parms->N_RB_DL*12];
    ul_ch_mag128_0b     = (int16x8_t *)&ul_ch_magb[0][symbol*frame_parms->N_RB_DL*12];
    ul_ch_mag128_1b     = (int16x8_t *)&ul_ch_magb[1][symbol*frame_parms->N_RB_DL*12];
      
    // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
    for (i=0; i<nb_rb*3; i++) {
      rxdataF_comp128_0[i] = vhaddq_s16(rxdataF_comp128_0[i],rxdataF_comp128_1[i]);
      ul_ch_mag128_0[i]    = vhaddq_s16(ul_ch_mag128_0[i],ul_ch_mag128_1[i]);
      ul_ch_mag128_0b[i]   = vhaddq_s16(ul_ch_mag128_0b[i],ul_ch_mag128_1b[i]);
      rxdataF_comp128_0[i] = vqaddq_s16(rxdataF_comp128_0[i],(*(int16x8_t *)&jitterc[0]));
    }
    #endif
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

/* Zero Forcing Rx function: nr_ulsch_zero_forcing_rx_2layers()
 *
 *
 * */
uint8_t nr_ulsch_zero_forcing_rx_2layers(int **rxdataF_comp,
                                   int **ul_ch_mag,
                                   int **ul_ch_magb,                                   
                                   int **ul_ch_estimates_ext,
                                   unsigned short nb_rb,
                                   unsigned char n_rx,
                                   unsigned char mod_order,
                                   int shift,
                                   unsigned char symbol,
                                   int length)
{
  int *ch00, *ch01, *ch10, *ch11;
  int *ch20, *ch30, *ch21, *ch31;
  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

  #ifdef __AVX2__
  int off = ((nb_rb&1) == 1)? 4:0;
  #else
  int off = 0;
  #endif

  /* we need at least alignment to 16 bytes, let's put 32 to be sure
   * (maybe not necessary but doesn't hurt)
   */
  int32_t conjch00_ch01[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch01_ch00[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch10_ch11[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch11_ch10[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch00_ch00[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch01_ch01[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch10_ch10[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch11_ch11[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch20_ch20[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch21_ch21[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch30_ch30[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch31_ch31[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch20_ch21[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch30_ch31[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch21_ch20[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch31_ch30[12*nb_rb] __attribute__((aligned(32)));

  int32_t af_mf_00[12*nb_rb] __attribute__((aligned(32)));
  int32_t af_mf_01[12*nb_rb] __attribute__((aligned(32)));
  int32_t af_mf_10[12*nb_rb] __attribute__((aligned(32)));
  int32_t af_mf_11[12*nb_rb] __attribute__((aligned(32)));
  int32_t determ_fin[12*nb_rb] __attribute__((aligned(32)));

  switch (n_rx) {
    case 2://
      ch00 = (int *)&ul_ch_estimates_ext[0][symbol*(off+nb_rb*12)];
      ch01 = (int *)&ul_ch_estimates_ext[2][symbol*(off+nb_rb*12)];
      ch10 = (int *)&ul_ch_estimates_ext[1][symbol*(off+nb_rb*12)];
      ch11 = (int *)&ul_ch_estimates_ext[3][symbol*(off+nb_rb*12)];
      ch20 = NULL;
      ch21 = NULL;
      ch30 = NULL;
      ch31 = NULL;
      break;

    case 4://
      ch00 = (int *)&ul_ch_estimates_ext[0][symbol*(off+nb_rb*12)];
      ch01 = (int *)&ul_ch_estimates_ext[4][symbol*(off+nb_rb*12)];
      ch10 = (int *)&ul_ch_estimates_ext[1][symbol*(off+nb_rb*12)];
      ch11 = (int *)&ul_ch_estimates_ext[5][symbol*(off+nb_rb*12)];
      ch20 = (int *)&ul_ch_estimates_ext[2][symbol*(off+nb_rb*12)];
      ch21 = (int *)&ul_ch_estimates_ext[6][symbol*(off+nb_rb*12)];
      ch30 = (int *)&ul_ch_estimates_ext[3][symbol*(off+nb_rb*12)];
      ch31 = (int *)&ul_ch_estimates_ext[7][symbol*(off+nb_rb*12)];
      break;

    default:
      return -1;
      break;
  }

  /* 1- Compute the rx channel matrix after compensation: (1/2^log2_max)x(H_herm x H)
   * for n_rx = 2
   * |conj_H_00       conj_H_10|    | H_00         H_01|   |(conj_H_00xH_00+conj_H_10xH_10)   (conj_H_00xH_01+conj_H_10xH_11)|
   * |                         |  x |                  | = |                                                                 |
   * |conj_H_01       conj_H_11|    | H_10         H_11|   |(conj_H_01xH_00+conj_H_11xH_10)   (conj_H_01xH_01+conj_H_11xH_11)|
   *
   */

  if (n_rx>=2){
    // (1/2^log2_maxh)*conj_H_00xH_00: (1/(64*2))conjH_00*H_00*2^15
    nr_ulsch_conjch0_mult_ch1(ch00,
                        ch00,
                        conjch00_ch00,
                        nb_rb_0,
                        shift);
    // (1/2^log2_maxh)*conj_H_10xH_10: (1/(64*2))conjH_10*H_10*2^15
    nr_ulsch_conjch0_mult_ch1(ch10,
                        ch10,
                        conjch10_ch10,
                        nb_rb_0,
                        shift);
    // conj_H_00xH_01
    nr_ulsch_conjch0_mult_ch1(ch00,
                        ch01,
                        conjch00_ch01,
                        nb_rb_0,
                        shift); // this shift is equal to the channel level log2_maxh
    // conj_H_10xH_11
    nr_ulsch_conjch0_mult_ch1(ch10,
                        ch11,
                        conjch10_ch11,
                        nb_rb_0,
                        shift);
    // conj_H_01xH_01
    nr_ulsch_conjch0_mult_ch1(ch01,
                        ch01,
                        conjch01_ch01,
                        nb_rb_0,
                        shift);
    // conj_H_11xH_11
    nr_ulsch_conjch0_mult_ch1(ch11,
                        ch11,
                        conjch11_ch11,
                        nb_rb_0,
                        shift);
    // conj_H_01xH_00
    nr_ulsch_conjch0_mult_ch1(ch01,
                        ch00,
                        conjch01_ch00,
                        nb_rb_0,
                        shift);
    // conj_H_11xH_10
    nr_ulsch_conjch0_mult_ch1(ch11,
                        ch10,
                        conjch11_ch10,
                        nb_rb_0,
                        shift);
  }
  if (n_rx==4){
    // (1/2^log2_maxh)*conj_H_20xH_20: (1/(64*2*16))conjH_20*H_20*2^15
    nr_ulsch_conjch0_mult_ch1(ch20,
                        ch20,
                        conjch20_ch20,
                        nb_rb_0,
                        shift);

    // (1/2^log2_maxh)*conj_H_30xH_30: (1/(64*2*4))conjH_30*H_30*2^15
    nr_ulsch_conjch0_mult_ch1(ch30,
                        ch30,
                        conjch30_ch30,
                        nb_rb_0,
                        shift);

    // (1/2^log2_maxh)*conj_H_20xH_20: (1/(64*2))conjH_20*H_20*2^15
    nr_ulsch_conjch0_mult_ch1(ch20,
                        ch21,
                        conjch20_ch21,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch30,
                        ch31,
                        conjch30_ch31,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch21,
                        ch21,
                        conjch21_ch21,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch31,
                        ch31,
                        conjch31_ch31,
                        nb_rb_0,
                        shift);

    // (1/2^log2_maxh)*conj_H_20xH_20: (1/(64*2))conjH_20*H_20*2^15
    nr_ulsch_conjch0_mult_ch1(ch21,
                        ch20,
                        conjch21_ch20,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch31,
                        ch30,
                        conjch31_ch30,
                        nb_rb_0,
                        shift);

    nr_ulsch_construct_HhH_elements(conjch00_ch00,
                              conjch01_ch01,
                              conjch11_ch11,
                              conjch10_ch10,//
                              conjch20_ch20,
                              conjch21_ch21,
                              conjch30_ch30,
                              conjch31_ch31,
                              conjch00_ch01,
                              conjch01_ch00,
                              conjch10_ch11,
                              conjch11_ch10,//
                              conjch20_ch21,
                              conjch21_ch20,
                              conjch30_ch31,
                              conjch31_ch30,
                              af_mf_00,
                              af_mf_01,
                              af_mf_10,
                              af_mf_11,
                              nb_rb_0,
                              symbol);
  }
  if (n_rx==2){
    nr_ulsch_construct_HhH_elements(conjch00_ch00,
                              conjch01_ch01,
                              conjch11_ch11,
                              conjch10_ch10,//
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              conjch00_ch01,
                              conjch01_ch00,
                              conjch10_ch11,
                              conjch11_ch10,//
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              af_mf_00,
                              af_mf_01,
                              af_mf_10,
                              af_mf_11,
                              nb_rb_0,
                              symbol);
  }
  //det_HhH = ad -bc
  nr_ulsch_det_HhH(af_mf_00,//a
             af_mf_01,//b
             af_mf_10,//c
             af_mf_11,//d
             determ_fin,
             nb_rb_0,
             symbol,
             shift);
  /* 2- Compute the channel matrix inversion **********************************
   *
     *    |(conj_H_00xH_00+conj_H_10xH_10)   (conj_H_00xH_01+conj_H_10xH_11)|
     * A= |                                                                 |
     *    |(conj_H_01xH_00+conj_H_11xH_10)   (conj_H_01xH_01+conj_H_11xH_11)|
     *
     *
     *
     *inv(A) =(1/det)*[d  -b
     *                 -c  a]
     *
     *
     **************************************************************************/
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1,*ul_ch_mag128_0=NULL,*ul_ch_mag128b_0=NULL,*determ_fin_128;//*dl_ch_mag128_1,*dl_ch_mag128b_1,*dl_ch_mag128r_1
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3;
  __m128i *after_mf_a_128,*after_mf_b_128, *after_mf_c_128, *after_mf_d_128;
  __m128i QAM_amp128={0},QAM_amp128b={0};

  determ_fin_128      = (__m128i *)&determ_fin[0];

  rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[0][symbol*(off+nb_rb*12)];//aatx=0 @ aarx =0
  rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[n_rx][symbol*(off+nb_rb*12)];//aatx=1 @ aarx =0

  after_mf_a_128 = (__m128i *)af_mf_00;
  after_mf_b_128 = (__m128i *)af_mf_01;
  after_mf_c_128 = (__m128i *)af_mf_10;
  after_mf_d_128 = (__m128i *)af_mf_11;

  if (mod_order>2) {
    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  //2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //4/sqrt{42}
      QAM_amp128b = _mm_set1_epi16(QAM64_n2); //2/sqrt{42}
    } 
    ul_ch_mag128_0      = (__m128i *)&ul_ch_mag[0][symbol*(off+nb_rb*12)];
    ul_ch_mag128b_0     = (__m128i *)&ul_ch_magb[0][symbol*(off+nb_rb*12)];
  }

  for (int rb=0; rb<3*nb_rb_0; rb++) {
    if (mod_order>2) {
      int sum_det =0;
      for (int k=0; k<4;k++) {
        sum_det += ((((int *)&determ_fin_128[0])[k])>>2);
        //printf("det_%d = %d\n",k,sum_det);
        }

      mmtmpD2 = _mm_slli_epi32(determ_fin_128[0],5);
      mmtmpD2 = _mm_srai_epi32(mmtmpD2,log2_approx(sum_det));
      mmtmpD2 = _mm_slli_epi32(mmtmpD2,5);

      mmtmpD3 = _mm_unpacklo_epi32(mmtmpD2,mmtmpD2);

      mmtmpD2 = _mm_unpackhi_epi32(mmtmpD2,mmtmpD2);

      mmtmpD2 = _mm_packs_epi32(mmtmpD3,mmtmpD2);

      ul_ch_mag128_0[0] = mmtmpD2;
      ul_ch_mag128b_0[0] = mmtmpD2;

      ul_ch_mag128_0[0] = _mm_mulhi_epi16(ul_ch_mag128_0[0],QAM_amp128);
      ul_ch_mag128_0[0] = _mm_slli_epi16(ul_ch_mag128_0[0],1);

      ul_ch_mag128b_0[0] = _mm_mulhi_epi16(ul_ch_mag128b_0[0],QAM_amp128b);
      ul_ch_mag128b_0[0] = _mm_slli_epi16(ul_ch_mag128b_0[0],1);

      //print_shorts("mag layer 1:",(int16_t*)&dl_ch_mag128_0[0]);
      //print_shorts("mag layer 2:",(int16_t*)&dl_ch_mag128_1[0]);
      //print_shorts("magb layer 1:",(int16_t*)&dl_ch_mag128b_0[0]);
      //print_shorts("magb layer 2:",(int16_t*)&dl_ch_mag128b_1[0]);
      //print_shorts("magr layer 1:",(int16_t*)&dl_ch_mag128r_0[0]);
      //print_shorts("magr layer 2:",(int16_t*)&dl_ch_mag128r_1[0]);
    }
    // multiply by channel Inv
    //rxdataF_zf128_0 = rxdataF_comp128_0*d - b*rxdataF_comp128_1
    //rxdataF_zf128_1 = rxdataF_comp128_1*a - c*rxdataF_comp128_0
    //printf("layer_1 \n");
    mmtmpD0 = nr_ulsch_comp_muli_sum(rxdataF_comp128_0[0],
                               after_mf_d_128[0],
                               rxdataF_comp128_1[0],
                               after_mf_b_128[0],
                               determ_fin_128[0]);

    //printf("layer_2 \n");
    mmtmpD1 = nr_ulsch_comp_muli_sum(rxdataF_comp128_1[0],
                               after_mf_a_128[0],
                               rxdataF_comp128_0[0],
                               after_mf_c_128[0],
                               determ_fin_128[0]);

    rxdataF_comp128_0[0] = mmtmpD0;
    rxdataF_comp128_1[0] = mmtmpD1;
#ifdef DEBUG_DLSCH_DEMOD
    printf("\n Rx signal after ZF l%d rb%d\n",symbol,rb);
    print_shorts(" Rx layer 1:",(int16_t*)&rxdataF_comp128_0[0]);
    print_shorts(" Rx layer 2:",(int16_t*)&rxdataF_comp128_1[0]);
#endif
    determ_fin_128 += 1;
    ul_ch_mag128_0 += 1;
    ul_ch_mag128b_0 += 1;    
    rxdataF_comp128_0 += 1;
    rxdataF_comp128_1 += 1;
    after_mf_a_128 += 1;
    after_mf_b_128 += 1;
    after_mf_c_128 += 1;
    after_mf_d_128 += 1;
  }
  _mm_empty();
  _m_empty();
   return(0);
}

//==============================================================================================

/* Main Function */
void nr_rx_pusch(PHY_VARS_gNB *gNB,
                 uint8_t ulsch_id,
                 uint32_t frame,
                 uint8_t slot,
                 unsigned char harq_pid)
{

  uint8_t aarx, aatx;
  uint32_t nb_re_pusch, bwp_start_subcarrier;
  int avgs = 0;

  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_pusch_pdu_t *rel15_ul = &gNB->ulsch[ulsch_id]->harq_processes[harq_pid]->ulsch_pdu;
  int avg[frame_parms->nb_antennas_rx*rel15_ul->nrOfLayers];

  gNB->pusch_vars[ulsch_id]->dmrs_symbol = INVALID_VALUE;
  gNB->pusch_vars[ulsch_id]->cl_done = 0;

  bwp_start_subcarrier = ((rel15_ul->rb_start + rel15_ul->bwp_start)*NR_NB_SC_PER_RB + frame_parms->first_carrier_offset) % frame_parms->ofdm_symbol_size;
  LOG_D(PHY,"pusch %d.%d : bwp_start_subcarrier %d, rb_start %d, first_carrier_offset %d\n", frame,slot,bwp_start_subcarrier, rel15_ul->rb_start, frame_parms->first_carrier_offset);
  LOG_D(PHY,"pusch %d.%d : ul_dmrs_symb_pos %x\n",frame,slot,rel15_ul->ul_dmrs_symb_pos);
  LOG_D(PHY,"ulsch RX %x : start_rb %d nb_rb %d mcs %d Nl %d Tpmi %d bwp_start %d start_sc %d start_symbol %d num_symbols %d cdmgrpsnodata %d num_dmrs %d dmrs_ports %d\n",
          rel15_ul->rnti,rel15_ul->rb_start,rel15_ul->rb_size,rel15_ul->mcs_index,
          rel15_ul->nrOfLayers,0,rel15_ul->bwp_start,0,rel15_ul->start_symbol_index,rel15_ul->nr_of_symbols,
          rel15_ul->num_dmrs_cdm_grps_no_data,rel15_ul->ul_dmrs_symb_pos,rel15_ul->dmrs_ports);
  //----------------------------------------------------------
  //--------------------- Channel estimation ---------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_channel_estimation_stats);
  for(uint8_t symbol = rel15_ul->start_symbol_index; symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); symbol++) {
    uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    LOG_D(PHY, "symbol %d, dmrs_symbol_flag :%d\n", symbol, dmrs_symbol_flag);
    
    if (dmrs_symbol_flag == 1) {
      if (gNB->pusch_vars[ulsch_id]->dmrs_symbol == INVALID_VALUE)
        gNB->pusch_vars[ulsch_id]->dmrs_symbol = symbol;

      for (int nl=0; nl<rel15_ul->nrOfLayers; nl++) {
        
        nr_pusch_channel_estimation(gNB,
                                    slot,
                                    get_dmrs_port(nl,rel15_ul->dmrs_ports),
                                    symbol,
                                    ulsch_id,
                                    bwp_start_subcarrier,
                                    rel15_ul);
      }

      nr_gnb_measurements(gNB, ulsch_id, harq_pid, symbol,rel15_ul->nrOfLayers);

      for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
        if (symbol == rel15_ul->start_symbol_index) {
          gNB->pusch_vars[ulsch_id]->ulsch_power[aarx] = 0;
          gNB->pusch_vars[ulsch_id]->ulsch_noise_power[aarx] = 0;
        }
        for (aatx = 0; aatx < rel15_ul->nrOfLayers; aatx++) {
          gNB->pusch_vars[ulsch_id]->ulsch_power[aarx] += signal_energy_nodc(
            &gNB->pusch_vars[ulsch_id]->ul_ch_estimates[aatx*gNB->frame_parms.nb_antennas_rx+aarx][symbol * frame_parms->ofdm_symbol_size],
            rel15_ul->rb_size * 12);
        }
        for (int rb = 0; rb < rel15_ul->rb_size; rb++) {
          gNB->pusch_vars[ulsch_id]->ulsch_noise_power[aarx] +=
              gNB->measurements.n0_subband_power[aarx][rel15_ul->bwp_start + rel15_ul->rb_start + rb] /
              rel15_ul->rb_size;
        }
        LOG_D(PHY,"aa %d, bwp_start%d, rb_start %d, rb_size %d: ulsch_power %d, ulsch_noise_power %d\n",aarx,
	      rel15_ul->bwp_start,rel15_ul->rb_start,rel15_ul->rb_size,
              gNB->pusch_vars[ulsch_id]->ulsch_power[aarx],
              gNB->pusch_vars[ulsch_id]->ulsch_noise_power[aarx]);
      }
    }
  }

  if (gNB->chest_time == 1) { // averaging time domain channel estimates
    nr_chest_time_domain_avg(frame_parms,
                             gNB->pusch_vars[ulsch_id]->ul_ch_estimates,
                             rel15_ul->nr_of_symbols,
                             rel15_ul->start_symbol_index,
                             rel15_ul->ul_dmrs_symb_pos,
                             rel15_ul->rb_size);

    gNB->pusch_vars[ulsch_id]->dmrs_symbol = get_next_dmrs_symbol_in_slot(rel15_ul->ul_dmrs_symb_pos, rel15_ul->start_symbol_index, rel15_ul->nr_of_symbols);
  }
  stop_meas(&gNB->ulsch_channel_estimation_stats);

#ifdef __AVX2__
  int off = ((rel15_ul->rb_size&1) == 1)? 4:0;
#else
  int off = 0;
#endif
  uint32_t rxdataF_ext_offset = 0;

  for(uint8_t symbol = rel15_ul->start_symbol_index; symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); symbol++) {
    uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    if (dmrs_symbol_flag == 1) {
      if ((rel15_ul->ul_dmrs_symb_pos >> ((symbol + 1) % frame_parms->symbols_per_slot)) & 0x01)
        AssertFatal(1==0,"Double DMRS configuration is not yet supported\n");

      if (gNB->chest_time == 0) // Non averaging time domain channel estimates
        gNB->pusch_vars[ulsch_id]->dmrs_symbol = symbol;

      if (rel15_ul->dmrs_config_type == 0) {
        // if no data in dmrs cdm group is 1 only even REs have no data
        // if no data in dmrs cdm group is 2 both odd and even REs have no data
        nb_re_pusch = rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*6));
      }
      else {
        nb_re_pusch = rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*4));
      }
    } 
    else {
      nb_re_pusch = rel15_ul->rb_size * NR_NB_SC_PER_RB;
    }

    gNB->pusch_vars[ulsch_id]->ul_valid_re_per_slot[symbol] = nb_re_pusch;
    LOG_D(PHY,"symbol %d: nb_re_pusch %d, DMRS symbl used for Chest :%d \n", symbol, nb_re_pusch, gNB->pusch_vars[ulsch_id]->dmrs_symbol);

    //----------------------------------------------------------
    //--------------------- RBs extraction ---------------------
    //----------------------------------------------------------
    if (nb_re_pusch > 0) {
      start_meas(&gNB->ulsch_rbs_extraction_stats);
      nr_ulsch_extract_rbs(gNB->common_vars.rxdataF,
                           gNB->pusch_vars[ulsch_id],
                           slot,
                           symbol,
                           dmrs_symbol_flag,
                           rel15_ul,
                           frame_parms);
      stop_meas(&gNB->ulsch_rbs_extraction_stats);

      //----------------------------------------------------------
      //--------------------- Channel Scaling --------------------
      //----------------------------------------------------------
      nr_ulsch_scale_channel(gNB->pusch_vars[ulsch_id]->ul_ch_estimates_ext,
                            frame_parms,
                            gNB->ulsch[ulsch_id],
                            symbol,
                            dmrs_symbol_flag,
                            nb_re_pusch,
                            rel15_ul->nrOfLayers,
                            rel15_ul->rb_size);

      if (gNB->pusch_vars[ulsch_id]->cl_done==0) {
        nr_ulsch_channel_level(gNB->pusch_vars[ulsch_id]->ul_ch_estimates_ext,
                              frame_parms,
                              avg,
                              symbol,
                              nb_re_pusch,
                              rel15_ul->nrOfLayers,
                              rel15_ul->rb_size);

        avgs = 0;

        for (aatx=0;aatx<rel15_ul->nrOfLayers;aatx++)
          for (aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++)
            avgs = cmax(avgs,avg[aatx*frame_parms->nb_antennas_rx+aarx]);

        gNB->pusch_vars[ulsch_id]->log2_maxh = (log2_approx(avgs)/2)+2;
        gNB->pusch_vars[ulsch_id]->cl_done = 1;
      }

      //----------------------------------------------------------
      //--------------------- Channel Compensation ---------------
      //----------------------------------------------------------
      start_meas(&gNB->ulsch_channel_compensation_stats);
      LOG_D(PHY,"Doing channel compensations log2_maxh %d, avgs %d (%d,%d)\n",gNB->pusch_vars[ulsch_id]->log2_maxh,avgs,avg[0],avg[1]);
      nr_ulsch_channel_compensation(gNB->pusch_vars[ulsch_id]->rxdataF_ext,
                                    gNB->pusch_vars[ulsch_id]->ul_ch_estimates_ext,
                                    gNB->pusch_vars[ulsch_id]->ul_ch_mag0,
                                    gNB->pusch_vars[ulsch_id]->ul_ch_magb0,
                                    gNB->pusch_vars[ulsch_id]->rxdataF_comp,
                                    (rel15_ul->nrOfLayers>1) ? gNB->pusch_vars[ulsch_id]->rho : NULL,
                                    frame_parms,
                                    symbol,
                                    nb_re_pusch,
                                    dmrs_symbol_flag,
                                    rel15_ul->qam_mod_order,
                                    rel15_ul->nrOfLayers,
                                    rel15_ul->rb_size,
                                    gNB->pusch_vars[ulsch_id]->log2_maxh);
      stop_meas(&gNB->ulsch_channel_compensation_stats);

      start_meas(&gNB->ulsch_mrc_stats);
      nr_ulsch_detection_mrc(frame_parms,
                             gNB->pusch_vars[ulsch_id]->rxdataF_comp,
                             gNB->pusch_vars[ulsch_id]->ul_ch_mag0,
                             gNB->pusch_vars[ulsch_id]->ul_ch_magb0,
                             (rel15_ul->nrOfLayers>1) ? gNB->pusch_vars[ulsch_id]->rho : NULL,
                             rel15_ul->nrOfLayers,
                             symbol,
                             rel15_ul->rb_size,
                             nb_re_pusch);
                 
      if (rel15_ul->nrOfLayers == 2)//Apply zero forcing for 2 Tx layers
        nr_ulsch_zero_forcing_rx_2layers(gNB->pusch_vars[ulsch_id]->rxdataF_comp,
                                   gNB->pusch_vars[ulsch_id]->ul_ch_mag0,
                                   gNB->pusch_vars[ulsch_id]->ul_ch_magb0,                                   
                                   gNB->pusch_vars[ulsch_id]->ul_ch_estimates_ext,
                                   rel15_ul->rb_size,
                                   frame_parms->nb_antennas_rx,
                                   rel15_ul->qam_mod_order,
                                   gNB->pusch_vars[ulsch_id]->log2_maxh,
                                   symbol,
                                   nb_re_pusch);
      stop_meas(&gNB->ulsch_mrc_stats);

      if (rel15_ul->transform_precoding == transformPrecoder_enabled) {
         #ifdef __AVX2__
        // For odd number of resource blocks need byte alignment to multiple of 8
        int nb_re_pusch2 = nb_re_pusch + (nb_re_pusch&7);
        #else
        int nb_re_pusch2 = nb_re_pusch;
        #endif

        // perform IDFT operation on the compensated rxdata if transform precoding is enabled
        nr_idft(&gNB->pusch_vars[ulsch_id]->rxdataF_comp[0][symbol * nb_re_pusch2], nb_re_pusch);
        LOG_D(PHY,"Transform precoding being done on data- symbol: %d, nb_re_pusch: %d\n", symbol, nb_re_pusch);
      }

      //----------------------------------------------------------
      //--------------------- PTRS Processing --------------------
      //----------------------------------------------------------
      /* In case PTRS is enabled then LLR will be calculated after PTRS symbols are processed *
      * otherwise LLR are calculated for each symbol based upon DMRS channel estimates only. */
      if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
        start_meas(&gNB->ulsch_ptrs_processing_stats);
        nr_pusch_ptrs_processing(gNB,
                                 frame_parms,
                                 rel15_ul,
                                 ulsch_id,
                                 slot,
                                 symbol,
                                 nb_re_pusch);
        stop_meas(&gNB->ulsch_ptrs_processing_stats);

        /*  Subtract total PTRS RE's in the symbol from PUSCH RE's */
        gNB->pusch_vars[ulsch_id]->ul_valid_re_per_slot[symbol] -= gNB->pusch_vars[ulsch_id]->ptrs_re_per_slot;
      }

      /*---------------------------------------------------------------------------------------------------- */
      /*--------------------  LLRs computation  -------------------------------------------------------------*/
      /*-----------------------------------------------------------------------------------------------------*/
      start_meas(&gNB->ulsch_llr_stats);
      for (aatx=0; aatx < rel15_ul->nrOfLayers; aatx++) {
        nr_ulsch_compute_llr(&gNB->pusch_vars[ulsch_id]->rxdataF_comp[aatx*frame_parms->nb_antennas_rx][symbol * (off + rel15_ul->rb_size * NR_NB_SC_PER_RB)],
                             gNB->pusch_vars[ulsch_id]->ul_ch_mag0[aatx*frame_parms->nb_antennas_rx],
                             gNB->pusch_vars[ulsch_id]->ul_ch_magb0[aatx*frame_parms->nb_antennas_rx],
                             &gNB->pusch_vars[ulsch_id]->llr_layers[aatx][rxdataF_ext_offset * rel15_ul->qam_mod_order],
                             rel15_ul->rb_size,
                             gNB->pusch_vars[ulsch_id]->ul_valid_re_per_slot[symbol],
                             symbol,
                             rel15_ul->qam_mod_order);
      }
      stop_meas(&gNB->ulsch_llr_stats);
      rxdataF_ext_offset += gNB->pusch_vars[ulsch_id]->ul_valid_re_per_slot[symbol];
    }
  } // symbol loop
}
