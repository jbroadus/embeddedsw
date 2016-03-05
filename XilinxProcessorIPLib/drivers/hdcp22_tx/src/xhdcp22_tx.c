/******************************************************************************
*
* Copyright (C) 2014 - 2015 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xhdcp22_tx.c
* @addtogroup hdcp22_tx_v1_0
* @{
* @details
*
* This file contains the main implementation of the Xilinx HDCP 2.2 Transmitter
* device driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ------ -------- -------------------------------------------------------
* 1.00  JO     06/24/15 Initial release.
* 1.01  MH     01/15/16 Replaced function XHdcp22Tx_SetDdcHandles with
*                       XHdcp22Tx_SetCallback. Removed test directives.
* 1.02  MG     02/25/16 Added authenticated callback and GetVersion.
* 1.03  MG     02/29/16 Added XHdcp22Cipher_Disable in function XHdcp22Tx_Reset.
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xhdcp22_tx.h"
#include "xhdcp22_tx_i.h"

/************************** Constant Definitions *****************************/

/***************** Macros (Inline Functions) Definitions *********************/

/** Case replacement to copy a case Id to a string with a pre-lead */
#define XHDCP22_TX_CASE_TO_STR_PRE(pre, arg) \
case pre ## arg : strcpy(str, #arg); break;

/**************************** Type Definitions *******************************/
/**
* Pointer to the state handling functions
*
* @param  InstancePtr is a pointer to the XHdcp22_Tx core instance.
*
* @return The next state that should be executed.
*
* @note   None.
*
*/
typedef XHdcp22_Tx_StateType XHdcp22Tx_StateFuncType(XHdcp22_Tx *InstancePtr);

/**
* Pointer to the transition functions going from one state to the next.
*
* @param  InstancePtr is a pointer to the XHdcp22_Tx core instance.
*
* @return None.
*
* @note   None.
*
*/
typedef void XHdcp22_Tx_TransitionFuncType(XHdcp22_Tx *InstancePtr);

/************************** Function Prototypes ******************************/

/* Initialization of peripherals */
static int XHdcp22Tx_InitializeTimer(XHdcp22_Tx *InstancePtr);
static int XHdcp22Tx_InitializeCipher(XHdcp22_Tx *InstancePtr);
static int XHdcp22Tx_InitializeRng(XHdcp22_Tx *InstancePtr);
static int XHdcp22Tx_ComputeBaseAddress(u32 BaseAddress, u32 SubcoreOffset, u32 *SubcoreAddressPtr);

/* Stubs for callbacks */
static int XHdcp22Tx_StubDdc(u8 DeviceAddress, u16 ByteCount, u8* BufferPtr,
                             u8 Stop, void *RefPtr);
static void XHdcp22Tx_StubAuthenticated(void* RefPtr);

