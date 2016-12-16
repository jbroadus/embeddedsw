/******************************************************************************
*
* Copyright (C) 2016 Xilinx, Inc.  All rights reserved.
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
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/****************************************************************************/
/**
*
* @file xusbpsu.h
* @addtogroup usbpsu_v1_0
* @{
* @details
*
* <pre>
*
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- -----  -------- -----------------------------------------------------
* 1.0   sg    06/06/16 First release
* 1.1   sg    10/24/16 Update for backward compatability
*                      Added XUsbPsu_IsSuperSpeed function in xusbpsu.c
*
* </pre>
*
*****************************************************************************/
#ifndef XUSBPSU_H  /* Prevent circular inclusions */
#define XUSBPSU_H  /* by using protection macros  */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files ********************************/
#include "xparameters.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xusbpsu_hw.h"
#include "xil_io.h"
/*
 * The header sleep.h and API usleep() can only be used with an arm design.
 * MB_Sleep() is used for microblaze design.
 */
#if defined (__arm__) || defined (__aarch64__)
#include "sleep.h"
#endif

#ifdef __MICROBLAZE__
#include "microblaze_sleep.h"
#endif
#include "xil_cache.h"

/************************** Constant Definitions ****************************/

#define ALIGNMENT_CACHELINE		__attribute__ ((aligned(64)))

#define	XUSBPSU_PHY_TIMEOUT		5000U /* in micro seconds */

#define XUSBPSU_EP_DIR_IN				1U
#define XUSBPSU_EP_DIR_OUT				0U

#define XUSBPSU_ENDPOINT_NUMBER_MASK        0x0f    /* in bEndpointAddress */
#define XUSBPSU_ENDPOINT_DIR_MASK           0x80

#define XUSBPSU_ENDPOINT_XFERTYPE_MASK      0x03    /* in bmAttributes */
#define XUSBPSU_ENDPOINT_XFER_CONTROL       0U
#define XUSBPSU_ENDPOINT_XFER_ISOC          1U
#define XUSBPSU_ENDPOINT_XFER_BULK          2U
#define XUSBPSU_ENDPOINT_XFER_INT           3U
#define XUSBPSU_ENDPOINT_MAX_ADJUSTABLE     0x80

#define	XUSBPSU_TEST_J							1U
#define	XUSBPSU_TEST_K							2U
#define	XUSBPSU_TEST_SE0_NAK					3U
#define	XUSBPSU_TEST_PACKET						4U
#define	XUSBPSU_TEST_FORCE_ENABLE				5U

#define XUSBPSU_NUM_TRBS				8

#define XUSBPSU_EVENT_PENDING		(0x00000001U << 0)

#define XUSBPSU_EP_ENABLED			(0x00000001U << 0)
#define XUSBPSU_EP_STALL			(0x00000001U << 1)
#define XUSBPSU_EP_WEDGE			(0x00000001U << 2)
#define XUSBPSU_EP_BUSY				((u32)0x00000001U << 4)
#define XUSBPSU_EP_PENDING_REQUEST	(0x00000001U << 5)
#define XUSBPSU_EP_MISSED_ISOC		(0x00000001U << 6)

#define	XUSBPSU_GHWPARAMS0				0U
#define	XUSBPSU_GHWPARAMS1				1U
#define	XUSBPSU_GHWPARAMS2				2U
#define	XUSBPSU_GHWPARAMS3				3U
#define	XUSBPSU_GHWPARAMS4				4U
#define	XUSBPSU_GHWPARAMS5				5U
#define	XUSBPSU_GHWPARAMS6				6U
#define	XUSBPSU_GHWPARAMS7				7U

/* HWPARAMS0 */
#define XUSBPSU_MODE(n)					((n) & 0x7)
#define XUSBPSU_MDWIDTH(n)				(((n) & 0xff00) >> 8)

/* HWPARAMS1 */
#define XUSBPSU_NUM_INT(n)				(((n) & (0x3f << 15)) >> 15)

