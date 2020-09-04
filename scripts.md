# The scripting
**NOTE:** Following $xx and $xxxx are addresses in WRAM (absolute address is $7exxxx).\
      $7fxxxx is 2nd WRAM bank, other addresses are ROM (or hardware registers).
 
(See jump tables at 8ce8bd (instructions) and 8cf041 ( ”subinstr” for instr 0x18, 0x08, etc.))\
Script instructions get executed from a pointer at $82 (lda [$82], etc.) at 8cd0af\
(Carry flag is used to decide if another instruction should be read; END clears carry)

When looting a gourd, $82..84 gets loaded from $028fc+0..2.\
$82..84 is a local variable “current script data” pointer and gets set when triggering a script. $028fc, etc. are 0x4f wide “script objects” which includes call stack, argument, data address, owner, ...\
Script object + 0x05..0x07 = “script timer expiration value”: sleep / pause execution

First 1 byte in “current script data” is instruction, parsed in code at 8cd0a6\
Depending on instruction: followed by
- optional data (e.g. write address) according to that instruction
- for some instructions parse one or more sub-instructions + their data
  - Sub-instructions can be chained (if msbit is not set another will be read)
  - This allows for AND, OR, NOT, etc. in branches
  - Some instructions parse a variable number of sub-instr chains
- optional data according to base instruction (like jump offset)

## Script stack / list
7e28fc is a list of active scripts with entry size of 4f\
When opening a gourd, script address gets written to 7e28fc (i.e. put on the execution list)\
When a gourd is calling a “sub script”, $7e28fc is already occupied, $7f294b gets used.\
$7e..f contains pointer to currently active “script object” in ram (so $28fc,+4f, +2*4f, etc.)\

## Looting & Map data
South Forest [$0038]:
- First gourd: runs 93802b then 92a482 then 92a584 then 92b12c then 92a4a6 then 92803e
  - Comes from 928a7f in ROM, encoded like for call, see rom2scriptaddr below
    - Comes from map script data at 9e801d (pointed to by 7e1069)

NPC kill script IDs are attached to entities at +66, with flags indicating it should be run on kill at +68 (see RAM map). The IDs are identical to what the 16bit Map script IDs are (see below). They are mostly set right after spawn by room-enter-script.\
NPC talk scripts work the same way as kill scripts, but use different flags at +68.

$1069 is a pointer to map B script data/table (48bit entries: 32bit position, 16bit script id)\
$1067 is length of that table in bytes -- yes, first time I’ve seen bounds checking in evermore\
Both get set during a room change. (see code at 8face6 for stuff below).\
$0f86 and $0f88 is part of what is used to match the 32bit position.\
When standing in front of a closed gourd, pressing B, *all* scripts get looked at. The position is Y1>>4 + A, X1>>4 + B, Y2>>4 + A, X2>>4 + B (A and B are magic (map offset?) numbers from $0f86 and $0f88, different for each map), X1|Y1 is one, X2|Y2 the opposite corner of the trigger).\
Additional check if “closed gourd exists” is at 8fce43, but points into unknown data. This is the actual object (script triggers are overlaid on top of objects).\
That’s why boy and dog do not attack closed gourds/ingredients, but do attack looted ones.\

928000 is some sort of table for scripts, format not yet known but first 16bits is “header length”\
Map (Gourd) scripts/IDs are run the same way as other “(16bit) indexed scripts”:\
Add value at 928000 (probably header length, = 0x0294) to script id.\
Read from 928000 + result above. Use lowest 15 lsb as-is, shift left highest 8 bits, add the 16th bit as lsb to highest 8 bits. Add 928000 to it to get 24bit address of script data.\
Or read as: add a 25 bit number with bit 0x8000 always set, encoded in 24 bits, to 920000, again see rom2scriptaddr blow\

When changing screen, $8b holds pointer to (some?) map data of next map\
$8b gets set from 9ffde7+x with x=$0adb<<2 (~= map id * 4) at 908f69\
Step-on script section starts at map data + 0x0d + 2 with value in +0x0d being section length in bytes. B-Triggered as above right after step-on.\
Also some map data gets copied to 21xx AND 0f8x (twice in WRAM) See 909019.

Step-on pointer at/copied to 1064..66, length at 1062..63.

Map enter script address gets read from 5*MAP_ID + 92801b. Encoded the same way as other script addresses.

**Side Note:** Dog ghost pickup only works for scripts with WRITE NEXT_AMOUNT_EXTRA = x before checking if pickup will happen.

    static inline uint32_t rom2scriptaddr(uint32_t romaddr)
    {
        romaddr &= ~(0x8000);
        romaddr -= 0x920000;
        return (romaddr&0x007fff) + ((romaddr&0x1ff0000)>>1);
    }
    static inline uint32_t script2romaddr(uint32_t scriptaddr)
    {
        return 0x928000 + (scriptaddr&0x007fff) + ((scriptaddr&0xff8000)<<1);
    }