/* state handling  functions */
static XHdcp22_Tx_StateType XHdcp22Tx_StateH0(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateH1(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA0(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA1(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_1(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_Nsk0(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_Nsk1(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_Sk0(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA2(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA2_1(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA3(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA4(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA5(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA6(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA7(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA8(XHdcp22_Tx *InstancePtr);
static XHdcp22_Tx_StateType XHdcp22Tx_StateA9(XHdcp22_Tx *InstancePtr);

/* State transition handling functions are used to do initial code before
* entering a new state. This is used in cases that a state is executed
* repeatedly due to the polling mechanism. A state transisition ensures in
* this case that the code is executed only once during authentication.
*/
static void XHdcp22Tx_A1A0(XHdcp22_Tx *InstancePtr);
static void XHdcp22Tx_A1A2(XHdcp22_Tx *InstancePtr);
static void XHdcp22Tx_A2A0(XHdcp22_Tx *InstancePtr);
static void XHdcp22Tx_A3A0(XHdcp22_Tx *InstancePtr);
static void XHdcp22Tx_A4A5(XHdcp22_Tx *InstancePtr);

/* Protocol specific functions */
static int XHdcp22Tx_WriteAKEInit(XHdcp22_Tx *InstancePtr);
static void XHdcp22Tx_WriteAKENoStoredKm(XHdcp22_Tx *InstancePtr,
                    const XHdcp22_Tx_PairingInfo *PairingInfoPtr,
                    const XHdcp22_Tx_CertRx *CertificatePtr);
static void XHdcp22Tx_WriteAKEStoredKm(XHdcp22_Tx *InstancePtr,
                    const XHdcp22_Tx_PairingInfo *PairingInfoPtr);
static void XHdcp22Tx_WriteLcInit(XHdcp22_Tx *InstancePtr, const u8 *Rn);
static void XHdcp22Tx_WriteSkeSendEks(XHdcp22_Tx *InstancePtr,
                    const u8 *EdkeyKsPtr, const u8 *RivPtr);

static int XHdcp22Tx_ReceiveMsg(XHdcp22_Tx *InstancePtr, u8 MessageId,
                    int MessageSize);


/* Generators and functions that return a generated value or a testvector */
static void XHdcp22Tx_GenerateRtx(XHdcp22_Tx *InstancePtr, u8* RtxPtr);
static void XHdcp22Tx_GenerateKm(XHdcp22_Tx *InstancePtr, u8* KmPtr);
static void XHdcp22Tx_GenerateKmMaskingSeed(XHdcp22_Tx *InstancePtr, u8* SeedPtr);
static void XHdcp22Tx_GenerateRn(XHdcp22_Tx *InstancePtr, u8* RnPtr);
static void XHdcp22Tx_GenerateRiv(XHdcp22_Tx *InstancePtr, u8* RivPtr);
static void XHdcp22Tx_GenerateKs(XHdcp22_Tx *InstancePtr, u8* KsPtr);
static const u8* XHdcp22Tx_GetKPubDpc(XHdcp22_Tx *InstancePtr);

/* Cryptographic functions */
static XHdcp22_Tx_PairingInfo *XHdcp22Tx_GetPairingInfo(const u8 *ReceiverId);
static void XHdcp22Tx_InvalidatePairingInfo(const u8* ReceiverId);
static XHdcp22_Tx_PairingInfo *XHdcp22Tx_UpdatePairingInfo(
                              const XHdcp22_Tx_PairingInfo *PairingInfo);

/* Timer functions */
static void XHdcp22Tx_TimerHandler(void *CallbackRef, u8 TmrCntNumber);
static int XHdcp22Tx_StartTimer(XHdcp22_Tx *InstancePtr, u32 TimeOut_mSec,
                                u8 ReasonId);
static int XHdcp22Tx_WaitForReceiver(XHdcp22_Tx *InstancePtr, int ExpectedSize);
static u32 XHdcp22Tx_GetTimerCount(XHdcp22_Tx *InstancePtr);

/* RxStatus handling */
static void XHdcp22Tx_ReadRxStatus(XHdcp22_Tx *InstancePtr);
static u8 XHdcp22_Tx_RxStatusMutex = FALSE;

/************************** Variable Definitions *****************************/

/**
* This table contains the function pointers for all possible states.
* The order of elements must match the #XHdcp22_Tx_StateType enumerator definitions.
*/
XHdcp22Tx_StateFuncType* const XHdcp22_Tx_StateTable[XHDCP22_TX_NUM_STATES] =
{
	XHdcp22Tx_StateH0, XHdcp22Tx_StateH1, XHdcp22Tx_StateA0,
	XHdcp22Tx_StateA1, Xhdcp22Tx_StateA1_1,
	Xhdcp22Tx_StateA1_Nsk0, Xhdcp22Tx_StateA1_Nsk1, Xhdcp22Tx_StateA1_Sk0,
	XHdcp22Tx_StateA2, XHdcp22Tx_StateA2_1,
	XHdcp22Tx_StateA3, XHdcp22Tx_StateA4, XHdcp22Tx_StateA5,
	XHdcp22Tx_StateA6, XHdcp22Tx_StateA7, XHdcp22Tx_StateA8, XHdcp22Tx_StateA9
};

/**
* This table is a matrix that contains the function pointers for functions to execute on
* state transition.
* It is filled dynamically.
* A NULL value is used if no transition is required.
*/
static XHdcp22_Tx_TransitionFuncType * transition_table[XHDCP22_TX_NUM_STATES][XHDCP22_TX_NUM_STATES];

/* public transmitter DCP LLC key - n=384 bytes, e=1 byte */
static const u8 XHdcp22_Tx_KpubDcp[XHDCP22_TX_KPUB_DCP_LLC_N_SIZE + XHDCP22_TX_KPUB_DCP_LLC_E_SIZE] = {
	0xB0, 0xE9, 0xAA, 0x45, 0xF1, 0x29, 0xBA, 0x0A, 0x1C, 0xBE, 0x17, 0x57, 0x28, 0xEB, 0x2B, 0x4E,
	0x8F, 0xD0, 0xC0, 0x6A, 0xAD, 0x79, 0x98, 0x0F, 0x8D, 0x43, 0x8D, 0x47, 0x04, 0xB8, 0x2B, 0xF4,
	0x15, 0x21, 0x56, 0x19, 0x01, 0x40, 0x01, 0x3B, 0xD0, 0x91, 0x90, 0x62, 0x9E, 0x89, 0xC2, 0x27,
	0x8E, 0xCF, 0xB6, 0xDB, 0xCE, 0x3F, 0x72, 0x10, 0x50, 0x93, 0x8C, 0x23, 0x29, 0x83, 0x7B, 0x80,
	0x64, 0xA7, 0x59, 0xE8, 0x61, 0x67, 0x4C, 0xBC, 0xD8, 0x58, 0xB8, 0xF1, 0xD4, 0xF8, 0x2C, 0x37,
	0x98, 0x16, 0x26, 0x0E, 0x4E, 0xF9, 0x4E, 0xEE, 0x24, 0xDE, 0xCC, 0xD1, 0x4B, 0x4B, 0xC5, 0x06,
	0x7A, 0xFB, 0x49, 0x65, 0xE6, 0xC0, 0x00, 0x83, 0x48, 0x1E, 0x8E, 0x42, 0x2A, 0x53, 0xA0, 0xF5,
	0x37, 0x29, 0x2B, 0x5A, 0xF9, 0x73, 0xC5, 0x9A, 0xA1, 0xB5, 0xB5, 0x74, 0x7C, 0x06, 0xDC, 0x7B,
	0x7C, 0xDC, 0x6C, 0x6E, 0x82, 0x6B, 0x49, 0x88, 0xD4, 0x1B, 0x25, 0xE0, 0xEE, 0xD1, 0x79, 0xBD,
	0x39, 0x85, 0xFA, 0x4F, 0x25, 0xEC, 0x70, 0x19, 0x23, 0xC1, 0xB9, 0xA6, 0xD9, 0x7E, 0x3E, 0xDA,
	0x48, 0xA9, 0x58, 0xE3, 0x18, 0x14, 0x1E, 0x9F, 0x30, 0x7F, 0x4C, 0xA8, 0xAE, 0x53, 0x22, 0x66,
	0x2B, 0xBE, 0x24, 0xCB, 0x47, 0x66, 0xFC, 0x83, 0xCF, 0x5C, 0x2D, 0x1E, 0x3A, 0xAB, 0xAB, 0x06,
	0xBE, 0x05, 0xAA, 0x1A, 0x9B, 0x2D, 0xB7, 0xA6, 0x54, 0xF3, 0x63, 0x2B, 0x97, 0xBF, 0x93, 0xBE,
	0xC1, 0xAF, 0x21, 0x39, 0x49, 0x0C, 0xE9, 0x31, 0x90, 0xCC, 0xC2, 0xBB, 0x3C, 0x02, 0xC4, 0xE2,
	0xBD, 0xBD, 0x2F, 0x84, 0x63, 0x9B, 0xD2, 0xDD, 0x78, 0x3E, 0x90, 0xC6, 0xC5, 0xAC, 0x16, 0x77,
	0x2E, 0x69, 0x6C, 0x77, 0xFD, 0xED, 0x8A, 0x4D, 0x6A, 0x8C, 0xA3, 0xA9, 0x25, 0x6C, 0x21, 0xFD,
	0xB2, 0x94, 0x0C, 0x84, 0xAA, 0x07, 0x29, 0x26, 0x46, 0xF7, 0x9B, 0x3A, 0x19, 0x87, 0xE0, 0x9F,
	0xEB, 0x30, 0xA8, 0xF5, 0x64, 0xEB, 0x07, 0xF1, 0xE9, 0xDB, 0xF9, 0xAF, 0x2C, 0x8B, 0x69, 0x7E,
	0x2E, 0x67, 0x39, 0x3F, 0xF3, 0xA6, 0xE5, 0xCD, 0xDA, 0x24, 0x9B, 0xA2, 0x78, 0x72, 0xF0, 0xA2,
	0x27, 0xC3, 0xE0, 0x25, 0xB4, 0xA1, 0x04, 0x6A, 0x59, 0x80, 0x27, 0xB5, 0xDA, 0xB4, 0xB4, 0x53,
	0x97, 0x3B, 0x28, 0x99, 0xAC, 0xF4, 0x96, 0x27, 0x0F, 0x7F, 0x30, 0x0C, 0x4A, 0xAF, 0xCB, 0x9E,
	0xD8, 0x71, 0x28, 0x24, 0x3E, 0xBC, 0x35, 0x15, 0xBE, 0x13, 0xEB, 0xAF, 0x43, 0x01, 0xBD, 0x61,
	0x24, 0x54, 0x34, 0x9F, 0x73, 0x3E, 0xB5, 0x10, 0x9F, 0xC9, 0xFC, 0x80, 0xE8, 0x4D, 0xE3, 0x32,
	0x96, 0x8F, 0x88, 0x10, 0x23, 0x25, 0xF3, 0xD3, 0x3E, 0x6E, 0x6D, 0xBB, 0xDC, 0x29, 0x66, 0xEB,
	0x03
};


/**
* This array contains the capabilities of the HDCP22 TX core, and is transmitted during
* authentication as part of the AKE_Init message.
*/
static const u8 XHdcp22_Tx_TxCaps[] = { 0x02, 0x00, 0x00 };

/**
* This array contains the pairing info keys that are used for authentication
* with stored Km. It uses a mechanism that tries to find an empty entry for adding
* a new one.
*
* - Optionally move to non-volatile secure storage to have persistent stored Km
*   authentication even after power-up.
* - It is not required by the protocol, but InvalidatePairingInfo()could be
*   used if a failure occurs in stored Km authentication, so automatically
*   the non-stored Km authentication is executed on next authentication.
* - If no empty slots, and the array is full, the first entry is returned
*   for pairing info storage.
*
*/
XHdcp22_Tx_PairingInfo XHdcp22_Tx_PairingInfoStore[XHDCP22_TX_MAX_STORED_PAIRINGINFO];

/************************** Function Definitions *****************************/
/*****************************************************************************/
/**
*
* This function initializes the HDCP22 TX core. This function must be called
* prior to using the HDCP TX core. Initialization of the HDCP TX includes
* setting up the instance data and ensuring the hardware is in a quiescent
* state.
*
* @param InstancePtr is a pointer to the XHdcp22_Tx core instance.
* @param CfgPtr points to the configuration structure associated with
*        the HDCP TX core.
* @param EffectiveAddr is the base address of the device. If address
*        translation is being used, then this parameter must reflect the
*        virtual base address. Otherwise, the physical address should be
*        used.
*
* @return
*    - XST_SUCCESS Initialization was successful.
*    - XST_FAILURE Initialization of the internal timer failed or there was a
*               HDCP TX PIO ID mismatch.
* @note   None.
*
******************************************************************************/
int XHdcp22Tx_CfgInitialize(XHdcp22_Tx *InstancePtr, XHdcp22_Tx_Config *CfgPtr,
                            u32 EffectiveAddr)
{
	int Result = XST_SUCCESS;

	/* Verify arguments. */
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CfgPtr != NULL);
	Xil_AssertNonvoid(EffectiveAddr != (u32)0x0);

	/* Setup the instance */
	(void)memset((void *)InstancePtr, 0, sizeof(XHdcp22_Tx));

	/* copy configuration settings */
	(void)memcpy((void *)&(InstancePtr->Config), (const void *)CfgPtr,
				 sizeof(XHdcp22_Tx_Config));

	InstancePtr->Config.BaseAddress = EffectiveAddr;

	/* Set all handlers to stub values, let user configure this data later */
	InstancePtr->DdcRead = XHdcp22Tx_StubDdc;
	InstancePtr->IsDdcReadSet = (FALSE);
	InstancePtr->DdcWrite = XHdcp22Tx_StubDdc;
	InstancePtr->IsDdcWriteSet = (FALSE);
    InstancePtr->AuthenticatedCallback = XHdcp22Tx_StubAuthenticated;
	InstancePtr->IsAuthenticatedCallbackSet = (FALSE);

	InstancePtr->Info.Protocol = XHDCP22_TX_HDMI;

	/* Initialize global parameters */
	InstancePtr->Info.IsEncryptionEnabled = (FALSE);
	InstancePtr->Info.IsReceiverHDCP2Capable = (FALSE);

	/* Initialize statemachine, but do not run it */
	/* Dynamically setup the transition table */
	memset(transition_table, 0x00, sizeof(transition_table));
	transition_table[XHDCP22_TX_STATE_A1][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A1A0;
	transition_table[XHDCP22_TX_STATE_A1_1][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A1A0;
	transition_table[XHDCP22_TX_STATE_A1_NSK0][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A1A0;
	transition_table[XHDCP22_TX_STATE_A1_NSK1][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A1A0;
	transition_table[XHDCP22_TX_STATE_A1_SK0][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A1A0;

	transition_table[XHDCP22_TX_STATE_A1_NSK1][XHDCP22_TX_STATE_A2] = XHdcp22Tx_A1A2;
	transition_table[XHDCP22_TX_STATE_A1_SK0][XHDCP22_TX_STATE_A2] = XHdcp22Tx_A1A2;

	transition_table[XHDCP22_TX_STATE_A2][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A2A0;
	transition_table[XHDCP22_TX_STATE_A2_1][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A2A0;
	transition_table[XHDCP22_TX_STATE_A3][XHDCP22_TX_STATE_A0] = XHdcp22Tx_A3A0;
	transition_table[XHDCP22_TX_STATE_A4][XHDCP22_TX_STATE_A5] = XHdcp22Tx_A4A5;

	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_UNAUTHENTICATED;
	InstancePtr->Info.CurrentState = XHDCP22_TX_STATE_H0;
	InstancePtr->Info.PrvState = XHDCP22_TX_STATE_H0;
	InstancePtr->Info.IsEnabled = (FALSE);
	InstancePtr->Info.StateContext = NULL;
	InstancePtr->Info.MsgAvailable = (FALSE);
	InstancePtr->Info.PollingValue = XHDCP22_TX_DEFAULT_RX_STATUS_POLLVALUE;

	/* Timer configuration */
	InstancePtr->Timer.TimerExpired = (TRUE);
	InstancePtr->Timer.ReasonId = XHDCP22_TX_TS_UNDEFINED;
	InstancePtr->Timer.InitialTicks = 0;

	/*
	* For the time being, we invalidate the storage here, but when
	* secure storage is available, secure storage should be formatted on
	* very first usage only, not on every initialization of this component.
	*/
	XHdcp22Tx_ClearPairingInfo(InstancePtr);

	/* Initialize hardware timer */
	Result = XHdcp22Tx_InitializeTimer(InstancePtr);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	/* Initialize random number generator */
	Result = XHdcp22Tx_InitializeRng(InstancePtr);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	/* Initialize random number generator */
	Result = XHdcp22Tx_InitializeCipher(InstancePtr);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	/* Indicate the instance is now ready to use, initialized without error */
	InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

	XHdcp22Tx_LogReset(InstancePtr, FALSE);

	return (XST_SUCCESS);
}

/*****************************************************************************/
/**
*
* This function is a called to initialize the hardware timer.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*    - XST_SUCCESS if authenticated is started successfully.
*    - XST_FAIL if the state machine is disabled.
*
* @note  None.
*
******************************************************************************/
static int XHdcp22Tx_InitializeTimer(XHdcp22_Tx *InstancePtr)
{
	/* Verify arguments */
	Xil_AssertNonvoid(InstancePtr != NULL);

	u32 SubcoreBaseAddr;
	XTmrCtr_Config *TimerConfigPtr;

	int Result = XST_SUCCESS;

	TimerConfigPtr = XTmrCtr_LookupConfig(InstancePtr->Config.TimerDeviceId);
	if(TimerConfigPtr == NULL) {
		return XST_FAILURE;
	}

	Result = XHdcp22Tx_ComputeBaseAddress(InstancePtr->Config.BaseAddress, TimerConfigPtr->BaseAddress, &SubcoreBaseAddr);
	XTmrCtr_CfgInitialize(&InstancePtr->Timer.TmrCtr, TimerConfigPtr, SubcoreBaseAddr);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	XTmrCtr_SetOptions(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0,
	                   XTC_INT_MODE_OPTION | XTC_DOWN_COUNT_OPTION);
	XTmrCtr_SetOptions(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_1,
	                   XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetHandler(&InstancePtr->Timer.TmrCtr, XHdcp22Tx_TimerHandler,
	                   InstancePtr);
	return Result;
}

/*****************************************************************************/
/**
*
* This function is a called to initialize the cipher.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*    - XST_SUCCESS if authenticated is started successfully.
*    - XST_FAIL if the state machine is disabled.
*
* @note  None.
*
******************************************************************************/
static int XHdcp22Tx_InitializeCipher(XHdcp22_Tx *InstancePtr)
{
	/* Verify arguments */
	Xil_AssertNonvoid(InstancePtr != NULL);

	int Result = XST_SUCCESS;

	XHdcp22_Cipher_Config *ConfigPtr = NULL;
	u32 SubcoreBaseAddr;

	ConfigPtr = XHdcp22Cipher_LookupConfig(InstancePtr->Config.CipherId);
	if (ConfigPtr == NULL) {
		return XST_DEVICE_NOT_FOUND;
	}

	Result = XHdcp22Tx_ComputeBaseAddress(InstancePtr->Config.BaseAddress, ConfigPtr->BaseAddress, &SubcoreBaseAddr);
	Result |= XHdcp22Cipher_CfgInitialize(&InstancePtr->Cipher, ConfigPtr, SubcoreBaseAddr);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	/* Set cipher to TX mode */
	XHdcp22Cipher_SetTxMode(&InstancePtr->Cipher);

	return Result;
}

/*****************************************************************************/
/**
*
* This function is a called to initialize the hardware random generator.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*    - XST_SUCCESS if authenticated is started successfully.
*    - XST_FAIL if the state machine is disabled.
*
* @note  None.
*
******************************************************************************/
static int XHdcp22Tx_InitializeRng(XHdcp22_Tx *InstancePtr)
{
	/* Verify arguments */
	Xil_AssertNonvoid(InstancePtr != NULL);

	int Result = XST_SUCCESS;
	u32 SubcoreBaseAddr;
	XHdcp22_Rng_Config *ConfigPtr = NULL;

	ConfigPtr = XHdcp22Rng_LookupConfig(InstancePtr->Config.RngId);
	if (ConfigPtr == NULL) {
		return XST_DEVICE_NOT_FOUND;
	}

	Result = XHdcp22Tx_ComputeBaseAddress(InstancePtr->Config.BaseAddress, ConfigPtr->BaseAddress, &SubcoreBaseAddr);
	Result |= XHdcp22Rng_CfgInitialize(&InstancePtr->Rng, ConfigPtr, SubcoreBaseAddr);
	if (Result != XST_SUCCESS) {
		return Result;
	}
	XHdcp22Rng_Enable(&InstancePtr->Rng);

	return Result;
}

/*****************************************************************************/
/**
*
* This function is used to load the Lc128 value by copying the contents
* of the array referenced by Lc128Ptr into the cipher.
*
* @param	InstancePtr is a pointer to the XHdcp22_Tx core instance.
* @param	Lc128Ptr is a pointer to an array.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XHdcp22Tx_LoadLc128(XHdcp22_Tx *InstancePtr, const u8 *Lc128Ptr)
{
	/* Verify arguments */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(Lc128Ptr != NULL);
	XHdcp22Cipher_SetLc128(&InstancePtr->Cipher, Lc128Ptr,
	                       XHDCP22_TX_LC128_SIZE);
}

/*****************************************************************************/
/**
*
* This function reads the version.
*
* @param    InstancePtr is a pointer to the XHdcp22_Tx core instance.
*
* @return   Returns the version register of the cipher.
*
* @note     None.
*
******************************************************************************/
u32 XHdcp22Tx_GetVersion(XHdcp22_Tx *InstancePtr)
{
	/* Verify arguments */
	Xil_AssertNonvoid(InstancePtr);

	return XHdcp22Cipher_GetVersion(&InstancePtr->Cipher);
}

/*****************************************************************************/
/**
*
* This function computes the subcore absolute address on axi-lite interface
* Subsystem is mapped at an absolute address and all included sub-cores are
* at pre-defined offset from the subsystem base address. To access the subcore
* register map from host CPU an absolute address is required.
*
* @param  BaseAddress is the base address of the subsystem instance
* @param  SubcoreOffset is the offset of the the subcore instance
* @param  SubcoreAddressPtr is the computed base address of the subcore instance
*
* @return XST_SUCCESS if base address computation is successful and within
*         subsystem address range else XST_FAILURE
*
******************************************************************************/
static int XHdcp22Tx_ComputeBaseAddress(u32 BaseAddress, u32 SubcoreOffset, u32 *SubcoreAddressPtr)
{
	int Status;
	u32 Address;

	Address = BaseAddress | SubcoreOffset;
	if((Address >= BaseAddress))
	{
		*SubcoreAddressPtr = Address;
		Status = XST_SUCCESS;
	}
	else
	{
		*SubcoreAddressPtr = 0;
		Status = XST_FAILURE;
	}

	return (Status);
}

/*****************************************************************************/
/**
*
* This function is a called to start authentication.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*    - XST_SUCCESS if authenticated is started successfully.
*    - XST_FAIL if the state machine is disabled.
*
* @note  None.
*
******************************************************************************/
int XHdcp22Tx_Authenticate(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr);

	/* return a failure if not enabled */
	if (InstancePtr->Info.IsEnabled == (FALSE))
		return XST_FAILURE;

	/* state H1 checks this one */
	InstancePtr->Info.IsReceiverHDCP2Capable = (FALSE);

	/* initialize statemachine, and set to busy status */
	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_AUTHENTICATION_BUSY;
	InstancePtr->Info.CurrentState = XHDCP22_TX_STATE_H0;
	InstancePtr->Info.PrvState = XHDCP22_TX_STATE_H0;

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is a executed every time to trigger the state machine.
* The user of HDCP22 TX is responsible to call this function on a regular base.
*
* @param   InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return  Enumerated authentication status defined in
*          #XHdcp22_Tx_AuthenticationType.
*
* @note    None.
*
******************************************************************************/
XHdcp22_Tx_AuthenticationType XHdcp22Tx_Poll(XHdcp22_Tx *InstancePtr)
{
	XHdcp22_Tx_StateType NewState;
	XHdcp22_Tx_TransitionFuncType *Transition;
	XHdcp22_Tx_AuthenticationType PrvAuthenticationStatus;

	Xil_AssertNonvoid(InstancePtr != NULL);

	/* return immediately if not enabled */
	if (InstancePtr->Info.IsEnabled == (FALSE))
		return InstancePtr->Info.AuthenticationStatus;

	/* Store the authentication status before executing the next state */
	PrvAuthenticationStatus = InstancePtr->Info.AuthenticationStatus;

	/* continue executing the statemachine */
	NewState = XHdcp22_Tx_StateTable[InstancePtr->Info.CurrentState](InstancePtr);
	Transition = transition_table[InstancePtr->Info.CurrentState][NewState];

	if (Transition != NULL)
		Transition(InstancePtr);

	InstancePtr->Info.PrvState = InstancePtr->Info.CurrentState;
	InstancePtr->Info.CurrentState = NewState;

	/* Log only if the AuthenticationStatus status changes and do not log
	 * XHDCP22_TX_AUTHENTICATION_BUSY to avoid polluting the log buffer
	 */
	if(PrvAuthenticationStatus != InstancePtr->Info.AuthenticationStatus &&
	   InstancePtr->Info.AuthenticationStatus != XHDCP22_TX_AUTHENTICATION_BUSY) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_POLL_RESULT,
		                (u8)InstancePtr->Info.AuthenticationStatus);

		// Authenticated callback
		if ( (PrvAuthenticationStatus != XHDCP22_TX_AUTHENTICATED)
			&& (InstancePtr->Info.AuthenticationStatus == XHDCP22_TX_AUTHENTICATED)
			&& InstancePtr->IsAuthenticatedCallbackSet) {
			InstancePtr->AuthenticatedCallback(InstancePtr->AuthenticatedCallbackRef);
		}
	}

	/* Enable only if the attached receiver is authenticated. */
	if (InstancePtr->Info.AuthenticationStatus == XHDCP22_TX_AUTHENTICATED) {
		XHdcp22Cipher_Enable(&InstancePtr->Cipher);
	} else {
		XHdcp22Cipher_Disable(&InstancePtr->Cipher);
	}

	return InstancePtr->Info.AuthenticationStatus;
}

/*****************************************************************************/
/**
*
* This function resets the state machine.
*
* @param   InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return  XST_SUCCESS
*
* @note    None.
*
******************************************************************************/
int XHdcp22Tx_Reset(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr != NULL);

	/* initialize statemachine, but do not run it */
	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_UNAUTHENTICATED;
	InstancePtr->Info.CurrentState = XHDCP22_TX_STATE_H0;
	InstancePtr->Info.PrvState = XHDCP22_TX_STATE_H0;

	/* Stop the timer if it's still running */
	XTmrCtr_Stop(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0);

	/* Disable the Cipher */
	XHdcp22Cipher_Disable(InstancePtr);

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_RESET, 0);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function enables the state machine and acts as a resume.
*
* @param   InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return  XST_SUCCESS
*
* @note    None.
*
******************************************************************************/
int XHdcp22Tx_Enable(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr != NULL);
	InstancePtr->Info.IsEnabled = (TRUE);
	XTmrCtr_Stop(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_ENABLED, 1);
	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function disables the state machine and acts as a pause.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return XST_SUCCESS.
*
* @note   None.
*
******************************************************************************/
int XHdcp22Tx_Disable(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr != NULL);
	InstancePtr->Info.IsEnabled = (FALSE);

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_ENABLED, 0);
	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function enables HDMI stream encryption by enabling the cipher.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return - XST_SUCCESS if authenticated is started successfully.
*         - XST_FAIL if the state machine is disabled.
*
* @note   None.
*
******************************************************************************/
int XHdcp22Tx_EnableEncryption(XHdcp22_Tx *InstancePtr)
{

	Xil_AssertNonvoid(InstancePtr != NULL);

	if (XHdcp22Tx_IsAuthenticated(InstancePtr)) {

		XHdcp22Cipher_EnableTxEncryption(&InstancePtr->Cipher);

		InstancePtr->Info.IsEncryptionEnabled = (TRUE);
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_ENCR_ENABLED, 1);
		return XST_SUCCESS;
  }

  return XST_FAILURE;
}

/*****************************************************************************/
/**
*
* This function disables HDMI stream encryption by disabling the cipher.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return XST_SUCCESS.
*
* @note   None.
*
******************************************************************************/
int XHdcp22Tx_DisableEncryption(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr != NULL);

	XHdcp22Cipher_DisableTxEncryption(&InstancePtr->Cipher);

	InstancePtr->Info.IsEncryptionEnabled = (FALSE);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_ENCR_ENABLED, 0);
	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function returns the current enabled state.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return TRUE if enabled otherwise FALSE.
*
* @note   None.
*
******************************************************************************/
u8 XHdcp22Tx_IsEnabled (XHdcp22_Tx *InstancePtr)
{
	return InstancePtr->Info.IsEnabled;
}

/*****************************************************************************/
/**
*
* This function returns the current encryption enabled state.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return TRUE if encryption is enabled otherwise FALSE.
*
* @note   None.
*
******************************************************************************/
u8 XHdcp22Tx_IsEncryptionEnabled (XHdcp22_Tx *InstancePtr)
{
  return InstancePtr->Info.IsEncryptionEnabled;
}

/*****************************************************************************/
/**
*
* This function returns the current progress state.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return TRUE if authentication is in progress otherwise FALSE.
*
* @note   None.
*
******************************************************************************/
u8 XHdcp22Tx_IsInProgress (XHdcp22_Tx *InstancePtr)
{
	return (InstancePtr->Info.AuthenticationStatus ==
	       XHDCP22_TX_AUTHENTICATION_BUSY) ? (TRUE) : (FALSE);
}

/*****************************************************************************/
/**
*
* This function returns the current authenticated state.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return TRUE if authenticated otherwise FALSE.
*
* @note   None.
*
******************************************************************************/
u8 XHdcp22Tx_IsAuthenticated (XHdcp22_Tx *InstancePtr)
{
	return (InstancePtr->Info.AuthenticationStatus ==
	                    XHDCP22_TX_AUTHENTICATED) ? (TRUE) : (FALSE);
}

/*****************************************************************************/
/**
*
* This function installs callback functions for the given
* HandlerType:
*
* <pre>
* HandlerType                            Callback Function Type
* -------------------------              ---------------------------
* (XHDCP22_TX_HANDLER_DDC_WRITE)         DdcWrite
* (XHDCP22_TX_HANDLER_DDC_READ)          DdcRead
* (XHDCP22_TX_HANDLER_AUTHENTICATED)     AuthenticatedCallback
* </pre>
*
* @param	InstancePtr is a pointer to the HDMI RX core instance.
* @param	HandlerType specifies the type of handler.
* @param	CallbackFunc is the address of the callback function.
* @param	CallbackRef is a user data item that will be passed to the
*			callback function when it is invoked.
*
* @return
*			- XST_SUCCESS if callback function installed successfully.
*			- XST_INVALID_PARAM when HandlerType is invalid.
*
* @note		Invoking this function for a handler that already has been
*			installed replaces it with the new handler.
*
******************************************************************************/
int XHdcp22Tx_SetCallback(XHdcp22_Tx *InstancePtr, XHdcp22_Tx_HandlerType HandlerType, void *CallbackFunc, void *CallbackRef)
{
	/* Verify arguments. */
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(HandlerType > (XHDCP22_TX_HANDLER_UNDEFINED));
	Xil_AssertNonvoid(HandlerType < (XHDCP22_TX_HANDLER_INVALID));
	Xil_AssertNonvoid(CallbackFunc != NULL);
	Xil_AssertNonvoid(CallbackRef != NULL);

	u32 Status;

	/* Check for handler type */
	switch (HandlerType)
	{
		// DDC write
		case (XHDCP22_TX_HANDLER_DDC_WRITE):
			InstancePtr->DdcWrite = (XHdcp22_Tx_DdcHandler)CallbackFunc;
			InstancePtr->DdcHandlerRef = CallbackRef;
			InstancePtr->IsDdcWriteSet = (TRUE);
			Status = (XST_SUCCESS);
			break;

		// DDC read
		case (XHDCP22_TX_HANDLER_DDC_READ):
			InstancePtr->DdcRead = (XHdcp22_Tx_DdcHandler)CallbackFunc;
			InstancePtr->DdcHandlerRef = CallbackRef;
			InstancePtr->IsDdcReadSet = (TRUE);
			Status = (XST_SUCCESS);
			break;

		// Authenticated
		case (XHDCP22_TX_HANDLER_AUTHENTICATED):
			InstancePtr->AuthenticatedCallback = (XHdcp22_Tx_Callback)CallbackFunc;
			InstancePtr->AuthenticatedCallbackRef = CallbackRef;
			InstancePtr->IsAuthenticatedCallbackSet = (TRUE);
			Status = (XST_SUCCESS);
			break;

		default:
			Status = (XST_INVALID_PARAM);
			break;
	}

	return Status;
}

/*****************************************************************************/
/**
*
* This function returns the pointer to the internal timer control instance
* needed for connecting the timer interrupt to an interrupt controller.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return A pointer to the internal timer control instance.
*
* @note   None.
*
******************************************************************************/
XTmrCtr* XHdcp22Tx_GetTimer(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr != NULL);
	return &InstancePtr->Timer.TmrCtr;
}

/*****************************************************************************/
/**
*
* This function is the implementation of the HDCP transmit H0 state.
* According protocol this is the reset state that is entered initially.
* As soon as hot plug is detected and Rx is available, a transisition to state h1 is
* performed. Since hotplug detect is controlled by the user,
* the next state is always state H1.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return XHDCP22_TX_STATE_H1
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateH0(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE, (u16)XHDCP22_TX_STATE_H0);

	return XHDCP22_TX_STATE_H1;
}

/*****************************************************************************/
/**
*
* This function is the implementation of the HDCP transmit H1 state.
* As soon as hot plug is detected and Rx is available, a transisition to
* this state is performed. Hotplug detection if controlled by the user of this core.
* This means that this state is practically the entry state.
*
* @param  InstancePtr is a pointer to the XHDCP22 TX core instance.
*
* @return
*         - XHDCP22_TX_STATE_A0 if the attached receiver is HDCP2 capable.
*         - XHDCP22_TX_STATE_H1 if the attached receiver is NOT HDCP2 capable.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateH1(XHdcp22_Tx *InstancePtr)
{
	u8 DdcBuf[1];
	int Status = XST_SUCCESS;

	/* Avoid polluting */
	if (InstancePtr->Info.PrvState != InstancePtr->Info.CurrentState) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE, (u16)XHDCP22_TX_STATE_H1);
	}

	/* Stay in this state until XHdcp22Tx_Authenticate is called and the status
	 * is set to XHDCP22_TX_AUTHENTICATION_BUSY */
	if (InstancePtr->Info.AuthenticationStatus != XHDCP22_TX_AUTHENTICATION_BUSY) {
		return XHDCP22_TX_STATE_H1;
	}

	/* HDCP2Version */
	InstancePtr->IsReceiverHDCP2Capable = (FALSE);
	DdcBuf[0] = XHDCP22_TX_HDCPPORT_VERSION_OFFSET;
	Status = InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS, 1, DdcBuf,
	                              (FALSE), InstancePtr->DdcHandlerRef);
	if (Status == (XST_SUCCESS)) {
		Status = InstancePtr->DdcRead(XHDCP22_TX_DDC_BASE_ADDRESS,
		                              sizeof(DdcBuf), DdcBuf, (TRUE),
		                              InstancePtr->DdcHandlerRef);
	}

	if (Status == (XST_SUCCESS)) {
		if ((DdcBuf[0] & 0x04) == 0x04) {
			InstancePtr->IsReceiverHDCP2Capable = (TRUE);
			return XHDCP22_TX_STATE_A0;
		}
	}

	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_INCOMPATIBLE_RX;
	return XHDCP22_TX_STATE_H1;
}

/*****************************************************************************/
/**
*
* This function is the implementation of the HDCP transmit A0 state.
* If content protection not desired, return to H1 state.
*
* @param  InstancePtr is a pointer to the XHDCP22 TX core instance.
*
* @return
*         - XHDCP22_TX_STATE_H1 if state machine is set to disabled (content protection
*           NOT desired).
*         - XHDCP22_TX_STATE_A1 if authentication must start.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA0(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE, (u16)XHDCP22_TX_STATE_A0);

	/* Content protection not desired; go back to H1 state */
	if (InstancePtr->Info.IsEnabled == (FALSE))
		return XHDCP22_TX_STATE_H1;

	/* Authentication starts, set status as BUSY */
	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_AUTHENTICATION_BUSY;

	/* Before doing any actions, make sure to initialize the timer */
	InstancePtr->Timer.TimerExpired = (TRUE);
	InstancePtr->Timer.ReasonId = XHDCP22_TX_TS_UNDEFINED;
	InstancePtr->Info.MsgAvailable = (FALSE);
	return XHDCP22_TX_STATE_A1;
}

/*****************************************************************************/
/**
*
* This function executes the first part of the A1 state of the
* HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return XHDCP22_TX_STATE_A1_1
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA1(XHdcp22_Tx *InstancePtr)
{
	int Result = XST_SUCCESS;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE, (u16)XHDCP22_TX_STATE_A1);

	Result = XHdcp22Tx_WriteAKEInit(InstancePtr);
	if (Result != XST_SUCCESS) {
		return XHDCP22_TX_STATE_A0;
	}

	/* Start the timer for receiving XHDCP22_TX_AKE_SEND_CERT */
	XHdcp22Tx_StartTimer(InstancePtr, 100, XHDCP22_TX_AKE_SEND_CERT);

	/* Goto the waiting state for AKE_SEND_CERT */
	return XHDCP22_TX_STATE_A1_1;
}

/******************************************************************************/
/**
*
* This function executes a part of the A1 state functionality of the HDCP2.2
* TX protocol.
* It receives the certificate and decides if it should continue to a
* 'no stored km' or 'stored Km'intermediate state.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*         - XHDCP22_TX_STATE_A0 on errors.
*         - XHDCP22_TX_STATE_A1_1 if busy waiting for the
*           XHDCP22_TX_AKE_SEND_CERT message.
*         - XHDCP22_TX_STATE_A1_SK0 for a 'Stored Km' scenario.
*         - XHDCP22_TX_STATE_A1_NSK0 for a 'Non Stored Km' scenario.
*
* @note   None.
*
******************************************************************************/
XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_1(XHdcp22_Tx *InstancePtr)
{
	int Result = XST_SUCCESS;
	XHdcp22_Tx_DDCMessage *MsgPtr = (XHdcp22_Tx_DDCMessage *)InstancePtr->MessageBuffer;
	XHdcp22_Tx_PairingInfo *PairingInfoPtr = NULL;
	const u8* KPubDpcPtr = NULL;
	XHdcp22_Tx_PairingInfo NewPairingInfo;

	/* receive AKE Send message, wait for 100 ms */
	Result = XHdcp22Tx_WaitForReceiver(InstancePtr, sizeof(XHdcp22_Tx_AKESendCert));
	if (Result != XST_SUCCESS) {
		return XHDCP22_TX_STATE_A0;
	}
	if (InstancePtr->Info.MsgAvailable == (FALSE)) {
		return XHDCP22_TX_STATE_A1_1;
	}

	/* Log after waiting */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE, (u16)XHDCP22_TX_STATE_A1_1);

	/* Receive the RX certificate */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_RX_CERT);
	Result = XHdcp22Tx_ReceiveMsg(InstancePtr, XHDCP22_TX_AKE_SEND_CERT,
									sizeof(XHdcp22_Tx_AKESendCert));
	if (Result != XST_SUCCESS) {
		return XHDCP22_TX_STATE_A0;
	}

	/* Store received Rrx for calculations in other states */
	memcpy(InstancePtr->Info.Rrx,  MsgPtr->Message.AKESendCert.Rrx,
			 sizeof(InstancePtr->Info.Rrx));

	/* Get pairing info pointer for the connected receiver */
	PairingInfoPtr = XHdcp22Tx_GetPairingInfo(MsgPtr->Message.AKESendCert.CertRx.ReceiverId);

	/********************* Handle Stored Km **********************************/
	/* If already existing handle Stored Km sequence: Write AKE_Stored_Km
	 * and wait for H Prime */
	if (PairingInfoPtr != NULL) {

		/* Write encrypted Km */
		XHdcp22Tx_WriteAKEStoredKm(InstancePtr, PairingInfoPtr);
		InstancePtr->Info.StateContext = PairingInfoPtr;

		/* Start the timer for receiving XHDCP22_TX_AKE_SEND_HPRIME */
		XHdcp22Tx_StartTimer(InstancePtr, 200, XHDCP22_TX_AKE_SEND_H_PRIME);
		return XHDCP22_TX_STATE_A1_SK0;
	}

	/********************* Handle No Stored Km *******************************/
	/* PairingInfoPtr == NULL, Non Stored Km Sequence: Verify the certificate */
	KPubDpcPtr = XHdcp22Tx_GetKPubDpc(InstancePtr);

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_VERIFY_SIGNATURE);
	Result = XHdcp22Tx_VerifyCertificate(&MsgPtr->Message.AKESendCert.CertRx,
	                   KPubDpcPtr, /* N */
	                   XHDCP22_TX_KPUB_DCP_LLC_N_SIZE,
	                   &KPubDpcPtr[XHDCP22_TX_KPUB_DCP_LLC_N_SIZE], /* e */
	                   XHDCP22_TX_KPUB_DCP_LLC_E_SIZE);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_VERIFY_SIGNATURE_DONE);

	if (Result != XST_SUCCESS) {
		return XHDCP22_TX_STATE_A0;
	}

	/* Update pairing info */
	memcpy(NewPairingInfo.ReceiverId, MsgPtr->Message.AKESendCert.CertRx.ReceiverId,
	       sizeof(NewPairingInfo.ReceiverId));
	memcpy(NewPairingInfo.Rrx, InstancePtr->Info.Rrx,
	       sizeof(NewPairingInfo.Rrx));
	memcpy(NewPairingInfo.Rtx, InstancePtr->Info.Rtx,
	       sizeof(NewPairingInfo.Rtx));
	memcpy(NewPairingInfo.RxCaps, MsgPtr->Message.AKESendCert.RxCaps,
	       sizeof(NewPairingInfo.RxCaps));

	/* Generate the hashed Km */
	XHdcp22Tx_GenerateKm(InstancePtr, NewPairingInfo.Km);

	/* Done with first step, update pairing info and goto the next
	 * step in the No Stored Km sequence: waiting for H Prime*/
	PairingInfoPtr = XHdcp22Tx_UpdatePairingInfo(&NewPairingInfo);
	if (PairingInfoPtr == NULL) {
		return XHDCP22_TX_STATE_A0;
	}

	InstancePtr->Info.StateContext = (void *)PairingInfoPtr;

	/* Write encrypted Km  */
	XHdcp22Tx_WriteAKENoStoredKm(InstancePtr, &NewPairingInfo,
	                             &MsgPtr->Message.AKESendCert.CertRx);

	/* Start the timer for receiving XHDCP22_TX_AKE_SEND_HPRIME */
	XHdcp22Tx_StartTimer(InstancePtr, 1000, XHDCP22_TX_AKE_SEND_H_PRIME);

	return XHDCP22_TX_STATE_A1_NSK0;
}

/******************************************************************************/
/**
*
* This function executes a part of the A1 state functionality of the HDCP2.2
* TX protocol.
* It handles a part of the 'No Stored Km' scenario and receives and verifies H'
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*         - XHDCP22_TX_STATE_A0 on errors.
*         - XHDCP22_TX_STATE_A1_NSK0 if busy waiting for
*           the XHDCP22_TX_AKE_SEND_H_PRIME message.
*         - XHDCP22_TX_STATE_A1_NSK1 for the next part of the 'Stored Km' scenario.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_Nsk0(XHdcp22_Tx *InstancePtr)
{
	int Result = XST_SUCCESS;
	u8 HPrime[XHDCP22_TX_H_PRIME_SIZE];

	XHdcp22_Tx_PairingInfo *PairingInfoPtr =
	                       (XHdcp22_Tx_PairingInfo *)InstancePtr->Info.StateContext;
	XHdcp22_Tx_DDCMessage *MsgPtr = (XHdcp22_Tx_DDCMessage *)InstancePtr->MessageBuffer;

	/* Wait for the receiver to respond within 1 second.
	 * If the receiver has timed out we go to state A0.
	 * If the receiver is busy, we stay in this state (return from polling).
	 * If the receiver has finished, but the message was not handled yet,
	 * we handle the message.*/
	Result = XHdcp22Tx_WaitForReceiver(InstancePtr, sizeof(XHdcp22_Tx_AKESendHPrime));
	if (Result != XST_SUCCESS) {
		XHdcp22Tx_InvalidatePairingInfo(PairingInfoPtr->ReceiverId);
		return XHDCP22_TX_STATE_A0;
	}
	if (InstancePtr->Info.MsgAvailable == (FALSE)) {
		return XHDCP22_TX_STATE_A1_NSK0;
	}

	/* Log after waiting */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE, (u16)XHDCP22_TX_STATE_A1_NSK0);

	/* Receive H' */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_RX_H1);
	Result = XHdcp22Tx_ReceiveMsg(InstancePtr, XHDCP22_TX_AKE_SEND_H_PRIME,
	                              sizeof(XHdcp22_Tx_AKESendHPrime));
	if (Result != XST_SUCCESS) {
		XHdcp22Tx_InvalidatePairingInfo(PairingInfoPtr->ReceiverId);
		return XHDCP22_TX_STATE_A0;
	}

	/* Verify the received H' */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
                  XHDCP22_TX_LOG_DBG_COMPUTE_H);
	XHdcp22Tx_ComputeHPrime(PairingInfoPtr->Rrx, PairingInfoPtr->RxCaps,
	                        PairingInfoPtr->Rtx, XHdcp22_Tx_TxCaps,
	                        PairingInfoPtr->Km, HPrime);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_COMPUTE_H_DONE);

	if(memcmp(MsgPtr->Message.AKESendHPrime.HPrime, HPrime, sizeof(HPrime)) != 0) {
		XHdcp22Tx_InvalidatePairingInfo(PairingInfoPtr->ReceiverId);
		return XHDCP22_TX_STATE_A0;
	}

	/* Start the timer for receiving XHDCP22_TX_AKE_SEND_PAIRING_INFO */
	XHdcp22Tx_StartTimer(InstancePtr, 200, XHDCP22_TX_AKE_SEND_PAIRING_INFO);
	return XHDCP22_TX_STATE_A1_NSK1;
}

