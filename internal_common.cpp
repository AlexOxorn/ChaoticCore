//
// Created by alexoxorn on 4/25/24.
//

#include "internal_common.h"
#include "protocol_buffers/messages.pb.h"

void write_location_info(MSG_locinfo* msg, loc_info info) {
    msg->set_controller(+info.controller);
    msg->set_location(+info.location);
    msg->set_position(+info.position);
    if (info.location == LOCATION::FIELD) {
        msg->mutable_sequence()->mutable_coord()->set_horizontal(info.sequence.horizontal);
        msg->mutable_sequence()->mutable_coord()->set_vertical(info.sequence.vertical);
    }
    msg->mutable_sequence()->mutable_index()->set_index(info.sequence.index);
}

class card;
using card_vector = std::vector<card*>;

