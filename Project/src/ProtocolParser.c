#include "ProtocolParser.h"
#include "_global.h"
#include "MyMessage.h"
#include "xliNodeConfig.h"
#include "button.h"
#include "rf24l01.h"

uint16_t delaySendTick = 0;
bool bDelaySend = FALSE;

/*// Assemble message
void build(uint8_t _destination, uint8_t _sensor, uint8_t _command, uint8_t _type, bool _enableAck, bool _isAck)
{
    sndMsg.header.version_length = PROTOCOL_VERSION;
    sndMsg.header.sender = gConfig.nodeID;
    sndMsg.header.destination = _destination;
    sndMsg.header.sensor = _sensor;
    sndMsg.header.type = _type;
    moSetCommand(_command);
    moSetRequestAck(_enableAck);
    moSetAck(_isAck);
#ifdef ENABLE_SDTM
    memcpy(sndMsg.header.uniqueid,_uniqueID,UNIQUE_ID_LEN);
#endif
}*/


bool NeedProcess(uint8_t *arrType,uint8_t num)
{
  bool bNeedProcess = FALSE;
  for(uint8_t tidx = 0; tidx < num; tidx++ )
  {
    if(*(arrType+tidx) == gConfig.type) 
    {
      bNeedProcess = TRUE;
      break;
    }
  }
  return bNeedProcess;
}
    
uint8_t ParseProtocol(){
  if( rcvMsg.header.destination != gConfig.nodeID && rcvMsg.header.destination != BROADCAST_ADDRESS ) return 0;
  
  uint8_t ret = ParseCommonProtocol();
  if(ret) return 1;
  
  uint8_t _cmd = miGetCommand();
  uint8_t _sender = rcvMsg.header.sender;  // The original sender
  uint8_t _type = rcvMsg.header.type;
  uint8_t _sensor = rcvMsg.header.sensor;
  uint8_t _lenPayl = miGetLength();
  bool _needAck = (bool)miGetRequestAck();
  bool _isAck = (bool)miGetAck();
  
  switch( _cmd ) {
  /*case C_INTERNAL:
    if( _type == I_ID_RESPONSE ) {
    } else if( _type == I_CONFIG ) {
      // Node Config
      switch( _sensor ) {
      case NCF_QUERY:
        // Inform controller with version & NCF data
        Msg_NodeConfigData(_sender);
        return 1;
        break;

      case NCF_DEV_SET_SUBID:    
        break;
        
      case NCF_DEV_MAX_NMRT:
        gConfig.rptTimes = rcvMsg.payload.data[0];   
        break;
      case NCF_DATA_FN_HUE:
        break;
      case NCF_DEV_SET_RF:
        if( _lenPayl >= 1)
        {
           // Node base rf info Config
          uint8_t newNid = rcvMsg.payload.data[0];
          if(newNid !=0 && !isNodeIdInvalid(newNid))
          {
            gConfig.nodeID = newNid;
            gResetRF = TRUE;
          }
        }
        if(_lenPayl >= 2) 
        {
          uint8_t rfchannel = rcvMsg.payload.data[1];
          if(rfchannel != 0)
          {
            gConfig.rfChannel = rfchannel;
            gResetRF = TRUE;
          }
        }
        if(_lenPayl >= 3)
        {
          uint8_t subid = rcvMsg.payload.data[2];
          gConfig.subID = subid;
        }
        if(_lenPayl >= 4)
        {
          uint8_t netlen = _lenPayl - 3;
          uint8_t bnetvalid = 0;
          for(uint8_t i = 0;i<netlen;i++)
          {
            if(rcvMsg.payload.data[3+i] != 0) 
            {
              bnetvalid = 1;
              break;
            }
          }
          if(bnetvalid)
          {
            memset(gConfig.NetworkID,0x00,sizeof(gConfig.NetworkID));
            for(uint8_t j = 0;j<netlen;j++)
            {
              gConfig.NetworkID[4-j] = rcvMsg.payload.data[3+j];
            }
            gResetRF = TRUE;
          }
        }
        break;
      }
      gIsConfigChanged = TRUE;
      Msg_NodeConfigAck(_sender, _sensor);
      return 1;
    }else if( _type == I_GET_NONCE ) {
      // RF Scanner Probe
        if( _sender == NODEID_RF_SCANNER ) {
          uint8_t _lenPayl = miGetLength();
          if( rcvMsg.payload.data[0] == SCANNER_PROBE ) {      
            MsgScanner_ProbeAck();
          } else if( rcvMsg.payload.data[0] == SCANNER_SETUP_RF ) {
          if(!IS_MINE_SUBID(_sensor)) return 0;  
            Process_SetupRF(rcvMsg.payload.data + 1,_lenPayl-1);
          }else if( rcvMsg.payload.data[0] == SCANNER_SETUPDEV_RF ) {
            if(!isIdentityEqual(rcvMsg.payload.data + 1,_uniqueID,UNIQUE_ID_LEN)) return 0;
            Process_SetupRF(rcvMsg.payload.data + 1 + UNIQUE_ID_LEN,_lenPayl-1 - UNIQUE_ID_LEN);
          }
          else if( rcvMsg.payload.data[0] == SCANNER_SETCONFIG ) {
            
            if(!IS_MINE_SUBID(_sensor)) return 0;          
            uint8_t cfg_len = _lenPayl - 2;
            Process_SetConfig(cfg_len);
          }
          else if( rcvMsg.payload.data[0] == SCANNER_SETDEV_CONFIG ) {  
            if(!isIdentityEqual(rcvMsg.payload.data + 2,_uniqueID,UNIQUE_ID_LEN)) return 0;
            uint8_t cfg_len = _lenPayl - 10;
            Process_SetDevConfig(cfg_len);
          }
          else if( rcvMsg.payload.data[0] == SCANNER_GETDEV_CONFIG ) {  
            uint8_t offset = rcvMsg.payload.data[1];
            uint8_t cfgblock_len = rcvMsg.payload.data[10];
            if(!isIdentityEqual(rcvMsg.payload.data + 2,_uniqueID,UNIQUE_ID_LEN)) return 0;
            MsgScanner_ConfigAck(offset,cfgblock_len,TRUE); 
          }
          else if( rcvMsg.payload.data[0] == SCANNER_GETCONFIG ) { 
            if(!IS_MINE_SUBID(_sensor)) return 0;  
            uint8_t offset = rcvMsg.payload.data[1];
            uint8_t cfgblock_len = rcvMsg.payload.data[2];
            MsgScanner_ConfigAck(offset,cfgblock_len,FALSE);
          }
          return 1;
        }      
      }
    break;*/
      
  case C_PRESENTATION:
    if( _sensor == S_DIMMER ) {
    }
    break;
  
  case C_REQ:
    if( _needAck ) {
      if( IS_MINE_SUBID(_sensor) ) {
        if( _type == V_STATUS ) {
/*
//typedef struct
//{
//    //uint8_t devNum;
//    uint8_t devType1;
//    uint8_t devType2;
//    uint8_t devType3;
      ...
//    uint8_t devType5;
//}MyMsgPayload_t  
*/ 
          bool bNeedProcess = TRUE;
          if(rcvMsg.header.destination == 0xFF)
          {
            uint8_t devTypeNum = _lenPayl;
            if(devTypeNum > 0)
            {
              bNeedProcess = NeedProcess(&rcvMsg.payload.data[0],devTypeNum);
            }
          }
          if(bNeedProcess)
          {
            MsgScanner_ProbeAck(_sender);
            bDelaySend = (rcvMsg.header.destination == BROADCAST_ADDRESS);
            if(rcvMsg.header.destination == BROADCAST_ADDRESS)
            {
              delaySendTick = GetDelayTick(rcvMsg.header.destination);
            }
            else
            {
              delaySendTick = 0;
            }
          }          
          return 0;
        }
      }
    }    
    break;  
  case C_SET:
    if( _isAck ) {
    }    
    break;
  }
  
  return 0;
}

