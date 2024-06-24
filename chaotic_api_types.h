//
// Created by alexoxorn on 5/2/24.
//

#ifndef CHAOTIC_CORE_CHAOTC_API_TYPES_H
#define CHAOTIC_CORE_CHAOTC_API_TYPES_H

#include <cstdint>
#include "common.h"

typedef void* CHAOTIC_Duel;

typedef struct CHAOTIC_CardData {
    int code;
    int supercode;
    int subcode;
    int supertype;
    int subtype;
    int tribe;
    int courage;
    int power;
    int wisdom;
    int speed;
    int energy;
    int fire;
    int air;
    int earth;
    int water;
    int mugic_ability;
    int loyal;
    int limited;
    int legendary;
} CHAOTIC_CardData;

typedef union {
    struct {
        int32_t seq_horizontal;
        int32_t seq_vertical;
        int32_t seq_type;
    };
    int32_t seq_index;
} c_sequence;

typedef struct CHAOTIC_NewCardInfo {
    uint32_t code; // card name
    SUPERTYPE supertype;
    PLAYER controller; // who controls card
    PLAYER owner;      // original owner
    LOCATION location; // new location
    c_sequence sequence;
    POSITION position;
} CHAOTIC_NewCardInfo;

typedef struct CHAOTIC_Player {
} CHAOTIC_Player;

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

typedef enum CHAOTIC_DuelStatus {
    CHAOTIC_DUEL_STATUS_END,
    CHAOTIC_DUEL_STATUS_AWAITING,
    CHAOTIC_DUEL_STATUS_CONTINUE
} CHAOTIC_DuelStatus;

typedef struct CHAOTIC_QueryInfo {
    QUERY_FLAGS flags;
    PLAYER con;
    LOCATION loc;
    c_sequence seq;
} CHAOTIC_QueryInfo;

#endif // CHAOTIC_CORE_CHAOTC_API_TYPES_H