/******************************************************************************/
/**
*
* This function executes a part of the A1 state functionality of the HDCP2.2
* TX protocol.
* It handles a part of the  'No Stored Km' scenario and receives and
* stores pairing info.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*         - XHDCP22_TX_STATE_A0 on errors.
*         - XHDCP22_TX_STATE_A1_NSK1 if busy waiting for
*           the XHDCP22_TX_AKE_SEND_PAIRING_INFO message.
*         - XHDCP22_TX_STATE_A2 if authentication is done.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_Nsk1(XHdcp22_Tx *InstancePtr)
{
	int Result = XST_SUCCESS;

	XHdcp22_Tx_PairingInfo *PairingInfoPtr =
	                       (XHdcp22_Tx_PairingInfo *)InstancePtr->Info.StateContext;
	XHdcp22_Tx_DDCMessage *MsgPtr =
	                       (XHdcp22_Tx_DDCMessage *)InstancePtr->MessageBuffer;

	/* Wait for the receiver to send AKE_Send_Pairing_Info */
	Result = XHdcp22Tx_WaitForReceiver(InstancePtr, sizeof(XHdcp22_Tx_AKESendPairingInfo));
	if (Result != XST_SUCCESS) {
		XHdcp22Tx_InvalidatePairingInfo(PairingInfoPtr->ReceiverId);
		return XHDCP22_TX_STATE_A0;
	}
	if (InstancePtr->Info.MsgAvailable == (FALSE)) {
		return XHDCP22_TX_STATE_A1_NSK1;
	}

	/* Log after waiting */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A1_NSK1);

	/* Receive the expected message */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_RX_EKHKM);
	Result = XHdcp22Tx_ReceiveMsg(InstancePtr, XHDCP22_TX_AKE_SEND_PAIRING_INFO,
	                              sizeof(XHdcp22_Tx_AKESendPairingInfo));
	if (Result != XST_SUCCESS) {
		XHdcp22Tx_InvalidatePairingInfo(PairingInfoPtr->ReceiverId);
		return XHDCP22_TX_STATE_A0;
	}

	/* Store the pairing info with the received Ekh(Km) */
	memcpy(PairingInfoPtr->Ekh_Km, MsgPtr->Message.AKESendPairingInfo.EKhKm,
	       sizeof(PairingInfoPtr->Ekh_Km));

	XHdcp22Tx_UpdatePairingInfo(PairingInfoPtr);

	/* Authentication done, goto the next state (exchange Ks) */
	return XHDCP22_TX_STATE_A2;
}

