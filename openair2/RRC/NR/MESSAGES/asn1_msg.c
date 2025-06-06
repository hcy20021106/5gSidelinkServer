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

/*! \file asn1_msg.c
* \brief primitives to build the asn1 messages
* \author Raymond Knopp and Navid Nikaein, WEI-TAI CHEN
* \date 2011, 2018
* \version 1.0
* \company Eurecom, NTUST
* \email: {raymond.knopp, navid.nikaein}@eurecom.fr and kroempa@gmail.com
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h> /* for atoi(3) */
#include <unistd.h> /* for getopt(3) */
#include <string.h> /* for strerror(3) */
#include <sysexits.h> /* for EX_* exit codes */
#include <errno.h>  /* for errno */
#include "common/utils/LOG/log.h"
#include <asn_application.h>
#include <asn_internal.h> /* for _ASN_DEFAULT_STACK_MAX */
#include <per_encoder.h>
#include <nr/nr_common.h>
#include <softmodem-common.h>

#include "executables/softmodem-common.h"
#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "asn1_msg.h"
#include "../nr_rrc_proto.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "RRC/NR_UE/rrc_vars.h"
#include "NR_DL-CCCH-Message.h"
#include "NR_UL-CCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_RRCReject.h"
#include "NR_RejectWaitTime.h"
#include "NR_RRCSetup.h"
#include "NR_RRCSetup-IEs.h"
#include "NR_SRB-ToAddModList.h"
#include "NR_CellGroupConfig.h"
#include "NR_RLC-BearerConfig.h"
#include "NR_RLC-Config.h"
#include "NR_LogicalChannelConfig.h"
#include "NR_PDCP-Config.h"
#include "NR_MAC-CellGroupConfig.h"
#include "NR_SecurityModeCommand.h"
#include "NR_CipheringAlgorithm.h"
#include "NR_RRCReconfiguration-IEs.h"
#include "NR_DRB-ToAddMod.h"
#include "NR_DRB-ToAddModList.h"
#include "NR_SecurityConfig.h"
#include "NR_RRCReconfiguration-v1530-IEs.h"
#include "NR_UL-DCCH-Message.h"
#include "NR_SDAP-Config.h"
#include "NR_RRCReconfigurationComplete.h"
#include "NR_RRCReconfigurationComplete-IEs.h"
#include "NR_DLInformationTransfer.h"
#include "NR_RRCReestablishmentRequest.h"
#include "NR_UE-CapabilityRequestFilterNR.h"
#include "PHY/defs_nr_common.h"
#include "common/utils/nr/nr_common.h"
#if defined(NR_Rel16)
  #include "NR_SCS-SpecificCarrier.h"
  #include "NR_TDD-UL-DL-ConfigCommon.h"
  #include "NR_FrequencyInfoUL.h"
  #include "NR_FrequencyInfoDL.h"
  #include "NR_RACH-ConfigGeneric.h"
  #include "NR_RACH-ConfigCommon.h"
  #include "NR_PUSCH-TimeDomainResourceAllocation.h"
  #include "NR_PUSCH-ConfigCommon.h"
  #include "NR_PUCCH-ConfigCommon.h"
  #include "NR_PDSCH-TimeDomainResourceAllocation.h"
  #include "NR_PDSCH-ConfigCommon.h"
  #include "NR_RateMatchPattern.h"
  #include "NR_RateMatchPatternLTE-CRS.h"
  #include "NR_SearchSpace.h"
  #include "NR_ControlResourceSet.h"
  #include "NR_EUTRA-MBSFN-SubframeConfig.h"
  #include "NR_BWP-DownlinkCommon.h"
  #include "NR_BWP-DownlinkDedicated.h"
  #include "NR_UplinkConfigCommon.h"
  #include "NR_SetupRelease.h"
  #include "NR_PDCCH-ConfigCommon.h"
  #include "NR_BWP-UplinkCommon.h"

  #include "assertions.h"
  //#include "RRCConnectionRequest.h"
  //#include "UL-CCCH-Message.h"
  #include "NR_UL-DCCH-Message.h"
  //#include "DL-CCCH-Message.h"
  #include "NR_DL-DCCH-Message.h"
  //#include "EstablishmentCause.h"
  //#include "RRCConnectionSetup.h"
  #include "NR_SRB-ToAddModList.h"
  #include "NR_DRB-ToAddModList.h"
  //#include "MCCH-Message.h"
  //#define MRB1 1

  //#include "RRCConnectionSetupComplete.h"
  //#include "RRCConnectionReconfigurationComplete.h"
  //#include "RRCConnectionReconfiguration.h"
  #include "NR_MIB.h"
  //#include "SystemInformation.h"

  #include "NR_SIB1.h"
  #include "NR_ServingCellConfigCommon.h"
  //#include "SIB-Type.h"

  //#include "BCCH-DL-SCH-Message.h"

  //#include "PHY/defs.h"

  #include "NR_MeasObjectToAddModList.h"
  #include "NR_ReportConfigToAddModList.h"
  #include "NR_MeasIdToAddModList.h"
  #include "gnb_config.h"
#endif

#include "intertask_interface.h"

#include "common/ran_context.h"

//#define XER_PRINT

typedef struct xer_sprint_string_s {
  char *string;
  size_t string_size;
  size_t string_index;
} xer_sprint_string_t;

//replace LTE
//extern unsigned char NB_eNB_INST;
extern unsigned char NB_gNB_INST;

extern RAN_CONTEXT_t RC;

/*
 * This is a helper function for xer_sprint, which directs all incoming data
 * into the provided string.
 */
static int xer__nr_print2s (const void *buffer, size_t size, void *app_key) {
  xer_sprint_string_t *string_buffer = (xer_sprint_string_t *) app_key;
  size_t string_remaining = string_buffer->string_size - string_buffer->string_index;

  if (string_remaining > 0) {
    if (size > string_remaining) {
      size = string_remaining;
    }

    memcpy(&string_buffer->string[string_buffer->string_index], buffer, size);
    string_buffer->string_index += size;
  }

  return 0;
}

int xer_nr_sprint (char *string, size_t string_size, asn_TYPE_descriptor_t *td, void *sptr) {
  asn_enc_rval_t er;
  xer_sprint_string_t string_buffer;
  string_buffer.string = string;
  string_buffer.string_size = string_size;
  string_buffer.string_index = 0;
  er = xer_encode(td, sptr, XER_F_BASIC, xer__nr_print2s, &string_buffer);

  if (er.encoded < 0) {
    LOG_E(RRC, "xer_sprint encoding error (%zd)!", er.encoded);
    er.encoded = string_buffer.string_size;
  } else {
    if (er.encoded > string_buffer.string_size) {
      LOG_E(RRC, "xer_sprint string buffer too small, got %zd need %zd!", string_buffer.string_size, er.encoded);
      er.encoded = string_buffer.string_size;
    }
  }

  return er.encoded;
}

//------------------------------------------------------------------------------
//TTN for D2D
// NR : 3GPP 38.331 (Section 5.8.5.3)
uint8_t do_MIB_SL_NR(const protocol_ctxt_t* const ctxt_pP, int abs_slot, NR_UE_RRC_INST_t* UE)
{
   asn_enc_rval_t enc_rval;
   uint16_t frame = abs_slot / 20;
   uint8_t slot = abs_slot % 20;

   NR_SBCCH_SL_BCH_MessageType_t *sl_mib = &UE->SL_mib_tx;
   sl_mib->present = NR_SBCCH_SL_BCH_MessageType_PR_c1;
   sl_mib->choice.c1 = CALLOC(1, sizeof(struct NR_SBCCH_SL_BCH_MessageType__c1));
   sl_mib->choice.c1->present = NR_SBCCH_SL_BCH_MessageType__c1_PR_masterInformationBlockSidelink;
   sl_mib->choice.c1->choice.masterInformationBlockSidelink = CALLOC(1, sizeof(NR_MasterInformationBlockSidelink_t));
   NR_MasterInformationBlockSidelink_t *nr_sl_mib = sl_mib->choice.c1->choice.masterInformationBlockSidelink;
   if (get_softmodem_params()->sl_mode != 2) {
      nr_sl_mib->inCoverage_r16 = 1;
      AssertFatal(1 == 0, "Needs development for in %s %d.\n", __FUNCTION__, __LINE__);
   } else {
     nr_sl_mib->inCoverage_r16 = 0;
     nr_sl_mib->sl_TDD_Config_r16.size = 2;
     nr_sl_mib->sl_TDD_Config_r16.buf = CALLOC(1, nr_sl_mib->sl_TDD_Config_r16.size);
     nr_sl_mib->sl_TDD_Config_r16.bits_unused = 4;
     nr_sl_mib->reservedBits_r16.size = 1;
     nr_sl_mib->reservedBits_r16.buf = CALLOC(1, nr_sl_mib->reservedBits_r16.size);
     nr_sl_mib->reservedBits_r16.bits_unused = 6;
   }

   nr_sl_mib->directFrameNumber_r16.size = 2;
   nr_sl_mib->directFrameNumber_r16.buf = CALLOC(1, nr_sl_mib->directFrameNumber_r16.size);
   nr_sl_mib->directFrameNumber_r16.buf[0] = frame & 255;
   nr_sl_mib->directFrameNumber_r16.buf[1] = (frame >> 8) & 3;
   nr_sl_mib->directFrameNumber_r16.bits_unused = 6;
   nr_sl_mib->slotIndex_r16.size = 1;
   nr_sl_mib->slotIndex_r16.buf = CALLOC(1, nr_sl_mib->slotIndex_r16.size);
   nr_sl_mib->slotIndex_r16.buf[0] = slot;
   nr_sl_mib->slotIndex_r16.bits_unused = 1;

   if ((frame & 255) == 0)
     LOG_D(NR_RRC, "[MIB-SL] sfn %x, slot %x\n", frame, slot);

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_SBCCH_SL_BCH_Message,
                                   NULL,
                                   (void*)&UE->SL_mib_tx,
                                   UE->SL_MIB,
                                   sizeof(UE->SL_MIB));

  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  return ((enc_rval.encoded + 7) / 8);
}

//------------------------------------------------------------------------------

uint8_t do_MIB_NR(gNB_RRC_INST *rrc,uint32_t frame) { 

  asn_enc_rval_t enc_rval;
  rrc_gNB_carrier_data_t *carrier = &rrc->carrier;

  NR_BCCH_BCH_Message_t *mib = &carrier->mib;
  NR_ServingCellConfigCommon_t *scc = carrier->servingcellconfigcommon;

  memset(mib,0,sizeof(NR_BCCH_BCH_Message_t));
  mib->message.present = NR_BCCH_BCH_MessageType_PR_mib;
  mib->message.choice.mib = CALLOC(1,sizeof(struct NR_MIB));
  memset(mib->message.choice.mib,0,sizeof(struct NR_MIB));
  //36.331 SFN BIT STRING (SIZE (8)  , 38.331 SFN BIT STRING (SIZE (6))
  uint8_t sfn_msb = (uint8_t)((frame>>4)&0x3f);
  mib->message.choice.mib->systemFrameNumber.buf = CALLOC(1,sizeof(uint8_t));
  mib->message.choice.mib->systemFrameNumber.buf[0] = sfn_msb << 2;
  mib->message.choice.mib->systemFrameNumber.size = 1;
  mib->message.choice.mib->systemFrameNumber.bits_unused=2;
  //38.331 spare BIT STRING (SIZE (1))
  uint16_t *spare= CALLOC(1, sizeof(uint16_t));

  if (spare == NULL) abort();

  mib->message.choice.mib->spare.buf = (uint8_t *)spare;
  mib->message.choice.mib->spare.size = 1;
  mib->message.choice.mib->spare.bits_unused = 7;  // This makes a spare of 1 bits

  AssertFatal(scc->ssbSubcarrierSpacing != NULL, "scc->ssbSubcarrierSpacing is null\n");
  int band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  frequency_range_t frequency_range = band<100?FR1:FR2;
  int ssb_subcarrier_offset = 31; // default value for NSA
  if (get_softmodem_params()->sa) {
    uint32_t absolute_diff = (*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB - scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);
    int scs_scaling = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA < 600000 ? 3 : 1;
    ssb_subcarrier_offset = (absolute_diff/scs_scaling)%24;
    if(frequency_range == FR2)
      ssb_subcarrier_offset >>= 1; // this assumes 120kHz SCS for SSB and subCarrierSpacingCommon (only option supported by OAI for now)
  }
  mib->message.choice.mib->ssb_SubcarrierOffset = ssb_subcarrier_offset&15;

  /*
  * The SIB1 will be sent in this allocation (Type0-PDCCH) : 38.213, 13-4 Table and 38.213 13-11 to 13-14 tables
  * the reverse allocation is in nr_ue_decode_mib()
  */
  if(rrc->carrier.pdcch_ConfigSIB1) {
    mib->message.choice.mib->pdcch_ConfigSIB1.controlResourceSetZero = rrc->carrier.pdcch_ConfigSIB1->controlResourceSetZero;
    mib->message.choice.mib->pdcch_ConfigSIB1.searchSpaceZero = rrc->carrier.pdcch_ConfigSIB1->searchSpaceZero;
  } else {
    mib->message.choice.mib->pdcch_ConfigSIB1.controlResourceSetZero = *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero;
    mib->message.choice.mib->pdcch_ConfigSIB1.searchSpaceZero = *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero;
  }

  switch (*scc->ssbSubcarrierSpacing) {
  case NR_SubcarrierSpacing_kHz15:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60;
    break;
    
  case NR_SubcarrierSpacing_kHz30:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs30or120;
    break;
    
  case NR_SubcarrierSpacing_kHz60:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60;
    break;
    
  case NR_SubcarrierSpacing_kHz120:
    mib->message.choice.mib->subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs30or120;
    break;
    
  case NR_SubcarrierSpacing_kHz240:
    AssertFatal(1==0,"Unknown subCarrierSpacingCommon %d\n",(int)*scc->ssbSubcarrierSpacing);
    break;
    
  default:
      AssertFatal(1==0,"Unknown subCarrierSpacingCommon %d\n",(int)*scc->ssbSubcarrierSpacing);
  }

  switch (scc->dmrs_TypeA_Position) {
  case 	NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2:
    mib->message.choice.mib->dmrs_TypeA_Position = NR_MIB__dmrs_TypeA_Position_pos2;
    break;
    
  case 	NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos3:
    mib->message.choice.mib->dmrs_TypeA_Position = NR_MIB__dmrs_TypeA_Position_pos3;
    break;
    
  default:
    AssertFatal(1==0,"Unknown dmrs_TypeA_Position %d\n",(int)scc->dmrs_TypeA_Position);
  }

  //  assign_enum
  mib->message.choice.mib->cellBarred = NR_MIB__cellBarred_notBarred;
  //  assign_enum
  mib->message.choice.mib->intraFreqReselection = NR_MIB__intraFreqReselection_notAllowed;
  //encode MIB to data
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                   NULL,
                                   (void *)mib,
                                   carrier->MIB,
                                   24);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

