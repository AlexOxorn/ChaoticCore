//
// Created by alexoxorn on 5/2/24.
//

#ifndef CHAOTIC_CORE_CHAOTC_API_TYPES_H
#define CHAOTIC_CORE_CHAOTC_API_TYPES_H

#include <cstdint>
#include "common.h"

typedef void* CHAOTIC_Duel;

typedef struct CHAOTIC_CardData {
    uint32_t code;
    uint32_t supercode;
    uint32_t subcode;
    uint32_t supertype;
    uint64_t subtype;
    uint8_t tribe;
    uint32_t courage;
    uint32_t power;
    uint32_t wisdom;
    uint32_t speed;
    uint32_t energy;
    uint8_t fire;
    uint8_t air;
    uint8_t earth;
    uint8_t water;
    uint8_t mugic_ability;
    uint8_t loyal;
    uint8_t limited;
    uint8_t legendary;
}CHAOTIC_CardData;

typedef struct CHAOTIC_NewCardInfo {
    uint32_t code; // card name
    SUPERTYPE supertype;
    PLAYER controller; // who controls card
    PLAYER owner; // original owner
    LOCATION location; // new location
    sequence_type sequence;
    POSITION position;
}CHAOTIC_NewCardInfo;

typedef struct CHAOTIC_Player {
}CHAOTIC_Player;

typedef void (*CHAOTIC_DataReader)(void* payload, uint32_t code, CHAOTIC_CardData* data);
typedef void (*CHAOTIC_DataReaderDone)(void* payload, CHAOTIC_CardData* data);
typedef int (*CHAOTIC_ScriptReader)(void* payload, CHAOTIC_Duel duel, const char* name);
typedef void (*CHAOTIC_LogHandler)(void* payload, const char* string, int type);

typedef struct CHAOTIC_DuelOptions {
    uint64_t seed;
    uint8_t columns;
    CHAOTIC_Player team1;
    CHAOTIC_Player team2;
    CHAOTIC_DataReader cardReader;
    void* cardReaderPayload;
    CHAOTIC_ScriptReader scriptReader;
    void* scriptReaderPayload;
    CHAOTIC_LogHandler logHandler;
    void* logHandlerPayload;
    CHAOTIC_DataReaderDone cardReaderDone;
    void* cardReaderDonePayload;
} CHAOTIC_DuelOptions;

#endif // CHAOTIC_CORE_CHAOTC_API_TYPES_H