/******************************************************************************/
/**
*
* This function executes a part of the A1 state functionality of the HDCP2.2
* TX protocol.
* It handles a part of the  'Stored Km' scenario and receives and verifies
* H'.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*         - XHDCP22_TX_STATE_A0 on errors.
*         - XHDCP22_TX_STATE_A1_SK0 if busy waiting for the
*         - XHDCP22_TX_AKE_SEND_H_PRIME message.
*         - XHDCP22_TX_STATE_A2 if authentication is done.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType Xhdcp22Tx_StateA1_Sk0(XHdcp22_Tx *InstancePtr)
{
	int Result = XST_SUCCESS;
	u8 HPrime[XHDCP22_TX_H_PRIME_SIZE];

	XHdcp22_Tx_PairingInfo *PairingInfoPtr =
						(XHdcp22_Tx_PairingInfo *)InstancePtr->Info.StateContext;
	XHdcp22_Tx_DDCMessage *MsgPtr = (XHdcp22_Tx_DDCMessage *)InstancePtr->MessageBuffer;

	/* Wait for the receiver to respond within 1 second.
	 * If the receiver has timed out we go to state A0.
	 * If the receiver is busy, we stay in this state (return from polling).
	 * If the receiver has finished, but the message was not handled yet,
	 * we handle the message.*/
	Result = XHdcp22Tx_WaitForReceiver(InstancePtr, sizeof(XHdcp22_Tx_AKESendHPrime));
	if (Result != XST_SUCCESS) {
		return XHDCP22_TX_STATE_A0;
	}
	if (InstancePtr->Info.MsgAvailable == (FALSE)) {
		return XHDCP22_TX_STATE_A1_SK0;
	}

	/* Log after waiting */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	(u16)XHDCP22_TX_STATE_A1_SK0);

	/* Receive the expected message */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_RX_H1);
	Result = XHdcp22Tx_ReceiveMsg(InstancePtr, XHDCP22_TX_AKE_SEND_H_PRIME,
									sizeof(XHdcp22_Tx_AKESendHPrime));
	if (Result != XST_SUCCESS) {
		return XHDCP22_TX_STATE_A0;
	}

	/* Verify the received H' */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
								 XHDCP22_TX_LOG_DBG_COMPUTE_H);
	XHdcp22Tx_ComputeHPrime(InstancePtr->Info.Rrx, PairingInfoPtr->RxCaps,
							InstancePtr->Info.Rtx, XHdcp22_Tx_TxCaps,
							PairingInfoPtr->Km, HPrime);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
								 XHDCP22_TX_LOG_DBG_COMPUTE_H_DONE);

	if(memcmp(MsgPtr->Message.AKESendHPrime.HPrime, HPrime, sizeof(HPrime)) != 0) {
		return XHDCP22_TX_STATE_A0;
	}
	return XHDCP22_TX_STATE_A2;
}

