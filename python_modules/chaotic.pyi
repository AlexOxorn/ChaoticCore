from typing import Callable, Tuple, List, Optional
import common as cc
from query_pb2 import QUERY_CardList, QUERY_CardInfo, QUERY_FieldData
from google.protobuf.message import Message as MSG

type cardReaderReturn = Tuple[
    int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int]

type Sequence = int | Tuple[int, int] | Tuple[int, int, int]


class Chaotic[cardReaderPayloadType]:
    def __init__(self,
                 seed: int,
                 columns: int,
                 cardReader: Callable[[cardReaderPayloadType, int], Tuple[int,...]],
                 scriptReader: Callable[[int], None] = None,
                 logHandler = None,
                 cardReaderDone = None,
                 cardReaderPayload: cardReaderPayloadType = None,
                 scriptReaderPayload = None,
                 logHandlerPayload = None,
                 cardReaderDonePayload = None
                 ) -> None: ...

    def new_card(self,
                 code: int,
                 supertype: cc.SUPERTYPE | int,
                 controller: int,
                 owner: int,
                 location: cc.LOCATION | int,
                 sequence: Sequence,
                 position: cc.POSITION | int
                 ) -> None: ...

    def start_duel(self) -> None: ...

    def process(self) -> int: ...

    def messages(self) -> List[MSG]: ...

    def message_group(self) -> Tuple[int, List[MSG]]: ...

    def respond(self, response: List[int]) -> None: ...

    def respond_and_get(self, response: Optional[List[int]] = None) -> Tuple[int, List[MSG]]: ...

    def print_board(self) -> None: ...

    def query_count(self, player: int, location: cc.LOCATION | int) -> int: ...

    def query(self,
              queries: cc.QUERY_FLAGS | int,
              player: int,
              location: cc.LOCATION | int,
              sequence: Sequence = 0
              ) -> QUERY_CardInfo: ...

    def query_location(self,
                       queries: cc.QUERY_FLAGS | int,
                       player: int,
                       location: cc.LOCATION | int,
                       sequence: Sequence = 0
                       ) -> QUERY_CardList: ...

    def query_field(self) -> QUERY_FieldData: ...

    def __enter__(self) -> Chaotic: ...

    def __exit__(self, exc_type, exc_val, exc_tb) -> None: ...