uint16_t do_SIB1_NR(rrc_gNB_carrier_data_t *carrier,
                    gNB_RrcConfigurationReq *configuration) {

  asn_enc_rval_t enc_rval;
  NR_BCCH_DL_SCH_Message_t *sib1_message = CALLOC(1,sizeof(NR_BCCH_DL_SCH_Message_t));
  carrier->siblock1 = sib1_message;
  sib1_message->message.present = NR_BCCH_DL_SCH_MessageType_PR_c1;
  sib1_message->message.choice.c1 = CALLOC(1,sizeof(struct NR_BCCH_DL_SCH_MessageType__c1));
  sib1_message->message.choice.c1->present = NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1;
  sib1_message->message.choice.c1->choice.systemInformationBlockType1 = CALLOC(1,sizeof(struct NR_SIB1));

  struct NR_SIB1 *sib1 = sib1_message->message.choice.c1->choice.systemInformationBlockType1;

  // cellSelectionInfo
  sib1->cellSelectionInfo = CALLOC(1,sizeof(struct NR_SIB1__cellSelectionInfo));
  // Fixme: should be in config file
  //The IE Q-RxLevMin is used to indicate for cell selection/ re-selection the required minimum received RSRP level in the (NR) cell.
  //Corresponds to parameter Qrxlevmin in TS38.304.
  //Actual value Qrxlevmin = field value * 2 [dBm].
  sib1->cellSelectionInfo->q_RxLevMin = -65;

  // cellAccessRelatedInfo
  // TODO : Add support for more than one PLMN
  int num_plmn = 1; // int num_plmn = configuration->num_plmn;
  asn1cSequenceAdd(sib1->cellAccessRelatedInfo.plmn_IdentityList.list, struct NR_PLMN_IdentityInfo, nr_plmn_info);
  for (int i = 0; i < num_plmn; ++i) {
    asn1cSequenceAdd(nr_plmn_info->plmn_IdentityList.list, struct NR_PLMN_Identity, nr_plmn);
    asn1cCalloc(nr_plmn->mcc,  mcc);
    int confMcc=configuration->mcc[i];
    asn1cSequenceAdd(mcc->list, NR_MCC_MNC_Digit_t, mcc0);
    *mcc0=(confMcc/100)%10;
    asn1cSequenceAdd(mcc->list, NR_MCC_MNC_Digit_t, mcc1);
    *mcc1=(confMcc/10)%10;
    asn1cSequenceAdd(mcc->list, NR_MCC_MNC_Digit_t, mcc2);
    *mcc2=confMcc%10;
    int mnc=configuration->mnc[i];
    if(configuration->mnc_digit_length[i] == 3) {
      asn1cSequenceAdd(nr_plmn->mnc.list, NR_MCC_MNC_Digit_t, mnc0);
      *mnc0=(configuration->mnc[i]/100)%10;
    }
    asn1cSequenceAdd(nr_plmn->mnc.list, NR_MCC_MNC_Digit_t, mnc1);
    *mnc1=(mnc/10)%10;
    asn1cSequenceAdd(nr_plmn->mnc.list, NR_MCC_MNC_Digit_t, mnc2);
    *mnc2=(mnc)%10;
  }//end plmn loop

  nr_plmn_info->cellIdentity.buf = CALLOC(1,5);
  nr_plmn_info->cellIdentity.size= 5;
  nr_plmn_info->cellIdentity.bits_unused= 4;
  uint64_t tmp=htobe64(configuration->cell_identity)<<4;
  memcpy(nr_plmn_info->cellIdentity.buf, ((char*)&tmp)+3, 5);
  nr_plmn_info->cellReservedForOperatorUse = NR_PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;

  nr_plmn_info->trackingAreaCode = CALLOC(1,sizeof(NR_TrackingAreaCode_t));
  uint32_t tmp2=htobe32(configuration->tac);
  nr_plmn_info->trackingAreaCode->buf = CALLOC(1,3);
  memcpy(nr_plmn_info->trackingAreaCode->buf, ((char*) &tmp2)+1, 3);
  nr_plmn_info->trackingAreaCode->size = 3;
  nr_plmn_info->trackingAreaCode->bits_unused = 0;

  // connEstFailureControl
  // TODO: add connEstFailureControl

  //si-SchedulingInfo
  /*sib1->si_SchedulingInfo = CALLOC(1,sizeof(struct NR_SI_SchedulingInfo));
  asn_set_empty(&sib1->si_SchedulingInfo->schedulingInfoList.list);
  sib1->si_SchedulingInfo->si_WindowLength = NR_SI_SchedulingInfo__si_WindowLength_s40;
  struct NR_SchedulingInfo *schedulingInfo = CALLOC(1,sizeof(struct NR_SchedulingInfo));
  schedulingInfo->si_BroadcastStatus = NR_SchedulingInfo__si_BroadcastStatus_broadcasting;
  schedulingInfo->si_Periodicity = NR_SchedulingInfo__si_Periodicity_rf8;
  asn_set_empty(&schedulingInfo->sib_MappingInfo.list);

  NR_SIB_TypeInfo_t *sib_type3 = CALLOC(1,sizeof(e_NR_SIB_TypeInfo__type));
  sib_type3->type = NR_SIB_TypeInfo__type_sibType3;
  sib_type3->valueTag = CALLOC(1,sizeof(sib_type3->valueTag));
  ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo.list,sib_type3);

  NR_SIB_TypeInfo_t *sib_type5 = CALLOC(1,sizeof(e_NR_SIB_TypeInfo__type));
  sib_type5->type = NR_SIB_TypeInfo__type_sibType5;
  sib_type5->valueTag = CALLOC(1,sizeof(sib_type5->valueTag));
  ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo.list,sib_type5);

  NR_SIB_TypeInfo_t *sib_type4 = CALLOC(1,sizeof(e_NR_SIB_TypeInfo__type));
  sib_type4->type = NR_SIB_TypeInfo__type_sibType4;
  sib_type4->valueTag = CALLOC(1,sizeof(sib_type4->valueTag));
  ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo.list,sib_type4);

  NR_SIB_TypeInfo_t *sib_type2 = CALLOC(1,sizeof(e_NR_SIB_TypeInfo__type));
  sib_type2->type = NR_SIB_TypeInfo__type_sibType2;
  sib_type2->valueTag = CALLOC(1,sizeof(sib_type2->valueTag));
  ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo.list,sib_type2);

  ASN_SEQUENCE_ADD(&sib1->si_SchedulingInfo->schedulingInfoList.list,schedulingInfo);*/

  // servingCellConfigCommon
  asn1cCalloc(sib1->servingCellConfigCommon,  ServCellCom);
  NR_BWP_DownlinkCommon_t  *initialDownlinkBWP=&ServCellCom->downlinkConfigCommon.initialDownlinkBWP;
  initialDownlinkBWP->genericParameters=
    configuration->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;

  for(int i = 0; i< configuration->scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.count; i++) {
    asn1cSequenceAdd(ServCellCom->downlinkConfigCommon.frequencyInfoDL.frequencyBandList.list,
		     struct NR_NR_MultiBandInfo, nrMultiBandInfo);
    nrMultiBandInfo->freqBandIndicatorNR = configuration->scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[i];
  }

  int scs_scaling0 = 1<<(configuration->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing);
  int scs_scaling  = scs_scaling0;
  int scs_scaling2 = scs_scaling0;
  if (configuration->scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA < 600000) {
    scs_scaling = scs_scaling0*3;
  }
  if (configuration->scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA > 2016666) {
    scs_scaling = scs_scaling0>>2;
    scs_scaling2 = scs_scaling0>>2;
  }
  uint32_t absolute_diff = (*configuration->scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB -
                             configuration->scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);

  sib1->servingCellConfigCommon->downlinkConfigCommon.frequencyInfoDL.offsetToPointA = scs_scaling2 * (absolute_diff/(12*scs_scaling) - 10);

  LOG_I(NR_RRC,"SIB1 freq: absoluteFrequencySSB %ld, absoluteFrequencyPointA %ld\n",
                                    *configuration->scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,
                                    configuration->scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);
  LOG_I(NR_RRC,"SIB1 freq: absolute_diff %d, %d*(absolute_diff/(12*%d) - 10) %d\n",
        absolute_diff,scs_scaling2,scs_scaling,(int)sib1->servingCellConfigCommon->downlinkConfigCommon.frequencyInfoDL.offsetToPointA);

  for(int i = 0; i< configuration->scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.count; i++) {
    ASN_SEQUENCE_ADD(&ServCellCom->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list,
		     configuration->scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[i]);
  }

  initialDownlinkBWP->pdcch_ConfigCommon = 
      configuration->scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon;
  initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList = 
       CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon__commonSearchSpaceList));

  asn1cSequenceAdd(initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list,
		   NR_SearchSpace_t, ss1);
  ss1->searchSpaceId = 1;
  asn1cCallocOne(ss1->controlResourceSetId, 0);
  ss1->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(*ss1->monitoringSlotPeriodicityAndOffset));
  ss1->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
  ss1->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss1->monitoringSymbolsWithinSlot));
  ss1->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  // should be '1000 0000 0000 00'B (LSB first!), first symbol in slot, adjust if needed
  ss1->monitoringSymbolsWithinSlot->buf[1] = 0;
  ss1->monitoringSymbolsWithinSlot->buf[0] = 0x80;
  ss1->monitoringSymbolsWithinSlot->size = 2;
  ss1->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss1->nrofCandidates = calloc(1,sizeof(*ss1->nrofCandidates));
  ss1->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss1->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
  ss1->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n2;
  ss1->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
  ss1->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
  ss1->searchSpaceType = calloc(1,sizeof(*ss1->searchSpaceType));
  ss1->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
  ss1->searchSpaceType->choice.common=calloc(1,sizeof(*ss1->searchSpaceType->choice.common));
  ss1->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(*ss1->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));

  asn1cSequenceAdd(initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list,
		   NR_SearchSpace_t, ss5);
  ss5->searchSpaceId = 5;
  ss5->controlResourceSetId=calloc(1,sizeof(*ss5->controlResourceSetId));
  *ss5->controlResourceSetId=0;
  ss5->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(*ss5->monitoringSlotPeriodicityAndOffset));
  ss5->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5;
  ss5->monitoringSlotPeriodicityAndOffset->choice.sl5 = 0;
  ss5->duration = calloc(1,sizeof(*ss5->duration));
  *ss5->duration = 2;
  ss5->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss5->monitoringSymbolsWithinSlot));
  ss5->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  // should be '1100 0000 0000 00'B (LSB first!), first two symols in slot, adjust if needed
  ss5->monitoringSymbolsWithinSlot->buf[1] = 0;
  ss5->monitoringSymbolsWithinSlot->buf[0] = 0x80;
  ss5->monitoringSymbolsWithinSlot->size = 2;
  ss5->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss5->nrofCandidates = calloc(1,sizeof(*ss5->nrofCandidates));
  ss5->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss5->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
  ss5->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n4;
  ss5->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n2;
  ss5->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n1;
  ss5->searchSpaceType = calloc(1,sizeof(*ss5->searchSpaceType));
  ss5->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
  ss5->searchSpaceType->choice.common=calloc(1,sizeof(*ss5->searchSpaceType->choice.common));
  ss5->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(*ss5->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));

  asn1cSequenceAdd(initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list,
		   NR_SearchSpace_t, ss7);
  ss7->searchSpaceId = 7;
  ss7->controlResourceSetId=calloc(1,sizeof(*ss7->controlResourceSetId));
  *ss7->controlResourceSetId=0;
  ss7->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(*ss7->monitoringSlotPeriodicityAndOffset));
  ss7->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
  ss7->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss7->monitoringSymbolsWithinSlot));
  ss7->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  // should be '1100 0000 0000 00'B (LSB first!), first two symols in slot, adjust if needed
  ss7->monitoringSymbolsWithinSlot->buf[1] = 0;
  ss7->monitoringSymbolsWithinSlot->buf[0] = 0x80;
  ss7->monitoringSymbolsWithinSlot->size = 2;
  ss7->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss7->nrofCandidates = calloc(1,sizeof(*ss7->nrofCandidates));
  ss7->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss7->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
  ss7->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n4;
  ss7->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n2;
  ss7->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n1;
  ss7->searchSpaceType = calloc(1,sizeof(*ss7->searchSpaceType));
  ss7->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
  ss7->searchSpaceType->choice.common=calloc(1,sizeof(*ss7->searchSpaceType->choice.common));
  ss7->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(*ss7->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));

  asn1cCallocOne(initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1,  0);
  asn1cCallocOne(initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation, 7);
  asn1cCallocOne(initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->pagingSearchSpace, 5);
  asn1cCallocOne( initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace, 1);
   
  initialDownlinkBWP->pdsch_ConfigCommon = configuration->scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon;
  ServCellCom->downlinkConfigCommon.bcch_Config.modificationPeriodCoeff = NR_BCCH_Config__modificationPeriodCoeff_n2;
  ServCellCom->downlinkConfigCommon.pcch_Config.defaultPagingCycle = NR_PagingCycle_rf256;
  ServCellCom->downlinkConfigCommon.pcch_Config.nAndPagingFrameOffset.present = NR_PCCH_Config__nAndPagingFrameOffset_PR_quarterT;
  ServCellCom->downlinkConfigCommon.pcch_Config.nAndPagingFrameOffset.choice.quarterT = 1;
  ServCellCom->downlinkConfigCommon.pcch_Config.ns = NR_PCCH_Config__ns_one;

  asn1cCalloc(ServCellCom->downlinkConfigCommon.pcch_Config.firstPDCCH_MonitoringOccasionOfPO,
	      P0);
  P0->present = NR_PCCH_Config__firstPDCCH_MonitoringOccasionOfPO_PR_sCS120KHZoneT_SCS60KHZhalfT_SCS30KHZquarterT_SCS15KHZoneEighthT;

  asn1cCalloc(P0->choice.sCS120KHZoneT_SCS60KHZhalfT_SCS30KHZquarterT_SCS15KHZoneEighthT,
	      Z8);
  asn1cSequenceAdd(Z8->list,
		   long,
		   ZoneEight);
  asn1cCallocOne(ZoneEight, 0);

  asn1cCalloc(ServCellCom->uplinkConfigCommon, UL)
  asn_set_empty(&UL->frequencyInfoUL.scs_SpecificCarrierList.list);
  for(int i = 0; i< configuration->scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.count; i++) {
    ASN_SEQUENCE_ADD(&UL->frequencyInfoUL.scs_SpecificCarrierList.list,
		     configuration->scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[i]);
  }

  asn1cCallocOne(UL->frequencyInfoUL.p_Max, 23);

  UL->initialUplinkBWP.genericParameters = configuration->scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
  UL->initialUplinkBWP.rach_ConfigCommon = configuration->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon;
  UL->initialUplinkBWP.pusch_ConfigCommon = configuration->scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon;
  UL->initialUplinkBWP.pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = null;

  UL->initialUplinkBWP.pucch_ConfigCommon = configuration->scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon;

  UL->timeAlignmentTimerCommon = NR_TimeAlignmentTimer_infinity;

  ServCellCom->n_TimingAdvanceOffset = configuration->scc->n_TimingAdvanceOffset;

  ServCellCom->ssb_PositionsInBurst.inOneGroup.buf = calloc(1, sizeof(uint8_t));
  uint8_t bitmap8,temp_bitmap=0;
  switch (configuration->scc->ssb_PositionsInBurst->present) {
    case NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap:
      ServCellCom->ssb_PositionsInBurst.inOneGroup = configuration->scc->ssb_PositionsInBurst->choice.shortBitmap;
      break;
    case NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap:
      ServCellCom->ssb_PositionsInBurst.inOneGroup = configuration->scc->ssb_PositionsInBurst->choice.mediumBitmap;
      break;
    /*
    groupPresence: This field is present when maximum number of SS/PBCH blocks per half frame equals to 64 as defined in TS 38.213 [13], clause 4.1.
                   The first/leftmost bit corresponds to the SS/PBCH index 0-7, the second bit corresponds to SS/PBCH block 8-15, and so on.
                   Value 0 in the bitmap indicates that the SSBs according to inOneGroup are absent. Value 1 indicates that the SS/PBCH blocks are transmitted in accordance with inOneGroup.
    inOneGroup: When maximum number of SS/PBCH blocks per half frame equals to 64 as defined in TS 38.213 [13], clause 4.1, all 8 bit are valid;
                The first/ leftmost bit corresponds to the first SS/PBCH block index in the group (i.e., to SSB index 0, 8, and so on); the second bit corresponds to the second SS/PBCH block index in the group
                (i.e., to SSB index 1, 9, and so on), and so on. Value 0 in the bitmap indicates that the corresponding SS/PBCH block is not transmitted while value 1 indicates that the corresponding SS/PBCH block is transmitted.
    */
    case NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_longBitmap:
      ServCellCom->ssb_PositionsInBurst.inOneGroup.size = 1;
      ServCellCom->ssb_PositionsInBurst.inOneGroup.bits_unused = 0;
      ServCellCom->ssb_PositionsInBurst.groupPresence = calloc(1, sizeof(BIT_STRING_t));
      ServCellCom->ssb_PositionsInBurst.groupPresence->size = 1;
      ServCellCom->ssb_PositionsInBurst.groupPresence->bits_unused = 0;
      ServCellCom->ssb_PositionsInBurst.groupPresence->buf = calloc(1, sizeof(uint8_t));
      ServCellCom->ssb_PositionsInBurst.groupPresence->buf[0] = 0;
      for (int i=0; i<8; i++){
        bitmap8 = configuration->scc->ssb_PositionsInBurst->choice.longBitmap.buf[i];
        if (bitmap8!=0){
          if(temp_bitmap==0)
            temp_bitmap = bitmap8;
          else
            AssertFatal(temp_bitmap==bitmap8,"For longBitmap the groups of 8 SSBs containing at least 1 transmitted SSB should be all the same\n");

          ServCellCom->ssb_PositionsInBurst.inOneGroup.buf[0] = bitmap8;
          ServCellCom->ssb_PositionsInBurst.groupPresence->buf[0] |= 1<<(7-i);
        }
      }
      break;
    default:
      AssertFatal(false,"ssb_PositionsInBurst not present\n");
      break;
  }

  ServCellCom->ssb_PeriodicityServingCell = *configuration->scc->ssb_periodicityServingCell;
  if (configuration->scc->tdd_UL_DL_ConfigurationCommon) {
    ServCellCom->tdd_UL_DL_ConfigurationCommon = CALLOC(1,sizeof(struct NR_TDD_UL_DL_ConfigCommon));
    ServCellCom->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing = configuration->scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing;
    ServCellCom->tdd_UL_DL_ConfigurationCommon->pattern1 = configuration->scc->tdd_UL_DL_ConfigurationCommon->pattern1;
    ServCellCom->tdd_UL_DL_ConfigurationCommon->pattern2 = configuration->scc->tdd_UL_DL_ConfigurationCommon->pattern2;
  }
  ServCellCom->ss_PBCH_BlockPower = configuration->scc->ss_PBCH_BlockPower;

  // ims-EmergencySupport
  // TODO: add ims-EmergencySupport

  // eCallOverIMS-Support
  // TODO: add eCallOverIMS-Support

  // ue-TimersAndConstants
  sib1->ue_TimersAndConstants = CALLOC(1,sizeof(struct NR_UE_TimersAndConstants));
  sib1->ue_TimersAndConstants->t300 = NR_UE_TimersAndConstants__t300_ms400;
  sib1->ue_TimersAndConstants->t301 = NR_UE_TimersAndConstants__t301_ms400;
  sib1->ue_TimersAndConstants->t310 = NR_UE_TimersAndConstants__t310_ms2000;
  sib1->ue_TimersAndConstants->n310 = NR_UE_TimersAndConstants__n310_n10;
  sib1->ue_TimersAndConstants->t311 = NR_UE_TimersAndConstants__t311_ms3000;
  sib1->ue_TimersAndConstants->n311 = NR_UE_TimersAndConstants__n311_n1;
  sib1->ue_TimersAndConstants->t319 = NR_UE_TimersAndConstants__t319_ms400;

  // uac-BarringInfo
  /*sib1->uac_BarringInfo = CALLOC(1, sizeof(struct NR_SIB1__uac_BarringInfo));
  NR_UAC_BarringInfoSet_t *nr_uac_BarringInfoSet = CALLOC(1, sizeof(NR_UAC_BarringInfoSet_t));
  asn_set_empty(&sib1->uac_BarringInfo->uac_BarringInfoSetList);
  nr_uac_BarringInfoSet->uac_BarringFactor = NR_UAC_BarringInfoSet__uac_BarringFactor_p95;
  nr_uac_BarringInfoSet->uac_BarringTime = NR_UAC_BarringInfoSet__uac_BarringTime_s4;
  nr_uac_BarringInfoSet->uac_BarringForAccessIdentity.buf = CALLOC(1, 1);
  nr_uac_BarringInfoSet->uac_BarringForAccessIdentity.size = 1;
  nr_uac_BarringInfoSet->uac_BarringForAccessIdentity.bits_unused = 1;
  ASN_SEQUENCE_ADD(&sib1->uac_BarringInfo->uac_BarringInfoSetList, nr_uac_BarringInfoSet);*/

  // useFullResumeID
  // TODO: add useFullResumeID

  // lateNonCriticalExtension
  // TODO: add lateNonCriticalExtension

  // nonCriticalExtension
  // TODO: add nonCriticalExtension

  xer_fprint(stdout, &asn_DEF_NR_SIB1, (const void*)sib1_message->message.choice.c1->choice.systemInformationBlockType1);

  if(carrier->SIB1 == NULL) carrier->SIB1=(uint8_t *) malloc16(NR_MAX_SIB_LENGTH/8);
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)sib1_message,
                                   carrier->SIB1,
                                   NR_MAX_SIB_LENGTH/8);
  AssertFatal(enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  AssertFatal(enc_rval.encoded <= NR_MAX_SIB_LENGTH, "ASN1 encoded length %zd bits. 3GPP TS 38.331 section 5.2.1 - The physical layer imposes a limit to the maximum size a SIB can take. The maximum SIB1 or SI message size is 2976 bits.\n", enc_rval.encoded);

  return((enc_rval.encoded+7)/8);
}

