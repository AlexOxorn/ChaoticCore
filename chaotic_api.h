//
// Created by alexoxorn on 4/29/24.
//

#ifndef CHAOTIC_CORE_CHAOTIC_API_H
#define CHAOTIC_CORE_CHAOTIC_API_H

#include "chaotic_api_types.h"

#ifdef __cplusplus
  #define EXTERN_C extern "C"
#else
  #define EXTERN_C
#endif

#if defined(CHAOTICCORE_EXPORT_FUNCTIONS)
  #if defined(_WIN32)
    #define CHAOTICAPI EXTERN_C __declspec(dllexport)
  #else
    #define CHAOTICAPI EXTERN_C __attribute__ ((visibility ("default")))
  #endif
#else
  #define CHAOTICAPI EXTERN_C
#endif


CHAOTICAPI int CHAOTIC_CreateDuel(CHAOTIC_Duel* out_chaotic_duel, CHAOTIC_DuelOptions options);
CHAOTICAPI void CHAOTIC_DestroyDuel(CHAOTIC_Duel chaotic_duel);
CHAOTICAPI void CHAOTIC_DuelNewCard(CHAOTIC_Duel chaotic_duel, CHAOTIC_NewCardInfo info);
CHAOTICAPI void CHAOTIC_StartDuel(CHAOTIC_Duel chaotic_duel);
CHAOTICAPI void CHAOTIC_PrintBoard(CHAOTIC_Duel chaotic_duel);

CHAOTICAPI int CHAOTIC_DuelProcess(CHAOTIC_Duel chaotic_duel);
CHAOTICAPI void* CHAOTIC_DuelGetMessage(CHAOTIC_Duel chaotic_duel, uint32_t* length);
CHAOTICAPI void CHAOTIC_DuelSetResponse(CHAOTIC_Duel chaotic_duel, const void* buffer, uint32_t length);

#endif//CHAOTIC_CORE_CHAOTIC_API_H
