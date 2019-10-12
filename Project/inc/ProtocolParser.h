#ifndef __PROTOCOL_PARSER_H
#define __PROTOCOL_PARSER_H

#include "_global.h"
#include "ProtocolBus.h"

extern bool bDelaySend;
extern uint16_t delaySendTick;

uint8_t ParseProtocol();
void Msg_RequestNodeID();
void Msg_Presentation();
void Msg_RequestDeviceStatus();
void Msg_DevOnOffDelay(uint8_t _sw,uint8_t _br,uint8_t _unit,uint8_t time,bool isGradual);
void Msg_DevOnOff(uint8_t _sw);
void Msg_SpecialDevOnOff(uint8_t nodeid,uint8_t subid,uint8_t _sw);
void Msg_RelayOnOff(uint8_t _sw);
void Msg_DevBrightness(uint8_t _op, uint8_t _br);
void Msg_DevCCT(uint8_t _op, uint16_t _cct);
void Msg_DevBR_CCT(uint8_t _br, uint16_t _cct);
void Msg_DevBR_RGBW(uint8_t _br, uint8_t _r, uint8_t _g, uint8_t _b, uint16_t _w);
void Msg_DevScenario(uint8_t _scenario);
void Msg_DevSpecialEffect(uint8_t _effect);
void Msg_PPT_ObjAction(uint8_t _obj, uint8_t _action);
void Msg_Pairing();

void Msg_RemoteKeyTable();

void Msg_RemoteKey(uint8_t btn,uint8_t op);
void Msg_Scenario(uint8_t btn);
#endif /* __PROTOCOL_PARSER_H */