uint8_t do_SIB23_NR(rrc_gNB_carrier_data_t *carrier,
                    gNB_RrcConfigurationReq *configuration) {
  asn_enc_rval_t enc_rval;
  SystemInformation_IEs__sib_TypeAndInfo__Member *sib2 = NULL;
  SystemInformation_IEs__sib_TypeAndInfo__Member *sib3 = NULL;

  NR_BCCH_DL_SCH_Message_t *sib_message = CALLOC(1,sizeof(NR_BCCH_DL_SCH_Message_t));
  sib_message->message.present = NR_BCCH_DL_SCH_MessageType_PR_c1;
  sib_message->message.choice.c1 = CALLOC(1,sizeof(struct NR_BCCH_DL_SCH_MessageType__c1));
  sib_message->message.choice.c1->present = NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformation;
  sib_message->message.choice.c1->choice.systemInformation = CALLOC(1,sizeof(struct NR_SystemInformation));
  
  struct NR_SystemInformation *sib = sib_message->message.choice.c1->choice.systemInformation;
  sib->criticalExtensions.present = NR_SystemInformation__criticalExtensions_PR_systemInformation;
  sib->criticalExtensions.choice.systemInformation = CALLOC(1, sizeof(struct NR_SystemInformation_IEs));

  struct NR_SystemInformation_IEs *ies = sib->criticalExtensions.choice.systemInformation;
  sib2 = CALLOC(1, sizeof(SystemInformation_IEs__sib_TypeAndInfo__Member));
  sib2->present = NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2;
  sib2->choice.sib2 = CALLOC(1, sizeof(struct NR_SIB2));
  sib2->choice.sib2->cellReselectionInfoCommon.q_Hyst = NR_SIB2__cellReselectionInfoCommon__q_Hyst_dB1;
  sib2->choice.sib2->cellReselectionServingFreqInfo.threshServingLowP = 2; // INTEGER (0..31)
  sib2->choice.sib2->cellReselectionServingFreqInfo.cellReselectionPriority =  2; // INTEGER (0..7)
  sib2->choice.sib2->intraFreqCellReselectionInfo.q_RxLevMin = -50; // INTEGER (-70..-22)
  sib2->choice.sib2->intraFreqCellReselectionInfo.s_IntraSearchP = 2; // INTEGER (0..31)
  sib2->choice.sib2->intraFreqCellReselectionInfo.t_ReselectionNR = 2; // INTEGER (0..7)
  sib2->choice.sib2->intraFreqCellReselectionInfo.deriveSSB_IndexFromCell = true;
  ASN_SEQUENCE_ADD(&ies->sib_TypeAndInfo.list, sib2);

  sib3 = CALLOC(1, sizeof(SystemInformation_IEs__sib_TypeAndInfo__Member));
  sib3->present = NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib3;
  sib3->choice.sib3 = CALLOC(1, sizeof(struct NR_SIB3));
  ASN_SEQUENCE_ADD(&ies->sib_TypeAndInfo.list, sib3);

  //encode SIB to data
  // carrier->SIB23 = (uint8_t *) malloc16(128);
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_DL_SCH_Message,
                                   NULL,
                                   (void *)sib_message,
                                   carrier->SIB23,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);

  if (enc_rval.encoded==-1) {
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}

void  do_RLC_BEARER(uint8_t Mod_id,
                    int CC_id,
                    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_BearerToAddModList,
                    rlc_bearer_config_t  *rlc_config) {
  struct NR_RLC_BearerConfig *rlc_bearer;
  rlc_bearer = CALLOC(1,sizeof(struct NR_RLC_BearerConfig));
  rlc_bearer->logicalChannelIdentity = rlc_config->LogicalChannelIdentity[CC_id];
  rlc_bearer->servedRadioBearer = CALLOC(1,sizeof(struct NR_RLC_BearerConfig__servedRadioBearer));
  rlc_bearer->servedRadioBearer->present = rlc_config->servedRadioBearer_present[CC_id];

  if(rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity) {
    rlc_bearer->servedRadioBearer->choice.srb_Identity = rlc_config->srb_Identity[CC_id];
  } else if(rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity) {
    rlc_bearer->servedRadioBearer->choice.drb_Identity = rlc_config->drb_Identity[CC_id];
  }

  rlc_bearer->reestablishRLC = CALLOC(1,sizeof(long));
  *(rlc_bearer->reestablishRLC) = rlc_config->reestablishRLC[CC_id];
  rlc_bearer->rlc_Config = CALLOC(1,sizeof(struct NR_RLC_Config));
  rlc_bearer->rlc_Config->present = rlc_config->rlc_Config_present[CC_id];

  if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_am) {
    rlc_bearer->rlc_Config->choice.am = CALLOC(1,sizeof(struct NR_RLC_Config__am));
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength     = CALLOC(1,sizeof(NR_SN_FieldLengthAM_t));
    *(rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength)  = rlc_config->ul_AM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit   = rlc_config->t_PollRetransmit[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.pollPDU            = rlc_config->pollPDU[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.pollByte           = rlc_config->pollByte[CC_id];
    rlc_bearer->rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold   = rlc_config->maxRetxThreshold[CC_id];
    rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength     = CALLOC(1,sizeof(NR_SN_FieldLengthAM_t));
    *(rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength)  = rlc_config->dl_AM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.t_Reassembly       = rlc_config->dl_AM_t_Reassembly[CC_id];
    rlc_bearer->rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit   = rlc_config->t_StatusProhibit[CC_id];
  } else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Bi_Directional) {
    rlc_bearer->rlc_Config->choice.um_Bi_Directional = CALLOC(1,sizeof(struct NR_RLC_Config__um_Bi_Directional));
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength) = rlc_config->ul_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength) = rlc_config->dl_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.t_Reassembly   = rlc_config->dl_UM_t_Reassembly[CC_id];
  } else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Uni_Directional_UL) {
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL = CALLOC(1,sizeof(struct NR_RLC_Config__um_Uni_Directional_UL));
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL->ul_UM_RLC.sn_FieldLength    = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Uni_Directional_UL->ul_UM_RLC.sn_FieldLength) = rlc_config->ul_UM_sn_FieldLength[CC_id];
  } else if(rlc_bearer->rlc_Config->present == NR_RLC_Config_PR_um_Uni_Directional_DL) {
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL = CALLOC(1,sizeof(struct NR_RLC_Config__um_Uni_Directional_DL));
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL->dl_UM_RLC.sn_FieldLength    = CALLOC(1,sizeof(NR_SN_FieldLengthUM_t));
    *(rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL->dl_UM_RLC.sn_FieldLength) = rlc_config->dl_UM_sn_FieldLength[CC_id];
    rlc_bearer->rlc_Config->choice.um_Uni_Directional_DL->dl_UM_RLC.t_Reassembly      = rlc_config->dl_UM_t_Reassembly[CC_id];
  }

  rlc_bearer->mac_LogicalChannelConfig = CALLOC(1,sizeof(struct NR_LogicalChannelConfig));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters = CALLOC(1,sizeof(struct NR_LogicalChannelConfig__ul_SpecificParameters));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->priority            = rlc_config->priority[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->prioritisedBitRate  = rlc_config->prioritisedBitRate[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->bucketSizeDuration  = rlc_config->bucketSizeDuration[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedServingCells = CALLOC(1,sizeof(struct NR_LogicalChannelConfig__ul_SpecificParameters__allowedServingCells));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedSCS_List     = CALLOC(1,sizeof(struct NR_LogicalChannelConfig__ul_SpecificParameters__allowedSCS_List));
  NR_ServCellIndex_t *servingcellindex;
  servingcellindex = CALLOC(1,sizeof(NR_ServCellIndex_t));
  *servingcellindex = rlc_config->allowedServingCells[CC_id];
  ASN_SEQUENCE_ADD(&(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedServingCells->list),&servingcellindex);
  NR_SubcarrierSpacing_t *subcarrierspacing;
  subcarrierspacing = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  *subcarrierspacing = rlc_config->subcarrierspacing[CC_id];
  ASN_SEQUENCE_ADD(&(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->allowedSCS_List->list),&subcarrierspacing);
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->maxPUSCH_Duration           = CALLOC(1,sizeof(long));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->configuredGrantType1Allowed = CALLOC(1,sizeof(long));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup         = CALLOC(1,sizeof(long));
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID         = CALLOC(1,sizeof(NR_SchedulingRequestId_t));
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->maxPUSCH_Duration)           = rlc_config->maxPUSCH_Duration[CC_id];
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->configuredGrantType1Allowed) = rlc_config->configuredGrantType1Allowed[CC_id];
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup)         = rlc_config->logicalChannelGroup[CC_id];
  *(rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID)         = rlc_config->schedulingRequestID[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_Mask               = rlc_config->logicalChannelSR_Mask[CC_id];
  rlc_bearer->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_DelayTimerApplied  = rlc_config->logicalChannelSR_DelayTimerApplied[CC_id];
  ASN_SEQUENCE_ADD(&(rlc_BearerToAddModList->list),&rlc_bearer);
}


