import sys
import time

import chaotic
import messages_pb2 as msg
import sqlite3
import traceback
from functools import cache
from enum import Enum
import re
from google.protobuf import text_format


def BIT(x: int):
    return 1 << (x - 1)


class LOCATION:
    NONE = 0,
    ATTACK_DECK = BIT(1)
    ATTACK_DISCARD = BIT(2)
    FIELD = BIT(3)
    MUGIC_HAND = BIT(4)
    ATTACK_HAND = BIT(5)
    GENERAL_DISCARD = BIT(6)
    REMOVED = BIT(7)
    BURST = BIT(8)
    ACTIVE_LOCATION = BIT(9)
    LOCATION_DECK = BIT(10)


class SUPERTYPE:
    CREATURE = 0
    BATTLE_GEAR = 1
    MUGIC = 2
    LOCATION = 3
    ATTACK = 4


def card_reader(conn: sqlite3.Connection, code: int):
    curr = conn.cursor()
    curr.execute("""
            SELECT id,
               name,
               subname,
               cardtype,
               archetype,
               tribe,
               courage,
               power,
               wisdom,
               speed,
               energy,
               COALESCE(fire, -1),
               COALESCE(air, -1),
               COALESCE(earth, -1),
               COALESCE(water, -1),
               mugic,
               loyal,
               limited,
               legendary
        FROM   cards
        WHERE  id = ?""", (code,))
    data = tuple(x or 0 for x in curr.fetchone())
    # formatted_data = dict(zip(map(lambda a: a[0], curr.description), data))
    # from pprint import pprint
    # pprint(formatted_data)
    # print(data)
    return data


c = sqlite3.connect('../assets/chaoticdata.db')
# c.set_trace_callback(print)



code_replace = re.compile(r'code: (\d+)')


def match_code(matchobj):
    name1, name2 = get_name(matchobj.group(1))
    if name2:
        return f'code: "{name1}: {name2}"'
    return f'code: "{name1}"'


def print_message(msg, one_line=True):
    base = f"<{type(msg).__name__}({text_format.MessageToString(msg, as_one_line=one_line)})>"
    print(code_replace.sub(match_code, base))


def replace_str(name):
    return f"lower(replace(replace(replace({name}, ' ', ''), ',', ''), '\'\'', ''))"

@cache
def get_id(name: str):
    x = c.execute(f"""
        SELECT ID FROM text_en WHERE 
            CASE
            WHEN NAME2 IS NULL THEN {replace_str('NAME1')} 
            ELSE {replace_str('NAME1')} || {replace_str('NAME2')}
            END = ?
    """, (name,))
    data = x.fetchone()
    return data[0]


@cache
def get_name(id: int):
    x = c.execute(f"""
        SELECT NAME1, NAME2 FROM text_en WHERE ID = ?
    """, (id,))
    data = x.fetchone()
    return data


def triangle_pos(size: int):
    for y in range(size):
        for x in range(y + 1):
            yield y, x


def triangle(size: int):
    return int(size * (size + 1) / 2)


def init_cards(core, file: str = "AszilCompostNetdeck.txt", size: int = 3):
    with open(f"../assets/{file}") as deck_:
        deck = map(str.strip, deck_)
        assert (next(deck) == 'creatures')
        for i, j in triangle_pos(size):
            x = get_id(next(deck))
            core.new_card(x, SUPERTYPE.CREATURE, 0, 0, LOCATION.FIELD, (i, j), 1)
            core.new_card(x, SUPERTYPE.CREATURE, 1, 1, LOCATION.FIELD, (2 * size - i - 1, j), 1)
            core.print_board()

        assert (next(deck) == 'battlegear')
        for _ in range(triangle(size)):
            next(deck)
        assert (next(deck) == 'mugic')
        for _ in range(triangle(size)):
            next(deck)

        assert (next(deck) == 'locations')
        for _ in range(10):
            x = get_id(next(deck))
            core.new_card(x, SUPERTYPE.LOCATION, 0, 0, LOCATION.LOCATION_DECK, 0, 2)
            core.new_card(x, SUPERTYPE.LOCATION, 1, 1, LOCATION.LOCATION_DECK, 0, 2)

        assert (next(deck) == 'attacks')
        for _ in range(20):
            x = get_id(next(deck))
            core.new_card(x, SUPERTYPE.ATTACK, 0, 0, LOCATION.ATTACK_DECK, 0, 2)
            core.new_card(x, SUPERTYPE.ATTACK, 1, 1, LOCATION.ATTACK_DECK, 0, 2)


def main():
    try:
        with chaotic.Chaotic(int(time.time()), 3, cardReader=card_reader, cardReaderPayload=c) as core:
            print(sys.getrefcount(core))
            print(sys.getrefcount(card_reader))
            print(sys.getrefcount(c))
            init_cards(core)
            core.start_duel()

            prompt = None
            while True:
                res = core.process()
                msgs = core.messages()
                if res == 0:
                    break
                if res == 2:
                    for m in msgs:
                        print_message(m)
                    continue

                for m in msgs[0:-1]:
                    print_message(m)
                prompt = prompt if isinstance(msgs[-1], msg.MSG_Retry) else msgs[-1]
                print_message(prompt, False)
                core.print_board()
                player = prompt.player
                if player == 0:
                    print("\033[31m", end='Player 0 Select => ')
                else:
                    print("\033[34m", end='Player 1 Select => ')
                x = input()
                core.respond(*map(int, x.split()))
                print("\033[0m")
    except KeyboardInterrupt:
        print("BYE")
    except Exception as e:
        print(traceback.format_exc())


if __name__ == '__main__':
    main()