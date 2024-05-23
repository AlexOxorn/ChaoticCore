//
// Created by alexoxorn on 5/13/24.
//

#include "../common.h"
#include "../chaotic_api_types.h"
#include "../chaotic_api.h"
#include "../protocol_buffers/messages.pb.h"
#include <sqlite3.h>
#include <cstdio>
#include <cstdlib>
#include <string>

sqlite3* database;

SUPERTYPE types[] = {
        SUPERTYPE::CREATURE, SUPERTYPE::BATTLE_GEAR, SUPERTYPE::MUGIC, SUPERTYPE::ATTACK, SUPERTYPE::LOCATION};
TRIBE tribes[] = {
        TRIBE::NONE, TRIBE::OVER_WORLD, TRIBE::UNDER_WORLD, TRIBE::MIPEDIAN, TRIBE::DANIAN, TRIBE::MARRILLIAN};

void read_card(void* db_v, uint32_t code, CHAOTIC_CardData* data) {
    auto* db = (sqlite3*) db_v;
    static const char query[] = "SELECT * FROM cards WHERE ID = ?;;";
    sqlite3_stmt* stmt;
    sqlite3_prepare(db, query, sizeof(query), &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, code);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        printf("%s\n", sqlite3_errmsg(db));
        return;
    }
    data->code = code;
    data->supercode = sqlite3_column_int(stmt, 1);
    data->subcode = sqlite3_column_int(stmt, 2);
    data->supertype = (uint32_t) types[sqlite3_column_int(stmt, 3)];
    data->tribe = (uint32_t) tribes[sqlite3_column_int(stmt, 4)];
    if (data->supertype == +SUPERTYPE::CREATURE) {
        data->courage = sqlite3_column_int(stmt, 6);
        data->power = sqlite3_column_int(stmt, 7);
        data->wisdom = sqlite3_column_int(stmt, 8);
        data->speed = sqlite3_column_int(stmt, 9);
    }

    data->energy = sqlite3_column_int(stmt, 10);
    data->fire = (int8_t)(sqlite3_column_type(stmt, 11) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 11));
    data->air = (int8_t)(sqlite3_column_type(stmt, 12) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 12));
    data->earth = (int8_t)(sqlite3_column_type(stmt, 13) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 13));
    data->water = (int8_t)(sqlite3_column_type(stmt, 14) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 14));
    data->mugic_ability = (int8_t)(sqlite3_column_type(stmt, 15) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 15));
    data->subtype = sqlite3_column_int(stmt, 16);
    data->loyal = sqlite3_column_int(stmt, 17);
    data->limited = sqlite3_column_int(stmt, 17);
    data->legendary = sqlite3_column_int(stmt, 19);
    sqlite3_finalize(stmt);
}

int main() {
    system("pwd");
    int x = sqlite3_open("file:../assets/chaoticdata.db?mode=ro", &database);
    if (x != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(database));
        return 1;
    }

    CHAOTIC_DuelOptions options{.seed = 0,
                                .columns = 3,
                                .team1 = CHAOTIC_Player{},
                                .team2 = CHAOTIC_Player{},
                                .cardReader = read_card,
                                .cardReaderPayload = database,
                                .scriptReader = nullptr};

    CHAOTIC_NewCardInfo data{
            .code = 65537,
            .supertype = SUPERTYPE::CREATURE,
            .controller = PLAYER::ONE,
            .location = LOCATION::FIELD,
            .sequence = {0, 0, 0},
            .position = POSITION::FACE_UP
    };
    CHAOTIC_NewCardInfo data2{
            .code = 65537,
            .supertype = SUPERTYPE::CREATURE,
            .controller = PLAYER::TWO,
            .location = LOCATION::FIELD,
            .sequence = {1, 0, 0},
            .position = POSITION::FACE_UP
    };
    CHAOTIC_NewCardInfo attack{
            .code = 458840,
            .supertype = SUPERTYPE::ATTACK,
            .controller = PLAYER::ONE,
            .location = LOCATION::ATTACK_DECK,
            .sequence = {0, 0, 0},
            .position = POSITION::FACE_DOWN
    };

    CHAOTIC_Duel match;
    CHAOTIC_CreateDuel(&match, options);
    CHAOTIC_DuelNewCard(match, data);
    CHAOTIC_DuelNewCard(match, data2);

    CHAOTIC_DuelNewCard(match, attack);
    CHAOTIC_DuelNewCard(match, attack);
    CHAOTIC_DuelNewCard(match, attack);
    CHAOTIC_DuelNewCard(match, attack);
    attack.controller = PLAYER::TWO;
    CHAOTIC_DuelNewCard(match, attack);
    CHAOTIC_DuelNewCard(match, attack);
    CHAOTIC_DuelNewCard(match, attack);

    CHAOTIC_StartDuel(match);
    while (true) {
        int res = CHAOTIC_DuelProcess(match);
        if (res == CHAOTIC_DUEL_STATUS_END) {
            break;
        }
        uint32_t len;
        char* messages = (char*)CHAOTIC_DuelGetMessage(match, &len);
        uint32_t read = 0;
        while(read < len) {
            int32_t subsize = *((int32_t*)(messages + read));
            read += sizeof(subsize);

            uint8_t message_type = *((uint32_t*)(messages + read));
            read += 1;

            std::string message_data(messages + read, subsize);

            switch (message_type) {
                case 0: {
                        MSG_ShuffleAttackHand msg;
                        msg.ParseFromArray(messages + read, subsize);
                        std::cout << "SHUFFLE AD " << msg.ShortDebugString() << std::endl;
                        break;
                    }
                case 1: {
                        MSG_ShuffleLocationDeck msg;
                        msg.ParseFromArray(messages + read, subsize);
                        std::cout << "SHUFFLE LD " << msg.ShortDebugString() << std::endl;
                        break;
                    }
                case 2: {
                        MSG_ShuffleAttackHand msg;
                        msg.ParseFromArray(messages + read, subsize);
                        std::cout << "SHUFFLE AH " << msg.ShortDebugString() << std::endl;
                        break;
                    }
                case 3: {
                        MSG_ShuffleMugicHand msg;
                        msg.ParseFromArray(messages + read, subsize);
                        std::cout << "SHUFFLE MH " << msg.ShortDebugString() << std::endl;
                        break;
                    }
                case 4: {
                        MSG_Move msg;
                        msg.ParseFromArray(messages + read, subsize);
                        std::cout << "MOVE " << msg.ShortDebugString() << std::endl;
                        break;
                    }
                case 5: {
                        MSG_Draw msg;
                        msg.ParseFromArray(messages + read, subsize);
                        std::cout << "DRAW " << msg.ShortDebugString() << std::endl;
                        break;
                    }
                default: {
                        std::cout << "ERROR" << std::endl;
                    }
            }
            read += subsize;
        }
        if (res == CHAOTIC_DUEL_STATUS_AWAITING) {
            int size;
            std::cin >> size;
            std::vector<int32_t> return_data(size);
            for (int i = 0; i < size; ++i) {
                std::cin >> return_data[i];
            }
            CHAOTIC_DuelSetResponse(match, return_data.data(), return_data.size() * sizeof(return_data[0]));
        }
    }


    CHAOTIC_DestroyDuel(match);
}