/* HWPARAMS3 */
#define XUSBPSU_NUM_IN_EPS_MASK		((u32)0x0000001fU << (u32)18)
#define XUSBPSU_NUM_EPS_MASK		((u32)0x0000003fU << (u32)12)
#define XUSBPSU_NUM_EPS(p)			(((u32)(p) &		\
									(XUSBPSU_NUM_EPS_MASK)) >> (u32)12)
#define XUSBPSU_NUM_IN_EPS(p)		(((u32)(p) &		\
									(XUSBPSU_NUM_IN_EPS_MASK)) >> (u32)18)

/* HWPARAMS7 */
#define XUSBPSU_RAM1_DEPTH(n)			((n) & 0xffff)

#define XUSBPSU_DEPEVT_XFERCOMPLETE		0x01U
#define XUSBPSU_DEPEVT_XFERINPROGRESS	0x02U
#define XUSBPSU_DEPEVT_XFERNOTREADY		0x03U
#define XUSBPSU_DEPEVT_STREAMEVT		0x06U
#define XUSBPSU_DEPEVT_EPCMDCMPLT		0x07U

/* Within XferNotReady */
#define DEPEVT_STATUS_TRANSFER_ACTIVE	(1 << 3)

/* Within XferComplete */
#define DEPEVT_STATUS_BUSERR			(1 << 0)
#define DEPEVT_STATUS_SHORT				(1 << 1)
#define DEPEVT_STATUS_IOC				(1 << 2)
#define DEPEVT_STATUS_LST				(1 << 3)

/* Stream event only */
#define DEPEVT_STREAMEVT_FOUND		1U
#define DEPEVT_STREAMEVT_NOTFOUND	2U

/* Control-only Status */
#define DEPEVT_STATUS_CONTROL_DATA				1U
#define DEPEVT_STATUS_CONTROL_STATUS			2U
#define DEPEVT_STATUS_CONTROL_DATA_INVALTRB		9
#define DEPEVT_STATUS_CONTROL_STATUS_INVALTRB	0xA

#define XUSBPSU_ENDPOINTS_NUM			12U

#define XUSBPSU_EVENT_SIZE				4U       /* bytes */
#define XUSBPSU_EVENT_MAX_NUM			64U      /* 2 events/endpoint */
#define XUSBPSU_EVENT_BUFFERS_SIZE		(XUSBPSU_EVENT_SIZE * \
										XUSBPSU_EVENT_MAX_NUM)

#define XUSBPSU_EVENT_TYPE_MASK                 0x000000feU

#define XUSBPSU_EVENT_TYPE_DEV                  0U
#define XUSBPSU_EVENT_TYPE_CARKIT               3U
#define XUSBPSU_EVENT_TYPE_I2C                  4U

#define XUSBPSU_DEVICE_EVENT_DISCONNECT         0U
#define XUSBPSU_DEVICE_EVENT_RESET              1U
#define XUSBPSU_DEVICE_EVENT_CONNECT_DONE       2U
#define XUSBPSU_DEVICE_EVENT_LINK_STATUS_CHANGE 3U
#define XUSBPSU_DEVICE_EVENT_WAKEUP             4U
#define XUSBPSU_DEVICE_EVENT_HIBER_REQ          5U
#define XUSBPSU_DEVICE_EVENT_EOPF               6U
#define XUSBPSU_DEVICE_EVENT_SOF                7U
#define XUSBPSU_DEVICE_EVENT_ERRATIC_ERROR      9U
#define XUSBPSU_DEVICE_EVENT_CMD_CMPL           10U
#define XUSBPSU_DEVICE_EVENT_OVERFLOW           11U

#define XUSBPSU_GEVNTCOUNT_MASK                 0x0000fffcU

/*
 * Control Endpoint state
 */
#define	XUSBPSU_EP0_SETUP_PHASE				1U	/**< Setup Phase */
#define	XUSBPSU_EP0_DATA_PHASE				2U	/**< Data Phase */
#define	XUSBPSU_EP0_STATUS_PHASE			3U	/**< Status Pahse */

/*
 * Link State
 */
#define		XUSBPSU_LINK_STATE_MASK			0x0FU