void do_MAC_CELLGROUP(uint8_t Mod_id,
                      int CC_id,
                      NR_MAC_CellGroupConfig_t *mac_CellGroupConfig,
                      mac_cellgroup_t  *mac_cellgroup_config) {
  mac_CellGroupConfig->drx_Config               = CALLOC(1,sizeof(struct NR_SetupRelease_DRX_Config));
  mac_CellGroupConfig->schedulingRequestConfig  = CALLOC(1,sizeof(struct NR_SchedulingRequestConfig));
  mac_CellGroupConfig->bsr_Config               = CALLOC(1,sizeof(struct NR_BSR_Config));
  mac_CellGroupConfig->tag_Config               = CALLOC(1,sizeof(struct NR_TAG_Config));
  mac_CellGroupConfig->phr_Config               = CALLOC(1,sizeof(struct NR_SetupRelease_PHR_Config));
  mac_CellGroupConfig->drx_Config->present      = mac_cellgroup_config->DRX_Config_PR[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup = CALLOC(1,sizeof(struct NR_DRX_Config));
  mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present = mac_cellgroup_config->drx_onDurationTimer_PR[CC_id];

  if(mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present == NR_DRX_Config__drx_onDurationTimer_PR_subMilliSeconds) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.choice.subMilliSeconds = mac_cellgroup_config->subMilliSeconds[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.present == NR_DRX_Config__drx_onDurationTimer_PR_milliSeconds) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_onDurationTimer.choice.milliSeconds    = mac_cellgroup_config->milliSeconds[CC_id];
  }

  mac_CellGroupConfig->drx_Config->choice.setup->drx_InactivityTimer        = mac_cellgroup_config->drx_InactivityTimer[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_HARQ_RTT_TimerDL       = mac_cellgroup_config->drx_HARQ_RTT_TimerDL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_HARQ_RTT_TimerUL       = mac_cellgroup_config->drx_HARQ_RTT_TimerUL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_RetransmissionTimerDL  = mac_cellgroup_config->drx_RetransmissionTimerDL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_RetransmissionTimerUL  = mac_cellgroup_config->drx_RetransmissionTimerUL[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present = mac_cellgroup_config->drx_LongCycleStartOffset_PR[CC_id];

  if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms10) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms10 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms20) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms20 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms32) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms32 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms40) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms40 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms60) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms60 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms64) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms64 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms70) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms70 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms80) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms80 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms128) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms128 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms160) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms160 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms256) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms256 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms320) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms320 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms512) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms512 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms640) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms640 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms1024) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms1024 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms1280) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms1280 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms2048) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms2048 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms2560) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms2560 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms5120) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms5120 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  } else if(mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.present == NR_DRX_Config__drx_LongCycleStartOffset_PR_ms10240) {
    mac_CellGroupConfig->drx_Config->choice.setup->drx_LongCycleStartOffset.choice.ms10240 = mac_cellgroup_config->drx_LongCycleStartOffset[CC_id];
  }

  mac_CellGroupConfig->drx_Config->choice.setup->shortDRX = CALLOC(1,sizeof(struct NR_DRX_Config__shortDRX));
  mac_CellGroupConfig->drx_Config->choice.setup->shortDRX->drx_ShortCycle       = mac_cellgroup_config->drx_ShortCycle[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->shortDRX->drx_ShortCycleTimer  = mac_cellgroup_config->drx_ShortCycleTimer[CC_id];
  mac_CellGroupConfig->drx_Config->choice.setup->drx_SlotOffset                 = mac_cellgroup_config->drx_SlotOffset[CC_id];
  mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList = CALLOC(1,sizeof(struct NR_SchedulingRequestConfig__schedulingRequestToAddModList));
  struct NR_SchedulingRequestToAddMod *schedulingrequestlist;
  schedulingrequestlist = CALLOC(1,sizeof(struct NR_SchedulingRequestToAddMod));
  schedulingrequestlist->schedulingRequestId  = mac_cellgroup_config->schedulingRequestId[CC_id];
  schedulingrequestlist->sr_ProhibitTimer = CALLOC(1,sizeof(long));
  *(schedulingrequestlist->sr_ProhibitTimer) = mac_cellgroup_config->sr_ProhibitTimer[CC_id];
  schedulingrequestlist->sr_TransMax      = mac_cellgroup_config->sr_TransMax[CC_id];
  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList->list),&schedulingrequestlist);
  mac_CellGroupConfig->bsr_Config->periodicBSR_Timer              = mac_cellgroup_config->periodicBSR_Timer[CC_id];
  mac_CellGroupConfig->bsr_Config->retxBSR_Timer                  = mac_cellgroup_config->retxBSR_Timer[CC_id];
  mac_CellGroupConfig->bsr_Config->logicalChannelSR_DelayTimer    = CALLOC(1,sizeof(long));
  *(mac_CellGroupConfig->bsr_Config->logicalChannelSR_DelayTimer)    = mac_cellgroup_config->logicalChannelSR_DelayTimer[CC_id];
  mac_CellGroupConfig->tag_Config->tag_ToAddModList = CALLOC(1,sizeof(struct NR_TAG_Config__tag_ToAddModList));
  struct NR_TAG *tag;
  tag = CALLOC(1,sizeof(struct NR_TAG));
  tag->tag_Id             = mac_cellgroup_config->tag_Id[CC_id];
  tag->timeAlignmentTimer = mac_cellgroup_config->timeAlignmentTimer[CC_id];
  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->tag_Config->tag_ToAddModList->list),&tag);
  mac_CellGroupConfig->phr_Config->present = mac_cellgroup_config->PHR_Config_PR[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup   = CALLOC(1,sizeof(struct NR_PHR_Config));
  mac_CellGroupConfig->phr_Config->choice.setup->phr_PeriodicTimer         = mac_cellgroup_config->phr_PeriodicTimer[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_ProhibitTimer         = mac_cellgroup_config->phr_ProhibitTimer[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Tx_PowerFactorChange  = mac_cellgroup_config->phr_Tx_PowerFactorChange[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->multiplePHR               = mac_cellgroup_config->multiplePHR[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->dummy                     = mac_cellgroup_config->phr_Type2SpCell[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Type2OtherCell        = mac_cellgroup_config->phr_Type2OtherCell[CC_id];
  mac_CellGroupConfig->phr_Config->choice.setup->phr_ModeOtherCG           = mac_cellgroup_config->phr_ModeOtherCG[CC_id];
  mac_CellGroupConfig->skipUplinkTxDynamic      = mac_cellgroup_config->skipUplinkTxDynamic[CC_id];
}


void  do_PHYSICALCELLGROUP(uint8_t Mod_id,
                           int CC_id,
                           NR_PhysicalCellGroupConfig_t *physicalCellGroupConfig,
                           physicalcellgroup_t *physicalcellgroup_config) {
  physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH = CALLOC(1,sizeof(long));
  physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH = CALLOC(1,sizeof(long));
  physicalCellGroupConfig->p_NR_FR1                      = CALLOC(1,sizeof(NR_P_Max_t));
  physicalCellGroupConfig->tpc_SRS_RNTI                  = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->tpc_PUCCH_RNTI                = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->tpc_PUSCH_RNTI                = CALLOC(1,sizeof(NR_RNTI_Value_t));
  physicalCellGroupConfig->sp_CSI_RNTI                   = CALLOC(1,sizeof(NR_RNTI_Value_t));
  *(physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH) = physicalcellgroup_config->harq_ACK_SpatialBundlingPUCCH[CC_id];
  *(physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH) = physicalcellgroup_config->harq_ACK_SpatialBundlingPUSCH[CC_id];
  *(physicalCellGroupConfig->p_NR_FR1)                      = physicalcellgroup_config->p_NR[CC_id];
  physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook          = physicalcellgroup_config->pdsch_HARQ_ACK_Codebook[CC_id];
  *(physicalCellGroupConfig->tpc_SRS_RNTI)                  = physicalcellgroup_config->tpc_SRS_RNTI[CC_id];
  *(physicalCellGroupConfig->tpc_PUCCH_RNTI)                = physicalcellgroup_config->tpc_PUCCH_RNTI[CC_id];
  *(physicalCellGroupConfig->tpc_PUSCH_RNTI)                = physicalcellgroup_config->tpc_PUSCH_RNTI[CC_id];
  *(physicalCellGroupConfig->sp_CSI_RNTI)                   = physicalcellgroup_config->sp_CSI_RNTI[CC_id];
  physicalCellGroupConfig->cs_RNTI                       = CALLOC(1,sizeof(struct NR_SetupRelease_RNTI_Value));
  physicalCellGroupConfig->cs_RNTI->present              = physicalcellgroup_config->RNTI_Value_PR[CC_id];

  if(physicalCellGroupConfig->cs_RNTI->present == NR_SetupRelease_RNTI_Value_PR_setup) {
    physicalCellGroupConfig->cs_RNTI->choice.setup = physicalcellgroup_config->RNTI_Value[CC_id];
  }
}



void do_SpCellConfig(gNB_RRC_INST *rrc,
                      struct NR_SpCellConfig  *spconfig){
  //gNB_RrcConfigurationReq  *common_configuration;
  //common_configuration = CALLOC(1,sizeof(gNB_RrcConfigurationReq));
  //Fill servingcellconfigcommon config value
  //Fill common config to structure
  //  rrc->configuration = common_configuration;
  spconfig->reconfigurationWithSync = CALLOC(1,sizeof(struct NR_ReconfigurationWithSync));
}

//------------------------------------------------------------------------------
uint8_t do_RRCReject(uint8_t Mod_id,
                  uint8_t *const buffer)
//------------------------------------------------------------------------------
{
    asn_enc_rval_t                                   enc_rval;;
    NR_DL_CCCH_Message_t                             dl_ccch_msg;
    NR_RRCReject_t                                   *rrcReject;
    NR_RejectWaitTime_t                              waitTime = 1;

    memset((void *)&dl_ccch_msg, 0, sizeof(NR_DL_CCCH_Message_t));
    dl_ccch_msg.message.present = NR_DL_CCCH_MessageType_PR_c1;
    dl_ccch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_DL_CCCH_MessageType__c1));
    dl_ccch_msg.message.choice.c1->present = NR_RRCReject__criticalExtensions_PR_rrcReject;

    dl_ccch_msg.message.choice.c1->choice.rrcReject = CALLOC(1,sizeof(NR_RRCReject_t));
    rrcReject = dl_ccch_msg.message.choice.c1->choice.rrcReject;

    rrcReject->criticalExtensions.choice.rrcReject           = CALLOC(1, sizeof(struct NR_RRCReject_IEs));
    rrcReject->criticalExtensions.choice.rrcReject->waitTime = CALLOC(1, sizeof(NR_RejectWaitTime_t));

    rrcReject->criticalExtensions.present = NR_RRCReject__criticalExtensions_PR_rrcReject;
    rrcReject->criticalExtensions.choice.rrcReject->waitTime = &waitTime;

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
        xer_fprint(stdout, &asn_DEF_NR_DL_CCCH_Message, (void *)&dl_ccch_msg);
    }

    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_CCCH_Message,
                                    NULL,
                                    (void *)&dl_ccch_msg,
                                    buffer,
                                    100);

    if(enc_rval.encoded == -1) {
        LOG_E(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
            enc_rval.failed_type->name, enc_rval.encoded);
        return -1;
    }

    LOG_D(NR_RRC,"RRCReject Encoded %zd bits (%zd bytes)\n",
            enc_rval.encoded,(enc_rval.encoded+7)/8);
    return((enc_rval.encoded+7)/8);
}

void fill_initial_SpCellConfig(int uid,
                               NR_SpCellConfig_t *SpCellConfig,
                               const NR_ServingCellConfigCommon_t *scc,
                               const NR_ServingCellConfig_t *servingcellconfigdedicated,
                               const gNB_RrcConfigurationReq *configuration) {

  // This assert will never happen in the current implementation because NUMBER_OF_UE_MAX = 4.
  // However, if in the future NUMBER_OF_UE_MAX is increased, it will be necessary to improve the allocation of SRS resources,
  // where the startPosition = 2 or 3 and sl160 = 17, 17, 27 ... 157 only give us 30 different allocations.
  AssertFatal(uid>=0 && uid<30, "gNB cannot allocate the SRS resources\n");

  int curr_bwp = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
  SpCellConfig->servCellIndex = NULL;
  SpCellConfig->reconfigurationWithSync = NULL;
  SpCellConfig->rlmInSyncOutOfSyncThreshold = NULL;
  SpCellConfig->rlf_TimersAndConstants = NULL;

  SpCellConfig->spCellConfigDedicated = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated));
  SpCellConfig->spCellConfigDedicated->uplinkConfig = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->uplinkConfig));

  NR_BWP_UplinkDedicated_t *initialUplinkBWP = calloc(1,sizeof(*initialUplinkBWP));
  SpCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP = initialUplinkBWP;
  initialUplinkBWP->pucch_Config = calloc(1,sizeof(*initialUplinkBWP->pucch_Config));
  initialUplinkBWP->pucch_Config->present = NR_SetupRelease_PUCCH_Config_PR_setup;
  NR_PUCCH_Config_t *pucch_Config = calloc(1,sizeof(*pucch_Config));
  initialUplinkBWP->pucch_Config->choice.setup=pucch_Config;
  pucch_Config->resourceSetToAddModList = calloc(1,sizeof(*pucch_Config->resourceSetToAddModList));
  pucch_Config->resourceSetToReleaseList = NULL;
  pucch_Config->resourceToAddModList = calloc(1,sizeof(*pucch_Config->resourceToAddModList));
  pucch_Config->resourceToReleaseList = NULL;
  config_pucch_resset0(pucch_Config, uid, curr_bwp, NULL);
  config_pucch_resset1(pucch_Config, NULL);
  set_pucch_power_config(pucch_Config, configuration->do_CSIRS);

  initialUplinkBWP->pusch_Config = calloc(1,sizeof(*initialUplinkBWP->pusch_Config));
  initialUplinkBWP->pusch_Config->present = NR_SetupRelease_PUSCH_Config_PR_setup;
  NR_PUSCH_Config_t *pusch_Config = calloc(1,sizeof(*pusch_Config));
  initialUplinkBWP->pusch_Config->choice.setup = pusch_Config;
  pusch_Config->dataScramblingIdentityPUSCH = NULL;
  pusch_Config->txConfig=calloc(1,sizeof(*pusch_Config->txConfig));
  *pusch_Config->txConfig= NR_PUSCH_Config__txConfig_codebook;
  pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA = NULL;
  pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB = calloc(1,sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB));
  pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->present = NR_SetupRelease_DMRS_UplinkConfig_PR_setup;
  pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup = calloc(1,sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup));
  NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup;
  NR_DMRS_UplinkConfig->dmrs_Type = NULL;
  NR_DMRS_UplinkConfig->dmrs_AdditionalPosition = NULL; /*calloc(1,sizeof(*NR_DMRS_UplinkConfig->dmrs_AdditionalPosition));
  *NR_DMRS_UplinkConfig->dmrs_AdditionalPosition = NR_DMRS_UplinkConfig__dmrs_AdditionalPosition_pos0;*/
  NR_DMRS_UplinkConfig->phaseTrackingRS=NULL;
  NR_DMRS_UplinkConfig->maxLength=NULL;
  NR_DMRS_UplinkConfig->transformPrecodingDisabled = calloc(1,sizeof(*NR_DMRS_UplinkConfig->transformPrecodingDisabled));
  NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID0 = NULL;
  NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID1 = NULL;
  NR_DMRS_UplinkConfig->transformPrecodingEnabled = NULL;
  pusch_Config->pusch_PowerControl = calloc(1,sizeof(*pusch_Config->pusch_PowerControl));
  pusch_Config->pusch_PowerControl->tpc_Accumulation = NULL;
  pusch_Config->pusch_PowerControl->msg3_Alpha = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->msg3_Alpha));
  *pusch_Config->pusch_PowerControl->msg3_Alpha = NR_Alpha_alpha1;
  pusch_Config->pusch_PowerControl->p0_NominalWithoutGrant = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->p0_NominalWithoutGrant));
  *pusch_Config->pusch_PowerControl->p0_NominalWithoutGrant = -76;
  pusch_Config->pusch_PowerControl->p0_AlphaSets = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->p0_AlphaSets));
  NR_P0_PUSCH_AlphaSet_t *aset = calloc(1,sizeof(*aset));
  aset->p0_PUSCH_AlphaSetId=0;
  aset->p0=calloc(1,sizeof(*aset->p0));
  *aset->p0 = 0;
  aset->alpha=calloc(1,sizeof(*aset->alpha));
  *aset->alpha=NR_Alpha_alpha1;
  ASN_SEQUENCE_ADD(&pusch_Config->pusch_PowerControl->p0_AlphaSets->list,aset);
  pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList));
  NR_PUSCH_PathlossReferenceRS_t *plrefRS = calloc(1,sizeof(*plrefRS));
  plrefRS->pusch_PathlossReferenceRS_Id=0;
  plrefRS->referenceSignal.present = NR_PUSCH_PathlossReferenceRS__referenceSignal_PR_ssb_Index;
  plrefRS->referenceSignal.choice.ssb_Index = 0;
  ASN_SEQUENCE_ADD(&pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList->list,plrefRS);
  pusch_Config->pusch_PowerControl->pathlossReferenceRSToReleaseList = NULL;
  pusch_Config->pusch_PowerControl->twoPUSCH_PC_AdjustmentStates = NULL;
  pusch_Config->pusch_PowerControl->deltaMCS = calloc(1, sizeof(*pusch_Config->pusch_PowerControl->deltaMCS));
  *pusch_Config->pusch_PowerControl->deltaMCS = NR_PUSCH_PowerControl__deltaMCS_enabled;
  pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToAddModList = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToAddModList));
  NR_SRI_PUSCH_PowerControl_t *sriPUSCHPC=calloc(1,sizeof(*sriPUSCHPC));
  sriPUSCHPC->sri_PUSCH_PowerControlId=0;
  sriPUSCHPC->sri_PUSCH_PathlossReferenceRS_Id=0;
  sriPUSCHPC->sri_P0_PUSCH_AlphaSetId=0;
  sriPUSCHPC->sri_PUSCH_ClosedLoopIndex=NR_SRI_PUSCH_PowerControl__sri_PUSCH_ClosedLoopIndex_i0;
  ASN_SEQUENCE_ADD(&pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToAddModList->list,sriPUSCHPC);
  pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToReleaseList = NULL;
  pusch_Config->frequencyHopping=NULL;
  pusch_Config->frequencyHoppingOffsetLists=NULL;
  pusch_Config->resourceAllocation = NR_PUSCH_Config__resourceAllocation_resourceAllocationType1;
  pusch_Config->pusch_TimeDomainAllocationList = NULL;
  pusch_Config->pusch_AggregationFactor=NULL;
  pusch_Config->mcs_Table=NULL;
  pusch_Config->mcs_TableTransformPrecoder=NULL;
  pusch_Config->transformPrecoder= NULL;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL) {
    pusch_Config->transformPrecoder=calloc(1,sizeof(*pusch_Config->transformPrecoder));
    *pusch_Config->transformPrecoder = NR_PUSCH_Config__transformPrecoder_disabled;
  }
  pusch_Config->codebookSubset=calloc(1,sizeof(*pusch_Config->codebookSubset));
  *pusch_Config->codebookSubset = NR_PUSCH_Config__codebookSubset_nonCoherent;
  pusch_Config->maxRank=calloc(1,sizeof(*pusch_Config->maxRank));
  *pusch_Config->maxRank= 1;
  pusch_Config->rbg_Size=NULL;
  pusch_Config->uci_OnPUSCH=NULL;
  pusch_Config->tp_pi2BPSK=NULL;

  // We are using do_srs = 0 here because the periodic SRS will only be enabled in update_cellGroupConfig() if do_srs == 1
  initialUplinkBWP->srs_Config = calloc(1,sizeof(*initialUplinkBWP->srs_Config));
  config_srs(initialUplinkBWP->srs_Config, NULL, curr_bwp, uid, 0, 0);

  scheduling_request_config(scc, pucch_Config);

  set_dl_DataToUL_ACK(pucch_Config, configuration->minRXTXTIME);

  SpCellConfig->spCellConfigDedicated->initialDownlinkBWP = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->initialDownlinkBWP));
  NR_BWP_DownlinkDedicated_t *bwp_Dedicated = SpCellConfig->spCellConfigDedicated->initialDownlinkBWP;
  bwp_Dedicated->pdcch_Config=calloc(1,sizeof(*bwp_Dedicated->pdcch_Config));
  bwp_Dedicated->pdcch_Config->present = NR_SetupRelease_PDCCH_Config_PR_setup;
  bwp_Dedicated->pdcch_Config->choice.setup = calloc(1,sizeof(*bwp_Dedicated->pdcch_Config->choice.setup));

  bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList = calloc(1,sizeof(*bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList));

  bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList = calloc(1,sizeof(*bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList));

  NR_ControlResourceSet_t *coreset = calloc(1,sizeof(*coreset));

  uint64_t bitmap = get_ssb_bitmap(scc);
  rrc_coreset_config(coreset, 0, curr_bwp, bitmap);

  ASN_SEQUENCE_ADD(&bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list,
                   coreset);

  bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList = calloc(1,sizeof(*bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList));
  
  NR_SearchSpace_t *ss2 = calloc(1,sizeof(*ss2));
 
  ss2->searchSpaceId=2;
  ss2->controlResourceSetId=calloc(1,sizeof(*ss2->controlResourceSetId));
  *ss2->controlResourceSetId=coreset->controlResourceSetId;
  ss2->monitoringSlotPeriodicityAndOffset=calloc(1,sizeof(*ss2->monitoringSlotPeriodicityAndOffset));
  ss2->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
  ss2->monitoringSlotPeriodicityAndOffset->choice.sl1=(NULL_t)0;
  ss2->duration=NULL;
  ss2->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss2->monitoringSymbolsWithinSlot));
  ss2->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  ss2->monitoringSymbolsWithinSlot->size = 2;
  ss2->monitoringSymbolsWithinSlot->buf[0]=0x80;
  ss2->monitoringSymbolsWithinSlot->buf[1]=0x0;
  ss2->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss2->nrofCandidates=calloc(1,sizeof(*ss2->nrofCandidates));
  ss2->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss2->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n2;
  ss2->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n1;
  ss2->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
  ss2->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
  ss2->searchSpaceType=calloc(1,sizeof(*ss2->searchSpaceType));
  ss2->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  ss2->searchSpaceType->choice.ue_Specific = calloc(1,sizeof(*ss2->searchSpaceType->choice.ue_Specific));
  ss2->searchSpaceType->choice.ue_Specific->dci_Formats=NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_1_And_1_1;
  
  ASN_SEQUENCE_ADD(&bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list,
                   ss2);
  bwp_Dedicated->pdsch_Config=calloc(1,sizeof(*bwp_Dedicated->pdsch_Config));
  bwp_Dedicated->pdsch_Config->present = NR_SetupRelease_PDSCH_Config_PR_setup;
  bwp_Dedicated->pdsch_Config->choice.setup = calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup));
  bwp_Dedicated->pdsch_Config->choice.setup->dataScramblingIdentityPDSCH = NULL;
  bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA));
  bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
  bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup));

  bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type=NULL;
  bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->maxLength=NULL;

  bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition));
  *bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1;
  bwp_Dedicated->pdsch_Config->choice.setup->resourceAllocation = NR_PDSCH_Config__resourceAllocation_resourceAllocationType1;
  bwp_Dedicated->pdsch_Config->choice.setup->prb_BundlingType.present = NR_PDSCH_Config__prb_BundlingType_PR_staticBundling;
  bwp_Dedicated->pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling = calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling));
  bwp_Dedicated->pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling->bundleSize =
      calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling->bundleSize));
  *bwp_Dedicated->pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling->bundleSize = NR_PDSCH_Config__prb_BundlingType__staticBundling__bundleSize_wideband;

  bwp_Dedicated->pdsch_Config->choice.setup->tci_StatesToAddModList=calloc(1,sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->tci_StatesToAddModList));
  NR_TCI_State_t *tcic;

  tcic=calloc(1,sizeof(*tcic));
  tcic->tci_StateId=0;
  tcic->qcl_Type1.cell=NULL;
  tcic->qcl_Type1.bwp_Id=NULL;
  tcic->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
  tcic->qcl_Type1.referenceSignal.choice.ssb = 0;
  tcic->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeD;

  ASN_SEQUENCE_ADD(&bwp_Dedicated->pdsch_Config->choice.setup->tci_StatesToAddModList->list,tcic);

  SpCellConfig->spCellConfigDedicated->tag_Id=0;
  SpCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig=calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig));
  NR_PDSCH_ServingCellConfig_t *pdsch_servingcellconfig = calloc(1,sizeof(*pdsch_servingcellconfig));
  SpCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->present = NR_SetupRelease_PDSCH_ServingCellConfig_PR_setup;
  SpCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup = pdsch_servingcellconfig;

  // Downlink BWPs
  int n_dl_bwp = 0;
  if (servingcellconfigdedicated &&
      servingcellconfigdedicated->downlinkBWP_ToAddModList &&
      servingcellconfigdedicated->downlinkBWP_ToAddModList->list.count > 0) {
    n_dl_bwp = servingcellconfigdedicated->downlinkBWP_ToAddModList->list.count;
  }
  if(n_dl_bwp>0){
    SpCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList));
    for (int bwp_loop = 0; bwp_loop < n_dl_bwp; bwp_loop++) {
      NR_BWP_Downlink_t *bwp = calloc(1, sizeof(*bwp));
      config_downlinkBWP(bwp, scc,
                         servingcellconfigdedicated,
                         NULL, 0,
                         false, bwp_loop, true);
      ASN_SEQUENCE_ADD(&SpCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list,bwp);
    }
    SpCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id));
    *SpCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id = servingcellconfigdedicated->firstActiveDownlinkBWP_Id ? *servingcellconfigdedicated->firstActiveDownlinkBWP_Id : 1;
    SpCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id));
    *SpCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id = servingcellconfigdedicated->defaultDownlinkBWP_Id ? *servingcellconfigdedicated->defaultDownlinkBWP_Id : 1;
  }

  // Uplink BWPs
  int n_ul_bwp = 0;
  if (servingcellconfigdedicated && servingcellconfigdedicated->uplinkConfig &&
      servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList &&
      servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count > 0) {
    n_ul_bwp = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count;
  }
  if(n_ul_bwp>0) {
    SpCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList));
    for (int bwp_loop = 0; bwp_loop < n_ul_bwp; bwp_loop++) {
      NR_BWP_Uplink_t *ubwp = calloc(1, sizeof(*ubwp));
      config_uplinkBWP(ubwp, bwp_loop, true, uid,
                       configuration,
                       servingcellconfigdedicated,
                       scc,
                       NULL);
      ASN_SEQUENCE_ADD(&SpCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list, ubwp);
    }
    SpCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id = calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id));
    *SpCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id = servingcellconfigdedicated->uplinkConfig->firstActiveUplinkBWP_Id ? *servingcellconfigdedicated->uplinkConfig->firstActiveUplinkBWP_Id : 1;
  }


  SpCellConfig->spCellConfigDedicated->csi_MeasConfig=calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->csi_MeasConfig));
  SpCellConfig->spCellConfigDedicated->csi_MeasConfig->present = NR_SetupRelease_CSI_MeasConfig_PR_setup;

  NR_CSI_MeasConfig_t *csi_MeasConfig = calloc(1,sizeof(*csi_MeasConfig));
  SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup = csi_MeasConfig;

  csi_MeasConfig->csi_SSB_ResourceSetToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_SSB_ResourceSetToAddModList));
  csi_MeasConfig->csi_SSB_ResourceSetToReleaseList = NULL;
  csi_MeasConfig->csi_ResourceConfigToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_ResourceConfigToAddModList));
  csi_MeasConfig->csi_ResourceConfigToReleaseList = NULL;
  csi_MeasConfig->csi_ReportConfigToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_ReportConfigToAddModList));
  csi_MeasConfig->csi_ReportConfigToReleaseList = NULL;

  NR_CSI_SSB_ResourceSet_t *ssbresset0 = calloc(1,sizeof(*ssbresset0));
  ssbresset0->csi_SSB_ResourceSetId=0;

  for (int i=0;i<64;i++) {
    if ((bitmap >> (63 - i)) & 0x01) {
      NR_SSB_Index_t *ssbres = NULL;
      asn1cCallocOne(ssbres, i);
      ASN_SEQUENCE_ADD(&ssbresset0->csi_SSB_ResourceList.list, ssbres);
    }
  }
  ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list,ssbresset0);

  int pdsch_AntennaPorts = configuration->pdsch_AntennaPorts.N1 * configuration->pdsch_AntennaPorts.N2 * configuration->pdsch_AntennaPorts.XP;

  int bwp_loop_end = n_dl_bwp>0 ? n_dl_bwp : 1;
  for (int bwp_loop = 0; bwp_loop < bwp_loop_end; bwp_loop++) {

    int curr_bwp, bwp_id;
    struct NR_SetupRelease_PDSCH_Config *pdsch_Config;
    if(n_dl_bwp == 0) {
      pdsch_Config = SpCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config;
      curr_bwp = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
      bwp_id = 0;
    }
    else {
      NR_BWP_Downlink_t *bwp = SpCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_loop];
      pdsch_Config = bwp->bwp_Dedicated->pdsch_Config;
      curr_bwp = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
      bwp_id = bwp->bwp_Id;
    }

    config_csirs(scc, csi_MeasConfig, uid, pdsch_AntennaPorts, curr_bwp, configuration->do_CSIRS, bwp_loop);
    config_csiim(configuration->do_CSIRS, pdsch_AntennaPorts, curr_bwp, csi_MeasConfig, bwp_loop);

    NR_CSI_ResourceConfig_t *csires1 = calloc(1,sizeof(*csires1));
    csires1->csi_ResourceConfigId = bwp_id+20;
    csires1->csi_RS_ResourceSetList.present = NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB;
    csires1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB = calloc(1,sizeof(*csires1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB));
    csires1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList = calloc(1,sizeof(*csires1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList));
    NR_CSI_SSB_ResourceSetId_t *ssbres00 = calloc(1,sizeof(*ssbres00));
    *ssbres00 = 0;
    ASN_SEQUENCE_ADD(&csires1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list,ssbres00);
    csires1->bwp_Id = bwp_id;
    csires1->resourceType = NR_CSI_ResourceConfig__resourceType_periodic;
    ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ResourceConfigToAddModList->list,csires1);

    NR_PUCCH_CSI_Resource_t *pucchcsires1 = calloc(1,sizeof(*pucchcsires1));
    pucchcsires1->uplinkBandwidthPartId=bwp_id;
    pucchcsires1->pucch_Resource=2;

    if (configuration->do_CSIRS && pdsch_AntennaPorts > 1) {
      NR_CSI_ResourceConfig_t *csires0 = calloc(1,sizeof(*csires0));
      csires0->csi_ResourceConfigId = bwp_id;
      csires0->csi_RS_ResourceSetList.present = NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB;
      csires0->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB = calloc(1,sizeof(*csires0->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB));
      csires0->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList = calloc(1,sizeof(*csires0->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList));
      NR_NZP_CSI_RS_ResourceSetId_t *nzp0 = calloc(1,sizeof(*nzp0));
      *nzp0 = bwp_loop;
      ASN_SEQUENCE_ADD(&csires0->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list,nzp0);
      csires0->bwp_Id = bwp_id;
      csires0->resourceType = NR_CSI_ResourceConfig__resourceType_periodic;
      ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ResourceConfigToAddModList->list,csires0);

      NR_CSI_ResourceConfig_t *csires2 = calloc(1,sizeof(*csires2));
      csires2->csi_ResourceConfigId = bwp_id+10;
      csires2->csi_RS_ResourceSetList.present = NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_csi_IM_ResourceSetList;
      csires2->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList = calloc(1,sizeof(*csires2->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList));
      NR_CSI_IM_ResourceSetId_t *csiim00 = calloc(1,sizeof(*csiim00));
      *csiim00 = bwp_loop;
      ASN_SEQUENCE_ADD(&csires2->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList->list,csiim00);
      csires2->bwp_Id = bwp_id;
      csires2->resourceType = NR_CSI_ResourceConfig__resourceType_periodic;
      ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ResourceConfigToAddModList->list,csires2);

      if (pdsch_AntennaPorts > 1) {
        LOG_D(NR_RRC,"Filling CSI Report Config for RI_PMI_CQI\n");
        NR_CSI_ReportConfig_t *csirep1 = calloc(1,sizeof(*csirep1));
        csirep1->reportConfigId=0;
        csirep1->carrier=NULL;
        csirep1->resourcesForChannelMeasurement=bwp_id;
        csirep1->csi_IM_ResourcesForInterference=calloc(1,sizeof(*csirep1->csi_IM_ResourcesForInterference));
        *csirep1->csi_IM_ResourcesForInterference=bwp_id+10;
        csirep1->nzp_CSI_RS_ResourcesForInterference=NULL;
        csirep1->reportConfigType.present = NR_CSI_ReportConfig__reportConfigType_PR_periodic;
        csirep1->reportConfigType.choice.periodic = calloc(1,sizeof(*csirep1->reportConfigType.choice.periodic));
        csirep1->reportConfigType.choice.periodic->reportSlotConfig.present=NR_CSI_ReportPeriodicityAndOffset_PR_slots320;
        csirep1->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320 = (7 + (20 * uid)) % 320;
        ASN_SEQUENCE_ADD(&csirep1->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list,pucchcsires1);
        csirep1->reportQuantity.present = NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI;
        csirep1->reportQuantity.choice.cri_RI_PMI_CQI=(NULL_t)0;
        csirep1->reportFreqConfiguration = calloc(1,sizeof(*csirep1->reportFreqConfiguration));
        csirep1->reportFreqConfiguration->cqi_FormatIndicator = calloc(1,sizeof(*csirep1->reportFreqConfiguration->cqi_FormatIndicator));
        *csirep1->reportFreqConfiguration->cqi_FormatIndicator=NR_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI;
        csirep1->reportFreqConfiguration->pmi_FormatIndicator = calloc(1,sizeof(*csirep1->reportFreqConfiguration->pmi_FormatIndicator));
        *csirep1->reportFreqConfiguration->pmi_FormatIndicator=NR_CSI_ReportConfig__reportFreqConfiguration__pmi_FormatIndicator_widebandPMI;
        csirep1->reportFreqConfiguration->csi_ReportingBand = NULL;
        /*calloc(1,sizeof(*csirep1->reportFreqConfiguration->csi_ReportingBand));
        csirep1->reportFreqConfiguration->csi_ReportingBand->present = NR_CSI_ReportConfig__reportFreqConfiguration__csi_ReportingBand_PR_subbands7;
        csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.size=1;
        csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.bits_unused=1;
        csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.buf=malloc(1);
        csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.buf[0]=254;*/
        csirep1->timeRestrictionForChannelMeasurements= NR_CSI_ReportConfig__timeRestrictionForChannelMeasurements_notConfigured;
        csirep1->timeRestrictionForInterferenceMeasurements=NR_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_notConfigured;
        csirep1->codebookConfig=calloc(1,sizeof(*csirep1->codebookConfig));
        csirep1->codebookConfig->codebookType.present = NR_CodebookConfig__codebookType_PR_type1;
        csirep1->codebookConfig->codebookType.choice.type1 = calloc(1,sizeof(*csirep1->codebookConfig->codebookType.choice.type1));
        csirep1->codebookConfig->codebookType.choice.type1->subType.present=NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel;
        csirep1->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel=calloc(1,sizeof(*csirep1->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel));
        struct NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel *singlePanelConfig = csirep1->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel;
        singlePanelConfig->nrOfAntennaPorts.present = NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_two;
        singlePanelConfig->nrOfAntennaPorts.choice.two = calloc(1,sizeof(*singlePanelConfig->nrOfAntennaPorts.choice.two));
        singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.size=1;
        singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.bits_unused=2;
        singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.buf=malloc(1);
        singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.buf[0]=0xfc;
        singlePanelConfig->typeI_SinglePanel_ri_Restriction.size=1;
        singlePanelConfig->typeI_SinglePanel_ri_Restriction.bits_unused=0;
        singlePanelConfig->typeI_SinglePanel_ri_Restriction.buf=malloc(1);
        singlePanelConfig->typeI_SinglePanel_ri_Restriction.buf[0]=0x03;
        csirep1->codebookConfig->codebookType.choice.type1->codebookMode=1;
        csirep1->dummy = NULL;
        csirep1->groupBasedBeamReporting.present = NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
        csirep1->groupBasedBeamReporting.choice.disabled=calloc(1,sizeof(*csirep1->groupBasedBeamReporting.choice.disabled));
        //csirep1->groupBasedBeamReporting.choice.disabled->nrofReportedRS = calloc(1,sizeof(*csirep1->groupBasedBeamReporting.choice.disabled->nrofReportedRS));
        //*csirep1->groupBasedBeamReporting.choice.disabled->nrofReportedRS=NR_CSI_ReportConfig__groupBasedBeamReporting__disabled__nrofReportedRS_n1;
        csirep1->cqi_Table = calloc(1,sizeof(*csirep1->cqi_Table));
        if(pdsch_Config->choice.setup->mcs_Table!=NULL)
          *csirep1->cqi_Table = NR_CSI_ReportConfig__cqi_Table_table2;
        else
          *csirep1->cqi_Table = NR_CSI_ReportConfig__cqi_Table_table1;
        csirep1->subbandSize = NR_CSI_ReportConfig__subbandSize_value2;
        csirep1->non_PMI_PortIndication = NULL;
        csirep1->ext1 = NULL;
        ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ReportConfigToAddModList->list,csirep1);
      }

      LOG_D(NR_RRC,"Filling CSI Report Config for CRI_RSRP\n");
      NR_CSI_ReportConfig_t *csirep2 = calloc(1,sizeof(*csirep2));
      csirep2->reportConfigId=bwp_id+10;
      csirep2->carrier=NULL;
      csirep2->resourcesForChannelMeasurement=bwp_id;
      csirep2->csi_IM_ResourcesForInterference=NULL;
      csirep2->nzp_CSI_RS_ResourcesForInterference=NULL;
      csirep2->reportConfigType.present = NR_CSI_ReportConfig__reportConfigType_PR_periodic;
      csirep2->reportConfigType.choice.periodic = calloc(1,sizeof(*csirep2->reportConfigType.choice.periodic));
      csirep2->reportConfigType.choice.periodic->reportSlotConfig.present=NR_CSI_ReportPeriodicityAndOffset_PR_slots320;
      csirep2->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320 = (37 + (20 * uid)) % 320;
      ASN_SEQUENCE_ADD(&csirep2->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list,pucchcsires1);
      csirep2->reportQuantity.present = NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP;
      csirep2->reportQuantity.choice.cri_RSRP=(NULL_t)0;
      csirep2->reportFreqConfiguration = calloc(1,sizeof(*csirep2->reportFreqConfiguration));
      csirep2->reportFreqConfiguration->cqi_FormatIndicator = NULL;
      csirep2->reportFreqConfiguration->pmi_FormatIndicator=NULL;
      csirep2->reportFreqConfiguration->csi_ReportingBand=NULL;
      csirep2->timeRestrictionForChannelMeasurements= NR_CSI_ReportConfig__timeRestrictionForChannelMeasurements_configured;
      csirep2->timeRestrictionForInterferenceMeasurements=NR_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_configured;
      csirep2->codebookConfig=NULL;
      csirep2->dummy = NULL;
      csirep2->groupBasedBeamReporting.present = NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
      csirep2->groupBasedBeamReporting.choice.disabled=calloc(1,sizeof(*csirep2->groupBasedBeamReporting.choice.disabled));
      csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS=calloc(1,sizeof(*csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS));
      *csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS=NR_CSI_ReportConfig__groupBasedBeamReporting__disabled__nrofReportedRS_n1;
      csirep2->cqi_Table = NULL;
      csirep2->subbandSize = NR_CSI_ReportConfig__subbandSize_value1;
      csirep2->non_PMI_PortIndication = NULL;
      csirep2->ext1 = NULL;
      ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ReportConfigToAddModList->list,csirep2);
    }
    else{
      NR_CSI_ReportConfig_t *csirep3 = calloc(1,sizeof(*csirep3));
      csirep3->reportConfigId=bwp_id+10;
      csirep3->carrier=NULL;
      csirep3->resourcesForChannelMeasurement=bwp_id+20;
      csirep3->csi_IM_ResourcesForInterference=NULL;
      csirep3->nzp_CSI_RS_ResourcesForInterference=NULL;
      csirep3->reportConfigType.present = NR_CSI_ReportConfig__reportConfigType_PR_periodic;
      csirep3->reportConfigType.choice.periodic = calloc(1,sizeof(*csirep3->reportConfigType.choice.periodic));
      csirep3->reportConfigType.choice.periodic->reportSlotConfig.present=NR_CSI_ReportPeriodicityAndOffset_PR_slots320;
      csirep3->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320 = 37 + (20 * uid) % 320;
      ASN_SEQUENCE_ADD(&csirep3->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list,pucchcsires1);
      csirep3->reportQuantity.present = NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP;
      csirep3->reportQuantity.choice.ssb_Index_RSRP=(NULL_t)0;
      csirep3->reportFreqConfiguration = NULL;
      csirep3->timeRestrictionForChannelMeasurements= NR_CSI_ReportConfig__timeRestrictionForChannelMeasurements_configured;
      csirep3->timeRestrictionForInterferenceMeasurements=NR_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_configured;
      csirep3->codebookConfig= NULL;
      csirep3->dummy = NULL;
      csirep3->groupBasedBeamReporting.present = NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
      csirep3->groupBasedBeamReporting.choice.disabled=calloc(1,sizeof(*csirep3->groupBasedBeamReporting.choice.disabled));
      csirep3->groupBasedBeamReporting.choice.disabled->nrofReportedRS = calloc(1,sizeof(*csirep3->groupBasedBeamReporting.choice.disabled->nrofReportedRS));
      *csirep3->groupBasedBeamReporting.choice.disabled->nrofReportedRS=NR_CSI_ReportConfig__groupBasedBeamReporting__disabled__nrofReportedRS_n1;
      csirep3->cqi_Table = NULL;
      csirep3->subbandSize = NR_CSI_ReportConfig__subbandSize_value1;
      csirep3->non_PMI_PortIndication = NULL;
      csirep3->ext1 = NULL;
      ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ReportConfigToAddModList->list,csirep3);
    }
  }
  pdsch_servingcellconfig->codeBlockGroupTransmission = NULL;
  pdsch_servingcellconfig->xOverhead = NULL;
  pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH = calloc(1, sizeof(*pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH));
  *pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH = NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n16;
  pdsch_servingcellconfig->pucch_Cell= NULL;
  pdsch_servingcellconfig->ext1=calloc(1,sizeof(*pdsch_servingcellconfig->ext1));
  pdsch_servingcellconfig->ext1->maxMIMO_Layers = calloc(1,sizeof(*pdsch_servingcellconfig->ext1->maxMIMO_Layers));
  *pdsch_servingcellconfig->ext1->maxMIMO_Layers = 2;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_SpCellConfig, (void *)SpCellConfig);
  }

}