/*****************************************************************************/
/**
*
* This function executes the A2 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA2(XHdcp22_Tx *InstancePtr)
{
	/* Log but don't clutter our log buffer, check on counter */
	if (InstancePtr->Info.LocalityCheckCounter == 0) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
		                (u16)XHDCP22_TX_STATE_A2);
	}

	/* It is allowed to retry locality check until it has been
	 * checked for 1024 times */
	InstancePtr->Info.LocalityCheckCounter++;

	if (InstancePtr->Info.LocalityCheckCounter > XHDCP22_TX_MAX_ALLOWED_LOCALITY_CHECKS) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_LCCHK_COUNT,
		                (u16)InstancePtr->Info.LocalityCheckCounter-1);
		return XHDCP22_TX_STATE_A0;
	}

	/* Generate Rn */
	XHdcp22Tx_GenerateRn(InstancePtr, InstancePtr->Info.Rn);

	/* Send LC Init message */
	XHdcp22Tx_WriteLcInit(InstancePtr, InstancePtr->Info.Rn);

	/* Start the timer for receiving XHDCP22_TX_AKE_SEND_PAIRING_INFO */
	XHdcp22Tx_StartTimer(InstancePtr, 20, XHDCP22_TX_LC_SEND_L_PRIME);

	return XHDCP22_TX_STATE_A2_1;
}


/*****************************************************************************/
/**
*
* This function executes a part of the A2 state functionality of the HDCP2.2
* TX protocol.
* It receives and verifies L' (locality check).
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return
*         - XHDCP22_TX_STATE_A2_1 if busy waiting for the
*           XHDCP22_TX_AKE_SEND_L_PRIME message.
*         - XHDCP22_TX_STATE_A3 if authentication is done.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA2_1(XHdcp22_Tx *InstancePtr)
{
	int Result = XST_SUCCESS;
	u8 LPrime[XHDCP22_TX_H_PRIME_SIZE];

	XHdcp22_Tx_PairingInfo *PairingInfoPtr =
	                       (XHdcp22_Tx_PairingInfo *)InstancePtr->Info.StateContext;
	XHdcp22_Tx_DDCMessage *MsgPtr =
	                      (XHdcp22_Tx_DDCMessage *)InstancePtr->MessageBuffer;

	/* Wait for the receiver to respond within 20 msecs.
	 * If the receiver has timed out we go to state A2 for a retry.
	 * If the receiver is busy, we stay in this state (return from polling).
	 * If the receiver has finished, but the message was not handled yet,
	 * we handle the message.*/
	Result = XHdcp22Tx_WaitForReceiver(InstancePtr, sizeof(XHdcp22_Tx_LCSendLPrime));
	if (Result != XST_SUCCESS) {
		/* Retry state state A2 */
		return XHDCP22_TX_STATE_A2;
	}
	if (InstancePtr->Info.MsgAvailable == (FALSE)) {
		return XHDCP22_TX_STATE_A2_1;
	}

	/* Log after waiting (don't clutter our log buffer, check on counter) */
	if (InstancePtr->Info.LocalityCheckCounter == 1) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
		                (u16)XHDCP22_TX_STATE_A2_1);
	}

	/* Receive the expected message */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_RX_L1);
	Result = XHdcp22Tx_ReceiveMsg(InstancePtr, XHDCP22_TX_LC_SEND_L_PRIME,
	                              sizeof(XHdcp22_Tx_LCSendLPrime));
	if (Result != XST_SUCCESS) {
		/* Retry state state A2 */
		return XHDCP22_TX_STATE_A2;
	}

	/* Verify the received L' */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	XHDCP22_TX_LOG_DBG_COMPUTE_L);
	XHdcp22Tx_ComputeLPrime(InstancePtr->Info.Rn, PairingInfoPtr->Km,
	                        InstancePtr->Info.Rrx, InstancePtr->Info.Rtx,
	                        LPrime);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	XHDCP22_TX_LOG_DBG_COMPUTE_L_DONE);

	if(memcmp(MsgPtr->Message.LCSendLPrime.LPrime, LPrime, sizeof(LPrime)) != 0) {
		/* Retry state A2 */
		return XHDCP22_TX_STATE_A2;
	}

	/* Log how many times the locality check was repeated (*/
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_LCCHK_COUNT,
	                (u16)InstancePtr->Info.LocalityCheckCounter);
	return XHDCP22_TX_STATE_A3;
}

/*****************************************************************************/
/**
*
* This function executes the A3 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA3(XHdcp22_Tx *InstancePtr)
{
	u8 Riv[XHDCP22_TX_RIV_SIZE];
	u8 Ks[XHDCP22_TX_KS_SIZE];
	u8 EdkeyKs[XHDCP22_TX_EDKEY_KS_SIZE];
	XHdcp22_Tx_PairingInfo *PairingInfoPtr =
	                       (XHdcp22_Tx_PairingInfo *)InstancePtr->Info.StateContext;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A3);


	XHdcp22Tx_GenerateRiv(InstancePtr, Riv);

	/* Set Riv in the cipher */
	XHdcp22Cipher_SetRiv(&InstancePtr->Cipher, Riv, XHDCP22_TX_RIV_SIZE);

	XHdcp22Tx_GenerateKs(InstancePtr, Ks);

	/* Set Ks in the cipher */
	XHdcp22Cipher_SetKs(&InstancePtr->Cipher, Ks, XHDCP22_TX_KS_SIZE);

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_COMPUTE_EDKEYKS);
	XHdcp22Tx_ComputeEdkeyKs(InstancePtr->Info.Rn, PairingInfoPtr->Km, Ks,
	                         InstancePtr->Info.Rrx, InstancePtr->Info.Rtx, EdkeyKs);
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_COMPUTE_EDKEYKS_DONE);
	XHdcp22Tx_WriteSkeSendEks(InstancePtr, EdkeyKs, Riv);

	return XHDCP22_TX_STATE_A4;
}

/*****************************************************************************/
/**
*
* This function executes the A4 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA4(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A4);

	/* Start the mandatory 200 ms timer before authentication can be granted
	 * and the cipher may be enabled */
	XHdcp22Tx_StartTimer(InstancePtr, 200, XHDCP22_TX_TS_WAIT_FOR_CIPHER);

	return XHDCP22_TX_STATE_A5;
}

/*****************************************************************************/
/**
*
* This function executes the A5 state of the HDCP2.2 TX protocol.
* Before authentication is granted, a 200 ms mandatory wait is added, so the
* integrating application does not start the cipher premature.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA5(XHdcp22_Tx *InstancePtr)
{
	u8 DdcBuf[2];
	u16 HdcpRxStatus = 0;

	/* wait for a timer to expire, either it is the 200 ms mandatory time
	 * before cipher enabling, or the re-authentication check timer */
	if (InstancePtr->Timer.TimerExpired == (FALSE)) {
		return XHDCP22_TX_STATE_A5;
	}

	/* Do not pollute the logging on polling log authenticated only once */
	if (InstancePtr->Info.AuthenticationStatus != XHDCP22_TX_AUTHENTICATED) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
		                (u16)XHDCP22_TX_STATE_A5);
	}

	/* Timer has expired, handle it */

	/* Handle mandatory 200 ms cipher timeout */
	if (InstancePtr->Timer.ReasonId == XHDCP22_TX_TS_WAIT_FOR_CIPHER) {
		/* Authenticated ! */
		InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_AUTHENTICATED;
		InstancePtr->Info.ReAuthenticationRequested = (FALSE);

		/* Start the re-authentication check timer */
		XHdcp22Tx_StartTimer(InstancePtr, 1000, XHDCP22_TX_TS_RX_REAUTH_CHECK);
		return XHDCP22_TX_STATE_A5;
	}

	/* Handle the re-authentication check timer */
	if (InstancePtr->Timer.ReasonId == XHDCP22_TX_TS_RX_REAUTH_CHECK) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
		                XHDCP22_TX_LOG_DBG_CHECK_REAUTH);

		DdcBuf[0] = XHDCP22_TX_HDCPPORT_RXSTATUS_OFFSET;
		InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS, 1, DdcBuf, (FALSE),
		                      InstancePtr->DdcHandlerRef);
		InstancePtr->DdcRead(XHDCP22_TX_DDC_BASE_ADDRESS, sizeof(DdcBuf), DdcBuf,
		                     (TRUE), InstancePtr->DdcHandlerRef);

		HdcpRxStatus = *(u16 *)DdcBuf;
		if ((HdcpRxStatus&XHDCP22_TX_RXSTATUS_REAUTH_REQ_MASK) ==
		    XHDCP22_TX_RXSTATUS_REAUTH_REQ_MASK)
		{
			InstancePtr->Info.ReAuthenticationRequested = (TRUE);
			InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_REAUTHENTICATE_REQUESTED;
			return XHDCP22_TX_STATE_A0;
		}
		/* Re-Start the timer for the next status check */
		XHdcp22Tx_StartTimer(InstancePtr, 1000, XHDCP22_TX_TS_RX_REAUTH_CHECK);
	}
	return XHDCP22_TX_STATE_A5;
}