typedef enum {
	XUSBPSU_LINK_STATE_U0 = 0x00U, /**< in HS - ON */
	XUSBPSU_LINK_STATE_U1 =	0x01U,
	XUSBPSU_LINK_STATE_U2 =	0x02U, /**< in HS - SLEEP */
	XUSBPSU_LINK_STATE_U3 =	0x03U, /**< in HS - SUSPEND */
	XUSBPSU_LINK_STATE_SS_DIS =	0x04U,
	XUSBPSU_LINK_STATE_RX_DET =	0x05U,
	XUSBPSU_LINK_STATE_SS_INACT = 0x06U,
	XUSBPSU_LINK_STATE_POLL	=	0x07U,
	XUSBPSU_LINK_STATE_RECOV =	0x08U,
	XUSBPSU_LINK_STATE_HRESET =	0x09U,
	XUSBPSU_LINK_STATE_CMPLY =	0x0AU,
	XUSBPSU_LINK_STATE_LPBK	=	0x0BU,
	XUSBPSU_LINK_STATE_RESET = 	0x0EU,
	XUSBPSU_LINK_STATE_RESUME =	0x0FU,
}XusbPsuLinkState;

typedef enum {
	XUSBPSU_LINK_STATE_CHANGE_U0 = 0x00U, /**< in HS - ON */
	XUSBPSU_LINK_STATE_CHANGE_SS_DIS =	0x04U,
	XUSBPSU_LINK_STATE_CHANGE_RX_DET =	0x05U,
	XUSBPSU_LINK_STATE_CHANGE_SS_INACT = 0x06U,
	XUSBPSU_LINK_STATE_CHANGE_RECOV =	0x08U,
	XUSBPSU_LINK_STATE_CHANGE_CMPLY =	0x0AU,
}XusbPsuLinkStateChange;

/*
 * Device States
 */
#define		XUSBPSU_STATE_ATTACHED			0U
#define		XUSBPSU_STATE_POWERED			1U
#define		XUSBPSU_STATE_DEFAULT			2U
#define		XUSBPSU_STATE_ADDRESS			3U
#define		XUSBPSU_STATE_CONFIGURED		4U
#define		XUSBPSU_STATE_SUSPENDED			5U

/*
 * Device Speeds
 */
#define		XUSBPSU_SPEED_UNKNOWN			0U
#define		XUSBPSU_SPEED_LOW				1U
#define		XUSBPSU_SPEED_FULL				2U
#define		XUSBPSU_SPEED_HIGH				3U
#define		XUSBPSU_SPEED_SUPER				4U



/**************************** Type Definitions ******************************/

/**
 * This typedef contains configuration information for the XUSBPSU
 * device.
 */
typedef struct {
        u16 DeviceId;           /**< Unique ID of controller */
        u32 BaseAddress;        /**< Core register base address */
} XUsbPsu_Config;

/**
 * Software Event buffer representation
 */
struct XUsbPsu_EvtBuffer {
	void	*BuffAddr;
	u32		Offset;
	u32		Count;
	u32		Flags;
};

/**
 * Transfer Request Block - Hardware format
 */
struct XUsbPsu_Trb {
	u32		BufferPtrLow;
	u32		BufferPtrHigh;
	u32		Size;
	u32		Ctrl;
} __attribute__((packed));


/*
 * Endpoint Parameters
 */
struct XUsbPsu_EpParams {
	u32	Param2;		/**< Parameter 2 */
	u32	Param1;		/**< Parameter 1 */
	u32	Param0;		/**< Parameter 0 */
};

/**
 * USB Standard Control Request
 */
typedef struct {
        u8  bRequestType;
        u8  bRequest;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
} __attribute__ ((packed)) SetupPacket;

/**
 * Endpoint representation
 */