NR_RLC_BearerConfig_t *get_SRB_RLC_BearerConfig(long channelId,
                                                long priority,
                                                e_NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration bucketSizeDuration)
{
  NR_RLC_BearerConfig_t *rlc_BearerConfig = NULL;
  rlc_BearerConfig                                                 = calloc(1, sizeof(NR_RLC_BearerConfig_t));
  rlc_BearerConfig->logicalChannelIdentity                         = channelId;
  rlc_BearerConfig->servedRadioBearer                              = calloc(1, sizeof(*rlc_BearerConfig->servedRadioBearer));
  rlc_BearerConfig->servedRadioBearer->present                     = NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity;
  rlc_BearerConfig->servedRadioBearer->choice.srb_Identity         = channelId;
  rlc_BearerConfig->reestablishRLC                                 = NULL;

  NR_RLC_Config_t *rlc_Config                                      = calloc(1, sizeof(NR_RLC_Config_t));
  rlc_Config->present                                              = NR_RLC_Config_PR_am;
  rlc_Config->choice.am                                            = calloc(1, sizeof(*rlc_Config->choice.am));
  rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength                  = calloc(1, sizeof(NR_SN_FieldLengthAM_t));
  *(rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength)               = NR_SN_FieldLengthAM_size12;
  rlc_Config->choice.am->dl_AM_RLC.t_Reassembly                    = NR_T_Reassembly_ms35;
  rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit                = NR_T_StatusProhibit_ms0;
  rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength                  = calloc(1, sizeof(NR_SN_FieldLengthAM_t));
  *(rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength)               = NR_SN_FieldLengthAM_size12;
  rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit                = NR_T_PollRetransmit_ms45;
  rlc_Config->choice.am->ul_AM_RLC.pollPDU                         = NR_PollPDU_infinity;
  rlc_Config->choice.am->ul_AM_RLC.pollByte                        = NR_PollByte_infinity;
  rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold                = NR_UL_AM_RLC__maxRetxThreshold_t8;
  rlc_BearerConfig->rlc_Config                                     = rlc_Config;

  NR_LogicalChannelConfig_t *logicalChannelConfig                  = calloc(1, sizeof(NR_LogicalChannelConfig_t));
  logicalChannelConfig->ul_SpecificParameters                      = calloc(1, sizeof(*logicalChannelConfig->ul_SpecificParameters));
  logicalChannelConfig->ul_SpecificParameters->priority            = priority;
  logicalChannelConfig->ul_SpecificParameters->prioritisedBitRate  = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  logicalChannelConfig->ul_SpecificParameters->bucketSizeDuration  = bucketSizeDuration;

  long *logicalChannelGroup                                        = CALLOC(1, sizeof(long));
  *logicalChannelGroup                                             = 0;
  logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup = logicalChannelGroup;
  logicalChannelConfig->ul_SpecificParameters->schedulingRequestID = CALLOC(1, sizeof(*logicalChannelConfig->ul_SpecificParameters->schedulingRequestID));
  *logicalChannelConfig->ul_SpecificParameters->schedulingRequestID = 0;
  logicalChannelConfig->ul_SpecificParameters->logicalChannelSR_Mask = 0;
  logicalChannelConfig->ul_SpecificParameters->logicalChannelSR_DelayTimerApplied = 0;
  rlc_BearerConfig->mac_LogicalChannelConfig                       = logicalChannelConfig;

  return rlc_BearerConfig;
}