/*****************************************************************************/
/**
*
* This function executes the A6 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA6(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A6);
	return XHDCP22_TX_STATE_A0;
}

/*****************************************************************************/
/**
*
* This function executes the A7 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA7(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A7);
	return XHDCP22_TX_STATE_A0;
}

/*****************************************************************************/
/**
*
* This function executes the A8 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA8(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A8);
	return XHDCP22_TX_STATE_A0;
}

/*****************************************************************************/
/**
*
* This function executes the A9 state of the HDCP2.2 TX protocol.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return The next state to execute.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_StateType XHdcp22Tx_StateA9(XHdcp22_Tx *InstancePtr)
{
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_STATE,
	                (u16)XHDCP22_TX_STATE_A9);
	return XHDCP22_TX_STATE_A0;
}

/*****************************************************************************/
/**
*
* This function executes on transition from state A1 or one of its sub states
* to A0, which means that an error in authentication has occured.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_A1A0(XHdcp22_Tx *InstancePtr)
{
	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_UNAUTHENTICATED;
}

/*****************************************************************************/
/**
*
* This function executes on transition from state A1_SK0 or A1_NSK1 to A2,
* which means that state A2 is executed for the first time.
* The locality check counter is initialized.
* Subsequent locality checks are then executed for an extra 1023 times.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_A1A2(XHdcp22_Tx *InstancePtr)
{
	InstancePtr->Info.LocalityCheckCounter = 0;
}

/*****************************************************************************/
/**
*
* This function executes on transition from state A2 or one of its substates
* to A0, which means that an error in authentication has occured.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_A2A0(XHdcp22_Tx *InstancePtr)
{
	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_UNAUTHENTICATED;
}

/*****************************************************************************/
/**
*
* This function executes on transition from state A3 to A0, which means that
* an error in authetication has occured.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_A3A0(XHdcp22_Tx *InstancePtr)
{
	InstancePtr->Info.AuthenticationStatus = XHDCP22_TX_UNAUTHENTICATED;
}

/*****************************************************************************/
/**
*
* This function executes on transition from state A4 to A5, which means that
* an authetication has succeeded and state A5 is executed the first time.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_A4A5(XHdcp22_Tx *InstancePtr)
{
}

/*****************************************************************************/
/**
*
* This function is a dummy called as long as there is no valid ddc handler set.
* It always returns an error.
*
* @param  DeviceAddress is the DDC (i2c) device address.
* @param  ByteCount is the number of bytes in the buffer.
* @param  BufferPtr is the buffer that stores the data to be written or read.
* @param  Stop is a flag to signal i2c stop condition.
* @param  RefPtr is the reference pointer for the handler.
*
* @return XST_FAILURE
*
* @note   None.
*
******************************************************************************/
static int XHdcp22Tx_StubDdc(u8 DeviceAddress, u16 ByteCount, u8* BufferPtr,
                             u8 Stop, void* RefPtr)
{
	Xil_AssertNonvoidAlways();
}

/*****************************************************************************/
/**
*
* This function is a dummy called as long as there is no valid
* authenticated handler set. It always returns an error.
*
* @param  RefPtr is the reference pointer for the handler.
*
* @return XST_FAILURE
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_StubAuthenticated(void* RefPtr)
{
	Xil_AssertVoidAlways();
}

/*****************************************************************************/
/**
*
* This function returns a pseudo random 64-bit value for Rtx as part of the
* HDCP22 TX initiation message. If appropriate it is overwritten with a test
* vector.
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  RtxPtr is a 64-bit random value;
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_GenerateRtx(XHdcp22_Tx *InstancePtr, u8* RtxPtr)
{
	/* get a 64 bits random number */
	XHdcp22Tx_GenerateRandom(InstancePtr, XHDCP22_TX_RTX_SIZE, RtxPtr);

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	XHdcp22Tx_TestGenerateRtx(InstancePtr, RtxPtr);
#endif
}

/*****************************************************************************/
/**
*
* This function returns a pseudo random 128-bit value for Km as part of the HDCP22 TX
* initiation message. If appropriate it is overwritten with a test vector.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  KmPtr is a pointer to a 128 bits random value.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_GenerateKm(XHdcp22_Tx *InstancePtr, u8* KmPtr)
{
	/* get a 128 bits random number */
	XHdcp22Tx_GenerateRandom(InstancePtr, XHDCP22_TX_KM_SIZE, KmPtr);

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	XHdcp22Tx_TestGenerateKm(InstancePtr, KmPtr);
#endif
}

/*****************************************************************************/
/**
*
* This function returns a pseudo random 64-bit value for Rn as part
* of the locality check.
* If appropriate it is overwritten with a test vector.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  KmPtr is a pointer to a 256 bits random seed value.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_GenerateKmMaskingSeed(XHdcp22_Tx *InstancePtr,
                                             u8* SeedPtr)
{
	/* get a 128 bits random number */
	XHdcp22Tx_GenerateRandom(InstancePtr, XHDCP22_TX_KM_MSK_SEED_SIZE, SeedPtr);

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	XHdcp22Tx_TestGenerateKmMaskingSeed(InstancePtr, SeedPtr);
#endif
}

/*****************************************************************************/
/**
*
* This function returns a pseudo random 64-bit value for Rn as part of the
* HDCP22 TX locality check. If appropriate it is overwritten with a test vector.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  RnPtr is a pointer to a 64 bits random value.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_GenerateRn(XHdcp22_Tx *InstancePtr, u8* RnPtr)
{
	/* get a 128 bits random number */
	XHdcp22Tx_GenerateRandom(InstancePtr, XHDCP22_TX_RN_SIZE, RnPtr);

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	XHdcp22Tx_TestGenerateRn(InstancePtr, RnPtr);
#endif
}

/*****************************************************************************/
/**
*
* This function returns a pseudo random 128-bit value for Ks as part of the HDCP22 TX
* session key exchange message. If appropriate it is overwritten with a test vector.
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  KsPtr is a 128-bit random value;
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_GenerateKs(XHdcp22_Tx *InstancePtr, u8* KsPtr)
{
	/* get a 64 bits random number */
	XHdcp22Tx_GenerateRandom(InstancePtr, XHDCP22_TX_KS_SIZE, KsPtr);

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	XHdcp22Tx_TestGenerateKs(InstancePtr, KsPtr);
#endif
}

/*****************************************************************************/
/**
*
* This function returns a pseudo random 64-bit value for Riv as part of the HDCP22 TX
* session key exchange message. If appropriate it is overwritten with a test vector.
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  RivPtr is a 64-bit random value;
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_GenerateRiv(XHdcp22_Tx *InstancePtr, u8* RivPtr)
{
	/* get a 64 bits random number */
	XHdcp22Tx_GenerateRandom(InstancePtr, XHDCP22_TX_RIV_SIZE, RivPtr);

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	XHdcp22Tx_TestGenerateRiv(InstancePtr, RivPtr);
#endif
}

/*****************************************************************************/
/**
*
* This function returns the DCP LLC public key
* If appropriate it returns a test vector.
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  RivPtr is a 64-bit random value;
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static const u8* XHdcp22Tx_GetKPubDpc(XHdcp22_Tx *InstancePtr)
{
	const u8* KPubDpcPtr = NULL;

#ifdef _XHDCP22_TX_TEST_
	/* Let the test module decide if it should be overwritten with a test vector */
	KPubDpcPtr = XHdcp22Tx_TestGetKPubDpc(InstancePtr);
#endif

	if (KPubDpcPtr == NULL) {
		KPubDpcPtr = XHdcp22_Tx_KpubDcp;
	}

	return KPubDpcPtr;
}

/*****************************************************************************/
/**
*
* This function starts the timer needed for checking the HDCP22 Receive status
* register. In case the timer is started to receive a message,
* the MsgAvailable flag is reset.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  TimeOut_mSec is the timeout for the next status update.
*
* @return XST_SUCCESS or XST_FAILURE.
*
* @note   None.
*
******************************************************************************/
static int XHdcp22Tx_StartTimer(XHdcp22_Tx *InstancePtr, u32 TimeOut_mSec,
                                u8 ReasonId)
{
	u32 Ticks = (u32)(InstancePtr->Timer.TmrCtr.Config.SysClockFreqHz/1e6) * TimeOut_mSec * 1000;

	InstancePtr->Timer.TimerExpired = (FALSE);
	InstancePtr->Timer.ReasonId = ReasonId;
	InstancePtr->Timer.InitialTicks = Ticks;

	/* If the timer was started for receiving a message,
	 * the message available flag must be reset */
	if (ReasonId != XHDCP22_TX_TS_UNDEFINED &&
	    ReasonId != XHDCP22_TX_TS_RX_REAUTH_CHECK &&
	    ReasonId != XHDCP22_TX_TS_WAIT_FOR_CIPHER) {
		InstancePtr->Info.MsgAvailable = (FALSE);
	}

#ifndef DISABLE_TIMEOUT_CHECKING
	if (InstancePtr->Timer.TmrCtr.IsReady == (FALSE)) {
		return XST_FAILURE;
	}

	XTmrCtr_SetResetValue(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0, Ticks);
	XTmrCtr_Start(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0);

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_STARTIMER);
#endif /* DISABLE_TIMEOUT_CHECKING */

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function returns the current timer count value of the expiration timer.
* register.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  TimeOut_mSec is the timeout for the next status update.
*
* @return
*         - XST_SUCCESS if the timer count could be acquired.
*         - XST_FAILURE if not.
*
* @note None.
*
******************************************************************************/
static u32 XHdcp22Tx_GetTimerCount(XHdcp22_Tx *InstancePtr)
{
	return XTmrCtr_GetValue(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0);
}

/*****************************************************************************/
/**
*
* This function handles timer interrupts
*
* @param  CallbackRef refers to a XHdcp22_Tx structure
* @param  TmrCntNumber the number of used timer.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_TimerHandler(void *CallbackRef, u8 TmrCntNumber)
{
	XHdcp22_Tx *InstancePtr = (XHdcp22_Tx *)CallbackRef;

	/* Verify arguments */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	if (TmrCntNumber == XHDCP22_TX_TIMER_CNTR_1) {
		return;
	}

	/* Set timer expired signaling flag */
	InstancePtr->Timer.TimerExpired = (TRUE);

	if (InstancePtr->Info.IsEnabled)
		XHdcp22Tx_ReadRxStatus(InstancePtr);
}


/*****************************************************************************/
/**
*
* This function can be used to change the polling value.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  PollingValue is the value used for the polling algorithm.
*         - 0 : No polling
*         - 1 : Always poll
*         - 2 : 50% - start polling if 50% of timeout value has expired
*         - 3 : 66%
*         - 4 : 75% etc.
* @return None.
*
* @note   None.
*
******************************************************************************/
void XHdcp22Tx_SetMessagePollingValue(XHdcp22_Tx *InstancePtr, u32 PollingValue)
{
	InstancePtr->Info.PollingValue = PollingValue;
}

/*****************************************************************************/
/**
*
* This function waits for receiving expected messages from the receiver.
* If the timer is not started, it will be started.
* This has to be used to avoid blocking waits, and allows polling to return
* to allow the main thread to continue handling other requests.
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  MessageId is the messageId that we are waiting for
* @param  TimeOutMs is the timeout in Ms
* @param  Busy is a flag to signal we are busy with thi message id
*
* @return
*         - XST_SUCCESS if no problems
*         - XST_FAILURE if the receiver has timed out
*
* @note   None.
*
******************************************************************************/
static int XHdcp22Tx_WaitForReceiver(XHdcp22_Tx *InstancePtr, int ExpectedSize)
{
#ifdef _XHDCP22_TX_TEST_
	if (XHdcp22Tx_TestSimulateTimeout(InstancePtr) == TRUE) {
		return XST_FAILURE;
	}
#endif

/* Some receivers require to read status as soon as possible otherwise the
 * receiver may request for a re-authentication so we must poll! */

	u32 TimerCount = 0; /* Timer is counting down */

#ifdef _XHDCP22_TX_TEST_
	/* If the timeout flag is disabled, we disable the timer and keep on polling */
	if ((InstancePtr->Test.TestFlags & XHDCP22_TX_TEST_NO_TIMEOUT) == XHDCP22_TX_TEST_NO_TIMEOUT) {
		if (InstancePtr->Timer.TmrCtr.IsStartedTmrCtr0) {
			XTmrCtr_Stop(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0);
		}
		XHdcp22Tx_ReadRxStatus(InstancePtr);
		InstancePtr->Timer.TimerExpired = (FALSE);
		if ((InstancePtr->Info.RxStatus & XHDCP22_TX_RXSTATUS_AVAIL_BYTES_MASK) ==
		    ExpectedSize) {
			/* Set timer expired flag to signal we've finished waiting */
			XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_MSGAVAILABLE);
			InstancePtr->Timer.TimerExpired = (TRUE);
			InstancePtr->Info.MsgAvailable = (TRUE);
		}
		return XST_SUCCESS;
	}
#endif

	/* busy waiting...*/
	if (InstancePtr->Timer.TimerExpired == (FALSE)) {
		/* Return immediately if polling is not required */
		if (InstancePtr->Info.PollingValue == 0) {
			return XST_SUCCESS;
		}

		/* Poll if requested */
		/* Read current timer count */
		TimerCount = XHdcp22Tx_GetTimerCount(InstancePtr);

		/* Apply polling value: 1=poll always, 2=poll at 50% etc. */
		if (TimerCount <= (InstancePtr->Timer.InitialTicks/InstancePtr->Info.PollingValue))
		{
			/* Read Rx status. */
			XHdcp22Tx_ReadRxStatus(InstancePtr);
			if ((InstancePtr->Info.RxStatus & XHDCP22_TX_RXSTATUS_AVAIL_BYTES_MASK) ==
			    ExpectedSize) {
				/* Stop the hardware timer */
				XTmrCtr_Stop(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_0);

				/* Set timer expired flag and MsgAvailable flag to signal we've finished waiting */
				XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_MSGAVAILABLE);
				InstancePtr->Timer.TimerExpired = (TRUE);
				InstancePtr->Info.MsgAvailable = (TRUE);
			}
		}
		return XST_SUCCESS;
	}

	/* timer expired: waiting done...check size in the status */
	if ((InstancePtr->Info.RxStatus & XHDCP22_TX_RXSTATUS_AVAIL_BYTES_MASK) ==
	    ExpectedSize) {
		XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_MSGAVAILABLE);
		InstancePtr->Info.MsgAvailable = (TRUE);
		return XST_SUCCESS;
	}
	/* The receiver has timed out...and the data size does not match
	 * the expected size! */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_TIMEOUT);
	return XST_FAILURE;
}