struct XUsbPsu_Ep {
	void (*Handler)(void *, u32, u32);
						/** < User handler called
						 *   when data is sent for IN Ep
						 *   and received for OUT Ep
						 */
	struct XUsbPsu_Trb	EpTrb ALIGNMENT_CACHELINE;/**< TRB used by endpoint */
	u32	EpStatus;		/**< Flags to represent Endpoint status */
	u32	RequestedBytes;	/**< RequestedBytes for transfer */
	u32	BytesTxed;		/**< Actual Bytes transferred */
	u16	MaxSize;		/**< Size of endpoint */
	u8	*BufferPtr;		/**< Buffer location */
	u8	ResourceIndex;	/**< Resource Index assigned to
						 *  Endpoint by core
						 */
	u8	PhyEpNum;		/**< Physical Endpoint Number in core */
	u8	UsbEpNum;		/**< USB Endpoint Number */
	u8	Type;			/**< Type of Endpoint -
						 *	 Control/BULK/INTERRUPT/ISOC
						 */
	u8	Direction;		/**< Direction - EP_DIR_OUT/EP_DIR_IN */
	u8	UnalignedTx;
};

/**
 * USB Device Controller representation
 */
struct XUsbPsu {
	SetupPacket SetupData ALIGNMENT_CACHELINE;
					/**< Setup Packet buffer */
	struct XUsbPsu_Trb Ep0_Trb ALIGNMENT_CACHELINE;
					/**< TRB for control transfers */
	XUsbPsu_Config *ConfigPtr;	/**< Configuration info pointer */
	struct XUsbPsu_Ep eps[XUSBPSU_ENDPOINTS_NUM]; /**< Endpoints */
	struct XUsbPsu_EvtBuffer Evt;
	struct XUsbPsu_EpParams EpParams;
	u32 BaseAddress;	/**< Core register base address */
	u32 DevDescSize;
	u32 ConfigDescSize;
	void (*Chapter9)(struct XUsbPsu *, SetupPacket *);
	void (*ResetIntrHandler)(struct XUsbPsu *);
	void (*DisconnectIntrHandler)(struct XUsbPsu *);
	void *DevDesc;
	void *ConfigDesc;
	u8 EventBuffer[XUSBPSU_EVENT_BUFFERS_SIZE]
						__attribute__((aligned(XUSBPSU_EVENT_BUFFERS_SIZE)));
	u8 NumOutEps;
	u8 NumInEps;
	u8 ControlDir;
	u8 IsInTestMode;
	u8 TestMode;
	u8 Speed;
	u8 State;
	u8 Ep0State;
	u8 LinkState;
	u8 UnalignedTx;
	u8 IsConfigDone;
	u8 IsThreeStage;
	void *data_ptr; /* pointer for storing applications data */
};

struct XUsbPsu_Event_Type {
	u32	Is_DevEvt:1;
	u32	Type:7;
	u32	Reserved8_31:24;
} __attribute__((packed));

/**
 * struct XUsbPsu_event_depvt - Device Endpoint Events
 * @Is_EpEvt: indicates this is an endpoint event
 * @endpoint_number: number of the endpoint
 * @endpoint_event: The event we have:
 *	0x00	- Reserved
 *	0x01	- XferComplete
 *	0x02	- XferInProgress
 *	0x03	- XferNotReady
 *	0x04	- RxTxFifoEvt (IN->Underrun, OUT->Overrun)
 *	0x05	- Reserved
 *	0x06	- StreamEvt
 *	0x07	- EPCmdCmplt
 * @Reserved11_10: Reserved, don't use.
 * @Status: Indicates the status of the event. Refer to databook for
 *	more information.
 * @Parameters: Parameters of the current event. Refer to databook for
 *	more information.
 */
struct XUsbPsu_Event_Epevt {
	u32	Is_EpEvt:1;
	u32	Epnumber:5;
	u32	Endpoint_Event:4;
	u32	Reserved11_10:2;
	u32	Status:4;
	u32	Parameters:16;
} __attribute__((packed));