NR_RLC_BearerConfig_t *get_DRB_RLC_BearerConfig(long lcChannelId, long drbId, NR_RLC_Config_PR rlc_conf, long priority)
{
  NR_RLC_BearerConfig_t *rlc_BearerConfig                  = calloc(1, sizeof(NR_RLC_BearerConfig_t));
  rlc_BearerConfig->logicalChannelIdentity                 = lcChannelId;
  rlc_BearerConfig->servedRadioBearer                      = calloc(1, sizeof(*rlc_BearerConfig->servedRadioBearer));
  rlc_BearerConfig->servedRadioBearer->present             = NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity;
  rlc_BearerConfig->servedRadioBearer->choice.drb_Identity = drbId;
  rlc_BearerConfig->reestablishRLC                         = NULL;

  NR_RLC_Config_t *rlc_Config  = calloc(1, sizeof(NR_RLC_Config_t));
  nr_drb_config(rlc_Config, rlc_conf);
  rlc_BearerConfig->rlc_Config = rlc_Config;

  NR_LogicalChannelConfig_t *logicalChannelConfig                 = calloc(1, sizeof(NR_LogicalChannelConfig_t));
  logicalChannelConfig->ul_SpecificParameters                     = calloc(1, sizeof(*logicalChannelConfig->ul_SpecificParameters));
  logicalChannelConfig->ul_SpecificParameters->priority           = priority;
  logicalChannelConfig->ul_SpecificParameters->prioritisedBitRate = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8;
  logicalChannelConfig->ul_SpecificParameters->bucketSizeDuration = NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms100;

  long *logicalChannelGroup                                          = CALLOC(1, sizeof(long));
  *logicalChannelGroup                                               = 1;
  logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup   = logicalChannelGroup;
  logicalChannelConfig->ul_SpecificParameters->schedulingRequestID   = CALLOC(1, sizeof(*logicalChannelConfig->ul_SpecificParameters->schedulingRequestID));
  *logicalChannelConfig->ul_SpecificParameters->schedulingRequestID  = 0;
  logicalChannelConfig->ul_SpecificParameters->logicalChannelSR_Mask = 0;
  logicalChannelConfig->ul_SpecificParameters->logicalChannelSR_DelayTimerApplied = 0;
  rlc_BearerConfig->mac_LogicalChannelConfig                         = logicalChannelConfig;

  return rlc_BearerConfig;
}

void fill_mastercellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig, NR_CellGroupConfig_t *ue_context_mastercellGroup, int use_rlc_um_for_drb, uint8_t configure_srb, uint8_t bearer_id_start, uint8_t nb_bearers_to_setup, long *priority) {

  cellGroupConfig->cellGroupId = 0;
  cellGroupConfig->rlc_BearerToReleaseList = NULL;
  cellGroupConfig->rlc_BearerToAddModList = calloc(1, sizeof(*cellGroupConfig->rlc_BearerToAddModList));

  // RLC Bearer Config
  // TS38.331 9.2.1 Default SRB configurations
  if (configure_srb){
    NR_RLC_BearerConfig_t *rlc_BearerConfig = get_SRB_RLC_BearerConfig(2, 3, NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms5);
    ASN_SEQUENCE_ADD(&cellGroupConfig->rlc_BearerToAddModList->list, rlc_BearerConfig);
    ASN_SEQUENCE_ADD(&ue_context_mastercellGroup->rlc_BearerToAddModList->list, rlc_BearerConfig);
  }

  // DRB Configuration
  for (int i = bearer_id_start; i < bearer_id_start + nb_bearers_to_setup; i++ ){
    const NR_RLC_Config_PR rlc_conf = use_rlc_um_for_drb ? NR_RLC_Config_PR_um_Bi_Directional : NR_RLC_Config_PR_am;
    NR_RLC_BearerConfig_t *rlc_BearerConfig = get_DRB_RLC_BearerConfig(3 + i, i, rlc_conf, priority[i-bearer_id_start]);
    ASN_SEQUENCE_ADD(&cellGroupConfig->rlc_BearerToAddModList->list, rlc_BearerConfig);
    ASN_SEQUENCE_ADD(&ue_context_mastercellGroup->rlc_BearerToAddModList->list, rlc_BearerConfig);
  }
}

//TODO temp function (to remove once CSI meas config harmonization is done)
void update_cqitables(struct NR_SetupRelease_PDSCH_Config *pdsch_Config,
                     NR_CSI_MeasConfig_t *csi_MeasConfig) {
  int nb_csi = csi_MeasConfig->csi_ReportConfigToAddModList->list.count;
  for (int i = 0; i < nb_csi; i++) {
    NR_CSI_ReportConfig_t *csirep = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[i];
    if(csirep->cqi_Table) {
      if(pdsch_Config->choice.setup->mcs_Table!=NULL)
        *csirep->cqi_Table = NR_CSI_ReportConfig__cqi_Table_table2;
      else
        *csirep->cqi_Table = NR_CSI_ReportConfig__cqi_Table_table1;
    }
  }
}

void update_cellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig,
                            const int uid,
                            NR_UE_NR_Capability_t *uecap,
                            const gNB_RrcConfigurationReq* configuration) {

  NR_SpCellConfig_t *SpCellConfig = cellGroupConfig->spCellConfig;

  if (SpCellConfig == NULL) return;

  NR_ServingCellConfigCommon_t *scc = configuration ? configuration->scc : NULL;

  if(scc) {
    int curr_bwp = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
    // SRS configuration
    if (configuration->do_SRS &&
        SpCellConfig &&
        SpCellConfig->spCellConfigDedicated &&
        SpCellConfig->spCellConfigDedicated->uplinkConfig &&
        SpCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP) {
      if (!SpCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config) {
        SpCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config =
            calloc(1,sizeof(*SpCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config));
      }
      config_srs(SpCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config,
                 uecap,
                 curr_bwp,
                 uid,
                 0,
                 configuration->do_SRS);
    }

    // Set DL MCS table
    NR_BWP_DownlinkDedicated_t *bwp_Dedicated = SpCellConfig->spCellConfigDedicated->initialDownlinkBWP;
    set_dl_mcs_table(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing,
                     configuration->force_256qam_off ? NULL : uecap, bwp_Dedicated, scc);
    struct NR_ServingCellConfig__downlinkBWP_ToAddModList *DL_BWP_list = SpCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList;
    struct NR_UplinkConfig__uplinkBWP_ToAddModList *UL_BWP_list = SpCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList;
    if (DL_BWP_list) {
      for (int i=0; i<DL_BWP_list->list.count; i++){
        NR_BWP_Downlink_t *bwp = DL_BWP_list->list.array[i];
        int scs = bwp->bwp_Common->genericParameters.subcarrierSpacing;
        set_dl_mcs_table(scs, configuration->force_256qam_off ? NULL : uecap, bwp->bwp_Dedicated, scc);
      }
    }
    if (UL_BWP_list) {
      for (int i=0; i<UL_BWP_list->list.count; i++) {
        NR_BWP_Uplink_t *ul_bwp = UL_BWP_list->list.array[i];
        int bwp_size = NRRIV2BW(ul_bwp->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
        config_srs(ul_bwp->bwp_Dedicated->srs_Config,
                   uecap,
                   bwp_size,
                   uid,
                   i+1,
                   configuration->do_SRS);
      }
    }
    update_cqitables(bwp_Dedicated->pdsch_Config, SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup);
  }
}

void fill_initial_cellGroupConfig(int uid,
                                  NR_CellGroupConfig_t *cellGroupConfig,
                                  const NR_ServingCellConfigCommon_t *scc,
                                  const NR_ServingCellConfig_t *servingcellconfigdedicated,
                                  const gNB_RrcConfigurationReq *configuration)
{
  NR_MAC_CellGroupConfig_t                         *mac_CellGroupConfig  = NULL;
  NR_PhysicalCellGroupConfig_t	                   *physicalCellGroupConfig = NULL;
  
  cellGroupConfig->cellGroupId = 0;
  
  /* Rlc Bearer Config */
  /* TS38.331 9.2.1	Default SRB configurations */
  cellGroupConfig->rlc_BearerToAddModList                          = calloc(1, sizeof(*cellGroupConfig->rlc_BearerToAddModList));
  NR_RLC_BearerConfig_t *rlc_BearerConfig = get_SRB_RLC_BearerConfig(1, 1, NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms1000);
  ASN_SEQUENCE_ADD(&cellGroupConfig->rlc_BearerToAddModList->list, rlc_BearerConfig);
  
  cellGroupConfig->rlc_BearerToReleaseList = NULL;
  
  /* mac CellGroup Config */
  mac_CellGroupConfig                                                     = calloc(1, sizeof(*mac_CellGroupConfig));

  mac_CellGroupConfig->bsr_Config                                         = calloc(1, sizeof(*mac_CellGroupConfig->bsr_Config));
  mac_CellGroupConfig->bsr_Config->periodicBSR_Timer                      = NR_BSR_Config__periodicBSR_Timer_sf10;
  mac_CellGroupConfig->bsr_Config->retxBSR_Timer                          = NR_BSR_Config__retxBSR_Timer_sf80;
  mac_CellGroupConfig->tag_Config                                         = calloc(1, sizeof(*mac_CellGroupConfig->tag_Config));
  mac_CellGroupConfig->tag_Config->tag_ToReleaseList = NULL;
  mac_CellGroupConfig->tag_Config->tag_ToAddModList  = calloc(1,sizeof(*mac_CellGroupConfig->tag_Config->tag_ToAddModList));
  struct NR_TAG *tag=calloc(1,sizeof(*tag));
  tag->tag_Id             = 0;
  tag->timeAlignmentTimer = NR_TimeAlignmentTimer_infinity;
  ASN_SEQUENCE_ADD(&mac_CellGroupConfig->tag_Config->tag_ToAddModList->list,tag);
  set_phr_config(mac_CellGroupConfig);

  mac_CellGroupConfig->schedulingRequestConfig = calloc(1, sizeof(*mac_CellGroupConfig->schedulingRequestConfig));
  mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList = CALLOC(1,sizeof(*mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList));
  struct NR_SchedulingRequestToAddMod *schedulingrequestlist = CALLOC(1,sizeof(*schedulingrequestlist));
  schedulingrequestlist->schedulingRequestId  = 0;
  schedulingrequestlist->sr_ProhibitTimer = NULL;
  schedulingrequestlist->sr_TransMax      = NR_SchedulingRequestToAddMod__sr_TransMax_n64;
  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList->list),schedulingrequestlist);

  cellGroupConfig->mac_CellGroupConfig                                      = mac_CellGroupConfig;

  physicalCellGroupConfig                                                   = calloc(1,sizeof(*physicalCellGroupConfig));
  physicalCellGroupConfig->p_NR_FR1                                         = calloc(1,sizeof(*physicalCellGroupConfig->p_NR_FR1));
  *physicalCellGroupConfig->p_NR_FR1                                        = 10;
  physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook                          = NR_PhysicalCellGroupConfig__pdsch_HARQ_ACK_Codebook_dynamic;
  cellGroupConfig->physicalCellGroupConfig                                  = physicalCellGroupConfig;
  
  cellGroupConfig->spCellConfig                                             = calloc(1,sizeof(*cellGroupConfig->spCellConfig));

  fill_initial_SpCellConfig(uid,cellGroupConfig->spCellConfig,scc,servingcellconfigdedicated,configuration);
  
  cellGroupConfig->sCellToAddModList                                        = NULL;
  cellGroupConfig->sCellToReleaseList                                       = NULL;
}

//------------------------------------------------------------------------------
int do_RRCSetup(rrc_gNB_ue_context_t         *const ue_context_pP,
                uint8_t                      *const buffer,
                const uint8_t                transaction_id,
                const uint8_t                *masterCellGroup,
                int                          masterCellGroup_len,
                const NR_ServingCellConfigCommon_t *scc,
                const NR_ServingCellConfig_t        *servingcellconfigdedicated,
                const gNB_RrcConfigurationReq *configuration)
//------------------------------------------------------------------------------
{
    asn_enc_rval_t                                   enc_rval;
    NR_DL_CCCH_Message_t                             dl_ccch_msg;
    NR_RRCSetup_t                                    *rrcSetup;
    NR_RRCSetup_IEs_t                                *ie;
    NR_SRB_ToAddMod_t                                *SRB1_config          = NULL;
    NR_PDCP_Config_t                                 *pdcp_Config          = NULL;
    NR_CellGroupConfig_t                             *cellGroupConfig      = NULL;

    AssertFatal(ue_context_pP != NULL,"ue_context_p is null\n");
    gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
    NR_SRB_ToAddModList_t        **SRB_configList = &ue_p->SRB_configList;



    memset((void *)&dl_ccch_msg, 0, sizeof(NR_DL_CCCH_Message_t));
    dl_ccch_msg.message.present            = NR_DL_CCCH_MessageType_PR_c1;
    dl_ccch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_DL_CCCH_MessageType__c1));
    dl_ccch_msg.message.choice.c1->present = NR_DL_CCCH_MessageType__c1_PR_rrcSetup;
    dl_ccch_msg.message.choice.c1->choice.rrcSetup = calloc(1, sizeof(NR_RRCSetup_t));

    rrcSetup = dl_ccch_msg.message.choice.c1->choice.rrcSetup;
    rrcSetup->criticalExtensions.present = NR_RRCSetup__criticalExtensions_PR_rrcSetup;
    rrcSetup->rrc_TransactionIdentifier  = transaction_id;
    rrcSetup->criticalExtensions.choice.rrcSetup = calloc(1, sizeof(NR_RRCSetup_IEs_t));
    ie = rrcSetup->criticalExtensions.choice.rrcSetup;

    /****************************** radioBearerConfig ******************************/

    /* Configure SRB1 */
    if (*SRB_configList) {
        free(*SRB_configList);
    }

    *SRB_configList = calloc(1, sizeof(NR_SRB_ToAddModList_t));
    // SRB1
    /* TODO */
    SRB1_config = calloc(1, sizeof(NR_SRB_ToAddMod_t));
    SRB1_config->srb_Identity = 1;
    // pdcp_Config->t_Reordering
    SRB1_config->pdcp_Config = pdcp_Config;
    ie->radioBearerConfig.srb_ToAddModList = *SRB_configList;
    ASN_SEQUENCE_ADD(&(*SRB_configList)->list, SRB1_config);

    ie->radioBearerConfig.srb3_ToRelease    = NULL;
    ie->radioBearerConfig.drb_ToAddModList  = NULL;
    ie->radioBearerConfig.drb_ToReleaseList = NULL;
    ie->radioBearerConfig.securityConfig    = NULL;
    
    /****************************** masterCellGroup ******************************/
    DevAssert(masterCellGroup && masterCellGroup_len > 0);
    ie->masterCellGroup.buf = malloc(masterCellGroup_len);
    AssertFatal(ie->masterCellGroup.buf != NULL, "could not allocate memory for masterCellGroup\n");
    memcpy(ie->masterCellGroup.buf, masterCellGroup, masterCellGroup_len);
    ie->masterCellGroup.size = masterCellGroup_len;

    // decode masterCellGroup OCTET_STRING received from DU and place in ue context
    uper_decode(NULL, &asn_DEF_NR_CellGroupConfig, (void **)&cellGroupConfig, masterCellGroup, masterCellGroup_len, 0, 0);
    ue_p->masterCellGroup = cellGroupConfig;

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void*)cellGroupConfig);
      xer_fprint(stdout, &asn_DEF_NR_DL_CCCH_Message, (void *)&dl_ccch_msg);
    }

    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_CCCH_Message,
				     NULL,
				     (void *)&dl_ccch_msg,
				     buffer,
				     1000);
    
    if(enc_rval.encoded == -1) {
      LOG_E(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
	    enc_rval.failed_type->name, enc_rval.encoded);
      return -1;
    }
    
    LOG_D(NR_RRC,"RRCSetup Encoded %zd bits (%zd bytes)\n",
            enc_rval.encoded,(enc_rval.encoded+7)/8);
    return((enc_rval.encoded+7)/8);
}