/*****************************************************************************/
/**
*
* This function read RX status from the DDC channel.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  MsgBufferPtr the buffer to use for messaging.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_ReadRxStatus(XHdcp22_Tx *InstancePtr)
{
	u8 DdcBuf[2];

	if (XHdcp22_Tx_RxStatusMutex == TRUE) {
		return;
	}

	XHdcp22_Tx_RxStatusMutex = TRUE;
	DdcBuf[0] = XHDCP22_TX_HDCPPORT_RXSTATUS_OFFSET; /* Status address */
	InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS, 1, DdcBuf, (FALSE),
	                      InstancePtr->DdcHandlerRef);
	InstancePtr->DdcRead(XHDCP22_TX_DDC_BASE_ADDRESS, sizeof(DdcBuf), DdcBuf, (TRUE),
	                     InstancePtr->DdcHandlerRef);
	InstancePtr->Info.RxStatus = *(u16 *)DdcBuf;
	XHdcp22_Tx_RxStatusMutex = FALSE;
}

/*****************************************************************************/
/**
*
* This function issues a AKE_Init message
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  MsgBufferPtr the buffer to use for messaging.
*
* @return
*         - XST_SUCCESS if writing succeeded
*         - XST_FAILURE if failed to write.
*
* @note   None.
*
******************************************************************************/
static int XHdcp22Tx_WriteAKEInit(XHdcp22_Tx *InstancePtr)
{
	XHdcp22_Tx_DDCMessage* MsgPtr = (XHdcp22_Tx_DDCMessage*)InstancePtr->MessageBuffer;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_TX_AKEINIT);

	MsgPtr->DdcAddress = XHDCP22_TX_HDCPPORT_WRITE_MSG_OFFSET;
	MsgPtr->Message.MsgId = XHDCP22_TX_AKE_INIT;

	/* Generate Rtx and add to the buffer*/
	XHdcp22Tx_GenerateRtx(InstancePtr, InstancePtr->Info.Rtx);

	memcpy(&MsgPtr->Message.AKEInit.Rtx, InstancePtr->Info.Rtx,
	       sizeof(MsgPtr->Message.AKEInit.Rtx));
	/* Add TxCaps to the buffer*/
	memcpy(&MsgPtr->Message.AKEInit.TxCaps, XHdcp22_Tx_TxCaps,
	       sizeof(MsgPtr->Message.AKEInit.TxCaps));

	/* Execute write */
	return InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS,
	                             sizeof(XHdcp22_Tx_AKEInit)+1,
	                             (u8 *)MsgPtr, (TRUE),
	                             InstancePtr->DdcHandlerRef);
}

/*****************************************************************************/
/**
*
* This function issues a AKE_No_Stored_km message
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  MsgBufferPtr the buffer containing the certificate and re-used
*         for sending the message.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_WriteAKENoStoredKm(XHdcp22_Tx *InstancePtr,
                                         const XHdcp22_Tx_PairingInfo *PairingInfoPtr,
                                         const XHdcp22_Tx_CertRx *CertificatePtr)
{
	XHdcp22_Tx_DDCMessage* MsgPtr = (XHdcp22_Tx_DDCMessage*)InstancePtr->MessageBuffer;
	u8 MaskingSeed[XHDCP22_TX_KM_MSK_SEED_SIZE];
	u8 EKpubKm[XHDCP22_TX_E_KPUB_KM_SIZE];

	MsgPtr->DdcAddress = XHDCP22_TX_HDCPPORT_WRITE_MSG_OFFSET;
	MsgPtr->Message.MsgId = XHDCP22_TX_AKE_NO_STORED_KM;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_ENCRYPT_KM);

	/* Get the seed for the masking RSA-OEAP masking function */
	XHdcp22Tx_GenerateKmMaskingSeed(InstancePtr, MaskingSeed);

	/* Encrypt, pass certificate (1st value is messageId) */
	XHdcp22Tx_EncryptKm(CertificatePtr, PairingInfoPtr->Km, MaskingSeed, EKpubKm);
	memcpy(MsgPtr->Message.AKENoStoredKm.EKpubKm, EKpubKm,
	       sizeof(MsgPtr->Message.AKENoStoredKm.EKpubKm));

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_ENCRYPT_KM_DONE);

	/* Execute write */
	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_TX_NOSTOREDKM);
	InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS,
	                      sizeof(XHdcp22_Tx_AKENoStoredKm)+1, (u8 *)MsgPtr,
	                      (TRUE), InstancePtr->DdcHandlerRef);
}

/*****************************************************************************/
/**
*
* This function issues a AKE_Stored_km message
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  MsgBufferPtr the buffer to use for messaging.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_WriteAKEStoredKm(XHdcp22_Tx *InstancePtr,
                                       const XHdcp22_Tx_PairingInfo *PairingInfoPtr)
{
	XHdcp22_Tx_DDCMessage* MsgPtr =
	                       (XHdcp22_Tx_DDCMessage*)InstancePtr->MessageBuffer;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_TX_STOREDKM);

	MsgPtr->DdcAddress = XHDCP22_TX_HDCPPORT_WRITE_MSG_OFFSET;
	MsgPtr->Message.MsgId = XHDCP22_TX_AKE_STORED_KM;

	memcpy(MsgPtr->Message.AKEStoredKm.EKhKm, PairingInfoPtr->Ekh_Km,
	       sizeof(MsgPtr->Message.AKEStoredKm.EKhKm));
	memcpy(MsgPtr->Message.AKEStoredKm.Rtx, PairingInfoPtr->Rtx,
	       sizeof(MsgPtr->Message.AKEStoredKm.Rtx));
	memcpy(MsgPtr->Message.AKEStoredKm.Rrx, PairingInfoPtr->Rrx,
	       sizeof(MsgPtr->Message.AKEStoredKm.Rrx));

	/* Execute write */
	InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS,
	                      sizeof(XHdcp22_Tx_AKEStoredKm)+1, (u8 *)MsgPtr,
	                      (TRUE), InstancePtr->DdcHandlerRef);
}

/*****************************************************************************/
/**
*
* This function write a locality check has value.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  RnPtr is a pointer to the XHdcp22Tx core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_WriteLcInit(XHdcp22_Tx *InstancePtr, const u8* RnPtr)
{
	XHdcp22_Tx_DDCMessage* MsgPtr =
	                      (XHdcp22_Tx_DDCMessage*)InstancePtr->MessageBuffer;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG,
	                XHDCP22_TX_LOG_DBG_TX_LCINIT);

	MsgPtr->DdcAddress = XHDCP22_TX_HDCPPORT_WRITE_MSG_OFFSET;
	MsgPtr->Message.MsgId = XHDCP22_TX_LC_INIT;

	memcpy(MsgPtr->Message.LCInit.Rn, RnPtr, sizeof(MsgPtr->Message.LCInit.Rn));

	/* Execute write */
	InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS,
	                      sizeof(XHdcp22_Tx_LCInit)+1, (u8 *)MsgPtr,
	                      (TRUE), InstancePtr->DdcHandlerRef);
}

/******************************************************************************/
/**
*
* This function sends the session key to the receiver.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  EdkeyKsPtr is a pointer to the encrypted session key.
* @param  RivPtr is a pointer to the random IV pair.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_WriteSkeSendEks(XHdcp22_Tx *InstancePtr,
                                      const u8 *EdkeyKsPtr, const u8 *RivPtr)
{
	XHdcp22_Tx_DDCMessage* MsgPtr =
	                      (XHdcp22_Tx_DDCMessage*)InstancePtr->MessageBuffer;

	XHdcp22Tx_LogWr(InstancePtr, XHDCP22_TX_LOG_EVT_DBG, XHDCP22_TX_LOG_DBG_TX_EKS);

	MsgPtr->DdcAddress = XHDCP22_TX_HDCPPORT_WRITE_MSG_OFFSET;
	MsgPtr->Message.MsgId = XHDCP22_TX_SKE_SEND_EKS;

	memcpy(MsgPtr->Message.SKESendEks.EDkeyKs, EdkeyKsPtr,
	       sizeof(MsgPtr->Message.SKESendEks.EDkeyKs));

	memcpy(MsgPtr->Message.SKESendEks.Riv, RivPtr,
	       sizeof(MsgPtr->Message.SKESendEks.Riv));

	/* Execute write */
	InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS,
	                      sizeof(XHdcp22_Tx_SKESendEks)+1, (u8 *)MsgPtr,
	                      (TRUE), InstancePtr->DdcHandlerRef);
}

/*****************************************************************************/
/**
*
* This function receives a message send by HDCP22 RX.
*
* @param  InstancePtr is a pointer to the XHdcp22Tx core instance.
* @param  MessageId the message Id of the expected message.
* @param  MessageSize the size of the expected message.
*
* @return
*         - XST_SUCCESS if the message size and id are as expected.
*         - XST_FAILURE if if the message size and id are incorrect.
* @note   None.
*
******************************************************************************/
static int XHdcp22Tx_ReceiveMsg(XHdcp22_Tx *InstancePtr, u8 MessageId,
                                int MessageSize)
{
	int Result = XST_SUCCESS;
	XHdcp22_Tx_DDCMessage* MsgPtr =
	                       (XHdcp22_Tx_DDCMessage*)InstancePtr->MessageBuffer;

	u32 ReceivedSize = InstancePtr->Info.RxStatus &
	                   XHDCP22_TX_RXSTATUS_AVAIL_BYTES_MASK;

	/* Check if the received size matches the expected size. */
	if (ReceivedSize != MessageSize) {
		return XST_FAILURE;
	}

	MsgPtr->DdcAddress = XHDCP22_TX_HDCPPORT_READ_MSG_OFFSET;
	/* Set the Expected msg ID in the buffer for testing purposes */
	MsgPtr->Message.MsgId = MessageId;

	Result = InstancePtr->DdcWrite(XHDCP22_TX_DDC_BASE_ADDRESS, 1,
	                               &MsgPtr->DdcAddress, (FALSE),
	                               InstancePtr->DdcHandlerRef);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	/* Reading starts on message id */
	Result = InstancePtr->DdcRead(XHDCP22_TX_DDC_BASE_ADDRESS, ReceivedSize,
	                              (u8 *)&MsgPtr->Message.MsgId,
	                              (TRUE), InstancePtr->DdcHandlerRef);
	if (Result != XST_SUCCESS) {
			return Result;
	}

	/* Check if the received message id matches the expected message id */
	if (MsgPtr->Message.MsgId != MessageId)
		return XST_FAILURE;

	return XST_SUCCESS;
}


/*****************************************************************************/
/**
*
* This function clear the global pairing info structure, so every
* HDCP2.2 receiver will have to go through the 'no stored km' sequence
* to authenticate.
*
* @param   InstancePtr is a pointer to the XHdcp22Tx core instance.
*
* @return  XST_SUCCESS
*
* @note    None.
*
******************************************************************************/
int XHdcp22Tx_ClearPairingInfo(XHdcp22_Tx *InstancePtr)
{
	Xil_AssertNonvoid(InstancePtr != NULL);

	int i;
	for (i=0; i<XHDCP22_TX_MAX_STORED_PAIRINGINFO; i++) {
		memset(&XHdcp22_Tx_PairingInfoStore[i], 0x00, sizeof(XHdcp22_Tx_PairingInfo));
	}
	return XST_SUCCESS;
}
/*****************************************************************************/
/**
*
* This function gets a stored pairing info entry from the storage.
*
* @param  ReceiverId is a pointer to a 5-byte receiver Id.
*
* @return A pointer to the found ReceiverId or NULL if the pairing info wasn't
*         stored yet.
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_PairingInfo *XHdcp22Tx_GetPairingInfo(const u8 *ReceiverId)
{
	int i = 0;
	u8 IllegalRecvID[] = {0x0, 0x0, 0x0, 0x0, 0x0};

	/* Check for illegal Receiver ID */
	if (memcmp(ReceiverId, IllegalRecvID, XHDCP22_TX_CERT_RCVID_SIZE) == 0) {
		return NULL;
	}

	for (i=0; i<XHDCP22_TX_MAX_STORED_PAIRINGINFO; i++) {
		if (memcmp(ReceiverId, &XHdcp22_Tx_PairingInfoStore[i],
		           XHDCP22_TX_CERT_RCVID_SIZE) == 0) {
			return &XHdcp22_Tx_PairingInfoStore[i];
		}
	}
	return NULL;
}