/**
 * struct XUsbPsu_event_devt - Device Events
 * @Is_DevEvt: indicates this is a non-endpoint event
 * @Device_Event: indicates it's a device event. Should read as 0x00
 * @Type: indicates the type of device event.
 *	0	- DisconnEvt
 *	1	- USBRst
 *	2	- ConnectDone
 *	3	- ULStChng
 *	4	- WkUpEvt
 *	5	- Reserved
 *	6	- EOPF
 *	7	- SOF
 *	8	- Reserved
 *	9	- ErrticErr
 *	10	- CmdCmplt
 *	11	- EvntOverflow
 *	12	- VndrDevTstRcved
 * @Reserved15_12: Reserved, not used
 * @Event_Info: Information about this event
 * @Reserved31_25: Reserved, not used
 */
struct XUsbPsu_Event_Devt {
	u32	Is_DevEvt:1;
	u32	Device_Event:7;
	u32	Type:4;
	u32	Reserved15_12:4;
	u32	Event_Info:9;
	u32	Reserved31_25:7;
} __attribute__((packed));

/**
 * struct XUsbPsu_event_gevt - Other Core Events
 * @one_bit: indicates this is a non-endpoint event (not used)
 * @device_event: indicates it's (0x03) Carkit or (0x04) I2C event.
 * @phy_port_number: self-explanatory
 * @reserved31_12: Reserved, not used.
 */
struct XUsbPsu_Event_Gevt {
	u32	Is_GlobalEvt:1;
	u32	Device_Event:7;
	u32	Phy_Port_Number:4;
	u32	Reserved31_12:20;
} __attribute__((packed));

/**
 * union XUsbPsu_event - representation of Event Buffer contents
 * @raw: raw 32-bit event
 * @type: the type of the event
 * @depevt: Device Endpoint Event
 * @devt: Device Event
 * @gevt: Global Event
 */
union XUsbPsu_Event {
	u32				Raw;
	struct XUsbPsu_Event_Type	Type;
	struct XUsbPsu_Event_Epevt	Epevt;
	struct XUsbPsu_Event_Devt	Devt;
	struct XUsbPsu_Event_Gevt	Gevt;
};

/***************** Macros (Inline Functions) Definitions *********************/

#define IS_ALIGNED(x, a)	(((x) & ((typeof(x))(a) - 1)) == 0U)

#define roundup(x, y) (                                 \
{                                                       \
        const typeof(y) y__ = (y);                        \
        (((x) + (u32)(y__ - 1)) / (u32)y__) * (u32)y__;                \
}                                                       \
)

#define DECLARE_DEV_DESC(Instance, desc)			\
	(Instance).DevDesc = &(desc); 					\
	(Instance).DevDescSize = sizeof((desc))

#define DECLARE_CONFIG_DESC(Instance, desc) 		\
	(Instance).ConfigDesc = &(desc); 				\
	(Instance).ConfigDescSize = sizeof((desc))

static inline void *XUsbPsu_get_drvdata(struct XUsbPsu *InstancePtr) {
	return InstancePtr->data_ptr;
}

static inline void XUsbPsu_set_drvdata(struct XUsbPsu *InstancePtr, void *data) {
	InstancePtr->data_ptr = data;
}

static inline void XUsbPsu_set_ch9handler(
		struct XUsbPsu *InstancePtr,
		void (*func)(struct XUsbPsu *, SetupPacket *)) {
	InstancePtr->Chapter9 = func;
}

static inline void XUsbPsu_set_rsthandler(
		struct XUsbPsu *InstancePtr,
		void (*func)(struct XUsbPsu *)) {
	InstancePtr->ResetIntrHandler = func;
}

static inline void XUsbPsu_set_disconnect(
		struct XUsbPsu *InstancePtr,
		void (*func)(struct XUsbPsu *)) {
	InstancePtr->DisconnectIntrHandler = func;
}

/************************** Function Prototypes ******************************/

/*
 * Functions in xusbpsu.c
 */
s32 XUsbPsu_Wait_Clear_Timeout(struct XUsbPsu *InstancePtr, u32 Offset,
								u32 BitMask, u32 Timeout);
s32 XUsbPsu_Wait_Set_Timeout(struct XUsbPsu *InstancePtr, u32 Offset,
								u32 BitMask, u32 Timeout);
