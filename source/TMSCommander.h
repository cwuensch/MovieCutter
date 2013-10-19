#ifndef TMSCMDR_H
#define TMSCMDR_H

#define EVT_TMSCommander    0xB00   // param1 = tTMSCMDR_Commands as below
#define TMSCMDRTAPID		0x8001ffff  //this is used to communicate with TMSCommander

typedef enum
{
	TMSCMDR_UnknownFunction,
	TMSCMDR_Stop,
	TMSCMDR_Menu,
	TMSCMDR_Capabilities,
	TMSCMDR_IsAlive,
	TMSCMDR_Exiting,
	TMSCMDR_UserEvent

} tTMSCMDR_Commands;

// bit setting
typedef enum
{
    TMSCMDR_HaveMenu=0x1000001,
    TMSCMDR_CanBeStopped=0x1000002,
    TMSCMDR_HaveUserEvent=0x1000004

} tTMSCMDR_Capabilities;

  
typedef enum
{
    TMSCMDR_NotOK,
    TMSCMDR_OK

} tTMSCMDR_Status;

#endif

