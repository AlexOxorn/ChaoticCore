import chaotic
import sqlite3


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
               fire OR -1,
               air OR -1,
               earth OR -1,
               water OR -1,
               mugic,
               loyal,
               limited,
               legendary
        FROM   cards
        WHERE  id = ?""", (code,))
    data = tuple(x or 0 for x in curr.fetchone())
    print(data)
    return data


c = sqlite3.connect('../assets/chaoticdata.db')
g = chaotic.Chaotic(0, 3, cardReader=card_reader, cardReaderPayload=c)
g.new_card(65537, 0, 0, 0, 0b100, (0, 0), 1)
g.new_card(65537, 0, 1, 1, 0b100, (1, 0), 1)
print('-----------------------------------')
g.new_card(65575, 0b100, 0, 0, 0b1, 0, 2)
print('-----------------------------------')
g.new_card(65575, 0b100, 0, 0, 0b1, 0, 2)
print('-----------------------------------')
g.new_card(65575, 0b100, 0, 0, 0b1, 0, 2)