void XUsbPsu_SetMode(struct XUsbPsu *InstancePtr, u32 Mode);
void XUsbPsu_PhyReset(struct XUsbPsu *InstancePtr);
void XUsbPsu_EventBuffersSetup(struct XUsbPsu *InstancePtr);
void XUsbPsu_EventBuffersReset(struct XUsbPsu *InstancePtr);
void XUsbPsu_CoreNumEps(struct XUsbPsu *InstancePtr);
void XUsbPsu_cache_hwparams(struct XUsbPsu *InstancePtr);
u32 XUsbPsu_ReadHwParams(struct XUsbPsu *InstancePtr, u8 RegIndex);
s32 XUsbPsu_CoreInit(struct XUsbPsu *InstancePtr);
void XUsbPsu_EnableIntr(struct XUsbPsu *InstancePtr, u32 Mask);
void XUsbPsu_DisableIntr(struct XUsbPsu *InstancePtr, u32 Mask);
s32 XUsbPsu_CfgInitialize(struct XUsbPsu *InstancePtr,
			XUsbPsu_Config *ConfigPtr, u32 BaseAddress);
s32 XUsbPsu_Start(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_Stop(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_SetTestMode(struct XUsbPsu *InstancePtr, u32 Mode);
XusbPsuLinkState XUsbPsu_GetLinkState(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_SetLinkState(struct XUsbPsu *InstancePtr,
		XusbPsuLinkStateChange State);
s32 XUsbPsu_SendGenericCmd(struct XUsbPsu *InstancePtr,
					s32 Cmd, u32 Param);
void XUsbPsu_SetSpeed(struct XUsbPsu *InstancePtr, u32 Speed);
s32 XUsbPsu_SetDeviceAddress(struct XUsbPsu *InstancePtr, u16 Addr);
s32 XUsbPsu_IsSuperSpeed(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_SetU1SleepTimeout(struct XUsbPsu *InstancePtr, u8 Sleep);
s32 XUsbPsu_SetU2SleepTimeout(struct XUsbPsu *InstancePtr, u8 Sleep);
s32 XUsbPsu_AcceptU1U2Sleep(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_U1SleepEnable(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_U2SleepEnable(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_U1SleepDisable(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_U2SleepDisable(struct XUsbPsu *InstancePtr);

/*
 * Functions in xusbpsu_endpoint.c
 */
struct XUsbPsu_EpParams *XUsbPsu_GetEpParams(struct XUsbPsu *InstancePtr);
u32 XUsbPsu_EpGetTransferIndex(struct XUsbPsu *InstancePtr, u8 UsbEpNum,
				u8 Dir);
const char *XUsbPsu_EpCmdString(u8 Cmd);
s32 XUsbPsu_SendEpCmd(struct XUsbPsu *InstancePtr, u8 UsbEpNum, u8 Dir,
			u32 Cmd, struct XUsbPsu_EpParams *Params);
s32 XUsbPsu_StartEpConfig(struct XUsbPsu *InstancePtr, u32 UsbEpNum,
				u8 Dir);
s32 XUsbPsu_SetEpConfig(struct XUsbPsu *InstancePtr, u8 UsbEpNum, u8 Dir,
				u16 Size, u8 Type);
s32 XUsbPsu_SetXferResource(struct XUsbPsu *InstancePtr, u8 UsbEpNum, u8 Dir);
s32 XUsbPsu_EpEnable(struct XUsbPsu *InstancePtr, u8 UsbEpNum, u8 Dir,
			u16 Maxsize, u8 Type);
s32 XUsbPsu_EpDisable(struct XUsbPsu *InstancePtr, u8 UsbEpNum, u8 Dir);
s32 XUsbPsu_EnableControlEp(struct XUsbPsu *InstancePtr, u16 Size);
void XUsbPsu_InitializeEps(struct XUsbPsu *InstancePtr);
void XUsbPsu_StopTransfer(struct XUsbPsu *InstancePtr, u8 UsbEpNum, u8 Dir);
void XUsbPsu_ClearStalls(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_EpBufferSend(struct XUsbPsu *InstancePtr, u8 UsbEp,
			u8 *BufferPtr, u32 BufferLen);
s32 XUsbPsu_EpBufferRecv(struct XUsbPsu *InstancePtr, u8 UsbEp,
				u8 *BufferPtr, u32 Length);
void XUsbPsu_EpSetStall(struct XUsbPsu *InstancePtr, u8 Epnum, u8 Dir);
void XUsbPsu_EpClearStall(struct XUsbPsu *InstancePtr, u8 Epnum, u8 Dir);
void XUsbPsu_SetEpHandler(struct XUsbPsu *InstancePtr, u8 Epnum,
			u8 Dir, void (*Handler)(void *, u32, u32));
s32 XUsbPsu_IsEpStalled(struct XUsbPsu *InstancePtr, u8 Epnum, u8 Dir);
void XUsbPsu_EpXferComplete(struct XUsbPsu *InstancePtr,
							const struct XUsbPsu_Event_Epevt *Event);

/*
 * Functions in xusbpsu_controltransfers.c
 */
s32 XUsbPsu_RecvSetup(struct XUsbPsu *InstancePtr);
void XUsbPsu_Ep0StallRestart(struct XUsbPsu *InstancePtr);
s32 XUsbPsu_SetConfiguration(struct XUsbPsu *InstancePtr,
				SetupPacket *Ctrl);
void XUsbPsu_Ep0DataDone(struct XUsbPsu *InstancePtr,
		const struct XUsbPsu_Event_Epevt *Event);
void XUsbPsu_Ep0StatusDone(struct XUsbPsu *InstancePtr,
		const struct XUsbPsu_Event_Epevt *Event);
void XUsbPsu_Ep0XferComplete(struct XUsbPsu *InstancePtr,
			const struct XUsbPsu_Event_Epevt *Event);
s32 XUsbPsu_Ep0StartStatus(struct XUsbPsu *InstancePtr,
				const struct XUsbPsu_Event_Epevt *Event);
void XUsbPsu_Ep0_EndControlData(struct XUsbPsu *InstancePtr,
					struct XUsbPsu_Ep *Ept);
void XUsbPsu_Ep0XferNotReady(struct XUsbPsu *InstancePtr,
		const struct XUsbPsu_Event_Epevt *Event);
void XUsbPsu_Ep0Intr(struct XUsbPsu *InstancePtr,
		const struct XUsbPsu_Event_Epevt *Event);
s32 XUsbPsu_Ep0Send(struct XUsbPsu *InstancePtr, u8 *BufferPtr,
			u32 BufferLen);
s32 XUsbPsu_Ep0Recv(struct XUsbPsu *InstancePtr, u8 *BufferPtr, u32 Length);
void XUsbSleep(u32 USeconds);

/*
 * Functions in xusbpsu_intr.c
 */
void XUsbPsu_EpInterrupt(struct XUsbPsu *InstancePtr,
		const struct XUsbPsu_Event_Epevt *Event);
void XUsbPsu_DisconnectIntr(struct XUsbPsu *InstancePtr);
void XUsbPsu_ResetIntr(struct XUsbPsu *InstancePtr);
void XUsbPsu_ConnDoneIntr(struct XUsbPsu *InstancePtr);
void XUsbPsu_LinkStsChangeIntr(struct XUsbPsu *InstancePtr,
				u32 EvtInfo);
void XUsbPsu_DevInterrupt(struct XUsbPsu *InstancePtr,
		const struct XUsbPsu_Event_Devt *Event);
void XUsbPsu_EventHandler(struct XUsbPsu *InstancePtr,
                const union XUsbPsu_Event *Event);
void XUsbPsu_EventBufferHandler(struct XUsbPsu *InstancePtr);
void XUsbPsu_IntrHandler(void *XUsbPsuInstancePtr);

/*
 * Functions in xusbpsu_sinit.c
 */
XUsbPsu_Config *XUsbPsu_LookupConfig(u16 DeviceId);

#ifdef __cplusplus
}
#endif

#endif  /* End of protection macro. */
/** @} */