/*****************************************************************************/
/**
*
* This function updates a pairing info entry in the storage.
*
* @param  PairingInfo is a pointer to a pairing info structure.
*
* @return
*         - XST_SUCCESS if there was enough room
*         - XST_FAILURE if there was no room for this receiver
*
* @note   None.
*
******************************************************************************/
static XHdcp22_Tx_PairingInfo *XHdcp22Tx_UpdatePairingInfo(
                              const XHdcp22_Tx_PairingInfo *PairingInfo)
{
	int i = 0;
	u8 Empty[XHDCP22_TX_CERT_RCVID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00};

	/* find the id or an empty slot */
	for (i=0; i<XHDCP22_TX_MAX_STORED_PAIRINGINFO; i++) {
		if (memcmp(PairingInfo->ReceiverId, &XHdcp22_Tx_PairingInfoStore[i],
		           XHDCP22_TX_CERT_RCVID_SIZE)== 0) {
			break;
		}
		if (memcmp(Empty, &XHdcp22_Tx_PairingInfoStore[i],
		           XHDCP22_TX_CERT_RCVID_SIZE) == 0) {
			break;
		}
	}

	/* If no slot available, use the first one */
	if (i==XHDCP22_TX_MAX_STORED_PAIRINGINFO) {
		i=0;
	}

	/* Set new pairing info*/
	memcpy(&XHdcp22_Tx_PairingInfoStore[i], PairingInfo,
	       sizeof(XHdcp22_Tx_PairingInfo));

	return &XHdcp22_Tx_PairingInfoStore[i];
}

/*****************************************************************************/
/**
*
* This function invalidates a pairing info structure
*
* @param  ReceiverId is a pointer to a 5-byte receiver Id.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void XHdcp22Tx_InvalidatePairingInfo(const u8* ReceiverId)
{
	XHdcp22_Tx_PairingInfo *InfoPtr = XHdcp22Tx_GetPairingInfo(ReceiverId);

	/* do nothing if the id was not found */
	if (InfoPtr == NULL) {
		return;
	}
	/* clear the found structure */
	memset(InfoPtr, 0x00, sizeof(XHdcp22_Tx_PairingInfo));
}

/*****************************************************************************/
/**
*
* This function clears the log pointers
*
* @param  InstancePtr is a pointer to the XHdcp22_Tx core instance.
* @param  Verbose allows to add debug logging.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
void XHdcp22Tx_LogReset(XHdcp22_Tx *InstancePtr, u8 Verbose)
{
	/* Verify arguments. */
	Xil_AssertVoid(InstancePtr != NULL);

	InstancePtr->Log.Tail = 0;
	InstancePtr->Log.Verbose = Verbose;
	/* Reset and start the logging timer. */
	/* Note: This timer increments continuously and will wrap at 42 second (100 Mhz clock) */
	if (InstancePtr->Timer.TmrCtr.IsReady == XIL_COMPONENT_IS_READY) {
	   XTmrCtr_SetResetValue(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_1, 0);
	   XTmrCtr_Start(&InstancePtr->Timer.TmrCtr, XHDCP22_TX_TIMER_CNTR_1);
	}
}

/*****************************************************************************/
/**
*
* This function returns the time expired since a log reset was called
*
* @param  InstancePtr is a pointer to the XHdcp22_Tx core instance.
*
* @return The expired logging time in useconds.
*
* @note   None.
*
******************************************************************************/
u32 XHdcp22Tx_LogGetTimeUSecs(XHdcp22_Tx *InstancePtr)
{
	if (InstancePtr->Timer.TmrCtr.IsReady != XIL_COMPONENT_IS_READY)
		return 0;

	u32 PeriodUsec = (u32)InstancePtr->Timer.TmrCtr.Config.SysClockFreqHz * 1e-6;
	return 	 ( XTmrCtr_GetValue(&InstancePtr->Timer.TmrCtr,
			   XHDCP22_TX_TIMER_CNTR_1) / PeriodUsec);
}

/*****************************************************************************/
/**
*
* This function writes HDCP TX logs into buffer.
*
* @param  InstancePtr is a pointer to the XHdcp22_Tx core instance.
* @param  Evt specifies an action to be carried out. Please refer
*         #XHdcp22_Tx_LogEvt enum in xhdcp22_tx.h.
* @param  Data specifies the information that gets written into log
*         buffer.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
void XHdcp22Tx_LogWr(XHdcp22_Tx *InstancePtr, XHdcp22_Tx_LogEvt Evt, u16 Data)
{
	int LogBufSize = 0;

	/* Verify arguments. */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(Evt < (XHDCP22_TX_LOG_INVALID));

	if (InstancePtr->Log.Verbose == FALSE && Evt == XHDCP22_TX_LOG_EVT_DBG) {
		return;
	}

	/* Write data and event into log buffer */
	InstancePtr->Log.LogItems[InstancePtr->Log.Head].Data = Data;
	InstancePtr->Log.LogItems[InstancePtr->Log.Head].LogEvent = Evt;
	InstancePtr->Log.LogItems[InstancePtr->Log.Head].TimeStamp =
                            XHdcp22Tx_LogGetTimeUSecs(InstancePtr);

	/* Update head pointer if reached to end of the buffer */
	LogBufSize = sizeof(InstancePtr->Log.LogItems)/sizeof(XHdcp22_Tx_LogItem);
	if (InstancePtr->Log.Head == (u16)(LogBufSize) - 1) {
		/* Clear pointer */
		InstancePtr->Log.Head = 0;
	} else {
		/* Increment pointer */
		InstancePtr->Log.Head++;
	}

	/* Check tail pointer. When the two pointer are equal, then the buffer
	 * is full.In this case then increment the tail pointer as well to
	 * remove the oldest entry from the buffer.
	 */
	if (InstancePtr->Log.Tail == InstancePtr->Log.Head) {
		if (InstancePtr->Log.Tail == (u16)(LogBufSize) - 1) {
			InstancePtr->Log.Tail = 0;
		} else {
			InstancePtr->Log.Tail++;
		}
	}
}

/*****************************************************************************/
/**
*
* This function provides the log information from the log buffer.
*
* @param  InstancePtr is a pointer to the XHdcp22_Tx core instance.
*
* @return
*         - Content of log buffer if log pointers are not equal.
*         - Otherwise Zero.
*
* @note   None.
*
******************************************************************************/
XHdcp22_Tx_LogItem* XHdcp22Tx_LogRd(XHdcp22_Tx *InstancePtr)
{
	XHdcp22_Tx_LogItem* LogPtr;
	int LogBufSize = 0;
	u16 Tail = 0;
	u16 Head = 0;

	/* Verify argument. */
	Xil_AssertNonvoid(InstancePtr != NULL);

	Tail = InstancePtr->Log.Tail;
	Head = InstancePtr->Log.Head;

	/* Check if there is any data in the log and return a NONE defined log item */
	LogBufSize = sizeof(InstancePtr->Log.LogItems)/sizeof(XHdcp22_Tx_LogItem);
	if (Tail == Head) {
		LogPtr = &InstancePtr->Log.LogItems[Tail];
		LogPtr->Data = 0;
		LogPtr->LogEvent = XHDCP22_TX_LOG_EVT_NONE;
		LogPtr->TimeStamp = 0;
		return LogPtr;
	}

	LogPtr = &InstancePtr->Log.LogItems[Tail];

	/* Increment tail pointer */
	if (Tail == (u16)(LogBufSize) - 1) {
		InstancePtr->Log.Tail = 0;
	}
	else {
		InstancePtr->Log.Tail++;
	}
	return LogPtr;
}

/*****************************************************************************/
/**
*
* This function prints the content of log buffer.
*
* @param  InstancePtr is a pointer to the HDCP22 TX core instance.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
void XHdcp22Tx_LogDisplay(XHdcp22_Tx *InstancePtr)
{
	XHdcp22_Tx_LogItem* LogPtr;
	char str[255];
	u64 TimeStampPrev = 0;

	/* Verify argument. */
	Xil_AssertVoid(InstancePtr != NULL);

#ifdef _XHDCP22_TX_TEST_
	if (InstancePtr->Test.TestMode == XHDCP22_TX_TESTMODE_UNIT) {
		XHdcp22Tx_LogDisplayUnitTest(InstancePtr);
		return;
	}
#endif

	xil_printf("\r\n-------HDCP22 TX log start-------\r\n");
	strcpy(str, "UNDEFINED");
	do {
		/* Read log data */
		LogPtr = XHdcp22Tx_LogRd(InstancePtr);

		/* Print timestamp */
		if(LogPtr->LogEvent != XHDCP22_TX_LOG_EVT_NONE)
		{
			if(LogPtr->TimeStamp < TimeStampPrev) TimeStampPrev = 0;
			xil_printf("[%8ld:", LogPtr->TimeStamp);
			xil_printf("%8ld] ", (LogPtr->TimeStamp - TimeStampPrev));
			TimeStampPrev = LogPtr->TimeStamp;
		}

		/* Print log event */
		switch (LogPtr->LogEvent) {
		case (XHDCP22_TX_LOG_EVT_NONE):
			xil_printf("-------HDCP22 TX log end-------\r\n\r\n");
			break;
		case XHDCP22_TX_LOG_EVT_STATE:
			switch(LogPtr->Data)
			{
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, H0)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, H1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A0)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A1_1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A1_NSK0)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A1_NSK1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A1_SK0)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A2)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A2_1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A3)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A4)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A5)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A6)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A7)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A8)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_STATE_, A9)
				default: break;
			};
			xil_printf("Current state [%s]\r\n", str);
			break;
		case XHDCP22_TX_LOG_EVT_POLL_RESULT:
			switch(LogPtr->Data)
			{
			case XHDCP22_TX_INCOMPATIBLE_RX: strcpy(str, "INCOMPATIBLE RX"); break;
			case XHDCP22_TX_AUTHENTICATION_BUSY: strcpy(str, "AUTHENTICATION BUSY"); break;
			case XHDCP22_TX_AUTHENTICATED: strcpy(str, "AUTHENTICATED"); break;
			case XHDCP22_TX_UNAUTHENTICATED: strcpy(str, "UN-AUTHENTICATED"); break;
			case XHDCP22_TX_REAUTHENTICATE_REQUESTED: strcpy(str, "RE-AUTHENTICATION REQUESTED"); break;
			default: break;
			}
			xil_printf("Poll result [%s]\r\n", str);
			break;
		case XHDCP22_TX_LOG_EVT_ENABLED:
			if (LogPtr->Data == (FALSE)) {
				strcpy(str, "DISABLED");
			} else {
				strcpy(str, "ENABLED");
			}
			xil_printf("State machine [%s]\r\n", str);
			break;
		case XHDCP22_TX_LOG_EVT_RESET:
			xil_printf("Asserted [RESET]\r\n");
			break;
		case XHDCP22_TX_LOG_EVT_ENCR_ENABLED:
			if (LogPtr->Data == (FALSE)) {
				strcpy(str, "DISABLED");
			} else {
				strcpy(str, "ENABLED");
			}
			xil_printf("Encryption [%s]\r\n", str);
			break;
		case XHDCP22_TX_LOG_EVT_TEST_ERROR:
			switch(LogPtr->Data)
			{
			case XHDCP22_TX_AKE_NO_STORED_KM:
			      strcpy(str, "EkpubKm does not match the calculated value.");
			      break;
			case XHDCP22_TX_SKE_SEND_EKS:
			      strcpy(str, "EdkeyKs does not match the calculated value.");
			      break;
			case XHDCP22_TX_MSG_UNDEFINED:
			      strcpy(str, "Trying to write an unexpected message.");
			      break;
			default: break;
			};
			xil_printf("Error: Test error [%s]\r\n", str);
			break;
		case XHDCP22_TX_LOG_EVT_LCCHK_COUNT:
			xil_printf("Locality check count [%d]\r\n", LogPtr->Data);
			break;
		case XHDCP22_TX_LOG_EVT_DBG:
			switch(LogPtr->Data)
			{
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, STARTIMER)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, MSGAVAILABLE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TX_AKEINIT)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, RX_CERT)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, VERIFY_SIGNATURE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, VERIFY_SIGNATURE_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, ENCRYPT_KM)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, ENCRYPT_KM_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TX_NOSTOREDKM)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TX_STOREDKM)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, RX_H1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, RX_EKHKM)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, COMPUTE_H)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, COMPUTE_H_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TX_LCINIT)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, RX_L1)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, COMPUTE_L)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, COMPUTE_L_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, COMPUTE_EDKEYKS)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, COMPUTE_EDKEYKS_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TX_EKS)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, CHECK_REAUTH)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TIMEOUT)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, TIMESTAMP)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, AES128ENC)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, AES128ENC_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, SHA256HASH)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, SHA256HASH_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, OEAPENC)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, OEAPENC_DONE)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, RSAENC)
				XHDCP22_TX_CASE_TO_STR_PRE(XHDCP22_TX_LOG_DBG_, RSAENC_DONE)
				default: break;
			};
			xil_printf("Debug: Event [%s]\r\n", str);
			break;
		case XHDCP22_TX_LOG_EVT_USER:
			xil_printf("User: %d\r\n", LogPtr->Data);
			break;
		default:
			xil_printf("Error: Unknown log event\r\n");
			break;
		}
	} while (LogPtr->LogEvent != XHDCP22_TX_LOG_EVT_NONE);
}


/** @} */