uint8_t do_NR_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *const buffer,
  const uint8_t Transaction_id,
  const uint8_t cipheringAlgorithm,
  NR_IntegrityProtAlgorithm_t *integrityProtAlgorithm
)
//------------------------------------------------------------------------------
{
  NR_DL_DCCH_Message_t dl_dcch_msg;
  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg,0,sizeof(NR_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = NR_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1=CALLOC(1,sizeof(struct NR_DL_DCCH_MessageType__c1));
  dl_dcch_msg.message.choice.c1->present = NR_DL_DCCH_MessageType__c1_PR_securityModeCommand;
  dl_dcch_msg.message.choice.c1->choice.securityModeCommand = CALLOC(1, sizeof(struct NR_SecurityModeCommand));
  dl_dcch_msg.message.choice.c1->choice.securityModeCommand->rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg.message.choice.c1->choice.securityModeCommand->criticalExtensions.present = NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand;

  dl_dcch_msg.message.choice.c1->choice.securityModeCommand->criticalExtensions.choice.securityModeCommand =
		  CALLOC(1, sizeof(struct NR_SecurityModeCommand_IEs));
  // the two following information could be based on the mod_id
  dl_dcch_msg.message.choice.c1->choice.securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm
    = (NR_CipheringAlgorithm_t)cipheringAlgorithm;
  dl_dcch_msg.message.choice.c1->choice.securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm
    = integrityProtAlgorithm;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message, (void *)&dl_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(NR_RRC,"[gNB %d] securityModeCommand for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    LOG_E(NR_RRC,"[gNB %d] ASN1 : securityModeCommand encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  //  rrc_ue_process_ueCapabilityEnquiry(0,1000,&dl_dcch_msg.message.choice.c1.choice.ueCapabilityEnquiry,0);
  //  exit(-1);
  return((enc_rval.encoded+7)/8);
}

/*TODO*/
//------------------------------------------------------------------------------
uint8_t do_NR_SA_UECapabilityEnquiry( const protocol_ctxt_t *const ctxt_pP,
                                   uint8_t               *const buffer,
                                   const uint8_t                Transaction_id)
//------------------------------------------------------------------------------
{
  NR_UE_CapabilityRequestFilterNR_t *sa_band_filter;
  NR_FreqBandList_t *sa_band_list;
  NR_FreqBandInformation_t *sa_band_info;
  NR_FreqBandInformationNR_t *sa_band_infoNR;

  NR_DL_DCCH_Message_t dl_dcch_msg;
  NR_UE_CapabilityRAT_Request_t *ue_capabilityrat_request;

  asn_enc_rval_t enc_rval;
  memset(&dl_dcch_msg,0,sizeof(NR_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = NR_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1 = CALLOC(1,sizeof(struct NR_DL_DCCH_MessageType__c1));
  dl_dcch_msg.message.choice.c1->present = NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry;
  dl_dcch_msg.message.choice.c1->choice.ueCapabilityEnquiry = CALLOC(1,sizeof(struct NR_UECapabilityEnquiry));
  dl_dcch_msg.message.choice.c1->choice.ueCapabilityEnquiry->rrc_TransactionIdentifier = Transaction_id;
  dl_dcch_msg.message.choice.c1->choice.ueCapabilityEnquiry->criticalExtensions.present = NR_UECapabilityEnquiry__criticalExtensions_PR_ueCapabilityEnquiry;
  dl_dcch_msg.message.choice.c1->choice.ueCapabilityEnquiry->criticalExtensions.choice.ueCapabilityEnquiry = CALLOC(1,sizeof(struct NR_UECapabilityEnquiry_IEs));
  ue_capabilityrat_request =  CALLOC(1,sizeof(NR_UE_CapabilityRAT_Request_t));
  memset(ue_capabilityrat_request,0,sizeof(NR_UE_CapabilityRAT_Request_t));
  ue_capabilityrat_request->rat_Type = NR_RAT_Type_nr;

  sa_band_infoNR = (NR_FreqBandInformationNR_t*)calloc(1,sizeof(NR_FreqBandInformationNR_t));
  sa_band_infoNR->bandNR = 78;
  sa_band_info = (NR_FreqBandInformation_t*)calloc(1,sizeof(NR_FreqBandInformation_t));
  sa_band_info->present = NR_FreqBandInformation_PR_bandInformationNR;
  sa_band_info->choice.bandInformationNR = sa_band_infoNR;
  
  sa_band_list = (NR_FreqBandList_t *)calloc(1, sizeof(NR_FreqBandList_t));
  ASN_SEQUENCE_ADD(&sa_band_list->list, sa_band_info);

  sa_band_filter = (NR_UE_CapabilityRequestFilterNR_t*)calloc(1,sizeof(NR_UE_CapabilityRequestFilterNR_t));
  sa_band_filter->frequencyBandListFilter = sa_band_list;

  OCTET_STRING_t req_freq;
  unsigned char req_freq_buf[1024];
  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UE_CapabilityRequestFilterNR,
				   NULL,
				   (void *)sa_band_filter,
				   req_freq_buf,
				   1024);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_UE_CapabilityRequestFilterNR, (void *)sa_band_filter);
  }

  req_freq.buf = req_freq_buf;
  req_freq.size = (enc_rval.encoded+7)/8;

  ue_capabilityrat_request->capabilityRequestFilter = &req_freq;

  ASN_SEQUENCE_ADD(&dl_dcch_msg.message.choice.c1->choice.ueCapabilityEnquiry->criticalExtensions.choice.ueCapabilityEnquiry->ue_CapabilityRAT_RequestList.list,
                   ue_capabilityrat_request);


  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message, (void *)&dl_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   100);

  if(enc_rval.encoded == -1) {
    LOG_I(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
          enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }

  LOG_D(NR_RRC,"[gNB %d] NR UECapabilityRequest for UE %x Encoded %zd bits (%zd bytes)\n",
        ctxt_pP->module_id,
        ctxt_pP->rnti,
        enc_rval.encoded,
        (enc_rval.encoded+7)/8);

  if (enc_rval.encoded==-1) {
    LOG_E(NR_RRC,"[gNB %d] ASN1 : NR UECapabilityRequest encoding failed for UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->rnti);
    return(-1);
  }

  return((enc_rval.encoded+7)/8);
}


uint8_t do_NR_RRCRelease(uint8_t                            *buffer,
                         size_t                              buffer_size,
                         uint8_t                             Transaction_id) {
  asn_enc_rval_t enc_rval;
  NR_DL_DCCH_Message_t dl_dcch_msg;
  NR_RRCRelease_t *rrcConnectionRelease;
  memset(&dl_dcch_msg,0,sizeof(NR_DL_DCCH_Message_t));
  dl_dcch_msg.message.present           = NR_DL_DCCH_MessageType_PR_c1;
  dl_dcch_msg.message.choice.c1=CALLOC(1,sizeof(struct NR_DL_DCCH_MessageType__c1));
  dl_dcch_msg.message.choice.c1->present = NR_DL_DCCH_MessageType__c1_PR_rrcRelease;
  dl_dcch_msg.message.choice.c1->choice.rrcRelease = CALLOC(1, sizeof(NR_RRCRelease_t));
  rrcConnectionRelease = dl_dcch_msg.message.choice.c1->choice.rrcRelease;
  // RRCConnectionRelease
  rrcConnectionRelease->rrc_TransactionIdentifier = Transaction_id;
  rrcConnectionRelease->criticalExtensions.present = NR_RRCRelease__criticalExtensions_PR_rrcRelease;
  rrcConnectionRelease->criticalExtensions.choice.rrcRelease = CALLOC(1, sizeof(NR_RRCRelease_IEs_t));
  rrcConnectionRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq =
      CALLOC(1, sizeof(struct NR_RRCRelease_IEs__deprioritisationReq));
  rrcConnectionRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq->deprioritisationType =
      NR_RRCRelease_IEs__deprioritisationReq__deprioritisationType_nr;
  rrcConnectionRelease->criticalExtensions.choice.rrcRelease->deprioritisationReq->deprioritisationTimer =
      NR_RRCRelease_IEs__deprioritisationReq__deprioritisationTimer_min10;

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message,
                                   NULL,
                                   (void *)&dl_dcch_msg,
                                   buffer,
                                   buffer_size);
  if(enc_rval.encoded == -1) {
    LOG_I(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
        enc_rval.failed_type->name, enc_rval.encoded);
    return -1;
  }
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
int16_t do_RRCReconfiguration(
    const protocol_ctxt_t        *const ctxt_pP,
    uint8_t                      *buffer,
    size_t                        buffer_size,
    uint8_t                       Transaction_id,
    NR_SRB_ToAddModList_t        *SRB_configList,
    NR_DRB_ToAddModList_t        *DRB_configList,
    NR_DRB_ToReleaseList_t       *DRB_releaseList,
    NR_SecurityConfig_t          *security_config,
    NR_SDAP_Config_t             *sdap_config,
    NR_MeasConfig_t              *meas_config,
    struct NR_RRCReconfiguration_v1530_IEs__dedicatedNAS_MessageList *dedicatedNAS_MessageList,
    rrc_gNB_ue_context_t         *const ue_context_pP,
    rrc_gNB_carrier_data_t       *carrier,
    const gNB_RrcConfigurationReq *configuration,
    NR_MAC_CellGroupConfig_t     *mac_CellGroupConfig,
    NR_CellGroupConfig_t         *cellGroupConfig)
//------------------------------------------------------------------------------
{
    NR_DL_DCCH_Message_t                             dl_dcch_msg;
    asn_enc_rval_t                                   enc_rval;
    NR_RRCReconfiguration_IEs_t                      *ie;
    unsigned char masterCellGroup_buf[1000];

    memset(&dl_dcch_msg, 0, sizeof(NR_DL_DCCH_Message_t));
    dl_dcch_msg.message.present            = NR_DL_DCCH_MessageType_PR_c1;
    dl_dcch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_DL_DCCH_MessageType__c1));
    dl_dcch_msg.message.choice.c1->present = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;

    dl_dcch_msg.message.choice.c1->choice.rrcReconfiguration = calloc(1, sizeof(NR_RRCReconfiguration_t));
    dl_dcch_msg.message.choice.c1->choice.rrcReconfiguration->rrc_TransactionIdentifier = Transaction_id;
    dl_dcch_msg.message.choice.c1->choice.rrcReconfiguration->criticalExtensions.present = NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;

    /******************** Radio Bearer Config ********************/
    /* Configure Security */
    // security_config    =  CALLOC(1, sizeof(NR_SecurityConfig_t));
    // security_config->securityAlgorithmConfig = CALLOC(1, sizeof(*ie->radioBearerConfig->securityConfig->securityAlgorithmConfig));
    // security_config->securityAlgorithmConfig->cipheringAlgorithm     = NR_CipheringAlgorithm_nea0;
    // security_config->securityAlgorithmConfig->integrityProtAlgorithm = NULL;
    // security_config->keyToUse = CALLOC(1, sizeof(*ie->radioBearerConfig->securityConfig->keyToUse));
    // *security_config->keyToUse = NR_SecurityConfig__keyToUse_master;

    ie = calloc(1, sizeof(NR_RRCReconfiguration_IEs_t));
    if (SRB_configList || DRB_configList) {
      ie->radioBearerConfig = calloc(1, sizeof(NR_RadioBearerConfig_t));
      ie->radioBearerConfig->srb_ToAddModList  = SRB_configList;
      ie->radioBearerConfig->drb_ToAddModList  = DRB_configList;
      ie->radioBearerConfig->securityConfig    = security_config;
      ie->radioBearerConfig->srb3_ToRelease    = NULL;
      ie->radioBearerConfig->drb_ToReleaseList = DRB_releaseList;
    }

    /******************** Meas Config ********************/
    // measConfig
    ie->measConfig = meas_config;
    // lateNonCriticalExtension
    ie->lateNonCriticalExtension = NULL;
    // nonCriticalExtension

    if (cellGroupConfig || dedicatedNAS_MessageList) {
      ie->nonCriticalExtension = calloc(1, sizeof(NR_RRCReconfiguration_v1530_IEs_t));
      if (dedicatedNAS_MessageList)
        ie->nonCriticalExtension->dedicatedNAS_MessageList = dedicatedNAS_MessageList;
    }

    if(cellGroupConfig!=NULL){
      update_cellGroupConfig(cellGroupConfig,
                             ue_context_pP->local_uid,
                             ue_context_pP ? ue_context_pP->ue_context.UE_Capability_nr : NULL,
                             configuration);

      enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig,
          NULL,
          (void *)cellGroupConfig,
          masterCellGroup_buf,
          1000);
      if(enc_rval.encoded == -1) {
        LOG_E(NR_RRC, "ASN1 message CellGroupConfig encoding failed (%s, %lu)!\n",
            enc_rval.failed_type->name, enc_rval.encoded);
        return -1;
      }
      if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
        xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) cellGroupConfig);
      }
      ie->nonCriticalExtension->masterCellGroup = calloc(1,sizeof(OCTET_STRING_t));

      ie->nonCriticalExtension->masterCellGroup->buf = masterCellGroup_buf;
      ie->nonCriticalExtension->masterCellGroup->size = (enc_rval.encoded+7)/8;
    }

    dl_dcch_msg.message.choice.c1->choice.rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration = ie;

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message, (void *)&dl_dcch_msg);
    }

    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message,
                                    NULL,
                                    (void *)&dl_dcch_msg,
                                    buffer,
                                    buffer_size);

    if(enc_rval.encoded == -1) {
        LOG_I(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
            enc_rval.failed_type->name, enc_rval.encoded);
        return -1;
    }

    LOG_D(NR_RRC,"[gNB %d] RRCReconfiguration for UE %x Encoded %zd bits (%zd bytes)\n",
            ctxt_pP->module_id,
            ctxt_pP->rnti,
            enc_rval.encoded,
            (enc_rval.encoded+7)/8);

    if (enc_rval.encoded == -1) {
        LOG_E(NR_RRC,"[gNB %d] ASN1 : RRCReconfiguration encoding failed for UE %x\n",
            ctxt_pP->module_id,
            ctxt_pP->rnti);
        return(-1);
    }

    return((enc_rval.encoded+7)/8);
}


uint8_t do_RRCSetupRequest(uint8_t Mod_id, uint8_t *buffer, size_t buffer_size, uint8_t *rv) {
  asn_enc_rval_t enc_rval;
  uint8_t buf[5],buf2=0;
  NR_UL_CCCH_Message_t ul_ccch_msg;
  NR_RRCSetupRequest_t *rrcSetupRequest;
  memset((void *)&ul_ccch_msg,0,sizeof(NR_UL_CCCH_Message_t));
  ul_ccch_msg.message.present           = NR_UL_CCCH_MessageType_PR_c1;
  ul_ccch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_UL_CCCH_MessageType__c1));
  ul_ccch_msg.message.choice.c1->present = NR_UL_CCCH_MessageType__c1_PR_rrcSetupRequest;
  ul_ccch_msg.message.choice.c1->choice.rrcSetupRequest = CALLOC(1, sizeof(NR_RRCSetupRequest_t));
  rrcSetupRequest          = ul_ccch_msg.message.choice.c1->choice.rrcSetupRequest;


  if (1) {
    rrcSetupRequest->rrcSetupRequest.ue_Identity.present = NR_InitialUE_Identity_PR_randomValue;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.size = 5;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.bits_unused = 1;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.buf = buf;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.buf[0] = rv[0];
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.buf[1] = rv[1];
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.buf[2] = rv[2];
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.buf[3] = rv[3];
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.randomValue.buf[4] = rv[4]&0xfe;
  } else {
    rrcSetupRequest->rrcSetupRequest.ue_Identity.present = NR_InitialUE_Identity_PR_ng_5G_S_TMSI_Part1;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.ng_5G_S_TMSI_Part1.size = 1;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.ng_5G_S_TMSI_Part1.bits_unused = 0;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.ng_5G_S_TMSI_Part1.buf = buf;
    rrcSetupRequest->rrcSetupRequest.ue_Identity.choice.ng_5G_S_TMSI_Part1.buf[0] = 0x12;
  }

  rrcSetupRequest->rrcSetupRequest.establishmentCause = NR_EstablishmentCause_mo_Signalling; //EstablishmentCause_mo_Data;
  rrcSetupRequest->rrcSetupRequest.spare.buf = &buf2;
  rrcSetupRequest->rrcSetupRequest.spare.size=1;
  rrcSetupRequest->rrcSetupRequest.spare.bits_unused = 7;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_UL_CCCH_Message, (void *)&ul_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_CCCH_Message,
                                   NULL,
                                   (void *)&ul_ccch_msg,
                                   buffer,
                                   buffer_size);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(NR_RRC,"[UE] RRCSetupRequest Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_NR_RRCReconfigurationComplete_for_nsa(
  uint8_t *buffer,
  size_t buffer_size,
  NR_RRC_TransactionIdentifier_t Transaction_id
)
//------------------------------------------------------------------------------
{
  NR_RRCReconfigurationComplete_t rrc_complete_msg;
  memset(&rrc_complete_msg, 0, sizeof(rrc_complete_msg));
  rrc_complete_msg.rrc_TransactionIdentifier = Transaction_id;
  rrc_complete_msg.criticalExtensions.choice.rrcReconfigurationComplete =
        CALLOC(1, sizeof(*rrc_complete_msg.criticalExtensions.choice.rrcReconfigurationComplete));
  rrc_complete_msg.criticalExtensions.present =
	NR_RRCReconfigurationComplete__criticalExtensions_PR_rrcReconfigurationComplete;
  rrc_complete_msg.criticalExtensions.choice.rrcReconfigurationComplete->nonCriticalExtension = NULL;
  rrc_complete_msg.criticalExtensions.choice.rrcReconfigurationComplete->lateNonCriticalExtension = NULL;
  if (0) {
    xer_fprint(stdout, &asn_DEF_NR_RRCReconfigurationComplete, (void *)&rrc_complete_msg);
  }

  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_RRCReconfigurationComplete,
                                                  NULL,
                                                  (void *)&rrc_complete_msg,
                                                  buffer,
                                                  buffer_size);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_A(NR_RRC, "rrcReconfigurationComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_NR_RRCReconfigurationComplete(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *buffer,
  size_t buffer_size,
  const uint8_t Transaction_id
)
//------------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  NR_UL_DCCH_Message_t ul_dcch_msg;
  NR_RRCReconfigurationComplete_t *rrcReconfigurationComplete;
  memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
  ul_dcch_msg.message.present                     = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1                   = CALLOC(1, sizeof(struct NR_UL_DCCH_MessageType__c1));
  ul_dcch_msg.message.choice.c1->present           = NR_UL_DCCH_MessageType__c1_PR_rrcReconfigurationComplete;
  ul_dcch_msg.message.choice.c1->choice.rrcReconfigurationComplete = CALLOC(1, sizeof(NR_RRCReconfigurationComplete_t));
  rrcReconfigurationComplete            = ul_dcch_msg.message.choice.c1->choice.rrcReconfigurationComplete;
  rrcReconfigurationComplete->rrc_TransactionIdentifier = Transaction_id;
  rrcReconfigurationComplete->criticalExtensions.choice.rrcReconfigurationComplete = CALLOC(1, sizeof(NR_RRCReconfigurationComplete_IEs_t));
  rrcReconfigurationComplete->criticalExtensions.present =
		  NR_RRCReconfigurationComplete__criticalExtensions_PR_rrcReconfigurationComplete;
  rrcReconfigurationComplete->criticalExtensions.choice.rrcReconfigurationComplete->nonCriticalExtension = NULL;
  rrcReconfigurationComplete->criticalExtensions.choice.rrcReconfigurationComplete->lateNonCriticalExtension = NULL;
  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message,
                                   NULL,
                                   (void *)&ul_dcch_msg,
                                   buffer,
                                   buffer_size);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval.failed_type->name, enc_rval.encoded);
  LOG_I(NR_RRC,"rrcReconfigurationComplete Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

uint8_t do_RRCSetupComplete(uint8_t Mod_id, uint8_t *buffer, size_t buffer_size,
                            const uint8_t Transaction_id, uint8_t sel_plmn_id, const int dedicatedInfoNASLength, const char *dedicatedInfoNAS){
  asn_enc_rval_t enc_rval;
  
  NR_UL_DCCH_Message_t  ul_dcch_msg;
  NR_RRCSetupComplete_t *RrcSetupComplete;
  memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));

  uint8_t buf[6];

  ul_dcch_msg.message.present = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1 = CALLOC(1,sizeof(struct NR_UL_DCCH_MessageType__c1));
  ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_rrcSetupComplete;
  ul_dcch_msg.message.choice.c1->choice.rrcSetupComplete = CALLOC(1, sizeof(NR_RRCSetupComplete_t));
  RrcSetupComplete                       = ul_dcch_msg.message.choice.c1->choice.rrcSetupComplete;
  RrcSetupComplete->rrc_TransactionIdentifier    = Transaction_id;
  RrcSetupComplete->criticalExtensions.present   = NR_RRCSetupComplete__criticalExtensions_PR_rrcSetupComplete;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete = CALLOC(1, sizeof(NR_RRCSetupComplete_IEs_t));
  // RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->nonCriticalExtension = CALLOC(1,
  //   sizeof(*RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->nonCriticalExtension));
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->selectedPLMN_Identity = sel_plmn_id;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->registeredAMF = NULL;

  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value = CALLOC(1, sizeof(struct NR_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value));
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->present = NR_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value_PR_ng_5G_S_TMSI;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.size = 6;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf = buf;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf[0] = 0x12;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf[1] = 0x34;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf[2] = 0x56;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf[3] = 0x78;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf[4] = 0x9A;
  RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI.buf[5] = 0xBC;

 memset(&RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->dedicatedNAS_Message,0,sizeof(OCTET_STRING_t));
 OCTET_STRING_fromBuf(&RrcSetupComplete->criticalExtensions.choice.rrcSetupComplete->dedicatedNAS_Message,dedicatedInfoNAS,dedicatedInfoNASLength);