void Msg_RequestNodeID() {
  // Request NodeID for remote
  build(BASESERVICE_ADDRESS, NODE_TYP_REMOTE, C_INTERNAL, I_ID_REQUEST, 1, 0);
  moSetPayloadType(P_ULONG32);
  moSetLength(UNIQUE_ID_LEN);
  memcpy(sndMsg.payload.data, _uniqueID, UNIQUE_ID_LEN);
  bMsgReady = 1;
}

// Prepare device presentation message
void Msg_Presentation() {
  build(NODEID_GATEWAY, S_ZENREMOTE, C_PRESENTATION, gConfig.type, 1, 0);
  moSetPayloadType(P_ULONG32);
  moSetLength(UNIQUE_ID_LEN);
  memcpy(sndMsg.payload.data, _uniqueID, UNIQUE_ID_LEN);
  bMsgReady = 1;
}

// msg of remote key operation
void Msg_RemoteKey(uint8_t btn,uint8_t op) {
  build(NODEID_GATEWAY, gConfig.subID, C_REQ, V_REMOTE_KEY, 0, 0);
  moSetPayloadType(P_BYTE);
  moSetLength(2);
  sndMsg.payload.data[0] = btn_map[btn];
  sndMsg.payload.data[1] = op;
  bMsgReady = 1;
}

// msg of remote key operation
void Msg_RemoteKeyTable() {
  build(NODEID_GATEWAY, gConfig.subID, C_REQ, V_REMOTE_KEY, 0, 0);
  moSetPayloadType(P_BYTE);
  uint8_t len = 0;
  for(uint8_t i = 0; i<keylstDummy;i++)
  {
    if(btn_action[i] >=1)
    {
      sndMsg.payload.data[len++] = btn_map[i];
      sndMsg.payload.data[len++] = btn_action[i]-1;
    }
  }
  moSetLength(len);
  bMsgReady = 1;
}


// msg of scenario
void Msg_Scenario(uint8_t btn) {
#ifdef SCENARIO_PANEL
  build(NODEID_GATEWAY, gConfig.subID, C_SET, V_SCENE_ON, 1, 0);
  moSetPayloadType(P_BYTE);
  moSetLength(1);
  uint8_t len = 0;
  sndMsg.payload.data[0]=btn_scenariomap[btn];
  bMsgReady = 1;
#endif
}

// Set current device brightness
void Msg_DevBrightness(uint8_t _op, uint8_t _br) {
  build(BROADCAST_ADDRESS, gConfig.subID, C_SET, V_PERCENTAGE, 1, 0);
  moSetLength(2);
  moSetPayloadType(P_BYTE);
  sndMsg.payload.data[0] = _op;
  sndMsg.payload.data[1] = _br;
  bMsgReady = 1;
}

// Set current device CCT
void Msg_DevCCT(uint8_t _op, uint16_t _cct) {
  build(BROADCAST_ADDRESS, gConfig.subID, C_SET, V_LEVEL, 1, 0);
  moSetLength(3);
  moSetPayloadType(P_UINT16);
  sndMsg.payload.data[0] = _op;
  sndMsg.payload.data[1] = _cct % 256;
  sndMsg.payload.data[2] = _cct / 256;
  bMsgReady = 1;
}
