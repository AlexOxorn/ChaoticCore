//
// Created by alexoxorn on 4/25/24.
//

#include "internal_common.h"
#include "protocol_buffers/messages.pb.h"

void write_coord_info(CC_coordinate* msg, sequence_type seq) {
    msg->set_horizontal(seq.horizontal);
    msg->set_vertical(seq.vertical);
    msg->set_type(seq.type);
}

void write_sequence_info(CC_sequence* msg, sequence_type seq, LOCATION loc) {
    if (loc == LOCATION::FIELD) {
        msg->mutable_coord()->set_horizontal(seq.horizontal);
        msg->mutable_coord()->set_vertical(seq.vertical);
    } else {
        msg->set_index(seq.index);
    }
}

void write_location_info(CC_locinfo* msg, loc_info info) {
    msg->set_controller(+info.controller);
    msg->set_location(+info.location);
    msg->set_position(+info.position);
    write_sequence_info(msg->mutable_sequence(), info.sequence, info.location);
}