if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
  xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
}

enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message,
                                 NULL,
                                 (void *)&ul_dcch_msg,
                                 buffer,
                                 buffer_size);
AssertFatal(enc_rval.encoded > 0,"ASN1 message encoding failed (%s, %lu)!\n",
    enc_rval.failed_type->name,enc_rval.encoded);
LOG_D(NR_RRC,"RRCSetupComplete Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);

return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t 
do_NR_DLInformationTransfer(
    uint8_t Mod_id,
    uint8_t **buffer,
    uint8_t transaction_id,
    uint32_t pdu_length,
    uint8_t *pdu_buffer
)
//------------------------------------------------------------------------------
{
    ssize_t encoded;
    NR_DL_DCCH_Message_t   dl_dcch_msg;
    memset(&dl_dcch_msg, 0, sizeof(NR_DL_DCCH_Message_t));
    dl_dcch_msg.message.present            = NR_DL_DCCH_MessageType_PR_c1;
    dl_dcch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_DL_DCCH_MessageType__c1));
    dl_dcch_msg.message.choice.c1->present = NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer;

    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer = CALLOC(1, sizeof(NR_DLInformationTransfer_t));
    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer->rrc_TransactionIdentifier = transaction_id;
    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer->criticalExtensions.present =
        NR_DLInformationTransfer__criticalExtensions_PR_dlInformationTransfer;

    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer->
        criticalExtensions.choice.dlInformationTransfer = CALLOC(1, sizeof(NR_DLInformationTransfer_IEs_t));
    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer->
        criticalExtensions.choice.dlInformationTransfer->dedicatedNAS_Message = CALLOC(1, sizeof(NR_DedicatedNAS_Message_t));
    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer->
        criticalExtensions.choice.dlInformationTransfer->dedicatedNAS_Message->buf = pdu_buffer;
    dl_dcch_msg.message.choice.c1->choice.dlInformationTransfer->
        criticalExtensions.choice.dlInformationTransfer->dedicatedNAS_Message->size = pdu_length;

    encoded = uper_encode_to_new_buffer (&asn_DEF_NR_DL_DCCH_Message, NULL, (void *) &dl_dcch_msg, (void **)buffer);
    AssertFatal(encoded > 0,"ASN1 message encoding failed (%s, %ld)!\n",
                "DLInformationTransfer", encoded);
    LOG_D(NR_RRC,"DLInformationTransfer Encoded %zd bytes\n", encoded);
    //for (int i=0;i<encoded;i++) printf("%02x ",(*buffer)[i]);
    return encoded;
}

uint8_t do_NR_ULInformationTransfer(uint8_t **buffer, uint32_t pdu_length, uint8_t *pdu_buffer) {
    ssize_t encoded;
    NR_UL_DCCH_Message_t ul_dcch_msg;
    memset(&ul_dcch_msg, 0, sizeof(NR_UL_DCCH_Message_t));
    ul_dcch_msg.message.present           = NR_UL_DCCH_MessageType_PR_c1;
    ul_dcch_msg.message.choice.c1          = CALLOC(1,sizeof(struct NR_UL_DCCH_MessageType__c1));
    ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_ulInformationTransfer;
    ul_dcch_msg.message.choice.c1->choice.ulInformationTransfer = CALLOC(1,sizeof(struct NR_ULInformationTransfer));
    ul_dcch_msg.message.choice.c1->choice.ulInformationTransfer->criticalExtensions.present = NR_ULInformationTransfer__criticalExtensions_PR_ulInformationTransfer;
    ul_dcch_msg.message.choice.c1->choice.ulInformationTransfer->criticalExtensions.choice.ulInformationTransfer = CALLOC(1,sizeof(struct NR_ULInformationTransfer_IEs));
    struct NR_ULInformationTransfer_IEs *ulInformationTransfer = ul_dcch_msg.message.choice.c1->choice.ulInformationTransfer->criticalExtensions.choice.ulInformationTransfer;
    ulInformationTransfer->dedicatedNAS_Message = CALLOC(1,sizeof(NR_DedicatedNAS_Message_t));
    ulInformationTransfer->dedicatedNAS_Message->buf = pdu_buffer;
    ulInformationTransfer->dedicatedNAS_Message->size = pdu_length;
    ulInformationTransfer->lateNonCriticalExtension = NULL;
    encoded = uper_encode_to_new_buffer (&asn_DEF_NR_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, (void **) buffer);
    AssertFatal(encoded > 0,"ASN1 message encoding failed (%s, %ld)!\n",
                "ULInformationTransfer",encoded);
    LOG_D(NR_RRC,"ULInformationTransfer Encoded %zd bytes\n",encoded);

    return encoded;
}

uint8_t do_RRCReestablishmentRequest(uint8_t Mod_id, uint8_t *buffer, uint16_t c_rnti) {
  asn_enc_rval_t enc_rval;
  NR_UL_CCCH_Message_t ul_ccch_msg;
  NR_RRCReestablishmentRequest_t *rrcReestablishmentRequest;
  uint8_t buf[2];

  memset((void *)&ul_ccch_msg,0,sizeof(NR_UL_CCCH_Message_t));
  ul_ccch_msg.message.present            = NR_UL_CCCH_MessageType_PR_c1;
  ul_ccch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_UL_CCCH_MessageType__c1));
  ul_ccch_msg.message.choice.c1->present = NR_UL_CCCH_MessageType__c1_PR_rrcReestablishmentRequest;
  ul_ccch_msg.message.choice.c1->choice.rrcReestablishmentRequest = CALLOC(1, sizeof(NR_RRCReestablishmentRequest_t));

  rrcReestablishmentRequest = ul_ccch_msg.message.choice.c1->choice.rrcReestablishmentRequest;
  // test
  rrcReestablishmentRequest->rrcReestablishmentRequest.reestablishmentCause = NR_ReestablishmentCause_reconfigurationFailure;
  rrcReestablishmentRequest->rrcReestablishmentRequest.ue_Identity.c_RNTI = c_rnti;
  rrcReestablishmentRequest->rrcReestablishmentRequest.ue_Identity.physCellId = 0;
  rrcReestablishmentRequest->rrcReestablishmentRequest.ue_Identity.shortMAC_I.buf = buf;
  rrcReestablishmentRequest->rrcReestablishmentRequest.ue_Identity.shortMAC_I.buf[0] = 0x08;
  rrcReestablishmentRequest->rrcReestablishmentRequest.ue_Identity.shortMAC_I.buf[1] = 0x32;
  rrcReestablishmentRequest->rrcReestablishmentRequest.ue_Identity.shortMAC_I.size = 2;


  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_UL_CCCH_Message, (void *)&ul_ccch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_CCCH_Message,
                                   NULL,
                                   (void *)&ul_ccch_msg,
                                   buffer,
                                   100);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(NR_RRC,"[UE] RRCReestablishmentRequest Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

//------------------------------------------------------------------------------
uint8_t
do_RRCReestablishment(
const protocol_ctxt_t     *const ctxt_pP,
rrc_gNB_ue_context_t      *const ue_context_pP,
int                              CC_id,
uint8_t                   *const buffer,
size_t                           buffer_size,
//const uint8_t                    transmission_mode,
const uint8_t                    Transaction_id,
NR_SRB_ToAddModList_t               **SRB_configList
) {
    asn_enc_rval_t enc_rval;
    //long *logicalchannelgroup = NULL;
    struct NR_SRB_ToAddMod *SRB1_config = NULL;
    struct NR_SRB_ToAddMod *SRB2_config = NULL;
    //gNB_RRC_INST *nrrrc               = RC.nrrrc[ctxt_pP->module_id];
    NR_DL_DCCH_Message_t dl_dcch_msg;
    NR_RRCReestablishment_t *rrcReestablishment = NULL;
    int i = 0;
    ue_context_pP->ue_context.reestablishment_xid = Transaction_id;
    NR_SRB_ToAddModList_t **SRB_configList2 = NULL;
    SRB_configList2 = &ue_context_pP->ue_context.SRB_configList2[Transaction_id];

    if (*SRB_configList2) {
      free(*SRB_configList2);
    }

    *SRB_configList2 = CALLOC(1, sizeof(NR_SRB_ToAddModList_t));
    memset((void *)&dl_dcch_msg, 0, sizeof(NR_DL_DCCH_Message_t));
    dl_dcch_msg.message.present           = NR_DL_DCCH_MessageType_PR_c1;
    dl_dcch_msg.message.choice.c1 = calloc(1,sizeof(struct NR_DL_DCCH_MessageType__c1));
    dl_dcch_msg.message.choice.c1->present = NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment;
    dl_dcch_msg.message.choice.c1->choice.rrcReestablishment = CALLOC(1,sizeof(NR_RRCReestablishment_t));
    rrcReestablishment = dl_dcch_msg.message.choice.c1->choice.rrcReestablishment;

    // get old configuration of SRB2
    if (*SRB_configList != NULL) {
      for (i = 0; (i < (*SRB_configList)->list.count) && (i < 3); i++) {
        LOG_D(NR_RRC, "(*SRB_configList)->list.array[%d]->srb_Identity=%ld\n",
              i, (*SRB_configList)->list.array[i]->srb_Identity);
    
        if ((*SRB_configList)->list.array[i]->srb_Identity == 2 ) {
          SRB2_config = (*SRB_configList)->list.array[i];
        } else if ((*SRB_configList)->list.array[i]->srb_Identity == 1 ) {
          SRB1_config = (*SRB_configList)->list.array[i];
        }
      }
    }

    if (SRB1_config == NULL) {
      // default SRB1 configuration
      LOG_W(NR_RRC,"SRB1 configuration does not exist in SRB configuration list, use default\n");
      /// SRB1
      SRB1_config = CALLOC(1, sizeof(*SRB1_config));
      SRB1_config->srb_Identity = 1;
    }

    if (SRB2_config == NULL) {
      LOG_W(NR_RRC,"SRB2 configuration does not exist in SRB configuration list\n");
    } else {
      ASN_SEQUENCE_ADD(&(*SRB_configList2)->list, SRB2_config);
    }

    if (*SRB_configList) {
      free(*SRB_configList);
    }

    *SRB_configList = CALLOC(1, sizeof(LTE_SRB_ToAddModList_t));
    ASN_SEQUENCE_ADD(&(*SRB_configList)->list,SRB1_config);

    rrcReestablishment->rrc_TransactionIdentifier = Transaction_id;
    rrcReestablishment->criticalExtensions.present = NR_RRCReestablishment__criticalExtensions_PR_rrcReestablishment;
    rrcReestablishment->criticalExtensions.choice.rrcReestablishment = CALLOC(1,sizeof(NR_RRCReestablishment_IEs_t));

    uint8_t KgNB_star[32] = { 0 };
    /** TODO
    uint16_t pci = nrrrc->carrier[CC_id].physCellId;
    uint32_t earfcn_dl = (uint32_t)freq_to_arfcn10(RC.mac[ctxt_pP->module_id]->common_channels[CC_id].eutra_band,
                         nrrrc->carrier[CC_id].dl_CarrierFreq);
    bool     is_rel8_only = true;
    
    if (earfcn_dl > 65535) {
      is_rel8_only = false;
    }
    LOG_D(NR_RRC, "pci=%d, eutra_band=%d, downlink_frequency=%d, earfcn_dl=%u, is_rel8_only=%s\n",
          pci,
          RC.mac[ctxt_pP->module_id]->common_channels[CC_id].eutra_band,
          nrrrc->carrier[CC_id].dl_CarrierFreq,
          earfcn_dl,
          is_rel8_only == true ? "true": "false");
    */
    
    if (ue_context_pP->ue_context.nh_ncc >= 0) {
      //TODO derive_keNB_star(ue_context_pP->ue_context.nh, pci, earfcn_dl, is_rel8_only, KgNB_star);
      rrcReestablishment->criticalExtensions.choice.rrcReestablishment->nextHopChainingCount = ue_context_pP->ue_context.nh_ncc;
    } else { // first HO
      //TODO derive_keNB_star (ue_context_pP->ue_context.kgnb, pci, earfcn_dl, is_rel8_only, KgNB_star);
      // LG: really 1
      rrcReestablishment->criticalExtensions.choice.rrcReestablishment->nextHopChainingCount = 0;
    }
    // copy KgNB_star to ue_context_pP->ue_context.kgnb
    memcpy (ue_context_pP->ue_context.kgnb, KgNB_star, 32);
    ue_context_pP->ue_context.kgnb_ncc = 0;
    rrcReestablishment->criticalExtensions.choice.rrcReestablishment->lateNonCriticalExtension = NULL;
    rrcReestablishment->criticalExtensions.choice.rrcReestablishment->nonCriticalExtension = NULL;

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message, (void *)&dl_dcch_msg);
    }

    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_DL_DCCH_Message,
                                     NULL,
                                     (void *)&dl_dcch_msg,
                                     buffer,
                                     100);

    if(enc_rval.encoded == -1) {
      LOG_E(NR_RRC, "[gNB AssertFatal]ASN1 message encoding failed (%s, %lu)!\n",
            enc_rval.failed_type->name, enc_rval.encoded);
      return -1;
    }
    
    LOG_D(NR_RRC,"RRCReestablishment Encoded %u bits (%u bytes)\n",
          (uint32_t)enc_rval.encoded, (uint32_t)(enc_rval.encoded+7)/8);
    return((enc_rval.encoded+7)/8);

}

uint8_t 
do_RRCReestablishmentComplete(uint8_t *buffer, size_t buffer_size, int64_t rrc_TransactionIdentifier) {
  asn_enc_rval_t enc_rval;
  NR_UL_DCCH_Message_t ul_dcch_msg;
  NR_RRCReestablishmentComplete_t *rrcReestablishmentComplete;

  memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
  ul_dcch_msg.message.present            = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1          = CALLOC(1, sizeof(struct NR_UL_DCCH_MessageType__c1));
  ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_rrcReestablishmentComplete;
  ul_dcch_msg.message.choice.c1->choice.rrcReestablishmentComplete = CALLOC(1, sizeof(NR_RRCReestablishmentComplete_t));

  rrcReestablishmentComplete = ul_dcch_msg.message.choice.c1->choice.rrcReestablishmentComplete;
  rrcReestablishmentComplete->rrc_TransactionIdentifier = rrc_TransactionIdentifier;
  rrcReestablishmentComplete->criticalExtensions.present = NR_RRCReestablishmentComplete__criticalExtensions_PR_rrcReestablishmentComplete;
  rrcReestablishmentComplete->criticalExtensions.choice.rrcReestablishmentComplete = CALLOC(1, sizeof(NR_RRCReestablishmentComplete_IEs_t));
  rrcReestablishmentComplete->criticalExtensions.choice.rrcReestablishmentComplete->lateNonCriticalExtension = NULL;
  rrcReestablishmentComplete->criticalExtensions.choice.rrcReestablishmentComplete->nonCriticalExtension = NULL;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_UL_CCCH_Message, (void *)&ul_dcch_msg);
  }

  enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message,
                                   NULL,
                                   (void *)&ul_dcch_msg,
                                   buffer,
                                   buffer_size);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
  LOG_D(NR_RRC,"[UE] RRCReestablishmentComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);
  return((enc_rval.encoded+7)/8);
}

